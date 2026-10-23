from pathlib import Path
from hashlib import sha256
import argparse,ctypes,json,re,shutil,subprocess,sys

work=Path(__file__).resolve().parent
repo=work.parent.parent
previous=repo/'build-windows/beta-subtitle-delay-repair-20261023'
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
def write(p,value):p.write_text(json.dumps(value,indent=2)+'\n',encoding='utf-8',newline='\n')
parser=argparse.ArgumentParser()
parser.add_argument('--media',choices=['frame-marker-ass.mkv','frame-marker.mp4'],required=True)
args=parser.parse_args()
manifest=work/'inputs-r3.json'
inputs=read(manifest)
assert all(digest(repo/p)==h and digest(Path(inputs['sourceCopies'][p]))==h for p,h in inputs['files'].items())
assert read(work/'component-r3/verification.json')['passed']
assert (work/'build-ordinary-r3-exit.txt').read_text().strip()=='0'
final=read(previous/'final-verification-r6.json')
base=previous/'ui-frame-marker-ass.mkv-r4/runtime'
assert all(digest(base/p)==h for p,h in final['runtimeSha256'].items())
out=work/('ui-'+args.media+'-r3')
assert not out.exists()
out.mkdir()
sys.path.insert(0,str(repo/'test/libvlc'))
from native_http_runtime import stage_runtime,environment
runtime=out/'runtime'
stage_runtime(base,runtime)
# Staged files may be hardlinks to the accepted base. Replace the link first.
gui=runtime/'plugins/gui/libqt_plugin.dll'
gui.unlink()
shutil.copyfile(work/'ordinary-r3-bin/libqt_plugin.dll',gui)
identities={p:digest(runtime/p) for p in final['runtimeSha256']}
assert identities['libvlccore.dll']==final['runtimeSha256']['libvlccore.dll']
media=repo/'build-windows/beta-external-subtitle-acceptance-20261023/media-r1'
fixtures={p.name:digest(p) for p in media.iterdir() if p.is_file()}
config=out/'vlcrc'
config.write_text('metadata-network-access=0\nml-network-discovery=0\none-instance=0\nqt-updates-notif=0\nqt-auto-raise=0\nqt-remember-window-geometry=0\nqt-controls-auto-hide=0\nqt-timeline-thumbnails=0\nsub-autodetect-file=0\nsub-language=fin\nsub-track=-1\nsub-track-id=\nsub-delay=12\n',encoding='utf-8',newline='\n')
ini=runtime/'portable/vlc/vlc-qt-interface.ini'
ini.parent.mkdir(parents=True,exist_ok=True)
ini.write_text('[General]\ngraphics-api=4\nfiledialog-path='+media.as_uri()+'\n[MainWindow]\ncolor-scheme-index=2\ninterface-scale=1\nplayqueue-panel-docked=true\n',encoding='utf-8',newline='\n')
command=[str(runtime/'vlc.exe'),'--intf=qt','--no-one-instance','--no-media-library',
         '--no-crashdump','--no-qt-privacy-ask','--no-qt-updates-notif',
         '--no-metadata-network-access','--no-ml-network-discovery',
         '--config='+str(config),'--file-logging','--log-verbose=2','--verbose=2',
         '--logfile='+str(out/'vlc.log'),'--restore-playback-pos=0','--start-paused',
         '--fullscreen',str(media/args.media)]
write(out/'launch.json',command)
record=dict(status='starting',observationsRequired=True,privateCore=False,privateGui=False,
            forcedCleanupUsed=False,fullBetaAccepted=False,media=args.media,runtimeSha256=identities,
            sourceManifestSha256=digest(manifest),sourceInputCount=len(inputs['files']),
            producerSha256=digest(Path(__file__)),fixtureSha256=fixtures,
            scope='Actual ordinary Windows accessibility writes to named subtitle timing controls, caption/reset and next-media help. Existing ordinary Core unchanged.')
kernel=ctypes.WinDLL('kernel32',use_last_error=True)
kernel.SetErrorMode.argtypes=[ctypes.c_uint]
kernel.SetErrorMode.restype=ctypes.c_uint
previousMode=kernel.SetErrorMode(0x8003)
env=environment(runtime,out)
env.pop('VLC_PLUGIN_PATH',None)
try:
    with (out/'stdout.txt').open('wb') as stdout,(out/'stderr.txt').open('wb') as stderr:
        child=subprocess.Popen(command,cwd=runtime,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
        record.update(status='running',pid=child.pid)
        write(out/'verification.json',record)
        print('Owned ordinary subtitle accessibility GUI PID',child.pid,flush=True)
        try:record['exitCode']=child.wait(timeout=1800)
        except subprocess.TimeoutExpired:
            record.update(status='observation-timeout-process-left-running')
            write(out/'verification.json',record)
            raise SystemExit('Inspect saved PID; no automatic cleanup/restart')
finally:kernel.SetErrorMode(previousMode)
shutil.copyfile(config,out/'config-after.txt')
logs='\n'.join((out/p).read_text(encoding='utf-8',errors='replace') for p in ['vlc.log','stderr.txt'])
record.update(status='terminal',normalExit=record['exitCode']==0,
              sourceInputsUnchanged=all(digest(repo/p)==h for p,h in inputs['files'].items()),
              runtimeUnchanged=all(digest(base/p)==h and digest(runtime/p)==identities[p] for p,h in final['runtimeSha256'].items()),
              fixturesUnchanged=all(digest(media/p)==h for p,h in fixtures.items()),
              diagnostics=[line for line in logs.splitlines() if re.search(r'\berror:|Assertion failed|Binding loop|ReferenceError|TypeError',line)])
write(out/'verification.json',record)
print(json.dumps({p:record[p] for p in ['status','pid','normalExit','sourceInputsUnchanged','runtimeUnchanged','fixturesUnchanged','diagnostics']}),flush=True)
