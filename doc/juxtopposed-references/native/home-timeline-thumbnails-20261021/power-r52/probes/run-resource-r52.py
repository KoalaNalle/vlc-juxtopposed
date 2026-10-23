from pathlib import Path
from hashlib import sha256
import argparse,json,os,platform,re,shutil,socket,subprocess,sys,time
repo=Path('C:/Modding/vlc-juxtopposed');work=Path(__file__).resolve().parent
sys.path.insert(0,str(repo/'test/libvlc'))
from native_http_runtime import stage_runtime,environment,regenerate_cache
from resource_metrics_r45 import sample_process

def digest(p):return sha256(p.read_bytes()).hexdigest()
def read(p):return json.loads(p.read_text(encoding='utf-8'))
parser=argparse.ArgumentParser();parser.add_argument('--trial',choices=['A1','B1'],required=True);args=parser.parse_args()
workaround=args.trial.startswith('B')
inputs=read(work/'inputs-ordinary-r52.json')
def unchanged():return all(digest(repo/p)==h for p,h in inputs.items())
assert unchanged()
out=work/('ordinary-resource-'+args.trial+'-r52');out.mkdir();runtime=out/'runtime'
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
video=work/'resource-fixtures-r45/motion-1080p50-silent.mp4'
command=[str(runtime/'vlc.exe'),'--intf=qt','--extraintf=rc','--rc-quiet','--rc-host=127.0.0.1:'+str(port),'--no-one-instance','--no-media-library','--no-crashdump','--no-qt-privacy-ask','--no-qt-updates-notif','--no-metadata-network-access','--no-ml-network-discovery','--config='+str(config),'--file-logging','--log-verbose=2','--verbose=2','--logfile='+str(out/'vlc.log'),'--no-qt-video-autoresize','--restore-playback-pos=0','--start-paused','--vout=direct3d11,none',str(video)]
result=dict(trial=args.trial,workaround=workaround,frameLatencyEnvironment=env.get('QT_D3D_MAX_FRAME_LATENCY'),os=platform.platform(),privateFixture=False,syntheticMedia=True,thumbnailJourneyHooksAbsent=True,fullBetaAccepted=False,responsivenessAccepted=False,forcedCleanupUsed=False,status='starting',passed=False,runnerSha256=digest(Path(__file__)),sourceInputsSha256=digest(work/'inputs-ordinary-r52.json'),mediaSha256=digest(video),identities={p:digest(runtime/p) for p in ['vlc.exe','libvlc.dll','libvlccore.dll','plugins/gui/libqt_plugin.dll','plugins/demux/libmp4_plugin.dll','plugins/control/librc_plugin.dll']},phases={})
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
    result.update(status='awaiting-owned-window-inspection',pid=child.pid,loopbackPort=port);save();print('Ordinary resource comparison',args.trial,'PID',child.pid,'awaiting inspection',flush=True)
    try:
        deadline=time.monotonic()+600
        while not (out/'continue').exists():
            assert child.poll() is None,'Owned VLC exited before measurement'
            assert time.monotonic()<deadline,'Inspection gate expired; owned process left running'
            time.sleep(.1)
        deadline=time.monotonic()+15
        while sock is None:
            try:sock=socket.create_connection(('127.0.0.1',port),timeout=.2)
            except OSError:
                assert time.monotonic()<deadline,'Owned loopback CLI unavailable'
                time.sleep(.1)
        sock.settimeout(.1)
        try:wire+=sock.recv(65536).decode(errors='replace')
        except socket.timeout:pass
        send('pause');time.sleep(5)
        result['status']='measuring-playing';save()
        before=stats();metrics=sample_process(child,25,out/'playing-metrics.json');after=stats()
        result['phases']['playing']=dict(before=before,after=after,metrics='playing-metrics.json',durationSeconds=after['wall']-before['wall']);save()
        send('pause');time.sleep(2)
        result['status']='measuring-paused';save()
        before=stats();timeBefore=send('get_time');metrics=sample_process(child,12,out/'paused-metrics.json');after=stats();timeAfter=send('get_time')
        result['phases']['paused']=dict(before=before,after=after,timeBefore=timeBefore,timeAfter=timeAfter,metrics='paused-metrics.json',durationSeconds=after['wall']-before['wall']);save()
        result['status']='awaiting-paused-preview-inspection';save();print('Baseline metrics complete',args.trial,'; primary held PAUSED for picture inspection',flush=True)
        deadline=time.monotonic()+600
        while not (out/'measure-preview').exists():
            assert child.poll() is None,'Owned VLC exited before preview measurement'
            assert time.monotonic()<deadline,'Preview inspection gate expired; owned process left running'
            time.sleep(.1)
        observation=read(out/'preview-inspection.json')
        assert observation['picturePreviewObserved'] and observation['ordinaryApp']
        result['previewInspection']=observation;send('pause');result['status']='warming-playing-preview';save();time.sleep(5);result['status']='measuring-playing-preview';save()
        before=stats();sample_process(child,20,out/'playing-preview-metrics.json');after=stats()
        result['phases']['playing-preview']=dict(before=before,after=after,metrics='playing-preview-metrics.json',durationSeconds=after['wall']-before['wall']);save()
        send('pause');time.sleep(5);result['status']='measuring-paused-preview';save()
        before=stats();sample_process(child,25,out/'paused-preview-metrics.json');after=stats()
        result['phases']['paused-preview']=dict(before=before,after=after,metrics='paused-preview-metrics.json',durationSeconds=after['wall']-before['wall']);save()
        result['status']='awaiting-paused-final-inspection';save();print('Preview measurements complete',args.trial,'; primary held PAUSED; inspect, resume, observe motion, and close normally',flush=True)
        result['exitCode']=child.wait(timeout=600)
    except Exception as error:
        result['error']=str(error);result['status']='measurement-incomplete-process-left-running';save();print('Measurement incomplete; owned process left available:',error,flush=True)
        raise
    finally:
        if sock:sock.close()
result.update(status='terminal',normalExit=result['exitCode']==0,sourceInputsUnchanged=unchanged(),identitiesUnchanged=all(digest(runtime/p)==h for p,h in result['identities'].items()))
logs='\n'.join((out/name).read_text(encoding='utf-8',errors='replace') for name in ['vlc.log','stderr.txt'])
result['diagnostics']=[line for line in logs.splitlines() if re.search(r'\berror:|OWNED_QT_DIAG\|(?:warning|critical|fatal)\||qt (?:warning|critical|fatal):|Assertion failed|Binding loop|ReferenceError|TypeError',line)]
result['realD3D11Opened']='using vout display module "direct3d11"' in logs
result['hardwareDecoderEvidence']=[line for line in logs.splitlines() if re.search(r'Using .* for hardware decoding|using hw decoder module|using hardware decoder|Using D3D11VA|D3D11VA',line,re.I)]
tokens={'wait':'application quit waiting for primary playback to stop','stopped':'primary playback stopped; quitting application','teardown':'removing "interface" module "qt"'}
lines=logs.splitlines();positions={k:next((i for i,line in enumerate(lines) if token in line),None) for k,token in tokens.items()};result['quitLogOrder']=positions
result['passed']=bool(result['normalExit'] and result['sourceInputsUnchanged'] and result['identitiesUnchanged'] and not result['diagnostics'] and result['realD3D11Opened'] and len(result['phases'])==4 and all(v is not None for v in positions.values()) and positions['wait']<positions['stopped']<positions['teardown'])
result['scope']='Ordinary app resource/statistics measurement. Native displayed/lost frame counters cover throughput; they do not prove every presentation interval, tear-free motion or supported Windows/renderer compatibility. NVIDIA power covers whole board and other applications. No production workaround promotion.'
save();print(json.dumps({k:result[k] for k in ['trial','passed','normalExit','diagnostics','hardwareDecoderEvidence']},indent=2));assert result['passed']
