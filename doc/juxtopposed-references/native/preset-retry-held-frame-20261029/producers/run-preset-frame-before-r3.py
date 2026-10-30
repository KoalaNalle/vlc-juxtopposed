"""Compare actual startup-held pictures under single and batched timing resets."""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
from PySide6.QtGui import QImage

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'build-windows/beta-preset-frame-20261029/before-r3'
BUILD = ROOT / 'build-windows/beta-preset-frame-20261029/test-build-r2'
BASE = ROOT / 'build-windows/beta-resume-20261029/gui-r25/runtime'
SOURCE = ROOT / 'test/src/player/subtitle_fps.c'
MEDIA = BASE.parent / 'media/frame-marker.mp4'
sys.path.insert(0, str(ROOT / 'test/libvlc'))
import native_http_runtime as support


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    assert not OUT.exists()
    build = json.loads((BUILD / 'verification.json').read_text())
    base = json.loads((BASE.parent / 'verification.json').read_text())
    assert build['passed'] and build['inputsUnchanged']
    assert base['status'] == 'terminal' and base['exitCode'] == 0 and base['preservationPassed']
    assert sha(SOURCE) == build['sourceSha256']
    assert sha(BUILD / 'subtitle-fps.exe') == build['binarySha256']
    assert all(sha(BASE / name) == value for name, value in base['runtimeSha256'].items())
    OUT.mkdir(parents=True)
    runtime = OUT / 'runtime'
    support.stage_runtime(BASE, runtime)
    os.link(BUILD / 'subtitle-fps.exe', runtime / 'subtitle-fps.exe')
    expected = dict(base['runtimeSha256'])
    expected['subtitle-fps.exe'] = build['binarySha256']
    support.regenerate_cache(runtime, OUT / 'cache')
    inputs = {str(path): sha(path) for path in
              (Path(__file__), Path(support.__file__), SOURCE, BUILD / 'verification.json',
               BUILD / 'subtitle-fps.exe', BASE.parent / 'verification.json', MEDIA)}
    result = {'status': 'running', 'scope': 'Core real decoder/snapshot comparison; separate from ordinary Qt model',
              'inputs': inputs, 'runtimeSha256': expected, 'cases': [], 'forcedCleanupUsed': False}

    def save():
        (OUT / 'verification.json').write_text(json.dumps(result, indent=2) + '\n')

    save()
    cases = [('single-fraction', 'startup-single', '20.5'),
             ('batch-fraction', 'startup-batch', '20.5'),
             ('batch-aligned', 'startup-batch', '20.48'),
             ('batch-other-fraction', 'startup-batch', '20.51')]
    for name, mode, target in cases:
        run = OUT / name
        run.mkdir()
        command = [str(runtime / 'subtitle-fps.exe'), str(MEDIA), '-', str(run), '0', mode, target, '1200000']
        row = {'name': name, 'mode': mode, 'target': target, 'command': command}
        result['cases'].append(row)
        env = support.environment(runtime, run)
        env['VLC_TEST_PLUGIN_PATH'] = str(runtime / 'plugins')
        for key in ('VLC_PLUGIN_PATH', 'QT_PLUGIN_PATH', 'QML_IMPORT_PATH', 'QML2_IMPORT_PATH'):
            env.pop(key, None)
        with (run / 'stdout.txt').open('wb') as stdout, (run / 'stderr.txt').open('wb') as stderr:
            child = subprocess.Popen(command, cwd=runtime, env=env, stdout=stdout, stderr=stderr,
                                     creationflags=subprocess.CREATE_NO_WINDOW)
            row.update(status='running', pid=child.pid)
            save()
            print('OWNED_PRESET_FRAME ' + str(child.pid) + ' ' + name, flush=True)
            try:
                code = child.wait(timeout=90)
            except subprocess.TimeoutExpired:
                row['status'] = 'observation-timeout-process-left-running'
                save()
                raise SystemExit('Inspect same child, do not restart')
        events = [json.loads(line) for line in (run / 'stdout.txt').read_text().splitlines() if line.startswith('{')]
        row.update(status='terminal', exitCode=code, events=events, snapshots=[])
        save()
        assert code == 0 and events[-1].get('executionComplete')
        for step in ('initial', 'delayed', 'reset', 'user-seek', 'after-seek-redraw'):
            path = run / (step + '.png')
            image = QImage(str(path))
            assert not image.isNull() and image.width() == 960 and image.height() == 540
            marker = sum(1 << bit for bit in range(11) if image.pixelColor(32 + 64 * bit, 20).red() > 128)
            row['snapshots'].append({'step': step, 'marker': marker, 'sha256': sha(path)})
        row['heldFramePreserved'] = len({item['marker'] for item in row['snapshots'][:3]}) == 1
        row['userSeekSupersedesRedraw'] = row['snapshots'][3]['marker'] != row['snapshots'][0]['marker'] and row['snapshots'][3]['marker'] == row['snapshots'][4]['marker']
        log = '\n'.join((run / part).read_text(encoding='utf-8', errors='replace')
                        for part in ('native.log', 'stderr.txt'))
        row['diagnostics'] = [line for line in log.splitlines() if re.search(
            r'\berror:|Assertion failed|Binding loop|ReferenceError|TypeError', line)]
        save()
        print(json.dumps({'name': name, 'exitCode': code, 'heldFramePreserved': row['heldFramePreserved'],
                          'markers': [item['marker'] for item in row['snapshots']]}), flush=True)
    result.update(status='terminal', inputsUnchanged=all(sha(path) == value for path, value in inputs.items()),
                  runtimeUnchanged=all(sha(runtime / name) == value for name, value in expected.items()),
                  fallbackUnchanged=all(sha(BASE / name) == value for name, value in base['runtimeSha256'].items()))
    result['allFramesPreserved'] = all(row['heldFramePreserved'] and row['userSeekSupersedesRedraw'] for row in result['cases'])
    save()
    assert result['inputsUnchanged'] and result['runtimeUnchanged'] and result['fallbackUnchanged']
    print(json.dumps({key: result[key] for key in ('status', 'allFramesPreserved', 'inputsUnchanged', 'runtimeUnchanged', 'fallbackUnchanged')}), flush=True)


if __name__ == '__main__':
    main()
