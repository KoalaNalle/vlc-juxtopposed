#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Run the real SoXR module clock contract in an owned portable Windows runtime."""
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
args = parser.parse_args()
package = args.package.resolve()
output = args.output_dir.resolve()
assert (package / 'portable').is_dir()
output.mkdir(parents=True, exist_ok=False)
(output / 'temp').mkdir()
relative = ['test_modules_soxr_timestamps.exe', 'libvlc.dll', 'libvlccore.dll',
            'plugins/audio_filter/libsoxr_plugin.dll']
hashes = {name: hashlib.sha256((package / name).read_bytes()).hexdigest() for name in relative}
environment = os.environ | dict(VLC_PLUGIN_PATH=str(package / 'plugins'),
    VLC_USERDATA_PATH=str(output / 'userdata'), TEMP=str(output / 'temp'), TMP=str(output / 'temp'))
report = dict(runtimeSha256=hashes, passed=False, timedOut=False, forcedCleanupUsed=False)
with (output / 'stdout.txt').open('wb') as stdout, (output / 'stderr.txt').open('wb') as stderr:
    kernel32 = ctypes.WinDLL('kernel32', use_last_error=True)
    kernel32.SetErrorMode.argtypes = [ctypes.c_uint]
    kernel32.SetErrorMode.restype = ctypes.c_uint
    previous = kernel32.SetErrorMode(0x0001 | 0x0002 | 0x8000)
    try:
        process = subprocess.Popen([str(package / relative[0])], cwd=output, env=environment,
            stdout=stdout, stderr=stderr, creationflags=subprocess.CREATE_NO_WINDOW)
    finally:
        kernel32.SetErrorMode(previous)
    report['ownedPid'] = process.pid
    (output / 'pid.txt').write_text(str(process.pid), encoding='utf-8')
    try:
        code = process.wait(timeout=45)
    except subprocess.TimeoutExpired:
        report.update(timedOut=True, forcedCleanupUsed=True)
        process.kill()
        code = process.wait(timeout=5)
report.update(exitCode=code, normalExit=code == 0 and not report['forcedCleanupUsed'],
              terminationConfirmed=process.poll() is not None)
text = (output / 'stdout.txt').read_text(encoding='utf-8', errors='replace')
report.update(passLines=[line for line in text.splitlines() if line.startswith('PASS:')],
              failLines=[line for line in text.splitlines() if line.startswith('FAIL:')])
report['passed'] = report['normalExit'] and not report['timedOut'] and not report['failLines'] and 'RESULT: 16 cases, 0 failures' in text
with (output / 'verification.json').open('w', encoding='utf-8', newline='\n') as stream:
    stream.write(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, indent=2), flush=True)
raise SystemExit(0 if report['passed'] else 1)
