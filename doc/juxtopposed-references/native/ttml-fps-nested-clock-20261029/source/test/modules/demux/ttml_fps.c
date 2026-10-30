// SPDX-License-Identifier: LGPL-2.1-or-later
/* Actual TTML parser, controls, demux packets and parser/serializer round trip. */
#include "../../libvlc/test.h"
#include "../../../lib/libvlc_internal.h"
const char vlc_module_name[] = "ttml_fps_test";
#include "../../../modules/demux/ttml.c"
#include <float.h>

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
static libvlc_instance_t *instance;

static bool check(bool value, const char *expression, unsigned line)
{
    checks++;
    if (!value) { failures++; fprintf(stderr,"line %u: %s\n",line,expression); }
    return value;
}
#define CHECK(x) check(!!(x),#x,__LINE__)
#define REQUIRE(x) do { if (!CHECK(x)) return; } while (0)

/* The unchanged parser translation unit contains a module descriptor. These
 * callbacks satisfy its unused PE references; selecting one fails this test.
 * Decoder rendering is qualified separately by the actual plugin matrix. */
int tt_OpenDecoder(vlc_object_t *object)
{ (void)object; CHECK(false); return VLC_EGENERIC; }
#ifdef ENABLE_SOUT
int tt_OpenEncoder(vlc_object_t *object)
{ (void)object; CHECK(false); return VLC_EGENERIC; }
#endif

static es_out_id_t *Add(es_out_t *out, input_source_t *source, const es_format_t *fmt)
{
    (void)source;
    struct capture *capture = (struct capture *)out;
    CHECK(fmt->i_codec == VLC_CODEC_TTML);
    return (es_out_id_t *)&capture->es;
}
static int Send(es_out_t *out, es_out_id_t *id, block_t *block)
{
    struct capture *capture = (struct capture *)out;
    CHECK(id == (es_out_id_t *)&capture->es);
    if (!CHECK(capture->count < ARRAY_SIZE(capture->blocks)))
    { block_Release(block); return VLC_EGENERIC; }
    capture->blocks[capture->count++] = block;
    return VLC_SUCCESS;
}
static void Del(es_out_t *out, es_out_id_t *id)
{
    CHECK(id == (es_out_id_t *)&((struct capture *)out)->es);
}
static int OutControl(es_out_t *out, input_source_t *source, int query, va_list args)
{
    (void)source; (void)args;
    if (query == ES_OUT_SET_PCR || query == ES_OUT_SET_GROUP_PCR)
        ((struct capture *)out)->pcrs++;
    return VLC_SUCCESS;
}
static const struct es_out_callbacks callbacks = { .add=Add,.send=Send,.del=Del,.control=OutControl };
static void Clear(struct capture *capture)
{
    for (size_t i=0; i<capture->count; ++i) block_Release(capture->blocks[i]);
    capture->count=0;
}
static demux_t *OpenMemory(struct capture *capture, const void *bytes, size_t length)
{
    memset(capture,0,sizeof(*capture)); capture->out.cbs=&callbacks;
    demux_t *demux=(vlc_object_create)(VLC_OBJECT(instance->p_libvlc_int),sizeof(*demux));
    if (!CHECK(demux != NULL)) return NULL;
    uint8_t *copy=malloc(length);
    if (!CHECK(copy != NULL)) { vlc_object_delete(demux); return NULL; }
    memcpy(copy,bytes,length);
    demux->s=vlc_stream_MemoryNew(VLC_OBJECT(demux),copy,length,false);
    demux->out=&capture->out;
    if (!CHECK(demux->s != NULL)) { free(copy); vlc_object_delete(demux); return NULL; }
    if (!CHECK(tt_OpenDemux(VLC_OBJECT(demux)) == VLC_SUCCESS))
    { vlc_stream_Delete(demux->s); vlc_object_delete(demux); return NULL; }
    return demux;
}
static demux_t *OpenFixture(struct capture *capture, const char *name)
{
    char path[2048]; snprintf(path,sizeof(path),"%s/%s",fixture_root,name);
    FILE *file=fopen(path,"rb"); if (!CHECK(file != NULL)) return NULL;
    fseek(file,0,SEEK_END); long size=ftell(file); rewind(file);
    uint8_t *data=malloc(size);
    if (!CHECK(data != NULL)) { fclose(file); return NULL; }
    bool loaded=fread(data,1,size,file)==(size_t)size; fclose(file);
    demux_t *demux=CHECK(loaded) ? OpenMemory(capture,data,size) : NULL;
    free(data); return demux;
}
static void CloseFixture(demux_t *demux, struct capture *capture)
{
    Clear(capture); tt_CloseDemux(VLC_OBJECT(demux));
    vlc_stream_Delete(demux->s); vlc_object_delete(demux);
}
static int Query(demux_t *demux, int query, ...)
{
    va_list args; va_start(args,query);
    int status=demux->pf_control(demux,query,args); va_end(args); return status;
}
static bool Contains(const block_t *block, const char *text)
{
    const size_t length=strlen(text);
    for (size_t i=0; i+length<=block->i_buffer; ++i)
        if (!memcmp(block->p_buffer+i,text,length)) return true;
    return false;
}
static void ContainingInterval(void)
{
    struct capture capture; demux_t *demux=OpenFixture(&capture,"flat.ttml"); REQUIRE(demux);
    CHECK(Query(demux,DEMUX_SET_TIME,VLC_TICK_FROM_SEC(12),true)==VLC_SUCCESS);
    CHECK(Query(demux,DEMUX_SET_NEXT_DEMUX_TIME,VLC_TICK_0+VLC_TICK_FROM_SEC(12))==VLC_SUCCESS);
    CHECK(demux->pf_demux(demux)==VLC_DEMUXER_SUCCESS); CHECK(capture.count==1);
    if (capture.count==1)
    {
        block_t *block=capture.blocks[0];
        CHECK(block->i_pts==VLC_TICK_0+VLC_TICK_FROM_SEC(10));
        CHECK(block->i_length==VLC_TICK_FROM_SEC(10));
        CHECK(block->i_flags & BLOCK_FLAG_DISCONTINUITY);
        CHECK(Contains(block,"BETA")); CHECK(!Contains(block,"ALPHA"));
        CHECK(Contains(block,"<body begin=\"00:00:10.000000\" end=\"00:00:20.000000\""));
    }
    CHECK(capture.pcrs==0); CloseFixture(demux,&capture);
}
static void RatesAndReset(void)
{
    struct capture capture; demux_t *demux=OpenFixture(&capture,"flat.ttml"); REQUIRE(demux);
    demux_sys_t *sys=demux->p_sys; bool can=false;
    CHECK(Query(demux,DEMUX_CAN_CONTROL_RATE,&can)==VLC_SUCCESS && can);
    size_t bytes=sys->times.i_count*sizeof(*sys->times.p_array);
    void *original=malloc(bytes); REQUIRE(original); memcpy(original,sys->times.p_array,bytes);
    const float rates[]={.5f,2.f,1.25f,1.f};
    const vlc_tick_t starts[]={5000000,20000000,12500000,10000000};
    const vlc_tick_t lengths[]={25000000,100000000,62500000,50000000};
    for (size_t i=0; i<ARRAY_SIZE(rates); ++i)
    {
        Clear(&capture); float rate=rates[i]; vlc_tick_t length=-1;
        CHECK(Query(demux,DEMUX_SET_NEXT_DEMUX_TIME,VLC_TICK_0+starts[i]+1000000)==VLC_SUCCESS);
        CHECK(Query(demux,DEMUX_SET_RATE,&rate)==VLC_SUCCESS);
        CHECK(Query(demux,DEMUX_GET_LENGTH,&length)==VLC_SUCCESS && length==lengths[i]);
        CHECK(demux->pf_demux(demux)==VLC_DEMUXER_SUCCESS); CHECK(capture.count==1);
        if (capture.count==1)
        {
            CHECK(capture.blocks[0]->i_pts==VLC_TICK_0+starts[i]);
            CHECK(capture.blocks[0]->i_length==starts[i]);
            CHECK(Contains(capture.blocks[0],"BETA"));
        }
        CHECK(!memcmp(original,sys->times.p_array,bytes));
    }
    free(original); CHECK(capture.pcrs==0); CloseFixture(demux,&capture);
}
static void InvalidControlsAtomic(void)
{
    struct capture capture; demux_t *demux=OpenFixture(&capture,"flat.ttml"); REQUIRE(demux);
    demux_sys_t *sys=demux->p_sys; unsigned char before[sizeof(*sys)]; memcpy(before,sys,sizeof(*sys));
    const float rates[]={0,-1,NAN,INFINITY,FLT_MAX};
    for (size_t i=0; i<ARRAY_SIZE(rates); ++i)
    { float rate=rates[i]; CHECK(Query(demux,DEMUX_SET_RATE,&rate)==VLC_EGENERIC); CHECK(!memcmp(before,sys,sizeof(*sys))); }
    const double positions[]={-1,1.01,NAN,INFINITY};
    for (size_t i=0; i<ARRAY_SIZE(positions); ++i)
    { CHECK(Query(demux,DEMUX_SET_POSITION,positions[i],true)==VLC_EGENERIC); CHECK(!memcmp(before,sys,sizeof(*sys))); }
    CHECK(Query(demux,DEMUX_SET_TIME,VLC_TICK_MIN,true)==VLC_EGENERIC); CHECK(!memcmp(before,sys,sizeof(*sys)));
    CHECK(Query(demux,DEMUX_SET_NEXT_DEMUX_TIME,VLC_TICK_MIN)==VLC_EGENERIC); CHECK(!memcmp(before,sys,sizeof(*sys)));
    CloseFixture(demux,&capture);
}
static void EofRevival(void)
{
    struct capture capture; demux_t *demux=OpenFixture(&capture,"flat.ttml"); REQUIRE(demux);
    float rate=.5f;
    CHECK(Query(demux,DEMUX_SET_TIME,VLC_TICK_FROM_SEC(40),true)==VLC_SUCCESS);
    CHECK(Query(demux,DEMUX_SET_NEXT_DEMUX_TIME,VLC_TICK_0+VLC_TICK_FROM_SEC(40))==VLC_SUCCESS);
    CHECK(Query(demux,DEMUX_SET_RATE,&rate)==VLC_SUCCESS);
    CHECK(demux->pf_demux(demux)==VLC_DEMUXER_EOF); CHECK(capture.count==0);
    rate=1.f; CHECK(Query(demux,DEMUX_SET_RATE,&rate)==VLC_SUCCESS);
    CHECK(demux->pf_demux(demux)==VLC_DEMUXER_EOF); CHECK(capture.count==1);
    if (capture.count==1) CHECK(Contains(capture.blocks[0],"GAMMA"));
    Clear(&capture);
    CHECK(Query(demux,DEMUX_SET_POSITION,1.0,true)==VLC_SUCCESS);
    CHECK(demux->pf_demux(demux)==VLC_DEMUXER_EOF); CHECK(capture.count==0);
    vlc_tick_t time=-1; CHECK(Query(demux,DEMUX_GET_TIME,&time)==VLC_SUCCESS && time==50000000);
    CloseFixture(demux,&capture);
}
static void NestedRoundTrip(void)
{
    const char *names[]={"nested-par.ttml","nested-seq.ttml"};
    for (size_t i=0; i<ARRAY_SIZE(names); ++i)
    {
        struct capture capture; demux_t *demux=OpenFixture(&capture,names[i]); REQUIRE(demux);
        CHECK(Query(demux,DEMUX_SET_TIME,VLC_TICK_FROM_SEC(12),true)==VLC_SUCCESS);
        CHECK(Query(demux,DEMUX_SET_NEXT_DEMUX_TIME,VLC_TICK_0+VLC_TICK_FROM_SEC(12))==VLC_SUCCESS);
        CHECK(demux->pf_demux(demux)==VLC_DEMUXER_EOF); CHECK(capture.count==1);
        if (capture.count==1)
        {
            block_t *block=capture.blocks[0]; CHECK(Contains(block,"BETA")); CHECK(!Contains(block,"ALPHA"));
            CHECK(block->i_pts==VLC_TICK_0+10000000 && block->i_length==10000000);
            struct capture again; demux_t *round=OpenMemory(&again,block->p_buffer,block->i_buffer); REQUIRE(round);
            demux_sys_t *sys=round->p_sys; CHECK(sys->times.i_count==3);
            if (sys->times.i_count==3)
            { CHECK(tt_time_Convert(&sys->times.p_array[1])==10000000); CHECK(tt_time_Convert(&sys->times.p_array[2])==20000000); }
            CloseFixture(round,&again);
        }
        CloseFixture(demux,&capture);
    }
}
static void Run(const char *name, void (*test)(void))
{
    unsigned before=checks, failed=failures; test();
    printf("{\"case\":\"%s\",\"checks\":%u,\"failures\":%u}\n",name,checks-before,failures-failed);
}
static void MixedClockAndFrame(void)
{
    /* .5 second + 15 frames at 30 FPS + 9 seconds starts at second 10. */
    struct capture capture; demux_t *demux=OpenFixture(&capture,"mixed-clock-frame.ttml"); REQUIRE(demux);
    CHECK(Query(demux,DEMUX_SET_TIME,INT64_C(10500000),true)==VLC_SUCCESS);
    CHECK(Query(demux,DEMUX_SET_NEXT_DEMUX_TIME,VLC_TICK_0+10500000)==VLC_SUCCESS);
    CHECK(demux->pf_demux(demux)==VLC_DEMUXER_EOF); CHECK(capture.count==1);
    if (capture.count==1)
    {
        CHECK(capture.blocks[0]->i_pts==VLC_TICK_0+10000000);
        CHECK(capture.blocks[0]->i_length==10000000);
        CHECK(Contains(capture.blocks[0],"BETA"));
    }
    tt_time_t first={.base=500000,.frames=15},second={.base=0,.frames=20};
    tt_time_t sum=tt_time_Add(first,second);
    CHECK(tt_time_Convert(&sum)==1666666);
    CloseFixture(demux,&capture);
}
int main(int argc, char **argv)
{
    if (argc!=2) return 77;
    fixture_root=argv[1]; test_init();
    const char *args[]={"--ignore-config","--no-media-library","--no-metadata-network-access","--no-ml-network-discovery"};
    instance=libvlc_new(ARRAY_SIZE(args),args); assert(instance);
    Run("containing-interval-precise-seek",ContainingInterval);
    Run("rates-reset-immutable-source-index",RatesAndReset);
    Run("invalid-controls-atomic",InvalidControlsAtomic);
    Run("eof-reset-and-ending-position",EofRevival);
    Run("nested-parallel-sequential-parser-roundtrip",NestedRoundTrip);
    Run("mixed-clock-frame-parent-offset-and-carry",MixedClockAndFrame);
    libvlc_release(instance);
    printf("{\"summary\":true,\"cases\":6,\"checks\":%u,\"failures\":%u}\n",checks,failures);
    return failures ? 1 : 0;
}
