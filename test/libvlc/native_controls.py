#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Run real Controls input tests in a fresh owned Windows portable runtime."""
import argparse
import ctypes
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import time

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--package', type=Path, required=True)
parser.add_argument('--core', type=Path, required=True)
parser.add_argument('--hotkeys', type=Path, required=True)
parser.add_argument('--executable', type=Path, required=True)
parser.add_argument('--fixture', type=Path, required=True)
parser.add_argument('--output-dir', type=Path, required=True)
parser.add_argument('--old-core', type=Path)
parser.add_argument('--before-click-core', type=Path)
parser.add_argument('--case', action='append', choices=('chain', 'reverse', 'legacy', 'binding', 'input', 'embedding', 'legacy-disabled'))
args = parser.parse_args()
output = args.output_dir.resolve()
output.mkdir(parents=True, exist_ok=False)
runtime = output / 'runtime'
shutil.copytree(args.package, runtime)
(runtime / 'portable').mkdir(exist_ok=True)
shutil.copy2(args.core, runtime / 'libvlccore.dll')
shutil.copy2(args.hotkeys, runtime / 'plugins/control/libhotkeys_plugin.dll')
shutil.copy2(args.fixture, runtime / 'plugins/misc/libcontrols_fixture_plugin.dll')
shutil.copy2(args.executable, runtime / 'test_controls_input.exe')
media = output / 'owned-controls.y4m'
with media.open('wb') as stream:
    stream.write(b'YUV4MPEG2 W16 H16 F5:1 Ip A1:1 C420jpeg\n')
    for frame in range(900):
        stream.write(b'FRAME\n' + bytes([32 + frame % 180]) * 256
                     + bytes([96]) * 64 + bytes([160]) * 64)
report = dict(passed=False, cases={}, runtimeSha256={})
for name in ('test_controls_input.exe', 'libvlc.dll', 'libvlccore.dll',
             'plugins/control/libhotkeys_plugin.dll', 'plugins/misc/libcontrols_fixture_plugin.dll'):
    report['runtimeSha256'][name] = hashlib.sha256((runtime / name).read_bytes()).hexdigest()
kernel = ctypes.WinDLL('kernel32', use_last_error=True)
kernel.SetErrorMode.argtypes = [ctypes.c_uint]
kernel.SetErrorMode.restype = ctypes.c_uint
user = ctypes.WinDLL('user32', use_last_error=True)
user.GetDoubleClickTime.restype = ctypes.c_uint
settle_ms = user.GetDoubleClickTime() + 250
report['hostDoubleClickIntervalMs'] = settle_ms - 250

def run_case(name, mode, expected, expected_exit=0, click_failure=False):
    case = output / name
    case.mkdir()
    for folder in ('temp', 'appdata', 'localappdata', 'userdata'):
        (case / folder).mkdir()
    env = {k: v for k, v in os.environ.items() if not k.startswith('VLC_')}
    env.update(VLC_PLUGIN_PATH=str(runtime / 'plugins'), VLC_USERDATA_PATH=str(case / 'userdata'),
               APPDATA=str(case / 'appdata'), LOCALAPPDATA=str(case / 'localappdata'),
               TEMP=str(case / 'temp'), TMP=str(case / 'temp'),
               CONTROLS_FIXTURE_SETTLE_MS=str(settle_ms))
    result = dict(passed=False, timedOut=False, forcedCleanupUsed=False, expectedChecks=expected)
    report['cases'][name] = result
    started = time.monotonic()
    child = None
    try:
        with (case / 'stdout.txt').open('wb') as stdout, (case / 'stderr.txt').open('wb') as stderr:
            previous = kernel.SetErrorMode(0x8003)
            try:
                child = subprocess.Popen([str(runtime / 'test_controls_input.exe'), mode, media.as_uri()],
                    cwd=runtime, env=env, stdout=stdout, stderr=stderr,
                    creationflags=subprocess.CREATE_NO_WINDOW)
            finally:
                kernel.SetErrorMode(previous)
            result['ownedPid'] = child.pid
            try:
                result['exitCode'] = child.wait(timeout=max(45, settle_ms / 1000 * 18 + 20))
            except subprocess.TimeoutExpired:
                result.update(timedOut=True, forcedCleanupUsed=True)
                child.kill()
                result['exitCode'] = child.wait(timeout=5)
    finally:
        if child is not None and child.poll() is None:
            result['forcedCleanupUsed'] = True
            child.kill()
            result['exitCode'] = child.wait(timeout=5)
        result.update(terminationConfirmed=child is not None and child.poll() is not None,
                      elapsedSeconds=round(time.monotonic() - started, 3))
    text = (case / 'stdout.txt').read_text(encoding='utf-8', errors='replace')
    result['checks'] = len(re.findall(r'^(?:ok|not ok) \d+ ', text, re.M))
    result['failedChecks'] = re.findall(r'^not ok .*', text, re.M)
    result['normalExit'] = result['exitCode'] == expected_exit and not result['forcedCleanupUsed']
    result['expectedFailure'] = bool(expected_exit)
    count_matches = result['checks'] == expected
    if click_failure:
        count_matches = (12 <= result['checks'] <= 33
            and all(re.search(r'^ok ' + str(n) + r' ', text, re.M) for n in range(1, 12)))
        result['clickConsumerFailureAfterSuccessfulSetup'] = bool(count_matches)
    result['passed'] = (result['normalExit'] and count_matches
                        and bool(result['failedChecks']) == bool(expected_exit))
    print(json.dumps(dict(case=name, **result)), flush=True)
    assert result['passed'], name + ': ' + text

try:
    if args.old_core:
        report['oldChainCoreSha256'] = hashlib.sha256(args.old_core.read_bytes()).hexdigest()
        shutil.copy2(args.old_core, runtime / 'libvlccore.dll')
        run_case('old-core-chain', 'chain', 2, 1)
        shutil.copy2(args.core, runtime / 'libvlccore.dll')
    if args.before_click_core:
        report['beforeClickCoreSha256'] = hashlib.sha256(args.before_click_core.read_bytes()).hexdigest()
        shutil.copy2(args.before_click_core, runtime / 'libvlccore.dll')
        run_case('before-click-core', 'input', 33, 1, click_failure=True)
        shutil.copy2(args.core, runtime / 'libvlccore.dll')
    for name, expected in [('chain', 2), ('reverse', 8), ('legacy', 8), ('binding', 8),
                           ('embedding', 16), ('legacy-disabled', 14), ('input', 33)]:
        if args.case and name not in args.case:
            continue
        run_case(name, name, expected)
    report['passed'] = True
except Exception as error:
    report['error'] = str(error)
finally:
    (output / 'verification.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8', newline='\n')
print(json.dumps(dict(passed=report['passed'], nativeCases=len(report['cases']))), flush=True)
raise SystemExit(0 if report['passed'] else 1)
