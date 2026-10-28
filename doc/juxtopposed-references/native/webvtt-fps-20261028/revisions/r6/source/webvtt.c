/*****************************************************************************
 * webvtt.c: WEBVTT text demuxer (as ISO1446-30 payload)
 *****************************************************************************
 * Copyright (C) 2017 VideoLabs, VLC authors and VideoLAN
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

/*****************************************************************************
 * Preamble
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_demux.h>
#include <vlc_memstream.h>
#include <assert.h>
#include <math.h>

#include "../codec/webvtt/webvtt.h"

/*****************************************************************************
 * Prototypes:
 *****************************************************************************/

struct index_entry_s
{
    vlc_tick_t time;
    unsigned active;
};

typedef struct
{
    es_out_id_t *es;
    bool         b_slave;
    bool         b_first_time;
    int          i_next_block_flags;
    vlc_tick_t   i_next_demux_time;
    vlc_tick_t   i_length;
    float        f_rate;
    struct
    {
        void    *p_data;
        size_t   i_data;
    } regions_headers, styles_headers;

    struct
    {
        webvtt_cue_t *p_array;
        size_t  i_alloc;
        size_t  i_count;
    } cues;

    struct
    {
        struct index_entry_s *p_array;
        size_t   i_alloc;
        size_t   i_count;
        size_t   i_current;
    } index;

    webvtt_text_parser_t *p_streamparser;
} demux_sys_t;

#define WEBVTT_PREALLOC 64

/*****************************************************************************
 *
 *****************************************************************************/
static int cue_Compare( const void *a_, const void *b_ )
{
    const webvtt_cue_t *a = a_;
    const webvtt_cue_t *b = b_;
    if( a->i_start == b->i_start )
    {
        if( a->i_stop > b->i_stop )
            return -1;
        else
            return ( a->i_stop < b->i_stop ) ? 1 : 0;
    }
    else return a->i_start < b->i_start ? -1 : 1;
}

/* Source cue/index times stay immutable. Only emitted presentation times change. */
static bool ScaleTime( vlc_tick_t source, float rate, vlc_tick_t *result )
{
    if( source < 0 || !isfinite( rate ) || rate <= 0 )
        return false;
    const long double scaled = (long double) source * rate;
    if( scaled > (long double) (VLC_TICK_MAX - VLC_TICK_0) - 0.5L )
        return false;
    *result = (vlc_tick_t) (scaled + 0.5L);
    return true;
}

/* Unlike the cue-line scanner, an inline tag must contain a complete timestamp. */
static bool ParseTimeTag( const char *begin, const char *end, vlc_tick_t *time )
{
    const size_t length = end - begin;
    if( length != 9 && length < 12 )
        return false;
    const char *dot = end - 4;
    if( *dot != '.' || dot[-3] != ':' )
        return false;
    const char *minutes = dot - 5;
    uint64_t hours = 0;
    if( minutes != begin )
    {
        const char *separator = minutes - 1;
        if( *separator != ':' || separator - begin < 2 )
            return false;
        for( const char *p = begin; p < separator; ++p )
        {
            if( *p < '0' || *p > '9' || hours > (UINT64_MAX - 9) / 10 )
                return false;
            hours = hours * 10 + (*p - '0');
        }
    }
    for( const char *p = minutes; p < end; ++p )
        if( p != dot && p != dot - 3 && (*p < '0' || *p > '9') )
            return false;
    unsigned m = (minutes[0] - '0') * 10 + minutes[1] - '0';
    unsigned s = (dot[-2] - '0') * 10 + dot[-1] - '0';
    unsigned ms = (dot[1] - '0') * 100 + (dot[2] - '0') * 10 + dot[3] - '0';
    if( m > 59 || s > 59 || hours > (uint64_t) VLC_TICK_MAX / CLOCK_FREQ / 3600 )
        return false;
    uint64_t seconds = hours * 3600 + m * 60 + s;
    if( seconds > ((uint64_t) VLC_TICK_MAX - VLC_TICK_FROM_MS(ms)) / CLOCK_FREQ )
        return false;
    *time = seconds * CLOCK_FREQ + VLC_TICK_FROM_MS(ms);
    return true;
}

/* A NULL stream validates all genuine tags before accepting a rate change. */
static bool TransformText( const char *text, float rate, struct vlc_memstream *out )
{
    const char *cursor = text;
    while( *cursor )
    {
        const char *tag = strchr( cursor, '<' );
        if( tag == NULL )
        {
            if( out ) vlc_memstream_puts( out, cursor );
            break;
        }
        const char *end = strchr( tag + 1, '>' );
        if( end == NULL )
        {
            if( out ) vlc_memstream_puts( out, cursor );
            break;
        }
        if( out ) vlc_memstream_write( out, cursor, tag - cursor );
        vlc_tick_t source, scaled;
        if( ParseTimeTag( tag + 1, end, &source ) )
        {
            if( !ScaleTime( source, rate, &scaled ) )
                return false;
            if( out )
            {
                uint64_t ms = scaled / VLC_TICK_FROM_MS(1);
                if( scaled % VLC_TICK_FROM_MS(1) >= VLC_TICK_FROM_MS(1) / 2 ) ms++;
                vlc_memstream_printf( out, "<%02"PRIu64":%02u:%02u.%03u>",
                    ms / 3600000, (unsigned) (ms / 60000 % 60),
                    (unsigned) (ms / 1000 % 60), (unsigned) (ms % 1000) );
            }
        }
        else if( out ) vlc_memstream_write( out, tag, end - tag + 1 );
        cursor = end + 1;
    }
    return true;
}

static block_t *ConvertWEBVTT( const webvtt_cue_t *p_cue, bool b_continued, float rate )
{
    struct vlc_memstream stream;
    struct vlc_memstream text;
    char *converted = NULL;
    const char *payload = p_cue->psz_text;
    if( rate != 1.f )
    {
        if( vlc_memstream_open( &text ) )
            return NULL;
        const bool valid = TransformText( payload, rate, &text );
        vlc_memstream_putc( &text, '\0' );
        if( vlc_memstream_close( &text ) )
            return NULL;
        if( !valid )
        {
            free( text.ptr );
            return NULL;
        }
        payload = converted = text.ptr;
    }

    if( vlc_memstream_open( &stream ) )
    {
        free( converted );
        return NULL;
    }

    const size_t paylsize = 8 + strlen( payload );
    const size_t idensize = (p_cue->psz_id) ? 8 + strlen( p_cue->psz_id ) : 0;
    const size_t attrsize = (p_cue->psz_attrs) ? 8 + strlen( p_cue->psz_attrs ) : 0;
    const size_t vttcsize = 8 + paylsize + attrsize + idensize;
    if( vttcsize > UINT32_MAX )
    {
        free( converted );
        vlc_memstream_close( &stream );
        free( stream.ptr );
        return NULL;
    }

    uint8_t vttcbox[8] = { 0, 0, 0, 0, 'v', 't', 't', 'c' };
    if( b_continued )
        vttcbox[7] = 'x';
    SetDWBE( vttcbox, vttcsize );
    vlc_memstream_write( &stream, vttcbox, 8 );

    if( p_cue->psz_id )
    {
        uint8_t idenbox[8] = { 0, 0, 0, 0, 'i', 'd', 'e', 'n' };
        SetDWBE( idenbox, idensize );
        vlc_memstream_write( &stream, idenbox, 8 );
        vlc_memstream_write( &stream, p_cue->psz_id, idensize - 8 );
    }

    if( p_cue->psz_attrs )
    {
        uint8_t attrbox[8] = { 0, 0, 0, 0, 's', 't', 't', 'g' };
        SetDWBE( attrbox, attrsize );
        vlc_memstream_write( &stream, attrbox, 8 );
        vlc_memstream_write( &stream, p_cue->psz_attrs, attrsize - 8 );
    }

    uint8_t paylbox[8] = { 0, 0, 0, 0, 'p', 'a', 'y', 'l' };
    SetDWBE( paylbox, paylsize );
    vlc_memstream_write( &stream, paylbox, 8 );
    vlc_memstream_write( &stream, payload, paylsize - 8 );
    free( converted );

    if( vlc_memstream_close( &stream ) == 0 )
        return block_heap_Alloc( stream.ptr, stream.length );
    return NULL;
}

struct memstream_wrap
{
    struct vlc_memstream memstream;
    bool b_opened;
};

static void memstream_Append( struct memstream_wrap *mw, const char *psz )
{
    if( mw->b_opened )
    {
        vlc_memstream_puts( &mw->memstream, psz );
        vlc_memstream_putc( &mw->memstream, '\n' );
    }
}

static void memstream_Grab( struct memstream_wrap *mw, void **pp, size_t *pi )
{
    if( mw->b_opened && vlc_memstream_close( &mw->memstream ) == 0 )
    {
        *pp = mw->memstream.ptr;
        *pi = mw->memstream.length;
    }
}

/*****************************************************************************
 * Seekable demux Open() parser callbacks
 *****************************************************************************/
struct callback_ctx
{
    demux_t *p_demux;
    struct memstream_wrap regions, styles;
    bool b_ordered;
};

static webvtt_cue_t * ParserGetCueHandler( void *priv )
{
    struct callback_ctx *ctx = (struct callback_ctx *) priv;
    demux_sys_t *p_sys = ctx->p_demux->p_sys;
    /* invalid recycled cue */
    if( p_sys->cues.i_count &&
        p_sys->cues.p_array[p_sys->cues.i_count - 1].psz_text == NULL )
    {
        return &p_sys->cues.p_array[p_sys->cues.i_count - 1];
    }

    if( p_sys->cues.i_alloc <= p_sys->cues.i_count &&
       (SIZE_MAX / sizeof(webvtt_cue_t)) - WEBVTT_PREALLOC > p_sys->cues.i_alloc )
    {
        webvtt_cue_t *p_realloc = realloc( p_sys->cues.p_array,
                sizeof(webvtt_cue_t) * ( p_sys->cues.i_alloc + WEBVTT_PREALLOC ) );
        if( p_realloc )
        {
            p_sys->cues.p_array = p_realloc;
            p_sys->cues.i_alloc += WEBVTT_PREALLOC;
        }
    }

    if( p_sys->cues.i_alloc > p_sys->cues.i_count )
        return &p_sys->cues.p_array[p_sys->cues.i_count++];

    return NULL;
}

static void ParserCueDoneHandler( void *priv, webvtt_cue_t *p_cue )
{
    struct callback_ctx *ctx = (struct callback_ctx *) priv;
    demux_sys_t *p_sys = ctx->p_demux->p_sys;
    if( p_cue->psz_text == NULL )
    {
        webvtt_cue_Clean( p_cue );
        webvtt_cue_Init( p_cue );
        return;
    }
    if( p_cue->i_stop > p_sys->i_length )
        p_sys->i_length = p_cue->i_stop;
    if( p_sys->cues.i_count > 0 &&
        p_sys->cues.p_array[p_sys->cues.i_count - 1].i_start != p_cue->i_start )
        ctx->b_ordered = false;

    /* Store timings */
    if( p_sys->index.i_alloc <= p_sys->index.i_count &&
       (SIZE_MAX / sizeof(struct index_entry_s)) - WEBVTT_PREALLOC * 2 > p_sys->index.i_alloc )
    {
        void *p_realloc = realloc( p_sys->index.p_array,
                                   sizeof(struct index_entry_s) *
                                   ( p_sys->index.i_alloc + WEBVTT_PREALLOC * 2 ) );
        if( p_realloc )
        {
            p_sys->index.p_array = p_realloc;
            p_sys->index.i_alloc += WEBVTT_PREALLOC * 2;
        }
    }
    if( p_sys->index.i_alloc > p_sys->index.i_count )
    {
        p_sys->index.p_array[p_sys->index.i_count].active = 1; /* tmp start tag */
        p_sys->index.p_array[p_sys->index.i_count++].time = p_cue->i_start;
        p_sys->index.p_array[p_sys->index.i_count].active = 0;
        p_sys->index.p_array[p_sys->index.i_count++].time = p_cue->i_stop;
    }
}

static void ParserHeaderHandler( void *priv, enum webvtt_header_line_e s,
                                 bool b_new, const char *psz_line )
{
    VLC_UNUSED(b_new);
    struct callback_ctx *ctx = (struct callback_ctx *) priv;
    if( s == WEBVTT_HEADER_STYLE )
        memstream_Append( &ctx->styles, psz_line );
    else if( s == WEBVTT_HEADER_REGION )
        memstream_Append( &ctx->regions, psz_line );
}

/*****************************************************************************
 * Streamed cues DemuxStream() parser callbacks
 *****************************************************************************/

static webvtt_cue_t * StreamParserGetCueHandler( void *priv )
{
    VLC_UNUSED(priv);
    return malloc( sizeof(webvtt_cue_t) );
}

static void StreamParserCueDoneHandler( void *priv, webvtt_cue_t *p_cue )
{
    demux_t *p_demux = (demux_t *) priv;
    demux_sys_t *p_sys = p_demux->p_sys;

    if( p_cue->psz_text )
    {
        block_t *p_block = ConvertWEBVTT( p_cue, true, 1.f );
        if( p_block )
        {
            if ( p_sys->b_first_time )
            {
                es_out_SetPCR( p_demux->out, p_cue->i_start + VLC_TICK_0 );
                p_sys->b_first_time = false;
            }
            p_sys->i_next_demux_time = p_cue->i_start;
            p_block->i_dts =
                    p_block->i_pts = VLC_TICK_0 + p_cue->i_start;
            if( p_cue->i_stop >= 0 && p_cue->i_stop >= p_cue->i_start )
                p_block->i_length = p_cue->i_stop - p_cue->i_start;
            es_out_Send( p_demux->out, p_sys->es, p_block );
            es_out_SetPCR( p_demux->out, p_cue->i_start + VLC_TICK_0 );
        }
    }
    webvtt_cue_Clean( p_cue );
    free( p_cue );
}

/*****************************************************************************
 * Demux Index
 *****************************************************************************/

static int index_Compare( const void *a_, const void *b_ )
{
    struct index_entry_s *a = (struct index_entry_s *) a_;
    struct index_entry_s *b = (struct index_entry_s *) b_;
    if( a->time == b->time )
    {
        if( a->active > b->active )
            return -1;
        else
            return b->active - a->active;
    }
    else return a->time < b->time ? -1 : 1;
}

static size_t getIndexByTime( demux_sys_t *p_sys, vlc_tick_t i_time )
{
    size_t first = 0, last = p_sys->index.i_count;
    while( first < last )
    {
        size_t middle = first + (last - first) / 2;
        vlc_tick_t scaled;
        if( !ScaleTime( p_sys->index.p_array[middle].time, p_sys->f_rate, &scaled ) )
            return p_sys->index.i_count;
        if( scaled <= i_time ) first = middle + 1;
        else last = middle;
    }
    if( first == 0 || first == p_sys->index.i_count )
        return first;
    first--;
    while( first > 0 && p_sys->index.p_array[first - 1].time == p_sys->index.p_array[first].time )
        first--;
    return first;
}

static void BuildIndex( demux_t *p_demux )
{
    demux_sys_t *p_sys = p_demux->p_sys;

    if( p_sys->index.i_count == 0 )
        return;
    assert( p_sys->index.p_array != NULL );

    if( p_sys->index.p_array == NULL )
        return;

    /* Order time entries ascending, start time before end time */
    qsort( p_sys->index.p_array, p_sys->index.i_count,
           sizeof(struct index_entry_s), index_Compare );

    /* Build actives count
    TIME 3000 count 1
    TIME 14500 count 2 (1 overlap)
    TIME 16100 count 3 (2 overlaps)
    TIME 16100 count 2 (1 overlap.. because there next start == end)
    TIME 18000 count 3
    TIME 18000 count 2 */
    unsigned i_overlaps = 0;
    for( size_t i=0; i<p_sys->index.i_count; i++ )
    {
        if( p_sys->index.p_array[i].active )
            p_sys->index.p_array[i].active = ++i_overlaps;
        else
            p_sys->index.p_array[i].active = --i_overlaps;
    }
}

static block_t *demux_From( demux_t *p_demux, vlc_tick_t i_start )
{
    demux_sys_t *p_sys = p_demux->p_sys;

    block_t *p_list = NULL;
    block_t **pp_append = &p_list;
    for( size_t i=0; i<p_sys->cues.i_count; i++ )
    {
        const webvtt_cue_t *p_cue = &p_sys->cues.p_array[i];
        if( p_cue->i_start > i_start )
        {
            break;
        }
        else if( p_cue->i_start <= i_start && p_cue->i_stop > i_start )
        {
            *pp_append = ConvertWEBVTT( p_cue, p_sys->index.i_current > 0, p_sys->f_rate );
            if( *pp_append )
                pp_append = &((*pp_append)->p_next);
        }
    }

    return ( p_list ) ? block_ChainGather( p_list ) : NULL;
}

static int ReadWEBVTT( demux_t *p_demux )
{
    demux_sys_t *p_sys = p_demux->p_sys;

    struct callback_ctx ctx;
    ctx.p_demux = p_demux;
    ctx.b_ordered = true;

    webvtt_text_parser_t *p_parser =
            webvtt_text_parser_New( &ctx, ParserGetCueHandler,
                                          ParserCueDoneHandler,
                                          ParserHeaderHandler );
    if( p_parser == NULL )
        return VLC_EGENERIC;

    ctx.regions.b_opened = !vlc_memstream_open( &ctx.regions.memstream );
    ctx.styles.b_opened = !vlc_memstream_open( &ctx.styles.memstream );

    char *psz_line;
    while( (psz_line = vlc_stream_ReadLine( p_demux->s )) )
        webvtt_text_parser_Feed( p_parser, psz_line );
    webvtt_text_parser_Feed( p_parser, NULL );

    if( !ctx.b_ordered )
        qsort( p_sys->cues.p_array, p_sys->cues.i_count, sizeof(webvtt_cue_t), cue_Compare );

    BuildIndex( p_demux );

    memstream_Grab( &ctx.regions, &p_sys->regions_headers.p_data,
                                  &p_sys->regions_headers.i_data );
    memstream_Grab( &ctx.styles, &p_sys->styles_headers.p_data,
                                 &p_sys->styles_headers.i_data );

    webvtt_text_parser_Delete( p_parser );

    return VLC_SUCCESS;
}

static void MakeExtradata( demux_sys_t *p_sys, void **p_extra, size_t *pi_extra )
{
    struct vlc_memstream extradata;
    if( vlc_memstream_open( &extradata ) )
        return;
    vlc_memstream_puts( &extradata, "WEBVTT\n\n");
    vlc_memstream_write( &extradata, p_sys->regions_headers.p_data,
                                     p_sys->regions_headers.i_data );
    vlc_memstream_write( &extradata, p_sys->styles_headers.p_data,
                                     p_sys->styles_headers.i_data );
    if( vlc_memstream_close( &extradata ) == 0 )
    {
        if( extradata.length )
        {
            *p_extra = extradata.ptr;
            *pi_extra = extradata.length;
        }
        else free( extradata.ptr );
    }
}

/*****************************************************************************
 * Control:
 *****************************************************************************/
static int Control( demux_t *p_demux, int i_query, va_list args )
{
    demux_sys_t *p_sys = p_demux->p_sys;
    double *pf;

    switch( i_query )
    {
        case DEMUX_CAN_SEEK:
            *va_arg( args, bool * ) = true;
            return VLC_SUCCESS;

        case DEMUX_GET_LENGTH:
            return ScaleTime( p_sys->i_length, p_sys->f_rate,
                              va_arg( args, vlc_tick_t * ) ) ? VLC_SUCCESS : VLC_EGENERIC;

        case DEMUX_GET_TIME:
            *va_arg( args, vlc_tick_t * ) = p_sys->i_next_demux_time;
            return VLC_SUCCESS;

        case DEMUX_SET_TIME:
            if( p_sys->cues.i_count )
            {
                const vlc_tick_t target = va_arg( args, vlc_tick_t );
                if( target == VLC_TICK_INVALID || target > VLC_TICK_MAX - VLC_TICK_0 )
                    return VLC_EGENERIC;
                p_sys->index.i_current = getIndexByTime( p_sys, target );
                p_sys->b_first_time = true;
                p_sys->i_next_demux_time = target;
                p_sys->i_next_block_flags |= BLOCK_FLAG_DISCONTINUITY;
                return VLC_SUCCESS;
            }
            break;

        case DEMUX_GET_POSITION:
            pf = va_arg( args, double * );
            if( p_sys->index.i_current >= p_sys->index.i_count )
            {
                *pf = 1.0;
            }
            else if( p_sys->index.i_count > 0 )
            {
                vlc_tick_t length;
                if( !ScaleTime( p_sys->i_length, p_sys->f_rate, &length ) )
                    return VLC_EGENERIC;
                *pf = p_sys->i_next_demux_time /
                      ((long double) length + VLC_TICK_FROM_MS(500));
            }
            else
            {
                *pf = 0.0;
            }
            return VLC_SUCCESS;

        case DEMUX_SET_POSITION:
            if( p_sys->cues.i_count )
            {
                double f = va_arg( args, double );
                vlc_tick_t length;
                if( !isfinite( f ) || f < 0 || f > 1 ||
                    !ScaleTime( p_sys->i_length, p_sys->f_rate, &length ) )
                    return VLC_EGENERIC;
                /* A double conversion can round a valid near-INT64_MAX
                 * length out of range, including at position1. */
                const long double fraction = (long double) length * f;
                if( f != 1. && fraction > (long double) (VLC_TICK_MAX - VLC_TICK_0) )
                    return VLC_EGENERIC;
                const vlc_tick_t target = f == 1. ? length : (vlc_tick_t) fraction;
                p_sys->index.i_current = getIndexByTime( p_sys, target );
                p_sys->b_first_time = true;
                p_sys->i_next_demux_time = target;
                p_sys->i_next_block_flags |= BLOCK_FLAG_DISCONTINUITY;
                return VLC_SUCCESS;
            }
            break;

        case DEMUX_CAN_CONTROL_RATE:
            *va_arg( args, bool * ) = true;
            return VLC_SUCCESS;

        case DEMUX_SET_RATE:
        {
            const float rate = *va_arg( args, float * );
            vlc_tick_t length;
            if( !ScaleTime( p_sys->i_length, rate, &length ) )
                return VLC_EGENERIC;
            for( size_t i = 0; i < p_sys->cues.i_count; i++ )
                if( p_sys->cues.p_array[i].psz_text != NULL &&
                    !TransformText( p_sys->cues.p_array[i].psz_text, rate, NULL ) )
                    return VLC_EGENERIC;
            p_sys->f_rate = rate;
            p_sys->index.i_current = getIndexByTime( p_sys, p_sys->i_next_demux_time );
            p_sys->b_first_time = true;
            p_sys->i_next_block_flags |= BLOCK_FLAG_DISCONTINUITY;
            return VLC_SUCCESS;
        }

        case DEMUX_SET_NEXT_DEMUX_TIME:
        {
            const vlc_tick_t target = va_arg( args, vlc_tick_t );
            if( target <= VLC_TICK_INVALID + VLC_TICK_0 )
                return VLC_EGENERIC;
            p_sys->b_slave = true;
            p_sys->i_next_demux_time = target - VLC_TICK_0;
            return VLC_SUCCESS;
        }

        case DEMUX_CAN_PAUSE:
        case DEMUX_SET_PAUSE_STATE:
        case DEMUX_CAN_CONTROL_PACE:
            return demux_vaControlHelper( p_demux->s, 0, -1, 0, 1, i_query, args );

        case DEMUX_GET_PTS_DELAY:
        case DEMUX_GET_FPS:
        case DEMUX_GET_META:
        case DEMUX_GET_ATTACHMENTS:
        case DEMUX_GET_TITLE_INFO:
        case DEMUX_HAS_UNSUPPORTED_META:
        case DEMUX_CAN_RECORD:
        default:
            break;

    }
    return VLC_EGENERIC;
}

static int ControlStream( demux_t *p_demux, int i_query, va_list args )
{
    demux_sys_t *p_sys = p_demux->p_sys;

    switch( i_query )
    {
        case DEMUX_GET_TIME:
            if( p_sys->i_next_demux_time != VLC_TICK_INVALID )
            {
                *va_arg( args, vlc_tick_t * ) = p_sys->i_next_demux_time;
                return VLC_SUCCESS;
            }
        default:
            break;
    }

    return VLC_EGENERIC;
}

/*****************************************************************************
 * Demux: Send subtitle to decoder
 *****************************************************************************/
static int Demux( demux_t *p_demux )
{
    demux_sys_t *p_sys = p_demux->p_sys;

    vlc_tick_t i_barrier = p_sys->i_next_demux_time;

    while( p_sys->index.i_current < p_sys->index.i_count )
    {
        vlc_tick_t scaled_start;
        if( !ScaleTime( p_sys->index.p_array[p_sys->index.i_current].time,
                        p_sys->f_rate, &scaled_start ) )
            return VLC_DEMUXER_EGENERIC;
        if( scaled_start > i_barrier )
            break;
        /* Find start and end of our interval */
        vlc_tick_t i_start_time = p_sys->index.p_array[p_sys->index.i_current].time;
        vlc_tick_t i_end_time = i_start_time;
        /* use next interval time as end time */
        while( ++p_sys->index.i_current < p_sys->index.i_count )
        {
            if( i_start_time != p_sys->index.p_array[p_sys->index.i_current].time )
            {
                i_end_time = p_sys->index.p_array[p_sys->index.i_current].time;
                break;
            }
        }

        block_t *p_block = demux_From( p_demux, i_start_time );
        if( p_block )
        {
            vlc_tick_t scaled_end;
            if( !ScaleTime( i_end_time, p_sys->f_rate, &scaled_end ) )
            {
                block_Release( p_block );
                return VLC_DEMUXER_EGENERIC;
            }
            p_block->i_length = scaled_end - scaled_start;
            p_block->i_dts = p_block->i_pts = VLC_TICK_0 + scaled_start;

            if( p_sys->i_next_block_flags )
            {
                p_block->i_flags = p_sys->i_next_block_flags;
                p_sys->i_next_block_flags = 0;
            }

            if ( !p_sys->b_slave && p_sys->b_first_time )
            {
                es_out_SetPCR( p_demux->out, p_block->i_dts );
                p_sys->b_first_time = false;
            }

            es_out_Send( p_demux->out, p_sys->es, p_block );
        }

        if( p_sys->index.i_current < p_sys->index.i_count &&
            p_sys->index.p_array[p_sys->index.i_current].active > 1 )
        {
            /* we'll need to clear up overlaps */
            p_sys->i_next_block_flags |= BLOCK_FLAG_DISCONTINUITY;
        }
    }

    if ( !p_sys->b_slave )
    {
        es_out_SetPCR( p_demux->out, VLC_TICK_0 + i_barrier );
        if( p_sys->index.i_current < p_sys->index.i_count )
        {
            const vlc_tick_t maximum = VLC_TICK_MAX - VLC_TICK_0;
            if( p_sys->i_next_demux_time > maximum - VLC_TICK_FROM_SEC(1) )
                p_sys->i_next_demux_time = maximum;
            else
                p_sys->i_next_demux_time += VLC_TICK_FROM_SEC(1);
        }
    }

    if( p_sys->index.i_current >= p_sys->index.i_count )
        return VLC_DEMUXER_EOF;

    return VLC_DEMUXER_SUCCESS;
}

static int DemuxStream( demux_t *p_demux )
{
    demux_sys_t *p_sys = p_demux->p_sys;

    char *psz_line = vlc_stream_ReadLine( p_demux->s );
    webvtt_text_parser_Feed( p_sys->p_streamparser, psz_line );

    return ( psz_line == NULL ) ? VLC_DEMUXER_EOF : VLC_DEMUXER_SUCCESS;
}

/*****************************************************************************
 * Module initializers common
 *****************************************************************************/
static int ProbeWEBVTT( demux_t *p_demux )
{
    const uint8_t *p_peek;
    ssize_t i_peek = vlc_stream_Peek( p_demux->s, &p_peek, 16 );
    if( i_peek < 16 )
        return VLC_EGENERIC;

    if( !memcmp( p_peek, "\xEF\xBB\xBF", 3 ) )
        p_peek += 3;

    if( ( memcmp( p_peek, "WEBVTT", 6 ) ||
          ( p_peek[6] != '\n' &&
            p_peek[6] != ' ' &&
            p_peek[6] != '\t' &&
           ( p_peek[6] != '\r' || p_peek[7] != '\n' ) )
        ) && !p_demux->obj.force )
    {
        msg_Dbg( p_demux, "subtitle demux discarded" );
        return VLC_EGENERIC;
    }

    return VLC_SUCCESS;
}

/*****************************************************************************
 * Module initializers
 *****************************************************************************/
int webvtt_OpenDemux ( vlc_object_t *p_this )
{
    demux_t        *p_demux = (demux_t*)p_this;
    demux_sys_t    *p_sys;

    int i_ret = ProbeWEBVTT( p_demux );
    if( i_ret != VLC_SUCCESS )
        return i_ret;

    p_demux->pf_demux = Demux;
    p_demux->pf_control = Control;

    p_demux->p_sys = p_sys = calloc( 1, sizeof( demux_sys_t ) );
    if( p_sys == NULL )
        return VLC_ENOMEM;
    p_sys->f_rate = 1.f;

    if( ReadWEBVTT( p_demux ) != VLC_SUCCESS )
    {
        webvtt_CloseDemux( p_this );
        return VLC_EGENERIC;
    }

    es_format_t fmt;
    es_format_Init( &fmt, SPU_ES, VLC_CODEC_WEBVTT );
    size_t i_extra = 0;
    MakeExtradata( p_sys, &fmt.p_extra, &i_extra );
    fmt.i_extra = i_extra;
    fmt.i_id = 0;
    p_sys->es = es_out_Add( p_demux->out, &fmt );
    es_format_Clean( &fmt );
    if( p_sys->es == NULL )
    {
        webvtt_CloseDemux( p_this );
        return VLC_EGENERIC;
    }

    return VLC_SUCCESS;
}

int webvtt_OpenDemuxStream ( vlc_object_t *p_this )
{
    demux_t        *p_demux = (demux_t*)p_this;
    demux_sys_t    *p_sys;

    int i_ret = ProbeWEBVTT( p_demux );
    if( i_ret != VLC_SUCCESS )
        return i_ret;

    p_demux->pf_demux = DemuxStream;
    p_demux->pf_control = ControlStream;

    p_demux->p_sys = p_sys = calloc( 1, sizeof( demux_sys_t ) );
    if( p_sys == NULL )
        return VLC_ENOMEM;
    p_sys->f_rate = 1.f;

    p_sys->p_streamparser = webvtt_text_parser_New( p_demux,
                                          StreamParserGetCueHandler,
                                          StreamParserCueDoneHandler,
                                          NULL );
    if( !p_sys->p_streamparser )
    {
        webvtt_CloseDemux( p_this );
        return VLC_EGENERIC;
    }

    es_format_t fmt;
    es_format_Init( &fmt, SPU_ES, VLC_CODEC_WEBVTT );
    p_sys->es = es_out_Add( p_demux->out, &fmt );
    es_format_Clean( &fmt );
    if( p_sys->es == NULL )
    {
        webvtt_CloseDemux( p_this );
        return VLC_EGENERIC;
    }

    return VLC_SUCCESS;
}

/*****************************************************************************
 * Close: Close subtitle demux
 *****************************************************************************/
void webvtt_CloseDemux( vlc_object_t *p_this )
{
    demux_t *p_demux = (demux_t*)p_this;
    demux_sys_t *p_sys = p_demux->p_sys;

    for( size_t i=0; i< p_sys->cues.i_count; i++ )
        webvtt_cue_Clean( &p_sys->cues.p_array[i] );
    free( p_sys->cues.p_array );

    free( p_sys->index.p_array );

    if( p_sys->p_streamparser )
    {
        webvtt_text_parser_Feed( p_sys->p_streamparser, NULL );
        webvtt_text_parser_Delete( p_sys->p_streamparser );
    }

    free( p_sys->regions_headers.p_data );
    free( p_sys->styles_headers.p_data );

    free( p_sys );
}
