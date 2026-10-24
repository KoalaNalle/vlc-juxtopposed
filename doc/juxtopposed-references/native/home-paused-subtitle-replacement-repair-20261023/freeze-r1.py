from pathlib import Path
from hashlib import sha256
import argparse,json,shutil,subprocess
work=Path(__file__).resolve().parent;repo=work.parent.parent
parser=argparse.ArgumentParser();parser.add_argument('--phase',choices=['tests','ordinary'],required=True);args=parser.parse_args()
digest=lambda p:sha256(p.read_bytes()).hexdigest()
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
prior=read(repo/'build-windows/beta-subtitle-frame-conservation-20261023/inputs-r1.json')['files']
changed=[p for p,h in prior.items() if digest(repo/p)!=h]
expected=['PROJECT_HANDOFF.md','doc/juxtopposed-beta-roadmap.md','doc/juxtopposed-settings-consumer-audit.md','test/Makefile.am']
if args.phase=='ordinary':expected.append('src/player/input.c')
assert sorted(changed)==sorted(expected),(changed,expected)
files=set(prior)
files.update(p.relative_to(repo).as_posix() for p in work.iterdir() if p.suffix in ['.py','.sh','.mk'])
files.update('test/libvlc/'+p for p in ['native_paused_subtitle_tracks.py','generate_paused_subtitle_tracks.sh','native_paused_subtitle_tracks.md'])
hashes={p:digest(repo/p) for p in sorted(files)}
suffix='tests-r1' if args.phase=='tests' else 'ordinary-r2'
folder=work/('source-'+suffix);assert not folder.exists();folder.mkdir()
for p,h in hashes.items():
    target=folder/p;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(repo/p,target);assert digest(target)==h
record=dict(sourceCount=len(hashes),files=hashes,sourceSnapshot=str(folder),phase=args.phase,productionApplied=args.phase=='ordinary',fullBetaAccepted=False)
(work/('inputs-'+suffix+'.json')).write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8',newline='\n')
subprocess.run(['git','diff','--check'],cwd=repo,check=True)
print(json.dumps(dict(sourceCount=len(hashes),phase=args.phase,changed=changed)))
