from pathlib import Path
from hashlib import file_digest
import ctypes
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
from native_http_runtime import stage_runtime, regenerate_cache, environment
def digest(path):
    with path.open('rb') as stream: return file_digest(stream,'sha256').hexdigest()
label=sys.argv[1]
assert label.isidentifier()
out=root/label;out.mkdir(exist_ok=False)
base=repo/'build-windows/beta-fps-20261028/ui_ass_replay_av2/runtime'
baseline=json.loads((base.parent/'verification.json').read_text())['runtimeSha256']
assert all(digest(base/n)==h for n,h in baseline.items())
gui=root/'gui-r1'; core=root/'native-r5'
assert (gui/'exit.txt').read_text().strip()=='0' and (core/'exit.txt').read_text().strip()=='0'
source={line.split('  ',1)[1]:line.split('  ',1)[0] for line in (gui/'inputs.sha256').read_text().splitlines()}
# Standalone test sources are not inputs to the ordinary GUI. The build's full
# inventory remains frozen; this run guards the production dependencies only.
source={name:h for name,h in source.items() if not name.startswith('modules/gui/qt/tests/')}
assert all(digest(repo/n)==h for n,h in source.items())
runtime=out/'runtime';stage=stage_runtime(base,runtime)
for name,built,identity in [('libvlccore.dll',core/'libvlccore.dll',core/'core.sha256'),
    ('plugins/gui/libqt_plugin.dll',gui/'libqt_plugin.dll',gui/'binary.sha256')]:
    assert digest(built)==identity.read_text().split()[0]
    (runtime/name).unlink();shutil.copyfile(built,runtime/name)
with (runtime/'plugins/gui/libqt_plugin.dll').open('rb') as stream:
    tail=b''
    while chunk:=stream.read(1024*1024):
        assert b'VLC_OWNED_UI_REPORT' not in tail+chunk
        tail=chunk[-128:]
regenerate_cache(runtime,out/'cache')
media=out/'media';media.mkdir()
fixture=repo/'build-windows/beta-fps-20261028/media-formats/frame-marker.mp4'
for name in ('Continue sample.mp4','Watch Again sample.mp4','Unwatched sample.mp4'):
    shutil.copyfile(fixture,media/name)
fixtures={str(p):digest(p) for p in media.iterdir()}
config=out/'vlcrc'
config.write_text('metadata-network-access=0\nml-network-discovery=0\none-instance=0\nqt-updates-notif=0\n'
    'qt-remember-window-geometry=0\nqt-controls-auto-hide=0\nqt-timeline-thumbnails=0\n'
    'sub-autodetect-file=0\nmedia-library=1\nrestore-playback-pos=0\n',newline='\n')
ini=runtime/'portable/vlc/vlc-qt-interface.ini';ini.parent.mkdir(parents=True,exist_ok=True)
ini.write_text('[General]\ngraphics-api=4\nfiledialog-path='+media.as_uri()+'\n[MainWindow]\ncolor-scheme-index=2\ninterface-scale=1\n',newline='\n')
env=environment(runtime,out);env.pop('VLC_PLUGIN_PATH',None)
human=[Path(os.environ['APPDATA'])/'vlc'/name for name in ('vlcrc','vlc-qt-interface.ini')]
human_before={str(p):digest(p) if p.is_file() else None for p in human}
identities={n:digest(runtime/n) for n in baseline}
producer=digest(Path(__file__))
command=[str(runtime/'vlc.exe'),'--intf=qt','--no-one-instance','--media-library',
    '--config='+str(config),'--no-crashdump','--no-qt-privacy-ask','--no-qt-updates-notif',
    '--no-metadata-network-access','--no-ml-network-discovery','--no-qt-video-autoresize',
    '--no-qt-remember-window-geometry','--file-logging','--verbose=2','--log-verbose=2',
    '--logfile='+str(out/'vlc.log'),'--qt-juxtopposed-controls']
record=dict(status='starting',scope='Ordinary Home sections candidate; fresh profile and generated media. No private hooks or final-package qualification.',
    sourceSha256=source,runtimeSha256=identities,fixtureSha256=fixtures,humanBeforeSha256=human_before,
    producerSha256=producer,command=command,stage=stage,started=time.time())
def save(): (out/'verification.json').write_text(json.dumps(record,indent=2)+'\n')
save()
with (out/'stdout.txt').open('wb') as stdout,(out/'stderr.txt').open('wb') as stderr:
    child=subprocess.Popen(command,cwd=runtime,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
    record.update(status='running',pid=child.pid);save();print('Owned Home VLC PID',child.pid,flush=True)
    try: code=child.wait(timeout=1500)
    except subprocess.TimeoutExpired:
        record.update(status='timeout-process-left-running');save();raise SystemExit('Inspect owned PID; do not restart.')
text='\n'.join((out/n).read_text(errors='replace') for n in ('vlc.log','stderr.txt'))
record.update(status='terminal',exitCode=code,sourceUnchanged=all(digest(repo/n)==h for n,h in source.items()),
    runtimeUnchanged=all(digest(runtime/n)==h for n,h in identities.items()),
    baselineUnchanged=all(digest(base/n)==h for n,h in baseline.items()),
    fixtureUnchanged=all(digest(Path(n))==h for n,h in fixtures.items()),
    producerUnchanged=digest(Path(__file__))==producer,
    humanConfigUnchanged=all((digest(Path(n)) if Path(n).is_file() else None)==h for n,h in human_before.items()),
    moduleCounts=re.findall(r'plug-ins loaded: (\d+) modules',text),
    diagnostics=[line for line in text.splitlines() if re.search(r'\berror:|Assertion failed|Binding loop|ReferenceError|TypeError|is not a type|Cannot assign',line)],ended=time.time())
save();print(json.dumps({k:v for k,v in record.items() if k not in ('sourceSha256','runtimeSha256','command')},indent=2))
assert code==0 and all(record[k] for k in ('sourceUnchanged','runtimeUnchanged','baselineUnchanged','fixtureUnchanged','producerUnchanged','humanConfigUnchanged'))
assert not record['diagnostics'], 'Review ordinary diagnostics before acceptance.'
