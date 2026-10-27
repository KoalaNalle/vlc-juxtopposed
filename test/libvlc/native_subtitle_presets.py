"""Run the actual Windows Qt/native subtitle preset diagnostic in isolated state."""
import argparse
from collections import Counter
import hashlib
import json
import os
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--package', type=Path, required=True)
parser.add_argument('--test-exe', type=Path, required=True)
parser.add_argument('--media', type=Path, required=True)
parser.add_argument('--output-dir', type=Path, required=True)
args = parser.parse_args()
package, executable, media, output = (p.resolve() for p in
    [args.package, args.test_exe, args.media, args.output_dir])
for path in [package / 'libvlc.dll', package / 'libvlccore.dll', executable, media,
             media.with_name('subtitle-style.srt'), media.with_name('duration-cues.srt')]:
    if not path.is_file(): raise SystemExit('Missing required native fixture or runtime: ' + str(path))
output.mkdir(parents=True, exist_ok=False)
userdata = output / 'userdata'
userdata.mkdir()
environment = os.environ.copy()
environment.update(PATH=str(package) + os.pathsep + environment.get('PATH', ''),
    VLC_PLUGIN_PATH=str(package / 'plugins'), VLC_USERDATA_PATH=str(userdata),
    VLC_LIBEXEC_PATH=str(package / 'vlc-preparser.exe'),
    VLC_SUBTITLE_FIXTURE=str(media), VLC_SUBTITLE_OUTPUT=str(output), QT_QPA_PLATFORM='offscreen')
results = output / 'results.txt'
command = [str(executable), '-o', str(results) + ',txt']
try:
    process = subprocess.run(command, cwd=output, env=environment, capture_output=True,
        timeout=55, creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
except subprocess.TimeoutExpired as error:
    (output / 'stdout.txt').write_bytes(error.stdout or b'')
    (output / 'stderr.txt').write_bytes(error.stderr or b'')
    (output / 'timeout.txt').write_text('Owned subtitle preset test exceeded 55 seconds.\n', encoding='utf-8', newline='\n')
    raise SystemExit('Native subtitle preset test timed out') from error
(output / 'stdout.txt').write_bytes(process.stdout)
(output / 'stderr.txt').write_bytes(process.stderr)
text = results.read_text(encoding='utf-8-sig') if results.is_file() else ''
cases = ['initTestCase', 'saveApplyDefaultAndActualPixels', 'batchedSnapshotNotifications',
    'submittedWritesAreNotReplayedDuringConfirmation', 'freshPlayerAndSettingsRestore',
    'actualTrackSelectionIsRestored', 'dualTrackOutputRolesAndLiveDurationCommands',
    'unavailableCorruptStorageAndWriteFailure',
    'staleOutputAndResetDoNotResurrectPresets', 'cleanupTestCase']
passed = process.returncode == 0 and all('PASS   : TestSubtitlePresets::' + case + '()' in text for case in cases)
passed = passed and '0 failed, 0 skipped' in text
overflow_count = 0
control_counts = Counter()
native_log = output / 'subtitle-presets-native.log'
if native_log.is_file():
    with native_log.open(encoding='utf-8', errors='replace') as stream:
        for line in stream:
            overflow_count += 'input control fifo overflow' in line.lower()
            if 'control type=' in line:
                control_counts[line.split('control type=', 1)[1].strip()] += 1
passed = passed and overflow_count == 0
verification = dict(exitCode=process.returncode, passed=passed, cases=cases,
    evidence='Actual SubtitlePresetModel, SubtitleAppearance, native player, Freetype pixels and isolated QSettings; no desktop GUI claim.',
    isolatedUserdata=True, inputControlFifoOverflows=overflow_count, inputControlCounts=dict(control_counts),
    fixture=str(media), fixtureSha256=hashlib.sha256(media.read_bytes()).hexdigest(),
    subtitleSha256=hashlib.sha256(media.with_name('subtitle-style.srt').read_bytes()).hexdigest(),
    testExeSha256=hashlib.sha256(executable.read_bytes()).hexdigest(),
    coreSha256=hashlib.sha256((package / 'libvlccore.dll').read_bytes()).hexdigest())
(output / 'verification.json').write_text(json.dumps(verification, ensure_ascii=False, indent=2) + '\n',
    encoding='utf-8', newline='\n')
print(text)
if not passed: raise SystemExit('Native subtitle preset results did not prove every required case')
