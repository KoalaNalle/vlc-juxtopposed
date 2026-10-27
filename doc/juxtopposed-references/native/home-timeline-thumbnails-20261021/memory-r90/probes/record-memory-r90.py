from pathlib import Path
from hashlib import sha256
import gzip,json,shutil
repo=Path('C:/Modding/vlc-juxtopposed');work=Path(__file__).resolve().parent
public=repo/'doc/juxtopposed-references/native/home-timeline-thumbnails-20261021/memory-r90'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def digest(p):return sha256(p.read_bytes()).hexdigest()
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
assert not public.exists();sources=read(public.parent/'resource-r67/checkpoint.json')['sourceBindings'];assert all(digest(repo/p)==h for p,h in sources.items())
public.mkdir();cases={}
for case in ['forward','dedup']:
 root=work/f'private-memory-activation-{case}-r90';r=read(root/'verification.json');assert r['passed'] and r['normalExit'] and r['exitCode']==0 and r['privateFixture'] and r['sourceInputsUnchanged'] and r['identitiesUnchanged'] and not r['diagnostics'] and not r['forcedCleanupUsed']
 rows=[json.loads(l) for l in (root/'objects.jsonl').read_text().splitlines() if json.loads(l)['label'] in ['10s','100s']];assert len(rows)==2 and all(v['accessibilityActive'] for v in rows)
 assert all(v['uiaRequests']==v['activationCalls'] for v in rows)
 if case=='forward':assert all(v['activationDuplicatesSkipped']==0 and v['activationForwarded']==v['activationCalls'] for v in rows)
 else:assert all(v['activationForwarded']==1 and v['activationDuplicatesSkipped']==v['activationCalls']-1 for v in rows)
 cases[case]=dict(verification='cases/'+case+'/verification.json',identities=r['identities'],phases=r['phases'],snapshots=[{k:v for k,v in row.items() if k!='classes'} for row in rows])
 for p in root.iterdir():
  if not p.is_file() or p.suffix not in ['.json','.jsonl','.log','.txt']:continue
  target=public/'cases'/case/p.name;target.parent.mkdir(parents=True,exist_ok=True)
  if p.suffix in ['.log','.txt']:target.with_suffix(p.suffix+'.gz').write_bytes(gzip.compress(p.read_bytes(),mtime=0))
  else:shutil.copyfile(p,target)
assert cases['forward']['identities']==cases['dedup']['identities']
for name in ['object-observer-r90.cpp','object-observer-r90.mk','activation-diagnostic-r90.hpp','uia-request-filter-r90.hpp','build-object-r90.sh','run-memory-activation-r90.py','inputs-memory-activation-forward-r90.json','inputs-memory-activation-dedup-r90.json','resource_metrics_r45.py','record-memory-r90.py']:
 p=work/name;target=public/'probes'/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,target)
artifacts={p.relative_to(public).as_posix():digest(p) for p in sorted(public.rglob('*')) if p.is_file()}
write(public/'checkpoint.json',dict(date='2026-10-22',scope='Private matching activation-notification factor controls, all accessibility requests still forwarded.',sourceBindings=sources,artifacts=artifacts,cases=cases,
 repeatedActivationDependencyObserved=True,scrollbarConnectionCauseVerified=False,shippingAccessibilityFixAccepted=False,memoryStabilityAccepted=False,resourceCostAccepted=False,fullBetaAccepted=False,
 notes=[
 'Same private GUI DLL, no QtTest, no media/RC/GPU/debugger/input during90s intervals. Forwarding every platform activation grows173.23->242.80MB; skipping unchanged activation states stays166.33->164.02MB.',
 'Both cases forward every WM_GETOBJECT request and remain accessibilityActive=true. Dedup10/100s forwards1 activation and skips1355/12875 duplicate notifications, while receiving1356/12876 accessibility requests. Message identifiers are counted separately, including UIA root, MSAA client and other requests.',
 'This narrows measured growth to repeated activation notifications. It is not complete screen-reader acceptance or a production fix; the private link wrapper must not ship as this diagnostic.',
 'Local Qt6.8.3 scrollbar activation code connects increase/decrease handlers again each time it sees true. A direct native scrollbar action regression and a narrow unique-connection repair are next; this receipt does not claim them passed.',
 'Actual empty Home remains rendered and both processes quit normally with CtrlQ, clean logs and unchanged source/binaries. Full resource/blur/design/platform/review/package gates remain open.'
 ]))
print('Saved matching activation controls',len(sources),'sources',len(artifacts),'artifacts')
