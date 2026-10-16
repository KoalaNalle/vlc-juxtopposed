# SPDX-License-Identifier: GPL-2.0-or-later
"""Run exact-source Qt/native modal key routing, with an exclusive VLC runtime."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--package', type=Path, required=True)
parser.add_argument('--test-exe', type=Path, required=True)
parser.add_argument('--production-include', type=Path, required=True)
parser.add_argument('--output-dir', type=Path, required=True)
args = parser.parse_args()
package, executable, include, output = [path.resolve() for path in
    [args.package, args.test_exe, args.production_include, args.output_dir]]
def digest(path):
    with path.open('rb') as stream: return hashlib.file_digest(stream, 'sha256').hexdigest()
for path in [executable, include, include.with_suffix(include.suffix + '.json'), package / 'libvlc.dll', package / 'libvlccore.dll']:
    if not path.is_file(): raise SystemExit('Missing native test artifact: ' + str(path))
for name in ['libvlc.dll', 'libvlccore.dll']:
    adjacent = executable.parent / name
    if adjacent.exists() and digest(adjacent) != digest(package / name):
        raise SystemExit('Adjacent core differs from exact package: ' + str(adjacent))
source = json.loads(include.with_suffix(include.suffix + '.json').read_text())
if source['includeSha256'] != digest(include): raise SystemExit('Production include changed after its manifest was written')
output.mkdir(parents=True, exist_ok=False)
userdata = output / 'userdata'
userdata.mkdir()
environment = os.environ.copy()
environment.pop('VLC_PLUGIN_PATH', None)
environment.update(PATH=str(package) + os.pathsep + environment.get('PATH', ''),
    VLC_LIB_PATH=str(package / 'plugins'), VLC_USERDATA_PATH=str(userdata))
# The fixture uses QCoreApplication and detached key/metadata receivers. It
# creates no window, controls or QPA integration; full GUI checks are separate.
results = output / 'results.txt'
verification = {'passed': False, 'timedOut': False, 'testExeSha256': digest(executable),
    'runtimeSha256': {name: digest(package / name) for name in ['libvlc.dll', 'libvlccore.dll']},
    'productionExtraction': source, 'processTimeoutSeconds': 30,
    'scope': 'QCore-only exact production key-method/branch bodies with dependency adapters; detached Qt key/metadata receiver press-release delivery and native LibVLC key-pressed callbacks. Window/controls/focus/scene, full MainCtx/provider lifetime and actual production modal/transport acceptance remain separate.'}
def evidence_hashes():
    return {Path(name).stem + 'Sha256': digest(output / name) if (output / name).is_file() else None
        for name in ['results.txt', 'stdout.txt', 'stderr.txt']}
try:
    process = subprocess.run([str(executable), '-o', str(results) + ',txt'], cwd=output, env=environment,
        capture_output=True, timeout=30, creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
except subprocess.TimeoutExpired as error:
    (output / 'stdout.txt').write_bytes(error.stdout or b'')
    (output / 'stderr.txt').write_bytes(error.stderr or b'')
    verification['timedOut'] = True
    verification['evidenceHashes'] = evidence_hashes()
    (output / 'verification.json').write_text(json.dumps(verification, indent=2) + '\n', encoding='utf-8', newline='\n')
    raise SystemExit('Owned native key-routing test exceeded 30 seconds') from error
(output / 'stdout.txt').write_bytes(process.stdout)
(output / 'stderr.txt').write_bytes(process.stderr)
text = results.read_text(encoding='utf-8-sig') if results.is_file() else ''
identity_line = 'ROUTING_PRODUCTION_SHA256 ' + json.dumps(source['sourceSha256'], sort_keys=True)
identity_matches = identity_line in text + process.stdout.decode('utf-8', errors='replace') + process.stderr.decode('utf-8', errors='replace')
cases = ['initTestCase', 'absentProviderPreservesDirectDispatch', 'mouseFocusOutsideModalDispatchesNativeKeys',
    'modalKeepsPressAndReleaseInQtAndBlocksDirectDispatch', 'outsideModalQtNavigationPolicyRemains', 'cleanupTestCase']
actual_cases = re.findall(r'^PASS\s+:\s+TestSharedModalHotkeys::([A-Za-z]\w*)\(\)\s*$', text, re.MULTILINE)
totals = re.findall(r'^Totals: 6 passed, 0 failed, 0 skipped, 0 blacklisted, \d+ms\s*$', text, re.MULTILINE)
rejected_statuses = re.findall(r'^(?:FAIL!?|SKIP|XFAIL|XPASS|BFAIL|BPASS)\s*:', text, re.MULTILINE)
verification.update(exitCode=process.returncode, cases=cases, compiledSourceIdentityMatches=identity_matches,
    actualOrderedPassCases=actual_cases, exactTotals=totals, rejectedStatuses=rejected_statuses,
    evidenceHashes=evidence_hashes(),
    passed=bool(process.returncode == 0 and actual_cases == cases and len(totals) == 1 and not rejected_statuses
        and identity_matches
        ))
(output / 'verification.json').write_text(json.dumps(verification, indent=2) + '\n', encoding='utf-8', newline='\n')
print(text)
if not verification['passed']: raise SystemExit('Native key-routing assertions did not all pass')
