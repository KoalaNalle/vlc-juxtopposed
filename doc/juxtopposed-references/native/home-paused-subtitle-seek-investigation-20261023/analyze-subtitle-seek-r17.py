from pathlib import Path
from hashlib import sha256
from decimal import Decimal
import json,re,shutil
repo=Path('C:/Modding/vlc-juxtopposed');work=Path(__file__).resolve().parent
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
out=work/'subtitle-seek-analysis-r17';assert not out.exists()
rows=[]
for variant,native in [('original','original-wasapi'),('preserve','preserve-wasapi')]:
    packets=read(work/'subtitle-packet-probes-r17'/(variant+'-subtitle-0.json'))['packets']
    assert len(packets)==3
    for phase in ['native-core-trace-r9c','native-tracks-r15']:
        log_path=work/phase/native/'native.log'
        text=log_path.read_text(encoding='utf-8',errors='replace')
        for target in [25,5]:
            request='mkv debug: seek request to i_pos = '+str(target*1_000_000)
            assert text.count(request)==1
            start=text.index(request)
            next_seek=text.find('mkv debug: seek request to i_pos =',start+len(request))
            section=text[start:next_seek if next_seek!=-1 else len(text)]
            match=re.search(r'seek: preroll\{ req: \d+, start-pts: (\d+), start-fpos: (\d+)\}',section);assert match
            cue=next(p for p in packets if Decimal(p['pts_time'])<=target<Decimal(p['pts_time'])+Decimal(p['duration_time']))
            lower_bound=int(match.group(2));packet_pos=int(cue['pos'])
            before=packet_pos<lower_bound
            if phase=='native-core-trace-r9c':
                assert ('ENQUEUE cat=3 id=spu/4' in section)==(not before)
            rows.append(dict(variant=variant,nativePhase=phase,requestedSeconds=target,seekStartPtsUs=int(match.group(1)),seekStartFilePosition=lower_bound,activeCueStart=cue['pts_time'],activeCueDuration=cue['duration_time'],activeCuePacketPosition=packet_pos,activeCueBeforeSeekStart=before,expectedPresenceInRetainedPictures=not before,nativeLogSha256=digest(log_path),subtitleProbeSha256=digest(work/'subtitle-packet-probes-r17'/(variant+'-subtitle-0.json'))))
assert [(r['variant'],r['requestedSeconds'],r['activeCueBeforeSeekStart']) for r in rows[:4]]==[('original',25,False),('original',5,True),('original',25,False),('original',5,True)]
assert all(r['activeCueBeforeSeekStart'] for r in rows if r['variant']=='preserve')
for row in rows:
    result=read(work/row['nativePhase']/('original-wasapi' if row['variant']=='original' else 'preserve-wasapi')/'investigation.json')
    step='seek-middle-paused' if row['requestedSeconds']==25 else 'seek-first-paused'
    picture=next(r for r in result['observations'] if r['step']==step)
    assert picture['snapshotAvailable'] and (picture['brightPixels']>1000)==row['expectedPresenceInRetainedPictures']
    row['retainedSnapshotSha256']=picture['snapshotSha256']
    if row['nativePhase']=='native-core-trace-r9c':
        row['traceScope']='Exact decoder trace, separate diagnostic Core; packet position and seek line match the ordinary follow-up.'
    else:row['traceScope']='Ordinary Core000f, unchanged MKV plugin; snapshot/log evidence, without private logging.'
files=['modules/demux/mkv/matroska_segment.cpp','modules/demux/mkv/matroska_segment_seeker.cpp','modules/demux/mkv/matroska_segment_seeker.hpp','modules/demux/mkv/matroska_segment.hpp']
source_hashes={p:digest(repo/p) for p in files}
source=(repo/files[1]).read_text(encoding='utf-8')
assert 'if( sp.fpos < start_fpos )\n            continue;' in source
record=dict(passed=False,analysisComplete=True,repairAccepted=False,fullBetaAccepted=False,producerSha256=digest(Path(__file__)),ordinaryCoreSha256=digest(work/'ordinary-ui-r14/runtime/libvlccore.dll'),mkvPluginSha256=digest(work/'ordinary-ui-r14/runtime/plugins/demux/libmkv_plugin.dll'),sourceHashes=source_hashes,rows=rows,conclusion='Active subtitle cue packets precede the selected seek start in precisely the retained missing-cue controls. Source inspection finds the Matroska seekpoint search excludes per-track points before the video-led lower file-position bound. This is a source/evidence inference, not a validated repair.',limits=['FFprobe packet positions identify payload locations; native seek logs identify EBML block locations. Header offsets differ by a few bytes; the strict before/after comparisons have much larger margins.','The new ordinary Core retains the same subtitle presence pattern and seek locations; the selected startup repair did not fix these gaps.','A private Matroska candidate must be tested before changing production. Preserve precise video/audio seek points, codec preroll, subtitle timing and bounded scanning cost.','Track switching/associated subtitle loading while paused and the desktop/native rendering distinction require separate work.'])
out.mkdir()
for p in files:
    target=out/p;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(repo/p,target);assert digest(target)==source_hashes[p]
(out/'analysis.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps(dict(analysisComplete=True,repairAccepted=False,rows=rows),indent=2))
