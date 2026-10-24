#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Run the real Controls preferences controller in an owned portable runtime."""
import argparse
import ctypes
import hashlib
import json
import os
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--package', type=Path, required=True)
parser.add_argument('--output-dir', type=Path, required=True)
parser.add_argument('--executable', default='test_qt_controls_preferences.exe',
                    choices=('test_qt_controls_preferences.exe', 'test_qt_controls_dialogs.exe'))
parser.add_argument('--expected-cases', type=int, default=19)
args = parser.parse_args()
package, output = args.package.resolve(), args.output_dir.resolve()
assert (package / 'portable').is_dir(), 'Use an owned portable native test runtime'
output.mkdir(parents=True, exist_ok=False)
for folder in ('temp', 'appdata', 'localappdata'):
    (output / folder).mkdir()
report = dict(passed=False, expectedCases=args.expected_cases, forcedCleanupUsed=False, runtimeSha256={})
for name in (args.executable, 'libvlc.dll', 'libvlccore.dll', 'plugins/lua/liblua_plugin.dll'):
    report['runtimeSha256'][name] = hashlib.sha256((package / name).read_bytes()).hexdigest()
env = {key: value for key, value in os.environ.items() if not key.startswith('VLC_')}
env.update(VLC_PLUGIN_PATH=str(package / 'plugins'), TEMP=str(output / 'temp'), TMP=str(output / 'temp'),
           APPDATA=str(output / 'appdata'), LOCALAPPDATA=str(output / 'localappdata'))
kernel = ctypes.WinDLL('kernel32', use_last_error=True)
kernel.SetErrorMode.argtypes = [ctypes.c_uint]
kernel.SetErrorMode.restype = ctypes.c_uint
child = None
try:
    with (output / 'stdout.txt').open('wb') as stdout, (output / 'stderr.txt').open('wb') as stderr:
        previous = kernel.SetErrorMode(0x8003)
        try:
            child = subprocess.Popen([str(package / args.executable),
                '-o', str(output / 'qtest.txt') + ',txt'], cwd=output, env=env,
                stdout=stdout, stderr=stderr, creationflags=subprocess.CREATE_NO_WINDOW)
        finally:
            kernel.SetErrorMode(previous)
        report['ownedPid'] = child.pid
        try:
            report['exitCode'] = child.wait(timeout=90)
        except subprocess.TimeoutExpired:
            report['forcedCleanupUsed'] = True
            child.kill()
            report['exitCode'] = child.wait(timeout=5)
    text = (output / 'qtest.txt').read_text(encoding='utf-8') if (output / 'qtest.txt').exists() else ''
    report['normalExit'] = report['exitCode'] == 0 and not report['forcedCleanupUsed']
    report['passed'] = report['normalExit'] and f'Totals: {args.expected_cases} passed, 0 failed, 0 skipped, 0 blacklisted' in text
finally:
    if child is not None and child.poll() is None:
        report['forcedCleanupUsed'] = True
        child.kill()
        child.wait(timeout=5)
    (output / 'verification.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8', newline='\n')
print(json.dumps(report), flush=True)
raise SystemExit(0 if report['passed'] else 1)
