from pathlib import Path
from hashlib import sha256
import json,shutil
work=Path(__file__).resolve().parent;repo=work.parent.parent
prior=repo/'build-windows/beta-subtitle-frame-feedback-20261023'
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
source=(repo/'test/libvlc/native_paused_subtitle_tracks.py').read_text(encoding='utf-8')
source=source.replace("parser.add_argument('--frame-feedback-abi',type=Path)","parser.add_argument('--frame-feedback-abi',type=Path)\nparser.add_argument('--extra-paused-mode',choices=['steps','hold'],required=True)")
source=source.replace("entry=dict(step=name,state=state(player)","entry=dict(observedAt=time.monotonic(),step=name,state=state(player)")
old="""                nextFrame(player)
                wait(lambda:len(frameEvents)>beforeCount,3)
                time.sleep(.3);observe('requested-next-'+str(number))"""
new="""                if args.extra_paused_mode=='steps':
                    nextFrame(player)
                    wait(lambda:len(frameEvents)>beforeCount,3)
                else:
                    time.sleep(.05)
                time.sleep(.3);observe(args.extra_paused_mode+'-'+str(number))"""
assert source.count(old)==1;source=source.replace(old,new)
old="""    checks['requestedFrameFeedback']=len(requestedEvents)==2 and all(event['event']=='next' and event['status']==0 for event in requestedEvents)
    originalMarker=rows['external-explicit-selected-paused'].get('frameMarker')
    checks['requestedNextPictures']=originalMarker is not None and all(rows['requested-next-'+str(number)].get('frameMarker')==originalMarker+number for number in [1,2])"""
new="""    originalMarker=rows['external-explicit-selected-paused'].get('frameMarker')
    if args.extra_paused_mode=='steps':
        checks['requestedFrameFeedback']=len(requestedEvents)==2 and all(event['event']=='next' and event['status']==0 for event in requestedEvents)
        checks['requestedNextPictures']=originalMarker is not None and all(rows['steps-'+str(number)].get('frameMarker')==originalMarker+number for number in [1,2])
    else:
        checks['holdSendsNoExtraFrameFeedback']=not requestedEvents
        checks['holdConservesPicture']=originalMarker is not None and all(rows['hold-'+str(number)].get('frameMarker')==originalMarker for number in [1,2])
    result['extraPausedMode']=args.extra_paused_mode
    result['extraPausedSeconds']=rows[args.extra_paused_mode+'-2']['observedAt']-rows['external-explicit-selected-paused']['observedAt']"""
assert source.count(old)==1;source=source.replace(old,new)
probe=work/'probe-control-r1.py';assert not probe.exists();probe.write_text(source,encoding='utf-8',newline='\n')
old=read(prior/'inputs-candidate-r2.json')['files'];last=read(prior/'final-verification-r4.json')
assert {p for p,h in old.items() if digest(repo/p)!=h}==set(last['onlyPostFreezeChanges'])
assert all(digest(repo/p)==h for p,h in last['onlyPostFreezeChanges'].items())
files=set(old)
for root in [prior,work]:files.update(p.relative_to(repo).as_posix() for p in root.iterdir() if p.is_file() and p.suffix in ['.py','.sh','.mk'])
hashes={p:digest(repo/p) for p in sorted(files)};snapshot=work/'source-r1';assert not snapshot.exists();snapshot.mkdir()
for p,h in hashes.items():
    d=snapshot/p;d.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(repo/p,d);assert digest(d)==h
(work/'inputs-r1.json').write_text(json.dumps(dict(files=hashes,sourceCount=len(hashes),sourceSnapshot=str(snapshot),productionApplied=False,fullBetaAccepted=False),indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps(dict(sourceCount=len(hashes),productionApplied=False)))
