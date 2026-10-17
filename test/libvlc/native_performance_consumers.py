#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Check real Windows Performance media consumers in an owned portable runtime."""
from pathlib import Path
import argparse, hashlib, json, re, shutil
from native_http_runtime import stage_runtime, regenerate_cache, run_owned

def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ['package','video-fixture','output-dir']:
        parser.add_argument('--'+name,required=True,type=Path)
    for name in ['core','avcodec','postproc','hardware-sample']:
        parser.add_argument('--'+name,type=Path)
    parser.add_argument('--hardware-device',default='d3d11_filters')
    args=parser.parse_args();source=args.package.resolve();fixture=args.video_fixture.resolve();out=args.output_dir.resolve()
    assert not out.is_relative_to(source) and (source/'portable').is_dir()
    out.mkdir(parents=True,exist_ok=False);runtime=out/'runtime'
    selected=['libvlccore.dll','plugins/codec/libavcodec_plugin.dll','plugins/video_filter/libpostproc_plugin.dll']
    before={name:digest(source/name) for name in selected}
    stage_runtime(source,runtime)
    for name,replacement in zip(selected,[args.core,args.avcodec,args.postproc]):
        if replacement:
            (runtime/name).unlink()  # Detach only the new runtime's immutable link.
            shutil.copy2(replacement,runtime/name)
    regenerate_cache(runtime,out/'cache')
    report=dict(passed=False,fullBetaAccepted=False,hardwareMatrixAccepted=False,cases={},
        runtimeSha256={name:digest(runtime/name) for name in selected},sourceFixtureSha256=digest(fixture))
    def save():
        (out/'verification.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8',newline='\n')
    def execute(folder,media,options=(),config=None,start=None,display='dummy',frames=True):
        folder.mkdir();path=folder/'vlcrc';log=folder/'vlc.log'
        path.write_text(''.join(str(k)+'='+str(v)+'\n' for k,v in (config or {}).items()),encoding='utf-8',newline='\n')
        command=[runtime/'vlc.exe','--intf=dummy','--no-one-instance','--no-media-library','--no-metadata-network-access',
            '--no-crashdump','--no-audio','--no-spu','--vout='+display,'--no-ignore-config','--config='+str(path),
            '--run-time=1.5','--play-and-exit','--file-logging','--log-verbose=2','--verbose=2','--logfile='+str(log)]
        if start is not None:command.append('--start-time='+str(start))
        if frames:
            (folder/'frames').mkdir()
            command+=['--scene-path='+str(folder/'frames'),'--scene-width=320','--scene-height=180',
                '--scene-ratio=5','--scene-prefix=owned-frame','--scene-replace']
        command+=list(options)+[media]
        process=run_owned(command,runtime,folder,timeout=35)
        text=log.read_text(errors='replace') if log.exists() else ''
        return process,text
    save()
    try:
        # Generate a small synthetic H.264 fixture from the caller's owned video;
        # this keeps loop-filter checks on H.264 rather than only on MPEG-4 Part2.
        generated=out/'owned-h264.ts'
        chain='#transcode{vcodec=h264,venc=x264{preset=ultrafast,threads=1},acodec=none}:std{access=file,mux=ts,dst='+str(generated).replace('\\','/')+'}'
        process,text=execute(out/'generate-h264',fixture,['--sout='+chain],{'hw-dec':0,'avcodec-threads':1},frames=False)
        assert process['normalExit'] and generated.exists() and generated.stat().st_size>0
        report['generatedH264']=dict(process=process,sha256=digest(generated),bytes=generated.stat().st_size)
        specs=[]
        for index,discard in enumerate([0,8,16,32,48]):
            specs.append(dict(name='h264-loop-'+str(index),config={'avcodec-skiploopfilter':index,'avcodec-threads':[1,2,3,32,0][index]},
                expectedThreads=[1,2,3,16,None][index],discard=discard))
        specs += [dict(name='clock-and-drop-off',config={'clock-synchro':0,'clock-jitter':1234,'cr-average':17,'drop-late-frames':0},pacing=1,jitter=1234000,average=17,drop=0),
            dict(name='clock-and-drop-on',config={'clock-synchro':1,'clock-jitter':0,'cr-average':0,'drop-late-frames':1},pacing=0,jitter=0,average=0,drop=1),
            dict(name='postproc-inline',config={'video-filter':'postproc{q=4}:scene','postproc-q':1},quality=4),
            dict(name='decoder-explicit',config={'codec':'avcodec'}),dict(name='decoder-incompatible',config={'codec':'flac'}),
            dict(name='decoder-missing',config={'codec':'owned-missing-decoder'})]
        patterns={'threads':r'allowing (\d+) thread\(s\) for decoding','discard':r'loop filter discard policy: (\d+)',
            'jitter':r'clock jitter maximum: (\d+) us','average':r'clock reference averaging: (\d+)',
            'pacing':r'source clock pacing: (\d+)','drop':r'late frame dropping: (\d+)','quality':r'postprocessing quality: (\d+)'}
        for spec in specs:
            folder=out/spec['name']
            cfg={'hw-dec':0,'file-caching':300,'video-filter':'scene','clock-synchro':-1,'clock-jitter':5000,
                'cr-average':40,'drop-late-frames':1,'avcodec-threads':1}|spec['config']
            result,text=execute(folder,generated,config=cfg)
            result['decodedFrameCount']=len(list((folder/'frames').glob('*.png')))
            result['h264DecoderStarted']='codec (h264) started' in text
            result['decoderModules']=re.findall(r'using video decoder module "([^"\n]+)"',text)
            result['observed']={key:re.findall(pattern,text) for key,pattern in patterns.items()}
            # MPEG-TS can reopen its decoder after the first SPS/PPS supplies
            # the full format. Every selected decoder must still be avcodec.
            result['passed']=result['normalExit'] and result['decodedFrameCount']>0 and result['h264DecoderStarted'] and bool(result['decoderModules']) and all(name=='avcodec' for name in result['decoderModules'])
            for key,expected in [('threads',spec.get('expectedThreads')),('discard',spec.get('discard')),('pacing',spec.get('pacing')),('jitter',spec.get('jitter')),('average',spec.get('average')),('drop',spec.get('drop')),('quality',spec.get('quality'))]:
                if expected is not None:result['passed'] &= bool(result['observed'][key]) and all(str(expected)==value for value in result['observed'][key])
            if spec['name']=='h264-loop-4':result['passed'] &= bool(result['observed']['threads']) and all(1<=int(v)<=6 for v in result['observed']['threads'])
            if spec['name']=='postproc-inline':result['passed'] &= 'using video filter module "postproc"' in text
            report['cases'][spec['name']]=result;save();print(spec['name'], 'PASS' if result['passed'] else 'FAIL',flush=True)
            assert result['passed'],spec['name']
        if args.hardware_sample:
            media=args.hardware_sample.resolve();stat=(media.stat().st_size,media.stat().st_mtime_ns)
            report['mediaAndHardwareCapturesPrivate']=True
            for name,enabled,device in [('hardware-automatic',1,'any'),('hardware-selected',1,args.hardware_device),('hardware-off-preserved-device',0,args.hardware_device)]:
                folder=out/name
                result,text=execute(folder,media,config={'hw-dec':enabled,'dec-dev':device,'video-filter':'scene'},start=60,display='direct3d11')
                result['hardwareDecode']=bool(re.search(r'Using D3D11VA',text,re.I))
                result['decodedFrameCount']=len(list((folder/'frames').glob('*.png')))
                result['deviceModules']=re.findall(r'using decoder device module "([^"\n]+)"',text)
                result['direct3d11Display']='using vout display module "direct3d11"' in text
                result['sourceStatUnchanged']=stat==(media.stat().st_size,media.stat().st_mtime_ns)
                result['passed']=result['normalExit'] and result['decodedFrameCount']>0 and result['hardwareDecode']==bool(enabled) and result['direct3d11Display'] and result['sourceStatUnchanged']
                if name=='hardware-selected':result['passed'] &= device in result['deviceModules']
                report['cases'][name]=result;save();print(name,'PASS' if result['passed'] else 'FAIL',flush=True)
                assert result['passed'],name
        report['sourceRuntimeUnchanged']=before=={name:digest(source/name) for name in selected}
        report['passed']=report['sourceRuntimeUnchanged'] and all(c['passed'] for c in report['cases'].values())
        assert report['passed']
    finally:save()
    return 0
if __name__=='__main__':raise SystemExit(main())
