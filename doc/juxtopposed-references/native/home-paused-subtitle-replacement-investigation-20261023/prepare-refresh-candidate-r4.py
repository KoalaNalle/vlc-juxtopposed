from pathlib import Path
from hashlib import sha256
import ast,json
work=Path(__file__).resolve().parent
repo=work.parent.parent
text=(work/'refresh-candidate-r3/input.c').read_text(encoding='utf-8')
before='''                    bool seeking = true;
                    vlc_tick_t time;
                    if (vlc_player_GetTimerPoint(player, &seeking,
                                                VLC_TICK_INVALID, &time, NULL)
                        == VLC_SUCCESS && time >= VLC_TICK_0)'''
after='''                    const vlc_tick_t time =
                        vlc_player_input_GetTime(input, true, vlc_tick_now());
                    if (time >= VLC_TICK_0)'''
assert text.count(before)==1;text=text.replace(before,after)
folder=work/'refresh-candidate-r4';assert not folder.exists();folder.mkdir()
(folder/'input.c').write_text(text,encoding='utf-8',newline='\n')
for a,b in [('refresh-candidate-r3.mk','refresh-candidate-r4.mk'),('build-refresh-core-r3.sh','build-refresh-core-r4.sh')]:
    content=(work/a).read_text(encoding='utf-8').replace('r3','r4')
    (work/b).write_text(content,encoding='utf-8',newline='\n')
worker=(work/'native-tracks-r3.py').read_text(encoding='utf-8')
worker=worker.replace("sourceManifest='build-windows/beta-subtitle-workflows-20261023/inputs-r3.json'", "sourceManifest='build-windows/beta-subtitle-workflows-20261023/inputs-r4.json'")
before="    observe('played-paused')"
after='''    observe('played-paused')
    normalTracks=tracks();normalFinnish=next(t for t in normalTracks if t['language']=='fin');normalEnglish=next(t for t in normalTracks if t['language']=='eng')
    select(player,2,normalFinnish['id'].encode());wait(lambda:any(t['id']==normalFinnish['id'] and t['selected'] for t in tracks()),3);time.sleep(.7);observe('normal-pause-finnish')
    select(player,2,normalEnglish['id'].encode());wait(lambda:any(t['id']==normalEnglish['id'] and t['selected'] for t in tracks()),3);time.sleep(.7);observe('normal-pause-english')'''
assert worker.count(before)==1;worker=worker.replace(before,after)
ast.parse(worker);(work/'native-tracks-r4.py').write_text(worker,encoding='utf-8',newline='\n')
parent=(work/'run-r3.py').read_text(encoding='utf-8').replace('r3','r4')
# Keep both rejected candidate sources in the evidence union.
parent=parent.replace("['buffer-candidate-r2/decoder.c','refresh-candidate-r4/input.c']", "['buffer-candidate-r2/decoder.c','refresh-candidate-r3/input.c','refresh-candidate-r4/input.c']")
ast.parse(parent);(work/'run-r4.py').write_text(parent,encoding='utf-8',newline='\n')
record=dict(productionApplied=False,candidatePlayerInputSha256=sha256((folder/'input.c').read_bytes()).hexdigest(),r2='Rejected: queued-SPU-only candidate changes none of the eight observed steps in any of three cases.',r3='Not executed. Raw timer points can be old during an ordinary play-then-pause. Baseline watch data shows a 21ms raw point with an approximately 1s paused clock; use the existing seek-aware interpolated query instead.',r4='Use existing seek-aware time query: pending paused seeks retain requested target, ordinary pauses interpolate to pause date. Add pre-seek Finnish/English switches to both controls and candidate. No production application or full acceptance yet.')
(work/'refresh-candidate-r4.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps(record))
