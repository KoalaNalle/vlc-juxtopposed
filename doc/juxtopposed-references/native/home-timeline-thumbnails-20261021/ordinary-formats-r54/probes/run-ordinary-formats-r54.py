from pathlib import Path
from hashlib import sha256
import argparse,json,os,platform,re,shutil,socket,subprocess,sys,time
repo=Path('C:/Modding/vlc-juxtopposed');work=Path(__file__).resolve().parent
sys.path.insert(0,str(repo/'test/libvlc'))
from native_http_runtime import stage_runtime,environment,regenerate_cache
from resource_metrics_r45 import sample_process

def digest(p):return sha256(p.read_bytes()).hexdigest()
def read(p):return json.loads(p.read_text(encoding='utf-8'))
parser=argparse.ArgumentParser();parser.add_argument('--trial',choices=['hevc','vp9'],required=True);args=parser.parse_args()
workaround=False
inputs=read(work/'inputs-ordinary-formats-r54.json')
def unchanged():return all(digest(repo/p)==h for p,h in inputs.items())
assert unchanged()
out=work/('ordinary-formats-'+args.trial+'-r54');out.mkdir();runtime=out/'runtime'
stage_runtime(repo/'build-windows/beta-precise-seek-20261021/ordinary-r8/runtime',runtime)
for name,source in [('libvlccore.dll',work/'native-core-r49/libvlccore.dll'),('plugins/gui/libqt_plugin.dll',work/'ordinary-r38/libqt_plugin.dll')]:
    target=runtime/name;target.unlink();shutil.copyfile(source,target)
gui=(runtime/'plugins/gui/libqt_plugin.dll').read_bytes()
assert b'VLC_OWNED_UI_REPORT' not in gui and b'VLC_OWNED_THUMBNAIL_MODE' not in gui
env=environment(runtime,out);env.pop('VLC_PLUGIN_PATH',None)
env.pop('QT_D3D_MAX_FRAME_LATENCY',None)
if workaround:env['QT_D3D_MAX_FRAME_LATENCY']='0'
env.update(QT_FORCE_STDERR_LOGGING='1',QT_MESSAGE_PATTERN='OWNED_QT_DIAG|%{type}|%{message}')
config=out/'vlcrc';config.write_text('metadata-network-access=0\nml-network-discovery=0\none-instance=0\nqt-updates-notif=0\nqt-auto-raise=0\nqt-remember-window-geometry=0\nqt-controls-auto-hide=0\nqt-timeline-thumbnails=1\n',encoding='utf-8',newline='\n')
qt=runtime/'portable/vlc/vlc-qt-interface.ini';qt.parent.mkdir(parents=True,exist_ok=True)
qt.write_text('[General]\ngraphics-api=4\n[MainWindow]\ncolor-scheme-index=2\ninterface-scale=1\n',encoding='utf-8')
regenerate_cache(runtime,out/'cache')
with socket.socket() as reserve:reserve.bind(('127.0.0.1',0));port=reserve.getsockname()[1]
video=work/'format-fixtures-r53'/dict(hevc='hevc-multitrack.mkv',vp9='vp9.webm')[args.trial]
command=[str(runtime/'vlc.exe'),'--intf=qt','--extraintf=rc','--rc-quiet','--rc-host=127.0.0.1:'+str(port),'--no-one-instance','--no-media-library','--no-crashdump','--no-qt-privacy-ask','--no-qt-updates-notif','--no-metadata-network-access','--no-ml-network-discovery','--config='+str(config),'--file-logging','--log-verbose=2','--verbose=2','--logfile='+str(out/'vlc.log'),'--no-qt-video-autoresize','--restore-playback-pos=0','--start-paused','--vout=direct3d11,none',str(video)]
result=dict(trial=args.trial,workaround=workaround,frameLatencyEnvironment=env.get('QT_D3D_MAX_FRAME_LATENCY'),os=platform.platform(),privateFixture=False,syntheticMedia=True,thumbnailJourneyHooksAbsent=True,fullBetaAccepted=False,responsivenessAccepted=False,forcedCleanupUsed=False,status='starting',passed=False,runnerSha256=digest(Path(__file__)),sourceInputsSha256=digest(work/'inputs-ordinary-formats-r54.json'),mediaSha256=digest(video),identities={p:digest(runtime/p) for p in ['vlc.exe','libvlc.dll','libvlccore.dll','plugins/gui/libqt_plugin.dll','plugins/demux/libmp4_plugin.dll','plugins/control/librc_plugin.dll']},phases={})
def save():
    temp=out/'verification.pending.json';temp.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8');os.replace(temp,out/'verification.json')
sock=None;wire=''
def send(command,marker=None):
    global wire
    sock.sendall((command+'\n').encode());data=b'';end=time.monotonic()+3
    while time.monotonic()<end:
        try:
            chunk=sock.recv(65536)
            if not chunk:raise RuntimeError('Owned CLI connection closed')
            data+=chunk
            if marker and marker.encode() in data:break
        except socket.timeout:
            if data and not marker:break
    text=data.decode('utf-8',errors='replace');wire+='\nCOMMAND '+command+'\n'+text
    (out/'cli-observations.txt').write_text(wire,encoding='utf-8')
    if marker:assert marker in text,'Native CLI response incomplete: '+command
    return text
def stats():
    text=send('stats','[ end of statistical info ]')
    fields=['video decoded','frames displayed','frames late','frames lost','audio decoded','buffers played','buffers lost']
    result={}
    for name in fields:
        match=re.search(re.escape(name)+r'\s*:\s*(\d+)',text)
        assert match,'Missing native statistic '+name
        result[name]=int(match[1])
    result['wall']=time.perf_counter()
    return result
with (out/'stdout.txt').open('wb') as stdout,(out/'stderr.txt').open('wb') as stderr:
    child=subprocess.Popen(command,cwd=runtime,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
    result.update(status='awaiting-paused-picture-inspection',pid=child.pid,loopbackPort=port);save();print('Ordinary codec case',args.trial,'PID',child.pid,'awaiting PAUSED picture inspection',flush=True)
    try:
        deadline=time.monotonic()+600
        while not (out/'continue').exists():
            assert child.poll() is None,'Owned VLC exited before initial picture inspection'
            assert time.monotonic()<deadline,'Inspection expired; process left running'
            time.sleep(.1)
        result['previewInspection']=read(out/'preview-inspection.json')
        assert result['previewInspection']['picturePreviewObserved'] and result['previewInspection']['ordinaryApp']
        deadline=time.monotonic()+15
        while sock is None:
            try:sock=socket.create_connection(('127.0.0.1',port),timeout=.2)
            except OSError:
                assert time.monotonic()<deadline,'Owned loopback CLI unavailable'
                time.sleep(.1)
        sock.settimeout(.1)
        try:wire+=sock.recv(65536).decode(errors='replace')
        except socket.timeout:pass
        result['nativeTimeBeforeReposition']=send('get_time')
        send('seek 2');time.sleep(1)
        result['nativeTimeAfterReposition']=send('get_time');result['pausedStats']=stats()
        result['status']='awaiting-independent-paused-check-resume-and-quit';save()
        print('Primary repositioned to2s; inspect main picture versus preview8s, resume, observe advancement and Quit normally',flush=True)
        result['exitCode']=child.wait(timeout=600)
    except Exception as error:
        result.update(error=str(error),status='incomplete-owned-process-left-running');save();raise
    finally:
        if sock:sock.close()
result.update(status='terminal',normalExit=result['exitCode']==0,sourceInputsUnchanged=unchanged(),identitiesUnchanged=all(digest(runtime/p)==h for p,h in result['identities'].items()))
result['finalInspection']=read(out/'final-inspection.json')
logs='\n'.join((out/name).read_text(encoding='utf-8',errors='replace') for name in ['vlc.log','stderr.txt'])
result['diagnostics']=[line for line in logs.splitlines() if re.search(r'\berror:|OWNED_QT_DIAG\|(?:warning|critical|fatal)\||qt (?:warning|critical|fatal):|Assertion failed|Binding loop|ReferenceError|TypeError',line)]
result['realD3D11Opened']='using vout display module "direct3d11"' in logs
result['hardwareDecoderEvidence']=[line for line in logs.splitlines() if re.search(r'Using .* for hardware decoding|using hw decoder module|using hardware decoder|Using D3D11VA|D3D11VA',line,re.I)]
tokens={'wait':'application quit waiting for primary playback to stop','stopped':'primary playback stopped; quitting application','teardown':'removing "interface" module "qt"'}
lines=logs.splitlines();positions={k:next((i for i,line in enumerate(lines) if token in line),None) for k,token in tokens.items()};result['quitLogOrder']=positions
result['passed']=bool(result['normalExit'] and result['sourceInputsUnchanged'] and result['identitiesUnchanged'] and not result['diagnostics'] and result['realD3D11Opened'] and result['finalInspection']['independentPictureObserved'] and result['finalInspection']['continuedPlaybackObserved'] and all(v is not None for v in positions.values()) and positions['wait']<positions['stopped']<positions['teardown'])
result['scope']='Selected ordinary Windows11/D3D11VA codec preview independence and normal Quit. No full track UI, resource-cost, latency, HDR/live/network or full beta acceptance.'
save();print(json.dumps({k:result[k] for k in ['trial','passed','normalExit','diagnostics','hardwareDecoderEvidence']},indent=2));assert result['passed']
