from pathlib import Path
from hashlib import sha256
import gzip,json,shutil
repo=Path('C:/Modding/vlc-juxtopposed');work=Path(__file__).resolve().parent
public=repo/'doc/juxtopposed-references/native/home-timeline-thumbnails-20261021/memory-r89'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def digest(p):return sha256(p.read_bytes()).hexdigest()
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
assert not public.exists()
sources=read(public.parent/'resource-r67/checkpoint.json')['sourceBindings'];assert all(digest(repo/p)==h for p,h in sources.items())
folders={'objects-r88':'private-memory-objects-r88','allow-r89':'private-memory-uia-allow-r89','block-r89':'private-memory-uia-block-r89'}
public.mkdir();cases={}
for label,folder in folders.items():
 root=work/folder;r=read(root/'verification.json');assert r['passed'] and r['normalExit'] and r['exitCode']==0 and r['privateFixture'] and r['sourceInputsUnchanged'] and r['identitiesUnchanged'] and not r['diagnostics'] and not r['forcedCleanupUsed']
 rows=[json.loads(l) for l in (root/'objects.jsonl').read_text().splitlines()];bounded=[{k:v for k,v in row.items() if k!='classes'} for row in rows if row['label']!='about-to-quit']
 cases[label]=dict(verification='cases/'+label+'/verification.json',phases=r['phases'],boundedObjectSnapshots=bounded,identities=r['identities'])
 for p in root.iterdir():
  if not p.is_file() or p.suffix not in ['.json','.jsonl','.log','.txt']:continue
  target=public/'cases'/label/p.name;target.parent.mkdir(parents=True,exist_ok=True)
  if p.suffix in ['.log','.txt']:target.with_suffix(p.suffix+'.gz').write_bytes(gzip.compress(p.read_bytes(),mtime=0))
  else:shutil.copyfile(p,target)
assert cases['allow-r89']['identities']==cases['block-r89']['identities']
for label,active in [('allow-r89',True),('block-r89',False)]:assert all(row['accessibilityActive']==active for row in cases[label]['boundedObjectSnapshots'])
for name in ['object-observer-r88.cpp','object-observer-r88.mk','build-object-r88.sh','build-object-r88-retry.sh','object-observer-r89.cpp','uia-request-filter-r89.hpp','build-object-r89.sh','run-memory-objects-r88.py','run-memory-uia-r89.py','inputs-memory-r88.json','inputs-memory-uia-allow-r89.json','inputs-memory-uia-block-r89.json','resource_metrics_r45.py','record-memory-r89.py']:
 p=work/name;target=public/'probes'/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,target)
artifacts={p.relative_to(public).as_posix():digest(p) for p in sorted(public.rglob('*')) if p.is_file()}
write(public/'checkpoint.json',dict(date='2026-10-22',scope='Private bounded QObject snapshots and matching per-process accessibility-request factor controls. Not a production fix.',sourceBindings=sources,artifacts=artifacts,cases=cases,
 accessibilityActivityDependencyObserved=True,allocationCallerEstablished=False,shippingAccessibilityFixAccepted=False,memoryStabilityAccepted=False,resourceCostAccepted=False,fullBetaAccepted=False,
 notes=[
 'R88 reachable QObject count4867->4866->4865 at10/100/190 seconds, six frameSwapped events total, while private memory172.43->309.53MB in180 seconds. This excludes growth of the inspected QObject trees, not all unparented/native allocations.',
 'Matching R89 binary: forwarding WM_GETOBJECT grows173.12->238.62MB/90s; consuming those messages in the isolated test process stays162.79->160.69MB. No media, RC, GPU queries, debugger or explicit Computer Use until after measurement.',
 'R89 field uiaRootRequests actually counts every WM_GETOBJECT message, not just requests whose object identifier equals UiaRootObjectId. Allow10/100s counts1252/13072; block1971/22027. Ambient client identity and precise allocation caller are not established.',
 'Accessibility is active in the forwarding case and inactive in the blocking case. Blocking accessibility is a factor control only and must not ship as a memory fix. Production files and system accessibility settings are unchanged.',
 'Both matching cases and R88 retain actual empty Home rendering, quit normally through CtrlQ, have unchanged source/binary inputs and clean diagnostics. Private runners do not qualify ordinary resource cost or full accessibility behavior.',
 'Qt6.8.3 local source shows WM_GETOBJECT repeatedly activates platform accessibility, which repeatedly notifies activation observers. R90 tests that narrower factor while still forwarding all accessibility requests.',
 'No package/release changes. Full beta roadmap remains active; current ordinary memory stability and real backdrop blur remain open.'
 ]))
print('Saved private cases',len(cases),'sources',len(sources),'artifacts',len(artifacts))
