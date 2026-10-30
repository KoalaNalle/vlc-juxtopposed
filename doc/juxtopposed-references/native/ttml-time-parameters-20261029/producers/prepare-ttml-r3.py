"""Add parameter-sensitive native fixtures and independent explicit-clock references."""
from fractions import Fraction
import hashlib
import json
import os
from pathlib import Path
import shutil

ROOT=Path(__file__).resolve().parents[2]
WORK=ROOT/'build-windows/beta-ttml-fps-20261029'
OUT=WORK/'media-r3'


def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def main():
    assert not OUT.exists() and shutil.disk_usage(ROOT).free>=399864390452+64*1024**2
    old=json.loads((WORK/'media-r2/verification.json').read_text())
    OUT.mkdir()
    for name,value in old['files'].items():
        source=WORK/'media-r2'/name;assert sha(source)==value;os.link(source,OUT/name)
    negative=WORK/'parameters-before-r1'
    assert json.loads((negative/'verification.json').read_text())['qualifiedFailures']
    for source in sorted((negative/'fixtures').glob('*.ttml')):
        shutil.copyfile(source,OUT/source.name);assert sha(source)==sha(OUT/source.name)
    prefix=(OUT/'flat.ttml').read_text().split('<body>',1)[0]
    def document(parameters,begin,end):
        root=prefix.replace('<tt xmlns=', '<tt xmlns:ttp="http://www.w3.org/ns/ttml#parameter" '+parameters+' xmlns=',1)
        return root+f'<body><div><p region="caption" begin="{begin}" end="{end}" tts:color="#FF00FF">BETA</p></div></body></tt>\n'
    def seconds(value):
        micros=round(value*1_000_000)
        return f'{micros//1000000}.{micros%1000000:06}s'
    intervals={}
    for name,parameters,begin,end,first,last in [
        ('multiplier','ttp:frameRate="24" ttp:frameRateMultiplier="1000 1001"','240f','480f',Fraction('10.01'),Fraction('20.02')),
        ('subframes','ttp:frameRate="25" ttp:subFrameRate="4"','00:00:10:05.2','00:00:11:05.2',Fraction('10.22'),Fraction('11.22')),
        ('derivedticks','ttp:frameRate="25" ttp:subFrameRate="4"','1000t','2000t',Fraction(10),Fraction(20)),
        ('defaultticks','','10t','20t',Fraction(10),Fraction(20))]:
        (OUT/(name+'.ttml')).write_text(document(parameters,begin,end),newline='\n')
        for suffix,ratio in [('',Fraction(1)),('-50',Fraction(1,2))]:
            (OUT/('reference-'+name+suffix+'.ttml')).write_text(document(parameters,seconds(first*ratio),seconds(last*ratio)),newline='\n')
        intervals[name]=[str(first),str(last)]
    for name,first,last in [('frame25',Fraction('10.1'),Fraction('20.1')),('ticks100',Fraction(10),Fraction(20))]:
        (OUT/('reference-'+name+'-50.ttml')).write_text(document('',seconds(first/2),seconds(last/2)),newline='\n')
        intervals[name]=[str(first),str(last)]
    result=dict(generatorSha256=sha(__file__),expectedSourceIntervals=intervals,
                priorFixtureManifestSha256=sha(WORK/'media-r2/verification.json'),
                negativeControlSha256=sha(negative/'verification.json'),files={p.name:sha(p) for p in sorted(OUT.iterdir())})
    (OUT/'verification.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(dict(files=len(result['files']),expected=intervals)))


if __name__=='__main__':main()
