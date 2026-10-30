/*****************************************************************************
 * ttml.c : TTML helpers
 *****************************************************************************
 * Copyright (C) 2017 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_xml.h>
#include <vlc_strings.h>
#include <vlc_charset.h>

#include <assert.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>

#include "ttml.h"

#define ALIGN_TEXT N_("Subtitle justification")
#define ALIGN_LONGTEXT N_("Set the justification of subtitles")

/*****************************************************************************
 * Modules descriptor.
 *****************************************************************************/

vlc_module_begin ()
    set_capability( "spu decoder", 10 )
    set_shortname( N_("TTML decoder"))
    set_description( N_("TTML subtitles decoder") )
    set_callback( tt_OpenDecoder )
    set_subcategory( SUBCAT_INPUT_SCODEC )
    add_integer( "ttml-align", 0, ALIGN_TEXT, ALIGN_LONGTEXT )

    add_submodule()
        set_shortname( N_("TTML") )
        set_description( N_("TTML demuxer") )
        set_capability( "demux", 11 )
        set_subcategory( SUBCAT_INPUT_DEMUX )
        set_callbacks( tt_OpenDemux, tt_CloseDemux )
        add_shortcut( "ttml" )
#ifdef ENABLE_SOUT
    add_submodule()
        set_shortname( N_("TTML") )
        set_description( N_("TTML encoder") )
        set_capability( "spu encoder", 101 )
        set_subcategory( SUBCAT_INPUT_SCODEC )
        set_callbacks( tt_OpenEncoder, NULL )
#endif
vlc_module_end ()

struct tt_namespace_s
{
    char *psz_prefix;
    char *psz_uri;
    struct vlc_list links;
};

void tt_namespaces_Clean( tt_namespaces_t *nss )
{
    struct tt_namespace_s *ns;
    vlc_list_foreach( ns, &nss->nodes, links )
    {
        free( ns->psz_prefix );
        free( ns->psz_uri );
        free( ns );
    }
}

void tt_namespaces_Init( tt_namespaces_t *nss )
{
    vlc_list_init( &nss->nodes );
}

const char * tt_namespaces_GetURI( const tt_namespaces_t *nss,
                                   const char *psz_qn )
{
    const struct tt_namespace_s *ns;
    vlc_list_foreach_const( ns, &nss->nodes, links )
    {
        /* compares prefixed name against raw prefix */
        for( size_t i=0; ; i++ )
        {
            if( ns->psz_prefix[i] == psz_qn[i] )
            {
                if( psz_qn[i] == '\0' )
                    return ns->psz_uri;
            }
            else
            {
                if( ns->psz_prefix[i] == '\0' && psz_qn[i] == ':' )
                    return ns->psz_uri;
                else
                    break;
            }
        }
    }
    return NULL;
}

const char * tt_namespaces_GetPrefix( const tt_namespaces_t *nss,
                                      const char *psz_uri )
{
    const struct tt_namespace_s *ns;
    vlc_list_foreach_const( ns, &nss->nodes, links )
    {
        if( !strcmp( ns->psz_uri, psz_uri ) )
            return ns->psz_prefix;
    }
    return NULL;
}

void tt_namespaces_Register( tt_namespaces_t *nss, const char *psz_prefix,
                             const char *psz_uri )
{
    if( !psz_uri || tt_namespaces_GetPrefix( nss, psz_uri ) )
        return;
    struct tt_namespace_s *ns = malloc(sizeof(*ns));
    if( ns )
    {
        const char *sep = strchr( psz_prefix, ':' );
        if( sep )
            ns->psz_prefix = strndup( psz_prefix, sep - psz_prefix );
        else
            ns->psz_prefix = strdup("");
        ns->psz_uri = strdup( psz_uri );
        if( !ns->psz_prefix || !ns->psz_uri )
        {
            free( ns->psz_prefix );
            free( ns->psz_uri );
            free( ns );
            return;
        }
        vlc_list_append( &ns->links, &nss->nodes );
    }
}

static const char * tt_node_InheritNS( const tt_node_t *p_node )
{
    for( ; p_node ; p_node = p_node->p_parent )
    {
        if( p_node->psz_namespace )
            return p_node->psz_namespace;
    }
    return NULL;
}

bool tt_node_Match( const tt_node_t *p_node, const char *psz_name, const char *psz_namespace )
{
    /* compare local part first (should have less chars) */
    const char *psz_nodelocal = tt_LocalName( p_node->psz_node_name );
    const char *psz_namelocal = tt_LocalName( psz_name );
    if( strcmp( psz_namelocal, psz_nodelocal ) )
        return false;

    const char *psz_nodens = p_node->psz_namespace;
    if( !psz_nodens )
        psz_nodens = tt_node_InheritNS( p_node->p_parent );
    if( psz_namespace && psz_nodens )
        return !strcmp( psz_namespace, psz_nodens );
    return !!psz_namespace == !!psz_nodens;
}

const char * tt_node_GetAttribute( tt_namespaces_t *p_nss, const tt_node_t *p_node,
                                   const char *psz_name, const char *psz_namespace )
{
    const void *value;
    char *alloc = NULL;
    if( psz_namespace )
    {
        const char *psz_prefix = tt_namespaces_GetPrefix( p_nss, psz_namespace );
        if( psz_prefix == NULL ||
            asprintf( &alloc, "%s:%s", psz_prefix, psz_name ) < 1 )
            return NULL;
        psz_name = alloc;
    }
    value = vlc_dictionary_value_for_key( &p_node->attr_dict, psz_name );
    free( alloc );
    return value != kVLCDictionaryNotFound ? (const char *)value : NULL;
}

bool tt_node_HasChild( const tt_node_t *p_node )
{
    return p_node->p_child;
}

static bool tt_ReadInteger( const char **cursor, uint64_t *value )
{
    const char *p = *cursor;
    if( *p < '0' || *p > '9' )
        return false;
    uint64_t result = 0;
    do
    {
        const unsigned digit = *p - '0';
        if( result > (UINT64_MAX - digit) / 10 )
            return false;
        result = result * 10 + digit;
        p++;
    } while( *p >= '0' && *p <= '9' );
    *cursor = p;
    *value = result;
    return true;
}

static bool tt_ParameterInteger( const char *text, uint64_t *value )
{
    return tt_ReadInteger( &text, value ) && *value > 0 && *text == '\0';
}

static bool tt_ReadParameters( tt_time_parameters_t *parameters,
                                const char *frame_rate, const char *multiplier,
                                const char *sub_frame_rate, const char *tick_rate )
{
    uint64_t frames = TT_FRAME_RATE, subframes = 1, numerator = 1, denominator = 1;
    uint64_t ticks;
    if( (frame_rate && !tt_ParameterInteger( frame_rate, &frames )) ||
        (sub_frame_rate && !tt_ParameterInteger( sub_frame_rate, &subframes )) )
        return false;
    if( multiplier )
    {
        if( !tt_ReadInteger( &multiplier, &numerator ) || numerator == 0 ||
            !strchr( " \t\r\n", *multiplier ) || *multiplier == '\0' )
            return false;
        do multiplier++; while( *multiplier && strchr( " \t\r\n", *multiplier ) );
        if( !tt_ParameterInteger( multiplier, &denominator ) )
            return false;
    }
    parameters->nominal_frame_rate = frames;
    parameters->sub_frame_rate = subframes;
    parameters->frame_rate = (long double) frames * numerator / denominator;
    if( tick_rate )
    {
        if( !tt_ParameterInteger( tick_rate, &ticks ) )
            return false;
        parameters->tick_rate = ticks;
    }
    else
    {
        parameters->tick_rate = frame_rate ? parameters->frame_rate * subframes : 1;
    }
    return isfinite( parameters->frame_rate ) && parameters->frame_rate > 0 &&
           isfinite( parameters->tick_rate ) && parameters->tick_rate > 0;
}

static bool tt_ReadFraction( const char **cursor, long double *value )
{
    const char *p = *cursor;
    if( *p++ != '.' || *p < '0' || *p > '9' )
        return false;
    long double fraction = 0, place = .1L;
    do
    {
        fraction += (*p++ - '0') * place;
        place /= 10;
    } while( *p >= '0' && *p <= '9' );
    *cursor = p;
    *value = fraction;
    return true;
}

static bool tt_ReadTwoDigits( const char **cursor, unsigned *value )
{
    const char *p = *cursor;
    if( p[0] < '0' || p[0] > '9' || p[1] < '0' || p[1] > '9' )
        return false;
    *value = (p[0] - '0') * 10 + p[1] - '0';
    *cursor = p + 2;
    return true;
}

static bool tt_ParseTime( const char *s, const tt_time_parameters_t *parameters,
                          tt_time_t *result )
{
    const char *first = s;
    uint64_t whole;
    if( !tt_ReadInteger( &s, &whole ) )
        return false;
    long double seconds;
    if( *s == ':' ) /* Clock time */
    {
        unsigned minutes, second;
        if( s - first < 2 )
            return false;
        s++;
        if( !tt_ReadTwoDigits( &s, &minutes ) || *s++ != ':' ||
            !tt_ReadTwoDigits( &s, &second ) || minutes >= 60 || second > 60 )
            return false;
        /* The deprecated leap-second value is interpreted as 59 in media time. */
        seconds = (long double) whole * 3600 + minutes * 60 + (second == 60 ? 59 : second);
        if( *s == '.' )
        {
            long double fraction;
            if( !tt_ReadFraction( &s, &fraction ) || (second == 60 && fraction > 0) )
                return false;
            seconds += fraction;
        }
        else if( *s == ':' )
        {
            uint64_t frames, subframes = 0;
            first = ++s;
            if( !tt_ReadInteger( &s, &frames ) || s - first < 2 ||
                frames >= parameters->nominal_frame_rate )
                return false;
            if( *s == '.' )
            {
                s++;
                if( !tt_ReadInteger( &s, &subframes ) || subframes >= parameters->sub_frame_rate )
                    return false;
            }
            seconds += (frames + (long double) subframes / parameters->sub_frame_rate)
                       / parameters->frame_rate;
        }
        if( *s != '\0' )
            return false;
    }
    else /* Offset time: no exponents, signs, whitespace or partial unit names. */
    {
        long double value = whole;
        if( *s == '.' )
        {
            long double fraction;
            if( !tt_ReadFraction( &s, &fraction ) )
                return false;
            value += fraction;
        }
        if( !strcmp( s, "h" ) ) seconds = value * 3600;
        else if( !strcmp( s, "m" ) ) seconds = value * 60;
        else if( !strcmp( s, "s" ) ) seconds = value;
        else if( !strcmp( s, "ms" ) ) seconds = value / 1000;
        else if( !strcmp( s, "f" ) ) seconds = value / parameters->frame_rate;
        else if( !strcmp( s, "t" ) ) seconds = value / parameters->tick_rate;
        else return false;
    }
    const long double ticks = seconds * CLOCK_FREQ;
    if( !isfinite( ticks ) || ticks < 0 ||
        ticks > (long double) (VLC_TICK_MAX - VLC_TICK_0) - .5L )
        return false;
    *result = tt_time_Create( (vlc_tick_t) (ticks + .5L) );
    return true;
}

bool tt_timings_Contains( const tt_timings_t *p_range, const tt_time_t *time )
{
    if( tt_time_Valid( &p_range->end ) &&
        tt_time_Compare( &p_range->end, time ) <= 0 )
        return false;

    if( tt_time_Valid( &p_range->begin ) &&
        tt_time_Compare( &p_range->begin, time ) > 0 )
        return false;

    return true;
}

static void tt_textnode_Delete( tt_textnode_t *p_node )
{
    free( p_node->psz_text );
    free( p_node );
}

static void tt_node_FreeDictValue( void* p_value, void* p_obj )
{
    VLC_UNUSED( p_obj );
    free( p_value );
}

static void tt_node_Delete( tt_node_t *p_node )
{
    free( p_node->psz_node_name );
    free( p_node->psz_namespace );
    vlc_dictionary_clear( &p_node->attr_dict, tt_node_FreeDictValue, NULL );
    free( p_node );
}

void tt_node_RecursiveDelete( tt_node_t *p_node )
{
    for( ; p_node->p_child ; )
    {
        tt_basenode_t *p_child = p_node->p_child;
        p_node->p_child = p_child->p_next;

        if( p_child->i_type == TT_NODE_TYPE_TEXT )
            tt_textnode_Delete( (tt_textnode_t *) p_child );
        else
            tt_node_RecursiveDelete( (tt_node_t *) p_child );
    }
    tt_node_Delete( p_node );
}

void tt_node_RemoveAttribute( tt_node_t *p_node, const char *key )
{
    vlc_dictionary_remove_value_for_key( &p_node->attr_dict, key,
                                        tt_node_FreeDictValue, NULL );
}

int tt_node_AddAttribute( tt_node_t *p_node, const char *key, const char *value )
{
    char *p_dup = strdup( value );
    if( p_dup )
        vlc_dictionary_insert( &p_node->attr_dict, key, p_dup );
    return p_dup ? VLC_SUCCESS : VLC_EGENERIC;
}

static void tt_node_ParentAddChild( tt_node_t* p_parent, tt_basenode_t *p_child )
{
    tt_basenode_t **pp_node = &p_parent->p_child;
    while( *pp_node != NULL )
        pp_node = &((*pp_node)->p_next);
    *pp_node = p_child;
}

static tt_textnode_t *tt_textnode_NewImpl( tt_node_t *p_parent, char *psz )
{
    if( !psz )
        return NULL;
    tt_textnode_t *p_node = calloc( 1, sizeof( *p_node ) );
    if( !p_node )
    {
        free( psz );
        return NULL;
    }
    p_node->psz_text = psz;
    p_node->i_type = TT_NODE_TYPE_TEXT;
    p_node->p_parent = p_parent;
    if( p_parent )
        tt_node_ParentAddChild( p_parent, (tt_basenode_t *) p_node );
    return p_node;
}

tt_textnode_t *tt_textnode_New( tt_node_t *p_parent, const char *psz_text )
{
    return tt_textnode_NewImpl( p_parent, strdup( psz_text ) );
}

tt_textnode_t *tt_subtextnode_New( tt_node_t *p_parent, const char *psz_text, size_t len )
{
    return tt_textnode_NewImpl( p_parent, strndup( psz_text, len ) );
}

static tt_node_t *tt_node_Alloc( tt_node_t* p_parent,
                                 const char* psz_node_name,
                                 const char *psz_namespace )
{
    tt_node_t *p_node = calloc( 1, sizeof( *p_node ) );
    if( !p_node )
        return NULL;

    p_node->i_type = TT_NODE_TYPE_ELEMENT;
    p_node->psz_node_name = strdup( psz_node_name );
    const char *psz_parent_ns = tt_node_InheritNS( p_parent );
    /* set new namespace if not same as parent */
    if( psz_namespace &&
        (!psz_parent_ns || strcmp( psz_namespace, psz_parent_ns )) )
        p_node->psz_namespace = strdup( psz_namespace );
    else
        p_node->psz_namespace = NULL;
    if( unlikely( p_node->psz_node_name == NULL ) )
    {
        free( p_node );
        return NULL;
    }
    vlc_dictionary_init( &p_node->attr_dict, 0 );
    tt_time_Init( &p_node->timings.begin );
    tt_time_Init( &p_node->timings.end );
    tt_time_Init( &p_node->timings.dur );
    p_node->p_parent = p_parent;
    p_node->time_parameters = p_parent ? p_parent->time_parameters :
        (tt_time_parameters_t) { .frame_rate = TT_FRAME_RATE, .tick_rate = 1,
                                .nominal_frame_rate = TT_FRAME_RATE, .sub_frame_rate = 1 };

    return p_node;
}

tt_node_t * tt_node_New( tt_node_t* p_parent, const char* psz_node_name,
                         const char *psz_namespace )
{
    tt_node_t *node = tt_node_Alloc( p_parent, psz_node_name, psz_namespace );
    if( node && p_parent )
        tt_node_ParentAddChild( p_parent, (tt_basenode_t *) node );
    return node;
}

tt_node_t * tt_node_NewRead( xml_reader_t* reader,
                             tt_namespaces_t *p_nss, tt_node_t* p_parent,
                             const char* psz_node_name, const char *psz_namespace )
{
    /* Link only after validation, so a failed child cannot leave a dangling
     * entry in its parent's list during parser cleanup. */
    tt_node_t *p_node = tt_node_Alloc( p_parent, psz_node_name, psz_namespace );
    if( !p_node )
        return NULL;

    const char* psz_value = NULL, *psz_ns = NULL;
    const char *frame_rate = NULL, *multiplier = NULL, *sub_frame_rate = NULL, *tick_rate = NULL;
    const char *begin = NULL, *end = NULL, *duration = NULL;
    for( const char* psz_key = xml_ReaderNextAttrNS( reader, &psz_value, &psz_ns );
         psz_key != NULL;
         psz_key = xml_ReaderNextAttrNS( reader, &psz_value, &psz_ns ) )
    {
        if( psz_ns && psz_key )
        tt_namespaces_Register( p_nss, psz_key, psz_ns );
        char *psz_val = strdup( psz_value );
        if( !psz_val )
            goto error;
        {
            vlc_dictionary_insert( &p_node->attr_dict, psz_key, psz_val );

            if( !strcasecmp( psz_key, "begin" ) )
                begin = psz_val;
            else if( ! strcasecmp( psz_key, "end" ) )
                end = psz_val;
            else if( ! strcasecmp( psz_key, "dur" ) )
                duration = psz_val;
            else if( ! strcasecmp( psz_key, "timeContainer" ) )
                p_node->timings.i_type = strcmp( psz_val, "seq" ) ? TT_TIMINGS_PARALLEL
                                                                  : TT_TIMINGS_SEQUENTIAL;
            if( !p_parent && psz_ns && !strcmp( psz_ns, TT_NS_PARAMETER ) )
            {
                const char *name = tt_LocalName( psz_key );
                if( !strcmp( name, "frameRate" ) ) frame_rate = psz_val;
                else if( !strcmp( name, "frameRateMultiplier" ) ) multiplier = psz_val;
                else if( !strcmp( name, "subFrameRate" ) ) sub_frame_rate = psz_val;
                else if( !strcmp( name, "tickRate" ) ) tick_rate = psz_val;
            }
        }
    }
    /* Collect the full root declaration first; XML attribute order is irrelevant.
     * Namespace identity comes from the reader, including alternate prefixes. */
    if( (!p_parent && !tt_ReadParameters( &p_node->time_parameters, frame_rate, multiplier,
                                         sub_frame_rate, tick_rate )) ||
        (begin && !tt_ParseTime( begin, &p_node->time_parameters, &p_node->timings.begin )) ||
        (end && !tt_ParseTime( end, &p_node->time_parameters, &p_node->timings.end )) ||
        (duration && !tt_ParseTime( duration, &p_node->time_parameters, &p_node->timings.dur )) )
        goto error;
    if( p_parent )
        tt_node_ParentAddChild( p_parent, (tt_basenode_t *) p_node );
    return p_node;
error:
    tt_node_RecursiveDelete( p_node );
    return NULL;
}
#if 0
static int tt_node_Skip( xml_reader_t *p_reader, const char *psz_skipped )
{
    size_t i_depth = 1;
    const char *psz_cur;

    /* need a copy as psz_skipped would point to next node after NextNode */
    char *psz_skip = strdup( psz_skipped );
    if(!psz_skip)
        return VLC_EGENERIC;

    for( ;; )
    {
        int i_type = xml_ReaderNextNode( p_reader, &psz_cur );
        switch( i_type )
        {
            case XML_READER_STARTELEM:
                if( i_depth == SIZE_MAX )
                {
                    free( psz_skip );
                    return VLC_EGENERIC;
                }
                if( !xml_ReaderIsEmptyElement( p_reader ) )
                    i_depth++;
                break;

            case XML_READER_ENDELEM:
                if( !strcmp( psz_cur, psz_skip ) )
                {
                    free( psz_skip );
                    if( i_depth != 1 )
                        return VLC_EGENERIC;
                    return VLC_SUCCESS;
                }
                else
                {
                    if( i_depth == 1 )
                    {
                        free( psz_skip );
                        return VLC_EGENERIC;
                    }
                    i_depth--;
                }
                break;

            default:
                if( i_type <= XML_READER_NONE )
                {
                    free( psz_skip );
                    return VLC_EGENERIC;
                }
                break;
        }
    }
    vlc_assert_unreachable();
    return VLC_EGENERIC;
}
#endif
int tt_nodes_Read( xml_reader_t *p_reader, tt_namespaces_t *p_nss, tt_node_t *p_root_node )
{
    size_t i_depth = 0;
    tt_node_t *p_node = p_root_node;

    do
    {
        const char *psz_node_name, *psz_node_namespace;
        int i_type = xml_ReaderNextNodeNS( p_reader, &psz_node_name, &psz_node_namespace );
        /* !warn read empty state now as attributes reading will **** it up */
        bool b_empty = xml_ReaderIsEmptyElement( p_reader );

        if( i_type <= XML_READER_NONE )
            break;

        switch( i_type )
        {
            default:
                break;

            case XML_READER_STARTELEM:
            {
                tt_namespaces_Register( p_nss, psz_node_name, psz_node_namespace );
                tt_node_t *p_newnode = tt_node_NewRead( p_reader, p_nss, p_node,
                                                        psz_node_name,
                                                        psz_node_namespace );
                if( !p_newnode )
                    return VLC_EGENERIC;
                if( !b_empty )
                {
                    p_node = p_newnode;
                    i_depth++;
                }
                break;
            }

            case XML_READER_TEXT:
            {
                tt_textnode_t *p_textnode = tt_textnode_New( p_node, psz_node_name );
                VLC_UNUSED(p_textnode);
            }
            break;

            case XML_READER_ENDELEM:
            {
                if( !tt_node_Match( p_node, psz_node_name, psz_node_namespace ) )
                    return VLC_EGENERIC;

                if( i_depth == 0 )
                {
                    if( p_node != p_root_node )
                        return VLC_EGENERIC;
                    break; /* END */
                }
                i_depth--;
                p_node = p_node->p_parent;
                break;
            }
        }
    } while( 1 );

    return VLC_SUCCESS;
}


/* Timings storage */
static int tt_bsearch_searchkey_Compare( const void *key, const void *other )
{
    struct tt_searchkey *p_key = (struct tt_searchkey *) key;
    tt_time_t time = *((tt_time_t *) other);
    p_key->p_last = (tt_time_t *) other;
    return tt_time_Compare( &p_key->time, &time );
}

size_t tt_timings_FindLowerIndex( const tt_time_t *p_times, size_t i_times, tt_time_t time, bool *pb_found )
{
    size_t i_index = 0;
    if( p_times )
    {
        struct tt_searchkey key;
        key.time = time;
        key.p_last = NULL;

        const tt_time_t *lookup = bsearch(&key, p_times, i_times,
                                          sizeof(tt_time_t), tt_bsearch_searchkey_Compare);
        if( lookup )
            key.p_last = lookup;
        *pb_found = !!lookup;

        /* Compute index from last visited */
        i_index = (key.p_last - p_times);
        if( tt_time_Compare( &p_times[i_index], &time ) < 0 )
            i_index++;
    }
    else *pb_found = false;
    return i_index;
}

static void tt_bsearch_Insert( tt_time_t **pp_times, size_t *pi_times, tt_time_t time )
{
    bool b_exists;
    size_t i_index = tt_timings_FindLowerIndex( *pp_times, *pi_times, time, &b_exists );
    if( b_exists )
        return;

    if( SIZE_MAX / sizeof(tt_time_t) < (*pi_times + 1) )
        return;

    tt_time_t *p_array = realloc( *pp_times, (*pi_times + 1) * sizeof(tt_time_t) );
    if( !p_array )
        return;
    *pp_times = p_array;

    if( *pi_times > 0 )
    {
        memmove( &p_array[i_index + 1],
                 &p_array[i_index],
                 (*pi_times - i_index) * sizeof(tt_time_t) );
    }

    p_array[i_index] = time;
    *pi_times += 1;
}


/* Timings storage */
static void tt_timings_MergeParallel( const tt_timings_t *p_ref, tt_timings_t *p_local )
{
    if( tt_time_Valid( &p_local->begin ) )
        p_local->begin = tt_time_Add( p_local->begin, p_ref->begin );
    else
        p_local->begin = p_ref->begin;

    if( tt_time_Valid( &p_local->end ) )
    {
        p_local->end = tt_time_Add( p_local->end, p_ref->begin );
    }
    else if( tt_time_Valid( &p_local->dur ) && tt_time_Valid( &p_local->begin ) )
    {
        p_local->end = tt_time_Add( p_local->begin, p_local->dur );
    }
    else p_local->end = p_ref->end;

    /* Enforce contained duration */

    if( tt_time_Valid( &p_ref->end ) && tt_time_Compare( &p_local->end, &p_ref->end ) > 0 )
        p_local->end = p_ref->end;

    /* Just for consistency */
    if( tt_time_Valid( &p_local->begin ) && tt_time_Valid( &p_local->end ) )
        p_local->dur = tt_time_Sub( p_local->end, p_local->begin );
}

static void tt_timings_MergeSequential( const tt_timings_t *p_restrict,
                                       const tt_timings_t *p_prevref, tt_timings_t *p_local )
{
    if( tt_time_Valid( &p_local->begin ) )
        p_local->begin = tt_time_Add( p_local->begin, p_prevref->end );
    else
        p_local->begin = p_prevref->end;

    if( tt_time_Valid( &p_local->end ) )
    {
        p_local->end = tt_time_Add( p_local->end, p_prevref->end );
    }
    else if( tt_time_Valid( &p_local->dur ) && tt_time_Valid( &p_local->begin ) )
    {
        p_local->end = tt_time_Add( p_local->begin, p_local->dur );
    }

    /* Enforce contained duration */
    if( tt_time_Valid( &p_restrict->end ) && tt_time_Compare( &p_local->end, &p_restrict->end ) > 0 )
        p_local->end = p_restrict->end;

    /* Just for consistency */
    if( tt_time_Valid( &p_local->begin ) && tt_time_Valid( &p_local->end ) )
        p_local->dur = tt_time_Sub( p_local->end, p_local->begin );
}

void tt_timings_Resolve( tt_basenode_t *p_child, const tt_timings_t *p_container_timings,
                         tt_time_t **pp_array, size_t *pi_count )
{
    const tt_node_t *p_prevnode = NULL;
    for(  ; p_child; p_child = p_child->p_next )
    {
        if( p_child->i_type != TT_NODE_TYPE_ELEMENT )
            continue;

        tt_node_t *p_childnode = (tt_node_t *) p_child;
        if( p_container_timings->i_type == TT_TIMINGS_SEQUENTIAL )
        {
            if( p_prevnode == NULL ) /* First */
                tt_timings_MergeParallel( p_container_timings, &p_childnode->timings );
            else
                tt_timings_MergeSequential( p_container_timings,
                                        &p_prevnode->timings, &p_childnode->timings );
        }
        else
        {
            tt_timings_MergeParallel( p_container_timings, &p_childnode->timings );
        }

        if( tt_time_Valid( &p_childnode->timings.begin ) )
            tt_bsearch_Insert( pp_array, pi_count, p_childnode->timings.begin );

        if( tt_time_Valid( &p_childnode->timings.end ) )
            tt_bsearch_Insert( pp_array, pi_count, p_childnode->timings.end );

        p_prevnode = p_childnode;

        tt_timings_Resolve( p_childnode->p_child, &p_childnode->timings,
                            pp_array, pi_count );
    }
}
