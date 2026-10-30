"""Read actual compiler dependency files and match every input to the frozen map."""
from pathlib import Path
import hashlib,json,shlex
PREP=Path(__file__).resolve().parent;REPO=Path('/mnt/c/Modding/vlc-juxtopposed');OUT=PREP/'unit-build-r1'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
cfg=json.loads((PREP/'unit-config-r1.json').read_text());expected=dict(cfg['dependencies'])
expected.update({str(REPO/p):h for p,h in cfg['sourceSha256'].items()})
actual={};errors=[]
for depfile in (OUT/'parser.d',OUT/'baseline/test.d',OUT/'candidate/test.d'):
    text=depfile.read_text().replace('\\\n',' ');tail=text.split(':',1)[1]
    for token in shlex.split(tail):
        p=Path(token);p=p if p.is_absolute() else OUT/p;p=p.resolve()
        if str(p) not in expected:errors.append('Unfrozen dependency: '+str(p))
        else:
            actual[str(p)]=sha(p)
            if actual[str(p)]!=expected[str(p)]:errors.append('Changed dependency: '+str(p))
record={'status':'terminal','actualDependencies':actual,'count':len(actual),'errors':errors,
    'compilerUnchanged':sha(cfg['compiler'])==cfg['compilerSha256'],
    'configSha256':sha(PREP/'unit-config-r1.json'),'producerSha256':sha(__file__)}
record['success']=not errors and record['compilerUnchanged']
(PREP/'actual-dependencies.json').write_text(json.dumps(record,indent=2)+'\n')
print(json.dumps({k:record[k] for k in ('status','count','success','compilerUnchanged','errors')},indent=2));assert record['success']
