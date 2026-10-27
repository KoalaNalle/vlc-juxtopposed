from pathlib import Path
from hashlib import sha256
import gzip,json,shutil
repo=Path('C:/Modding/vlc-juxtopposed');work=Path(__file__).resolve().parent
public=repo/'doc/juxtopposed-references/native/home-timeline-thumbnails-20261021/memory-r87'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def digest(p):return sha256(p.read_bytes()).hexdigest()
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
assert not public.exists()
sources=read(public.parent/'resource-r67/checkpoint.json')['sourceBindings'];assert all(digest(repo/p)==h for p,h in sources.items())
folders={'dummy-r85':'ordinary-memory-dummy-r85','opengl-r86':'ordinary-memory-opengl-r86','prior-gui-r87':'ordinary-memory-prior-gui-r87'}
cases={}
public.mkdir()
for label,folder in folders.items():
 root=work/folder;r=read(root/'verification.json');assert r['status']=='terminal' and r['passed'] and r['normalExit'] and r['exitCode']==0 and not r['forcedCleanupUsed'] and not r['diagnostics']
 assert r['sourceInputsUnchanged'] and r['identitiesUnchanged']
 phases={}
 for phase in r['phases']:
  d=read(root/(phase+'.json'));v=d['samples'];assert d['status']=='terminal' and len(v)==91
  phases[phase]=dict(samples=len(v),seconds=v[-1]['elapsedSeconds'],firstPrivateMB=v[0]['privateBytes']/1e6,lastPrivateMB=v[-1]['privateBytes']/1e6)
 cases[label]=dict(verification='cases/'+label+'/verification.json',phases=phases,identities=r['identities'])
 for p in root.iterdir():
  if not p.is_file() or p.suffix not in ['.json','.log','.txt']:continue
  target=public/'cases'/label/p.name;target.parent.mkdir(parents=True,exist_ok=True)
  if p.suffix in ['.log','.txt']:target.with_suffix(target.suffix+'.gz').write_bytes(gzip.compress(p.read_bytes(),mtime=0))
  else:shutil.copyfile(p,target)
for name in ['run-memory-dummy-r85.py','run-memory-opengl-r86.py','run-memory-prior-gui-r87.py','inputs-memory-r85.json','inputs-memory-r86.json','inputs-memory-r87.json','resource_metrics_r45.py','record-memory-r87.py']:
 p=work/name;target=public/'probes'/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,target)
base=work.parent/'beta-precise-seek-20261021/ordinary-r8/runtime/plugins/gui/libqt_plugin.dll';b=base.read_bytes();assert digest(base)==cases['prior-gui-r87']['identities']['plugins/gui/libqt_plugin.dll']
assert all(s not in b for s in [b'TimelineThumbnails',b'qt-timeline-thumbnails',b'Qt 6.8.3 DirectComposition/D3D11: disabled optional frame-latency wait'])
artifacts={p.relative_to(public).as_posix():digest(p) for p in sorted(public.rglob('*')) if p.is_file()}
write(public/'checkpoint.json',dict(date='2026-10-22',sourceBindings=sources,artifacts=artifacts,cases=cases,scope='Ordinary binary-scoped headless, alternate graphics and historical-GUI memory controls.',
 memoryStabilityAccepted=False,allocationCallerEstablished=False,resourceCostAccepted=False,frameWaitWorkaroundPromoted=False,fullBetaAccepted=False,
 notes=[
 'Dummy main interface plus owned loopback RC and no media stays3.94->3.82MB over90s and exits normally through RC. Other controls show the GUI with no media or RC, followed by real Home inspection and CtrlQ.',
 'Cached OpenGL3 GUI still grows243.30->306.48MB over90s; DComp/D3D11 is not a necessary condition. It changes compositor/backend paths together, not one internal renderer function.',
 'Protected historical GUI DLL c382ebb2 grows175.08->238.80MB with the same current Core/MP4. It lacks thumbnail provider/configuration and the new latency-default log strings. The growth predates those GUI additions.',
 'Historical GUI comparison is explicitly binary-scoped: sourceBindings describe the current checkout inputs, not the older binary source. Its original runtime remains unchanged.',
 'No decoder, GPU-monitoring queries, debugger or forced cleanup. These short controls narrow necessary conditions; they do not identify the allocation caller, eventual plateau or prove an unbounded leak.',
 'No release/package changes; memory stability, backdrop blur and full beta roadmap remain open.'
 ]))
print('Saved3 controls',len(sources),'source hashes',len(artifacts),'artifacts')
