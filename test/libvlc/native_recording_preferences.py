#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Run the owned Windows Recording preference model checks."""
import argparse, hashlib, json, os, re, shutil
from pathlib import Path
from native_http_runtime import stage_runtime, regenerate_cache, run_owned


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['package', 'core', 'cache-generator', 'test-exe', 'output-dir']:
        parser.add_argument('--' + name, required=True, type=Path)
    args = parser.parse_args()
    package, core, executable, output = (p.resolve() for p in
        [args.package, args.core, args.test_exe, args.output_dir])
    output.mkdir(parents=True, exist_ok=False)
    source_before = {name: digest(package / name) for name in ['libvlccore.dll', 'plugins/plugins.dat']}
    runtime = output / 'runtime'
    stage_runtime(package, runtime)
    (runtime / 'libvlccore.dll').unlink()  # Only the new runtime's immutable link.
    shutil.copy2(core, runtime / 'libvlccore.dll')
    (runtime / 'vlc-cache-gen.exe').unlink()
    shutil.copy2(args.cache_generator, runtime / 'vlc-cache-gen.exe')
    native = runtime / 'test_qt_recording_preferences.exe'
    shutil.copy2(executable, native)
    regenerate_cache(runtime, output / 'cache')
    results = output / 'results.txt'
    process = run_owned([native, '-o', str(results) + ',txt'], runtime, output / 'test', timeout=120)
    text = results.read_text(encoding='utf-8-sig') if results.exists() else ''
    expected = ['initTestCase', 'openingAndReloadNeverWriteDefaults', 'disabledConsumersRejectWithoutSaving', 'encoderDefaultsAndNoneChoiceRemainUsable', 'strictTypesAndChoicesPreserveMemoryAndDisk', 'unicodeRecordingFolderSurvivesRestartAndEmptyRestoresDefault', 'nativeTrackAndEncoderPreferencesPersist', 'customSavedValuesRemainExplicitAndUnchanged', 'failedNativeSaveRestoresValuesThenRetries', 'discoveryPolicyPersistsAndOnlyAcceptedOffRequestsStop', 'rejectedInterfaceSavePreservesDiscoveryAndDoesNotStop', 'commandLineOverridesAndApplyTimingStayVisible', 'headlessPickerAndAdvancedActionsDoNotWrite', 'cleanupTestCase']
    actual = re.findall(r'^PASS\s+:\s+TestRecordingPreferences::([A-Za-z]\w*)\(\)\s*$', text, re.MULTILINE)
    totals = re.findall(r'^Totals: 14 passed, 0 failed, 0 skipped, 0 blacklisted, \d+ms\s*$', text, re.MULTILINE)
    rejected = re.findall(r'^(?:FAIL!?|SKIP|XFAIL|XPASS|BFAIL|BPASS)\s*:', text, re.MULTILINE)
    cleanup_warnings = [line for path in [results, output / 'test/stdout.txt', output / 'test/stderr.txt']
                        if path.exists() for line in path.read_text(encoding='utf-8-sig', errors='replace').splitlines()
                        if 'QTemporaryDir: Unable to remove' in line]
    unchanged = source_before == {name: digest(package / name) for name in source_before}
    report = dict(passed=bool(process['normalExit'] and actual == expected and len(totals) == 1
                             and not rejected and not cleanup_warnings and unchanged),
                  process=process, actualOrderedPassCases=actual, expectedCases=expected,
                  exactTotals=totals, rejectedStatuses=rejected, sourcePackageUnchanged=unchanged,
                  cleanupWarnings=cleanup_warnings,
                  coreSha256=digest(core), testExeSha256=digest(native),
                  resultsSha256=digest(results) if results.exists() else None,
                  evidence='Actual native model, Unicode recording folder, typed native track/encoder defaults, platform-default cast quality, checked config/INI file-lock rejection and retry, discovery policy persistence/signals, command-line precedence and headless picker/Advanced action; downstream recording, renderer, stream jobs and graphical page remain separate.',
                  recordingPageAccepted=False, fullBetaAccepted=False)
    (output / 'verification.json').write_text(json.dumps(report, indent=2) + '\n',
        encoding='utf-8', newline='\n')
    print(text)
    print('Recording model:', 'PASS' if report['passed'] else 'FAIL')
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
