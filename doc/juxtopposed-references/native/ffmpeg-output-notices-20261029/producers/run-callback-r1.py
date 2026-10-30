"""Fresh real LibVLC error-callback comparison; no desktop or human-profile writes."""
import hashlib,json,os,re,shutil,sys,time
from pathlib import Path
ROOT=Path(r'C:\Modding\vlc-juxtopposed')
WORK=ROOT/'build-windows/beta-record-notice-20261029'
OUT=WORK/'callbacks-r1'
BASE=ROOT/'build-windows/beta-resume-20261029/gui-r28/runtime'
OLD=ROOT/'build-windows/beta-fps-20261028/tail-drain-repair-r1'
FIXTURE=ROOT/'build-windows/beta-fps-20261028/media-record-flac/frame-marker.mkv'
WORKER=ROOT/'test/libvlc/native_avformat_notice.py'
sys.dont_write_bytecode=True
sys.path.insert(0,str(ROOT/'test/libvlc'))
import native_http_runtime as support
sys.path.insert(0,str(OLD))
from runtime_support import manifest,stage_hardlinks
from pipe_runtime import run_closed_sink

def sha(p):
    with Path(p).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()

def main():
    assert not OUT.exists()
    assert shutil.disk_usage(ROOT).free >= 399864390452+2*1024**3
    build=WORK/'plugin-build-r2'
    b=json.loads((build/'verification.json').read_text())
    assert b['success'] and b['exitCode']==0 and b['ownedArtifactsRestored']
    assert b['inputsUnchanged'] and b['untouchedUnchanged'] and b['writerCleanupConfirmed']
    assert not b['forcedCleanupUsed'] and not b['restoreErrors'] and not b['errors']
    candidate=build/'libavcodec_plugin.dll'
    assert sha(candidate)==b['pluginSha256']
    control=build/'original/1-libavcodec_plugin.dll'
    assert sha(control)==b['originalPluginSha256']=='26e1b8cc539ac5299c8e821dfba36f2449e588af5f8582b4a33c7605c0508cce'
    assert sha(FIXTURE)=='ff9c28f52e759fcfcfd657dda5af5d3115b29d0255363b208dbbc08c9ae16a21'
    OUT.mkdir()
    report=dict(status='running',scope='actual LibVLC FFmpeg mux error callbacks and generated output; no ordinary Qt/record-control qualification',cases=[])
    before=manifest(BASE)
    sources={str(p):sha(p) for p in [Path(__file__),WORKER,ROOT/'modules/demux/avformat/mux.c',ROOT/'modules/demux/avformat/avformat_mux_queue.h',FIXTURE,build/'verification.json']}
    human={str(Path(os.environ['APPDATA'])/'vlc'/n):None for n in ['vlcrc','vlc-qt-interface.ini']}
    human={p:sha(p) if Path(p).is_file() else None for p in human}
    def save(): (OUT/'verification.json').write_text(json.dumps(report,indent=2)+'\n')
    error=None
    runtimes={}
    original_environment=support.environment
    def environment(runtime,out):
        e=original_environment(runtime,out);e.pop('VLC_PLUGIN_PATH',None);return e
    support.environment=environment
    try:
        for variant,plugin in [('control',control),('candidate',candidate)]:
            runtime=OUT/('runtime-'+variant)
            stage_hardlinks(BASE,runtime,before)
            target=runtime/'plugins/codec/libavcodec_plugin.dll';target.unlink();os.link(plugin,target)
            cache=OUT/('cache-'+variant)
            cache.mkdir()
            result=support.run_owned([runtime/'vlc-cache-gen.exe',runtime/'plugins'],runtime,cache)
            assert result['normalExit'],result
            runtimes[variant]=(runtime,manifest(runtime))
        save()
        for variant,mode in [('control','header-error'),('candidate','header-error'),('control','writer-error'),('candidate','writer-error'),('control','success'),('candidate','success'),('candidate','cancel')]:
            runtime=runtimes[variant][0]
            out=OUT/(variant+'-'+mode);out.mkdir()
            command=[sys.executable,str(WORKER),str(runtime),str(FIXTURE),str(out),mode]
            row=dict(variant=variant,mode=mode,started=time.time())
            report['cases'].append(row);save()
            print('CASE_STARTED '+variant+' '+mode,flush=True)
            if mode=='writer-error':
                row.update(run_closed_sink(command,runtime,out,environment(runtime,out),timeout=85))
            else:
                row.update(support.run_owned(command,runtime,out,timeout=85))
            assert row['normalExit'],row
            observation=json.loads((out/'callbacks.json').read_text())
            log=(out/'vlc.log').read_text(errors='replace')
            row['callbacks']=observation['events']
            row['noticeCount']=sum(e['title']=='Output could not be completed' for e in row['callbacks'])
            row['expectedNoticeCount']=int(variant=='candidate' and mode in ['header-error','writer-error'])
            row['selectedMux']='using sout mux module "avcodec"' in log
            row['modules']=re.findall(r'plug-ins loaded: (\d+) modules',log)
            row['diagnostics']=[l for l in log.splitlines() if re.search(r'\berror:|Assertion failed|ReferenceError|TypeError',l)]
            row['observation']=observation
            assert row['selectedMux'] and row['modules']==['603']
            assert row['noticeCount']==row['expectedNoticeCount'],row
            assert len(row['callbacks'])==row['noticeCount'],row
            if mode in ['success','cancel']:
                assert not row['diagnostics'],row
                assert (out/'output.mkv').stat().st_size>0
                row['outputSha256']=sha(out/'output.mkv')
                row['outputBytes']=(out/'output.mkv').stat().st_size
            if mode=='cancel':assert observation['cancellationRequested']
            if mode=='header-error':assert 'could not write header' in log
            if mode=='writer-error':assert any(t in log for t in ['could not write frame','could not flush buffered mux output','could not flush mux header'])
            row.update(passed=True,ended=time.time());save()
            print('CASE_PASSED '+variant+' '+mode,flush=True)
    except BaseException as exc:
        error=exc;report['failure']=repr(exc)
    finally:
        report['sourcesUnchanged']=all(sha(p)==h for p,h in sources.items())
        report['fallbackUnchanged']=manifest(BASE)==before
        report['runtimeUnchanged']=all(manifest(path)==entries for path,entries in runtimes.values())
        report['humanConfigUnchanged']=all((sha(p) if Path(p).is_file() else None)==h for p,h in human.items())
        report['inputs']=sources
        report['status']='terminal'
        report['passed']=error is None and all(report[k] for k in ['sourcesUnchanged','fallbackUnchanged','runtimeUnchanged','humanConfigUnchanged'])
        save()
    print(json.dumps({k:v for k,v in report.items() if k not in ['cases','inputs']},indent=2),flush=True)
    if error:raise error
    assert report['passed']
if __name__=='__main__':main()
