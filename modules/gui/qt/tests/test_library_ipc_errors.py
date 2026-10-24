#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Compile fault tests against extracted production Library opening helpers.

The C fixtures replace only dependencies, so deterministic allocation/queue/
reply failures can exercise the actual helper bodies. This does not replace
native Windows two-process acceptance.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess


def between(source, first, last):
    start = source.index(first)
    end = source.index(last, start + len(first))
    return source[start:end]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', default='cc')
    parser.add_argument('--output-dir', type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[4]
    source_dir = Path(__file__).resolve().parent
    out = args.output_dir.resolve()
    out.mkdir(parents=True, exist_ok=False)
    sender = (root / 'src/win32/specific.c').read_text(encoding='utf-8')
    receiver = (root / 'modules/control/win_msg.c').read_text(encoding='utf-8')
    protocol = between(receiver, '/* Explicit Library IPC v1.', 'static void add_to_playlist')
    assert protocol == between(sender, '/* Explicit Library IPC v1.', '/* Query without sending files first:')
    typedef = between(receiver, 'typedef struct\n{\n    int argc;', '/* Explicit Library IPC v1.')
    generated = {
        'receiver': typedef + protocol + between(receiver, 'static LRESULT LibraryOpenFiles', 'static LRESULT CALLBACK'),
        'sender': protocol + between(sender, 'static int LibrarySendExplicit', 'static bool LibrarySingleInstance'),
    }
    results = {}
    for name, body in generated.items():
        (out / f'{name}-under-test.inc').write_text(body, encoding='utf-8', newline='\n')
        fixture = source_dir / f'library_ipc_{name}_errors.c'
        binary = out / name
        command = [args.cc, '-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                   '-I', str(out), str(fixture), '-o', str(binary)]
        subprocess.run(command, check=True, capture_output=True, text=True, timeout=30)
        result = subprocess.run([str(binary)], check=True, capture_output=True, text=True, timeout=10)
        results[name] = {'stdout': result.stdout.strip(), 'exitCode': result.returncode,
                         'extractedSha256': hashlib.sha256(body.encode()).hexdigest(),
                         'fixtureSha256': hashlib.sha256(fixture.read_bytes()).hexdigest()}
    report = {'scope': 'Production helper fault branches with dependency stubs; no native IPC claim',
              'passed': True, 'results': results}
    (out / 'verification.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps(report))


if __name__ == '__main__':
    main()
