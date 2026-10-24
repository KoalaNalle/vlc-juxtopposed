from pathlib import Path
from hashlib import sha256
import gzip,json,shutil
repo=Path('C:/Modding/vlc-juxtopposed');work=Path(__file__).resolve().parent
public=repo/'doc/juxtopposed-references/native/home-timeline-thumbnails-20261021/scrollbar-r93'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def digest(p):return sha256(p.read_bytes()).hexdigest()
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
assert not public.exists()
sources=read(public.parent/'resource-r67/checkpoint.json')['sourceBindings']
for name in ['contrib/src/qtdeclarative/rules.mak','contrib/src/qtdeclarative/0003-ScrollBar-keep-accessibility-action-connections-unique.patch','modules/gui/qt/tests/native_ui/scrollbar_accessibility_journey.cpp']:sources[name]=digest(repo/name)
assert all(digest(repo/p)==h for p,h in sources.items())
folders={'setup-failed-r91':'private-scrollbar-baseline-r91','baseline-r92':'private-scrollbar-baseline-r92','fixed-r93':'private-scrollbar-fixed-r93'}
public.mkdir();cases={}
for label,folder in folders.items():
 root=work/folder;r=read(root/'verification.json');t=read(root/'regression.json');assert r['normalExit'] and r['exitCode']==0 and not r['forcedCleanupUsed'] and not r['diagnostics'] and r['sourceInputsUnchanged'] and r['identitiesUnchanged']
 if label=='setup-failed-r91':assert not r['passed'] and t.get('setupError') and not t['checks']
 elif label=='baseline-r92':assert r['passed'] and not t['allPassed'] and [i for i,x in enumerate(t['checks']) if not x['passed']]==[2,3]
 else:assert r['passed'] and t['allPassed'] and len(t['checks'])==7 and all(x['passed'] for x in t['checks'])
 cases[label]=dict(verification='cases/'+label+'/verification.json',nativeRegression='cases/'+label+'/regression.json',nativeAllPassed=t['allPassed'],identities=r['identities'])
 for p in root.iterdir():
  if not p.is_file() or p.suffix not in ['.json','.txt','.log']:continue
  target=public/'cases'/label/p.name;target.parent.mkdir(parents=True,exist_ok=True)
  if p.suffix in ['.txt','.log']:target.with_suffix(p.suffix+'.gz').write_bytes(gzip.compress(p.read_bytes(),mtime=0))
  else:shutil.copyfile(p,target)
for name in ['scrollbar-regression-r91.cpp','scrollbar-regression-r92.cpp','object-observer-r88.mk','run-scrollbar-r91.py','run-scrollbar-r92.py','run-scrollbar-fixed-r93.py','build-scrollbar-r91.sh','build-scrollbar-r92.sh','build-scrollbar-fixed-r93.sh','build-qt-scrollbar-r93.sh','inputs-scrollbar-baseline-r91.json','inputs-scrollbar-baseline-r92.json','inputs-scrollbar-fixed-fixed-r93.json','qt-scrollbar-dependencies-r93.json','qt-scrollbar-patched-r93-sha256.txt','record-scrollbar-r93.py']:
 p=work/name;target=public/'probes'/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,target)
for name in ['contrib/src/qtdeclarative/0003-ScrollBar-keep-accessibility-action-connections-unique.patch','modules/gui/qt/tests/native_ui/scrollbar_accessibility_journey.cpp']:
 p=repo/name;target=public/'sources'/p.name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,target)
artifacts={p.relative_to(public).as_posix():digest(p) for p in sorted(public.rglob('*')) if p.is_file()}
write(public/'checkpoint.json',dict(date='2026-10-22',scope='Direct native Qt scrollbar regression before/after narrow unique action-connection patch; separate fixture setup failure retained.',sourceBindings=sources,artifacts=artifacts,cases=cases,
 scrollbarDuplicateActionRegressionVerified=True,privateSevenChecksAccepted=True,ordinaryIdleMemoryAccepted=False,previewResourceCostAccepted=False,fullAccessibilityAccepted=False,frameWaitWorkaroundPromoted=False,fullBetaAccepted=False,
 notes=[
 'Baseline native action tests: one activation steps position0.4->0.41 or0.39 as expected. After1000 repeated true notifications, one increase clamps0.75 and one decrease clamps0; both fail. Deactivation and reactivation/cycle checks pass.',
 'Patch adds Qt::UniqueConnection only to the two scrollbar member-action connects. Accessibility requests and repeated activation notifications remain enabled. All seven native action/lifecycle checks now pass; no UIA blocker or activation dedup wrapper is linked into this fixture.',
 'The project native regression creates one hidden generated QtQuick.Templates ScrollBar on the existing isolated engine; no user media or settings are touched. It tests real Qt action signals, not a mirrored connection-count model.',
 'R91 fixture originally imported the unavailable generic QtQuick.Controls static plugin, so setup failed before any checks. That raw failure is retained; R92 switches to the linked QtQuick.Templates module. The setup failure is not a functional before/after test.',
 'Runner passed for baseline means expected native failure plus normal lifecycle; nativeRegression.allPassed remains false in its raw output. All three processes quit normally through CtrlQ with clean diagnostics and unchanged inputs.',
 'Source bindings include contrib patch/rules and canonical test, plus the R67 production source cohort. Qt version stays6.8.3. Original Qt source/build/installed static archives were copied into ignored storage before patching; dependency manifest binds their original and patched hashes.',
 'This receipt accepts selected native actions only. Fresh ordinary Home memory, preview resources, broader screen-reader/DPI/design/platform/review/package work remain open.'
 ]))
print('Saved direct before/after regression',len(sources),'sources',len(artifacts),'artifacts')
