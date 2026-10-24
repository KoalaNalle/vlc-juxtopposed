from pathlib import Path
from hashlib import file_digest
import json,os,queue,shutil,sqlite3,subprocess,sys,time,wave
from threading import Thread
sys.path.insert(0,str(Path('E:/Modding/vlc-juxtopposed/test/libvlc')))
from native_http_runtime import stage_runtime as immutable_stage
def stage_runtime(source,destination,mutable):
 immutable_stage(source,destination)
 for relative in mutable:
  (destination/relative).unlink() # Only the new runtime's hardlink, before copying its replacement.

repo=Path('E:/Modding/vlc-juxtopposed')
work=repo/'build-windows/beta-network-20261017'
controls=repo/'build-windows/beta-controls-20261017'
previous=work
pcr=repo/'build-windows/beta-pcr-20261017'
visual=repo/'build-windows/beta-visual-callback-20261017'
popup=repo/'build-windows/beta-popup-20261017'
clock=repo/'build-windows/beta-live-clock-20261016'
package=repo/'build-windows/package'
performance=repo/'build-windows/beta-performance-20261017'
recording=repo/'build-windows/beta-recording-20261017'
extensions=repo/'build-windows/beta-extensions-20261017'
out=extensions/'ordinary-page-acceptance-r1'
runtime=out/'runtime'
def digest(path):
 with path.open('rb') as stream: return file_digest(stream,'sha256').hexdigest()
def run_owned(command,folder,environment,timeout):
 folder.mkdir(parents=True,exist_ok=True)
 (folder/'launch.json').write_text(json.dumps({'command':command},indent=2)+'\n')
 started=time.monotonic();timed=False
 with (folder/'stdout.txt').open('wb') as stdout,(folder/'stderr.txt').open('wb') as stderr:
  process=subprocess.Popen(command,cwd=runtime,env=environment,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
  (folder/'pid.txt').write_text(str(process.pid))
  try:code=process.wait(timeout=timeout)
  except subprocess.TimeoutExpired:timed=True;process.kill();code=process.wait(timeout=5)
 result=dict(exitCode=code,terminationConfirmed=process.poll() is not None,timedOut=timed,forcedCleanupUsed=timed,elapsedSeconds=time.monotonic()-started)
 (folder/'process.json').write_text(json.dumps(result,indent=2)+'\n')
 assert code==0 and not timed and result['terminationConfirmed'], result
 return result
out.mkdir()
selected=('vlc-cache-gen.exe','vlc.exe','libvlc.dll','libvlccore.dll','plugins/gui/libqt_plugin.dll','plugins/access_output/libaccess_output_file_plugin.dll','plugins/demux/libwav_plugin.dll','plugins/stream_out/libstream_out_transcode_plugin.dll','plugins/audio_filter/libsoxr_plugin.dll','plugins/misc/libmedialibrary_plugin.dll','plugins/control/libhotkeys_plugin.dll','plugins/lua/liblua_plugin.dll','lua/intf/http.luac','lua/intf/modules/httprequests.luac','lua/http/js/ui.js')
selected+=('plugins/codec/libavcodec_plugin.dll','plugins/video_filter/libpostproc_plugin.dll','plugins/codec/libx264_plugin.dll','plugins/demux/libmkv_plugin.dll')
http_names=['libhttp_plugin.dll','libhttps_plugin.dll','libwebdav_plugin.dll','libaccess_http_put_plugin.dll','libadaptive_plugin.dll']
source=extensions/'page-runtime-r4'
selected+=tuple(next(source.glob('plugins/**/'+name)).relative_to(source).as_posix() for name in http_names)
before={name:digest(package/name) for name in selected}
source_before={name:digest(source/name) for name in selected}
selected+=('plugins/misc/libaddonsfsstorage_plugin.dll','plugins/misc/libaddonsvorepository_plugin.dll')
source_before={name:digest(source/name) for name in selected}
before={name:digest(package/name) for name in selected}
mutable=set()
stage_runtime(source,runtime,mutable)
replacements={}
for target,origin in replacements.items():os.link(origin,runtime/target)
assert not any(p.is_file() and ('fixture' in p.name or 'probe' in p.name) for p in (runtime/'plugins').rglob('*'))
assert not list(runtime.glob('test*.exe'))
assert b'VLC_OWNED_UI_REPORT' not in (runtime/'plugins/gui/libqt_plugin.dll').read_bytes()
private=[str(path) for path in (runtime/'plugins').rglob('*') if path.is_file() and ('library_fixture' in path.name or 'winmsg_probe' in path.name)]
assert not private,private
identities={name:digest(runtime/name) for name in selected}
assert identities['plugins/misc/libmedialibrary_plugin.dll']=='e3ec5ed46c8abb561c99c45864261efb003abfa276ee9b341a50bacfba213c0a'
assert identities['plugins/audio_filter/libsoxr_plugin.dll']=='4934e6c5f3f9711fbb7dc625e8409d7f51730ecfd26db5ee7aee25e15dc3ad16'
assert identities['vlc.exe']=='3d3dbaed76083f1063b444189bdd9555a40ab3c9c610229dbaff23f3a4e58e76'
assert identities['libvlccore.dll']==digest(extensions/'page-native-r5/libvlccore.dll')
assert identities['plugins/gui/libqt_plugin.dll']==digest(extensions/'page-native-r5/libqt_plugin.dll')
assert identities['plugins/access_output/libaccess_output_file_plugin.dll']=='c5851fbd152e4065ee4a796e0caab953e8e76fa491ea99241e81d56f21cccbc6'
assert identities['plugins/demux/libwav_plugin.dll']=='541d48feda653bed962a18b3f98b3d016f5b3041077d462cd2f74baa1be7dc0b'
assert identities['plugins/stream_out/libstream_out_transcode_plugin.dll']=='b1a422d272a9486d1366a02841ce9cb9a97bdab7fe611aff24dd19fac4d5a23f'
(out/'runtime-identities.json').write_text(json.dumps(identities,indent=2)+'\n')
base_env={key:value for key,value in os.environ.items() if not key.startswith('VLC_')}
for key in ('QML_DISABLE_DISK_CACHE','QML_FORCE_DISK_CACHE','QT_QUICK_BACKEND'):base_env.pop(key,None)
base_env.update(QT_ENABLE_REGEXP_JIT='1',QML_DISK_CACHE='aot',QT_DISABLE_SHADER_DISK_CACHE='1',QT_QPA_PLATFORM='windows')
cache_env=base_env|dict(VLC_USERDATA_PATH=str(out/'cache-generator-data'))
run_owned([str(runtime/'vlc-cache-gen.exe'),str(runtime/'plugins')],out/'cache',cache_env,60)
media=out/'media';media.mkdir()
shutil.copyfile(repo/'build-windows/subtitles-generated-fixtures-20261015/plain.mp4',media/'smoke.mp4')
shutil.copyfile(repo/'build-windows/subtitles-generated-fixtures-20261015/plain.srt',media/'smoke.srt')
with wave.open(str(media/'silence.wav'),'wb') as stream:
 stream.setnchannels(2);stream.setsampwidth(2);stream.setframerate(44100);stream.writeframes(b'\0'*(44100*2*2*2))
reports={}
for name,mode in [('redesigned',None),('traditional','--no-qt-juxtopposed-controls'),('redesigned-library',None)]:
 case=out/name;case.mkdir();frames=case/'frames';frames.mkdir()
 config=case/'vlcrc';config.write_text('metadata-network-access=0\nml-network-discovery=0\none-instance=0\nqt-updates-notif=0\n')
 env=base_env|dict(VLC_USERDATA_PATH=str(case/'userdata'),QML_DISK_CACHE_PATH=str(case/'qml-cache'))
 log=case/'vlc.log'
 command=[str(runtime/'vlc.exe'),'--intf=qt','--no-one-instance',('--media-library' if name=='redesigned-library' else '--no-media-library'),'--no-crashdump','--no-qt-privacy-ask','--no-qt-updates-notif','--no-metadata-network-access','--http-proxy=direct://','--no-ml-network-discovery','--gain=0','--no-hw-dec','--run-time=3','--play-and-exit','--file-logging','--log-verbose=2','--verbose=2','--logfile='+str(log),'--config='+str(config),mode,'--video-filter=scene','--scene-path='+str(frames),'--scene-ratio=10','--scene-prefix=frame','--scene-replace',str(media/'smoke.mp4'),str(media/'silence.wav')]
 command=[arg for arg in command if arg is not None]
 result=run_owned(command,case,env,60)
 text=log.read_text(errors='replace') if log.exists() else ''
 result.update(qtLoaded='using interface module "qt"' in text,d3d11Loaded='using vout display module "direct3d11"' in text,wasapiLoaded='using aout stream module "wasapi"' in text,decodedFrames=[p.name for p in frames.glob('*.png') if p.stat().st_size>0],qmlDiagnostics=[line for line in text.splitlines() if any(term in line for term in ('ReferenceError','TypeError','Binding loop','Cannot assign','Unable to assign','unregistered datatype','is not a type','failed to load component'))])
 result.update(fileWriteErrors=sum('file error: cannot write:' in line for line in text.splitlines()),missingVisualCallbackErrors=sum("nonexistent variable 'visual'" in line for line in text.splitlines()),pcrMatchErrors=sum('Failed to match transcode input' in line for line in text.splitlines()))
 assert not result['fileWriteErrors'] and not result['missingVisualCallbackErrors'] and not result['pcrMatchErrors']
 result['newClockPropertyErrors']=[line for line in result['qmlDiagnostics'] if 'displayTime' in line or 'Player.live' in line]
 (case/'verification.json').write_text(json.dumps(result,indent=2)+'\n');reports[name]=result
 print(name,'normal quit, decoded video/audio, no QML errors',flush=True)
 assert result['qtLoaded'] and result['d3d11Loaded'] and result['wasapiLoaded'] and result['decodedFrames'] and not result['qmlDiagnostics']
# Run the same local live input through the ordinary Qt interface.
case=out/'live';case.mkdir()
fixtures=clock/'fixtures-r6'
records=[];duration=None
for line in (fixtures/'generated.m3u8').read_text().splitlines():
 if line.startswith('#EXTINF:'):duration=line
 elif line and not line.startswith('#'):
  assert duration is not None
  records.append(duration+'\n'+line+'\n');duration=None
playlist=case/'live.m3u8'
playlist.write_text('#EXTM3U\n#EXT-X-VERSION:3\n#EXT-X-TARGETDURATION:2\n#EXT-X-MEDIA-SEQUENCE:0\n'+''.join(records[:12]))
server_log=(case/'server-stderr.txt').open('wb')
server=subprocess.Popen([sys.executable,str(fixtures/'native_timeline_server.py'),str(fixtures),str(playlist)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=server_log,creationflags=subprocess.CREATE_NO_WINDOW)
(case/'server-pid.txt').write_text(str(server.pid))
lines=queue.Queue()
Thread(target=lambda:lines.put(server.stdout.readline()),daemon=True).start()
try:
 greeting=lines.get(timeout=10).decode().strip()
 assert greeting.startswith('PORT '),greeting
 port=int(greeting.split()[1])
 config=case/'vlcrc';config.write_text('metadata-network-access=0\nml-network-discovery=0\none-instance=0\nqt-updates-notif=0\n')
 env=base_env|dict(VLC_USERDATA_PATH=str(case/'userdata'),QML_DISK_CACHE_PATH=str(case/'qml-cache'))
 log=case/'vlc.log'
 command=[str(runtime/'vlc.exe'),'--intf=qt','--no-one-instance','--no-media-library','--no-crashdump','--no-qt-privacy-ask','--no-qt-updates-notif','--no-metadata-network-access','--http-proxy=direct://','--no-ml-network-discovery','--gain=0','--no-hw-dec','--run-time=3','--play-and-exit','--file-logging','--log-verbose=2','--verbose=2','--logfile='+str(log),'--config='+str(config),'--qt-juxtopposed-controls',f'http://127.0.0.1:{port}/live.m3u8']
 command=[arg for arg in command if arg is not None]
 result=run_owned(command,case,env,60)
 text=log.read_text(errors='replace') if log.exists() else ''
 result.update(qtLoaded='using interface module "qt"' in text,adaptiveLoaded='using demux module "adaptive"' in text,audioDecoderLoaded='using audio decoder module' in text,qmlDiagnostics=[line for line in text.splitlines() if any(term in line for term in ('ReferenceError','TypeError','Binding loop','Cannot assign','Unable to assign','unregistered datatype','is not a type','failed to load component'))])
 result.update(fileWriteErrors=sum('file error: cannot write:' in line for line in text.splitlines()),missingVisualCallbackErrors=sum("nonexistent variable 'visual'" in line for line in text.splitlines()),pcrMatchErrors=sum('Failed to match transcode input' in line for line in text.splitlines()))
 assert not result['fileWriteErrors'] and not result['missingVisualCallbackErrors'] and not result['pcrMatchErrors']
 result['newClockPropertyErrors']=[line for line in result['qmlDiagnostics'] if 'displayTime' in line or 'Player.live' in line]
 (case/'verification.json').write_text(json.dumps(result,indent=2)+'\n');reports['live']=result
 print('live: normal quit, adaptive/audio decoding, no QML errors',flush=True)
 assert result['qtLoaded'] and result['adaptiveLoaded'] and result['audioDecoderLoaded'] and not result['qmlDiagnostics']
finally:
 if server.poll() is None:
  server.stdin.write(b'stop\n');server.stdin.flush();server.stdin.close()
 try:server_code=server.wait(timeout=5)
 except subprocess.TimeoutExpired:
  (case/'server-stop-failure.json').write_text(json.dumps({'pid':server.pid,'normalExit':False}))
  raise
 server_log.close()
 (case/'server-exit.json').write_text(json.dumps({'exitCode':server_code,'normalExit':server_code==0})+'\n')
 assert server_code==0
# One bounded read-only playback from the user-offered Downloads directory.
media_candidates=sorted((p for p in Path('E:/Downloads').iterdir() if p.is_file() and p.suffix.lower() in ['.mkv','.mp4','.webm']),key=lambda p:p.stat().st_size)
assert media_candidates
sample=media_candidates[0].resolve();assert sample.is_relative_to(Path('E:/Downloads'))
original_stat=(sample.stat().st_size,sample.stat().st_mtime_ns)
case=out/'downloads-sample';case.mkdir();frames=case/'frames';frames.mkdir()
config=case/'vlcrc';config.write_text('metadata-network-access=0\nml-network-discovery=0\none-instance=0\nqt-updates-notif=0\n')
env=base_env|dict(VLC_USERDATA_PATH=str(case/'userdata'),QML_DISK_CACHE_PATH=str(case/'qml-cache'))
log=case/'vlc.log'
command=[str(runtime/'vlc.exe'),'--intf=qt','--no-one-instance','--no-media-library','--no-crashdump','--no-qt-privacy-ask','--no-qt-updates-notif','--no-metadata-network-access','--http-proxy=direct://','--no-ml-network-discovery','--no-audio','--hw-dec','--start-time=60','--run-time=2','--play-and-exit','--file-logging','--log-verbose=2','--verbose=2','--logfile='+str(log),'--config='+str(config),'--video-filter=scene','--scene-path='+str(frames),'--scene-width=320','--scene-height=180','--scene-ratio=10','--scene-prefix=private-frame','--scene-replace',str(sample)]
result=run_owned(command,case,env,60)
text=log.read_text(errors='replace')
result.update(qtLoaded='using interface module "qt"' in text,d3d11Loaded='using vout display module "direct3d11"' in text,decodedFrameCount=len(list(frames.glob('*.png'))),sourceSizeBytes=sample.stat().st_size,sourceStatUnchanged=original_stat==(sample.stat().st_size,sample.stat().st_mtime_ns),hardwareDecoderMessages=[line for line in text.splitlines() if 'D3D11VA' in line],mediaAndFramePrivate=True)
assert result['qtLoaded'] and result['d3d11Loaded'] and result['decodedFrameCount']>0 and result['sourceStatUnchanged'],result
reports['downloads-sample']=result
(case/'verification.json').write_text(json.dumps(result,indent=2)+'\n')
print('Bounded Downloads sample: normal quit, decoded frame, source stat unchanged',flush=True)

# This is an ordinary GUI/resource smoke, not native UI visual or DPI acceptance.
report=dict(passed=True,qtRegexpJit='1',runtimeSha256=identities,sourceRuntimeUnchanged=source_before=={name:digest(source/name) for name in selected},cases=reports,ordinaryPackageUnchanged=before=={name:digest(package/name) for name in selected},privateFixtureModulesShipped=False,fullVisualDpiOrBetaAcceptance=False)
(out/'verification.json').write_text(json.dumps(report,indent=2)+'\n')
assert report['ordinaryPackageUnchanged']
assert source_before=={name:digest(source/name) for name in selected}
print('Ordinary finite playback checks complete',flush=True)
