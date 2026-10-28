"""Run only the reviewed native queue unit executable in a fresh two-file runtime.

Preparation may parse this source; it must not import or execute the producer.
"""
import argparse
import ctypes
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
parser.add_argument('--execute-reviewed-unit', action='store_true', required=True)
parser.parse_args()
prep = Path(__file__).resolve().parent
repo = Path(r'C:\Modding\vlc-juxtopposed')
out = prep / 'unit'
assert not out.exists(), 'Use a fresh reviewed unit output; never overwrite a receipt'
out.mkdir()
runtime = out / 'runtime'
errors = []
record = dict(status='running', started=time.time(), phase='preflight',
    scope='Native production queue helper and real Core FIFO/frame tests only; no FFmpeg or VLC playback.',
    normalExit=False, forcedCleanupUsed=False, exitCode=None,
    cases=0, checks=0, failures=None, inspectionErrors=errors)
config = inputs = producers = build = None
build_file = build_exe = base_core = None
copied_exe = runtime / 'test_avformat_mux_queue.exe'
linked_core = runtime / 'libvlccore.dll'
child = kernel = previous_error_mode = failure = None
old_handlers = {}


def digest(path):
    value = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b''):
            value.update(chunk)
    return value.hexdigest()


def safe_digest(path, check):
    try:
        return digest(path)
    except BaseException as error:
        errors.append(dict(check=check, path=str(path), error=repr(error)))
        return None


def matches_all(mapping, check):
    values = []
    for path, expected in mapping.items():
        actual = safe_digest(path, check)
        values.append(actual == expected)
        if actual is not None and actual != expected:
            errors.append(dict(check=check, path=str(path), error='SHA-256 mismatch',
                               expected=expected, actual=actual))
    return bool(mapping) and all(values)


def save(strict=True):
    try:
        temporary = out / 'verification.next.json'
        temporary.write_text(json.dumps(record, indent=2) + '\n')
        os.replace(temporary, out / 'verification.json')
        return True
    except BaseException as error:
        errors.append(dict(check='receipt-write', error=repr(error)))
        if record['status'] == 'terminal':
            record['success'] = False
        try:
            (out / 'verification.json').write_text(json.dumps(record, indent=2) + '\n')
            return True
        except BaseException as second_error:
            errors.append(dict(check='receipt-write-fallback', error=repr(second_error)))
            if strict:
                raise
            print(json.dumps(record, indent=2), file=sys.stderr, flush=True)
            return False


def load_map(path):
    value = json.loads(path.read_text(encoding='utf-8-sig'))
    assert isinstance(value, dict), str(path) + ' must contain an object'
    return value


def before_deadline():
    assert time.time() + 45 < config['stopByUnix'], 'Insufficient time for bounded unit execution and checkpoint'


def interrupted(signum, _frame):
    raise InterruptedError('Unit producer received signal ' + str(signum))


try:
    for signum in (signal.SIGINT, signal.SIGTERM, getattr(signal, 'SIGBREAK', None)):
        if signum is not None:
            old_handlers[signum] = signal.signal(signum, interrupted)
    save()
    config = load_map(prep / 'preflight.json')
    producers = load_map(prep / 'producer-identities.json')
    inputs = load_map(prep / 'build-inputs.json')
    assert len(inputs) == 537, 'Keep the complete reviewed source inventory strict'
    excluded = ('test/libvlc/native_soxr_reference.py',
        'test/modules/audio_filter/soxr_lifecycle_trace.c',
        'test/modules/audio_filter/soxr_native_reference.c',
        'test/modules/audio_filter/soxr_pcm_trace.c')
    assert not any(name.replace('\\', '/') in excluded for name in inputs)
    assert matches_all({prep / name: value for name, value in producers.items()}, 'producer preflight')
    assert matches_all({repo / name: value for name, value in inputs.items()}, 'source preflight')
    before_deadline()

    build_file = prep / 'unit-build/verification.json'
    build = load_map(build_file)
    assert build['status'] == 'terminal' and build['success'] is True and build['exitCode'] == 0
    assert all(build[name] is True for name in ('sourceUnchanged', 'producerUnchanged',
        'dependencyInputGuards', 'untouchedBuildBinariesUnchanged'))
    build_exe = prep / 'unit-build/test_avformat_mux_queue.exe'
    assert digest(build_exe) == build['exeSha256']
    assert 0 < build_exe.stat().st_size <= 8 * 1024 * 1024
    preserved_runtime = repo / config['preservedRuntime']
    assert preserved_runtime.resolve().is_relative_to(repo.resolve()), 'Preserved runtime must stay in the home project'
    base_core = preserved_runtime / 'libvlccore.dll'
    assert config['testCoreSha256'] == 'f8fc980bbe03da57908f79045f266710dd27a16c6ee66b6c45d60cd1f9b93598'
    assert digest(base_core) == config['testCoreSha256']
    assert base_core.drive.upper() == runtime.drive.upper(), 'Core must be hardlinked on the same volume'
    budget = config['unitStorageBudget']
    assert budget['reserveFraction'] == .2
    assert all(isinstance(value, int) and value >= 0 for value in budget['componentsBytes'].values())
    assert sum(budget['componentsBytes'].values()) == budget['peakAdditionalBytes'] > 0
    disk = shutil.disk_usage(repo)
    reserve = (disk.total + 4) // 5
    record['storageBeforeStage'] = dict(totalBytes=disk.total, freeBytes=disk.free,
        minimumFreeBytes=reserve, availableHeadroomBytes=disk.free - reserve,
        peakAdditionalBytes=budget['peakAdditionalBytes'],
        fitsBelow80Percent=disk.free - reserve >= budget['peakAdditionalBytes'])
    record.update(buildVerificationSha256=digest(build_file), unitExeSha256=build['exeSha256'],
        testCoreSha256=config['testCoreSha256'], sourceSha256=inputs,
        producerSha256=digest(__file__), preservedCorePath=str(base_core),
        deadlineUnix=config['stopByUnix'])
    save()
    assert record['storageBeforeStage']['fitsBelow80Percent'], 'Unit peak exceeds SSD headroom below80percent'

    record['phase'] = 'staging'
    runtime.mkdir()
    shutil.copyfile(build_exe, copied_exe)
    os.link(base_core, linked_core)  # No copy fallback, including on EMLINK.
    assert digest(copied_exe) == build['exeSha256']
    assert copied_exe.stat().st_size == build_exe.stat().st_size <= 8 * 1024 * 1024
    assert digest(linked_core) == digest(base_core) == config['testCoreSha256']
    assert {path.name for path in runtime.iterdir()} == {'test_avformat_mux_queue.exe', 'libvlccore.dll'}
    record['runtimeBeforeSha256'] = {copied_exe.name: digest(copied_exe), linked_core.name: digest(linked_core)}
    before_deadline()
    kernel = ctypes.WinDLL('kernel32', use_last_error=True)
    kernel.SetErrorMode.argtypes = [ctypes.c_uint]
    kernel.SetErrorMode.restype = ctypes.c_uint
    previous_error_mode = kernel.SetErrorMode(0x8003)
    record['phase'] = 'unit'
    with (out / 'stdout.txt').open('wb') as stdout, (out / 'stderr.txt').open('wb') as stderr:
        child = subprocess.Popen([str(copied_exe)], cwd=runtime, stdout=stdout,
            stderr=stderr, creationflags=subprocess.CREATE_NO_WINDOW)
        record['pid'] = child.pid
        save()
        try:
            record['exitCode'] = child.wait(timeout=30)
        except subprocess.TimeoutExpired:
            record['timedOut'] = True
            raise
    record['normalExit'] = record['exitCode'] == 0
    record['phase'] = 'inspection'
    stdout_text = (out / 'stdout.txt').read_text(errors='strict')
    stderr_text = (out / 'stderr.txt').read_text(errors='strict')
    rows = [json.loads(line) for line in stdout_text.splitlines() if line.strip()]
    case_rows = [row for row in rows if 'case' in row and 'checks' in row and 'failures' in row]
    summaries = [row for row in rows if 'case' not in row and 'cases' in row]
    record['caseRows'] = case_rows
    assert len(case_rows) == 41 and len(summaries) == 1 and len(rows) == 42
    assert len({row['case'] for row in case_rows}) == 41
    assert all(row['passed'] is True and row['failures'] == 0 and row['checks'] > 0 for row in case_rows)
    summary = summaries[0]
    record.update(cases=summary['cases'], checks=summary['checks'], failures=summary['failures'])
    assert record['cases'] == 41 and record['failures'] == 0 and record['checks'] > 0
    assert sum(row['checks'] for row in case_rows) == record['checks']
    assert not stderr_text, 'Native unit emitted unexpected stderr'
    assert record['normalExit'], 'Native unit did not exit successfully'
except BaseException as error:
    failure = error
    record['failure'] = repr(error)
finally:
    for signum in old_handlers:
        signal.signal(signum, signal.SIG_IGN)
    # The unit has no child-job API. Stop only this exact Popen-owned process;
    # never find/terminate other executables by name or touch a parent build.
    if child is not None:
        try:
            if child.poll() is None:
                record['forcedCleanupUsed'] = True
                child.kill()
            record['exitCode'] = child.wait(timeout=5)
        except BaseException as cleanup_error:
            errors.append(dict(check='owned-unit-cleanup', error=repr(cleanup_error)))
    if kernel is not None and previous_error_mode is not None:
        try:
            kernel.SetErrorMode(previous_error_mode)
        except BaseException as mode_error:
            errors.append(dict(check='error-mode-restore', error=repr(mode_error)))
    try:
        record['allSourceInputsUnchanged'] = bool(inputs) and matches_all(
            {repo / name: value for name, value in inputs.items()}, 'final source')
        record['allProducersUnchanged'] = bool(producers) and matches_all(
            {prep / name: value for name, value in producers.items()}, 'final producer')
        record['buildVerificationUnchanged'] = bool(build_file and 'buildVerificationSha256' in record) and (
            safe_digest(build_file, 'final build receipt') == record['buildVerificationSha256'])
        expected_exe = build.get('exeSha256') if isinstance(build, dict) else None
        expected_core = config.get('testCoreSha256') if isinstance(config, dict) else None
        record['unitBuildExeUnchanged'] = isinstance(expected_exe, str) and safe_digest(build_exe, 'final built unit') == expected_exe
        record['runtimeExeUnchanged'] = isinstance(expected_exe, str) and safe_digest(copied_exe, 'final copied unit') == expected_exe
        record['preservedCoreUnchanged'] = isinstance(expected_core, str) and safe_digest(base_core, 'final preserved Core') == expected_core
        record['runtimeCoreUnchanged'] = isinstance(expected_core, str) and safe_digest(linked_core, 'final linked Core') == expected_core
        record['runtimeFileSetUnchanged'] = runtime.is_dir() and {path.name for path in runtime.iterdir()} == {
            'test_avformat_mux_queue.exe', 'libvlccore.dll'}
    except BaseException as inspection_error:
        errors.append(dict(check='final inspection', error=repr(inspection_error)))
    record['normalExit'] = child is not None and record['exitCode'] == 0 and not record['forcedCleanupUsed']
    record.update(status='terminal', ended=time.time())
    guard_names = ('allSourceInputsUnchanged', 'allProducersUnchanged', 'buildVerificationUnchanged',
        'unitBuildExeUnchanged', 'runtimeExeUnchanged', 'preservedCoreUnchanged',
        'runtimeCoreUnchanged', 'runtimeFileSetUnchanged')
    record['success'] = (failure is None and not errors and record['normalExit']
        and record['cases'] == 41 and record['failures'] == 0 and record['checks'] > 0
        and all(record.get(name) is True for name in guard_names))
    if not save(strict=False):
        record['success'] = False
    for signum, handler in old_handlers.items():
        signal.signal(signum, handler)

print(json.dumps(record, indent=2), flush=True)
if failure is not None:
    raise failure
assert record['success'], 'Native unit qualification failed; inspect its terminal receipt'
