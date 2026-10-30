"""Stage current ordinary Qt/Core with the qualified FFmpeg candidate; isolated profile."""
import hashlib,json,os,re,shutil,subprocess,sys,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
WORK=ROOT/'build-windows/beta-record-notice-20261029'
OUT=WORK/'gui-r29'
BASE=ROOT/'build-windows/beta-resume-20261029/gui-r28'
sys.path.insert(0,str(ROOT/'test/libvlc'))
import native_http_runtime as support

def sha(p):
    with Path(p).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def load(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def main():
    assert not OUT.exists()
    assert shutil.disk_usage(ROOT).free>=399864390452+1024**3
    prior=load(BASE/'verification.json')
    assert prior['status']=='terminal' and prior['exitCode']==0 and prior['preservationPassed']
    expected=dict(prior['runtimeSha256'])
    assert all(sha(BASE/'runtime'/n)==h for n,h in expected.items())
    build=load(WORK/'plugin-build-r2/verification.json')
    assert build['success'] and build['ownedArtifactsRestored'] and build['inputsUnchanged'] and build['untouchedUnchanged']
    plugin=WORK/'plugin-build-r2/libavcodec_plugin.dll'
    assert sha(plugin)==build['pluginSha256']
    native=load(WORK/'callbacks-r3/verification.json')
    rows=[r for r in native['cases'] if r.get('passed')]
    assert len(rows)==6 and all(r['normalExit'] and r['exitCode']==0 and not r['forcedCleanupUsed'] for r in rows)
    assert load(WORK/'callbacks-r3/output-analysis-r3.json')['passed']
    cancel=load(WORK/'callbacks-r5/verification.json')
    assert cancel['passed'] and len(cancel['cases'])==2 and all(r['normalExit'] and r['exitCode']==0 and r['observation']['cancellationRequested'] for r in cancel['cases'])
    OUT.mkdir();runtime=OUT/'runtime'
    support.stage_runtime(BASE/'runtime',runtime)
    target=runtime/'plugins/codec/libavcodec_plugin.dll';target.unlink();os.link(plugin,target)
    expected['plugins/codec/libavcodec_plugin.dll']=build['pluginSha256']
    preserved={}
    for src,dest in [(BASE/'userdata',OUT/'userdata'),(BASE/'runtime/portable',runtime/'portable')]:
        for p in src.rglob('*'):
            if p.is_file():
                preserved[str(p)]=sha(p);q=dest/p.relative_to(src);q.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,q)
    config=OUT/'vlcrc';shutil.copyfile(BASE/'vlcrc',config);preserved[str(BASE/'vlcrc')]=sha(BASE/'vlcrc')
    media=OUT/'media';media.mkdir()
    sources=[ROOT/'build-windows/beta-fps-20261028/media-record-flac/frame-marker.mkv',BASE/'media/frame-marker.mp4']
    for source in sources:os.link(source,media/source.name)
    fixtures={str(p):sha(p) for p in media.iterdir()}
    sources_sha=dict(prior['sourceSha256'])
    sources_sha['modules/demux/avformat/mux.c']=sha(ROOT/'modules/demux/avformat/mux.c')
    sources_sha['modules/demux/avformat/avformat_mux_queue.h']=sha(ROOT/'modules/demux/avformat/avformat_mux_queue.h')
    assert all(sha(ROOT/n)==h for n,h in sources_sha.items())
    support.regenerate_cache(runtime,OUT/'cache')
    human={str(Path(os.environ['APPDATA'])/'vlc'/n):None for n in ['vlcrc','vlc-qt-interface.ini']}
    human={p:sha(p) if Path(p).exists() else None for p in human}
    inputs={str(p):sha(p) for p in [Path(__file__),plugin,WORK/'plugin-build-r2/verification.json',WORK/'callbacks-r3/verification.json',WORK/'callbacks-r3/output-analysis-r3.json',WORK/'callbacks-r5/verification.json',BASE/'verification.json']}
    chain='#standard{access=file{no-overwrite},mux=avformat{mux=mp4},dst='+(OUT/'incompatible-output.mp4').as_posix()+'}'
    command=[str(runtime/'vlc.exe'),'--intf=qt','--no-one-instance','--media-library','--config='+str(config),'--no-crashdump','--no-qt-privacy-ask','--no-qt-updates-notif','--no-metadata-network-access','--no-ml-network-discovery','--no-qt-video-autoresize','--no-qt-remember-window-geometry','--file-logging','--verbose=2','--log-verbose=2','--logfile='+str(OUT/'vlc.log'),'--qt-juxtopposed-controls','--no-sub-autodetect-file','--start-paused',str(media/'frame-marker.mkv'),':sout='+chain,':sout-all',':start-paused',str(media/'frame-marker.mp4'),':start-time=20.5',':start-paused']
    report=dict(status='starting',scope='ordinary FFmpeg header-error notice/details and successor playback recovery on current Qt/Core; generated profile only',runtimeSha256=expected,sourceSha256=sources_sha,inputs=inputs,fixtures=fixtures,priorProfileSha256=preserved,humanBeforeSha256=human,command=command,forcedCleanupUsed=False)
    def save():(OUT/'verification.json').write_text(json.dumps(report,indent=2)+'\n')
    env=support.environment(runtime,OUT)
    for k in ['VLC_PLUGIN_PATH','QT_PLUGIN_PATH','QML_IMPORT_PATH','QML2_IMPORT_PATH']:env.pop(k,None)
    save()
    with (OUT/'stdout.txt').open('wb') as stdout,(OUT/'stderr.txt').open('wb') as stderr:
        child=subprocess.Popen(command,cwd=runtime,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
        report.update(status='running',pid=child.pid);save();print('OWNED_GUI_RUNNING '+str(child.pid),flush=True)
        try:code=child.wait(timeout=1800)
        except subprocess.TimeoutExpired:
            report['status']='observation-timeout-process-left-running';save();raise SystemExit('Inspect exact owned PID; do not restart')
    text='\n'.join((OUT/n).read_text(errors='replace') for n in ['vlc.log','stderr.txt'])
    report.update(status='terminal',exitCode=code,normalExit=code==0,moduleCounts=re.findall(r'plug-ins loaded: (\d+) modules',text),diagnostics=[l for l in text.splitlines() if re.search(r'\berror:|Assertion failed|ReferenceError|TypeError|Binding loop',l)],sourcesUnchanged=all(sha(ROOT/n)==h for n,h in sources_sha.items()),runtimeUnchanged=all(sha(runtime/n)==h for n,h in expected.items()),fallbackUnchanged=all(sha(BASE/'runtime'/n)==h for n,h in prior['runtimeSha256'].items()),inputsUnchanged=all(sha(p)==h for p,h in inputs.items()),fixturesUnchanged=all(sha(p)==h for p,h in fixtures.items()),priorProfileUnchanged=all(sha(p)==h for p,h in preserved.items()),humanConfigUnchanged=all((sha(p) if Path(p).exists() else None)==h for p,h in human.items()))
    report['preservationPassed']=all(v for k,v in report.items() if k.endswith('Unchanged'))
    save();print(json.dumps({k:v for k,v in report.items() if k in ['status','exitCode','preservationPassed','moduleCounts','diagnostics']},indent=2))
    assert code==0 and report['preservationPassed']
if __name__=='__main__':main()
