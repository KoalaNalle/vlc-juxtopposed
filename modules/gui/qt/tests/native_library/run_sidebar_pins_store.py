#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Run native sidebar-store checks in a fresh owned Windows portable runtime."""
import argparse
import ctypes
from hashlib import file_digest
import json
import os
from pathlib import Path
import shutil
import sys
import subprocess
import xml.etree.ElementTree as ET


def digest(path):
    with path.open('rb') as source:
        return file_digest(source, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--package', type=Path, required=True)
    parser.add_argument('--program', type=Path, required=True)
    parser.add_argument('--inputs', type=Path, required=True)
    parser.add_argument('--dependency', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[5]
    package, program, output = (p.resolve() for p in (args.package, args.program, args.output))
    assert os.name == 'nt'
    assert output.drive.upper() == 'E:' and output != package and package not in output.parents
    assert program.is_file()
    frozen = json.loads(args.inputs.read_text(encoding='utf-8-sig'))
    def inputs_match():
        return all(digest(root / name) == sha for name, sha in frozen.items())
    assert inputs_match(), 'Inputs changed since compilation'
    names = ('libvlc.dll', 'libvlccore.dll', 'plugins/gui/libqt_plugin.dll')
    before = {name: digest(package / name) for name in names}
    output.mkdir(parents=True, exist_ok=False)
    runtime = output / 'runtime'
    results = output / 'results'
    results.mkdir()
    sys.path.insert(0, str(root / 'test/libvlc'))
    from native_http_runtime import stage_runtime, environment
    stage_runtime(package, runtime)
    destination = runtime / program.name
    assert not destination.exists()
    shutil.copyfile(program, destination)
    if args.dependency:
        target = runtime / args.dependency.name
        if target.exists():
            target.unlink()
        shutil.copyfile(args.dependency, target)
    env = environment(runtime, results)
    env['VLC_SIDEBAR_PINS_TEST_OUTPUT'] = str(results / 'owned')
    command = [str(destination), '-o', str(results / 'qtest.txt') + ',txt',
               '-o', str(results / 'qtest.xml') + ',xunitxml']
    kernel = ctypes.WinDLL('kernel32', use_last_error=True)
    kernel.SetErrorMode.argtypes = [ctypes.c_uint]
    kernel.SetErrorMode.restype = ctypes.c_uint
    previous = kernel.SetErrorMode(0x8003)
    result = dict(forcedCleanupUsed=False)
    try:
        with (results / 'stdout.txt').open('wb') as stdout, (results / 'stderr.txt').open('wb') as stderr:
            child = subprocess.Popen(command, cwd=runtime, env=env, stdout=stdout, stderr=stderr,
                                     creationflags=subprocess.CREATE_NO_WINDOW)
            result['pid'] = child.pid
            try:
                result['exitCode'] = child.wait(timeout=90)
            except subprocess.TimeoutExpired:
                result['forcedCleanupUsed'] = True
                child.kill()
                result['exitCode'] = child.wait(timeout=5)
    finally:
        kernel.SetErrorMode(previous)
    result['normalExit'] = result['exitCode'] == 0 and not result['forcedCleanupUsed']
    xml = results / 'qtest.xml'
    counts = dict(tests=0, failures=0, errors=0, skipped=0)
    if xml.exists():
        report = ET.parse(xml).getroot()
        suites = [report] if report.tag == 'testsuite' else report.findall('testsuite')
        for suite in suites:
            for key in counts:
                counts[key] += int(suite.get(key, '0'))
    result.update(counts=counts, sourceInputsUnchanged=inputs_match(),
                  sourcePackageUnchanged=all(digest(package / name) == sha for name, sha in before.items()),
                  sourceRuntimeSha256=before, programSha256=digest(program),
                  runnerSha256=digest(Path(__file__)))
    result['passed'] = (result['normalExit'] and counts['tests'] > 0
                        and not any(counts[k] for k in ('failures', 'errors', 'skipped'))
                        and result['sourceInputsUnchanged'] and result['sourcePackageUnchanged'])
    (output / 'verification.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(result, indent=2))
    return 0 if result['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
