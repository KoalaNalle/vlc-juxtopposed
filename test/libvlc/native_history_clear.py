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
parser.add_argument('--library-off', action='store_true', help='Exercise the lazy history backend with Library disabled')
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
environment.pop('VLC_HISTORY_LIBRARY_OFF', None)
if args.library_off: environment['VLC_HISTORY_LIBRARY_OFF'] = '1'
report['libraryOff'] = args.library_off
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
        background_before = None
        if args.library_off and not args.qt_controller:
            seeded = until('SEEDED cached=')
            cached_id = int(seeded.split('=')[1])
            databases = list((output / 'userdata').rglob('ml.db'))
            assert len(databases) == 1, databases
            with sqlite3.connect(databases[0], timeout=5) as seed:
                seed.execute('PRAGMA foreign_keys=ON')
                device = seed.execute("INSERT INTO Device(uuid,scheme,is_removable,is_present,is_network,last_seen) VALUES('codex-owned-removable','file://',1,1,0,1)").lastrowid
                mountpoint = fixture.parent.as_uri() + '/'
                seed.execute('INSERT INTO DeviceMountpoint(device_id,mrl,last_seen) VALUES(?,?,1)', (device,mountpoint))
                folder = seed.execute("INSERT INTO Folder(path,name,device_id,is_removable,is_public) VALUES('','Owned cached root',?,1,1)", (device,)).lastrowid
                seed.execute('UPDATE File SET mrl=?,folder_id=?,is_removable=1,is_external=0 WHERE media_id=?', (fixture.name+'.cached.wav',folder,cached_id))
                seed.execute("INSERT INTO Device(uuid,scheme,is_removable,is_present,is_network,last_seen) VALUES('codex-owned-network','smb://',1,1,1,1)")
                # Valid pending Creation task from SDK 0.13.2's Task schema.
                seed.execute('INSERT INTO Task(attempts_left,type,mrl,file_type,parent_folder_id,link_to_id,link_to_type,link_extra,link_to_mrl) VALUES(3,0,?,0,?,0,0,0,?)', (mountpoint+'owned-pending.wav',folder,''))
                seed.commit()
                def background_snapshot(db):
                    return {name: db.execute('SELECT * FROM '+name+' ORDER BY 1').fetchall() for name in ['Device','DeviceMountpoint','Folder','Task']}
                background_before = background_snapshot(seed)
            report['ownedPendingTaskAndCachedRemovablePathSeeded'] = True
            child.stdin.write(b'offline\n'); child.stdin.flush()
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
        if background_before is not None:
            with sqlite3.connect(database, timeout=5) as read:
                report['backgroundDatabaseRowsUnchanged'] = background_snapshot(read) == background_before
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
    and f'RESULT: {9 if args.qt_controller else 52 if args.library_off else 40} checks, 0 failures' in transcript
    and (report.get('backgroundDatabaseRowsUnchanged') is True if args.library_off and not args.qt_controller else True))
with (output / 'verification.json').open('w', encoding='utf-8', newline='\n') as stream:
    stream.write(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, indent=2), flush=True)
raise SystemExit(0 if report['passed'] else 1)
