/*****************************************************************************
 * soxr.c: resampler/converter using The SoX Resampler library
 *****************************************************************************
 * Copyright (C) 2015 VLC authors, VideoLAN and VideoLabs
 *
 * Authors: Thomas Guillem <thomas@gllm.fr>
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
#include <vlc_aout.h>
#include <vlc_filter.h>
#include <vlc_plugin.h>

#include <assert.h>
#include <math.h>
#include <soxr.h>

#define SOXR_QUALITY_TEXT N_( "Resampling quality" )
#define SOXR_QUALITY_LONGTEXT N_( "Resampling quality, from worst to best" )

static const int soxr_resampler_quality_vlclist[] = { 0, 1, 2, 3, 4 };
static const char *const soxr_resampler_quality_vlctext[] =
{
     /* Note: Skip translation - too technical */
     "Quick cubic interpolation",
     "Low 16-bit with larger roll-off",
     "Medium 16-bit with medium roll-off",
     "High quality",
     "Very high quality"
};
static const soxr_datatype_t soxr_resampler_quality_list[] =
{
    SOXR_QQ,
    SOXR_LQ,
    SOXR_MQ,
    SOXR_HQ,
    SOXR_VHQ
};
#define MAX_SOXR_QUALITY 4

static int OpenConverter( vlc_object_t * );
static int OpenResampler( vlc_object_t * );
static void Close( filter_t * );

vlc_module_begin ()
    set_shortname( N_("SoX Resampler") )
    set_subcategory( SUBCAT_AUDIO_RESAMPLER )
    add_integer( "soxr-resampler-quality", 2,
                SOXR_QUALITY_TEXT, NULL )
        change_integer_list( soxr_resampler_quality_vlclist,
                             soxr_resampler_quality_vlctext )
    set_capability ( "audio converter", 51 )
    set_callback( OpenConverter )

    add_submodule()
    set_capability( "audio resampler", 51 )
    set_callback( OpenResampler )
    add_shortcut( "soxr" )
vlc_module_end ()

typedef struct
{
    soxr_t  soxr;
    soxr_t  vr_soxr;
    soxr_t  last_soxr;
    double  f_fixed_ratio;
    size_t  i_last_olen;
    vlc_tick_t i_next_pts;
    date_t fixed_input_pts;
    date_t fixed_output_pts;
    vlc_tick_t fixed_pts_offset;
    bool fixed_pts_valid;
} filter_sys_t;

static block_t *Resample( filter_t *, block_t * );
static block_t *FixedResample( filter_t *, block_t * );
static block_t *Drain( filter_t * );
static void     Flush( filter_t * );

static bool
SoXR_GetFormat( vlc_fourcc_t i_format, soxr_datatype_t *p_type )
{
    switch( i_format )
    {
        case VLC_CODEC_FL64:
            *p_type = SOXR_FLOAT64_I;
            return true;
        case VLC_CODEC_FL32:
            *p_type = SOXR_FLOAT32_I;
            return true;
        case VLC_CODEC_S32N:
            *p_type = SOXR_INT32_I;
            return true;
        case VLC_CODEC_S16N:
            *p_type = SOXR_INT16_I;
            return true;
        default:
            return false;
    }
}

static int
Open( vlc_object_t *p_obj, bool b_change_ratio )
{
    filter_t *p_filter = (filter_t *)p_obj;

    /* Cannot remix */
    if( p_filter->fmt_in.audio.i_channels != p_filter->fmt_out.audio.i_channels )
        return VLC_EGENERIC;

    /* Get SoXR input/output format */
    soxr_datatype_t i_itype, i_otype;
    if( !SoXR_GetFormat( p_filter->fmt_in.audio.i_format, &i_itype )
     || !SoXR_GetFormat( p_filter->fmt_out.audio.i_format, &i_otype ) )
        return VLC_EGENERIC;

    filter_sys_t *p_sys = calloc( 1, sizeof( filter_sys_t ) );
    if( unlikely( p_sys == NULL ) )
        return VLC_ENOMEM;

    p_sys->i_next_pts = VLC_TICK_INVALID;
    date_Init( &p_sys->fixed_input_pts, p_filter->fmt_in.audio.i_rate, 1 );
    date_Init( &p_sys->fixed_output_pts, p_filter->fmt_out.audio.i_rate, 1 );
    date_Set( &p_sys->fixed_input_pts, VLC_TICK_0 );
    date_Set( &p_sys->fixed_output_pts, VLC_TICK_0 );
    p_sys->fixed_pts_offset = 0;
    p_sys->fixed_pts_valid = false;

    /* Setup SoXR */
    int64_t i_vlc_q = var_InheritInteger( p_obj, "soxr-resampler-quality" );
    if( i_vlc_q < 0 )
        i_vlc_q = 0;
    else if( i_vlc_q > MAX_SOXR_QUALITY )
        i_vlc_q = MAX_SOXR_QUALITY;
    const unsigned long i_recipe = soxr_resampler_quality_list[i_vlc_q];
    const unsigned i_channels = p_filter->fmt_in.audio.i_channels;
    const double f_ratio = p_filter->fmt_out.audio.i_rate
                           / (double) p_filter->fmt_in.audio.i_rate;

    p_sys->f_fixed_ratio = f_ratio;
    soxr_error_t error;
    /* IO spec */
    soxr_io_spec_t io_spec = soxr_io_spec( i_itype, i_otype );
    /* Quality spec */
    soxr_quality_spec_t q_spec = soxr_quality_spec( i_recipe, 0 );
    /* Create SoXR */
    p_sys->soxr = soxr_create( 1, f_ratio, i_channels,
                               &error, &io_spec, &q_spec, NULL );
    if( error )
    {
        msg_Err( p_filter, "soxr_create failed: %s", soxr_strerror( error ) );
        free( p_sys );
        return VLC_EGENERIC;
    }

    /* Create a 'variable-rate' SoXR if needed: it is slower than the fixed
     * one, but it will be only used when the input rate is changing (to catch
     * up a delay).  */
    if( b_change_ratio )
    {
        q_spec = soxr_quality_spec( SOXR_LQ, SOXR_VR );
        p_sys->vr_soxr = soxr_create( 1, f_ratio, i_channels,
                                      &error, &io_spec, &q_spec, NULL );
        if( error )
        {
            msg_Err( p_filter, "soxr_create failed: %s", soxr_strerror( error ) );
            soxr_delete( p_sys->soxr );
            free( p_sys );
            return VLC_EGENERIC;
        }
        soxr_set_io_ratio( p_sys->vr_soxr, 1 / f_ratio, 0 );
    }

    msg_Dbg( p_filter, "Using SoX Resampler with '%s' engine and '%s' quality "
             "to convert %4.4s/%dHz to %4.4s/%dHz.",
             soxr_engine( p_sys->soxr ), soxr_resampler_quality_vlctext[i_vlc_q],
             (const char *)&p_filter->fmt_in.audio.i_format,
             p_filter->fmt_in.audio.i_rate,
             (const char *)&p_filter->fmt_out.audio.i_format,
             p_filter->fmt_out.audio.i_rate );

    static const struct vlc_filter_operations filter_ops =
    {
        .filter_audio = Resample,
        .drain_audio = Drain,
        .flush = Flush,
        .close = Close,
    };
    static const struct vlc_filter_operations fixed_filter_ops =
    {
        .filter_audio = FixedResample,
        .drain_audio = Drain,
        .flush = Flush,
        .close = Close,
    };

    p_filter->ops = b_change_ratio ? &filter_ops : &fixed_filter_ops;
    p_filter->p_sys = p_sys;
    return VLC_SUCCESS;
}

static int
OpenResampler( vlc_object_t *p_obj )
{
    filter_t *p_filter = (filter_t *)p_obj;

    /* A resampler doesn't convert the format */
    if( p_filter->fmt_in.audio.i_format != p_filter->fmt_out.audio.i_format )
        return VLC_EGENERIC;
    return Open( p_obj, true );
}

static int
OpenConverter( vlc_object_t *p_obj )
{
    filter_t *p_filter = (filter_t *)p_obj;

    /* Don't use SoXR to convert format. Prefer to use converter/format.c that
     * has better performances */
    if( p_filter->fmt_in.audio.i_rate == p_filter->fmt_out.audio.i_rate )
        return VLC_EGENERIC;
    return Open( p_obj, false );
}

static void
Close( filter_t *p_filter )
{
    filter_sys_t *p_sys = p_filter->p_sys;

    soxr_delete( p_sys->soxr );
    if( p_sys->vr_soxr )
        soxr_delete( p_sys->vr_soxr );

    free( p_sys );
}

static block_t *
SoXR_Resample( filter_t *p_filter, soxr_t soxr, block_t *p_in, size_t i_olen )
{
    filter_sys_t *p_sys = p_filter->p_sys;
    size_t i_idone, i_odone;
    const size_t i_oframesize = p_filter->fmt_out.audio.i_bytes_per_frame;
    const size_t i_ilen = p_in ? p_in->i_nb_samples : 0;

    block_t *p_out;
    if( i_ilen >= i_olen )
    {
        i_olen = i_ilen;
        p_out = p_in;
    }
    else
    {
        p_out = block_Alloc( i_olen * i_oframesize );
        if( p_out == NULL )
            goto error;
    }

    soxr_error_t error = soxr_process( soxr, p_in ? p_in->p_buffer : NULL,
                                       i_ilen, &i_idone, p_out->p_buffer,
                                       i_olen, &i_odone );
    if( error )
    {
        msg_Err( p_filter, "soxr_process failed: %s", soxr_strerror( error ) );
        if( p_out == p_in )
            p_in = NULL; /* The input/output alias is released once below. */
        block_Release( p_out );
        p_out = NULL;
        goto error;
    }
    if( unlikely( i_idone < i_ilen ) )
        msg_Err( p_filter, "lost %zd of %zd input frames",
                 i_ilen - i_idone, i_idone);

    p_out->i_buffer = i_odone * i_oframesize;
    p_out->i_nb_samples = i_odone;
    p_out->i_length = vlc_tick_from_samples(i_odone, p_filter->fmt_out.audio.i_rate);

    if( p_in )
    {
        p_sys->i_next_pts = p_in->i_pts + p_out->i_length;
        p_sys->i_last_olen = i_olen;
        p_sys->last_soxr = soxr;
    }
    else
    {
        soxr_clear( soxr );
        p_sys->i_last_olen = 0;
        p_sys->last_soxr = NULL;
    }

error:
    if( p_in && p_out != p_in )
        block_Release( p_in );

    return p_out;
}

/* Complete both fixed and variable engines before clearing them. The bundled
 * variable engine reports a placeholder delay; only an actual empty native
 * drain establishes that all retained output has been produced. */
static block_t *
SoXR_Drain( filter_t *p_filter, soxr_t soxr )
{
    filter_sys_t *p_sys = p_filter->p_sys;
    const size_t framesize = p_filter->fmt_out.audio.i_bytes_per_frame;
    const unsigned rate = p_filter->fmt_out.audio.i_rate;
    const double pending = ceil(soxr_delay(soxr));
    if( !isfinite(pending) || pending < 0 || framesize == 0 || rate == 0 )
        goto invalid;
    const size_t max_samples = SIZE_MAX / framesize;
    if( max_samples < 2 || pending >= (double)(max_samples - 2) )
        goto invalid;
    const size_t capacity = __MAX(__MAX(p_sys->i_last_olen, (size_t)pending + 2),
                                 __MIN((size_t)4096, max_samples));
    if( capacity > max_samples || capacity > UINT_MAX )
        goto invalid;

    block_t *chain = NULL, **tail = &chain;
    unsigned samples = 0;
    /* Fixed delay is an accurate capacity hint; VR must still drain to zero.
     * A broken engine cannot produce unbounded chunks in one filter call. */
    for( unsigned attempt = 0; attempt < 64; ++attempt )
    {
        block_t *output = block_Alloc(capacity * framesize);
        if( output == NULL )
        {
            if( chain == NULL )
                return NULL; /* No native consumption: initial OOM is retryable. */
            goto error;
        }
        size_t consumed, produced;
        soxr_error_t error = soxr_process(soxr, NULL, 0, &consumed,
                                         output->p_buffer, capacity, &produced);
        if( error || consumed != 0 || produced > capacity )
        {
            msg_Err(p_filter, "soxr drain failed: %s", error ? error : "invalid native counts");
            block_Release(output);
            goto error;
        }
        if( produced == 0 )
        {
            block_Release(output);
            soxr_clear(soxr);
            p_sys->i_last_olen = 0;
            p_sys->last_soxr = NULL;
            if( chain == NULL )
                return NULL;
            block_t *result = chain->p_next ? block_ChainGather(chain) : chain;
            if( result == NULL )
            {
                block_ChainRelease(chain); /* Failed Gather retains its chain. */
                return NULL;
            }
            result->i_nb_samples = samples;
            result->i_length = vlc_tick_from_samples(samples, rate);
            return result;
        }
        if( produced > UINT_MAX - samples || samples + produced > max_samples )
        {
            block_Release(output);
            goto error;
        }
        samples += produced;
        output->i_buffer = produced * framesize;
        output->i_nb_samples = produced;
        output->i_length = vlc_tick_from_samples(produced, rate);
        *tail = output;
        tail = &output->p_next;
    }
    msg_Err(p_filter, "SoXR drain exceeded its bounded chunk count");
error:
    block_ChainRelease(chain);
    soxr_clear(soxr); /* Partial/error consumption cannot be retried as a full tail. */
    p_sys->i_last_olen = 0;
    p_sys->last_soxr = NULL;
    return NULL;
invalid:
    msg_Err(p_filter, "invalid pending SoXR drain size");
    return NULL;
}

static size_t
SoXR_GetOutLen( size_t i_ilen, double f_ratio )
{
    /* Processed output len might be a little bigger than expected. Indeed SoXR
     * can hold a few samples to meet the need of the resampling filter. */
    return lrint( ( i_ilen + 2 ) * f_ratio * 11. / 10. );
}

static block_t *
Resample( filter_t *p_filter, block_t *p_in )
{
    filter_sys_t *p_sys = p_filter->p_sys;
    const vlc_tick_t i_pts = p_in->i_pts;
    assert( p_sys->vr_soxr != NULL );

    /* "audio resampler" with variable ratio: use the fixed resampler when
     * the ratio is the same than the fixed one, otherwise use the variable
     * resampler. */

    soxr_t soxr;
    block_t *p_flushed_out = NULL, *p_out = NULL;
    const double f_ratio = p_filter->fmt_out.audio.i_rate
                         / (double) p_filter->fmt_in.audio.i_rate;
    size_t i_olen = SoXR_GetOutLen( p_in->i_nb_samples,
        f_ratio > p_sys->f_fixed_ratio ? f_ratio : p_sys->f_fixed_ratio );

    if( f_ratio != p_sys->f_fixed_ratio )
    {
        /* using variable resampler */
        soxr_set_io_ratio( p_sys->vr_soxr, 1 / f_ratio, 0 /* instant change */ );
        soxr = p_sys->vr_soxr;
    }
    else if( f_ratio == 1.0f )
    {
        /* not using any resampler */
        soxr = NULL;
        p_out = p_in;
    }
    else
    {
        /* using fixed resampler */
        soxr = p_sys->soxr;
    }

    /* If the new soxr is different than the last one, flush it */
    if( p_sys->last_soxr && soxr != p_sys->last_soxr && p_sys->i_last_olen )
    {
        p_flushed_out = SoXR_Drain( p_filter, p_sys->last_soxr );
        if( p_flushed_out == NULL && p_sys->i_last_olen )
        {
            /* Initial drain allocation failed without consuming the old engine.
             * Preserve it for retry instead of overwriting it with a new input. */
            block_Release(p_in);
            return NULL;
        }
        if( soxr )
            msg_Dbg( p_filter, "Using '%s' engine", soxr_engine( soxr ) );
    }

    if( soxr )
    {
        assert( !p_out );
        p_out = SoXR_Resample( p_filter, soxr, p_in, i_olen );
        if( !p_out )
            goto error;
    }

    if( p_flushed_out )
    {
        /* Prepend the flushed output data to p_out */
        const unsigned i_nb_samples = p_flushed_out->i_nb_samples
                                    + p_out->i_nb_samples;

        block_ChainAppend( &p_flushed_out, p_out );
        p_out = block_ChainGather( p_flushed_out );
        if( !p_out )
            goto error;
        p_flushed_out = NULL; /* Successful gather consumed the chain. */
        p_out->i_nb_samples = i_nb_samples;
    }
    p_out->i_pts = i_pts;
    return p_out;
error:
    /* SoXR_Resample consumes its input. A failed gather retains the chain. */
    block_ChainRelease( p_flushed_out );
    return NULL;
}

static void
FixedTimestamp( filter_sys_t *p_sys, block_t *p_out )
{
    const vlc_tick_t start = date_Get( &p_sys->fixed_output_pts );
    const vlc_tick_t end = date_Increment( &p_sys->fixed_output_pts,
                                           p_out->i_nb_samples );
    p_out->i_length = end - start;
    p_out->i_pts = p_sys->fixed_pts_valid
                ? start + p_sys->fixed_pts_offset : VLC_TICK_INVALID;
    p_sys->i_next_pts = p_sys->fixed_pts_valid
                     ? end + p_sys->fixed_pts_offset : VLC_TICK_INVALID;
}

static block_t *
FixedResample( filter_t *p_filter, block_t *p_in )
{
    /* Fixed conversion emits buffered samples from earlier input calls. Its
     * output clock follows emitted samples, including fractional tick remainders.
     * Adaptive playback's input-media-clock contract in Resample is separate. */
    filter_sys_t *p_sys = p_filter->p_sys;
    assert( p_sys->vr_soxr == NULL );

    if( p_in->i_flags & BLOCK_FLAG_DISCONTINUITY )
        Flush( p_filter );

    if( p_in->i_pts != VLC_TICK_INVALID )
    {
        const vlc_tick_t expected = date_Get( &p_sys->fixed_input_pts );
        if( !p_sys->fixed_pts_valid )
        {
            /* Retain unanchored PCM. A later timestamp also locates the
             * buffered samples that arrived before it. */
            p_sys->fixed_pts_offset = p_in->i_pts - expected;
            p_sys->fixed_pts_valid = true;
        }
        else
        {
            const vlc_tick_t delta = p_in->i_pts - expected
                                  - p_sys->fixed_pts_offset;
            if( delta < -1 || delta > 1 )
                /* Follow source-clock changes without discarding PCM or
                 * resetting either sample clock's fractional remainder. */
                p_sys->fixed_pts_offset += delta;
        }
    }
    date_Increment( &p_sys->fixed_input_pts, p_in->i_nb_samples );

    const size_t i_olen = SoXR_GetOutLen( p_in->i_nb_samples,
                                          p_sys->f_fixed_ratio );
    block_t *p_out = SoXR_Resample( p_filter, p_sys->soxr, p_in, i_olen );
    if( p_out )
        FixedTimestamp( p_sys, p_out );
    else
        Flush( p_filter ); /* A consumed/dropped input cannot extend this clock. */
    return p_out;
}

static block_t *
Drain( filter_t *p_filter )
{
    filter_sys_t *p_sys = p_filter->p_sys;

    if( p_sys->last_soxr && p_sys->i_last_olen )
    {
        block_t *p_out = SoXR_Drain( p_filter, p_sys->last_soxr );
        if( p_sys->vr_soxr == NULL )
        {
            if( p_out != NULL )
                FixedTimestamp( p_sys, p_out );
            if( p_sys->last_soxr == NULL )
            {
                date_Set( &p_sys->fixed_input_pts, VLC_TICK_0 );
                date_Set( &p_sys->fixed_output_pts, VLC_TICK_0 );
                p_sys->fixed_pts_offset = 0;
                p_sys->fixed_pts_valid = false;
            }
        }
        else if( p_out != NULL )
        {
            assert( p_sys->i_next_pts != VLC_TICK_INVALID );
            p_out->i_pts = p_sys->i_next_pts;
        }
        return p_out;
    }
    else
        return NULL;
}

static void
Flush( filter_t *p_filter )
{
    filter_sys_t *p_sys = p_filter->p_sys;

    if( p_sys->last_soxr )
    {
        soxr_clear( p_sys->last_soxr );
        p_sys->i_last_olen = 0;
        p_sys->last_soxr = NULL;
    }
    p_sys->i_next_pts = VLC_TICK_INVALID;
    date_Set( &p_sys->fixed_input_pts, VLC_TICK_0 );
    date_Set( &p_sys->fixed_output_pts, VLC_TICK_0 );
    p_sys->fixed_pts_offset = 0;
    p_sys->fixed_pts_valid = false;
}
