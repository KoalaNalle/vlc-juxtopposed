from pathlib import Path
from hashlib import sha256
import argparse,json,shutil,subprocess
work=Path(__file__).resolve().parent;repo=work.parent.parent
parser=argparse.ArgumentParser();parser.add_argument('--phase',choices=['before','after'],required=True);args=parser.parse_args()
digest=lambda p:sha256(p.read_bytes()).hexdigest()
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
prior=read(repo/'build-windows/beta-caption-accessibility-20261023/inputs-ordinary-r3.json')['files']
changed=[p for p,h in prior.items() if digest(repo/p)!=h]
expected=['PROJECT_HANDOFF.md','doc/juxtopposed-beta-roadmap.md','doc/juxtopposed-settings-consumer-audit.md']
if args.phase=='after':expected.append('modules/gui/qt/player/qml/Player.qml')
assert sorted(changed)==sorted(expected),(changed,expected)
for p in ['test/libvlc/native_soxr_reference.py','test/modules/audio_filter/soxr_lifecycle_trace.c','test/modules/audio_filter/soxr_native_reference.c','test/modules/audio_filter/soxr_pcm_trace.c']:assert p not in prior
files=set(prior)
files.update(p.relative_to(repo).as_posix() for p in work.iterdir() if p.is_file() and p.suffix in ['.py','.mk','.sh','.md','.txt','.png'])
hashes={p:digest(repo/p) for p in sorted(files)}
root=work/('source-'+args.phase+'-r1');assert not root.exists();root.mkdir()
for p,h in hashes.items():
    target=root/p;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(repo/p,target);assert digest(target)==h
record=dict(sourceCount=len(hashes),files=hashes,sourceSnapshot=str(root),phase=args.phase,fullBetaAccepted=False,changedSincePriorFreeze=changed,baselineRuntime='build-windows/beta-caption-accessibility-20261023/ordinary-ui-r4/runtime')
(work/('inputs-'+args.phase+'-r1.json')).write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8',newline='\n')
subprocess.run(['git','diff','--check'],cwd=repo,check=True)
print(json.dumps(dict(phase=args.phase,sourceCount=len(hashes),changed=changed)))
