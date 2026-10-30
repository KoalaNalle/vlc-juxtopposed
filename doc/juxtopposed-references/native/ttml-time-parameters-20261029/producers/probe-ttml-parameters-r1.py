"""Separate declared-frame/tick parameter controls before changing the parser."""
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[2]
WORK=ROOT/'build-windows/beta-ttml-fps-20261029'
OUT=WORK/'parameters-before-r1'
RUNTIME=WORK/'after-r3/runtime'
BUILD=ROOT/'build-windows/beta-inline-webvtt-20261029/test-build-r1'
SMOKE=Path(__file__).with_name('run-inline-webvtt-smoke-r1.py')
spec=importlib.util.spec_from_file_location('inline_masks',SMOKE)
analysis=importlib.util.module_from_spec(spec);spec.loader.exec_module(analysis)
sys.path.insert(0,str(ROOT/'test/libvlc'))
import native_http_runtime as support


def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def main():
    assert not OUT.exists()
    accepted=json.loads((WORK/'after-r3/verification.json').read_text())
    assert accepted['passed'] and accepted['runtimeUnchanged']
    assert all(sha(RUNTIME/p)==h for p,h in accepted['runtimeSha256'].items())
    build=json.loads((BUILD/'verification.json').read_text())
    assert build['passed'] and sha(BUILD/'subtitle-fps.exe')==build['binarySha256']
    OUT.mkdir()
    media=OUT/'fixtures';media.mkdir()
    source=ROOT/'test/modules/demux/fixtures/ttml-fps/mixed-clock-frame.ttml'
    frame25=source.read_text().replace('ttp:frameRate="30"','ttp:frameRate="25"')
    (media/'frame25.ttml').write_text(frame25,newline='\n')
    prefix=frame25.split('<body',1)[0]
    reference=prefix+'<body><div><p region="caption" begin="10.1s" end="20.1s" tts:color="#FF00FF">BETA</p></div></body></tt>\n'
    (media/'reference-frame25.ttml').write_text(reference,newline='\n')
    ticks=prefix.replace('ttp:frameRate="25"','ttp:tickRate="100"')+'<body><div><p region="caption" begin="1000t" end="2000t" tts:color="#FF00FF">BETA</p></div></body></tt>\n'
    (media/'ticks100.ttml').write_text(ticks,newline='\n')
    (media/'reference-ticks.ttml').write_text(ticks.replace('1000t','10s').replace('2000t','20s'),newline='\n')
    inputs={str(p):sha(p) for p in [Path(__file__),SMOKE,Path(support.__file__),source,BUILD/'subtitle-fps.exe',
            ROOT/'modules/codec/ttml/ttml.c',ROOT/'modules/codec/ttml/ttml.h',*sorted(media.glob('*.ttml'))]}
    record=dict(status='running',scope='Unfinished declared-frame/tick parser behavior; deliberate negative controls, not accepted rendering',
                inputs=inputs,runtimeSha256=accepted['runtimeSha256'],cases=[],forcedCleanupUsed=False)
    save=lambda:(OUT/'verification.json').write_text(json.dumps(record,indent=2)+'\n')
    save();images={}
    for name,subtitle,time in [('frame25-10_05','frame25.ttml',10.05),('ref-frame25-10_05','reference-frame25.ttml',10.05),
            ('ref-frame25-10_15','reference-frame25.ttml',10.15),('ticks100-12','ticks100.ttml',12),('ref-ticks100-12','reference-ticks.ttml',12)]:
        run=OUT/name;run.mkdir();env=support.environment(RUNTIME,run);env['VLC_TEST_PLUGIN_PATH']=str(RUNTIME/'plugins')
        command=[str(BUILD/'subtitle-fps.exe'),str(WORK/'media-r2/frame-marker-black-bottom.mp4'),str(media/subtitle),str(run),'0','reference',str(time),'0']
        row=dict(name=name,command=command,status='starting');record['cases'].append(row);save()
        with (run/'stdout.txt').open('wb') as stdout,(run/'stderr.txt').open('wb') as stderr:
            child=subprocess.Popen(command,cwd=RUNTIME,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
            row.update(status='running',pid=child.pid);save();print('OWNED_TTML_PARAMETER '+str(child.pid)+' '+name,flush=True)
            try:code=child.wait(timeout=60)
            except subprocess.TimeoutExpired:
                row['status']='observation-timeout-process-left-running';save();raise SystemExit('Inspect same PID')
        data=[json.loads(l) for l in (run/'stdout.txt').read_text().splitlines() if l.startswith('{')]
        row.update(status='terminal',exitCode=code,observations=data);save()
        assert code==0 and data[-1].get('executionComplete')
        images[name]=analysis.masks(run/'initial.png')
        row['pixels']={k:len(v) for k,v in images[name].items()}
        errors=[l for l in (run/'native.log').read_text(errors='replace').splitlines() if 'error:' in l]
        row['otherErrors']=[l for l in errors if l.rsplit('error:',1)[1].strip()!='window closed'];save()
    record['declaredFrameRateFailure']=len(images['frame25-10_05']['magenta'])>100 and not any(images['ref-frame25-10_05'].values()) and len(images['ref-frame25-10_15']['magenta'])>100
    record['declaredTickRateFailure']=not any(images['ticks100-12'].values()) and len(images['ref-ticks100-12']['magenta'])>100
    record.update(status='terminal',inputsUnchanged=all(sha(p)==h for p,h in inputs.items()),
                  runtimeUnchanged=all(sha(RUNTIME/p)==h for p,h in accepted['runtimeSha256'].items()))
    record['qualifiedFailures']=record['declaredFrameRateFailure'] and record['declaredTickRateFailure'] and record['inputsUnchanged'] and record['runtimeUnchanged'] and all(not r['otherErrors'] for r in record['cases'])
    save();print(json.dumps({k:record[k] for k in ['qualifiedFailures','declaredFrameRateFailure','declaredTickRateFailure']},indent=2));assert record['qualifiedFailures']


if __name__=='__main__':main()
