"""Actual decoder colour references before qualifying live inline FPS changes."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
from PySide6.QtGui import QImage

ROOT=Path(__file__).resolve().parents[2]
WORK=ROOT/'build-windows/beta-inline-webvtt-20261029'
OUT=WORK/'renderer-r1'
BASE=ROOT/'build-windows/beta-resume-20261029/gui-r15/runtime'
BUILD=WORK/'test-build-r1'
MEDIA=WORK/'media-r1'
sys.path.insert(0,str(ROOT/'test/libvlc'))
import native_http_runtime as support


def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def load(p):return json.loads(Path(p).read_text(encoding='utf-8-sig'))


def masks(path):
    im=QImage(str(path));assert not im.isNull() and im.size().width()==960 and im.size().height()==540
    im=im.convertToFormat(QImage.Format_RGBA8888);b=bytes(im.bits());stride=im.bytesPerLine()
    out={k:set() for k in ['green','magenta','white']}
    for y in range(270,540):
        for x in range(960):
            r,g,blue=b[y*stride+x*4:y*stride+x*4+3];p=y*960+x
            if g>150 and r<100 and blue<100:out['green'].add(p)
            if r>150 and blue>150 and g<100:out['magenta'].add(p)
            if min(r,g,blue)>160:out['white'].add(p)
    return out


def main():
    assert not OUT.exists();build=load(BUILD/'verification.json');assert build['passed']
    assert sha(BUILD/'subtitle-fps.exe')==build['binarySha256']
    assert sha(ROOT/'test/src/player/subtitle_fps.c')==build['sourceSha256']
    assert sha(BASE/'libvlccore.dll')=='4a2865915e342744a5a7092171b92279b08f7b229eede231e00c3e86d88cff84'
    OUT.mkdir();runtime=OUT/'runtime';support.stage_runtime(BASE,runtime)
    support.regenerate_cache(runtime,OUT/'cache')
    expected={str(p.relative_to(runtime)):sha(p) for p in runtime.rglob('*')
              if p.is_file() and p.name!='plugins.dat' and 'portable' not in p.relative_to(runtime).parts}
    inputs={str(p):sha(p) for p in [Path(__file__),Path(support.__file__),ROOT/'test/src/player/subtitle_fps.c',
             ROOT/'modules/demux/webvtt.c',BUILD/'subtitle-fps.exe',*sorted(MEDIA.glob('*.vtt')),MEDIA/'frame-marker-black-bottom.mp4']}
    record=dict(status='running',scope='Actual Core/native decoder colour smoke, not GUI or full WebVTT acceptance',
                inputs=inputs,runtimeSha256=expected,build=build,cases=[],forcedCleanupUsed=False)
    save=lambda:(OUT/'verification.json').write_text(json.dumps(record,indent=2)+'\n')
    save();pictures={}
    for name,subtitle,fps,mode in [('base-8','base.vtt',0,'reference'),
                                  ('reference50-8','reference-50.vtt',0,'reference'),
                                  ('inherited50-8','base.vtt',50,'inherited')]:
        run=OUT/name;run.mkdir();env=support.environment(runtime,run)
        env['VLC_TEST_PLUGIN_PATH']=str(runtime/'plugins')
        command=[str(BUILD/'subtitle-fps.exe'),str(MEDIA/'frame-marker-black-bottom.mp4'),str(MEDIA/subtitle),str(run),str(fps),mode,'8','0']
        row=dict(name=name,command=command,status='starting');record['cases'].append(row);save()
        with (run/'stdout.txt').open('wb') as stdout,(run/'stderr.txt').open('wb') as stderr:
            child=subprocess.Popen(command,cwd=runtime,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
            row.update(status='running',pid=child.pid);save();print('OWNED_INLINE_RUNNING '+str(child.pid)+' '+name,flush=True)
            try:code=child.wait(timeout=60)
            except subprocess.TimeoutExpired:
                row.update(status='observation-timeout-process-left-running');save();raise SystemExit('Inspect owned PID; do not restart')
        rows=[json.loads(l) for l in (run/'stdout.txt').read_text().splitlines() if l.startswith('{')]
        row.update(status='terminal',exitCode=code,observations=rows);save()
        assert code==0 and rows[-1].get('executionComplete')
        pictures[name]={}
        for p in sorted(run.glob('*.png')):
            m=masks(p);pictures[name][p.stem]=m
            row.setdefault('pixels',{})[p.stem]={k:len(v) for k,v in m.items()}
        save()
    a=pictures['base-8']['initial'];b=pictures['reference50-8']['initial'];c=pictures['inherited50-8']['initial']
    similarities={k:len(b[k]&c[k])/max(1,len(b[k]|c[k])) for k in ['green','magenta']}
    changed=len(a['green']^b['green'])+len(a['magenta']^b['magenta'])
    record.update(status='terminal',inputsUnchanged=all(sha(p)==h for p,h in inputs.items()),
                  runtimeUnchanged=all(sha(runtime/p)==h for p,h in expected.items()),
                  referenceChangedPixels=changed,referenceSimilarity=similarities,
                  usableReferences=min(len(a['green']),len(a['magenta']),len(b['green']),len(b['magenta']))>50 and changed>100)
    save();print(json.dumps({k:record[k] for k in ['usableReferences','referenceChangedPixels','referenceSimilarity','inputsUnchanged','runtimeUnchanged']},indent=2))
    assert record['inputsUnchanged'] and record['runtimeUnchanged']
    assert record['usableReferences'] and min(similarities.values())>.97


if __name__=='__main__':main()
