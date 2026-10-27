from pathlib import Path
from hashlib import sha256
import ctypes,json,os,re,shutil,subprocess,sys
repo=Path('C:/Modding/vlc-juxtopposed');work=Path(__file__).resolve().parent
sys.path.insert(0,str(repo/'test/libvlc'))
from native_http_runtime import stage_runtime,environment,regenerate_cache
def digest(p):return sha256(p.read_bytes()).hexdigest()
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
manifest=work/'inputs-r4.json';inputs=read(manifest)
assert len(inputs)>=2120 and all(digest(repo/p)==h for p,h in inputs.items())
media=work/'media-r1';probe=read(media/'multi-probe.json');plain=read(media/'plain-probe.json')
audio=[s for s in probe['streams'] if s['codec_type']=='audio'];subs=[s for s in probe['streams'] if s['codec_type']=='subtitle']
assert len(audio)==2 and [s['tags']['language'] for s in audio]==['eng','fin']
assert len(subs)==2 and [s['tags']['language'] for s in subs]==['eng','fin'] and all(s['codec_name']=='subrip' for s in subs)
assert not [s for s in plain['streams'] if s['codec_type']=='subtitle']
assert abs(float(probe['format']['duration'])-60)<.1
mediaHashes={p.name:digest(p) for p in media.iterdir() if p.is_file()}
out=work/'ordinary-ui-r4';assert not out.exists();out.mkdir();runtime=out/'runtime'
base=repo/'build-windows/beta-queue-workflow-20261022/candidate-runtime-r23'
names=['vlc.exe','libvlc.dll','libvlccore.dll','plugins/gui/libqt_plugin.dll','plugins/audio_output/libwasapi_plugin.dll']
protected={p:digest(base/p) for p in names}
assert protected['libvlccore.dll']=='4422fbb02fb22857c257340d486a0310c923526d6d276127258566d3ac2671de'
stage_runtime(base,runtime)
qt=runtime/'plugins/gui/libqt_plugin.dll';qt.unlink();shutil.copyfile(work/'ordinary-r4/libqt_plugin.dll',qt)
identities={p:digest(runtime/p) for p in names}
gui=(runtime/'plugins/gui/libqt_plugin.dll').read_bytes()
assert all(flag not in gui for flag in [b'VLC_OWNED_HOVER_TRACE',b'VLC_OWNED_UI_REPORT',b'VLC_OWNED_THUMBNAIL_MODE',b'VLC_OWNED_OBJECT_TRACE',b'VLC_OWNED_DEDUP_ACTIVATION',b'VLC_OWNED_BLOCK_UIA',b'VLC_OWNED_SCROLLBAR_REGRESSION',b'VLC_OWNED_QUEUE_FOCUS_REPORT',b'VLC_OWNED_QUEUE_DRAG_REPORT'])
env=environment(runtime,out);env.pop('VLC_PLUGIN_PATH',None);env.pop('QT_D3D_MAX_FRAME_LATENCY',None)
regenerate_cache(runtime,out/'cache')
config=out/'vlcrc';config.write_text('metadata-network-access=0\nml-network-discovery=0\none-instance=0\nqt-updates-notif=0\nqt-auto-raise=0\nqt-remember-window-geometry=0\nqt-controls-auto-hide=0\nqt-timeline-thumbnails=0\nsub-autodetect-file=0\naudio-language=eng\nsub-language=eng\naudio-track=-1\nsub-track=-1\n',encoding='utf-8',newline='\n')
ini=runtime/'portable/vlc/vlc-qt-interface.ini';ini.parent.mkdir(parents=True,exist_ok=True)
ini.write_text('[General]\ngraphics-api=4\nfiledialog-path='+media.as_uri()+'\n[MainWindow]\ncolor-scheme-index=2\ninterface-scale=1\nplayqueue-panel-docked=true\n',encoding='utf-8',newline='\n')
(out/'config-before.txt').write_bytes(config.read_bytes())
command=[str(runtime/'vlc.exe'),'--intf=qt','--no-one-instance','--no-media-library','--no-crashdump','--no-qt-privacy-ask','--no-qt-updates-notif','--no-metadata-network-access','--no-ml-network-discovery','--config='+str(config),'--file-logging','--log-verbose=2','--verbose=2','--logfile='+str(out/'vlc.log'),'--restore-playback-pos=0','--no-sub-autodetect-file','--start-paused',str(media/'multi-language.mkv'),str(media/'plain.mkv')]
result=dict(status='starting',passed=False,observationsRequired=True,privateGui=False,forcedCleanupUsed=False,fullBetaAccepted=False,
    sourceManifest='build-windows/beta-tracks-subtitles-20261023/inputs-r4.json',sourceManifestSha256=digest(manifest),compiledSourceCount=len(inputs),runtimeSha256=identities,
    producerSha256=digest(Path(__file__)),fixtureProducerSha256=digest(work/'generate-fixtures-r1.sh'),mediaSha256=mediaHashes,
    scope='Ordinary Windows11 audio/subtitle track and cue workflows using generated media; no physical audibility, real-service, design/DPI or whole-beta acceptance.')
def save():write(out/'verification.json',result)
k=ctypes.WinDLL('kernel32',use_last_error=True);k.SetErrorMode.argtypes=[ctypes.c_uint];k.SetErrorMode.restype=ctypes.c_uint;previous=k.SetErrorMode(0x8003)
try:
    with (out/'stdout.txt').open('wb') as stdout,(out/'stderr.txt').open('wb') as stderr:
        child=subprocess.Popen(command,cwd=runtime,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
        result.update(status='running',pid=child.pid);save();print('Owned track/subtitle workflow PID',child.pid,flush=True)
        try:result['exitCode']=child.wait(timeout=1800)
        except subprocess.TimeoutExpired:
            result.update(status='observation-timeout-process-left-running');save();raise SystemExit('Inspect saved PID; no automatic cleanup/restart')
finally:k.SetErrorMode(previous)
logs='\n'.join((out/p).read_text(encoding='utf-8',errors='replace') for p in ['vlc.log','stderr.txt'])
diagnostics=[line for line in logs.splitlines() if re.search(r'\berror:|Assertion failed|Binding loop|ReferenceError|TypeError',line)]
result.update(status='terminal',normalExit=result['exitCode']==0,diagnostics=diagnostics,
    sourceInputsUnchanged=all(digest(repo/p)==h for p,h in inputs.items()),runtimeUnchanged=all(digest(base/p)==h and digest(runtime/p)==identities[p] for p,h in protected.items()),
    mediaUnchanged=all(digest(media/p)==h for p,h in mediaHashes.items()))
(out/'config-after.txt').write_bytes(config.read_bytes());save();print(json.dumps(result,indent=2))
