"""Collect the true paused-caption failure, timestamp trace and scoped repair."""
import difflib
import hashlib
import json
from pathlib import Path
import shutil

ROOT=Path(__file__).resolve().parents[2]
WORK=ROOT/'build-windows/beta-record-transition-20261029'
GUI=ROOT/'build-windows/beta-resume-20261029'
OUT=ROOT/'doc/juxtopposed-references/native/recorded-webvtt-paused-20261029'


def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def load(p):return json.loads(Path(p).read_text(encoding='utf-8-sig'))


def main():
    assert not OUT.exists()
    core=load(WORK/'core-build-r3/verification.json')
    assert core['sharedTreeRestored'] and core['sourcesUnchanged'] and core['fallbackUnchanged']
    assert core['candidateSha256']=='4a2865915e342744a5a7092171b92279b08f7b229eede231e00c3e86d88cff84'
    assert sha(ROOT/'src/player/input.c')=='f85d9bfa8801096b0853c25875ae9e937454bbd3588560f2a931d133bdbde20e'
    OUT.mkdir(parents=True);copied=[]

    def copy(p,name):
        q=OUT/name;q.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,q)
        assert sha(p)==sha(q);copied.append(dict(file=name,sha256=sha(q),bytes=q.stat().st_size))

    def write(name,value):
        p=OUT/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(json.dumps(value,indent=2)+'\n')

    native={};traces=[]
    for run in ['paused-selection-r2','paused-selection-r3','paused-selection-r4','paused-selection-r5','paused-selection-r6']:
        v=load(WORK/run/'verification.json')
        assert v['status']=='terminal' and v['inputsUnchanged'] and v['runtimeUnchanged']
        cases=[]
        for p in sorted((WORK/run).iterdir()):
            if not p.is_dir() or not (p/'result/result.json').exists():continue
            result=load(p/'result/result.json');assert result['teardownComplete'] and result['stopReached']
            assert 'executionError' not in result
            for file in [p/'stdout.txt',*sorted((p/'result').glob('*.png')),p/'result/result.json']:
                copy(file,run+'/'+p.name+'/'+file.name)
            lines=(p/'result/native.log').read_text(errors='replace').splitlines()
            errors=[l for l in lines if 'error:' in l]
            assert all(l.rsplit('error:',1)[1].strip() == 'window closed' for l in errors)
            traces += [dict(run=run,case=p.name,text=l) for l in lines if 'subtitle redraw trace' in l]
            cases.append(dict(case=p.name,passed=result['passed'],checks=result['checks'],
                              start=result['startTimeOption'],coreSha256=result['coreSha256'],
                              producerSha256=result['producerSha256'],teardownDiagnostics=len(errors),
                              observations=result['observations']))
        native[run]=cases
    assert not native['paused-selection-r2'][0]['passed']
    assert all(c['passed'] for n in ['paused-selection-r4','paused-selection-r6'] for c in native[n])
    assert any('ts=-39999 ' in t['text'] for t in traces)
    fractional=native['paused-selection-r6']
    covered=next(c for c in fractional if c['start']==12.019)
    assert covered['observations'][0]['frameMarker']==1048
    assert covered['observations'][1]['frameMarker']==1048 and covered['observations'][1]['captionPixels']>100
    ordinary={}
    for run in ['gui-r14','gui-r15']:
        v=load(GUI/run/'verification.json')
        assert v['status']=='terminal' and v['exitCode']==0 and v['preservationPassed'] and not v['diagnostics']
        for p in sorted((GUI/run/'observations').iterdir()):
            if p.is_file():copy(p,run+'/observations/'+p.name)
        log=(GUI/run/'vlc.log').read_text(errors='replace').splitlines()
        assert any('Using D3D11VA' in l for l in log)
        ordinary[run]=dict(exitCode=v['exitCode'],modules=v['moduleCounts'],preservationPassed=True,
                          diagnostics=v['diagnostics'],d3d11va=True,
                          runtimeSha256={k:v['runtimeSha256'][k] for k in ['libvlccore.dll','plugins/gui/libqt_plugin.dll','plugins/codec/libwebvtt_plugin.dll']})
    recording=load(WORK/'after-r3/verification.json')
    assert recording['exitCode']==0 and recording['inputsUnchanged'] and recording['runtimeUnchanged']
    assert 'RESULT cases=7 failures=0' in recording['stdout']
    copy(WORK/'after-r3/stdout.txt','recording-regression.txt')
    copy(WORK/'mp4-timing-r1/sample-tables.json','sample-tables.json')
    for name in ['verification.json','build.log','command.json']:
        copy(WORK/'core-build-r3'/name,'core-build/'+name)
    old=(WORK/'input-record-fix-before-redraw-trace.c').read_text().splitlines(keepends=True)
    new=(ROOT/'src/player/input.c').read_text().splitlines(keepends=True)
    (OUT/'source.patch').write_text(''.join(difflib.unified_diff(old,new,fromfile='a/src/player/input.c',tofile='b/src/player/input.c')))
    for name in ['inspect-recorded-webvtt-r1.py','probe-recorded-webvtt-r2.py','run-recorded-webvtt-probe-r2.py',
                 'run-recorded-webvtt-probe-r3.py','run-recorded-webvtt-probe-r4.py','run-recorded-webvtt-probe-r5.py',
                 'run-recorded-webvtt-probe-r6.py','build-recording-core-r2.py','build-recording-core-r3.py',
                 'run-recording-transition-after-r3.py','run-gui-r14.py','run-gui-r15.py']:
        copy(GUI/name,'producers/'+name)
    copy(ROOT/'test/libvlc/native_recorded_webvtt_pause.py','producers/native_recorded_webvtt_pause.py')
    copy(WORK/'paused-selection-r4/native_recorded_webvtt_pause.py','producers/native-test-before-fractional.py')
    copy(WORK/'input-redraw-trace.c','producers/diagnostic-input.c')
    copy(Path(__file__),'producers/'+Path(__file__).name)
    write('verification.json',dict(scope='Generated recorded WVTT paused startup/selection and seek; current Windows 11 Core, software snapshots and one ordinary D3D11 topology',
        coreBuild=core,ordinary=ordinary,native=native,diagnosticTraces=traces,
        recordingRegressionCases=7,recordingRegressionFailures=0,
        findings=['Stored R8 cue spans movie time 10.08 through 29.98 seconds after its empty edit.',
                  'Old normalized held video time -39999 was rejected, while time 1 passed; the diagnostic build changed logging only.',
                  'Candidate case start=12.019 retains marker 1048, also observed at file time 12 in the earlier no-offset probe; the picture lies before that start offset and now renders its cue.',
                  'R4 six positive integer-start runs did not by themselves exercise a negative normalized picture.',
                  'R5 fractional old-Core cases all passed and are positive controls, not intended negative controls.',
                  'R14 ordinary held caption stayed missing; R15 renders it with Play shown and visible elapsed zero.'],
        limitations=['Native held-time assertion is within 50000 microseconds; frame markers must match exactly.',
                     'No all-formats, FPS/delay matrix, cue-boundary/tail, broad GPU/DPI, multi-instance or package acceptance.',
                     'Earlier R1 probe sought beyond the start-offset playback interval; that coordinate error is retained locally and not counted as a program failure.']))
    write('copy-verification.json',copied);(OUT/'.gitattributes').write_text('* -text\n')
    print(json.dumps(dict(receipt=str(OUT),copiedFiles=len(copied),repairedCore=core['candidateSha256'])))


if __name__=='__main__':main()
