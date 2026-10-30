"""Strict default-presentation regression for one generated duration-repair case."""
from pathlib import Path
import argparse, hashlib, json, os, re, subprocess, sys, time
sys.dont_write_bytecode=True
from runtime_support import digest, safe_digest, manifest, manifest_matches, storage_state, stage_hardlinks
ROOT=Path(__file__).resolve().parent
REPO=Path(r'C:\Modding\vlc-juxtopposed')
parser=argparse.ArgumentParser(); parser.add_argument('--execute-frozen-smoke',action='store_true',required=True); parser.add_argument('--case',choices=('mp4','mpegts','short-matroska'),required=True)
args=parser.parse_args()
config_digest=digest(ROOT/'regression-config.json'); cfg=json.loads((ROOT/'regression-config.json').read_text())
out=ROOT/('short-'+args.case+'-r1'); assert not out.exists(); out.mkdir()
errors=[]; failure=None; runtime_inventories={}; fixtures=[cfg['shortCases'][args.case]]; base=REPO/cfg['preservedRun']/'runtime'
human={str(Path(os.environ['APPDATA'])/'vlc'/p):None for p in ('vlcrc','vlc-qt-interface.ini')}
human={p:digest(p) if Path(p).is_file() else None for p in human}
record={'status':'running','started':time.time(),'scope':'Strict generated MP4 defaultpresentation/MPEGTS/120msMatroska duration regression; not ordinaryUI/finalpackage acceptance.',
    'inspectionErrors':errors,'cases':[],'humanBeforeSha256':human}
def save():
    tmp=out/'verification.next.json'; tmp.write_text(json.dumps(record,indent=2)+'\n'); os.replace(tmp,out/'verification.json')
def linux(path): return '/mnt/c/'+Path(path).relative_to(Path('C:/')).as_posix()
def wsl(tool, argv, caseout, tag, binary=False):
    command=['wsl.exe','-d',cfg['distro'],'--','/usr/bin/timeout','--kill-after=2s','25s',tool,*argv]
    start=time.time()
    p=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=35,creationflags=subprocess.CREATE_NO_WINDOW)
    (caseout/(tag+'.stderr.txt')).write_bytes(p.stderr)
    assert p.returncode==0 and not p.stderr, (tag,p.returncode,p.stderr.decode(errors='replace'))
    info={'command':command,'exitCode':p.returncode,'elapsed':time.time()-start,'stderrBytes':len(p.stderr),'bytes':len(p.stdout),'sha256':hashlib.sha256(p.stdout).hexdigest()}
    if not binary: (caseout/(tag+'.json')).write_bytes(p.stdout)
    return p.stdout,info
def inspect(path,caseout,tag):
    raw,process=wsl('/usr/bin/ffprobe',['-v','error','-show_format','-show_streams','-show_packets','-show_data_hash','sha256','-of','json',linux(path)],caseout,tag+'-probe')
    data=json.loads(raw); streams=data['streams']; assert len(streams)==2
    video=next(x for x in streams if x['codec_type']=='video'); audio=next(x for x in streams if x['codec_type']=='audio')
    v,vprocess=wsl('/usr/bin/ffmpeg',['-nostdin','-v','error','-i',linux(path),'-map','0:v:0','-vsync','0','-pix_fmt','yuv420p','-threads:v','1','-f','rawvideo','pipe:1'],caseout,tag+'-video',True)
    a,aprocess=wsl('/usr/bin/ffmpeg',['-nostdin','-v','error','-i',linux(path),'-map','0:a:0','-acodec','pcm_s16le','-f','s16le','pipe:1'],caseout,tag+'-audio',True)
    framebytes=int(video['width'])*int(video['height'])*3//2
    assert len(v)%framebytes==0 and len(a)%(int(audio['channels'])*2)==0
    packets={kind:[{k:p.get(k) for k in ('data_hash','size','pts_time','dts_time','duration_time','flags')} for p in data['packets'] if p['stream_index']==stream['index']]
        for kind,stream in [('video',video),('audio',audio)]}
    return {'format':data['format'].get('format_name'),'duration':data['format'].get('duration'),
        'codecs':[video['codec_name'],audio['codec_name']],'videoFrames':len(v)//framebytes,
        'videoSha256':hashlib.sha256(v).hexdigest(),'audioSamples':len(a)//(int(audio['channels'])*2),
        'audioSha256':hashlib.sha256(a).hexdigest(),'audioRate':audio['sample_rate'],'audioChannels':audio['channels'],
        'packets':packets,'processes':{'probe':process,'videoDecode':vprocess,'audioDecode':aprocess}},v,a
save()
try:
    assert time.time()+600 < cfg['stopByUnix'], 'Too late for bounded run plus checkpoint'
    assert all(digest(ROOT/p)==h for p,h in cfg['producers'].items())
    assert all(digest(REPO/p)==h for p,h in cfg['sourceSha256'].items())
    assert digest(REPO/cfg['priorRuntimeConfig'])==cfg['priorRuntimeConfigSha256']
    assert digest(REPO/cfg['buildReceipt'])==cfg['buildReceiptSha256']
    build=json.loads((REPO/cfg['buildReceipt']).read_text()); assert build['success'] and build['actualLinkObserved'] and build['writerLockReleased']
    assert digest(REPO/cfg['candidatePlugin'])==cfg['candidatePluginSha256']
    assert manifest(base)==cfg['preservedRuntimeInventory']
    assert all(digest(REPO/c['fixture'])==c['fixtureSha256'] for c in fixtures)
    toolcmd=['wsl.exe','-d',cfg['distro'],'--','/usr/bin/python3','-c',
        'import hashlib,json; print(json.dumps({p:hashlib.sha256(open(p,"rb").read()).hexdigest() for p in ["/usr/bin/ffmpeg","/usr/bin/ffprobe","/usr/bin/timeout"]}))']
    assert json.loads(subprocess.check_output(toolcmd,timeout=10,creationflags=subprocess.CREATE_NO_WINDOW))==cfg['toolsSha256']
    helper_path=REPO/'test/libvlc/native_http_runtime.py'; assert digest(helper_path)==cfg['runtimeHelperSha256']
    sys.path.insert(0,str(helper_path.parent)); import native_http_runtime as helper
    oldenv=helper.environment
    def env(runtime,output):
        result=oldenv(runtime,output); result.pop('VLC_PLUGIN_PATH',None); return result
    helper.environment=env
    record['storageBeforeStage']=storage_state(REPO,cfg['shortStorageBudget']); save()
    assert record['storageBeforeStage']['fits'], 'C worst peak exceeds80%target'
    for case in fixtures:
        fixture=REPO/case['fixture']; case_summary={'name':case['name'],'fixtureSha256':case['fixtureSha256'],'variants':[]}
        record['cases'].append(case_summary); case_dir=out/case['name']; case_dir.mkdir()
        source,source_video,source_audio=inspect(fixture,case_dir,'source'); case_summary['source']=source; save()
        assert source['codecs']==case['expectedCodecs'] and source['videoFrames']==case['expectedVideoFrames']
        for variant in ('candidate',):
            assert time.time()+120<cfg['stopByUnix']; run=case_dir/variant; run.mkdir(); runtime=run/'runtime'
            item={'variant':variant,'status':'running'}; case_summary['variants'].append(item); save()
            item['stage']=stage_hardlinks(base,runtime,cfg['preservedRuntimeInventory'])
            if variant=='candidate':
                target=runtime/'plugins/codec/libavcodec_plugin.dll'; target.unlink(); os.link(REPO/cfg['candidatePlugin'],target)
            item['cache']=helper.run_owned([runtime/'vlc-cache-gen.exe',runtime/'plugins'],runtime,run/'cache',timeout=30); assert item['cache']['normalExit']
            inventory=manifest(runtime); runtime_inventories[str(runtime)]=inventory
            assert set(inventory)==set(cfg['preservedRuntimeInventory'])
            expected_plugin=cfg['candidatePluginSha256'] if variant=='candidate' else cfg['controlPluginSha256']
            assert inventory['plugins/codec/libavcodec_plugin.dll']['sha256']==expected_plugin
            assert all(entry['sha256']==cfg['preservedRuntimeInventory'][p]['sha256'] for p,entry in inventory.items()
                if p!='plugins/codec/libavcodec_plugin.dll' and Path(p).name!='plugins.dat')
            output=run/('output.'+case['outputExtension'])
            sout='#standard{access=file{no-overwrite},mux=avformat{mux='+case['mux']+'},dst='+output.as_posix()+'}'
            command=[runtime/'vlc.exe','--ignore-config','--intf=dummy','--no-one-instance','--no-media-library','--no-save-recentplay',
                '--no-metadata-network-access','--no-ml-network-discovery','--no-auto-preparse','--extraintf=','--control=',
                '--no-crashdump','--play-and-exit','--file-logging','--verbose=2','--log-verbose=2','--logfile='+str(run/'vlc.log'),
                '--sout-all','--sout-mux-caching='+str(case['muxCachingMs']),'--sout='+sout,fixture]
            item['command']=[str(x) for x in command]; save(); item['process']=helper.run_owned(command,runtime,run,timeout=cfg['runTimeoutSeconds'])
            assert item['process']['normalExit']
            log=(run/'vlc.log').read_text(errors='replace'); stderr=(run/'stderr.txt').read_text(errors='replace')
            item.update(moduleCounts=re.findall(r'plug-ins loaded: (\d+) modules',log),
                selectedLibavformat='using sout mux module "avcodec"' in log,
                diagnostics=[x for x in (log+'\n'+stderr).splitlines() if re.search(r'\berror:|Assertion failed|Binding loop|ReferenceError|TypeError',x)],
                startupEvidence=[x for x in log.splitlines() if re.search(r'caching|forcing|header|removing an output|writing trailer',x)],
                outputBytes=output.stat().st_size,outputSha256=digest(output),runtimeSha256={p:x['sha256'] for p,x in inventory.items()})
            assert item['moduleCounts']==['603'] and item['selectedLibavformat'] and not item['diagnostics']
            assert 0<item['outputBytes']<=cfg['outputMaxBytes']
            observed,video,audio=inspect(output,run,'output'); item['decoded']=observed
            item['comparisonToSource']={'videoFramesEqual':observed['videoFrames']==source['videoFrames'],
                'videoExact':video==source_video,'audioExact':audio==source_audio,
                'audioPrefix':source_audio.startswith(audio),'audioSampleDelta':observed['audioSamples']-source['audioSamples'],
                'packetCountsEqual':{k:len(observed['packets'][k])==len(source['packets'][k]) for k in ('video','audio')},
                'packetPayloadsEqual':{k:[x['data_hash'] for x in observed['packets'][k]]==[x['data_hash'] for x in source['packets'][k]] for k in ('video','audio')}}
            assert observed['codecs']==source['codecs'] and observed['videoFrames']>0 and observed['audioSamples']>0
            item['status']='terminal'; item['success']=True; save()
        b=case_summary['variants'][0]
        assert b['comparisonToSource']['videoFramesEqual'] and b['comparisonToSource']['videoExact']
        assert b['comparisonToSource']['audioExact'], 'Default presentation must preserve ALL source PCM'
        assert b['comparisonToSource']['packetCountsEqual']['audio']
        assert b['comparisonToSource']['packetPayloadsEqual']['audio']
        assert all('D' not in p['flags'] for p in b['decoded']['packets']['video']), 'Default presentation must retain every final video packet'
        case_summary['accepted']=True
        save()
except BaseException as e:
    failure=e; record['failure']=repr(e)
finally:
    try:
        record['sourceUnchanged']=all([safe_digest(REPO/p,errors)==h for p,h in cfg['sourceSha256'].items()])
        record['fixturesUnchanged']=all([safe_digest(REPO/c['fixture'],errors)==c['fixtureSha256'] for c in fixtures])
        record['producersUnchanged']=all([safe_digest(ROOT/p,errors)==h for p,h in cfg['producers'].items()])
        record['configUnchanged']=safe_digest(ROOT/'regression-config.json',errors)==config_digest
        record['parentConfigUnchanged']=safe_digest(REPO/cfg['priorRuntimeConfig'],errors)==cfg['priorRuntimeConfigSha256']
        record['buildReceiptUnchanged']=safe_digest(REPO/cfg['buildReceipt'],errors)==cfg['buildReceiptSha256']
        record['candidatePluginUnchanged']=safe_digest(REPO/cfg['candidatePlugin'],errors)==cfg['candidatePluginSha256']
        record['preservedRuntimeUnchanged']=manifest_matches(base,cfg['preservedRuntimeInventory'],errors)
        record['runtimesUnchanged']=all([manifest_matches(Path(p),v,errors) for p,v in runtime_inventories.items()])
        record['humanConfigUnchanged']=all([(safe_digest(p,errors) if Path(p).is_file() else None)==h for p,h in human.items()])
        if 'toolcmd' in globals():
            record['toolsUnchanged']=json.loads(subprocess.check_output(toolcmd,timeout=10,creationflags=subprocess.CREATE_NO_WINDOW))==cfg['toolsSha256']
    except BaseException as e: errors.append(repr(e))
    record.update(status='terminal',ended=time.time(),success=failure is None and not errors and all(v for k,v in record.items() if k.endswith('Unchanged')))
    save()
print(json.dumps({'status':record['status'],'success':record['success'],'failure':record.get('failure'),'cases':len(record['cases'])},indent=2))
assert record['success'], 'Inspect frozen writer smoke receipt'
