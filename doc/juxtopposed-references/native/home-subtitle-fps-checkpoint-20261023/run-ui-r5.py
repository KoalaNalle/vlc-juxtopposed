from pathlib import Path
from hashlib import sha256
import ctypes,json,re,shutil,subprocess,sys

work=Path(__file__).resolve().parent
repo=work.parent.parent
previous=repo/'build-windows/beta-subtitle-accessibility-20261023'
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
manifest=work/'inputs-after-r4.json'
inputs=read(manifest)
assert all(digest(repo/p)==h and digest(Path(inputs['sourceCopies'][p]))==h for p,h in inputs['files'].items())
native=read(work/'native-after-r4/verification.json')
assert native['status']=='terminal' and native['exitCode']==0
assert len(native['functions'])==10 and all(i['type']=='pass' for f in native['functions'] for i in f['incidents'])
assert (work/'build-ordinary-r5-exit.txt').read_text().strip()=='0'
base=previous/'ui-frame-marker.mp4-r3/runtime'
expected=read(work/'preparation-r4.json')['runtimeSha256']
assert all(digest(base/p)==h for p,h in expected.items())
out=work/'ui-fps-r5'
assert not out.exists()
out.mkdir()
sys.path.insert(0,str(repo/'test/libvlc'))
from native_http_runtime import stage_runtime,environment
runtime=out/'runtime'
stage_runtime(base,runtime)
gui=runtime/'plugins/gui/libqt_plugin.dll'
gui.unlink()
shutil.copyfile(work/'ordinary-r5-bin/libqt_plugin.dll',gui)
identities={p:digest(runtime/p) for p in expected}
media=work/'media-r1'
external=media/'external-fps.srt'
assert not external.exists()
external.write_text('1\n00:00:00,000 --> 00:00:19,999\nEXTERNAL FPS FIRST\n\n2\n00:00:20,000 --> 00:00:39,999\nEXTERNAL FPS MIDDLE\n\n3\n00:00:40,000 --> 00:00:59,900\nEXTERNAL FPS LAST\n',encoding='utf-8',newline='\n')
fixtures={p.name:digest(p) for p in media.iterdir() if p.is_file()}
config=out/'vlcrc'
config.write_text('metadata-network-access=0\nml-network-discovery=0\none-instance=0\nqt-updates-notif=0\nqt-auto-raise=0\nqt-remember-window-geometry=0\nqt-controls-auto-hide=0\nqt-timeline-thumbnails=0\nsub-autodetect-file=0\nsub-language=eng\nsub-track=-1\nsub-track-id=\nsub-delay=0\nsub-fps=23.976\n',encoding='utf-8',newline='\n')
ini=runtime/'portable/vlc/vlc-qt-interface.ini'
ini.parent.mkdir(parents=True,exist_ok=True)
ini.write_text('[General]\ngraphics-api=4\nfiledialog-path='+media.as_uri()+'\n[MainWindow]\ncolor-scheme-index=2\ninterface-scale=1\nplayqueue-panel-docked=true\n',encoding='utf-8',newline='\n')
command=[str(runtime/'vlc.exe'),'--intf=qt','--no-one-instance','--no-media-library',
         '--no-crashdump','--no-qt-privacy-ask','--no-qt-updates-notif',
         '--no-metadata-network-access','--no-ml-network-discovery',
         '--config='+str(config),'--file-logging','--log-verbose=2','--verbose=2',
         '--logfile='+str(out/'vlc.log'),'--restore-playback-pos=0','--start-paused',
         '--fullscreen','--sub-file='+str(external),str(media/'frame-marker.mp4')]
write(out/'launch.json',command)
record=dict(status='starting',observationsRequired=True,privateCore=False,privateGui=False,
            forcedCleanupUsed=False,fullBetaAccepted=False,sourceManifestSha256=digest(manifest),
            sourceInputCount=len(inputs['files']),producerSha256=digest(Path(__file__)),
            runtimeSha256=identities,fixtureSha256=fixtures,
            scope='Actual ordinary fractional FPS startup, real-unit accessibility and keyboard controls, alternate-editor synchronization, Reset and generated external-caption timing. Embedded captions do not qualify external FPS.')
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
        print('Owned ordinary fractional FPS GUI PID',child.pid,flush=True)
        try:record['exitCode']=child.wait(timeout=1800)
        except subprocess.TimeoutExpired:
            record['status']='observation-timeout-process-left-running'
            write(out/'verification.json',record)
            raise SystemExit('Inspect owned PID; no automatic cleanup/restart')
finally:kernel.SetErrorMode(previousMode)
shutil.copyfile(config,out/'config-after.txt')
logs='\n'.join((out/p).read_text(encoding='utf-8',errors='replace') for p in ['vlc.log','stderr.txt'])
record.update(status='terminal',normalExit=record['exitCode']==0,
              sourceInputsUnchanged=all(digest(repo/p)==h for p,h in inputs['files'].items()),
              runtimeUnchanged=all(digest(base/p)==h and digest(runtime/p)==identities[p] for p,h in expected.items()),
              fixturesUnchanged=all(digest(media/p)==h for p,h in fixtures.items()),
              diagnostics=[line for line in logs.splitlines() if re.search(r'\berror:|Assertion failed|Binding loop|ReferenceError|TypeError',line)])
write(out/'verification.json',record)
print(json.dumps({p:record[p] for p in ['status','pid','normalExit','sourceInputsUnchanged','runtimeUnchanged','fixturesUnchanged','diagnostics']}),flush=True)
