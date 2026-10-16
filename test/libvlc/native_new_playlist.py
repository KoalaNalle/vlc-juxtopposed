"""Run actual native playlist creation in fresh isolated library storage."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--package', type=Path, required=True)
parser.add_argument('--test-exe', type=Path, required=True)
parser.add_argument('--fixtures', type=Path, required=True)
parser.add_argument('--output-dir', type=Path, required=True)
args = parser.parse_args()
package, executable, fixtures, output = (p.resolve() for p in
    [args.package, args.test_exe, args.fixtures, args.output_dir])
fixture_names = ['alpha-one.flac', 'alpha-two.flac', 'beta-one.flac',
                 'beta-two.flac', 'home-video.mp4']
required = [package / 'libvlc.dll', package / 'libvlccore.dll',
            package / 'vlc-preparser.exe', executable]
required += [fixtures / name for name in fixture_names]
for path in required:
    if not path.is_file():
        raise SystemExit('Missing required native fixture or runtime: ' + str(path))
for name in ['libvlc.dll', 'libvlccore.dll']:
    adjacent = executable.parent / name
    if adjacent.exists() and digest(adjacent) != digest(package / name):
        raise SystemExit('The test executable has a different adjacent runtime: ' + str(adjacent))
output.mkdir(parents=True, exist_ok=False)
userdata = output / 'userdata'
userdata.mkdir()
environment = os.environ.copy()
environment.update(PATH=str(package) + os.pathsep + environment.get('PATH', ''),
    VLC_PLUGIN_PATH=str(package / 'plugins'), VLC_USERDATA_PATH=str(userdata),
    VLC_LIBEXEC_PATH=str(package / 'vlc-preparser.exe'),
    VLC_PLAYLIST_FIXTURE_DIR=str(fixtures), QT_QPA_PLATFORM='offscreen')
results = output / 'results.txt'
try:
    process = subprocess.run([str(executable), '-o', str(results) + ',txt'],
        cwd=output, env=environment, capture_output=True, timeout=100,
        creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
except subprocess.TimeoutExpired as error:
    (output / 'stdout.txt').write_bytes(error.stdout or b'')
    (output / 'stderr.txt').write_bytes(error.stderr or b'')
    (output / 'timeout.txt').write_text('Owned playlist diagnostic exceeded 100 seconds.\n',
        encoding='utf-8', newline='\n')
    raise SystemExit('Native playlist diagnostic timed out') from error
(output / 'stdout.txt').write_bytes(process.stdout)
(output / 'stderr.txt').write_bytes(process.stderr)
text = results.read_text(encoding='utf-8-sig') if results.is_file() else ''
cases = ['initTestCase', 'validationAndSelectionOrder', 'rollbackAndPartialFailure',
    'actualMixedSaveWithoutPlayback', 'actualExternalMediaUnknownDurationWithoutDiscovery',
    'actualThumbnailWriteFailureRollsBack',
    'actualEmptyPlaylist', 'persistenceAfterRestart',
    'submittedSaveCompletesBeforeModelDestruction', 'cleanupTestCase']
passed = (process.returncode == 0 and '0 failed, 0 skipped' in text
    and all('PASS   : TestNewPlaylist::' + case + '()' in text for case in cases))
verification = dict(exitCode=process.returncode, passed=passed, cases=cases,
    evidence='Actual native library, ordered repeated mixed playlist, copied PNG, restart and owner shutdown; no desktop GUI claim.',
    injectedFailureCases=['rollbackAndPartialFailure'], isolatedUserdata=True,
    fixtureSha256={name: digest(fixtures / name) for name in fixture_names},
    testExeSha256=digest(executable), coreSha256=digest(package / 'libvlccore.dll'))
(output / 'verification.json').write_text(json.dumps(verification, ensure_ascii=False, indent=2) + '\n',
    encoding='utf-8', newline='\n')
print(text)
if not passed:
    raise SystemExit('Native playlist results did not prove every required case')
