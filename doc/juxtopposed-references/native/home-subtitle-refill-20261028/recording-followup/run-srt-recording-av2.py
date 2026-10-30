from pathlib import Path
from hashlib import file_digest
import ctypes,json,os,shutil,subprocess,sys,time,xml.etree.ElementTree as ET
repo=Path(__file__).resolve().parents[2]
work=repo/'build-windows/beta-follow-20261027'
sys.path.insert(0,str(repo/'test/libvlc'))
from native_http_runtime import stage_runtime,regenerate_cache,environment
def digest(p):
    with Path(p).open('rb') as f:return file_digest(f,'sha256').hexdigest()
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))

if len(sys.argv)!=4: raise SystemExit('arguments: baseline|candidate selector label')
variant,selector,label=sys.argv[1:]
assert variant in ('baseline','candidate') and selector.replace(':','').replace('-','').isidentifier() and label.isidentifier()
manifest_path=work/'runtime-r7-manifest.json'
manifest=read(manifest_path)
base=work/'runtime-r7'
source=repo/'modules/gui/qt/tests/test_player_subtitle_delay.cpp'
build=repo/'build-windows/beta-fps-20261028/test-r23'
candidate=repo/'build-windows/beta-fps-20261028/core-r5/libvlccore.dll'
candidate_sha='f8fc980bbe03da57908f79045f266710dd27a16c6ee66b6c45d60cd1f9b93598'
assert digest(candidate)==candidate_sha
exe=build/'test_qt_player_subtitle_delay.exe'
assert (build/'exit.txt').read_text().strip()=='0'
assert digest(source)==(build/'source.sha256').read_text().split()[0]
assert digest(exe)==(build/'binary.sha256').read_text().split()[0]
assert all(digest(base/n)==v['sha256'] for n,v in manifest['files'].items())
media=repo/'build-windows/beta-fps-20261028/media-formats'
fixture={n:digest(media/n) for n in ['frame-marker.mp4','subtitle-style.srt','duration-cues.srt','subtitle-style.ass']}
initial_source=digest(source); initial_exe=digest(exe); producer=digest(Path(__file__))
out=repo/'build-windows/beta-fps-20261028'/label; out.mkdir(exist_ok=False)
runtime=out/'runtime'; stage=stage_runtime(base,runtime)
if variant=='candidate':
    (runtime/'libvlccore.dll').unlink()
    shutil.copyfile(candidate,runtime/'libvlccore.dll')
    assert digest(runtime/'libvlccore.dll')==candidate_sha
os.link(exe,runtime/exe.name)
plugin=repo/'build-windows/beta-fps-20261028/ass-plugin-r2/liblibass_plugin.dll'
plugin_sha='b622871c205345373343a463f193271b12cf875853e7c913c707acadb16e9b40'
assert digest(plugin)==plugin_sha
(runtime/'plugins/codec/liblibass_plugin.dll').unlink()
shutil.copyfile(plugin,runtime/'plugins/codec/liblibass_plugin.dll')
subtitle_plugin=repo/'build-windows/beta-fps-20261028/subtitle-plugin-r1/libsubtitle_plugin.dll'
subtitle_plugin_sha='1e8abb4593f82b5badf9dc9d103f655d4a0f30334c28f89e24b102b301139ea8'
assert digest(subtitle_plugin)==subtitle_plugin_sha
(runtime/'plugins/demux/libsubtitle_plugin.dll').unlink()
shutil.copyfile(subtitle_plugin,runtime/'plugins/demux/libsubtitle_plugin.dll')
avbuild=repo/'build-windows/beta-fps-20261028/avcodec-plugin-r2'
assert (avbuild/'exit.txt').read_text().strip()=='0'
avplugin=avbuild/'libavcodec_plugin.dll'
avplugin_sha=(avbuild/'binary.sha256').read_text().split()[0]
assert digest(avplugin)==avplugin_sha
muxsource=repo/'modules/demux/avformat/mux.c'
muxsource_sha=digest(muxsource)
assert muxsource_sha+'  modules/demux/avformat/mux.c' in (avbuild/'inputs.sha256').read_text().splitlines()
(runtime/'plugins/codec/libavcodec_plugin.dll').unlink()
shutil.copyfile(avplugin,runtime/'plugins/codec/libavcodec_plugin.dll')
regenerate_cache(runtime,out/'cache')
snaps=out/'snapshots'; snaps.mkdir()
env=environment(runtime,out);env.pop('VLC_PLUGIN_PATH',None)
for k in ['QT_PLUGIN_PATH','QML_IMPORT_PATH','QML2_IMPORT_PATH']:env.pop(k,None)
env.update(VLC_TEST_PLUGIN_PATH=str(runtime/'plugins'),VLC_SUBTITLE_FIXTURE=str(media/'frame-marker.mp4'),VLC_FPS_SNAPSHOTS=str(snaps),VLC_TEST_NATIVE_D3D11='1',QT_QPA_PLATFORM='offscreen',QT_QPA_FONTDIR=str(repo/'modules/gui/qt/pixmaps/juxtopposed'))
env['VLC_TEST_FIRST_FPS_FRESH']='1'
env['VLC_TEST_PRIMARY_SUBTITLE']='subtitle-style.srt'



if selector=='firstFpsChangePreservesNonseekableDelayedCaption': env['VLC_TEST_NONSEEK_SUBTITLE']='1'
if selector=='inheritedFpsDisplaysFirstAssociatedCue': env['VLC_TEST_INITIAL_SUBTITLE_FPS']='50'

env['VLC_TEST_PRIMARY_SUBTITLE']='subtitle-style.srt'

recordings=out/'recordings'; recordings.mkdir()
env['VLC_FPS_RECORDINGS']=str(recordings)

command=[str(runtime/exe.name),selector,'-o',str(out/'result.xml')+',xml','-o',str(out/'result.txt')+',txt']
record=dict(status='starting',sourceSha256=initial_source,testSha256=initial_exe,producerSha256=producer,fixtureSha256=fixture,manifestSha256=digest(manifest_path),stage=stage,selector=command[1],variant=variant,candidateSha256=candidate_sha,libassSha256=plugin_sha,subtitlePluginSha256=subtitle_plugin_sha,avcodecPluginSha256=avplugin_sha,muxSourceSha256=muxsource_sha,nativeVout='direct3d11',started=time.time())
def save(): (out/'verification.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8')
save()
kernel=ctypes.WinDLL('kernel32',use_last_error=True);kernel.SetErrorMode.argtypes=[ctypes.c_uint];kernel.SetErrorMode.restype=ctypes.c_uint;previous=kernel.SetErrorMode(0x8003)
try:
    with (out/'stdout.txt').open('wb') as stdout,(out/'stderr.txt').open('wb') as stderr:
        child=subprocess.Popen(command,cwd=runtime,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
        record.update(status='running',pid=child.pid);save();print('Native fresh FPS PID',child.pid,flush=True)
        try:code=child.wait(timeout=180)
        except subprocess.TimeoutExpired:
            record.update(status='observation-timeout-process-left-running');save();raise SystemExit('Inspect owned PID; do not restart.')
finally:kernel.SetErrorMode(previous)
record.update(status='terminal',exitCode=code,ended=time.time(),sourceUnchanged=digest(source)==initial_source and digest(muxsource)==muxsource_sha,testUnchanged=digest(exe)==initial_exe and digest(runtime/exe.name)==initial_exe,fixturesUnchanged=all(digest(media/n)==h for n,h in fixture.items()),runtimeUnchanged=all(digest(base/n)==v['sha256'] and digest(runtime/n)==(avplugin_sha if n=='plugins/codec/libavcodec_plugin.dll' else subtitle_plugin_sha if n=='plugins/demux/libsubtitle_plugin.dll' else plugin_sha if n=='plugins/codec/liblibass_plugin.dll' else candidate_sha if variant=='candidate' and n=='libvlccore.dll' else v['sha256']) for n,v in manifest['files'].items() if not n.endswith('plugins.dat')) and digest(candidate)==candidate_sha and digest(plugin)==plugin_sha and digest(subtitle_plugin)==subtitle_plugin_sha and digest(avplugin)==avplugin_sha,producerUnchanged=digest(Path(__file__))==producer)
if (out/'result.xml').exists():
    tree=ET.parse(out/'result.xml').getroot()
    record['functions']=[dict(name=f.get('name'),incidents=[dict(type=i.get('type'),description=i.findtext('Description','')) for i in f.findall('Incident')]) for f in tree.findall('TestFunction')]
save();print(json.dumps(record,indent=2),flush=True)
