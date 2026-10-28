"""Reviewed execution only: temporarily observe one mux and restore shared inputs.

Run only in the established home WSL distro after root reviews this preparation.
Preparation validates syntax; it never imports or executes this producer.
"""
import argparse
import fcntl
import hashlib
import json
import os
from pathlib import Path
import shutil
import signal
import subprocess
import sys
import time

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--execute-reviewed-instrumentation', action='store_true', required=True)
parser.parse_args()
prep = Path(__file__).resolve().parent
repo = Path('/mnt/c/Modding/vlc-juxtopposed')
base = Path('/root/vlc-juxtopposed-build')
modules = base / 'win64/modules'
source = repo / 'modules/demux/avformat/mux.c'
out = prep / 'build'
assert not out.exists(), 'Use a newly reviewed preparation; never overwrite a build receipt'
out.mkdir()
record = dict(status='running', started=time.time(), target='libavcodec_plugin.la',
    scope='Temporary scalar metadata instrumentation only; no queue mutation or repair.',
    writerLock=str(base / '.vlc-build.lock'), writerLockAcquired=False,
    writerLockReleased=False, forcedBuildCleanupUsed=False, writerCleanupConfirmed=False,
    sourceRestored=False, sharedBuildArtifactsRestored=False,
    allProductionInputsRestored=False, dependencyInputsRestored=False,
    producerUnchanged=False, buildInputGuards=False, exitCode=None,
    verificationErrors=[], restoreErrors=[], cleanupErrors=[], receiptWriteErrors=[], events=[])
config, expected, frozen = {}, {}, {}
backups = []
process = lock_handle = failure = None
lock_held = source_written = False
old_handlers = {}


def digest(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def safe_digest(path, label):
    try:
        return digest(path)
    except BaseException as error:
        record['verificationErrors'].append(dict(path=str(path), check=label, error=repr(error)))
        return None


def matches_all(mapping, root=None, label='guard', replacement=None):
    # Evaluate every entry: a missing file must not hide later guard failures.
    matches = []
    for name, value in mapping.items():
        path = Path(name) if root is None else root / name
        wanted = replacement.get(name, value) if replacement else value
        actual = safe_digest(path, label)
        matches.append(actual == wanted)
        if actual is not None and actual != wanted:
            record['verificationErrors'].append(dict(path=str(path), check=label,
                error='SHA-256 mismatch', expected=wanted, actual=actual))
    return bool(mapping) and all(matches)


def save(strict=True):
    # Missing artifacts cannot prevent receipt generation. Receipt-disk failures
    # are collected too; the last fallback emits the terminal record to stderr.
    try:
        temporary = out / 'verification.next.json'
        temporary.write_text(json.dumps(record, indent=2) + '\n')
        os.replace(temporary, out / 'verification.json')
        return True
    except BaseException as error:
        record['receiptWriteErrors'].append(repr(error))
        if record['status'] == 'terminal':
            record['success'] = False
        try:
            (out / 'verification.json').write_text(json.dumps(record, indent=2) + '\n')
            return True
        except BaseException as fallback_error:
            record['receiptWriteErrors'].append(repr(fallback_error))
            if strict:
                raise
            print(json.dumps(record, indent=2), file=sys.stderr, flush=True)
            return False


def load_map(path):
    value = json.loads(path.read_text(encoding='utf-8-sig'))
    assert isinstance(value, dict), str(path) + ' must contain an object'
    return value


def group_members(pgid):
    active, zombies = [], []
    # A waited make can leave a compiler/linker in its session. Check the entire
    # owned process group. A defunct process cannot write shared artifacts.
    for entry in Path('/proc').iterdir():
        if not entry.name.isdigit():
            continue
        try:
            fields = (entry / 'stat').read_text().rsplit(') ', 1)[1].split()
            if int(fields[2]) == pgid:
                (zombies if fields[0] == 'Z' else active).append(int(entry.name))
        except FileNotFoundError:
            continue
    return active, zombies


def stop_owned_group():
    if process is None:
        record['writerCleanupConfirmed'] = True
        return True
    try:
        active, zombies = group_members(process.pid)
        if active or process.poll() is None or failure is not None:
            # Address the group on every failed/interrupted path, even if make
            # itself has exited. Only this producer's new session is signaled.
            record['forcedBuildCleanupUsed'] = bool(active or process.poll() is None)
            try:
                os.killpg(process.pid, signal.SIGTERM)
            except ProcessLookupError:
                pass
            deadline = time.monotonic() + 10
            while time.monotonic() < deadline:
                process.poll()
                active, zombies = group_members(process.pid)
                if not active and process.poll() is not None:
                    break
                time.sleep(.05)
            if active or process.poll() is None:
                try:
                    os.killpg(process.pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass
                try:
                    process.wait(timeout=10)
                except subprocess.TimeoutExpired as error:
                    record['cleanupErrors'].append(repr(error))
                deadline = time.monotonic() + 10
                while time.monotonic() < deadline:
                    active, zombies = group_members(process.pid)
                    if not active:
                        break
                    time.sleep(.05)
        if process.poll() is not None:
            process.wait(timeout=1)
        active, zombies = group_members(process.pid)
        record['remainingOwnedWriterPids'] = active
        record['defunctOwnedGroupPids'] = zombies
        record['writerCleanupConfirmed'] = not active and process.poll() is not None and not record['cleanupErrors']
    except BaseException as error:
        record['cleanupErrors'].append(repr(error))
        record['writerCleanupConfirmed'] = False
    record['events'].append(dict(event='writer-cleanup', time=time.time(),
        confirmed=record['writerCleanupConfirmed']))
    return record['writerCleanupConfirmed']


def interrupted(signum, _frame):
    raise InterruptedError('Build producer received signal ' + str(signum))


try:
    for signum in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP):
        old_handlers[signum] = signal.signal(signum, interrupted)
    save()
    loaded = load_map(prep / 'preflight.json')
    assert isinstance(loaded.get('dependencyInputs'), dict)
    assert len(loaded['dependencyInputs']) == 355, 'Keep all 355 reviewed compiled dependencies'
    assert isinstance(loaded.get('sharedBuildArtifacts'), list) and len(loaded['sharedBuildArtifacts']) == 6
    for item in loaded['sharedBuildArtifacts']:
        assert isinstance(item, dict) and isinstance(item.get('path'), str) and isinstance(item.get('sha256'), str)
    config = loaded
    frozen = load_map(prep / 'producer-identities.json')
    expected = load_map(prep / 'build-inputs.json')
    assert len(expected) == 535, 'Keep all 535 reviewed source inputs'
    assert matches_all(frozen, prep, 'frozen producer preflight')
    assert matches_all(expected, repo, 'source preflight'), 'Review a new preparation after input drift'
    assert digest(modules / 'Makefile') == config['configuredMakefile']['sha256']
    assert matches_all(config['dependencyInputs'], label='compiled dependency preflight')
    original = (prep / 'mux-original.c').read_bytes()
    instrumented = (prep / 'mux-instrumented.c').read_bytes()
    assert digest(source) == config['sourceOriginalSha256'] == hashlib.sha256(original).hexdigest()
    assert hashlib.sha256(instrumented).hexdigest() == config['sourceInstrumentedSha256']
    record.update(sourceOriginalSha256=config['sourceOriginalSha256'],
        sourceInstrumentedSha256=config['sourceInstrumentedSha256'],
        producerSha256=digest(__file__), sharedArtifactsBefore=config['sharedBuildArtifacts'])

    lock_handle = (base / '.vlc-build.lock').open('a')
    fcntl.flock(lock_handle, fcntl.LOCK_EX | fcntl.LOCK_NB)
    lock_held = True
    record['writerLockAcquired'] = True
    assert matches_all(expected, repo, 'source under writer lock')
    assert matches_all(config['dependencyInputs'], label='compiled dependency under writer lock')
    for item in config['sharedBuildArtifacts']:
        path = Path(item['path'])
        assert not path.is_symlink() and path.is_file()
        assert path.stat().st_nlink == 1 and digest(path) == item['sha256']
    budget = config['storageBudget']
    assert budget['reserveFraction'] == .2 and budget['stagingPolicy'] == 'hardlink-only-fail-closed'
    assert all(isinstance(value, int) and value >= 0 for value in budget['componentsBytes'].values())
    peak = sum(budget['componentsBytes'].values())
    assert peak == budget['peakAdditionalBytes'] and peak > 0
    disk = shutil.disk_usage(repo)
    floor = (disk.total + 4) // 5  # ceil(20 percent of the actual C: capacity)
    record['storageBudget'] = dict(budget, totalBytes=disk.total, freeBytes=disk.free,
        minimumFreeBytes=floor, availableHeadroomBytes=disk.free - floor,
        fitsBelow80Percent=disk.free - floor >= peak)
    save()
    assert record['storageBudget']['fitsBelow80Percent'], 'Worst-case peak exceeds C: headroom below 80 percent'

    recovery = out / 'recovery'
    recovery.mkdir()
    shutil.copyfile(source, recovery / 'mux-original.c')
    assert digest(recovery / 'mux-original.c') == config['sourceOriginalSha256']
    for index, item in enumerate(config['sharedBuildArtifacts']):
        path = Path(item['path'])
        backup = recovery / (str(index) + '-' + path.name)
        shutil.copyfile(path, backup)
        assert digest(backup) == item['sha256']
        backups.append((path, backup, item['sha256']))
    assert len(backups) == 6
    save()

    replacement = out / 'instrumented-source.next'
    replacement.write_bytes(instrumented)
    source_written = True  # before replace: interruption cannot miss a write
    os.replace(replacement, source)
    assert digest(source) == config['sourceInstrumentedSha256']
    env = dict(os.environ)
    env['PATH'] = str(base / 'extras/tools/build/bin') + ':' + str(base / 'contrib/bin') + ':' + env['PATH']
    env.pop('CC', None)
    env.pop('CXX', None)
    record['command'] = ['make', '-C', str(modules), '-j2', 'V=0', 'libavcodec_plugin.la']
    with (out / 'build.log').open('wb') as log:
        process = subprocess.Popen(record['command'], env=env, stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
        record['pid'] = process.pid
        save()
        try:
            record['exitCode'] = process.wait(timeout=600)
        except subprocess.TimeoutExpired:
            record['buildTimedOut'] = True
            raise
    assert record['exitCode'] == 0, 'Incremental plugin build failed'
    assert matches_all(expected, repo, 'instrumented source guard',
        {'modules/demux/avformat/mux.c': config['sourceInstrumentedSha256']})
    assert matches_all(config['dependencyInputs'], label='instrumented dependency guard',
        replacement={str(source): config['sourceInstrumentedSha256']})
    built = modules / '.libs/libavcodec_plugin.dll'
    assert built.stat().st_size <= budget['candidatePluginMaxBytes'], 'Candidate exceeds the frozen peak budget'
    shutil.copyfile(built, out / 'libavcodec_plugin.dll')
    assert (out / 'libavcodec_plugin.dll').stat().st_size <= budget['candidatePluginMaxBytes']
    record['instrumentedPluginSha256'] = digest(out / 'libavcodec_plugin.dll')
    assert record['instrumentedPluginSha256'] != config['preservedTuple']['plugins/codec/libavcodec_plugin.dll']
    record['buildInputGuards'] = True
except BaseException as error:
    failure = error
    record['failure'] = repr(error)
finally:
    # Further catchable interrupts cannot interrupt bounded recovery halfway.
    # SIGKILL or host power loss cannot be handled by a producer.
    for signum in old_handlers:
        signal.signal(signum, signal.SIG_IGN)
    writers_gone = stop_owned_group()
    if source_written and not writers_gone:
        record['restoreErrors'].append('Restoration skipped: owned writers are not confirmed stopped')
        record['manualRecoveryRequired'] = True
    elif source_written:
        for path, backup, value in backups:
            try:
                assert digest(backup) == value, 'Recovery copy drifted'
                if path.exists() or path.is_symlink():
                    assert not path.is_symlink() and path.is_file(), 'Unexpected replacement artifact type'
                    assert path.stat().st_nlink == 1, 'Do not overwrite a newly hardlinked shared artifact'
                else:
                    assert path.parent.is_dir(), 'Shared artifact parent disappeared'
                # A failed linker may delete its output; restore absent paths
                # from the exact preserved copy instead of failing at stat().
                shutil.copyfile(backup, path)
                assert digest(path) == value
                record['events'].append(dict(event='artifact-restored', path=str(path), time=time.time()))
            except BaseException as error:
                record['restoreErrors'].append(str(path) + ': ' + repr(error))
        try:
            assert digest(source) in (config['sourceInstrumentedSha256'], config['sourceOriginalSha256']), 'Concurrent source drift; recovery copy preserved'
            assert digest(recovery / 'mux-original.c') == config['sourceOriginalSha256']
            replacement = out / 'restored-source.next'
            shutil.copyfile(recovery / 'mux-original.c', replacement)
            os.replace(replacement, source)
            latest = time.time_ns()
            for path, _, _ in backups:
                try:
                    latest = max(latest, path.stat().st_mtime_ns + 1000000)
                except OSError as error:
                    record['restoreErrors'].append('mtime ' + str(path) + ': ' + repr(error))
            os.utime(source, ns=(source.stat().st_atime_ns, latest))
            record['events'].append(dict(event='source-restored-last', time=time.time()))
        except BaseException as error:
            record['restoreErrors'].append('source: ' + repr(error))

    # All final digests are safe even if a restore failed or an input disappeared.
    final_source_digest = safe_digest(source, 'final source')
    expected_source_digest = config.get('sourceOriginalSha256')
    record['sourceRestored'] = isinstance(expected_source_digest, str) and final_source_digest == expected_source_digest
    artifact_checks = []
    for item in config.get('sharedBuildArtifacts', []):
        artifact_checks.append(safe_digest(item['path'], 'final shared artifact') == item['sha256'])
    record['sharedBuildArtifactsRestored'] = len(artifact_checks) == 6 and all(artifact_checks)
    record['allProductionInputsRestored'] = matches_all(expected, repo, 'final production input') and len(expected) == 535
    dependencies = config.get('dependencyInputs', {})
    record['dependencyInputsRestored'] = matches_all(dependencies, label='final compiled dependency') and len(dependencies) == 355
    record['producerUnchanged'] = matches_all(frozen, prep, 'final producer')
    if lock_handle is not None:
        try:
            if lock_held:
                fcntl.flock(lock_handle, fcntl.LOCK_UN)
            lock_handle.close()
            record['writerLockReleased'] = True
        except BaseException as error:
            record['cleanupErrors'].append('lock: ' + repr(error))
    record.update(status='terminal', ended=time.time())
    record['success'] = (failure is None and record['exitCode'] == 0
        and record['writerCleanupConfirmed'] and not record['forcedBuildCleanupUsed']
        and not record['restoreErrors'] and not record['cleanupErrors']
        and not record['verificationErrors'] and not record['receiptWriteErrors']
        and all(record[name] for name in ('sourceRestored', 'sharedBuildArtifactsRestored',
            'allProductionInputsRestored', 'dependencyInputsRestored', 'producerUnchanged', 'buildInputGuards')))
    if not save(strict=False):
        record['success'] = False
    for signum, handler in old_handlers.items():
        signal.signal(signum, handler)

print(json.dumps(record, indent=2), flush=True)
if failure is not None:
    raise failure
assert record['success'], 'Build/recovery qualification failed; inspect the terminal receipt and exact recovery copies'
