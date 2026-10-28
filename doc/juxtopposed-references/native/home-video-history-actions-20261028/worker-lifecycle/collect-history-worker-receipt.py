"""Freeze completed history-model qualification only; never build or launch."""
from pathlib import Path
from hashlib import sha256
import json
import shutil

work = Path(__file__).resolve().parent
repo = work.parents[1]
target = repo / 'doc/juxtopposed-references/native/home-video-history-actions-20261028/worker-lifecycle'
assert not target.exists(), 'A completed receipt must never be overwritten'

def digest(path):
    value = sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            value.update(block)
    return value.hexdigest()

def load(path):
    return json.loads(Path(path).read_text(encoding='utf-8'))

def save(path, value):
    Path(path).write_text(json.dumps(value, indent=2) + '\n', encoding='utf-8', newline='\n')

runs = [
    ('r1-original', 'history-worker-preparation-r1', 8, 6, 2),
    ('r2-strengthened-preimage', 'history-worker-preimage-r2', 4, 2, 2),
    ('r3-candidate', 'history-worker-candidate-r3', 8, 8, 0),
]
prepared = []
for name, directory, incident_count, passes, failures in runs:
    source = work / directory
    build = load(source / 'build/verification.json')
    run = load(source / 'run/verification.json')
    assert build['status'] == 'terminal' and build['success'] and build['exitCode'] == 0
    assert build['writerLockAcquired'] and build['writerLockReleased']
    assert not build['forcedBuildCleanupUsed'] and not build['remainingOwnedWriterPids']
    assert not build['errors'] and build['sourceUnchanged'] and build['toolchainUnchanged']
    assert build['producersUnchanged'] and build['compilerInputsUnchanged']
    assert build['actualDependenciesGuarded'] and build['strippedDebugVerified']
    assert run['status'] == 'terminal' and not run['forcedCleanupUsed']
    assert run['normalExit'] == (failures == 0)
    assert run['moduleCounts'] == ['603'] and not run['diagnostics']
    assert run['rawXmlRowsMatch'] and len(run['incidents']) == incident_count
    assert sum(row['type'] == 'pass' for row in run['incidents']) == passes
    assert sum(row['type'] == 'fail' for row in run['incidents']) == failures
    assert run['exitCode'] == (2 if failures else 0)
    assert run['success'] == (failures == 0)
    assert run['errors'] == (['AssertionError()'] if failures else [])
    guards = ['compilerInputsBefore', 'compilerInputsAfter', 'runtimeUnchanged',
              'baselineUnchanged', 'testUnchanged', 'buildVerificationUnchanged', 'producersUnchanged']
    assert all(run[key] for key in guards)
    assert run['testSha256'] == build['testSha256']
    assert run['buildVerificationSha256'] == digest(source / 'build/verification.json')
    producers = load(source / 'producer-identities.json')
    for relative, expected in producers.items():
        assert digest(source / relative) == expected, relative
    selected = load(source / 'initial-inputs.json')
    assert len(selected) == 44
    assert all(digest(source / 'inputs' / path) == expected for path, expected in selected.items())
    # This harness uses generated external IDs, not the owner's settings/media.
    assert not any('human' in key.lower() or 'profile' in key.lower() for key in run)
    prepared.append((name, source, build, run, producers, selected, guards))

target.mkdir(parents=True)
copies = []

def copy(source, relative):
    output = target / relative
    output.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(source, output)
    expected = digest(source)
    assert digest(output) == expected
    copies.append(dict(source=source.relative_to(repo).as_posix(), destination=relative,
                       bytes=output.stat().st_size, sha256=expected, byteIdentical=True))

summaries = []
root_files = ['worker_support.py', 'build-worker.py', 'run-worker.py', 'verify-compile-inputs.py',
              'prepare-worker.py', 'native_http_runtime.py', 'initial-inputs.json', 'linux-inputs.json',
              'baseline.json', 'preflight.json', 'producer-identities.json', 'REVIEW.md']
build_files = ['verification.json', 'compiler-inputs.json', 'actual-compiler-dependencies.json', 'exe-sections.txt']
run_files = ['verification.json', 'result.txt', 'result.xml']
for name, source, build, run, producers, selected, guards in prepared:
    for relative in root_files:
        copy(source / relative, f'{name}/{relative}')
    for relative in ['reviewed-mode-fix.patch', 'reviewed-source-change.json']:
        if (source / relative).is_file():
            copy(source / relative, f'{name}/{relative}')
    for relative in build_files:
        copy(source / 'build' / relative, f'{name}/build/{relative}')
    for relative in run_files:
        copy(source / 'run' / relative, f'{name}/run/{relative}')
    for relative in selected:
        copy(source / 'inputs' / relative, f'{name}/inputs/{relative}')
    summaries.append(dict(receipt=name, status=run['status'], success=run['success'],
        exitCode=run['exitCode'], normalExit=run['normalExit'],
        forcedCleanupUsed=run['forcedCleanupUsed'], incidents=run['incidents'],
        producerErrors=run['errors'],
        rawXmlRowsMatch=run['rawXmlRowsMatch'], moduleCounts=run['moduleCounts'],
        diagnostics=run['diagnostics'], guards={key: run[key] for key in guards},
        testSha256=run['testSha256'], exeBytes=build['exeBytes'],
        productionModelSha256=selected['modules/gui/qt/medialibrary/mlrecentvideomodel.cpp'],
        testSourceSha256=selected['modules/gui/qt/tests/test_history_worker.cpp'],
        compilerDependencyFiles=build['compilerDependencyFiles'],
        compilerInputCount=build['compilerInputCount'],
        compilerInputsSha256=build['compilerInputsSha256'],
        sourceSnapshotCount=len(selected), frozenProducerIdentityCount=len(producers),
        writerLockReleased=build['writerLockReleased'], remainingOwnedWriterPids=build['remainingOwnedWriterPids']))

fix = work / 'history-mode-cache-fix-20261028'
for relative in ['mlrecentvideomodel.before.cpp', 'mlrecentvideomodel.after.cpp', 'mode-cache.patch', 'identities.json']:
    copy(fix / relative, f'mode-cache-fix/{relative}')
copy(Path(__file__), Path(__file__).name)
(target / '.gitattributes').write_text('* -text\n', encoding='utf-8', newline='\n')
save(target / 'summary.json', dict(
    scope='Actual production MLRecentVideoModel QObject/QThread lifetime; no ordinary desktop claim',
    runs=summaries,
    candidateSixDataRowsPass=True,
    oldSourceFailuresRetained=True,
    cacheFixturePreconditionsRetained=True,
    exactModeCacheFix=load(fix / 'identities.json'),
    generatedExternalVideoIds=4,
    callbackGateTimeoutMs=10000,
    independentDestructorGateReleaseDelayMs=150,
    workerFailureScope='Real missing-ID lookup; not a disk write or partial-removal I/O fault',
    rawLogsMediaBinariesSettingsCopied=False,
    currentProductionFilesRehashedByCollector=False,
))
save(target / 'copy-verification.json', dict(byteIdenticalCopies=copies,
    omissions=['Raw VLC/device/human logs', 'Runtime executables/DLLs/objects',
               'Isolated profile/database contents', 'Compiler build.log; frozen graphs and terminal receipts retained'],
    source='Previously frozen inputs and completed records only; no new runtime test'))
print(json.dumps(dict(destination=str(target), originalCopiesVerified=len(copies),
    originalBytes=sum(row['bytes'] for row in copies), runs=[row['receipt'] for row in summaries])))
