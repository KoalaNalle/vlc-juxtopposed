from pathlib import Path
from hashlib import sha256
import json
work=Path(__file__).resolve().parent;repo=work.parent.parent
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
for phase in ['before','candidate']:
    data=read(work/('native-'+phase+'-r1/processes.json'));assert len(data['records'])==6 and data['allProcessesNormal']
    assert all(r['passed']==(phase=='candidate') and r['runtimeUnchanged'] and r['sourceInputsUnchanged'] and r['fixtureUnchanged'] for r in data['records'])
source=repo/'src/player/input.c';assert sha256(source.read_bytes()).hexdigest()=='070effdd2dffbadc64c28668f0e49af99541ea8b4596f1bced769092c851a129'
candidate=repo/'build-windows/beta-subtitle-frame-conservation-20261023/candidate-r1/input.c'
assert sha256(candidate.read_bytes()).hexdigest()=='2181c37920bc7ac6c043b179662282f9120d49de1c4093ff03246cb03e55d4e6'
text=candidate.read_text(encoding='utf-8')
log='''                        msg_Dbg(input->thread,
                                "candidate subtitle frame refresh clock=%"PRId64" target=%"PRId64,
                                clock_time, target);
'''
assert text.count(log)==1;text=text.replace(log,'')
needle='''                if (newly_selected && ev->fmt->i_cat == SPU_ES'''
assert text.count(needle)==1
text=text.replace(needle,'''                /* Refill a new subtitle decoder without advancing the frozen
                 * picture or resuming audio. Video points and demux dates have
                 * different origins; keep their conversion explicit. */
'''+needle)
source.write_text(text,encoding='utf-8',newline='\n')
(work/'application-r2.json').write_text(json.dumps(dict(productionApplied=True,playerInputSha256=sha256(source.read_bytes()).hexdigest(),candidateOnlyLogRemoved=True,ordinaryBuildAccepted=False,fullBetaAccepted=False),indent=2)+'\n',encoding='utf-8',newline='\n')
print('Applied tested subtitle reconstruction with explicit frame/date normalization')
