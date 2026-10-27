from pathlib import Path
from hashlib import sha256
import json,shutil,subprocess
repo=Path('C:/Modding/vlc-juxtopposed');work=Path(__file__).resolve().parent
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
prior=read(work/'mkv-spu-seek-r18b-source/source-record.json')['files']
assert all(digest(repo/p)==h for p,h in prior.items())
root=work/'mkv-interval-r23c-source';assert not root.exists();root.mkdir()
mkv_files=subprocess.check_output(['git','ls-files','--','modules/demux/mkv'],cwd=repo,text=True).splitlines()
for phase in ['before','after']:
    out=root/phase;out.mkdir()
    for relative in mkv_files:
        target=out/Path(relative).name;shutil.copyfile(repo/relative,target)
def change(name,before,after):
    p=root/'after'/name;text=p.read_text(encoding='utf-8');assert text.count(before)==1,(name,before,text.count(before));p.write_text(text.replace(before,after),encoding='utf-8',newline='\n')

change('matroska_segment_seeker.hpp','''        struct Cluster {''','''        struct SubtitleBlock {
            fptr_t fpos;
            vlc_tick_t pts;
            vlc_tick_t end_pts;
        };

        struct Cluster {''')
change('matroska_segment_seeker.hpp','''        void add_seekpoint( track_id_t, Seekpoint );''','''        void add_seekpoint( track_id_t, Seekpoint );
        void add_subtitle_block( track_id_t, fptr_t, vlc_tick_t, vlc_tick_t ) noexcept;
        bool index_subtitles( matroska_segment_c&, fptr_t );
        fptr_t get_subtitle_seekpoint( track_id_t, long double, fptr_t ) const;''')
change('matroska_segment_seeker.hpp','''        cluster_map_t       _clusters;''','''        cluster_map_t       _clusters;
        std::map<track_id_t, std::vector<SubtitleBlock>> _subtitle_blocks;
        fptr_t _subtitle_first_cluster = std::numeric_limits<fptr_t>::max();
        fptr_t _subtitle_indexed_through = std::numeric_limits<fptr_t>::max();
        bool _subtitle_index_available = true;''')
change('matroska_segment_seeker.cpp','''#include <sstream>''','''#include <vlc_interrupt.h>

#include <sstream>''')
functions='''void
SegmentSeeker::add_subtitle_block( track_id_t track_id, fptr_t fpos,
                                  vlc_tick_t pts, vlc_tick_t end_pts ) noexcept
{
    if( !_subtitle_index_available )
        return;
    try {
        auto &blocks = _subtitle_blocks[track_id];
        auto it = std::lower_bound( blocks.begin(), blocks.end(), fpos,
            [](const SubtitleBlock &block, fptr_t pos) { return block.fpos < pos; } );
        const SubtitleBlock block = { fpos, pts, end_pts };
        if( it != blocks.end() && it->fpos == fpos )
            *it = block;
        else
            blocks.insert( it, block );
    }
    catch( const std::exception& ) {
        /* Keep playback usable if the optional index cannot be retained.
         * Precise seeks then replay from the first cluster instead. */
        _subtitle_index_available = false;
        _subtitle_blocks.clear();
    }
}

bool
SegmentSeeker::index_subtitles( matroska_segment_c &ms, fptr_t end_fpos )
{
    const fptr_t unset = std::numeric_limits<fptr_t>::max();
    if( !_subtitle_index_available || _subtitle_first_cluster == unset
     || end_fpos <= _subtitle_first_cluster
     || (_subtitle_indexed_through != unset && _subtitle_indexed_through >= end_fpos) )
        return true;

    /* Extend one contiguous prefix from a known block boundary. Re-reading
     * its final block avoids seeking into an EBML element's payload. */
    const fptr_t start = _subtitle_indexed_through == unset
                      ? _subtitle_first_cluster : _subtitle_indexed_through;
    mkv_jump_to( ms, start );
    fptr_t previous = unset;
    while( !vlc_killed() )
    {
        KaxBlock *block = nullptr;
        KaxSimpleBlock *simpleblock = nullptr;
        KaxBlockAdditions *additions = nullptr;
        bool key, discardable;
        uint64_t duration;
        if( ms.BlockGet( block, simpleblock, additions, &key, &discardable, &duration ) )
        {
            delete additions;
            return false;
        }
        const KaxInternalBlock &data = simpleblock
            ? static_cast<const KaxInternalBlock&>( *simpleblock )
            : static_cast<const KaxInternalBlock&>( *block );
        const fptr_t pos = data.GetElementPosition();
        delete block;
        delete additions;
        if( previous != unset && pos <= previous )
            return false;
        previous = pos;
        if( !_subtitle_index_available )
            return true;
        if( _subtitle_indexed_through == unset || pos > _subtitle_indexed_through )
            _subtitle_indexed_through = pos;
        if( pos >= end_fpos )
            return true;
    }
    return false;
}

SegmentSeeker::fptr_t
SegmentSeeker::get_subtitle_seekpoint( track_id_t track_id, long double target_pts,
                                      fptr_t end_fpos ) const
{
    const fptr_t unset = std::numeric_limits<fptr_t>::max();
    if( !_subtitle_index_available )
        return _subtitle_first_cluster;
    const auto track = _subtitle_blocks.find( track_id );
    if( track == _subtitle_blocks.end() )
        return unset;
    for( const SubtitleBlock &block : track->second )
    {
        if( block.fpos >= end_fpos )
            break;
        /* Unknown durations remain conservative; the subtitle decoder and
         * renderer decide whether such a packet is still active. */
        if( block.pts <= target_pts
         && (block.end_pts == VLC_TICK_INVALID || block.end_pts >= target_pts) )
            return block.fpos;
    }
    return unset;
}

'''
change('matroska_segment_seeker.cpp','''void
SegmentSeeker::add_seekpoint( track_id_t track_id, Seekpoint sp )''',functions+'''void
SegmentSeeker::add_seekpoint( track_id_t track_id, Seekpoint sp )''')
change('matroska_segment.cpp','''                // add first cluster as trusted seekpoint for all tracks''','''                _seeker._subtitle_first_cluster = cluster->GetElementPosition();

                // add first cluster as trusted seekpoint for all tracks''')
point='''            return VLC_SUCCESS;
        }

        i_level = ep.GetLevel();'''
record='''            if( p_track->fmt.i_cat == SPU_ES )
            {
                const KaxInternalBlock &data = pp_simpleblock
                    ? static_cast<const KaxInternalBlock&>( *pp_simpleblock )
                    : static_cast<const KaxInternalBlock&>( *pp_block );
                const vlc_tick_t pts = VLC_TICK_FROM_NS(data.GlobalTimestamp());
                vlc_tick_t end_pts = VLC_TICK_INVALID;
                /* Match send_Block's duration units. Unknown or unrepresentable
                 * spans stay conservative rather than overflowing a date. */
                const long double span = static_cast<long double>(*pi_duration)
                                       * p_track->f_timecodescale * i_timescale
                                       * CLOCK_FREQ / 1000000000.L;
                const long double stop = static_cast<long double>(pts) + span;
                if( !p_track->b_no_duration && span >= 1
                 && stop >= static_cast<long double>(VLC_TICK_MIN)
                 && stop <= static_cast<long double>(VLC_TICK_MAX) )
                    end_pts = static_cast<vlc_tick_t>(stop);
                _seeker.add_subtitle_block( p_track->i_number, data.GetElementPosition(),
                                            pts, end_pts );
            }

'''
change('matroska_segment.cpp',point,record+point)
point='''    if ( i_seek_position == std::numeric_limits<SegmentSeeker::fptr_t>::max() )
        return false;
'''
seek='''
    if( b_accurate )
    {
        const bool subtitles = std::any_of( selected_tracks.begin(), selected_tracks.end(),
            [this](SegmentSeeker::track_id_t id) {
                const auto track = tracks.find(id);
                return track != tracks.end() && track->second->fmt.i_cat == SPU_ES;
            } );
        if( subtitles )
        {
            const auto av_seek_position = i_seek_position;
            try {
                if( !_seeker.index_subtitles( *this, av_seek_position ) )
                    return false;
                for( const auto id : selected_tracks )
                {
                    auto track = tracks.find(id);
                    if( track == tracks.end() || track->second->fmt.i_cat != SPU_ES )
                        continue;
                    const long double date = static_cast<long double>(i_mk_date)
                                           + track->second->i_codec_delay;
                    const auto pos = _seeker.get_subtitle_seekpoint( id, date, av_seek_position );
                    if( pos != std::numeric_limits<SegmentSeeker::fptr_t>::max() )
                    {
                        track->second->i_skip_until_fpos = pos;
                        i_seek_position = std::min( i_seek_position, pos );
                    }
                }
            }
            catch( const std::exception &e ) {
                msg_Err( &sys.demuxer, "error indexing subtitles during seek: %s", e.what() );
                return false;
            }
        }
    }
'''
change('matroska_segment.cpp',point,point+seek)

files=set(prior)
files.update(['include/vlc_interrupt.h','modules/demux/Makefile.am','modules/demux/vobsub.h','modules/demux/av1_unpack.h','modules/codec/webvtt/helpers.h','modules/demux/windows_audio_commons.h'])
for name in ['prepare-mkv-interval-r23c.py','mkv-interval-r23c.mk','build-mkv-interval-r23c.sh']:
    files.add('build-windows/beta-tracks-subtitles-20261023/'+name)
hashes={p:digest(repo/p) for p in sorted(files)}
snapshot=work/'source-mkv-interval-r23c';assert not snapshot.exists();snapshot.mkdir()
for p,h in hashes.items():
    dest=snapshot/p;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(repo/p,dest);assert digest(dest)==h
record=dict(candidateOnly=True,repairAccepted=False,producerSha256=digest(Path(__file__)),sourceCount=len(hashes),files=hashes,sourceSnapshot=str(snapshot),privateSources={phase:{p.name:digest(p) for p in sorted((root/phase).iterdir()) if p.is_file()} for phase in ['before','after']},changedPrivateFiles=['matroska_segment.cpp','matroska_segment_seeker.cpp','matroska_segment_seeker.hpp'],scope='Private contiguous subtitle block-span index; all sixteen Matroska C++ units must be rebuilt together in each phase because SegmentSeeker layout changes. Production source/runtime stay untouched.')
(root/'source-record.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps({k:v for k,v in record.items() if k not in ['files','privateSources']},indent=2))
