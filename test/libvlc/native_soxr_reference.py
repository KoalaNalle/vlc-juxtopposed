# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 VLC authors and VideoLAN
"""Bounded independent native SDK SoXR PCM/count oracle and unchanged fault assertions."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import subprocess

CASES = ['soxr_fixed_complete_and_repeat', 'soxr_downsample_complete',
         'soxr_flush_then_new_signal', 'soxr_fixed_variable_transitions_independent_reference',
         'soxr_true_bypass_variable_transitions_independent_reference',
         'soxr_injected_allocation_failure_consumes_input',
         'soxr_injected_alias_process_failure_releases_once',
         'soxr_injected_gather_failure_releases_preserved_chain',
         'soxr_injected_drain_allocation_failure_can_retry',
         'soxr_switch_initial_drain_allocation_failure_can_retry',
         'soxr_native_reference_lifecycle_cleanup']


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--package', type=Path, required=True)
    parser.add_argument('--test-exe', type=Path, required=True)
    parser.add_argument('--output-dir', type=Path, required=True)
    args = parser.parse_args()
    package, executable, output = [item.resolve() for item in
        [args.package, args.test_exe, args.output_dir]]
    output.mkdir(parents=True, exist_ok=False)
    report = {'startedUtc': datetime.now(timezone.utc).isoformat(), 'passed': False,
              'package': str(package), 'testExe': str(executable), 'expectedCases': CASES,
              'timeoutSeconds': 45,
              'scope': 'Current production SoXR source compiled into the native diagnostic with real libSoXR/native blocks. Ordinary fixed/downsample/flush/fixed/VR transitions and initial-ratio1 true bypass are compared to independently created SDK engines using exact counts and PCM tolerance1e-5, with repeated NULL-input SDK drains to actual zero; allocation/Gather/process error outcomes are explicitly injected at source API call sites and tracked via native release callbacks. This does not test actual process-wide OOM, the loaded SoXR DLL code path, encoded FLAC output, desktop playback or arbitrary media.'}

    def save():
        (output / 'verification.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')

    try:
        required = [executable, package / 'libvlc.dll', package / 'libvlccore.dll',
                    package / 'plugins/audio_filter/libsoxr_plugin.dll']
        for item in required:
            if not item.is_file():
                raise RuntimeError('Missing selected native executable/runtime: ' + str(item))
        for name in ['libvlc.dll', 'libvlccore.dll']:
            adjacent = executable.parent / name
            if adjacent.exists():
                required.append(adjacent)
                if digest(adjacent) != digest(package / name):
                    raise RuntimeError('Adjacent core and selected package do not match: ' + name)
        report['inputHashes'] = {str(item): digest(item) for item in required}
        root = Path(__file__).resolve().parents[2]
        source_paths = [root / 'test/modules/audio_filter/soxr_lifecycle.c',
                        root / 'test/modules/audio_filter/soxr_native_reference.c',
                        root / 'modules/audio_filter/resampler/soxr.c']
        report['sourceHashesAtRun'] = {str(item.relative_to(root)): digest(item)
            for item in source_paths if item.is_file()}
        report['sourceBuildProof'] = 'Current source identities are recorded; Root must separately retain the successful compilation log/manifest linking this executable to those sources.'
        userdata = output / 'userdata'
        userdata.mkdir()
        environment = os.environ.copy()
        environment.pop('VLC_PLUGIN_PATH', None)
        environment.update(PATH=str(package) + os.pathsep + environment.get('PATH', ''),
            VLC_LIB_PATH=str(package / 'plugins'), VLC_USERDATA_PATH=str(userdata))
        report['environment'] = {name: environment[name] for name in ['VLC_LIB_PATH', 'VLC_USERDATA_PATH']}
        with (output / 'stdout.txt').open('wb') as stdout, (output / 'stderr.txt').open('wb') as stderr:
            process = subprocess.Popen([str(executable)], cwd=output, env=environment,
                stdout=stdout, stderr=stderr, creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
            report['ownedPid'] = process.pid
            save()
            try:
                report['exitCode'] = process.wait(timeout=45)
            except subprocess.TimeoutExpired:
                report['timedOut'] = True
                process.kill()
                report['exitCode'] = process.wait(timeout=10)
        content = (output / 'stdout.txt').read_text(encoding='utf-8-sig', errors='replace')
        actual = [line.removeprefix('PASS ').strip() for line in content.splitlines() if line.startswith('PASS ')]
        report['actualCases'] = actual
        report['passed'] = report['exitCode'] == 0 and actual == CASES and not report.get('timedOut', False)
        report['evidenceHashes'] = {name: digest(output / name) for name in ['stdout.txt', 'stderr.txt']}
    except Exception as error:
        report['error'] = str(error)
    report['finishedUtc'] = datetime.now(timezone.utc).isoformat()
    save()
    print(json.dumps({key: report.get(key) for key in ['passed', 'exitCode', 'timedOut', 'error']}, indent=2))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
