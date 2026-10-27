from pathlib import Path
import json
w=Path(__file__).resolve().parent
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
for phase in ['before-r1','candidate-r1','candidate-r2','ordinary-r1']:
    root=w/('native-'+phase)
    for p in sorted(root.glob('*/process.json')):
        d=read(p);r=read(p.parent/'probe/result.json')
        print(json.dumps(dict(phase=phase,case=p.parent.name,normal=d.get('normalExit'),passed=r['passed'],checks=r.get('checks'),snapshots=len(r['observations']),keys=list(d.keys()),probeKeys=list(r.keys()) if p.parent.name=='frame-marker.mp4-wasapi' else [])))
print('Regression records',len(read(w/'regressions-ordinary-r3/processes.json')['records']))
