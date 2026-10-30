// SPDX-License-Identifier: LGPL-2.1-or-later
/* Actual indexed WebVTT parser, Control, Demux and packet serializer boundaries.
 * No duplicate timing or inline-tag conversion algorithm is implemented here. */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
const char vlc_module_name[] = "webvtt_fps_test";
#ifndef WEBVTT_DEMUX_SOURCE
# define WEBVTT_DEMUX_SOURCE "../../../modules/demux/webvtt.c"
#endif
#include WEBVTT_DEMUX_SOURCE
#include <vlc_es_out.h>
#include <float.h>
#include <stdio.h>

struct capture
{
    es_out_t out;
    block_t *blocks[64];
    size_t count;
    unsigned pcrs;
    int es;
};
static unsigned checks, failures;
static const char *fixture_root;
static bool check( bool value, const char *expression, unsigned line )
{
    checks++;
    if( !value )
    {
        failures++;
        fprintf( stderr, "line%u: %s\n", line, expression );
    }
    return value;
}
#define CHECK(x) check( !!(x), #x, __LINE__ )
#define REQUIRE(x) do { if( !CHECK(x) ) return; } while(0)

/* The unchanged parser translation unit also contains its module descriptor.
 * Its unused non-demux callback references need definitions on PE/COFF even
 * with section GC. They are never selected here; any call fails the test.
 * This harness makes no decoder/encoder/muxer qualification claim. */
int webvtt_OpenDecoder(vlc_object_t *object) { (void)object;CHECK(false);return VLC_EGENERIC; }
void webvtt_CloseDecoder(vlc_object_t *object) { (void)object;CHECK(false); }
#ifdef ENABLE_SOUT
int webvtt_OpenEncoder(vlc_object_t *object) { (void)object;CHECK(false);return VLC_EGENERIC; }
int webvtt_OpenMuxer(vlc_object_t *object) { (void)object;CHECK(false);return VLC_EGENERIC; }
void webvtt_CloseMuxer(vlc_object_t *object) { (void)object;CHECK(false); }
#endif

static es_out_id_t *Add( es_out_t *out, input_source_t *source, const es_format_t *fmt )
{
    (void) source;
    struct capture *capture = (struct capture *) out;
    CHECK( fmt->i_codec == VLC_CODEC_WEBVTT );
    return (es_out_id_t *) &capture->es;
}
static int Send( es_out_t *out, es_out_id_t *id, block_t *block )
{
    struct capture *capture = (struct capture *) out;
    CHECK( id == (es_out_id_t *) &capture->es );
    if( !CHECK( capture->count < ARRAY_SIZE(capture->blocks) ) )
    {
        block_Release( block );
        return VLC_EGENERIC;
    }
    capture->blocks[capture->count++] = block;
    return VLC_SUCCESS;
}
static void Del( es_out_t *out, es_out_id_t *id )
{
    struct capture *capture = (struct capture *) out;
    CHECK( id == (es_out_id_t *) &capture->es );
}
static int OutControl( es_out_t *out, input_source_t *source, int query, va_list args )
{
    (void) source; (void) args;
    struct capture *capture = (struct capture *) out;
    if( query == ES_OUT_SET_PCR || query == ES_OUT_SET_GROUP_PCR ) capture->pcrs++;
    return VLC_SUCCESS;
}
static const struct es_out_callbacks callbacks = { .add=Add,.send=Send,.del=Del,.control=OutControl };
static void Clear( struct capture *capture )
{
    for( size_t i=0; i<capture->count; i++ ) block_Release(capture->blocks[i]);
    capture->count=0;
}
static demux_t *OpenFixture( struct capture *capture, const char *name, bool stream )
{
    memset( capture, 0, sizeof(*capture) ); capture->out.cbs=&callbacks;
    char path[2048]; snprintf(path,sizeof(path),"%s/%s",fixture_root,name);
    FILE *file=fopen(path,"rb"); if( !CHECK(file != NULL) ) return NULL;
    fseek(file,0,SEEK_END); long length=ftell(file); rewind(file);
    uint8_t *data=malloc(length); if( !CHECK(data != NULL) ) { fclose(file); return NULL; }
    bool loaded=fread(data,1,length,file)==(size_t)length; fclose(file);
    if( !CHECK(loaded) ) { free(data); return NULL; }
    demux_t *demux=(vlc_object_create)(NULL,sizeof(*demux));
    if( !CHECK(demux != NULL) ) { free(data); return NULL; }
    demux->s=vlc_stream_MemoryNew(VLC_OBJECT(demux),data,length,false);
    demux->out=&capture->out;
    if( !CHECK(demux->s != NULL) ) { free(data); vlc_object_delete(demux); return NULL; }
    int status=stream ? webvtt_OpenDemuxStream(VLC_OBJECT(demux)) : webvtt_OpenDemux(VLC_OBJECT(demux));
    if( !CHECK(status==VLC_SUCCESS) ) { vlc_stream_Delete(demux->s);vlc_object_delete(demux);return NULL; }
    return demux;
}
static void CloseFixture( demux_t *demux, struct capture *capture )
{
    Clear(capture); webvtt_CloseDemux(VLC_OBJECT(demux));
    vlc_stream_Delete(demux->s); vlc_object_delete(demux);
}
static int Query( demux_t *demux, int query, ... )
{
    va_list args;va_start(args,query);int status=demux->pf_control(demux,query,args);va_end(args);return status;
}
static bool Contains( const block_t *block, const char *text )
{
    size_t length=strlen(text);
    for(size_t i=0;i+length<=block->i_buffer;i++)
        if(!memcmp(block->p_buffer+i,text,length)) return true;
    return false;
}
static void CheckBoxes( const block_t *block )
{
    size_t offset=0;
    while(offset<block->i_buffer)
    {
        if(!CHECK(block->i_buffer-offset>=8))return;
        uint32_t size=GetDWBE(block->p_buffer+offset);
        if(!CHECK(size>=8 && size<=block->i_buffer-offset))return;
        size_t inner=offset+8;
        while(inner<offset+size)
        {
            if(!CHECK(offset+size-inner>=8))return;
            uint32_t child=GetDWBE(block->p_buffer+inner);
            if(!CHECK(child>=8 && child<=offset+size-inner))return;
            inner+=child;
        }
        CHECK(inner==offset+size); offset+=size;
    }
    CHECK(offset==block->i_buffer);
}
static void RefillInsideCue( void )
{
    struct capture capture;demux_t *demux=OpenFixture(&capture,"long-cue.vtt",false);REQUIRE(demux);
    CHECK(Query(demux,DEMUX_SET_TIME,VLC_TICK_FROM_SEC(5),true)==VLC_SUCCESS);
    CHECK(Query(demux,DEMUX_SET_NEXT_DEMUX_TIME,VLC_TICK_0+VLC_TICK_FROM_SEC(5))==VLC_SUCCESS);
    CHECK(demux->pf_demux(demux)==VLC_DEMUXER_SUCCESS);
    CHECK(capture.count==1);
    if(capture.count==1){CHECK(capture.blocks[0]->i_pts==VLC_TICK_0+VLC_TICK_FROM_SEC(2));CHECK(capture.blocks[0]->i_length==VLC_TICK_FROM_SEC(8));CheckBoxes(capture.blocks[0]);}
    CHECK(capture.pcrs==0);CloseFixture(demux,&capture);
}
static void RateAndInline( void )
{
    struct capture capture;demux_t *demux=OpenFixture(&capture,"overlap-inline.vtt",false);REQUIRE(demux);
    demux_sys_t *sys=demux->p_sys;bool can=false;float rate=2.f;
    CHECK(Query(demux,DEMUX_CAN_CONTROL_RATE,&can)==VLC_SUCCESS && can);
    size_t bytes=sys->index.i_count*sizeof(*sys->index.p_array);void *original=malloc(bytes);
    char *text=strdup(sys->cues.p_array[0].psz_text);
    if(!CHECK(original && text)){free(original);free(text);CloseFixture(demux,&capture);return;}
    memcpy(original,sys->index.p_array,bytes);
    CHECK(Query(demux,DEMUX_SET_NEXT_DEMUX_TIME,VLC_TICK_0+VLC_TICK_FROM_MS(6500))==VLC_SUCCESS);
    CHECK(Query(demux,DEMUX_SET_RATE,&rate)==VLC_SUCCESS);
    CHECK(demux->pf_demux(demux)==VLC_DEMUXER_SUCCESS);CHECK(capture.count==1);
    if(capture.count==1){block_t *b=capture.blocks[0];CHECK(b->i_pts==VLC_TICK_0+VLC_TICK_FROM_SEC(6));CHECK(b->i_length==VLC_TICK_FROM_SEC(4));CHECK(b->i_flags&BLOCK_FLAG_DISCONTINUITY);CHECK(Contains(b,"<00:00:08.000>"));CHECK(Contains(b,"<00:00:12.000>"));CHECK(Contains(b,"&lt;00:00:09.000&gt;"));CHECK(Contains(b,"<b title=\"00:00:08.000\">"));CHECK(Contains(b,"<00:00:07.000x>"));CHECK(Contains(b,"region:bottom line:80%"));CHECK(Contains(b,"overlap"));CheckBoxes(b);}
    CHECK(!memcmp(original,sys->index.p_array,bytes));CHECK(!strcmp(text,sys->cues.p_array[0].psz_text));
    CHECK(capture.pcrs==0);free(original);free(text);CloseFixture(demux,&capture);
}
static void ResetAndFractional( void )
{
    struct capture capture;demux_t *demux=OpenFixture(&capture,"long-cue.vtt",false);REQUIRE(demux);
    const float rates[]={2.f,.5f,1.25f,1.f};
    for(size_t i=0;i<ARRAY_SIZE(rates);i++)
    {
        Clear(&capture);float rate=rates[i];vlc_tick_t deadline=VLC_TICK_FROM_SEC(3)*rate;
        CHECK(Query(demux,DEMUX_SET_NEXT_DEMUX_TIME,VLC_TICK_0+deadline)==VLC_SUCCESS);
        CHECK(Query(demux,DEMUX_SET_RATE,&rate)==VLC_SUCCESS);
        CHECK(demux->pf_demux(demux)==VLC_DEMUXER_SUCCESS);CHECK(capture.count==1);
        if(capture.count==1){block_t*b=capture.blocks[0];CHECK(b->i_pts==VLC_TICK_0+(vlc_tick_t)(VLC_TICK_FROM_SEC(2)*rate));CHECK(b->i_length==(vlc_tick_t)(VLC_TICK_FROM_SEC(8)*rate));const char *tags[]={"<00:00:08.000>","<00:00:02.000>","<00:00:05.000>","<00:00:04.000>"};CHECK(Contains(b,tags[i]));CheckBoxes(b);}
    }
    CHECK(capture.pcrs==0);CloseFixture(demux,&capture);
}
static void EofRevivalAndNoWrap( void )
{
    struct capture capture;demux_t *demux=OpenFixture(&capture,"overlap-inline.vtt",false);REQUIRE(demux);
    CHECK(Query(demux,DEMUX_SET_NEXT_DEMUX_TIME,VLC_TICK_0+VLC_TICK_FROM_SEC(40))==VLC_SUCCESS);
    CHECK(demux->pf_demux(demux)==VLC_DEMUXER_EOF);Clear(&capture);
    CHECK(Query(demux,DEMUX_SET_NEXT_DEMUX_TIME,VLC_TICK_0+VLC_TICK_FROM_SEC(20))==VLC_SUCCESS);
    float rate=2;CHECK(Query(demux,DEMUX_SET_RATE,&rate)==VLC_SUCCESS);
    CHECK(demux->pf_demux(demux)==VLC_DEMUXER_SUCCESS);CHECK(capture.count==1);
    if(capture.count==1){CHECK(Contains(capture.blocks[0],"edge"));CHECK(capture.blocks[0]->i_pts==VLC_TICK_0+VLC_TICK_FROM_SEC(20));}
    Clear(&capture);rate=1;CHECK(Query(demux,DEMUX_SET_RATE,&rate)==VLC_SUCCESS);
    CHECK(demux->pf_demux(demux)==VLC_DEMUXER_EOF);CHECK(capture.count==0);
    CHECK(Query(demux,DEMUX_SET_TIME,VLC_TICK_FROM_SEC(99),true)==VLC_SUCCESS);
    CHECK(demux->pf_demux(demux)==VLC_DEMUXER_EOF);CHECK(capture.count==0);CloseFixture(demux,&capture);
}
static void InvalidRatesAreAtomic( void )
{
    struct capture capture;demux_t *demux=OpenFixture(&capture,"long-cue.vtt",false);REQUIRE(demux);
    const float invalid[]={0,-1,NAN,INFINITY,FLT_MAX};
    for(size_t i=0;i<ARRAY_SIZE(invalid);i++){float rate=invalid[i];CHECK(Query(demux,DEMUX_SET_RATE,&rate)==VLC_EGENERIC);vlc_tick_t length=-1;CHECK(Query(demux,DEMUX_GET_LENGTH,&length)==VLC_SUCCESS);CHECK(length==VLC_TICK_FROM_SEC(10));}
    CloseFixture(demux,&capture);
}
static void StreamModeUnchanged( void )
{
    struct capture capture;demux_t *demux=OpenFixture(&capture,"long-cue.vtt",true);REQUIRE(demux);
    bool can=true;float rate=2;CHECK(Query(demux,DEMUX_CAN_CONTROL_RATE,&can)==VLC_EGENERIC);CHECK(Query(demux,DEMUX_SET_RATE,&rate)==VLC_EGENERIC);
    for(unsigned i=0;i<20 && demux->pf_demux(demux)>0;i++);
    CHECK(capture.count==1);if(capture.count==1){CHECK(capture.blocks[0]->i_pts==VLC_TICK_0+VLC_TICK_FROM_SEC(2));CHECK(Contains(capture.blocks[0],"<00:00:04.000>"));CHECK(!Contains(capture.blocks[0],"<00:00:08.000>"));CheckBoxes(capture.blocks[0]);}
    CloseFixture(demux,&capture);
}
static void ZeroDuplicatesAndEmpty( void )
{
    struct capture capture;demux_t *demux=OpenFixture(&capture,"zero-duplicate.vtt",false);REQUIRE(demux);
    CHECK(Query(demux,DEMUX_SET_NEXT_DEMUX_TIME,VLC_TICK_0)==VLC_SUCCESS);
    float rate=.5f;CHECK(Query(demux,DEMUX_SET_RATE,&rate)==VLC_SUCCESS);
    CHECK(demux->pf_demux(demux)==VLC_DEMUXER_SUCCESS);CHECK(capture.count==1);
    if(capture.count==1){CHECK(capture.blocks[0]->i_pts==VLC_TICK_0);CHECK(Contains(capture.blocks[0],"zero"));}
    Clear(&capture);CHECK(Query(demux,DEMUX_SET_TIME,VLC_TICK_FROM_SEC(2),true)==VLC_SUCCESS);
    CHECK(Query(demux,DEMUX_SET_NEXT_DEMUX_TIME,VLC_TICK_0+VLC_TICK_FROM_SEC(2))==VLC_SUCCESS);
    CHECK(demux->pf_demux(demux)==VLC_DEMUXER_SUCCESS);CHECK(capture.count==1);
    if(capture.count==1){CHECK(capture.blocks[0]->i_pts==VLC_TICK_0+VLC_TICK_FROM_SEC(2));CHECK(capture.blocks[0]->i_length==VLC_TICK_FROM_MS(500));CHECK(Contains(capture.blocks[0],"after"));CHECK(!Contains(capture.blocks[0],"zero"));CHECK(!Contains(capture.blocks[0],"duplicate"));CheckBoxes(capture.blocks[0]);}
    CHECK(capture.pcrs==0);CloseFixture(demux,&capture);
    demux=OpenFixture(&capture,"empty.vtt",false);REQUIRE(demux);rate=2;
    CHECK(Query(demux,DEMUX_SET_RATE,&rate)==VLC_SUCCESS);CHECK(demux->pf_demux(demux)==VLC_DEMUXER_EOF);CHECK(capture.count==0);CloseFixture(demux,&capture);
}
static void Run( const char *name, void (*test)(void) )
{
    unsigned before=checks,failed=failures;test();printf("{\"case\":\"%s\",\"checks\":%u,\"failures\":%u,\"passed\":%s}\n",name,checks-before,failures-failed,failures==failed?"true":"false");
}
static void LargeLengthPosition( void )
{
    struct capture capture;demux_t *demux=OpenFixture(&capture,"large-length.vtt",false);REQUIRE(demux);
    float rate=1.5f;CHECK(Query(demux,DEMUX_SET_RATE,&rate)==VLC_SUCCESS);
    vlc_tick_t length=-1;CHECK(Query(demux,DEMUX_GET_LENGTH,&length)==VLC_SUCCESS);
    CHECK(length==INT64_C(9223372036854775500));
    CHECK(Query(demux,DEMUX_SET_POSITION,1.0,true)==VLC_SUCCESS);
    vlc_tick_t target=-1;CHECK(Query(demux,DEMUX_GET_TIME,&target)==VLC_SUCCESS);CHECK(target==length);
    CHECK(demux->pf_demux(demux)==VLC_DEMUXER_EOF);CHECK(capture.count==0);
    CloseFixture(demux,&capture);
}
int main( int argc, char **argv )
{
    if(argc!=2)return 2;
    fixture_root=argv[1];
    Run("actual-demux-containing-cue-refill",RefillInsideCue);
    Run("actual-demux-rate-inline-overlap-and-immutable-source",RateAndInline);
    Run("actual-demux-fractional-and-reset-without-cumulative-transform",ResetAndFractional);
    Run("actual-demux-eof-revival-and-no-wrap",EofRevivalAndNoWrap);
    Run("actual-demux-invalid-rates-atomic",InvalidRatesAreAtomic);
    Run("actual-stream-mode-rejects-rate-and-preserves-inline-times",StreamModeUnchanged);
    Run("actual-demux-zero-duplicate-boundaries-and-empty-file",ZeroDuplicatesAndEmpty);
    Run("actual-demux-large-length-exact-ending-position",LargeLengthPosition);
    printf("{\"summary\":true,\"cases\":8,\"checks\":%u,\"failures\":%u}\n",checks,failures);
    return failures?1:0;
}
