#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Check Library-off history with real decoded playback in an owned portable runtime."""
import argparse
import ctypes
import hashlib
import json
import os
from pathlib import Path
import sqlite3
import subprocess
import time

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--package', type=Path, required=True)
parser.add_argument('--output-dir', type=Path, required=True)
args = parser.parse_args()
package, output = args.package.resolve(), args.output_dir.resolve()
assert (package / 'portable').is_dir(), 'Use an owned portable test runtime'
output.mkdir(parents=True, exist_ok=False)
(output / 'temp').mkdir()
identities = ['vlc.exe', 'libvlc.dll', 'libvlccore.dll',
              'plugins/gui/libqt_plugin.dll', 'plugins/misc/libmedialibrary_plugin.dll']
report = dict(passed=False, cases={},
    runtimeSha256={name: hashlib.sha256((package / name).read_bytes()).hexdigest() for name in identities})
environment = {key: value for key, value in os.environ.items() if not key.startswith('VLC_')}
for key in ('QML_DISABLE_DISK_CACHE', 'QML_FORCE_DISK_CACHE', 'QT_QUICK_BACKEND'):
    environment.pop(key, None)
environment.update(VLC_PLUGIN_PATH=str(package / 'plugins'), QML_DISK_CACHE='aot',
    QT_DISABLE_SHADER_DISK_CACHE='1', QT_QPA_PLATFORM='windows',
    TEMP=str(output / 'temp'), TMP=str(output / 'temp'))
# 60 seconds of seekable video, 117 KiB, without an encoder or personal media.
media = output / 'owned-history.y4m'
with media.open('wb') as stream:
    stream.write(b'YUV4MPEG2 W16 H16 F5:1 Ip A1:1 C420jpeg\n')
    for frame in range(300):
        stream.write(b'FRAME\n' + bytes([32 + frame % 180]) * 256
                     + bytes([96]) * 64 + bytes([160]) * 64)
profile = output / 'shared-profile'
profile.mkdir()
config = profile / 'vlcrc'
config.write_text('media-library=0\nmetadata-network-access=0\nml-network-discovery=0\n'
                  'one-instance=0\nqt-updates-notif=0\nrestore-playback-pos=2\n',
                  encoding='utf-8', newline='\n')
kernel = ctypes.WinDLL('kernel32', use_last_error=True)
kernel.SetErrorMode.argtypes = [ctypes.c_uint]
kernel.SetErrorMode.restype = ctypes.c_uint
first_progress = None
try:
    for name, options in [
            ('save', ['--start-time=10', '--stop-time=13']),
            ('resume', ['--start-time=0', '--stop-time=17']),
            ('resume-before-start', ['--start-time=20', '--stop-time=22']),
            ('resume-after-stop', ['--start-time=0', '--stop-time=2']),
            ('position-off', ['--no-remember-playback-pos', '--start-time=0', '--stop-time=2']),
            ('history-off', ['--no-save-recentplay', '--no-restore-playback-states', '--start-time=0', '--stop-time=2'])]:
        case = output / name
        case.mkdir()
        frames = case / 'frames'
        frames.mkdir()
        userdata = case / 'userdata' if name == 'history-off' else profile / 'userdata'
        log = case / 'vlc.log'
        env = environment | dict(VLC_USERDATA_PATH=str(userdata), QML_DISK_CACHE_PATH=str(case / 'qml-cache'))
        command = [str(package / 'vlc.exe'), '--intf=qt', '--no-one-instance',
            '--no-media-library', '--no-plugins-cache', '--no-crashdump',
            '--no-qt-privacy-ask', '--no-qt-updates-notif', '--no-metadata-network-access',
            '--no-ml-network-discovery', '--gain=0', '--no-hw-dec', '--play-and-exit',
            '--file-logging', '--log-verbose=2', '--verbose=2', '--logfile=' + str(log),
            '--config=' + str(config), '--qt-juxtopposed-controls', '--video-filter=scene',
            '--scene-path=' + str(frames), '--scene-ratio=1', '--scene-prefix=frame',
            '--scene-replace', *options, str(media)]
        result = dict(passed=False, timedOut=False, forcedCleanupUsed=False)
        report['cases'][name] = result
        (case / 'launch.json').write_text(json.dumps(command, indent=2) + '\n', encoding='utf-8', newline='\n')
        child = None
        started = time.monotonic()
        try:
            with (case / 'stdout.txt').open('wb') as stdout, (case / 'stderr.txt').open('wb') as stderr:
                previous = kernel.SetErrorMode(0x8003)
                try:
                    child = subprocess.Popen(command, cwd=package, env=env, stdout=stdout,
                        stderr=stderr, creationflags=subprocess.CREATE_NO_WINDOW)
                finally:
                    kernel.SetErrorMode(previous)
                result['ownedPid'] = child.pid
                try:
                    result['exitCode'] = child.wait(timeout=40)
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
        result['normalExit'] = result.get('exitCode') == 0 and not result['forcedCleanupUsed']
        assert result['normalExit'], result
        text = log.read_text(encoding='utf-8', errors='replace') if log.exists() else ''
        diagnostics = ('ReferenceError', 'TypeError', 'Binding loop', 'Cannot assign',
            'Unable to assign', 'unregistered datatype', 'is not a type', 'failed to load component')
        result.update(qtLoaded='using interface module "qt"' in text,
            d3d11Loaded='using vout display module "direct3d11"' in text,
            decodedFrames=[path.name for path in frames.glob('*.png') if path.stat().st_size > 0],
            qmlDiagnostics=[line for line in text.splitlines() if any(term in line for term in diagnostics)])
        assert result['qtLoaded'] and result['d3d11Loaded'] and result['decodedFrames'] and not result['qmlDiagnostics'], result
        database = userdata / 'ml/ml.db'
        if name == 'history-off':
            result['databaseNotCreated'] = not database.exists()
            assert result['databaseNotCreated'], result
        else:
            with sqlite3.connect('file:' + database.as_posix() + '?mode=ro', uri=True) as db:
                rows = db.execute('SELECT last_position,last_played_date FROM Media').fetchall()
                result.update(mediaRows=len(rows), progress=rows[0][0] if len(rows) == 1 else None,
                    rootCount=db.execute('SELECT COUNT(*) FROM Folder').fetchone()[0],
                    pendingTasks=db.execute('SELECT COUNT(*) FROM Task').fetchone()[0])
            assert len(rows) == 1 and rows[0][1] > 0 and result['rootCount'] == 0 and result['pendingTasks'] == 0, result
            if name == 'save':
                first_progress = result['progress']
                assert .18 < first_progress < .25, result
            elif name == 'resume':
                assert first_progress is not None and result['progress'] > first_progress + .025, result
                assert .25 < result['progress'] < .32 and result['elapsedSeconds'] < 12, result
            elif name == 'resume-before-start':
                assert .34 < result['progress'] < .38 and result['elapsedSeconds'] < 8, result
            elif name == 'resume-after-stop':
                assert .01 < result['progress'] < .04 and result['elapsedSeconds'] < 8, result
            else:
                assert result['progress'] == 0, result
        result['libraryPreferenceRemainsOff'] = 'media-library=0' in config.read_text(encoding='utf-8')
        assert result['libraryPreferenceRemainsOff'], result
        result['passed'] = True
        print(json.dumps(dict(case=name, **result)), flush=True)
    report['passed'] = True
except Exception as error:
    report['error'] = str(error)
finally:
    (output / 'verification.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8', newline='\n')
print(json.dumps(dict(passed=report['passed'], ordinaryPlaybackCases=len(report['cases']))), flush=True)
raise SystemExit(0 if report['passed'] else 1)
