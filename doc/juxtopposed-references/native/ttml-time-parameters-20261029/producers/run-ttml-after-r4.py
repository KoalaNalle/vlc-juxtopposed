"""Actual TTML live/inherited/reset, nesting and EOF observations on the candidate."""
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import sys
from PySide6.QtGui import QImage

ROOT = Path(__file__).resolve().parents[2]
WORK = ROOT / 'build-windows/beta-ttml-fps-20261029'
OUT = WORK / 'after-r4'
PREVIOUS = WORK / 'before-r1'
MEDIA = WORK / 'media-r3'
BUILD = ROOT / 'build-windows/beta-inline-webvtt-20261029/test-build-r1'
PLUGIN = WORK / 'plugin-build-r4'
SMOKE = Path(__file__).with_name('run-inline-webvtt-smoke-r1.py')
spec = importlib.util.spec_from_file_location('inline_pixel_masks', SMOKE)
analysis = importlib.util.module_from_spec(spec)
spec.loader.exec_module(analysis)
sys.path.insert(0, str(ROOT / 'test/libvlc'))
import native_http_runtime as support


def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def load(p):
    return json.loads(Path(p).read_text(encoding='utf-8-sig'))


def score(a,b):
    return {k:len(a[k]&b[k])/len(a[k]|b[k]) if a[k]|b[k] else 1. for k in a}


def main():
    assert not OUT.exists() and shutil.disk_usage(ROOT).free >= 399864390452 + 128 * 1024**2
    before = load(PREVIOUS / 'verification.json')
    plugin = load(PLUGIN / 'verification.json')
    build = load(BUILD / 'verification.json')
    assert before['baselineQualified'] and plugin['success'] and plugin['ownedArtifactsRestored']
    assert build['passed'] and sha(BUILD / 'subtitle-fps.exe') == build['binarySha256']
    assert sha(ROOT / 'test/src/player/subtitle_fps.c') == build['sourceSha256']
    OUT.mkdir()
    runtime = OUT / 'runtime'
    support.stage_runtime(PREVIOUS / 'runtime',runtime)
    (runtime / 'plugins/codec/libttml_plugin.dll').unlink()
    shutil.copyfile(PLUGIN / 'libttml_plugin.dll',runtime / 'plugins/codec/libttml_plugin.dll')
    assert sha(runtime / 'plugins/codec/libttml_plugin.dll') == plugin['pluginSha256']
    support.regenerate_cache(runtime,OUT / 'cache')
    hashes = {str(p.relative_to(runtime)):sha(p) for p in runtime.rglob('*') if p.is_file() and p.name != 'plugins.dat' and 'portable' not in p.relative_to(runtime).parts}
    inputs = {str(p):sha(p) for p in [Path(__file__),SMOKE,Path(support.__file__),ROOT / 'test/src/player/subtitle_fps.c',
              ROOT / 'modules/demux/ttml.c',ROOT / 'modules/codec/ttml/genttml.c',ROOT / 'modules/codec/ttml/ttml.h',
              BUILD / 'subtitle-fps.exe',PLUGIN / 'verification.json',PREVIOUS / 'verification.json',
              *sorted(MEDIA.glob('*.ttml')),MEDIA / 'frame-marker-black-bottom.mp4']}
    record = dict(status='running',scope='Actual TTML decoder FPS/nested/reset/EOF cases; not GUI or all TTML profiles',
                  inputs=inputs,runtimeSha256=hashes,cases=[],forcedCleanupUsed=False)
    save = lambda:(OUT / 'verification.json').write_text(json.dumps(record,indent=2)+'\n')
    save()
    images = {name:{p.stem:analysis.masks(p) for p in (PREVIOUS / name).glob('*.png')}
              for name in ['flat0-8','reference50-8','flat0-12']}
    rows = {}
    specs = [
        ('flat0-18','flat.ttml',0,'reference',18,0),
        ('ref2997-18','reference-29_97.ttml',0,'reference',18,0),
        ('flat0-20_4','flat.ttml',0,'reference',20.4,0),
        ('ref23976-20_4','reference-23_976.ttml',0,'reference',20.4,0),
        ('ref50-12','reference-50.ttml',0,'reference',12,0),
        ('flat0-delay2','flat.ttml',0,'reference',8,2000000),
        ('ref50-delay2','reference-50.ttml',0,'reference',8,2000000),
        ('flat0-40','flat.ttml',0,'reference',40,0),
        ('ref50-40','reference-50.ttml',0,'reference',40,0),
        ('hold50-8','flat.ttml',50,'hold',8,0),
        ('hold50-12','flat.ttml',50,'hold',12,0),
        ('hold2997-18','flat.ttml',29.97,'hold',18,0),
        ('hold23976-20_4','flat.ttml',23.976,'hold',20.4,0),
        ('hold50-delay2','flat.ttml',50,'hold',8,2000000),
        ('hold50-eof-reset','flat.ttml',50,'hold',40,0),
        ('nested-par0-12','nested-par.ttml',0,'reference',12,0),
        ('nested-seq0-12','nested-seq.ttml',0,'reference',12,0),
        ('inherited50-8','flat.ttml',50,'inherited',8,0),
        ('playing-first50','flat.ttml',50,'playing',5.5,0),
        ('mixed0-10_5','mixed-clock-frame.ttml',0,'reference',10.5,0),
        ('mixed0-8','mixed-clock-frame.ttml',0,'reference',8,0),
        ('mixed50-8','mixed-clock-frame.ttml',50,'hold',8,0),
        ('frame25-before','frame25.ttml',0,'reference',10.05,0),
        ('ref-frame25-before','reference-frame25.ttml',0,'reference',10.05,0),
        ('frame25-after','frame25.ttml',0,'reference',10.15,0),
        ('ref-frame25-after','reference-frame25.ttml',0,'reference',10.15,0),
        ('ref-frame25-50','reference-frame25-50.ttml',0,'reference',8,0),
        ('frame25-0','frame25.ttml',0,'reference',8,0),
        ('frame25-hold50','frame25.ttml',50,'hold',8,0),
        ('frame25-inherited50','frame25.ttml',50,'inherited',8,0),
        ('ticks100-12','ticks100.ttml',0,'reference',12,0),
        ('ref-ticks100-12','reference-ticks.ttml',0,'reference',12,0),
        ('ticks100-0','ticks100.ttml',0,'reference',8,0),
        ('ref-ticks100-50','reference-ticks100-50.ttml',0,'reference',8,0),
        ('ticks100-hold50','ticks100.ttml',50,'hold',8,0),
        ('derivedticks-12','derivedticks.ttml',0,'reference',12,0),
        ('defaultticks-12','defaultticks.ttml',0,'reference',12,0),
        ('multiplier-before','multiplier.ttml',0,'reference',10.005,0),
        ('ref-multiplier-before','reference-multiplier.ttml',0,'reference',10.005,0),
        ('multiplier-after','multiplier.ttml',0,'reference',10.05,0),
        ('ref-multiplier-after','reference-multiplier.ttml',0,'reference',10.05,0),
        ('subframes-before','subframes.ttml',0,'reference',10.21,0),
        ('ref-subframes-before','reference-subframes.ttml',0,'reference',10.21,0),
        ('subframes-after','subframes.ttml',0,'reference',10.26,0),
        ('ref-subframes-after','reference-subframes.ttml',0,'reference',10.26,0)]
    for name, subtitle, fps, mode, target, delay in specs:
        run = OUT / name
        run.mkdir()
        env = support.environment(runtime,run)
        env['VLC_TEST_PLUGIN_PATH'] = str(runtime / 'plugins')
        command = [str(BUILD / 'subtitle-fps.exe'),str(MEDIA / 'frame-marker-black-bottom.mp4'),str(MEDIA / subtitle),str(run),str(fps),mode,str(target),str(delay)]
        row = dict(name=name,command=command,status='starting')
        record['cases'].append(row)
        save()
        with (run / 'stdout.txt').open('wb') as stdout,(run / 'stderr.txt').open('wb') as stderr:
            child = subprocess.Popen(command,cwd=runtime,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
            row.update(status='running',pid=child.pid)
            save()
            print('OWNED_TTML_RUNNING '+str(child.pid)+' '+name,flush=True)
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
        rows[name] = [d for d in data if 'step' in d]
        row['pixels'] = {step:{k:len(v) for k,v in masks.items()} for step,masks in images[name].items()}
        row['markers'] = {}
        for p in run.glob('*.png'):
            image = QImage(str(p))
            row['markers'][p.stem] = sum(1<<b for b in range(11) if image.pixelColor(32+64*b,20).red()>128)
        errors = [l for l in (run / 'native.log').read_text(errors='replace').splitlines() if 'error:' in l]
        row['otherErrors'] = [l for l in errors if l.rsplit('error:',1)[1].strip() != 'window closed']
        row['teardownWindowCloseDiagnostics'] = len(errors)-len(row['otherErrors'])
        save()
    comparisons = []
    for name, normal, reference in [('hold50-8','flat0-8','reference50-8'),('hold50-12','flat0-12','ref50-12'),
            ('hold2997-18','flat0-18','ref2997-18'),('hold23976-20_4','flat0-20_4','ref23976-20_4'),
            ('hold50-delay2','flat0-delay2','ref50-delay2'),('hold50-eof-reset','flat0-40','ref50-40'),
            ('mixed50-8','mixed0-8','reference50-8'),
            ('frame25-hold50','frame25-0','ref-frame25-50'),
            ('ticks100-hold50','ticks100-0','ref-ticks100-50')]:
        similarities = {step:score(images[name][step],images[ref]['initial']) for step,ref in [('initial',normal),('changed',reference),('reset',normal)]}
        row = next(c for c in record['cases'] if c['name']==name)
        held = len(set(row['markers'].values()))==1 and max(d['timeUs'] for d in rows[name])-min(d['timeUs'] for d in rows[name])<50000
        correct_state = all(d['state']==3 and d['delayUs']==int(row['command'][-1]) for d in rows[name])
        comparisons.append(dict(case=name,similarity=similarities,heldFrameAndTime=held,
                               pausedAndDelay=correct_state,passed=held and correct_state and min(v for s in similarities.values() for v in s.values())>.97))
    inherited = images['inherited50-8']
    record['inherited'] = dict(initial=score(inherited['initial'],images['reference50-8']['initial']),reset=score(inherited['reset'],images['flat0-8']['initial']))
    record['nested'] = {name:score(images[name]['initial'],images['flat0-12']['initial']) for name in ['nested-par0-12','nested-seq0-12']}
    playing = [d for d in rows['playing-first50'] if d['step'].startswith('playing-')]
    times = [d['timeUs'] for d in playing]
    phases = [score(images['playing-first50'][d['step']],images['reference50-8' if d['step']=='playing-changed' else 'flat0-8']['initial']) for d in playing]
    record['playing'] = dict(times=times,similarity=phases,passed=len(playing)==3 and all(d['state']==2 for d in playing)
                            and all(5_000_000<t<9_900_000 for t in times) and times[0]<times[1]<times[2]
                            and min(v for s in phases for v in s.values())>.97)
    record['mixedClockMatchesReference'] = min(score(images['mixed0-10_5']['initial'],images['flat0-12']['initial']).values())>.97
    record['referencePhasesQualified'] = len(images['ref50-12']['initial']['white'])>100 and len(images['ref2997-18']['initial']['white'])>100
    record['referencePhasesQualified'] &= len(images['ref23976-20_4']['initial']['magenta'])>100 and not any(images['ref50-40']['initial'].values())
    parameter_scores = {name:score(images[name]['initial'],images[ref]['initial']) for name,ref in [
        ('frame25-before','ref-frame25-before'),('frame25-after','ref-frame25-after'),
        ('ticks100-12','ref-ticks100-12'),('derivedticks-12','ref-ticks100-12'),('defaultticks-12','ref-ticks100-12'),
        ('multiplier-before','ref-multiplier-before'),('multiplier-after','ref-multiplier-after'),
        ('subframes-before','ref-subframes-before'),('subframes-after','ref-subframes-after')]}
    record['parameterScores'] = parameter_scores
    record['parameterReferencesQualified'] = all(len(images[n]['initial']['magenta'])>100 for n in ['ref-frame25-after','ref-ticks100-12','ref-multiplier-after','ref-subframes-after'])
    record['parameterReferencesQualified'] &= all(not any(images[n]['initial'].values()) for n in ['ref-frame25-before','ref-multiplier-before','ref-subframes-before'])
    record['parameterInherited'] = dict(initial=score(images['frame25-inherited50']['initial'],images['ref-frame25-50']['initial']),reset=score(images['frame25-inherited50']['reset'],images['frame25-0']['initial']))
    record['parametersPassed'] = record['parameterReferencesQualified'] and min(v for s in parameter_scores.values() for v in s.values())>.97
    record['parametersPassed'] &= min(v for s in record['parameterInherited'].values() for v in s.values())>.97
    record.update(status='terminal',comparisons=comparisons,inputsUnchanged=all(sha(p)==h for p,h in inputs.items()),
                  runtimeUnchanged=all(sha(runtime / p)==h for p,h in hashes.items()))
    record['passed'] = all(c['passed'] for c in comparisons) and record['playing']['passed'] and record['referencePhasesQualified'] and record['mixedClockMatchesReference']
    record['passed'] &= min(v for s in record['inherited'].values() for v in s.values())>.97 and min(v for s in record['nested'].values() for v in s.values())>.97
    record['passed'] &= record['parametersPassed']
    record['passed'] &= record['inputsUnchanged'] and record['runtimeUnchanged'] and all(not c['otherErrors'] for c in record['cases'])
    save()
    print(json.dumps({k:record[k] for k in ['passed','parametersPassed','parameterScores','parameterInherited','playing']},indent=2))
    assert record['passed']


if __name__ == '__main__':
    main()
