"""Compare actual live inline colour phases with independently timed VTT files."""
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
from PySide6.QtGui import QImage

ROOT=Path(__file__).resolve().parents[2]
WORK=ROOT/'build-windows/beta-inline-webvtt-20261029'
OUT=WORK/'renderer-r2'
PREVIOUS=WORK/'renderer-r1'
RUNTIME=PREVIOUS/'runtime'
MEDIA=WORK/'media-r1'
BUILD=WORK/'test-build-r1'
SMOKE=Path(__file__).with_name('run-inline-webvtt-smoke-r1.py')
spec=importlib.util.spec_from_file_location('frozen_inline_masks',SMOKE)
analysis=importlib.util.module_from_spec(spec);spec.loader.exec_module(analysis)
sys.path.insert(0,str(ROOT/'test/libvlc'))
import native_http_runtime as support


def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def load(p):return json.loads(Path(p).read_text(encoding='utf-8-sig'))
def score(a,b):
    return {k:(len(a[k]&b[k])/len(a[k]|b[k]) if a[k]|b[k] else 1.) for k in a}
def marker(p):
    im=QImage(str(p));assert not im.isNull()
    return sum(1<<b for b in range(11) if im.pixelColor(32+64*b,20).red()>128)


def main():
    assert not OUT.exists();old=load(PREVIOUS/'verification.json')
    assert old['status']=='terminal' and old['inputsUnchanged'] and old['runtimeUnchanged']
    assert all(sha(RUNTIME/p)==h for p,h in old['runtimeSha256'].items())
    build=load(BUILD/'verification.json');assert build['passed']
    assert sha(BUILD/'subtitle-fps.exe')==build['binarySha256']
    assert sha(ROOT/'test/src/player/subtitle_fps.c')==build['sourceSha256']
    OUT.mkdir();baseline=OUT/'baseline-runtime';support.stage_runtime(RUNTIME,baseline)
    original=ROOT/'build-windows/beta-fps-webvtt-20261028/plugin-r2-20261029/build/original/1-libwebvtt_plugin.dll'
    assert sha(original)=='e6d599aabf0e035d6bce10b9a01e10b70bb1c889ca331604267823461aad9393'
    (baseline/'plugins/codec/libwebvtt_plugin.dll').unlink();shutil.copyfile(original,baseline/'plugins/codec/libwebvtt_plugin.dll')
    support.regenerate_cache(baseline,OUT/'baseline-cache')
    inputs={str(p):sha(p) for p in [Path(__file__),SMOKE,Path(support.__file__),ROOT/'test/src/player/subtitle_fps.c',
            ROOT/'modules/demux/webvtt.c',BUILD/'subtitle-fps.exe',original,PREVIOUS/'verification.json',
            *sorted(MEDIA.glob('*.vtt')),MEDIA/'frame-marker-black-bottom.mp4']}
    baseline_hashes={str(p.relative_to(baseline)):sha(p) for p in baseline.rglob('*') if p.is_file() and p.name!='plugins.dat' and 'portable' not in p.relative_to(baseline).parts}
    record=dict(status='running',inputs=inputs,scope='Actual Core FPS, inline CSS phases, held frame/time, playing clock and native inherited/reset; no ordinary UI, broad-format or package acceptance',
                runtimeSha256=old['runtimeSha256'],baselineRuntimeSha256=baseline_hashes,cases=[],checks={},forcedCleanupUsed=False)
    save=lambda:(OUT/'verification.json').write_text(json.dumps(record,indent=2)+'\n')
    images={};rows={}
    for label in ['base-8','reference50-8','inherited50-8']:
        images[label]={p.stem:analysis.masks(p) for p in (PREVIOUS/label).glob('*.png')}
        row=next(r for r in old['cases'] if r['name']==label)
        rows[label]=[o for o in row['observations'] if 'step' in o]
    record['checks']['referenceContrast']=sum(len(images['base-8']['initial'][k]^images['reference50-8']['initial'][k]) for k in ['green','magenta','white'])>100
    record['checks']['referencePhases']=len(images['base-8']['initial']['white'])>50 and len(images['base-8']['initial']['magenta'])>50 and len(images['reference50-8']['initial']['green'])>50
    record['checks']['inherited50MatchesReference']=min(score(images['inherited50-8']['initial'],images['reference50-8']['initial']).values())>.97
    record['checks']['inheritedResetMatchesDefault']=min(score(images['inherited50-8']['reset'],images['base-8']['initial']).values())>.97
    save()
    specs=[('reference0-12','base.vtt',0,'reference',12,0,False),
           ('reference50-12','reference-50.vtt',0,'reference',12,0,False),
           ('reference0-18','base.vtt',0,'reference',18,0,False),
           ('reference2997-18','reference-29_97.vtt',0,'reference',18,0,False),
           ('reference0-20_4','base.vtt',0,'reference',20.4,0,False),
           ('reference23976-20_4','reference-23_976.vtt',0,'reference',20.4,0,False),
           ('reference0-delay2','base.vtt',0,'reference',8,2_000_000,False),
           ('reference50-delay2','reference-50.vtt',0,'reference',8,2_000_000,False),
           ('hold50-8','base.vtt',50,'hold',8,0,False),
           ('hold50-12','base.vtt',50,'hold',12,0,False),
           ('hold2997-18','base.vtt',29.97,'hold',18,0,False),
           ('hold23976-20_4','base.vtt',23.976,'hold',20.4,0,False),
           ('hold50-delay2','base.vtt',50,'hold',8,2_000_000,False),
           ('playing-first50','base.vtt',50,'playing',5.5,0,False),
           ('original-hold50-8','base.vtt',50,'hold',8,0,True)]
    for name,subtitle,fps,mode,target,delay,is_old in specs:
        runtime=baseline if is_old else RUNTIME
        run=OUT/name;run.mkdir();env=support.environment(runtime,run);env['VLC_TEST_PLUGIN_PATH']=str(runtime/'plugins')
        command=[str(BUILD/'subtitle-fps.exe'),str(MEDIA/'frame-marker-black-bottom.mp4'),str(MEDIA/subtitle),str(run),str(fps),mode,str(target),str(delay)]
        row=dict(name=name,command=command,status='starting',variant='original' if is_old else 'candidate');record['cases'].append(row);save()
        with (run/'stdout.txt').open('wb') as stdout,(run/'stderr.txt').open('wb') as stderr:
            child=subprocess.Popen(command,cwd=runtime,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
            row.update(status='running',pid=child.pid);save();print('OWNED_INLINE_RUNNING '+str(child.pid)+' '+name,flush=True)
            try:code=child.wait(timeout=60)
            except subprocess.TimeoutExpired:
                row.update(status='observation-timeout-process-left-running');save();raise SystemExit('Inspect owned PID; do not restart')
        data=[json.loads(l) for l in (run/'stdout.txt').read_text().splitlines() if l.startswith('{')]
        row.update(status='terminal',exitCode=code,observations=data);save()
        assert code==0 and data[-1].get('executionComplete'),name
        images[name]={p.stem:analysis.masks(p) for p in run.glob('*.png')};rows[name]=[o for o in data if 'step' in o]
        row['pixels']={step:{k:len(v) for k,v in masks.items()} for step,masks in images[name].items()}
        row['markers']={p.stem:marker(p) for p in run.glob('*.png')}
        errors=[l for l in (run/'native.log').read_text(errors='replace').splitlines() if 'error:' in l]
        row['otherErrors']=[l for l in errors if l.rsplit('error:',1)[1].strip()!='window closed']
        row['teardownWindowCloseDiagnostics']=len(errors)-len(row['otherErrors']);save()
    comparisons=[]
    for live,normal,reference in [('hold50-8','base-8','reference50-8'),('hold50-12','reference0-12','reference50-12'),
          ('hold2997-18','reference0-18','reference2997-18'),('hold23976-20_4','reference0-20_4','reference23976-20_4'),
          ('hold50-delay2','reference0-delay2','reference50-delay2')]:
        s={step:score(images[live][step],images[ref]['initial']) for step,ref in [('initial',normal),('changed',reference),('reset',normal)]}
        row=next(r for r in record['cases'] if r['name']==live)
        frame=len(set(row['markers'].values()))==1
        held_time=max(r['timeUs'] for r in rows[live])-min(r['timeUs'] for r in rows[live])<50_000
        values=all(r['delayUs']==int(row['command'][-1]) and r['state']==3 for r in rows[live])
        passed=min(v for t in s.values() for v in t.values())>.97 and frame and held_time and values
        comparisons.append(dict(case=live,similarity=s,heldFrame=frame,heldTime=held_time,delayAndPausedState=values,passed=passed))
    playing=rows['playing-first50'];playing=[r for r in playing if r['step'].startswith('playing-')]
    phase=[min(score(images['playing-first50'][r['step']],images['reference50-8' if r['step']=='playing-changed' else 'base-8']['initial']).values()) for r in playing]
    times=[r['timeUs'] for r in playing]
    record['playing']=dict(similarity=phase,times=times,passed=len(playing)==3 and min(phase)>.97
        and all(r['state']==2 for r in playing) and all(5_000_000<t<9_900_000 for t in times)
        and times[0]<times[1]<times[2])
    old_images=images['original-hold50-8']
    old_scores={step:score(old_images[step],images[ref]['initial']) for step,ref in [('initial','base-8'),('changed','reference50-8'),('reset','base-8')]}
    record['negativeControl']=dict(similarity=old_scores,qualified=min(old_scores['initial'].values())>.97
        and min(old_scores['reset'].values())>.97 and min(old_scores['changed'].values())<.8)
    record.update(status='terminal',comparisons=comparisons,inputsUnchanged=all(sha(p)==h for p,h in inputs.items()),
                  runtimeUnchanged=all(sha(RUNTIME/p)==h for p,h in old['runtimeSha256'].items()),
                  baselineUnchanged=all(sha(baseline/p)==h for p,h in baseline_hashes.items()))
    record['passed']=all(record['checks'].values()) and all(r['passed'] for r in comparisons) and record['playing']['passed']
    record['passed'] &= all(not r['otherErrors'] for r in record['cases']) and record['negativeControl']['qualified']
    record['passed'] &= record['inputsUnchanged'] and record['runtimeUnchanged'] and record['baselineUnchanged']
    save();print(json.dumps({k:record[k] for k in ['passed','checks','comparisons','playing','negativeControl']},indent=2))
    assert record['passed']


if __name__=='__main__':main()
