"""Check mixed fractional-second/frame parent offsets against an independent cue."""
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[2]
WORK=ROOT/'build-windows/beta-ttml-fps-20261029'
OUT=WORK/'mixed-clock-before-r1'
RUNTIME=WORK/'after-r2/runtime'
BUILD=ROOT/'build-windows/beta-inline-webvtt-20261029/test-build-r1'
FIXTURE=ROOT/'test/modules/demux/fixtures/ttml-fps/mixed-clock-frame.ttml'
REFERENCE=WORK/'before-r1/flat0-12/initial.png'
SMOKE=Path(__file__).with_name('run-inline-webvtt-smoke-r1.py')
spec=importlib.util.spec_from_file_location('inline_masks',SMOKE)
analysis=importlib.util.module_from_spec(spec);spec.loader.exec_module(analysis)
sys.path.insert(0,str(ROOT/'test/libvlc'))
import native_http_runtime as support


def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def main():
    assert not OUT.exists()
    build=json.loads((BUILD/'verification.json').read_text());assert build['passed'] and sha(BUILD/'subtitle-fps.exe')==build['binarySha256']
    OUT.mkdir()
    inputs={str(p):sha(p) for p in [Path(__file__),SMOKE,Path(support.__file__),FIXTURE,REFERENCE,
            ROOT/'modules/codec/ttml/ttml.h',RUNTIME/'plugins/codec/libttml_plugin.dll',BUILD/'subtitle-fps.exe']}
    env=support.environment(RUNTIME,OUT);env['VLC_TEST_PLUGIN_PATH']=str(RUNTIME/'plugins')
    command=[str(BUILD/'subtitle-fps.exe'),str(WORK/'media-r1/frame-marker-black-bottom.mp4'),str(FIXTURE),str(OUT),'0','reference','10.5','0']
    record=dict(status='starting',inputs=inputs,command=command,forcedCleanupUsed=False)
    save=lambda:(OUT/'verification.json').write_text(json.dumps(record,indent=2)+'\n')
    with (OUT/'stdout.txt').open('wb') as stdout,(OUT/'stderr.txt').open('wb') as stderr:
        child=subprocess.Popen(command,cwd=RUNTIME,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
        record.update(status='running',pid=child.pid);save()
        try:code=child.wait(timeout=60)
        except subprocess.TimeoutExpired:
            record['status']='observation-timeout-process-left-running';save();raise SystemExit('Inspect same PID')
    data=[json.loads(l) for l in (OUT/'stdout.txt').read_text().splitlines() if l.startswith('{')]
    actual=analysis.masks(OUT/'initial.png');reference=analysis.masks(REFERENCE)
    similarity={k:len(actual[k]&reference[k])/len(actual[k]|reference[k]) if actual[k]|reference[k] else 1. for k in actual}
    record.update(status='terminal',exitCode=code,observations=data,similarity=similarity,
                  actualPixels={k:len(v) for k,v in actual.items()},referencePixels={k:len(v) for k,v in reference.items()},
                  inputsUnchanged=all(sha(p)==h for p,h in inputs.items()))
    record['qualifiedFailure']=code==0 and data[-1].get('executionComplete') and record['inputsUnchanged'] and len(reference['magenta'])>100 and not any(actual.values())
    save();print(json.dumps({k:record[k] for k in ['exitCode','similarity','actualPixels','referencePixels','qualifiedFailure']},indent=2));assert record['qualifiedFailure']


if __name__=='__main__':main()
