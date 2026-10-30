"""Retain the failed nominal-time calibration; bracket boundaries at actual frames."""
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[2]
WORK=ROOT/'build-windows/beta-ttml-fps-20261029'
OUT=WORK/'after-r5'
PREVIOUS=WORK/'after-r4'
RUNTIME=PREVIOUS/'runtime'
MEDIA=WORK/'media-r3'
BUILD=ROOT/'build-windows/beta-inline-webvtt-20261029/test-build-r1'
SMOKE=Path(__file__).with_name('run-inline-webvtt-smoke-r1.py')
spec=importlib.util.spec_from_file_location('inline_masks',SMOKE)
analysis=importlib.util.module_from_spec(spec);spec.loader.exec_module(analysis)
sys.path.insert(0,str(ROOT/'test/libvlc'))
import native_http_runtime as support


def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def load(p):return json.loads(Path(p).read_text(encoding='utf-8-sig'))


def main():
    assert not OUT.exists()
    old=load(PREVIOUS/'verification.json')
    assert old['status']=='terminal' and not old['passed'] and not old['parameterReferencesQualified']
    assert old['inputsUnchanged'] and old['runtimeUnchanged']
    assert all(sha(p)==h for p,h in old['inputs'].items())
    assert all(sha(RUNTIME/p)==h for p,h in old['runtimeSha256'].items())
    assert all(c['passed'] for c in old['comparisons']) and old['playing']['passed']
    assert old['referencePhasesQualified'] and old['mixedClockMatchesReference']
    assert all(c['exitCode']==0 and c['observations'][-1].get('executionComplete') and not c['otherErrors'] for c in old['cases'])
    assert min(v for group in ['parameterScores','parameterInherited','inherited','nested'] for row in old[group].values() for v in row.values())>.97
    OUT.mkdir()
    parser=ROOT/'modules/codec/ttml/ttml.c'
    compiled=load(WORK/'plugin-build-r4/inputs.json')
    assert sha(parser)==compiled['inputs']['/mnt/c/Modding/vlc-juxtopposed/modules/codec/ttml/ttml.c']
    inputs=dict(old['inputs'])
    inputs.update({str(p):sha(p) for p in [Path(__file__),PREVIOUS/'verification.json',parser,WORK/'plugin-build-r4/inputs.json']})
    record=dict(status='running',scope='Full R4 matrix plus corrected pre-boundary samples using actual video clock; no broad TTML/GUI/package acceptance',
                inputs=inputs,runtimeSha256=old['runtimeSha256'],cases=[],forcedCleanupUsed=False,
                preservedR4MatrixCases=len(old['cases']))
    save=lambda:(OUT/'verification.json').write_text(json.dumps(record,indent=2)+'\n')
    save();images={}
    for name,subtitle,target in [('multiplier-before','multiplier.ttml',10),('ref-multiplier-before','reference-multiplier.ttml',10),
                                 ('subframes-before','subframes.ttml',10.18),('ref-subframes-before','reference-subframes.ttml',10.18)]:
        run=OUT/name;run.mkdir();env=support.environment(RUNTIME,run);env['VLC_TEST_PLUGIN_PATH']=str(RUNTIME/'plugins')
        command=[str(BUILD/'subtitle-fps.exe'),str(MEDIA/'frame-marker-black-bottom.mp4'),str(MEDIA/subtitle),str(run),'0','reference',str(target),'0']
        row=dict(name=name,command=command,status='starting');record['cases'].append(row);save()
        with (run/'stdout.txt').open('wb') as stdout,(run/'stderr.txt').open('wb') as stderr:
            child=subprocess.Popen(command,cwd=RUNTIME,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
            row.update(status='running',pid=child.pid);save();print('OWNED_TTML_BOUNDARY '+str(child.pid)+' '+name,flush=True)
            try:code=child.wait(timeout=60)
            except subprocess.TimeoutExpired:
                row['status']='observation-timeout-process-left-running';save();raise SystemExit('Inspect same PID')
        rows=[json.loads(l) for l in (run/'stdout.txt').read_text().splitlines() if l.startswith('{')]
        row.update(status='terminal',exitCode=code,observations=rows)
        assert code==0 and rows[-1].get('executionComplete')
        images[name]=analysis.masks(run/'initial.png')
        row['pixels']={k:len(v) for k,v in images[name].items()}
        errors=[l for l in (run/'native.log').read_text(errors='replace').splitlines() if 'error:' in l]
        row['otherErrors']=[l for l in errors if l.rsplit('error:',1)[1].strip()!='window closed'];save()
    checks={}
    for name,boundary in [('multiplier',10010000),('subframes',10220000)]:
        fresh=[r for r in record['cases'] if r['name'] in [name+'-before','ref-'+name+'-before']]
        positive=[r for r in old['cases'] if r['name'] in [name+'-after','ref-'+name+'-after']]
        checks[name]=len(fresh)==2 and len(positive)==2
        checks[name] &= all(r['observations'][0]['timeUs']<boundary and not any(r['pixels'].values()) for r in fresh)
        checks[name] &= all(r['observations'][0]['timeUs']>boundary and r['pixels']['initial']['magenta']>100 for r in positive)
    positive_names=['ref-frame25-after','ref-ticks100-12','ref-multiplier-after','ref-subframes-after']
    checks['nonemptyPositiveReferences']=all(next(r for r in old['cases'] if r['name']==n)['pixels']['initial']['magenta']>100 for n in positive_names)
    checks['frame25Before']=not any(next(r for r in old['cases'] if r['name']=='ref-frame25-before')['pixels']['initial'].values())
    record.update(status='terminal',checks=checks,inputsUnchanged=all(sha(p)==h for p,h in inputs.items()),
                  runtimeUnchanged=all(sha(RUNTIME/p)==h for p,h in old['runtimeSha256'].items()))
    record['passed']=all(checks.values()) and record['inputsUnchanged'] and record['runtimeUnchanged'] and all(not r['otherErrors'] for r in record['cases'])
    save();print(json.dumps({k:record[k] for k in ['passed','checks','preservedR4MatrixCases','inputsUnchanged','runtimeUnchanged']},indent=2));assert record['passed']


if __name__=='__main__':main()
