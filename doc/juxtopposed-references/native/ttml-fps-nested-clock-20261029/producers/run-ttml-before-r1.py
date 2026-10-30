"""Actual TTML decoder baseline and intended rate/nesting negative controls."""
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
WORK = ROOT / 'build-windows/beta-ttml-fps-20261029'
OUT = WORK / 'before-r1'
MEDIA = WORK / 'media-r1'
BASE = ROOT / 'build-windows/beta-resume-20261029/gui-r16/runtime'
BUILD = ROOT / 'build-windows/beta-inline-webvtt-20261029/test-build-r1'
SMOKE = Path(__file__).with_name('run-inline-webvtt-smoke-r1.py')
spec = importlib.util.spec_from_file_location('inline_pixel_masks', SMOKE)
analysis = importlib.util.module_from_spec(spec)
spec.loader.exec_module(analysis)
sys.path.insert(0, str(ROOT / 'test/libvlc'))
import native_http_runtime as support


def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def main():
    assert not OUT.exists()
    build = json.loads((BUILD / 'verification.json').read_text())
    assert build['passed'] and sha(BUILD / 'subtitle-fps.exe') == build['binarySha256']
    assert sha(ROOT / 'test/src/player/subtitle_fps.c') == build['sourceSha256']
    assert sha(BASE / 'libvlccore.dll') == '4a2865915e342744a5a7092171b92279b08f7b229eede231e00c3e86d88cff84'
    OUT.mkdir()
    runtime = OUT / 'runtime'
    support.stage_runtime(BASE, runtime)
    support.regenerate_cache(runtime, OUT / 'cache')
    hashes = {str(p.relative_to(runtime)):sha(p) for p in runtime.rglob('*') if p.is_file() and p.name != 'plugins.dat' and 'portable' not in p.relative_to(runtime).parts}
    inputs = {str(p):sha(p) for p in [Path(__file__), SMOKE, Path(support.__file__), ROOT / 'test/src/player/subtitle_fps.c',
              ROOT / 'modules/demux/ttml.c', ROOT / 'modules/codec/ttml/genttml.c', BUILD / 'subtitle-fps.exe',
              *sorted(MEDIA.glob('*.ttml')), MEDIA / 'frame-marker-black-bottom.mp4']}
    record = dict(status='running',scope='Native baseline/control qualification; expected regressions are evidence, not candidate acceptance',
                  inputs=inputs,runtimeSha256=hashes,cases=[],forcedCleanupUsed=False)
    save = lambda:(OUT / 'verification.json').write_text(json.dumps(record, indent=2) + '\n')
    save()
    images = {}
    for name, subtitle, fps, mode, target in [
        ('flat0-8','flat.ttml',0,'reference',8), ('reference50-8','reference-50.ttml',0,'reference',8),
        ('hold50-8','flat.ttml',50,'hold',8), ('flat0-12','flat.ttml',0,'reference',12),
        ('nested-par0-12','nested-par.ttml',0,'reference',12),
        ('nested-seq0-12','nested-seq.ttml',0,'reference',12)]:
        run = OUT / name
        run.mkdir()
        env = support.environment(runtime, run)
        env['VLC_TEST_PLUGIN_PATH'] = str(runtime / 'plugins')
        command = [str(BUILD / 'subtitle-fps.exe'),str(MEDIA / 'frame-marker-black-bottom.mp4'),str(MEDIA / subtitle),str(run),str(fps),mode,str(target),'0']
        row = dict(name=name,command=command,status='starting')
        record['cases'].append(row)
        save()
        with (run / 'stdout.txt').open('wb') as stdout, (run / 'stderr.txt').open('wb') as stderr:
            child = subprocess.Popen(command,cwd=runtime,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
            row.update(status='running',pid=child.pid)
            save()
            print('OWNED_TTML_RUNNING ' + str(child.pid) + ' ' + name,flush=True)
            try:
                code = child.wait(timeout=60)
            except subprocess.TimeoutExpired:
                row.update(status='observation-timeout-process-left-running')
                save()
                raise SystemExit('Inspect same owned PID before further work')
        data = [json.loads(l) for l in (run / 'stdout.txt').read_text().splitlines() if l.startswith('{')]
        row.update(status='terminal',exitCode=code,observations=data)
        save()
        assert code == 0 and data[-1].get('executionComplete'),name
        images[name] = {p.stem:analysis.masks(p) for p in run.glob('*.png')}
        row['pixels'] = {step:{k:len(v) for k,v in masks.items()} for step,masks in images[name].items()}
        errors = [l for l in (run / 'native.log').read_text(errors='replace').splitlines() if 'error:' in l]
        row['otherErrors'] = [l for l in errors if l.rsplit('error:',1)[1].strip() != 'window closed']
        row['teardownWindowCloseDiagnostics'] = len(errors)-len(row['otherErrors'])
        save()
    def score(a,b):
        return {k:len(a[k]&b[k])/len(a[k]|b[k]) if a[k]|b[k] else 1. for k in a}
    flat = images['flat0-8']['initial']
    ref = images['reference50-8']['initial']
    changed = images['hold50-8']['changed']
    record['qualifiedFlatReferences'] = len(flat['green']) > 100 and len(ref['magenta']) > 100 and len(flat['magenta']) == 0 and len(ref['green']) == 0
    record['rateControl'] = dict(similarity=score(changed,ref),unchanged=score(changed,flat))
    record['nestedControls'] = {k:score(images[k]['initial'],images['flat0-12']['initial']) for k in ['nested-par0-12','nested-seq0-12']}
    record.update(status='terminal',inputsUnchanged=all(sha(p)==h for p,h in inputs.items()),
                  runtimeUnchanged=all(sha(runtime / p)==h for p,h in hashes.items()))
    record['baselineQualified'] = record['qualifiedFlatReferences'] and record['inputsUnchanged'] and record['runtimeUnchanged'] and all(not r['otherErrors'] for r in record['cases'])
    save()
    print(json.dumps({k:record[k] for k in ['baselineQualified','qualifiedFlatReferences','rateControl','nestedControls']},indent=2))
    assert record['baselineQualified']


if __name__ == '__main__':
    main()
