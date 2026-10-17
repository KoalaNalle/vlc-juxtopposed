#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Quiet owned-child Windows startup scheduling and exact native readback."""
import argparse, hashlib, json, os, shutil
from pathlib import Path
from native_http_runtime import stage_runtime, regenerate_cache, run_owned


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['package', 'probe-exe', 'output-dir']:
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--core', type=Path)
    parser.add_argument('--expected-missing-saved-high', action='store_true')
    args = parser.parse_args()
    package, out = args.package.resolve(), args.output_dir.resolve()
    out.mkdir(parents=True, exist_ok=False)
    runtime = out / 'runtime'
    stage_runtime(package, runtime)
    if args.core:
        (runtime / 'libvlccore.dll').unlink()
        shutil.copy2(args.core, runtime / 'libvlccore.dll')
    probe = runtime / 'test_priority_startup.exe'
    shutil.copy2(args.probe_exe, probe)
    regenerate_cache(runtime, out / 'cache')
    report = dict(passed=False, expectedMissingSavedHigh=args.expected_missing_saved_high, cases={})
    cases = [
        ('default', {}, [], None, 'inherit'),
        ('saved-high', {'high-priority':1}, [], None, 'above'),
        ('cli-high', {}, ['--high-priority'], None, 'above'),
        ('cli-no-high', {'high-priority':1}, ['--no-high-priority'], None, 'inherit'),
        ('inherited-below', {}, [], '0x4000', 'inherit'),
        ('inherited-below-cli-no-high', {'high-priority':1}, ['--no-high-priority'], '0x4000', 'inherit'),
        ('cli-lookalike', {}, ['--http-user-agent', '--high-priority'], None, 'inherit'),
    ]
    if not args.expected_missing_saved_high:
        cases += [
            ('saved-idle', {'win32-process-priority':-2}, [], None, 'idle'),
            ('saved-below', {'win32-process-priority':-1}, [], None, 'below'),
            ('saved-normal', {'win32-process-priority':0}, [], '0x4000', 'normal'),
            ('saved-above', {'win32-process-priority':1}, [], None, 'above'),
            ('saved-modern-high', {'win32-process-priority':2}, [], None, 'high'),
            ('cli-over-saved', {'win32-process-priority':2}, ['--win32-process-priority=0'], None, 'normal'),
            ('legacy-cli-over-saved-modern', {'win32-process-priority':2}, ['--high-priority'], None, 'above'),
            ('legacy-no-high-preserves-host', {'win32-process-priority':2,'win32-realtime-priority':1},
                ['--no-high-priority'], '0x4000', 'inherit'),
            ('modern-cli-over-legacy', {'high-priority':1}, ['--high-priority','--win32-process-priority=-1'], None, 'below'),
            ('modern-no-rt-over-saved', {'win32-realtime-priority':1,'win32-process-priority':2},
                ['--no-win32-realtime-priority','--win32-process-priority=0'], None, 'normal'),
            ('invalid-cli', {}, ['--win32-process-priority=99'], '0x4000', 'rejected'),
            ('overflow-cli', {}, ['--win32-process-priority=4294967296'], '0x4000', 'rejected'),
            ('realtime-cli', {}, ['--win32-realtime-priority'], None, 'realtime'),
            ('realtime-saved', {'win32-realtime-priority':1}, [], None, 'realtime'),
        ]
    classes = dict(idle=0x40, below=0x4000, normal=0x20, above=0x8000, high=0x80)
    for name, saved, extra, initial, expectation in cases:
        case = out / name
        case.mkdir()
        config = case / 'vlcrc'
        config.write_text(''.join(f'{key}={value}\n' for key,value in saved.items()),
                          encoding='utf-8', newline='\n')
        if initial:
            os.environ['JUXTO_PRIORITY_INITIAL'] = initial
        else:
            os.environ.pop('JUXTO_PRIORITY_INITIAL', None)
        command = [probe, '--no-ignore-config', '--config=' + str(config), '--quiet',
                   '--no-media-library', '--no-metadata-network-access', '--extraintf=', '--control=',
                   '--no-one-instance', *extra]
        result = run_owned(command, runtime, case)
        lines = (case/'stdout.txt').read_text().strip().splitlines()
        observed = json.loads(lines[-1]) if lines else {}
        result.update(observed)
        result['expectation'] = expectation
        report['cases'][name] = result
        healthy = result['normalExit'] and observed.get('initialized') and observed.get('restored')
        if healthy:
            error = observed['startupError']
            if expectation == 'realtime':
                result['realTimeGranted'] = observed['after'] == 0x100 and error == 0
                matches = result['realTimeGranted'] or (observed['after'] == observed['before'] and error > 0)
            elif expectation == 'rejected':
                matches = observed['after'] == observed['before'] and error > 0
            else:
                wanted = observed['before'] if expectation == 'inherit' else classes[expectation]
                matches = observed['after'] == wanted and (args.expected_missing_saved_high or error == 0)
        else:
            matches = False
        result['passed'] = bool(healthy and matches)
        (out/'verification.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8',newline='\n')
        print(name, observed, 'pass=' + str(result['passed']), flush=True)
    if args.expected_missing_saved_high:
        negatives = {'saved-high','cli-lookalike'}
        report['passed'] = all(not v['passed'] if k in negatives else v['passed']
                               for k,v in report['cases'].items())
    else:
        report['passed'] = all(case['passed'] for case in report['cases'].values())
    report['coreSha256'] = hashlib.sha256((runtime/'libvlccore.dll').read_bytes()).hexdigest()
    report['fullPerformanceOrBetaAccepted'] = False
    (out/'verification.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8',newline='\n')
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
