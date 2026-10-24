// SPDX-License-Identifier: LGPL-2.1-or-later
// Compile with the extracted production helper, not a duplicate implementation.
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#define VLC_SUCCESS 0
#define VLC_EGENERIC (-1)
#define unlikely(x) (x)
typedef struct {
    uint32_t i_sample_count, i_sample_first;
    uint64_t i_first_dts;
    uint32_t i_entries_dts;
    uint32_t *p_sample_count_dts, *p_sample_delta_dts;
} mp4_chunk_t;
typedef struct {uint32_t i_chunk_count,i_sample_count;mp4_chunk_t *chunk;} mp4_track_t;
#include "mp4_seek_under_test.h"

static unsigned checks, failures;
static void sample(mp4_track_t *track,uint64_t dts,int result,uint32_t chunk,uint32_t index)
{
    uint32_t actual_chunk=UINT32_MAX,actual_sample=UINT32_MAX;
    const int actual=STTSToSampleChunk(track,dts,&actual_chunk,&actual_sample);
    ++checks;
    if(actual!=result || (result==VLC_SUCCESS && (actual_chunk!=chunk || actual_sample!=index))){
        ++failures;fprintf(stderr,"dts=%llu result=%d chunk=%u sample=%u; expected=%d/%u/%u\n",(unsigned long long)dts,actual,actual_chunk,actual_sample,result,chunk,index);
    }
}
int main(void)
{
    // CFR chunks match the long-GOP native fixture's sample-index boundary.
    uint32_t counts[]={457,457,457,129},delta=1;
    mp4_chunk_t chunks[4];
    for(unsigned i=0;i<4;++i)chunks[i]=(mp4_chunk_t){counts[i],i*457,i*457,1,&counts[i],&delta};
    mp4_track_t track={4,1500,chunks};
    for(uint32_t i=0;i<1500;++i)sample(&track,i,VLC_SUCCESS,i/457,i);
    sample(&track,1500,VLC_EGENERIC,0,0);
    sample(&track,2000,VLC_EGENERIC,0,0);
    // A variable-duration later chunk exercises both DTS entries and their seam.
    uint32_t first_count=100,first_delta=2,mixed_counts[]={3,7},mixed_deltas[]={2,4};
    mp4_chunk_t mixed[]={{100,0,0,1,&first_count,&first_delta},{10,100,200,2,mixed_counts,mixed_deltas}};
    mp4_track_t variable={2,110,mixed};
    for(uint64_t t=200;t<234;++t){uint32_t expected=t<206?100+(t-200)/2:103+(t-206)/4;sample(&variable,t,VLC_SUCCESS,1,expected);}
    sample(&variable,234,VLC_EGENERIC,0,0);
    // A nonzero base smaller than the local count used to exit prematurely
    // between DTS runs, even though it entered the loop for the first run.
    uint32_t small_first_count=3,small_first_delta=10;
    uint32_t small_counts[]={2,3},small_deltas[]={10,20};
    mp4_chunk_t small[]={{3,0,0,1,&small_first_count,&small_first_delta},{5,3,30,2,small_counts,small_deltas}};
    mp4_track_t early={2,8,small};
    for(uint64_t t=30;t<110;++t){uint32_t expected=t<50?3+(t-30)/10:5+(t-50)/20;sample(&early,t,VLC_SUCCESS,1,expected);}
    sample(&early,110,VLC_EGENERIC,0,0);
    // A high absolute base must not be compared to the small local count or
    // require adding base+count in the loop guard.
    uint32_t high_count=5;
    mp4_chunk_t high={5,UINT32_MAX-5,0,1,&high_count,&delta};mp4_track_t large={1,UINT32_MAX,&high};
    for(uint32_t i=0;i<5;++i)sample(&large,i,VLC_SUCCESS,0,UINT32_MAX-5+i);
    sample(&large,5,VLC_EGENERIC,0,0);
    printf("{\"checks\":%u,\"failures\":%u}\n",checks,failures);
    return failures?1:0;
}
