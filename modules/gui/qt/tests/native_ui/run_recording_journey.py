#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Exercise only the private GUI's own VLC windows, with real native services."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import time

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--package', type=Path, required=True)
parser.add_argument('--qa-gui', type=Path, required=True)
parser.add_argument('--output-dir', type=Path, required=True)
args = parser.parse_args()
ordinary, qa, output = args.package.resolve(), args.qa_gui.resolve(), args.output_dir.resolve()
assert (ordinary / 'portable').is_dir(), 'Use an owned ordinary portable runtime'
assert ordinary.drive.upper() == output.drive.upper() == qa.drive.upper(), 'Immutable hard links require one volume'
assert not output.is_relative_to(ordinary), 'Output must be outside the source runtime'
output.mkdir(parents=True, exist_ok=False)
runtime = output / 'runtime'
runtime.mkdir()
for source in ordinary.rglob('*'):
    relative = source.relative_to(ordinary)
    if relative.parts[0] == 'portable' or source.name == 'plugins.dat':
        continue
    destination = runtime / relative
    if source.is_dir():
        destination.mkdir(exist_ok=True)
    elif relative.as_posix() == 'plugins/gui/libqt_plugin.dll':
        os.link(qa, destination)
    else:
        os.link(source, destination)
(runtime / 'portable').mkdir()

def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()

ordinary_gui = ordinary / 'plugins/gui/libqt_plugin.dll'
ordinary_sha = digest(ordinary_gui)
assert digest(qa) != ordinary_sha, 'The opt-in fixture must be a separate private GUI'
configuration = output / 'vlcrc'
initial_recordings = output / 'Initial Recordings'
initial_recordings.mkdir()
configuration.write_text('metadata-network-access=0\nml-network-discovery=0\none-instance=0\nqt-updates-notif=0\ninput-record-path=' + initial_recordings.as_posix() + '\n', encoding='utf-8', newline='\n')
for name in ('userdata', 'temp', 'appdata', 'localappdata'):
    (output / name).mkdir()
environment = {k: v for k, v in os.environ.items() if not k.startswith('VLC_')}
for key in ('QML_DISABLE_DISK_CACHE', 'QML_FORCE_DISK_CACHE', 'QT_QUICK_BACKEND'):
    environment.pop(key, None)
environment.update(QT_ENABLE_REGEXP_JIT='1',QML_DISK_CACHE='aot', QT_DISABLE_SHADER_DISK_CACHE='1', QT_QPA_PLATFORM='windows',
    VLC_USERDATA_PATH=str(output / 'userdata'), TEMP=str(output / 'temp'), TMP=str(output / 'temp'),
    APPDATA=str(output / 'appdata'), LOCALAPPDATA=str(output / 'localappdata'),
    HOME=str(output / 'userdata'), USERPROFILE=str(output / 'userdata'))
report = dict(passed=False, ordinaryGuiUnchanged=False, privateGuiOnly=True, cases={},
    runtimeSha256={'libvlccore.dll': digest(runtime / 'libvlccore.dll'), 'plugins/gui/libqt_plugin.dll': digest(qa)},
    qaSourceSha256=digest(Path(__file__).with_name('recording_journey.cpp')), ordinaryGuiSha256=ordinary_sha,
    fullBetaAccepted=False, windowsDpiMatrixAccepted=False)

def save():
    (output / 'verification.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8', newline='\n')

child = None
try:
    cache = subprocess.run([str(runtime / 'vlc-cache-gen.exe'), str(runtime / 'plugins')], cwd=runtime,
        env=environment, capture_output=True, timeout=60, creationflags=subprocess.CREATE_NO_WINDOW)
    (output / 'cache-stderr.txt').write_bytes(cache.stderr)
    cache.check_returncode()
    for mode, expected_checks in [('edit', 87), ('reopen', 15)]:
        case = output / mode
        case.mkdir()
        command = [str(runtime / 'vlc.exe'), '--intf=qt', '--no-one-instance', '--no-media-library',
            '--no-crashdump', '--no-qt-privacy-ask', '--no-qt-updates-notif', '--no-metadata-network-access',
            '--http-proxy=direct://', '--no-ml-network-discovery', '--config=' + str(configuration),
            '--file-logging', '--log-verbose=2', '--verbose=2', '--logfile=' + str(case / 'vlc.log')]
        child_environment = environment | dict(VLC_OWNED_UI_REPORT=str(case / 'journey.json'),
            VLC_OWNED_UI_OUTPUT=str(case), VLC_OWNED_UI_BASE=str(output), VLC_OWNED_UI_CONFIG=str(configuration), VLC_OWNED_UI_MODE=mode)
        result = dict(normalExit=False, forcedCleanupUsed=False)
        report['cases'][mode] = result
        with (case / 'stdout.txt').open('wb') as stdout, (case / 'stderr.txt').open('wb') as stderr:
            started = time.monotonic()
            child = subprocess.Popen(command, cwd=runtime, env=child_environment, stdout=stdout, stderr=stderr,
                creationflags=subprocess.CREATE_NO_WINDOW)
            result['pid'] = child.pid
            save()
            try:
                result['exitCode'] = child.wait(timeout=90)
            except subprocess.TimeoutExpired:
                result['forcedCleanupUsed'] = True
                child.kill()
                result['exitCode'] = child.wait(timeout=5)
            result['elapsedSeconds'] = time.monotonic() - started
        result['normalExit'] = result['exitCode'] == 0 and not result['forcedCleanupUsed']
        journey = case / 'journey.json'
        result['journey'] = json.loads(journey.read_text()) if journey.exists() else None
        save()
        print(mode, json.dumps(result), flush=True)
        assert result['normalExit'] and result['journey'] and result['journey']['passed'], mode
        checks = result['journey']['checks']
        assert len(checks)==expected_checks and all(c['passed'] for c in checks), mode
    report['passed'] = True
finally:
    if child is not None and child.poll() is None:
        report['forcedCleanupUsed'] = True
        child.kill()
        child.wait(timeout=5)
    report['ordinaryGuiUnchanged'] = digest(ordinary_gui) == ordinary_sha
    save()
