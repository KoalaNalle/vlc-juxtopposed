#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Run the real-module MMDevice stop regression in an owned Windows runtime."""
from pathlib import Path
import argparse
import hashlib
import json
import shutil
import sys

from native_http_runtime import stage_runtime, regenerate_cache, run_owned


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--package', type=Path, required=True)
    parser.add_argument('--probe', type=Path, required=True,
                        help='The actual .libs/test_mmdevice_stop.exe, not the libtool launcher')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    assert sys.platform == 'win32', 'Native Windows test required'
    root = Path(__file__).resolve().parents[2]
    owned = root / 'build-windows'
    package, probe, output = (p.resolve() for p in
                              (args.package, args.probe, args.output))
    assert all(p.is_relative_to(owned) for p in (package, probe, output))
    assert probe.stat().st_size > 100000, 'Select the real executable under .libs'
    assert not output.exists(), 'Use a fresh output directory'
    output.mkdir(parents=True)
    files = [root / 'modules/audio_output/mmdevice.c',
             root / 'modules/audio_output/mmdevice.h',
             root / 'test/modules/audio_output/mmdevice_stop.c', probe,
             package / 'libvlccore.dll']
    before = {str(p): digest(p) for p in files}
    runtime = output / 'runtime'
    stage_runtime(package, runtime)
    executable = runtime / 'test_mmdevice_stop.exe'
    shutil.copy2(probe, executable)
    regenerate_cache(runtime, output / 'cache')
    run = output / 'run'
    result = run_owned([executable], runtime, run, timeout=30)
    rows = [json.loads(line) for line in (run / 'stdout.txt').read_text(
        encoding='utf-8').splitlines() if line.startswith('{')]
    result.update(checks=rows, sourceAndPackageUnchanged=all(
        digest(Path(p)) == value for p, value in before.items()),
        inputSha256=before)
    result['passed'] = (result['normalExit'] and len(rows) == 32 and
        all(row['passed'] for row in rows) and
        {row['mode'] for row in rows} == {'timer', 'play', 'buffer-wait', 'idle'} and
        result['sourceAndPackageUnchanged'])
    (output / 'result.json').write_text(json.dumps(result, indent=2) + '\n',
                                      encoding='utf-8')
    print(json.dumps({k: v for k, v in result.items()
                      if k not in ('checks', 'inputSha256')}), flush=True)
    assert result['passed'], 'See result.json and run/stderr.txt'


if __name__ == '__main__':
    main()
