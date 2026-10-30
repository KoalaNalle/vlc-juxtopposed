"""Run the real parser/control source test with the matching isolated runtime."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
WORK = ROOT/'build-windows/beta-ttml-fps-20261029'
OUT = WORK/'unit-run-r2'
BUILD = WORK/'unit-build-r3'
RUNTIME = WORK/'after-r2/runtime'
FIXTURES = ROOT/'test/modules/demux/fixtures/ttml-fps'
sys.path.insert(0,str(ROOT/'test/libvlc'))
import native_http_runtime as support


def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def main():
    assert not OUT.exists()
    build=json.loads((BUILD/'verification.json').read_text())
    assert build['passed'] and sha(BUILD/'ttml-fps.exe')==build['binarySha256']
    assert all(sha(ROOT/p)==h for p,h in build['sources'].items())
    OUT.mkdir()
    inputs={str(p):sha(p) for p in [Path(__file__),Path(support.__file__),BUILD/'ttml-fps.exe',
            *[ROOT/p for p in build['sources']],ROOT/'modules/demux/ttml.c',ROOT/'modules/codec/ttml/ttml.h',*sorted(FIXTURES.glob('*.ttml'))]}
    env=support.environment(RUNTIME,OUT)
    env['VLC_TEST_PLUGIN_PATH']=str(RUNTIME/'plugins')
    command=[str(BUILD/'ttml-fps.exe'),str(FIXTURES)]
    record=dict(status='running',command=command,inputs=inputs,forcedCleanupUsed=False)
    save=lambda:(OUT/'verification.json').write_text(json.dumps(record,indent=2)+'\n')
    save()
    with (OUT/'stdout.txt').open('wb') as stdout,(OUT/'stderr.txt').open('wb') as stderr:
        child=subprocess.Popen(command,cwd=RUNTIME,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
        record['pid']=child.pid;save()
        try:
            code=child.wait(timeout=60)
        except subprocess.TimeoutExpired:
            record['status']='observation-timeout-process-left-running';save()
            raise SystemExit('Inspect same owned PID')
    rows=[json.loads(l) for l in (OUT/'stdout.txt').read_text().splitlines() if l.startswith('{')]
    record.update(status='terminal',exitCode=code,results=rows,inputsUnchanged=all(sha(p)==h for p,h in inputs.items()))
    record['passed']=code==0 and rows[-1]['summary'] and rows[-1]['failures']==0 and record['inputsUnchanged']
    save();print(json.dumps({k:record[k] for k in ['passed','exitCode','results','inputsUnchanged']},indent=2))
    assert record['passed']


if __name__=='__main__':
    main()
