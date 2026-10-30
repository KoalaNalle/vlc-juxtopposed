"""Generate a black caption area and independently timed inline-colour references."""
from pathlib import Path
from fractions import Fraction
import hashlib
import json
import shutil
import subprocess

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'build-windows/beta-inline-webvtt-20261029/media-r1'
SOURCE=ROOT/'build-windows/beta-fps-webvtt-20261028/player-candidate-held-r2-20261029/media/frame-marker.mp4'


def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def main():
    assert not OUT.exists() and shutil.disk_usage(ROOT).free>=399864390452+128*1024**2
    OUT.mkdir(parents=True)
    inputs={str(p):sha(p) for p in [SOURCE,Path(__file__)]}
    target=OUT/'frame-marker-black-bottom.mp4'
    linux=lambda p:'/mnt/c/'+p.relative_to(Path('C:/')).as_posix()
    cmd=['wsl','-d','VLC-Juxtopposed-Home-20261018','--','ffmpeg','-nostdin','-v','error','-n',
         '-i',linux(SOURCE),'-map','0:v:0','-map','0:a:0','-vf','drawbox=x=0:y=ih/2:w=iw:h=ih/2:color=black:t=fill',
         '-filter_threads','1','-c:v','libx264','-preset','ultrafast','-crf','18','-threads','2',
         '-c:a','copy','-movflags','+faststart',linux(target)]
    with (OUT/'video.log').open('w') as f:
        code=subprocess.run(cmd,stdout=f,stderr=subprocess.STDOUT,timeout=120).returncode
    assert code==0

    def stamp(seconds):
        ms=round(seconds*1000)
        return f'{ms//3600000:02}:{ms//60000%60:02}:{ms//1000%60:02}.{ms%1000:03}'

    css='WEBVTT\n\nSTYLE\n::cue(:past) { color: lime; }\n::cue(:future) { color: magenta; }\n\n'
    for name,ratio in [('base',Fraction(1)),('reference-50',Fraction(1,2)),
                       ('reference-29_97',Fraction(25)/Fraction('29.97')),
                       ('reference-23_976',Fraction(25)/Fraction('23.976'))]:
        body=css+f'00:00:00.000 --> {stamp(50*ratio)} line:80% position:50% align:center\n'
        body+=f'ALPHA <{stamp(10*ratio)}>BETA <{stamp(20*ratio)}>GAMMA\n'
        (OUT/(name+'.vtt')).write_text(body,newline='\n')
    assert all(sha(p)==h for p,h in inputs.items())
    record=dict(inputs=inputs,inputsUnchanged=True,videoCommand=cmd,
                files={p.name:sha(p) for p in OUT.iterdir() if p.suffix in ['.mp4','.vtt']},
                scope='Generated A/V and independent explicit timestamp references, not renderer acceptance')
    (OUT/'verification.json').write_text(json.dumps(record,indent=2)+'\n');print(json.dumps(record['files']),flush=True)


if __name__=='__main__':main()
