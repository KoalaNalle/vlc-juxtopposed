"""Run two isolated, bounded actual decoder probes against the current Core."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'build-windows/beta-record-transition-20261029/paused-selection-r4'
BASE=ROOT/'build-windows/beta-resume-20261029/gui-r13/runtime'
PROBE=ROOT/'test/libvlc/native_recorded_webvtt_pause.py'
MEDIA=ROOT/'build-windows/beta-resume-20261029/gui-r11/media/recorded-wvtt.mp4'
sys.path.insert(0,str(ROOT/'test/libvlc'))
import native_http_runtime as support


def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    assert not OUT.exists()
    assert sha(BASE/'libvlccore.dll')=='4d3952cedf6d6eccc747cf4545d29a7e1283e2968e8e15093c6c754299959d8d'
    assert sha(MEDIA)=='9fabdf4bea090ae9cff35799c80a286717a7ae72d8ee3820bda3ccb88ef95a56'
    OUT.mkdir()
    runtime=OUT/'runtime';support.stage_runtime(BASE,runtime)
    core=ROOT/'build-windows/beta-record-transition-20261029/core-build-r3/candidate/libvlccore.dll'
    assert sha(core)=='4a2865915e342744a5a7092171b92279b08f7b229eede231e00c3e86d88cff84'
    import shutil
    (runtime/'libvlccore.dll').unlink()
    shutil.copyfile(core,runtime/'libvlccore.dll')
    support.regenerate_cache(runtime,OUT/'cache')
    expected={str(p.relative_to(runtime)):sha(p) for p in runtime.rglob('*')
              if p.is_file() and p.name!='plugins.dat' and 'portable' not in p.relative_to(runtime).parts}
    inputs={str(p):sha(p) for p in [Path(__file__),PROBE,MEDIA,Path(support.__file__),core,ROOT/'src/player/input.c']}
    record=dict(status='running',inputs=inputs,runtimeSha256=expected,children=[],forcedCleanupUsed=False)

    def save():(OUT/'verification.json').write_text(json.dumps(record,indent=2)+'\n')

    children=[];save()
    for attempt,start in enumerate([0,12,12,12,12,12],1):
        run=OUT/f'case-{attempt}-start-{start}'
        run.mkdir();result=run/'result';result.mkdir()
        env=support.environment(runtime,run)
        command=[sys.executable,str(PROBE),str(runtime),str(result),str(MEDIA),str(start)]
        stdout=(run/'stdout.txt').open('wb');stderr=(run/'stderr.txt').open('wb')
        child=subprocess.Popen(command,env=env,cwd=runtime,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
        row=dict(start=start,pid=child.pid,command=command,status='running')
        record['children'].append(row);children.append((child,row,run,stdout,stderr));save()
        print('OWNED_NATIVE_PROBE_RUNNING '+str(child.pid)+' start='+str(start),flush=True)
    for child,row,run,stdout,stderr in children:
        try:code=child.wait(timeout=90)
        except subprocess.TimeoutExpired:
            row['status']='observation-timeout-process-left-running';save()
            raise SystemExit('Inspect exact owned PID; do not restart')
        stdout.close();stderr.close()
        row.update(status='terminal',exitCode=code)
        result=json.loads((run/'result/result.json').read_text())
        row.update(checks=result.get('checks'),passed=result['passed'],executionError=result.get('executionError'))
        save();print(json.dumps(row),flush=True)
    record.update(status='terminal',inputsUnchanged=all(sha(p)==h for p,h in inputs.items()),
                  runtimeUnchanged=all(sha(runtime/p)==h for p,h in expected.items()))
    save();assert record['inputsUnchanged'] and record['runtimeUnchanged']


if __name__=='__main__':main()
