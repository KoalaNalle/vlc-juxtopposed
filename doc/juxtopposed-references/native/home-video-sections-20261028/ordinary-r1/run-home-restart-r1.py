from pathlib import Path
from hashlib import file_digest
import json
import os
import re
import shutil
import subprocess
import sys
import time

root=Path(__file__).resolve().parent
repo=root.parents[1]
sys.path.insert(0,str(repo/'test/libvlc'))
from native_http_runtime import stage_runtime,regenerate_cache,environment
def digest(path):
    with Path(path).open('rb') as stream: return file_digest(stream,'sha256').hexdigest()
previous=root/'ordinary_home_r1'
old=json.loads((previous/'verification.json').read_text())
closed=json.loads((previous/'observed-exit.json').read_text())
assert closed['exitCode']==0 and all(v for k,v in closed.items() if k.endswith('Unchanged'))
out=root/'ordinary_home_restart_r1';out.mkdir(exist_ok=False)
base=previous/'runtime'; runtime=out/'runtime'
assert all(digest(base/n)==h for n,h in old['runtimeSha256'].items())
assert all(digest(repo/n)==h for n,h in old['sourceSha256'].items())
stage=stage_runtime(base,runtime)
preserved={}
for name,source in [('userdata',previous/'userdata'),('portable',base/'portable')]:
    target=runtime/'portable' if name=='portable' else out/name
    for path in source.rglob('*'):
        if path.is_file():
            relative=path.relative_to(source)
            preserved[str(path)]=digest(path)
            destination=target/relative;destination.parent.mkdir(parents=True,exist_ok=True)
            shutil.copyfile(path,destination)
regenerate_cache(runtime,out/'cache')
config=out/'vlcrc'
config.write_text((previous/'vlcrc').read_text().replace('restore-playback-pos=0','restore-playback-pos=2'),newline='\n')
env=environment(runtime,out);env.pop('VLC_PLUGIN_PATH',None)
identities={n:digest(runtime/n) for n in old['runtimeSha256']}
producer=digest(__file__)
command=[str(runtime/'vlc.exe'),'--intf=qt','--no-one-instance','--media-library',
    '--config='+str(config),'--no-crashdump','--no-qt-privacy-ask','--no-qt-updates-notif',
    '--no-metadata-network-access','--no-ml-network-discovery','--no-qt-video-autoresize',
    '--no-qt-remember-window-geometry','--file-logging','--verbose=2','--log-verbose=2',
    '--logfile='+str(out/'vlc.log'),'--qt-juxtopposed-controls']
human={str(Path(os.environ['APPDATA'])/'vlc'/n):h for n,h in
    [(Path(k).name,v) for k,v in old['humanBeforeSha256'].items()]}
record=dict(status='starting',scope='Ordinary cold restart of a copied generated-media Home profile; general resume Always.',
    sourceSha256=old['sourceSha256'],runtimeSha256=identities,priorProfileSha256=preserved,
    fixtureSha256=old['fixtureSha256'],humanBeforeSha256=human,producerSha256=producer,
    stage=stage,command=command,started=time.time())
def save(): (out/'verification.json').write_text(json.dumps(record,indent=2)+'\n')
save()
with (out/'stdout.txt').open('wb') as stdout,(out/'stderr.txt').open('wb') as stderr:
    child=subprocess.Popen(command,cwd=runtime,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
    record.update(status='running',pid=child.pid);save();print('Owned restart PID',child.pid,flush=True)
    try: code=child.wait(timeout=1500)
    except subprocess.TimeoutExpired:
        record.update(status='timeout-process-left-running');save();raise SystemExit('Inspect owned PID; do not restart.')
text='\n'.join((out/n).read_text(errors='replace') for n in ('vlc.log','stderr.txt'))
record.update(status='terminal',exitCode=code,ended=time.time(),
    sourceUnchanged=all(digest(repo/n)==h for n,h in old['sourceSha256'].items()),
    runtimeUnchanged=all(digest(runtime/n)==h for n,h in identities.items()),
    baselineUnchanged=all(digest(base/n)==h for n,h in old['runtimeSha256'].items()),
    priorProfileUnchanged=all(digest(n)==h for n,h in preserved.items()),
    fixtureUnchanged=all(digest(n)==h for n,h in old['fixtureSha256'].items()),
    producerUnchanged=digest(__file__)==producer,
    humanConfigUnchanged=all((digest(n) if Path(n).is_file() else None)==h for n,h in human.items()),
    moduleCounts=re.findall(r'plug-ins loaded: (\d+) modules',text),
    diagnostics=[line for line in text.splitlines() if re.search(r'\berror:|Assertion failed|Binding loop|ReferenceError|TypeError|is not a type|Cannot assign',line)])
save();print(json.dumps({k:v for k,v in record.items() if k.endswith('Unchanged') or k in ('status','exitCode','diagnostics','moduleCounts')},indent=2))
assert code==0 and all(v for k,v in record.items() if k.endswith('Unchanged'))
assert not record['diagnostics']
