"""Execute only a reviewed unit compile or narrowly scoped merged-plugin build.

No production source is installed, restored, touched or overwritten by this file.
Preparation parses the producer; it does not import or execute it.
"""
import argparse
import fcntl
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import signal
import subprocess
import sys
import time

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--execute-reviewed-qualification', action='store_true', required=True)
parser.add_argument('--phase', choices=('unit', 'plugin'), required=True)
phase = parser.parse_args().phase
prep = Path(__file__).resolve().parent
repo = Path('/mnt/c/Modding/vlc-juxtopposed')
base = Path('/root/vlc-juxtopposed-build')
modules = base / 'win64/modules'
out = prep / ('unit-build' if phase == 'unit' else 'build')
assert not out.exists(), 'Fresh reviewed output required; never overwrite a terminal receipt'
out.mkdir()
record = dict(status='running', phase=phase, started=time.time(), exitCode=None,
    scope='Reviewed native queue qualification; production repair source is never mutated.',
    writerLock=str(base / '.vlc-build.lock'), writerLockAcquired=False,
    writerLockReleased=False, forcedBuildCleanupUsed=False, writerCleanupConfirmed=False,
    sourceUnchanged=False, producerUnchanged=False, dependencyInputGuards=False,
    untouchedBuildBinariesUnchanged=False, buildInputGuards=False,
    originalBackupsUnchanged=False, verificationErrors=[], restoreErrors=[],
    cleanupErrors=[], receiptWriteErrors=[], events=[])
config, expected, frozen = {}, {}, {}
backups = []
process = lock_handle = failure = None
lock_held = shared_changed = False
old_handlers = {}
unit_file = None

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


def deadline(timeout):
    assert time.time() + timeout + 60 < config['stopByUnix'], 'Insufficient time for bounded job, recovery and checkpoint'


def guard_artifacts(label):
    checks = []
    for item in config.get('sharedBuildArtifacts', []):
        actual = safe_digest(item['path'], label)
        checks.append(actual == item['sha256'])
    return len(checks) == 8 and all(checks)


def unit_qualified():
    unit = load_map(unit_file)
    assert unit['status'] == 'terminal' and unit['success'] is True
    assert unit['normalExit'] is True and unit['exitCode'] == 0
    assert unit['cases'] == 41 and unit['failures'] == 0 and unit['checks'] > 0
    assert not unit['forcedCleanupUsed'] and not unit['inspectionErrors']
    guards = ('allSourceInputsUnchanged', 'allProducersUnchanged', 'buildVerificationUnchanged',
        'unitBuildExeUnchanged', 'runtimeExeUnchanged', 'preservedCoreUnchanged',
        'runtimeCoreUnchanged', 'runtimeFileSetUnchanged')
    assert all(unit[name] is True for name in guards)
    assert digest(prep / 'unit-build/verification.json') == unit['buildVerificationSha256']
    assert digest(prep / 'unit-build/test_avformat_mux_queue.exe') == unit['unitExeSha256']
    assert digest(prep / 'unit/runtime/test_avformat_mux_queue.exe') == unit['unitExeSha256']
    assert digest(prep / 'unit/runtime/libvlccore.dll') == unit['testCoreSha256'] == config['testCoreSha256']
    return unit


try:
    for signum in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP):
        old_handlers[signum] = signal.signal(signum, interrupted)
    save()
    config = load_map(prep / 'preflight.json')
    expected = load_map(prep / 'build-inputs.json')
    frozen = load_map(prep / 'producer-identities.json')
    assert len(expected) == 537 and len(config['dependencyInputs']) == 356
    assert len(config['sharedBuildArtifacts']) == 8
    excluded = ('test/libvlc/native_soxr_reference.py',
        'test/modules/audio_filter/soxr_lifecycle_trace.c',
        'test/modules/audio_filter/soxr_native_reference.c',
        'test/modules/audio_filter/soxr_pcm_trace.c')
    assert not any(name.replace('\\', '/') in excluded for name in expected)
    assert matches_all(frozen, prep, 'producer preflight')
    assert matches_all(expected, repo, 'source preflight'), 'Input drift requires a fresh reviewed preparation'
    assert matches_all(config['dependencyInputs'], label='compiled dependency preflight')
    assert matches_all(config['untouchedBuildInputs'], label='untouched build preflight')
    assert digest(config['sspArchive']) == config['sspArchiveSha256']
    timeout = 120 if phase == 'unit' else 600
    deadline(timeout)
    if phase == 'plugin':
        unit_file = prep / 'unit/verification.json'
        unit_qualified()
        record['unitVerificationSha256'] = digest(unit_file)
    lock_handle = (base / '.vlc-build.lock').open('a')
    fcntl.flock(lock_handle, fcntl.LOCK_EX | fcntl.LOCK_NB)
    lock_held = True
    record['writerLockAcquired'] = True
    assert matches_all(expected, repo, 'source under writer lock')
    assert matches_all(config['dependencyInputs'], label='compiled dependencies under writer lock')
    assert matches_all(config['untouchedBuildInputs'], label='untouched binaries under writer lock')
    for item in config['sharedBuildArtifacts']:
        path = Path(item['path'])
        assert path.is_absolute() and path.is_relative_to(modules)
        assert not path.is_symlink() and path.is_file() and path.stat().st_nlink == 1
        assert path.stat().st_size == item['bytes'] and digest(path) == item['sha256']
    budget = config['storageBudget']
    assert budget['reserveFraction'] == .2
    assert all(isinstance(value, int) and value >= 0 for value in budget['componentsBytes'].values())
    peak = sum(budget['componentsBytes'].values())
    assert peak == budget['peakAdditionalBytes'] and peak > 0
    disk = shutil.disk_usage(repo)
    reserve = (disk.total + 4) // 5
    record['storageBudget'] = dict(budget, totalBytes=disk.total, freeBytes=disk.free,
        minimumFreeBytes=reserve, availableHeadroomBytes=disk.free - reserve,
        fitsBelow80Percent=disk.free - reserve >= peak)
    save()
    assert record['storageBudget']['fitsBelow80Percent'], 'Full reviewed peak exceeds C: headroom below80percent'
    env = dict(os.environ)
    env['PATH'] = str(base / 'extras/tools/build/bin') + ':' + str(base / 'contrib/bin') + ':' + env['PATH']
    env.pop('CC', None)
    env.pop('CXX', None)
    if phase == 'unit':
        command = [argument.replace('{out}', str(out)) for argument in config['unitCompileCommand']]
        assert Path(command[0]).name == 'x86_64-w64-mingw32-gcc'
        assert '-DHAVE_CONFIG_H=1' in command and '-o' in command
        assert command[command.index('-o') + 1] == str(out / 'test_avformat_mux_queue.exe')
    else:
        original = out / 'original'
        original.mkdir()
        for index, item in enumerate(config['sharedBuildArtifacts']):
            path = Path(item['path'])
            metadata = path.stat()
            backup = original / (str(index) + '-' + path.name)
            shutil.copy2(path, backup)
            assert digest(backup) == item['sha256']
            backups.append((path, backup, item['sha256'], metadata.st_atime_ns, metadata.st_mtime_ns))
        assert len(backups) == 8
        record['originalSharedBuildArtifacts'] = config['sharedBuildArtifacts']
        save()
        marker = modules / 'demux/avformat/libavcodec_plugin_la-mux.lo'
        assert any(path == marker for path, *_ in backups)
        deadline(timeout)
        shared_changed = True
        marker.unlink()  # Only this exact backed-up target marker forces compile.
        command = ['make', '-C', str(modules), '-o', 'Makefile', '-j2', 'V=1', 'libavcodec_plugin.la']
    deadline(timeout)
    record['command'] = command
    with (out / 'build.log').open('wb') as log:
        process = subprocess.Popen(command, cwd=repo, env=env, stdout=log,
            stderr=subprocess.STDOUT, start_new_session=True)
        record['pid'] = process.pid
        save()
        try:
            record['exitCode'] = process.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            record['buildTimedOut'] = True
            raise
    assert record['exitCode'] == 0, 'Owned qualification build failed'
    assert matches_all(expected, repo, 'post-build source')
    assert matches_all(config['dependencyInputs'], label='post-build compiled dependency')
    assert matches_all(config['untouchedBuildInputs'], label='post-build untouched binaries')
    assert digest(config['sspArchive']) == config['sspArchiveSha256']
    if phase == 'unit':
        exe = out / 'test_avformat_mux_queue.exe'
        assert 0 < exe.stat().st_size <= 8 * 1024 * 1024
        record['exeSha256'] = digest(exe)
        assert guard_artifacts('post-unit shared artifact')
        record['sharedBuildArtifactsUnchanged'] = True
    else:
        log = (out / 'build.log').read_text(errors='replace')
        record['actualCompileObserved'] = bool(re.search(r'(?m)^.*x86_64-w64-mingw32-gcc.*\s-c\s.*avformat/mux\.c', log))
        record['actualLinkObserved'] = bool(re.search(r'(?m)^libtool: link:.*-shared.*-o\s+\.libs/libavcodec_plugin\.dll', log))
        assert record['actualCompileObserved'] and record['actualLinkObserved'], 'Actual mux compile and merged-plugin link must be logged'
        built = modules / '.libs/libavcodec_plugin.dll'
        assert built.stat().st_size <= budget['candidatePluginMaxBytes']
        shutil.copyfile(built, out / 'libavcodec_plugin.dll')
        record['pluginSha256'] = digest(out / 'libavcodec_plugin.dll')
        assert record['pluginSha256'] == digest(built)
        assert record['pluginSha256'] != '545f715418c4d20659dce5d6c81ee33b292b21c87243fafa62036b9ffd739e1f'
        assert unit_qualified()
        assert digest(unit_file) == record['unitVerificationSha256']
    assert matches_all(frozen, prep, 'post-build producer')
    record['buildInputGuards'] = True
except BaseException as error:
    failure = error
    record['failure'] = repr(error)
finally:
    for signum in old_handlers:
        signal.signal(signum, signal.SIG_IGN)
    writers_gone = stop_owned_group()
    # Include final inspection failures and unexpected lingering compiler/linker
    # cleanup in the rollback decision, not only exceptions from make itself.
    candidate_qualified = failure is None and record['exitCode'] == 0 and record['buildInputGuards']
    if phase == 'plugin' and shared_changed:
        before_restore = [
            matches_all(expected, repo, 'candidate source'),
            matches_all(frozen, prep, 'candidate producer'),
            matches_all(config.get('dependencyInputs', {}), label='candidate compiled dependency'),
            matches_all(config.get('untouchedBuildInputs', {}), label='candidate untouched binary'),
            len(backups) == 8 and all([
                safe_digest(backup, 'candidate original backup') == value
                for _, backup, value, _, _ in backups]),
        ]
        plugin_hash = record.get('pluginSha256')
        before_restore.append(isinstance(plugin_hash, str) and
            safe_digest(out / 'libavcodec_plugin.dll', 'candidate copied plugin') == plugin_hash and
            safe_digest(modules / '.libs/libavcodec_plugin.dll', 'candidate shared plugin') == plugin_hash)
        before_restore.append(unit_file is not None and isinstance(record.get('unitVerificationSha256'), str) and
            safe_digest(unit_file, 'candidate unit receipt') == record['unitVerificationSha256'])
        candidate_qualified = candidate_qualified and all(before_restore)
    candidate_qualified = (candidate_qualified and writers_gone and not record['forcedBuildCleanupUsed']
        and not record['verificationErrors'] and not record['cleanupErrors'] and not record['receiptWriteErrors'])
    record['candidateQualifiedBeforeRestore'] = bool(candidate_qualified)
    # Preserve the repair source even on failure. Only known shared build
    # artifacts are restored, after the entire owned writer group is stopped.
    if phase == 'plugin' and shared_changed and not candidate_qualified:
        if not writers_gone:
            record['restoreErrors'].append('Restoration skipped: owned writers are not confirmed stopped')
            record['manualRecoveryRequired'] = True
        else:
            for path, backup, value, atime_ns, mtime_ns in backups:
                try:
                    assert digest(backup) == value, 'Original backup drifted'
                    if path.exists() or path.is_symlink():
                        assert not path.is_symlink() and path.is_file() and path.stat().st_nlink == 1
                    else:
                        assert path.parent.is_dir()
                    shutil.copyfile(backup, path)  # Missing failed-link outputs are recreated.
                    os.utime(path, ns=(atime_ns, mtime_ns))
                    assert digest(path) == value
                    record['events'].append(dict(event='artifact-restored', path=str(path), time=time.time()))
                except BaseException as error:
                    record['restoreErrors'].append(str(path) + ': ' + repr(error))
            record['sharedBuildArtifactsRestored'] = guard_artifacts('restored shared artifact')
    record['sourceUnchanged'] = len(expected) == 537 and matches_all(expected, repo, 'final source')
    record['producerUnchanged'] = matches_all(frozen, prep, 'final producer')
    record['dependencyInputGuards'] = len(config.get('dependencyInputs', {})) == 356 and matches_all(config.get('dependencyInputs', {}), label='final compiled dependency')
    record['untouchedBuildBinariesUnchanged'] = matches_all(config.get('untouchedBuildInputs', {}), label='final untouched binary')
    record['sspArchiveUnchanged'] = isinstance(config.get('sspArchiveSha256'), str) and safe_digest(config.get('sspArchive'), 'final static SSP') == config['sspArchiveSha256']
    if phase == 'unit':
        record['sharedBuildArtifactsUnchanged'] = guard_artifacts('final unit shared artifact')
        if 'exeSha256' in record:
            record['unitExeUnchanged'] = safe_digest(out / 'test_avformat_mux_queue.exe', 'final unit executable') == record['exeSha256']
    else:
        backup_checks = [safe_digest(backup, 'final original backup') == value for _, backup, value, _, _ in backups]
        record['originalBackupsUnchanged'] = len(backup_checks) == 8 and all(backup_checks)
        if unit_file is not None and 'unitVerificationSha256' in record:
            record['unitVerificationUnchanged'] = safe_digest(unit_file, 'final unit receipt') == record['unitVerificationSha256']
        if 'pluginSha256' in record:
            record['candidatePluginUnchanged'] = safe_digest(out / 'libavcodec_plugin.dll', 'final candidate plugin') == record['pluginSha256']
            record['sharedCandidatePluginUnchanged'] = safe_digest(modules / '.libs/libavcodec_plugin.dll', 'final shared candidate plugin') == record['pluginSha256']
    if lock_handle is not None:
        try:
            if lock_held:
                fcntl.flock(lock_handle, fcntl.LOCK_UN)
            lock_handle.close()
            record['writerLockReleased'] = True
        except BaseException as error:
            record['cleanupErrors'].append('lock: ' + repr(error))
    record.update(status='terminal', ended=time.time())
    guards = ['sourceUnchanged', 'producerUnchanged', 'dependencyInputGuards',
        'untouchedBuildBinariesUnchanged', 'sspArchiveUnchanged', 'buildInputGuards', 'writerCleanupConfirmed', 'writerLockReleased']
    guards += ['sharedBuildArtifactsUnchanged', 'unitExeUnchanged'] if phase == 'unit' else [
        'originalBackupsUnchanged', 'unitVerificationUnchanged', 'candidatePluginUnchanged',
        'sharedCandidatePluginUnchanged', 'actualCompileObserved', 'actualLinkObserved',
        'candidateQualifiedBeforeRestore']
    record['success'] = (failure is None and record['exitCode'] == 0
        and not record['forcedBuildCleanupUsed'] and not record['restoreErrors']
        and not record['cleanupErrors'] and not record['verificationErrors'] and not record['receiptWriteErrors']
        and all(record.get(name) is True for name in guards))
    if not save(strict=False):
        record['success'] = False
    for signum, handler in old_handlers.items():
        signal.signal(signum, handler)

print(json.dumps(record, indent=2), flush=True)
if failure is not None:
    raise failure
assert record['success'], 'Qualification build/recovery failed; inspect the terminal receipt'
