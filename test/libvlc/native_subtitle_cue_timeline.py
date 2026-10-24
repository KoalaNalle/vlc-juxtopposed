# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 VLC authors and VideoLAN
"""Run native source-cue, normal-lock and precise-seek checks with a finite timeout."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
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
package, executable, fixtures, output = (path.resolve() for path in
    [args.package, args.test_exe, args.fixtures, args.output_dir])
fixture_names = ['subtitle-style.mp4', 'duration-cues.srt', 'paused-seek-colors.mp4']
runtime_paths = [package / 'libvlc.dll', package / 'libvlccore.dll', package / 'vlc-preparser.exe',
    package / 'plugins/demux/libmp4_plugin.dll', package / 'plugins/demux/libsubtitle_plugin.dll',
    package / 'plugins/codec/libsubsdec_plugin.dll', package / 'plugins/codec/libavcodec_plugin.dll',
    package / 'plugins/video_output/libvdummy_plugin.dll']
required = [*runtime_paths, executable]
required += [fixtures / name for name in fixture_names]
for path in required:
    if not path.is_file():
        raise SystemExit('Missing native fixture or runtime: ' + str(path))
if not (package / 'plugins').is_dir():
    raise SystemExit('Missing package plugin directory')
for name in ['libvlc.dll', 'libvlccore.dll']:
    adjacent = executable.parent / name
    if adjacent.exists() and digest(adjacent) != digest(package / name):
        raise SystemExit('The test executable has a different adjacent runtime: ' + str(adjacent))
output.mkdir(parents=True, exist_ok=False)
userdata = output / 'userdata'
userdata.mkdir()
environment = os.environ.copy()
# Windows prioritizes VLC_LIB_PATH. Avoid supplementing its inventory with an
# inherited developer or installed-VLC plugin directory.
environment.pop('VLC_PLUGIN_PATH', None)
environment.update(PATH=str(package) + os.pathsep + environment.get('PATH', ''),
    VLC_LIB_PATH=str(package / 'plugins'), VLC_USERDATA_PATH=str(userdata),
    VLC_LIBEXEC_PATH=str(package / 'vlc-preparser.exe'),
    VLC_TIMELINE_FIXTURE_DIR=str(fixtures), QT_QPA_PLATFORM='offscreen')
results = output / 'results.txt'
try:
    process = subprocess.run([str(executable), '-o', str(results) + ',txt'],
        cwd=output, env=environment, capture_output=True, timeout=100,
        creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
except subprocess.TimeoutExpired as error:
    (output / 'stdout.txt').write_bytes(error.stdout or b'')
    (output / 'stderr.txt').write_bytes(error.stderr or b'')
    (output / 'verification.json').write_text(json.dumps({
        'passed': False, 'timedOut': True, 'timeoutSeconds': 100,
        'evidence': 'The owned native diagnostic did not finish; no native pass is claimed.'
    }, indent=2) + '\n', encoding='utf-8', newline='\n')
    raise SystemExit('Native subtitle-cue diagnostic exceeded 100 seconds') from error
(output / 'stdout.txt').write_bytes(process.stdout)
(output / 'stderr.txt').write_bytes(process.stderr)
text = results.read_text(encoding='utf-8-sig') if results.is_file() else ''
cases = ['initTestCase', 'parserUnitsOrderingAndOverlap',
    'invalidEmptyUnsupportedAndCanceledReads', 'actualSelectedExternalSource',
    'nativeDelayInheritanceOverrideAndFpsGuard', 'actualCueSeekAndStaleRequestGuards',
    'actualNormalLockAndExactMicrosecondSeekRequest',
    'actualGuardedTimingShiftAndRetainedPlayback',
    'durationEffectsDoNotPretendToBeSourceExpiry', 'ambiguousSourcesFailClosed',
    'staleReadOnDisableAndMediaReplacement', 'cleanupTestCase']
actual_passes = re.findall(r'^PASS\s*:\s*TestSubtitleCueTimeline::([^\(]+)\(\)', text, re.MULTILINE)
totals_match = re.search(r'Totals:\s*(\d+) passed, (\d+) failed, (\d+) skipped', text)
totals = dict(zip(['passed', 'failed', 'skipped'], map(int, totals_match.groups()))) if totals_match else None
log_path = output / 'subtitle-cue-native.log'
log = log_path.read_text(encoding='utf-8-sig', errors='replace') if log_path.is_file() else ''
required_modules = ['using demux module "mp4"', 'using demux module "subtitle"',
    'using spu decoder module "subsdec"', 'using video decoder module "avcodec"',
    'using vout display module "vdummy"']
module_audit = {name: name in log for name in required_modules}
startup_errors = re.findall(r'[^\r\n]*(?:unknown option|unrecognized option|no suitable (?:demux|spu decoder|video decoder|vout display) module)[^\r\n]*', log, re.IGNORECASE)
passed = (process.returncode == 0 and actual_passes == cases
    and totals == {'passed': len(cases), 'failed': 0, 'skipped': 0}
    and not re.search(r'^(?:FAIL!|SKIP)\s*:', text, re.MULTILINE)
    and all(module_audit.values()) and not startup_errors)
verification = dict(exitCode=process.returncode, passed=passed, cases=cases,
    evidence='Actual native external-SRT source identity, delay, precise request and completed paused seeking with NORMAL/REENTRANT players; separate from desktop GUI and decoder-rendered expiry.',
    exactNormalSeekRequestTicks=4352555, nativeCompletionToleranceTicks=50000,
    isolatedUserdata=True, timeoutSeconds=100,
    actualPasses=actual_passes, totals=totals,
    package=str(package), environment={name: environment[name] for name in
        ['VLC_LIB_PATH', 'VLC_LIBEXEC_PATH', 'VLC_USERDATA_PATH', 'VLC_TIMELINE_FIXTURE_DIR']},
    nativeLogSha256=digest(log_path) if log_path.is_file() else None,
    nativeModules=module_audit, startupErrors=startup_errors,
    preparserExecution='Not requested by this no-ML suite; selected package helper existence/hash is recorded.',
    fixtureSha256={name: digest(fixtures / name) for name in fixture_names},
    testExeSha256=digest(executable),
    runtimeSha256={str(path.relative_to(package)): digest(path) for path in runtime_paths})
(output / 'verification.json').write_text(json.dumps(verification, ensure_ascii=False, indent=2) + '\n',
    encoding='utf-8', newline='\n')
print(text)
if not passed:
    raise SystemExit('Native subtitle-cue results did not prove every required case')
