"""Affected new-Core inline WebVTT/TTML decoder timing against explicit references."""
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
from PySide6.QtGui import QImage

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'build-windows/beta-preset-frame-20261029/formats-r1'
BASE = ROOT / 'build-windows/beta-resume-20261029/gui-r27/runtime'
BUILD = ROOT / 'build-windows/beta-preset-frame-20261029/test-build-r2'
SMOKE = Path(__file__).with_name('run-inline-webvtt-smoke-r1.py')
spec = importlib.util.spec_from_file_location('frozen_format_masks', SMOKE)
analysis = importlib.util.module_from_spec(spec)
spec.loader.exec_module(analysis)
sys.path.insert(0, str(ROOT / 'test/libvlc'))
import native_http_runtime as support


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def score(a, b):
    return {key: len(a[key] & b[key]) / len(a[key] | b[key]) if a[key] | b[key] else 1.
            for key in a}


def main():
    assert not OUT.exists()
    base = json.loads((BASE.parent / 'verification.json').read_text())
    build = json.loads((BUILD / 'verification.json').read_text())
    assert base['status'] == 'terminal' and base['exitCode'] == 0 and base['preservationPassed']
    assert build['passed'] and sha(ROOT / 'test/src/player/subtitle_fps.c') == build['sourceSha256']
    assert sha(BUILD / 'subtitle-fps.exe') == build['binarySha256']
    assert all(sha(BASE / name) == value for name, value in base['runtimeSha256'].items())
    OUT.mkdir(parents=True)
    runtime = OUT / 'runtime'
    support.stage_runtime(BASE, runtime)
    os.link(BUILD / 'subtitle-fps.exe', runtime / 'subtitle-fps.exe')
    support.regenerate_cache(runtime, OUT / 'cache')
    hashes = dict(base['runtimeSha256'])
    hashes['subtitle-fps.exe'] = build['binarySha256']
    inputs = {str(path): sha(path) for path in (Path(__file__), SMOKE, Path(support.__file__),
              BASE.parent / 'verification.json', BUILD / 'verification.json', BUILD / 'subtitle-fps.exe',
              ROOT / 'test/src/player/subtitle_fps.c', ROOT / 'src/player/input.c',
              ROOT / 'src/player/player.h', ROOT / 'src/player/timer.c')}
    groups = [('webvtt', ROOT / 'build-windows/beta-inline-webvtt-20261029/media-r1', 'base.vtt', 'reference-50.vtt'),
              ('ttml', ROOT / 'build-windows/beta-ttml-fps-20261029/media-r3', 'ticks100.ttml', 'reference-ticks100-50.ttml')]
    for _, media, source, reference in groups:
        for name in ('frame-marker-black-bottom.mp4', source, reference):
            inputs[str(media / name)] = sha(media / name)
    result = {'status': 'running', 'scope': 'Coreccb6 real decoder held/inherited/playing/positive-delay timing, explicit references; not GUI/all formats/package',
              'inputs': inputs, 'runtimeSha256': hashes, 'cases': [], 'comparisons': [], 'forcedCleanupUsed': False}

    def save():
        (OUT / 'verification.json').write_text(json.dumps(result, indent=2) + '\n')

    save()
    for family, media, source, reference in groups:
        images = {}
        observations = {}
        definitions = [('normal', source, 0, 'reference', 8, 0),
                       ('reference50', reference, 0, 'reference', 8, 0),
                       ('held50', source, 50, 'hold', 8, 0),
                       ('inherited50', source, 50, 'inherited', 8, 0),
                       ('held50-delay2', source, 50, 'hold', 8, 2000000),
                       ('playing50', source, 50, 'playing', 5.5, 0)]
        for name, subtitle, fps, mode, target, delay in definitions:
            run = OUT / (family + '-' + name)
            run.mkdir()
            env = support.environment(runtime, run)
            env['VLC_TEST_PLUGIN_PATH'] = str(runtime / 'plugins')
            command = [str(runtime / 'subtitle-fps.exe'), str(media / 'frame-marker-black-bottom.mp4'),
                       str(media / subtitle), str(run), str(fps), mode, str(target), str(delay)]
            row = {'family': family, 'name': name, 'command': command, 'status': 'starting'}
            result['cases'].append(row)
            with (run / 'stdout.txt').open('wb') as stdout, (run / 'stderr.txt').open('wb') as stderr:
                child = subprocess.Popen(command, cwd=runtime, env=env, stdout=stdout, stderr=stderr,
                                         creationflags=subprocess.CREATE_NO_WINDOW)
                row.update(status='running', pid=child.pid)
                save()
                print('OWNED_FORMAT_TIMING ' + str(child.pid) + ' ' + family + '-' + name, flush=True)
                try:
                    code = child.wait(timeout=90)
                except subprocess.TimeoutExpired:
                    row['status'] = 'observation-timeout-process-left-running'
                    save()
                    raise SystemExit('Inspect same child, do not restart')
            events = [json.loads(line) for line in (run / 'stdout.txt').read_text().splitlines() if line.startswith('{')]
            row.update(status='terminal', exitCode=code, observations=events)
            save()
            assert code == 0 and events[-1].get('executionComplete')
            observations[name] = [event for event in events if 'step' in event]
            images[name] = {path.stem: analysis.masks(path) for path in run.glob('*.png')}
            row['markers'] = {}
            for path in run.glob('*.png'):
                image = QImage(str(path))
                assert not image.isNull()
                row['markers'][path.stem] = sum(1 << bit for bit in range(11) if image.pixelColor(32 + 64 * bit, 20).red() > 128)
            lines = (run / 'native.log').read_text(errors='replace').splitlines()
            errors = [line for line in lines if 'error:' in line]
            row['otherErrors'] = [line for line in errors if line.rsplit('error:', 1)[1].strip() != 'window closed']
            row['teardownWindowCloseDiagnostics'] = len(errors) - len(row['otherErrors'])
            assert not row['otherErrors']
            save()
        contrast = sum(len(images['normal']['initial'][key] ^ images['reference50']['initial'][key])
                       for key in ('green', 'magenta', 'white'))
        assert contrast > 100
        for name in ('held50', 'inherited50', 'held50-delay2'):
            expected = {'initial': 'reference50' if name == 'inherited50' else 'normal', 'reset': 'normal'}
            if name != 'inherited50':
                expected['changed'] = 'reference50'
            scores = {step: score(images[name][step], images[reference]['initial']) for step, reference in expected.items()}
            row = next(row for row in result['cases'] if row['family'] == family and row['name'] == name)
            data = observations[name]
            held = len(set(row['markers'].values())) == 1 and max(item['timeUs'] for item in data) - min(item['timeUs'] for item in data) < 50000
            native = all(item['state'] == 3 and item['delayUs'] == (2000000 if name.endswith('delay2') else 0) for item in data)
            result['comparisons'].append({'family': family, 'name': name, 'similarity': scores, 'heldFrameAndClock': held,
                                          'nativeStateAndDelay': native, 'passed': held and native and min(value for group in scores.values() for value in group.values()) > .97})
        playing = [item for item in observations['playing50'] if item['step'].startswith('playing-')]
        scores = {item['step']: score(images['playing50'][item['step']], images['reference50' if item['step'] == 'playing-changed' else 'normal']['initial']) for item in playing}
        clocks = [item['timeUs'] for item in playing]
        passed = len(clocks) == 3 and clocks[0] < clocks[1] < clocks[2] and all(5000000 < clock < 9900000 for clock in clocks)
        passed &= all(item['state'] == 2 for item in playing) and min(value for group in scores.values() for value in group.values()) > .97
        result['comparisons'].append({'family': family, 'name': 'playing50', 'similarity': scores, 'clocks': clocks, 'passed': passed})
        save()
    result.update(status='terminal', inputsUnchanged=all(sha(name) == value for name, value in inputs.items()),
                  runtimeUnchanged=all(sha(runtime / name) == value for name, value in hashes.items()),
                  fallbackUnchanged=all(sha(BASE / name) == value for name, value in base['runtimeSha256'].items()))
    result['passed'] = all(row['passed'] for row in result['comparisons']) and result['inputsUnchanged'] and result['runtimeUnchanged'] and result['fallbackUnchanged']
    save()
    print(json.dumps({'passed': result['passed'], 'processes': len(result['cases']), 'comparisons': result['comparisons']}, indent=2), flush=True)
    assert result['passed']


if __name__ == '__main__':
    main()
