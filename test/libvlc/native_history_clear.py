#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Exercise actual loaded media-library clear failure/retry in an owned SQLite DB."""
import argparse
import ctypes
import hashlib
import json
import os
from pathlib import Path
import queue
import sqlite3
import subprocess
import threading
import wave

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--package', type=Path, required=True)
parser.add_argument('--output-dir', type=Path, required=True)
parser.add_argument('--qt-controller', action='store_true', help='Run the Qt controller real-database failure/retry mode')
args = parser.parse_args()
package = args.package.resolve()
output = args.output_dir.resolve()
assert (package / 'portable').is_dir(), 'Use an owned portable test runtime'
output.mkdir(parents=True, exist_ok=False)
for name in ['userdata', 'temp']: (output / name).mkdir()
fixture = output / 'owned.wav'
with wave.open(str(fixture), 'wb') as stream:
    stream.setnchannels(1); stream.setsampwidth(2); stream.setframerate(8000)
    stream.writeframes(b'\0' * 32000)
program = 'test_qt_privacy_preferences.exe' if args.qt_controller else 'test_modules_medialibrary_history.exe'
relative = [program, 'libvlc.dll', 'libvlccore.dll',
            'plugins/misc/libmedialibrary_plugin.dll']
report = dict(passed=False, timedOut=False, forcedCleanupUsed=False,
    runtimeSha256={name: hashlib.sha256((package / name).read_bytes()).hexdigest() for name in relative})
environment = os.environ | dict(VLC_PLUGIN_PATH=str(package / 'plugins'),
    VLC_USERDATA_PATH=str(output / 'userdata'), VLC_HISTORY_FIXTURE=str(fixture),
    TEMP=str(output / 'temp'), TMP=str(output / 'temp'))
if args.qt_controller:
    environment.update(VLC_PRIVACY_PIPE_CONFIG=str(output / 'vlcrc'),
        VLC_PRIVACY_PIPE_SETTINGS=str(output / 'interface.ini'))
command = [str(package / program)] + (['--pipe-history-failure'] if args.qt_controller else [])
child = None
lines = queue.Queue()
transcript = []
connection = None
pump_thread = None
kernel32 = ctypes.WinDLL('kernel32', use_last_error=True)
kernel32.SetErrorMode.argtypes = [ctypes.c_uint]
kernel32.SetErrorMode.restype = ctypes.c_uint
previous = kernel32.SetErrorMode(0x0001 | 0x0002 | 0x8000)
try:
    with (output / 'stderr.txt').open('wb') as stderr:
        try:
            child = subprocess.Popen(command, cwd=output,
                env=environment, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                stderr=stderr, creationflags=subprocess.CREATE_NO_WINDOW)
        finally:
            kernel32.SetErrorMode(previous)
        report['ownedPid'] = child.pid
        def pump():
            for raw in child.stdout:
                text = raw.decode('utf-8', errors='replace').rstrip('\r\n')
                transcript.append(text)
                lines.put(text)
            lines.put(None)
        pump_thread = threading.Thread(target=pump, daemon=True)
        pump_thread.start()
        def until(prefix):
            while True:
                line = lines.get(timeout=30)
                if line is None: raise RuntimeError('Native child exited before ' + prefix)
                if line.startswith(prefix): return line
        until('READY ')
        databases = list((output / 'userdata').rglob('ml.db'))
        assert len(databases) == 1, databases
        database = databases[0]
        report['databaseWithinOwnedUserdata'] = database.is_relative_to(output / 'userdata')
        connection = sqlite3.connect(database, timeout=5)
        def snapshot():
            rows = connection.execute('SELECT id_media, last_position, last_time, play_count, last_played_date, is_favorite FROM Media ORDER BY id_media').fetchall()
            return hashlib.sha256(repr(rows).encode('utf-8')).hexdigest()
        before = snapshot()
        connection.execute("CREATE TRIGGER codex_owned_block_clear BEFORE UPDATE ON Media BEGIN SELECT RAISE(ABORT, 'owned history clear failure'); END")
        connection.commit()
        report['realDatabaseWriteRejectionInstalled'] = True
        child.stdin.write(b'blocked\n'); child.stdin.flush()
        until('RETRY')
        report['failedClearDataUnchanged'] = snapshot() == before
        connection.execute('DROP TRIGGER codex_owned_block_clear'); connection.commit()
        connection.close(); connection = None
        report['writeRejectionRemovedForRetry'] = True
        child.stdin.write(b'retry\n'); child.stdin.flush(); child.stdin.close()
        report['exitCode'] = child.wait(timeout=30)
except (subprocess.TimeoutExpired, queue.Empty):
    report['timedOut'] = True
except Exception as error:
    report['error'] = str(error)
finally:
    if connection is not None:
        try:
            connection.execute('DROP TRIGGER IF EXISTS codex_owned_block_clear'); connection.commit()
        except sqlite3.Error: pass
        connection.close()
    if child is not None and child.poll() is None:
        report['forcedCleanupUsed'] = True
        child.kill(); report['exitCode'] = child.wait(timeout=5)
    report['terminationConfirmed'] = child is not None and child.poll() is not None
    if pump_thread is not None:
        pump_thread.join(timeout=5)
    report['stdoutReaderTerminated'] = pump_thread is not None and not pump_thread.is_alive()
    (output / 'stdout.txt').write_text('\n'.join(transcript) + '\n', encoding='utf-8', newline='\n')
report['normalExit'] = report.get('exitCode') == 0 and not report['forcedCleanupUsed']
report['passLines'] = [line for line in transcript if line.startswith('PASS:')]
report['failLines'] = [line for line in transcript if line.startswith('FAIL:')]
report['passed'] = (report['normalExit'] and report['stdoutReaderTerminated'] and not report['timedOut']
    and not report['failLines'] and report.get('failedClearDataUnchanged') is True
    and ('RESULT: 9 checks, 0 failures' if args.qt_controller else 'RESULT: 40 checks, 0 failures') in transcript)
with (output / 'verification.json').open('w', encoding='utf-8', newline='\n') as stream:
    stream.write(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, indent=2), flush=True)
raise SystemExit(0 if report['passed'] else 1)
