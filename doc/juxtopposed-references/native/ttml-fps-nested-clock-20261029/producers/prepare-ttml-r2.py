"""Preserve the first fixture set and add the mixed-frame clock regression."""
import hashlib
import json
import os
from pathlib import Path
import shutil

ROOT=Path(__file__).resolve().parents[2]
WORK=ROOT/'build-windows/beta-ttml-fps-20261029'
OUT=WORK/'media-r2'


def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def main():
    assert not OUT.exists() and shutil.disk_usage(ROOT).free>=399864390452+64*1024**2
    previous=json.loads((WORK/'media-r1/verification.json').read_text())
    OUT.mkdir()
    for name,value in previous['files'].items():
        source=WORK/'media-r1'/name;assert sha(source)==value
        os.link(source,OUT/name)
    source=ROOT/'test/modules/demux/fixtures/ttml-fps/mixed-clock-frame.ttml'
    shutil.copyfile(source,OUT/source.name);assert sha(source)==sha(OUT/source.name)
    result=dict(generatorSha256=sha(__file__),previousManifestSha256=sha(WORK/'media-r1/verification.json'),
                mixedClockExpected=['BETA',10,20],files={p.name:sha(p) for p in sorted(OUT.iterdir())})
    (OUT/'verification.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result['files']))


if __name__=='__main__':main()
