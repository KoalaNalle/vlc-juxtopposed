from pathlib import Path
from hashlib import sha256
import argparse,ctypes,json,os,re,shutil,subprocess,sys
repo=Path('C:/Modding/vlc-juxtopposed');work=Path(__file__).resolve().parent;old=repo/'build-windows/beta-tracks-subtitles-20261023'
sys.path.insert(0,str(repo/'test/libvlc'))
from native_http_runtime import stage_runtime,environment,regenerate_cache
def digest(p):return sha256(p.read_bytes()).hexdigest()
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
parser=argparse.ArgumentParser();parser.add_argument('--case',choices=['before','after','ass','legacy'],required=True);args=parser.parse_args()
phase='before' if args.case=='before' else 'after'
manifest=work/('inputs-'+phase+'-r1.json');inputs=read(manifest)['files']
assert len(inputs)>=2232 and all(digest(repo/p)==h for p,h in inputs.items())
media=old/'media-r1';probe=read(media/'multi-probe.json');plain=read(media/'plain-probe.json')
audio=[s for s in probe['streams'] if s['codec_type']=='audio'];subs=[s for s in probe['streams'] if s['codec_type']=='subtitle']
assert len(audio)==2 and [s['tags']['language'] for s in audio]==['eng','fin']
assert len(subs)==2 and [s['tags']['language'] for s in subs]==['eng','fin'] and all(s['codec_name']=='subrip' for s in subs)
assert not [s for s in plain['streams'] if s['codec_type']=='subtitle']
assert abs(float(probe['format']['duration'])-60)<.1
mediaHashes={p.name:digest(p) for p in media.iterdir() if p.is_file()}
extraMedia=old/('media-generator-smoke-r35/ass-overlap.mkv' if args.case=='ass' else 'media-generator-smoke-r35/colored-overlap.mkv');extraMediaHash=digest(extraMedia)
out=work/('ui-'+args.case+'-r1');assert not out.exists();out.mkdir();runtime=out/'runtime'
base=repo/'build-windows/beta-caption-accessibility-20261023/ordinary-ui-r4/runtime'
names=['vlc.exe','libvlc.dll','libvlccore.dll','plugins/gui/libqt_plugin.dll','plugins/audio_output/libwasapi_plugin.dll','plugins/demux/libmkv_plugin.dll']
protected={p:digest(base/p) for p in names}
assert protected['libvlccore.dll']=='000f1159b7d190d83edaba83739b97b869669ca2eec2ba97bc35d6c51f9bb9cb'
stage_runtime(base,runtime)
if phase=='after':
    gui=runtime/'plugins/gui/libqt_plugin.dll';gui.unlink();shutil.copyfile(work/'ordinary-r1-bin/libqt_plugin.dll',gui)
identities={p:digest(runtime/p) for p in names}
gui=(runtime/'plugins/gui/libqt_plugin.dll').read_bytes()
assert all(flag not in gui for flag in [b'VLC_OWNED_HOVER_TRACE',b'VLC_OWNED_UI_REPORT',b'VLC_OWNED_THUMBNAIL_MODE',b'VLC_OWNED_OBJECT_TRACE',b'VLC_OWNED_DEDUP_ACTIVATION',b'VLC_OWNED_BLOCK_UIA',b'VLC_OWNED_SCROLLBAR_REGRESSION',b'VLC_OWNED_QUEUE_FOCUS_REPORT',b'VLC_OWNED_QUEUE_DRAG_REPORT'])
env=environment(runtime,out);env.pop('VLC_PLUGIN_PATH',None);env.pop('QT_D3D_MAX_FRAME_LATENCY',None)
regenerate_cache(runtime,out/'cache')
config=out/'vlcrc';config.write_text('metadata-network-access=0\nml-network-discovery=0\none-instance=0\nqt-updates-notif=0\nqt-auto-raise=0\nqt-remember-window-geometry=0\nqt-controls-auto-hide=0\nqt-pin-controls=0\nqt-timeline-thumbnails=0\nsub-autodetect-file=0\naudio-language=eng\nsub-language=eng\naudio-track=-1\nsub-track=-1\n',encoding='utf-8',newline='\n')
ini=runtime/'portable/vlc/vlc-qt-interface.ini';ini.parent.mkdir(parents=True,exist_ok=True)
ini.write_text('[General]\ngraphics-api=4\nfiledialog-path='+media.as_uri()+'\n[MainWindow]\ncolor-scheme-index=2\ninterface-scale=1\nplayqueue-panel-docked=true\n',encoding='utf-8',newline='\n')
(out/'config-before.txt').write_bytes(config.read_bytes())
command=[str(runtime/'vlc.exe'),'--intf=qt','--no-one-instance','--no-media-library','--no-crashdump','--no-qt-privacy-ask','--no-qt-updates-notif','--no-metadata-network-access','--no-ml-network-discovery','--config='+str(config),'--file-logging','--log-verbose=2','--verbose=2','--logfile='+str(out/'vlc.log'),'--restore-playback-pos=0','--no-sub-autodetect-file','--start-paused','--fullscreen',str(extraMedia)]
if args.case=='legacy':command.insert(-1,'--no-qt-juxtopposed-controls')
command=command[:-1]+[command[-1]]
result=dict(status='starting',passed=False,observationsRequired=True,privateGui=False,forcedCleanupUsed=False,fullBetaAccepted=False,
    sourceManifest=manifest.relative_to(repo).as_posix(),sourceManifestSha256=digest(manifest),compiledSourceCount=len(inputs),runtimeSha256=identities,
    producerSha256=digest(Path(__file__)),fixtureProducerSha256=digest(repo/'test/libvlc/generate_paused_subtitle_seek.sh'),mediaSha256=mediaHashes,coloredMediaSha256=extraMediaHash,
    scope='Ordinary fullscreen redesigned/legacy bottom shading and overlapping SubRip/ASS observations. No caption-placement, exact frame PTS, screen-reader speech or full-beta acceptance.',case=args.case,baselineCompiledManifest='build-windows/beta-caption-accessibility-20261023/inputs-ordinary-r3.json')
def save():write(out/'verification.json',result)
k=ctypes.WinDLL('kernel32',use_last_error=True);k.SetErrorMode.argtypes=[ctypes.c_uint];k.SetErrorMode.restype=ctypes.c_uint;previous=k.SetErrorMode(0x8003)
try:
    with (out/'stdout.txt').open('wb') as stdout,(out/'stderr.txt').open('wb') as stderr:
        child=subprocess.Popen(command,cwd=runtime,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
        result.update(status='running',pid=child.pid);save();print('Owned caption layout PID',child.pid,flush=True)
        try:result['exitCode']=child.wait(timeout=1800)
        except subprocess.TimeoutExpired:
            result.update(status='observation-timeout-process-left-running');save();raise SystemExit('Inspect saved PID; no automatic cleanup/restart')
finally:k.SetErrorMode(previous)
logs='\n'.join((out/p).read_text(encoding='utf-8',errors='replace') for p in ['vlc.log','stderr.txt'])
diagnostics=[line for line in logs.splitlines() if re.search(r'\berror:|Assertion failed|Binding loop|ReferenceError|TypeError',line)]
result.update(status='terminal',normalExit=result['exitCode']==0,diagnostics=diagnostics,
    sourceInputsUnchanged=all(digest(repo/p)==h for p,h in inputs.items()),runtimeUnchanged=all(digest(base/p)==h and digest(runtime/p)==identities[p] for p,h in protected.items()),
    mediaUnchanged=all(digest(media/p)==h for p,h in mediaHashes.items()) and digest(extraMedia)==extraMediaHash)
(out/'config-after.txt').write_bytes(config.read_bytes());save();print(json.dumps({k:result[k] for k in ['status','case','pid','exitCode','normalExit','diagnostics','sourceInputsUnchanged','runtimeUnchanged','mediaUnchanged']}))
