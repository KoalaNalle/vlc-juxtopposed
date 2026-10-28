"""Freeze the specifically reviewed local mux delta; no build/run operation."""
import ast
from datetime import datetime, timezone
from hashlib import file_digest
import json
from pathlib import Path
import shutil
import subprocess

root = Path(r'C:\Modding\vlc-juxtopposed')
prep = Path(__file__).resolve().parent
repair = prep.parent
r4 = root / 'build-windows/beta-fps-20261028/fifo-tail-probe-preparation-r4'
assert not (prep / 'preflight.json').exists()
load = lambda p: json.loads(p.read_text(encoding='utf-8-sig'))
def digest(p):
    with Path(p).open('rb') as f:
        return file_digest(f, 'sha256').hexdigest()
approved = load(repair / 'adapter-guards-r1/source-identities.json')
assert len(approved) == 4 and all(digest(root/n) == h for n,h in approved.items())
assert approved['modules/demux/avformat/mux.c'] == 'd06d150d3fdb92c80fce3afe47af6bf7933876d81900a2dac5d5becea7422f35'
original_ready = load(repair / 'source-identities.json')
assert all(digest(repair/'candidate'/n) == h for n,h in original_ready.items())
before = load(repair/'pre-edit-identities.json')
assert all(digest(repair/'original'/n) == h for n,h in before.items())
baseline = load(r4/'build-inputs.json')
assert len(baseline) == 535
inputs = dict(baseline, **approved)
assert len(inputs) == 537
for name, value in inputs.items():
    assert not any(token in name for token in ('native_soxr_reference', 'soxr_lifecycle_trace', 'soxr_native_reference', 'soxr_pcm_trace'))
    assert digest(root/name) == value, 'Unreviewed source drift: '+name
(prep/'build-inputs.json').write_text(json.dumps(inputs,indent=2)+'\n')
previous = load(r4/'preflight.json')
reviewed = prep/'reviewed-probe-preflight.json'
if not reviewed.exists():
    shutil.copyfile(r4/'preflight.json', reviewed)
assert digest(reviewed) == digest(r4/'preflight.json')
assert len(previous['dependencyInputs']) == 355
dependencies = dict(previous['dependencyInputs'])
wslroot = '/mnt/c/Modding/vlc-juxtopposed/'
assert wslroot+'modules/demux/avformat/mux.c' in dependencies
dependencies[wslroot+'modules/demux/avformat/mux.c'] = approved['modules/demux/avformat/mux.c']
dependencies[wslroot+'modules/demux/avformat/avformat_mux_queue.h'] = approved['modules/demux/avformat/avformat_mux_queue.h']
assert len(dependencies) == 356
(prep/'dependency-inputs.json').write_text(json.dumps(dependencies,indent=2)+'\n')
wsl_code = r'''
from pathlib import Path
from hashlib import file_digest
import json,subprocess
r=Path('/mnt/c/Modding/vlc-juxtopposed')
p=r/'build-windows/beta-fps-20261028/tail-drain-repair-r1/native-qualification-r1'
old=json.loads((p/'reviewed-probe-preflight.json').read_text())
deps=json.loads((p/'dependency-inputs.json').read_text())
def h(p):
    with Path(p).open('rb') as f:return file_digest(f,'sha256').hexdigest()
assert len(deps)==356
for n,v in deps.items():assert h(n)==v,'Compiled dependency drift: '+n
assert h(old['configuredMakefile']['path'])==old['configuredMakefile']['sha256']
artifacts=list(old['sharedBuildArtifacts'])
for i in artifacts:
    f=Path(i['path'])
    assert f.is_file() and not f.is_symlink() and f.stat().st_nlink==1
    assert h(f)==i['sha256'] and f.stat().st_size==i['bytes']
b=Path('/root/vlc-juxtopposed-build')
for rel in ['modules/demux/avformat/.deps/libavcodec_plugin_la-mux.Plo','modules/.libs/libavcodec_plugin.dll.def']:
    f=b/'win64'/rel
    assert f.is_file() and not f.is_symlink() and f.stat().st_nlink==1
    artifacts.append({'path':str(f),'sha256':h(f),'bytes':f.stat().st_size,'hardlinks':1})
untouched={str(b/'win64'/n):h(b/'win64'/n) for n in ['modules/Makefile','src/libvlccore.la','src/.libs/libvlccore.dll','src/.libs/libvlccore.dll.a','modules/gui/qt/.libs/libqt_plugin.dll']}
ssp=subprocess.check_output(['x86_64-w64-mingw32-gcc','-print-file-name=libssp.a'],text=True).strip()
assert Path(ssp).is_file()
print(json.dumps({'sharedBuildArtifacts':artifacts,'untouchedBuildInputs':untouched,'sspArchive':ssp,'sspArchiveSha256':h(ssp),'configuredMakefile':old['configuredMakefile']}))
'''
result = subprocess.run(['wsl.exe','-d','VLC-Juxtopposed-Home-20261018','--','/usr/bin/python3','-c',wsl_code],capture_output=True,timeout=45)
assert result.returncode == 0 and not result.stderr, result.stderr.decode(errors='replace')
config=json.loads(result.stdout)
assert len(config['sharedBuildArtifacts']) == 8
runtime='build-windows/beta-fps-20261028/forced_srt_matroska_clean_candidate/runtime'
core='f8fc980bbe03da57908f79045f266710dd27a16c6ee66b6c45d60cd1f9b93598'
assert digest(root/runtime/'libvlccore.dll') == core
old_plugin=next(i for i in config['sharedBuildArtifacts'] if i['path'].endswith('.dll'))
assert old_plugin['sha256'] == '545f715418c4d20659dce5d6c81ee33b292b21c87243fafa62036b9ffd739e1f'
mib=1024*1024
maximum=old_plugin['bytes']+16*mib
components=dict(originalArtifactBackups=sum(i['bytes'] for i in config['sharedBuildArtifacts']),
    candidateDllMaximum=maximum,linkOutputAndTemporaryAllowance=2*maximum,
    compilerTemporaryAllowance=8*mib,unitExecutableAllowance=8*mib,
    logsAndReceiptsAllowance=16*mib,unitHardlinkMetadataAllowance=2*mib,
    sourceSnapshotAllowance=2*mib,snapshotAndPagingAllowance=128*mib)
budget=dict(componentsBytes=components,peakAdditionalBytes=sum(components.values()),reserveFraction=.2,candidatePluginMaxBytes=maximum)
unit_components=dict(unitExecutableAllowance=8*mib,logsAndReceiptsAllowance=2*mib,unitHardlinkMetadataAllowance=2*mib)
unit_budget=dict(componentsBytes=unit_components,peakAdditionalBytes=sum(unit_components.values()),reserveFraction=.2)
space=shutil.disk_usage(root)
minimum=(space.total+4)//5
config.update(status='prepared-not-executed',sourceIdentities=approved,dependencyInputs=dependencies,
    preservedRuntime=runtime,testCoreSha256=core,storageBudget=budget,unitStorageBudget=unit_budget,
    stopByUnix=datetime(2026,10,28,21,35,tzinfo=timezone.utc).timestamp(),
    storageAtPreparation=dict(totalBytes=space.total,freeBytes=space.free,minimumFreeBytes=minimum,
        headroomBytes=space.free-minimum,peakAdditionalBytes=budget['peakAdditionalBytes'],
        fits=space.free-minimum>=budget['peakAdditionalBytes'],marginBytes=space.free-minimum-budget['peakAdditionalBytes']))
b='/root/vlc-juxtopposed-build'
config['unitCompileCommand']=['x86_64-w64-mingw32-gcc','-std=gnu11','-DHAVE_CONFIG_H=1',
    '-D__MSVCRT_VERSION__=0x700','-D_WIN32_WINNT=0x0601','-DWINVER=0x0601','-D__USE_MINGW_ANSI_STDIO=1',
    '-I'+b+'/win64','-I'+wslroot+'include','-I'+b+'/win64/include',
    '-I'+b+'/contrib/x86_64-w64-mingw32/include','-I'+wslroot+'compat/stdbit','-I'+wslroot+'compat/stdckdint',
    '-O1','-g','-mms-bitfields','-Wall','-Wextra','-Werror=implicit-function-declaration','-Werror=incompatible-pointer-types',
    '-fstack-protector-strong','-fdata-sections','-ffunction-sections','-static-libgcc',
    '-MD','-MF','{out}/compiled.d',wslroot+'test/modules/demux/avformat_mux_queue.c',
    b+'/win64/src/.libs/libvlccore.dll.a',config['sspArchive'],
    '-Wl,--gc-sections,--nxcompat,--dynamicbase,--high-entropy-va','-o','{out}/test_avformat_mux_queue.exe']
assert all(digest(root/n)==h for n,h in inputs.items())
for p in prep.glob('*.py'):ast.parse(p.read_text(encoding='utf-8-sig'),filename=str(p))
(prep/'preflight.json').write_text(json.dumps(config,indent=2)+'\n')
frozen={p.name:digest(p) for p in sorted(prep.iterdir()) if p.is_file() and p.name not in ('producer-identities.json','preparation.log')}
assert 'qualify-build.py' in frozen and 'run-unit.py' in frozen
(prep/'producer-identities.json').write_text(json.dumps(frozen,indent=2)+'\n')
print(json.dumps(dict(status=config['status'],sourceInputs=537,compiledDependencies=356,
    originalSharedArtifacts=8,frozenInputs=len(frozen),storage=config['storageAtPreparation'],
    noBuildOrTestExecuted=True),indent=2))
