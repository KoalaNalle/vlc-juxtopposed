"""Run the real Qt/native independent FLAC conversion diagnostic on Windows."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import re

def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--package', type=Path, required=True)
parser.add_argument('--test-exe', type=Path, required=True)
parser.add_argument('--output-dir', type=Path, required=True)
args = parser.parse_args()
package, executable, output = (p.resolve() for p in
    [args.package, args.test_exe, args.output_dir])
for path in [package / 'libvlc.dll', package / 'libvlccore.dll', executable,
             package / 'vlc-preparser.exe']:
    if not path.is_file():
        raise SystemExit('Missing required native runtime: ' + str(path))
for name in ['libvlc.dll', 'libvlccore.dll']:
    adjacent = executable.parent / name
    if adjacent.exists() and digest(adjacent) != digest(package / name):
        raise SystemExit('Adjacent native runtime differs from the selected package: ' + str(adjacent))
output.mkdir(parents=True, exist_ok=False)
(executable.parent / 'portable').mkdir(exist_ok=True)
userdata = output / 'userdata'
userdata.mkdir()
environment = os.environ.copy()
for name in ['temp', 'appdata', 'localappdata']:
    (output / name).mkdir()
environment.update(TEMP=str(output / 'temp'), TMP=str(output / 'temp'),
    APPDATA=str(output / 'appdata'), LOCALAPPDATA=str(output / 'localappdata'))
# Windows treats VLC_LIB_PATH as the complete plugin inventory; supplementary
# VLC_PLUGIN_PATH could retain an unrelated inherited/default build inventory.
environment.pop('VLC_PLUGIN_PATH', None)
environment.update(PATH=str(package) + os.pathsep + environment.get('PATH', ''),
    VLC_LIB_PATH=str(package / 'plugins'), VLC_LIBEXEC_PATH=str(package / 'vlc-preparser.exe'),
    VLC_USERDATA_PATH=str(userdata), VLC_CONVERSION_EVIDENCE_DIR=str(output), QT_QPA_PLATFORM='offscreen')
results = output / 'results.txt'
command = [str(executable), '-o', str(results) + ',txt']
try:
    process = subprocess.run(command, cwd=output, env=environment, capture_output=True,
        timeout=80, creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
except subprocess.TimeoutExpired as error:
    (output / 'stdout.txt').write_bytes(error.stdout or b'')
    (output / 'stderr.txt').write_bytes(error.stderr or b'')
    (output / 'timeout.txt').write_text('Owned conversion test exceeded 80 seconds.\n', encoding='utf-8', newline='\n')
    failure = dict(exitCode=None, passed=False, timedOut=True, isolatedUserdata=True,
        processTimeoutSeconds=80, testExeSha256=digest(executable),
        nativeCoreHashesMatch=True,
        runtimeSha256={name: digest(package / name) for name in ['libvlc.dll', 'libvlccore.dll', 'vlc-preparser.exe']})
    (output / 'verification.json').write_text(json.dumps(failure, indent=2) + '\n', encoding='utf-8', newline='\n')
    raise SystemExit('Native conversion test timed out') from error
(output / 'stdout.txt').write_bytes(process.stdout)
(output / 'stderr.txt').write_bytes(process.stderr)
text = results.read_text(encoding='utf-8-sig') if results.is_file() else ''
cases = ['initTestCase', 'localValidationAndNoOverwrite', 'realFlacAndActualPlayback',
    'inheritedRunTimeAndInputChoicesDoNotTrim', 'inheritedFileFormatKeepsLiteralPercentDestination',
    'encoderPreferenceIsCapturedBeforeStarting',
    'corruptSourceDoesNotPublish',
    'destinationRaceIsPreserved', 'runningCancellationAndReuse',
    'independentPrimaryQueueAndPausedClock', 'runningWorkerShutdown', 'cleanupTestCase']
actual_cases = re.findall(r'^PASS\s+:\s+TestConversionController::([A-Za-z]\w*)\(\)\s*$', text, re.MULTILINE)
totals = re.findall(r'^Totals: 12 passed, 0 failed, 0 skipped, 0 blacklisted, \d+ms\s*$', text, re.MULTILINE)
rejected_statuses = re.findall(r'^(?:FAIL!?|SKIP|XFAIL|XPASS|BFAIL|BPASS)\s*:', text, re.MULTILINE)
passed = bool(process.returncode == 0 and actual_cases == cases and len(totals) == 1 and not rejected_statuses)
# Log names identify the actual selected stages. The exact complete plugin
# root above plus each selected module's package DLL/hash records what was
# exercised; it does not assert arbitrary unrelated network/media support.
log_path = output / 'conversion-captured-native.log'
log_text = log_path.read_text(encoding='utf-8-sig', errors='replace') if log_path.is_file() else ''
selected = sorted(set(re.findall(r'using (audio encoder|audio decoder|audio resampler|audio converter|sout filter|sout mux|sout access) module "([^"]+)"', log_text)))
module_aliases = {
    ('audio converter', 'trivial'): 'audio_filter/libtrivial_channel_mixer_plugin.dll',
    ('sout filter', 'transcode'): 'stream_out/libstream_out_transcode_plugin.dll',
    ('sout mux', 'dummy'): 'mux/libmux_dummy_plugin.dll',
    ('sout access', 'file'): 'access_output/libaccess_output_file_plugin.dll',
}
module_audit = []
for capability, name in selected:
    relative_module = module_aliases.get((capability, name))
    candidates = [package / 'plugins' / relative_module] if relative_module else list((package / 'plugins').rglob('lib' + name + '_plugin.dll'))
    candidates = [candidate for candidate in candidates if candidate.is_file()]
    module_audit.append({'capability': capability, 'module': name,
        'resolvedPackageDll': str(candidates[0].relative_to(package)) if len(candidates) == 1 else None,
        'sha256': digest(candidates[0]) if len(candidates) == 1 else None})
required_stages = {'audio encoder', 'audio decoder', 'sout filter', 'sout mux', 'sout access'}
startup_errors = re.findall(r'unknown option|unrecognized option|could not load module|cannot load module', log_text, re.IGNORECASE)
loaded_module_audit = {'passed': bool(module_audit and required_stages <= {capability for capability,name in selected}
    and all(module['sha256'] for module in module_audit) and not startup_errors),
    'completePluginRoot': str(package / 'plugins'), 'selectedModules': module_audit,
    'startupErrors': startup_errors,
    'evidence': 'Actual selected module names from native log resolved to the exclusive package inventory and recorded DLL identities; no live process memory-map claim.'}
passed = passed and loaded_module_audit['passed']
source, converted = output / 'source.wav', output / 'converted.flac'
passed = passed and source.is_file() and converted.is_file()
if converted.is_file():
    with converted.open('rb') as file:
        passed = passed and file.read(4) == b'fLaC'
verification = dict(exitCode=process.returncode, passed=bool(passed), cases=cases,
    actualOrderedPassCases=actual_cases, exactTotals=totals, rejectedStatuses=rejected_statuses,
    evidenceSha256={name: digest(output / name) if (output / name).is_file() else None
        for name in ['results.txt', 'stdout.txt', 'stderr.txt']},
    evidence='Actual ConversionController, native FLAC encoder/mux, complete validation decode, ordinary LibVLC audio playback, independent primary playlist and paused clock. No desktop or original-design fidelity claim.',
    isolatedUserdata=True,
    timedOut=False, processTimeoutSeconds=80, nativeCoreHashesMatch=True,
    diagnosticEnvironment={name: environment[name] for name in ['VLC_LIB_PATH', 'VLC_LIBEXEC_PATH', 'VLC_USERDATA_PATH']},
    loadedModuleAudit=loaded_module_audit, nativeLogSha256=digest(log_path) if log_path.is_file() else None,
    sourceSha256=digest(source) if source.is_file() else None,
    convertedSha256=digest(converted) if converted.is_file() else None,
    testExeSha256=digest(executable), coreSha256=digest(package / 'libvlccore.dll'),
    runtimeSha256={name: digest(package / name) for name in ['libvlc.dll', 'libvlccore.dll', 'vlc-preparser.exe']})
(output / 'verification.json').write_text(json.dumps(verification, ensure_ascii=False, indent=2) + '\n',
    encoding='utf-8', newline='\n')
print(text)
if not passed:
    raise SystemExit('Native conversion results did not prove every required case')
