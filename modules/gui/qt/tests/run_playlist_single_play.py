# SPDX-License-Identifier: GPL-2.0-or-later
"""Bound a windowless exact-body Qt fixture for the empty/single Play repair."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--repo-root', type=Path, required=True)
parser.add_argument('--overlay', type=Path, required=True)
parser.add_argument('--mode', choices=['baseline', 'fixed'], required=True)
parser.add_argument('--output-dir', type=Path, required=True)
args = parser.parse_args()
output = args.output_dir.resolve()
output.mkdir(parents=True, exist_ok=False)
fixture = Path(__file__).with_name('test_playlist_single_play.py')
worker_dir = output / 'worker'
command = [sys.executable, str(fixture), '--repo-root', str(args.repo_root.resolve()),
    '--overlay', str(args.overlay.resolve()), '--mode', args.mode, '--output-dir', str(worker_dir)]
started = time.monotonic()
try:
    process = subprocess.run(command, cwd=output, env=os.environ.copy(), capture_output=True,
        timeout=30, creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
    exit_code, stdout, stderr, timed_out = process.returncode, process.stdout, process.stderr, False
except subprocess.TimeoutExpired as error:
    exit_code, stdout, stderr, timed_out = None, error.stdout or b'', error.stderr or b'', True
    (output / 'timeout.txt').write_text('Owned QCore/QQml single-Play fixture exceeded 30 seconds.\n',
        encoding='utf-8', newline='\n')
(output / 'stdout.txt').write_bytes(stdout)
(output / 'stderr.txt').write_bytes(stderr)


def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()


names = ['knownZeroAccessiblePlayKeepsLibraryView', 'knownZeroCoverPlayKeepsLibraryView',
    'knownZeroLeftDoubleTapBodyOpensDetails', 'rightDoubleTapBodyDoesNotOpenDetails',
    'filledPlaylistAccessibleDispatchesOnce', 'filledPlaylistCoverDispatchesOnce',
    'unknownCountDispatchesOnce', 'nullCountDispatchesOnce', 'nonNumericZeroDispatchesOnce',
    'nonCollectionVideoRetainsDefaultPlay', 'oneSelectionKeyboardBodyOpensEmptyDetails',
    'multipleSelectionKeyboardBodyRetainsModelPlay', 'noSelectionKeyboardBodyDoesNothing']
expected_failure_names = names[:2] + names[4:9]
worker_report = worker_dir / 'verification.json'
worker = json.loads(worker_report.read_text(encoding='utf-8')) if worker_report.is_file() else None
shape = bool(worker and [case['name'] for case in worker['cases']] == names
    and worker['mode'] == args.mode and not worker['messages'] and not worker['allMessages']
    and not stderr and not timed_out
    and worker['fixtureSha256'] == sha(fixture))
baseline_shape = bool(shape and args.mode == 'baseline' and exit_code == 1 and not worker['passed']
    and worker['failureNames'] == expected_failure_names and worker['baselineShapeMatches']
    and all(case.get('observedMediaCalls') == [42, 42]
        and case.get('observedPlayerView') is True and case.get('observedViewAssignments') == [True]
        and case.get('observedHistoryCalls') == [] and case.get('messages') == []
        for case in worker['cases'] if case['name'] in expected_failure_names)
    and all(case['passed'] for case in worker['cases'] if case['name'] not in expected_failure_names))
fixed_shape = bool(shape and args.mode == 'fixed' and exit_code == 0 and worker['passed']
    and not worker['failureNames'] and all(case['passed'] for case in worker['cases']))
verification = dict(mode=args.mode, passed=fixed_shape, baselineShapeMatches=baseline_shape,
    expectedOutcomeObserved=baseline_shape or fixed_shape, exitCode=exit_code, timedOut=timed_out,
    ownedProcessTimeoutSeconds=30, elapsedSeconds=time.monotonic() - started,
    exactOrderedCases=names, expectedOriginalBaselineFailureNames=expected_failure_names,
    fixtureSha256=sha(fixture), launcherSha256=sha(Path(__file__)),
    workerVerificationSha256=sha(worker_report) if worker_report.is_file() else None,
    evidenceSha256={name: sha(output / name) for name in ['stdout.txt', 'stderr.txt']},
    scope='Bounded windowless QCore/QQml exact routing bodies and supplied dependencies. One requested ML dispatch is required for nonempty/unknown playlists; no native input/ML/queue/player, rendering, lifetime or Figma acceptance.')
(output / 'verification.json').write_text(json.dumps(verification, indent=2) + '\n', encoding='utf-8', newline='\n')
print(json.dumps(verification))
raise SystemExit(0 if fixed_shape else 1)
