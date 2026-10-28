"""Freeze selected worker inputs and read-only toolchain identities; do not build."""
from pathlib import Path
import json,os,re,shutil,subprocess,sys
sys.dont_write_bytecode=True
from worker_support import digest,save,storage,PEAK_COMPONENTS

prep=Path(__file__).resolve().parent
repo=prep.parents[2]
work=prep.parent
assert not (prep/'producer-identities.json').exists(), 'One-use preparation already frozen'
sidecar=repo/'modules/gui/qt/tests/native_library/history-worker.mk'
text=sidecar.read_text()
source=re.search(r'HISTORY_WORKER_SOURCES\s*=\s*(.*?)(?=\nHISTORY_WORKER_HEADERS)',text,re.S)[1]
headers=re.search(r'HISTORY_WORKER_HEADERS\s*=\s*(.*?)(?=\nHISTORY_WORKER_OBJECTS)',text,re.S)[1]
names=[name for name in (source+' '+headers).replace(chr(92),' ').split() if name.endswith(('.cpp','.hpp'))]
selected=['modules/gui/qt/'+name for name in names]
selected+=['modules/gui/qt/tests/native_library/history-worker.mk','modules/gui/qt/tests/network_stream_probe.c',
    'modules/gui/qt/Makefile.am','modules/gui/qt/qt.hpp','modules/gui/qt/maininterface/compositor.hpp','include/vlc_player.h',
    'src/player/player.h','src/input/input_internal.h','lib/libvlc_internal.h','src/libvlc.h']
initial={name:digest(repo/name) for name in sorted(set(selected))}
original=json.loads((work/'history-actions-native-r2/verification.json').read_text())
review=json.loads((prep/'reviewed-source-change.json').read_text())
assert digest(prep/'reviewed-mode-fix.patch')==review['patchSha256']
assert original['status']=='terminal' and original['normalExit']
assert all(value for key,value in original.items() if key.endswith('Unchanged'))
for name,expected in initial.items():
    if name in original['sourceSha256']:
        if expected!=original['sourceSha256'][name]:
            assert name==review['relativePath'] and original['sourceSha256'][name]==review['beforeSha256'] and expected==review['afterSha256'], 'Unexpected f09 model/ABI input change: '+name
base=work/'history-actions-native-r2/runtime'
expected_core='f09c45fe4075a02836807f00cab03163cdba1ec79e37ba35a0328f7cb6d5462d'
expected_ml='e834a41a700bbc6ffeaa83bfde895f5b60f35c2e11f08cd6526d5122afe5af3f'
assert original['runtimeSha256']['libvlccore.dll']==expected_core
assert original['runtimeSha256']['plugins/misc/libmedialibrary_plugin.dll']==expected_ml
for name,expected in original['runtimeSha256'].items(): assert digest(base/name)==expected
for name in initial:
    destination=prep/'inputs'/name
    destination.parent.mkdir(parents=True,exist_ok=True)
    shutil.copyfile(repo/name,destination)
    assert digest(destination)==initial[name]
save(prep/'initial-inputs.json',initial)
save(prep/'baseline.json',dict(relativeRoot=base.relative_to(repo).as_posix(),
    runtimeSha256=original['runtimeSha256'],baselineVerificationSha256=digest(work/'history-actions-native-r2/verification.json'),
    scope='Immutable f09 Core/e834 media-library runtime inventory only; no claim that old GUI005 includes later source edits'))
helper=repo/'test/libvlc/native_http_runtime.py'
shutil.copyfile(helper,prep/'native_http_runtime.py')
linux_code=r'''
from pathlib import Path
from hashlib import sha256
import json,os,shutil
base=Path('/root/vlc-juxtopposed-build')
os.environ['PATH']=str(base/'extras/tools/build/bin')+':'+str(base/'contrib/bin')+':'+os.environ['PATH']
names=[base/'win64/modules/gui/qt/Makefile',base/'win64/config.h',base/'win64/libtool',
 base/'win64/src/libvlccore.la',base/'win64/src/.libs/libvlccore.dll.a',base/'win64/src/.libs/libvlccore.dll',
 base/'win64/lib/libvlc.la',base/'win64/lib/.libs/libvlc.dll.a',base/'contrib/libexec/moc']
for tool in ['make','x86_64-w64-mingw32-g++','x86_64-w64-mingw32-gcc','x86_64-w64-mingw32-objdump']:
 names.append(Path(shutil.which(tool)).resolve())
result={str(path):sha256(path.read_bytes()).hexdigest() for path in names}
assert result[str(base/'win64/src/.libs/libvlccore.dll')]=='f09c45fe4075a02836807f00cab03163cdba1ec79e37ba35a0328f7cb6d5462d'
print(json.dumps(result))
'''
probe=subprocess.run(['wsl','-d','VLC-Juxtopposed-Home-20261018','--','python3','-c',linux_code],
    stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=60)
assert probe.returncode==0,probe.stderr.decode(errors='replace')
linux=json.loads(probe.stdout.decode())
save(prep/'linux-inputs.json',linux)
save(prep/'preflight.json',dict(status='prepared-not-executed-awaiting-root-lane-and-storage',
    scope='Actual production QObject/QThread model lifetime only; no shipping source edits, GUI build or extra dependency',
    initialSelectedSources=len(initial),realCompilerGraph='Captured with exact -M flags before build; compared to actual -MD graph afterward',
    knownExistingCppObjectBytes=30717278,knownExistingMocObjectBytes=15614651,
    existingNativeExeBytes=477529227,existingNativeExeDebugSectionBytes=446515503,
    existingNativeExeNonDebugSectionBytes=16905752,
    expectedStrippedExe='Tens of MB; enforce <=80 MiB and absence of debug sections',
    peakBudget=dict(componentsBytes=PEAK_COMPONENTS,peakAdditionalBytes=sum(PEAK_COMPONENTS.values()),reserveFraction=.2),
    storageAtPreparation=storage(repo),deadlineLocal='2026-10-28T23:35:00+02:00',
    expectedFunctions=['initTestCase','busyStateAndCapturedMode','workerFailureAndRetry','deletionDrainsActiveWorker','cleanupTestCase'],
    expectedDataTags=['continue','watch-again'],expectedDataRows=6,expectedTotalIncidents=8,
    rootMustExplicitlyReleaseWriterLaneBeforeBuild=True,buildInvoked=False,runInvoked=False,
    runtimeStaging='Only frozen immutable inventory, hardlink-only with no EMLINK copy fallback; fresh profile; unlink before replacement'))
top_names=['worker_support.py','build-worker.py','run-worker.py','verify-compile-inputs.py',
    'prepare-worker.py','native_http_runtime.py','initial-inputs.json','linux-inputs.json','baseline.json','preflight.json','REVIEW.md','reviewed-source-change.json','reviewed-mode-fix.patch']
frozen={name:digest(prep/name) for name in top_names}
frozen.update({'inputs/'+name:digest(prep/'inputs'/name) for name in initial})
save(prep/'producer-identities.json',frozen)
print(json.dumps(dict(status='prepared-only',selectedSources=len(initial),toolchainInputs=len(linux),
    frozenInputs=len(frozen),peakBytes=sum(PEAK_COMPONENTS.values()),
    storageAtPreparation=storage(repo),noBuildOrRunInvoked=True),indent=2))
