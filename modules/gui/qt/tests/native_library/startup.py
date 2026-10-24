#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Exercise saved Library startup policy through the real Windows vlc.exe.

Stages an ordinary package and optional rebuilt launcher into an owned portable
runtime. Checks native module loading and real database creation, not source
spelling or a libvlc-only approximation of WinMain.
"""
import argparse
from hashlib import file_digest
import json
import os
from pathlib import Path
import shutil
import subprocess
import time


def digest(path):
    with path.open('rb') as stream:
        return file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--package', type=Path, required=True)
    parser.add_argument('--launcher', type=Path)
    parser.add_argument('--output-dir', type=Path, required=True)
    args = parser.parse_args()
    if os.name != 'nt':
        parser.error('This acceptance driver requires Windows.')
    package, out = args.package.resolve(), args.output_dir.resolve()
    if out == package or package in out.parents:
        parser.error('Use a fresh output outside the supplied package.')
    out.mkdir(parents=True, exist_ok=False)
    runtime = out / 'runtime'
    runtime.mkdir()
    (runtime / 'portable').mkdir()
    shutil.copytree(package / 'plugins', runtime / 'plugins',
                    ignore=shutil.ignore_patterns('plugins.dat'))
    for name in ('libvlc.dll', 'libvlccore.dll', 'vlc.exe', 'vlc-preparser.exe', 'vlc-cache-gen.exe'):
        shutil.copyfile(package / name, runtime / name)
    if args.launcher:
        shutil.copyfile(args.launcher.resolve(), runtime / 'vlc.exe')
    env = os.environ.copy()
    for key in list(env):
        if key.startswith('VLC_'):
            env.pop(key)
    env['VLC_USERDATA_PATH'] = str(out / 'cache-generator-data')
    cache = subprocess.run([str(runtime / 'vlc-cache-gen.exe'), str(runtime / 'plugins')],
                           cwd=runtime, env=env, capture_output=True, timeout=60,
                           creationflags=subprocess.CREATE_NO_WINDOW)
    (out / 'cache-stderr.txt').write_bytes(cache.stderr)
    cache.check_returncode()
    cases = [
        ('saved-off', 'media-library=0\n', [], False),
        ('saved-default-off', '#media-library=0\n', [], False),
        ('saved-on', 'media-library=1\n', [], True),
        ('explicit-on', 'media-library=0\n', ['--media-library'], True),
        ('explicit-off', 'media-library=1\n', ['--no-media-library'], False),
    ]
    results = []
    identities = {name: digest(runtime / name) for name in ('vlc.exe', 'libvlc.dll', 'libvlccore.dll')}
    for name, saved, extra, expected in cases:
        work = out / name
        work.mkdir()
        config, log, data = work / 'vlcrc', work / 'vlc.log', work / 'data'
        config.write_text('[core]\n' + saved, encoding='utf-8')
        config_before = digest(config)
        case_env = env.copy()
        case_env['VLC_USERDATA_PATH'] = str(data)
        command = [str(runtime / 'vlc.exe'), '--no-crashdump', '--no-one-instance',
                   '--intf=dummy', '--no-auto-preparse', '--no-metadata-network-access',
                   '--no-ml-network-discovery', '--file-logging', '--verbose=2',
                   '--logfile=' + str(log), '--config=' + str(config), *extra, 'vlc://quit']
        (work / 'launch.json').write_text(json.dumps({'command': command, 'runtimeSha256': identities}, indent=2) + '\n')
        started = time.monotonic()
        with (work / 'stdout.txt').open('wb') as stdout, (work / 'stderr.txt').open('wb') as stderr:
            process = subprocess.Popen(command, cwd=runtime, env=case_env, stdout=stdout, stderr=stderr,
                                       creationflags=subprocess.CREATE_NO_WINDOW)
            (work / 'pid.txt').write_text(str(process.pid))
            timed_out = False
            try:
                code = process.wait(timeout=30)
            except subprocess.TimeoutExpired:
                timed_out, code = True, process.poll()
        text = log.read_text(encoding='utf-8', errors='replace') if log.exists() else ''
        loaded = 'using medialibrary module "medialibrary"' in text
        attempted = 'looking for medialibrary module' in text
        database = data / 'ml' / 'ml.db'
        result = dict(name=name, expectedLibrary=expected, moduleLoaded=loaded,
                      moduleAttempted=attempted, databaseCreated=database.exists(),
                      configUnchanged=digest(config) == config_before, exitCode=code,
                      timedOut=timed_out, terminationConfirmed=process.poll() is not None,
                      forcedCleanupUsed=False, elapsedSeconds=time.monotonic() - started)
        result['passed'] = (code == 0 and not timed_out and result['terminationConfirmed']
                            and result['configUnchanged'] and loaded == expected
                            and attempted == expected and database.exists() == expected)
        results.append(result)
        (work / 'verification.json').write_text(json.dumps(result, indent=2) + '\n')
        print(json.dumps(result), flush=True)
        if timed_out:
            break  # Preserve the live PID; never start a second run over it.
    report = dict(passed=len(results) == len(cases) and all(row['passed'] for row in results),
                  cases=results, runtimeSha256=identities, ordinaryPackageModified=False,
                  scope='Real Windows launcher with owned config/userdata and no GUI automation')
    (out / 'verification.json').write_text(json.dumps(report, indent=2) + '\n')
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
