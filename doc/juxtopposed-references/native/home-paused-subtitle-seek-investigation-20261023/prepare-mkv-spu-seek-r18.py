from pathlib import Path
from hashlib import sha256
import json,shutil,subprocess
repo=Path('C:/Modding/vlc-juxtopposed');work=Path(__file__).resolve().parent
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
inputs=read(work/'inputs-r14.json');assert all(digest(repo/p)==h for p,h in inputs.items())
relative='modules/demux/mkv/matroska_segment.cpp';source=repo/relative
original=source.read_text(encoding='utf-8')
before='''    if ( i_seek_position == std::numeric_limits<SegmentSeeker::fptr_t>::max() )
        return false;
'''
addition='''    /* A selected subtitle may still be active even when its block precedes
     * the video-led seek range. Re-read from its last known seekpoint without
     * moving the audio/video time anchor or their per-track skip positions. */
    if ( b_accurate )
    {
        for( const auto track_id : selected_tracks )
        {
            auto track_it = tracks.find( track_id );
            if( track_it == tracks.end() || track_it->second->fmt.i_cat != SPU_ES )
                continue;

            auto points_it = _seeker._tracks_seekpoints.find( track_id );
            if( points_it == _seeker._tracks_seekpoints.end() || points_it->second.empty() )
                continue;

            const auto point = _seeker.get_first_seekpoint_around( i_mk_date, points_it->second );
            if( point.trust_level < SegmentSeeker::Seekpoint::QUESTIONABLE || point.pts > i_mk_date )
                continue;

            track_it->second->i_skip_until_fpos = point.fpos;
            i_seek_position = std::min( i_seek_position, point.fpos );
        }
    }

'''
assert original.count(before)==1
out=work/'mkv-spu-seek-r18-source';assert not out.exists();out.mkdir()
(out/'matroska_segment-before.cpp').write_bytes(source.read_bytes())
(out/'matroska_segment-after.cpp').write_text(original.replace(before,addition+before),encoding='utf-8',newline='\n')
files=set(inputs)
files.update(subprocess.check_output(['git','ls-files','--','modules/demux/mkv'],cwd=repo,text=True).splitlines())
files.update(['modules/packetizer/dts_header.c','modules/packetizer/dts_header.h'])
for name in ['prepare-mkv-spu-seek-r18.py','mkv-spu-seek-r18.mk','build-mkv-spu-seek-r18.sh']:
    files.add('build-windows/beta-tracks-subtitles-20261023/'+name)
protected={'test/libvlc/native_soxr_reference.py','test/modules/audio_filter/soxr_lifecycle_trace.c','test/modules/audio_filter/soxr_native_reference.c','test/modules/audio_filter/soxr_pcm_trace.c'}
assert not files&protected
hashes={p:digest(repo/p) for p in sorted(files)}
snapshot=work/'source-mkv-r18';assert not snapshot.exists();snapshot.mkdir()
for p,h in hashes.items():
    target=snapshot/p;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(repo/p,target);assert digest(target)==h
record=dict(candidateOnly=True,repairAccepted=False,productionSourceUnchanged=digest(source)==hashes[relative],producerSha256=digest(Path(__file__)),beforeSha256=digest(out/'matroska_segment-before.cpp'),afterSha256=digest(out/'matroska_segment-after.cpp'),files=hashes,sourceCount=len(hashes),sourceSnapshot=str(snapshot),scope='Private precise-seek candidate replays selected SPU packets from their previous indexed point while preserving ordinary AV time/skip anchors. Scan cost, cold seeking and broader cue correctness remain unqualified.')
(out/'source-record.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps({k:v for k,v in record.items() if k!='files'},indent=2))
