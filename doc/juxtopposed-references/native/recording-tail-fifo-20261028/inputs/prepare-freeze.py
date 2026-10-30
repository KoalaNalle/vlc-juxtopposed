"""Preparation only. Read exact reviewed inputs; freeze new ignored receipts."""
import ast
import json
from pathlib import Path
import subprocess
from probe_support import digest, manifest, storage_state

prep = Path(__file__).resolve().parent
repo = Path(r'C:\Modding\vlc-juxtopposed')
r3 = prep.with_name('fifo-tail-probe-preparation-r3')
assert not (prep / 'preflight.json').exists() and not (prep / 'producer-identities.json').exists()
assert not (prep / 'build').exists() and not (prep / 'run').exists()
load = lambda path: json.loads(path.read_text(encoding='utf-8-sig'))
original_r3 = load(prep / 'r3-input-identities.json')
assert {path.name: digest(path) for path in r3.iterdir() if path.is_file()} == original_r3
config = load(prep / 'reviewed-r3-preflight.json')
inputs = load(prep / 'build-inputs.json')
assert len(inputs) == 535 and len(config['dependencyInputs']) == 355
for name, value in inputs.items():
    assert not any(token in name for token in ('native_soxr_reference', 'soxr_lifecycle_trace', 'soxr_native_reference', 'soxr_pcm_trace'))
    assert digest(repo / name) == value, f'Review changed source guard; do not refresh: {name}'
source = repo / 'modules/demux/avformat/mux.c'
assert digest(source) == digest(prep / 'mux-original.c') == config['sourceOriginalSha256'] == 'd1de834e1108e03565b3a347830f30debf9948b64315355ba5ae57e0acef777a'
assert digest(prep / 'mux-instrumented.c') == config['sourceInstrumentedSha256']
helper = (prep / 'probe-helper.c.txt').read_text()
assert all(token not in helper for token in ('vlc_fifo_Dequeue', 'vlc_fifo_Queue', 'MuxBlock(', 'block_Release('))
check = subprocess.run(['git', 'apply', '--check', str(prep / 'instrumentation-only.patch')], cwd=repo, capture_output=True)
assert check.returncode == 0, check.stderr.decode(errors='replace')

# Validate all inherited compiled dependencies and artifacts; never replace a
# stale expected value with whatever happens to be in the shared tree today.
wsl_code = r'''
from pathlib import Path
from hashlib import file_digest
import json
p=Path('/mnt/c/Modding/vlc-juxtopposed/build-windows/beta-fps-20261028/fifo-tail-probe-preparation-r4/reviewed-r3-preflight.json')
c=json.loads(p.read_text(encoding='utf-8-sig'))
def h(p):
    with Path(p).open('rb') as f:return file_digest(f,'sha256').hexdigest()
assert len(c['dependencyInputs'])==355
for n,v in c['dependencyInputs'].items():assert h(n)==v, 'Compiled dependency drift: '+n
assert h(c['configuredMakefile']['path'])==c['configuredMakefile']['sha256']
for i in c['sharedBuildArtifacts']:
    f=Path(i['path'])
    assert not f.is_symlink() and f.is_file() and f.stat().st_nlink==i['hardlinks']==1
    assert f.stat().st_size==i['bytes'] and h(f)==i['sha256'], 'Shared artifact drift: '+str(f)
assert len(c['sharedBuildArtifacts'])==6
print(json.dumps({'compiledDependenciesChecked':355,'sharedArtifactsChecked':6,'configuredMakefileUnchanged':True}))
'''
check = subprocess.run(['wsl.exe', '-d', 'VLC-Juxtopposed-Home-20261018', '--', '/usr/bin/python3', '-c', wsl_code], capture_output=True, timeout=45)
assert check.returncode == 0 and not check.stderr, check.stderr.decode(errors='replace')
compiled_check = json.loads(check.stdout)
previous_run = repo / config['preservedRun']
previous_file = previous_run / 'verification.json'
assert digest(previous_file) == config['preservedVerificationSha256']
previous = load(previous_file)
assert previous['status'] == 'terminal' and previous['normalExit']
assert len(previous['runtimeSha256']) == 415
full_runtime = manifest(previous_run / 'runtime')
assert len(full_runtime) == 533, 'Runtime file count drift requires review'
assert not any(name.startswith('portable/') for name in full_runtime)
assert all(full_runtime[name]['sha256'] == value for name, value in previous['runtimeSha256'].items())
assert all(full_runtime[name]['sha256'] == value for name, value in config['preservedTuple'].items())
assert config['preservedTuple']['plugins/codec/libavcodec_plugin.dll'] == '545f715418c4d20659dce5d6c81ee33b292b21c87243fafa62036b9ffd739e1f'
assert digest(previous_run / 'output.mkv') == previous['outputSha256']
fixture = repo / config['fixture']
assert digest(fixture) == config['fixtureSha256']
assert digest(repo / 'test/libvlc/native_http_runtime.py') == config['runtimeHelperSha256']

mib = 1024 * 1024
old_plugin = full_runtime['plugins/codec/libavcodec_plugin.dll']['bytes']
candidate_max = old_plugin + 16 * mib
output_max = max(4 * fixture.stat().st_size, 16 * mib)
components = {
    'exactRecoveryCopies': sum(item['bytes'] for item in config['sharedBuildArtifacts']) + (prep / 'mux-original.c').stat().st_size,
    'savedCandidateDllMaximum': candidate_max,
    'stagedReplacementDllMaximum': candidate_max,
    'linkOutputAndTemporaryDllAllowance': 2 * candidate_max,
    'objectAndDependencyTemporaryAllowance': 8 * mib,
    'runOutputAllowance': output_max,
    'cacheRegenerationAllowance': 4 * mib,
    'logsAndReceiptsAllowance': 32 * mib,
    'hardlinkDirectoryMetadataAllowance': 16 * mib,
    'snapshotAndPagingAllowance': 128 * mib,
}
budget = dict(componentsBytes=components, peakAdditionalBytes=sum(components.values()), reserveFraction=.2,
    stagingPolicy='hardlink-only-fail-closed', candidatePluginMaxBytes=candidate_max,
    outputMaxBytes=output_max, runtimePluginRelative='plugins/codec/libavcodec_plugin.dll')
runtime_components = {name: value for name, value in components.items() if name in (
    'stagedReplacementDllMaximum', 'runOutputAllowance', 'cacheRegenerationAllowance',
    'logsAndReceiptsAllowance', 'hardlinkDirectoryMetadataAllowance', 'snapshotAndPagingAllowance')}
runtime_budget = dict(budget, componentsBytes=runtime_components, peakAdditionalBytes=sum(runtime_components.values()))
config.update(status='prepared-only-not-executed', revision='r4', inheritedStrictSourceInputs=535,
    preservedRuntimeInventory=full_runtime, preservedRuntimeFiles=len(full_runtime),
    preservedLegacyManifestedBinaries=415, addedSupportFiles=118,
    fullRuntimeBytes=sum(item['bytes'] for item in full_runtime.values()),
    storageBudget=budget, runtimeStorageBudget=runtime_budget, storageAtPreparation=storage_state(repo, budget),
    compiledGuardCheck=compiled_check,
    sourceGuardAfterPreparation=digest(source)==config['sourceOriginalSha256'],
    allCurrentBuildInputGuards=all(digest(repo/name)==value for name,value in inputs.items()),
    priorPreparationUnchanged={path.name:digest(path) for path in r3.iterdir() if path.is_file()}==original_r3)
assert config['sourceGuardAfterPreparation'] and config['allCurrentBuildInputGuards'] and config['priorPreparationUnchanged']
for path in prep.glob('*.py'):
    ast.parse(path.read_text(encoding='utf-8-sig'), filename=str(path))
(prep / 'preflight.json').write_text(json.dumps(config, indent=2) + '\n')
frozen = {path.name: digest(path) for path in sorted(prep.iterdir()) if path.is_file()
          and path.name not in ('producer-identities.json', 'preparation.log')}
(prep / 'producer-identities.json').write_text(json.dumps(frozen, indent=2) + '\n')
assert all(digest(prep/name)==value for name,value in frozen.items())
print(json.dumps(dict(status=config['status'], fullRuntimeFiles=533, strictSourceInputs=535,
    strictCompiledDependencies=355, frozenFiles=len(frozen), priorR3Unchanged=config['priorPreparationUnchanged'],
    storage=config['storageAtPreparation'], noBuildOrRuntimeExecuted=True), indent=2))
