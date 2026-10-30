"""One-use real-player notification baseline; no GUI automation or test hooks."""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[2]
WORK = ROOT / 'build-windows/beta-record-transition-20261029'
OUT = WORK / 'before-r2'
BASE = ROOT / 'build-windows/beta-resume-20261029/gui-r9/runtime'
BUILD = WORK / 'test-build-r3'
sys.path.insert(0, str(ROOT / 'test/libvlc'))
import native_http_runtime as support


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    assert not OUT.exists()
    build = json.loads((BUILD / 'verification.json').read_text())
    assert build['passed'] and sha(BUILD / 'recording.exe') == build['binarySha256']
    assert sha(ROOT / 'test/src/player/recording.c') == build['sourceSha256']
    assert sha(BASE / 'libvlccore.dll') == 'f09c45fe4075a02836807f00cab03163cdba1ec79e37ba35a0328f7cb6d5462d'
    OUT.mkdir()
    runtime = OUT / 'runtime'
    support.stage_runtime(BASE, runtime)
    support.regenerate_cache(runtime, OUT / 'cache')
    expected = {str(p.relative_to(runtime)): sha(p) for p in runtime.rglob('*')
                if p.is_file() and 'portable' not in p.relative_to(runtime).parts
                and p.name != 'plugins.dat'}
    binary = OUT / 'recording.exe'
    os.link(BUILD / 'recording.exe', binary)
    fixture = WORK / 'fixtures/four-seconds.mp4'
    recordings = OUT / 'recordings'
    recordings.mkdir()
    inputs = {str(p): sha(p) for p in [Path(__file__), binary, fixture,
              Path(support.__file__), ROOT / 'test/src/player/recording.c',
              ROOT / 'src/player/input.c', ROOT / 'lib/libvlc_internal.h']}
    env = support.environment(runtime, OUT)
    env['VLC_TEST_PLUGIN_PATH'] = str(runtime / 'plugins')
    command = [str(binary), str(fixture), str(recordings)]
    record = dict(status='starting', scope='Actual Core recording notifications; real generated A/V; no content or GUI claim',
                  inputs=inputs, build=build, runtimeSha256=expected,
                  forcedCleanupUsed=False, command=command, startedUtc=time.time())

    def save():
        (OUT / 'verification.json').write_text(json.dumps(record, indent=2) + '\n')

    save()
    with (OUT / 'stdout.txt').open('wb') as stdout, (OUT / 'stderr.txt').open('wb') as stderr:
        child = subprocess.Popen(command, cwd=runtime, env=env, stdout=stdout, stderr=stderr,
                                 creationflags=subprocess.CREATE_NO_WINDOW)
        record.update(status='running', pid=child.pid)
        save()
        print('OWNED_NATIVE_RUNNING ' + str(child.pid), flush=True)
        try:
            code = child.wait(timeout=180)
        except subprocess.TimeoutExpired:
            record.update(status='observation-timeout-process-left-running')
            save()
            raise SystemExit('Inspect exact owned PID; do not restart')
    text = (OUT / 'stdout.txt').read_text(errors='replace')
    record.update(status='terminal', exitCode=code, stdout=text, finishedUtc=time.time(),
                  inputsUnchanged=all(sha(p) == value for p, value in inputs.items()),
                  runtimeUnchanged=all(sha(runtime / p) == value for p, value in expected.items()),
                  normalTestResult=code in (0, 1) and re.search(r'^RESULT cases=7 failures=\d+$', text, re.M) is not None)
    save()
    print(json.dumps({k: record[k] for k in ['exitCode', 'inputsUnchanged', 'runtimeUnchanged', 'normalTestResult', 'stdout']}, indent=2))
    assert record['inputsUnchanged'] and record['runtimeUnchanged'] and record['normalTestResult']


if __name__ == '__main__':
    main()
