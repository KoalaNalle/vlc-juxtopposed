#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Extract the production MP4 sample lookup and prepare/run its C regression.

No compiler is installed or invoked implicitly. Prepare, compile the emitted C
file with a platform C11 compiler, then run this script with --run to validate
the resulting executable. A preserved old source can be an expected negative.
"""
from pathlib import Path
from hashlib import sha256
import argparse,json,shutil,subprocess

repo=Path(__file__).resolve().parents[3]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--source',type=Path,default=repo/'modules/demux/mp4/mp4.c')
p.add_argument('--output-dir',required=True,type=Path)
p.add_argument('--run',action='store_true')
p.add_argument('--expect-failure',action='store_true')
args=p.parse_args();out=args.output_dir.resolve();out.mkdir(parents=True,exist_ok=True)
def digest(path):return sha256(path.read_bytes()).hexdigest()
def write(path,v):path.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
if not args.run:
    source=args.source.resolve();text=source.read_text(encoding='utf-8')
    start=text.index('struct cmplowestchunkdts_ctx\n');end=text.index('\n/* given a time it return sample/chunk',start)
    helper=text[start:end];assert helper.count('static int STTSToSampleChunk(')==1
    (out/'mp4_seek_under_test.h').write_text(helper,encoding='utf-8',newline='\n')
    shutil.copyfile(Path(__file__).with_suffix('.c'),out/'mp4_seek_samples.c')
    write(out/'source.json',dict(source=str(source),sourceSha256=digest(source),helperSha256=digest(out/'mp4_seek_under_test.h'),harnessSha256=digest(out/'mp4_seek_samples.c'),runnerSha256=digest(Path(__file__))))
    print('Prepared production helper and regression harness:',out)
else:
    identity=json.loads((out/'source.json').read_text(encoding='utf-8'))
    assert digest(out/'mp4_seek_under_test.h')==identity['helperSha256'] and digest(out/'mp4_seek_samples.c')==identity['harnessSha256']
    exe=out/'mp4_seek_samples.exe';assert exe.is_file()
    with (out/'stdout.txt').open('wb') as stdout,(out/'stderr.txt').open('wb') as stderr:
        child=subprocess.Popen([str(exe)],stdout=stdout,stderr=stderr,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0));code=child.wait(timeout=30)
    counts=json.loads((out/'stdout.txt').read_text(encoding='utf-8'))
    passed=bool(counts['checks']==1624 and code==(1 if args.expect_failure else 0) and (counts['failures']>0 if args.expect_failure else counts['failures']==0) and (args.expect_failure or not (out/'stderr.txt').read_bytes()))
    result=dict(passed=passed,expectedNegative=args.expect_failure,exitCode=code,counts=counts,sourceIdentity=identity,exeSha256=digest(exe),stdoutSha256=digest(out/'stdout.txt'),stderrSha256=digest(out/'stderr.txt'),scope='Extracted production sample/chunk lookup with small field-compatible structs; not whole demux/parser or native playback acceptance.')
    write(out/'verification.json',result);print(json.dumps(result,indent=2));raise SystemExit(0 if passed else 1)
