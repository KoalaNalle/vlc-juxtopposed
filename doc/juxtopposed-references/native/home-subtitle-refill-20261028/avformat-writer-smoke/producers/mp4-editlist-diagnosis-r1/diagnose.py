"""Read only existing generated MP4 files; distinguish edit-list cropping from lost payload."""
from pathlib import Path
import hashlib,json,subprocess,sys,time
ROOT=Path(__file__).resolve().parent
REPO=Path(r'C:\Modding\vlc-juxtopposed')
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
frozen=json.loads((ROOT/'frozen.json').read_text())
assert all(sha(REPO/p)==h for p,h in frozen['files'].items())
assert sha(__file__)==frozen['producerSha256']
out=ROOT/'result';assert not out.exists();out.mkdir()
record={'status':'running','started':time.time(),'processes':[],'cases':{},'inspectionErrors':[]}
def save():(out/'verification.json').write_text(json.dumps(record,indent=2)+'\n')
def run(tool,args,tag):
    cmd=['wsl.exe','-d','VLC-Juxtopposed-Home-20261018','--','/usr/bin/timeout','--kill-after=2s','25s',tool,*args]
    p=subprocess.run(cmd,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=35,creationflags=subprocess.CREATE_NO_WINDOW)
    record['processes'].append({'command':cmd,'exitCode':p.returncode,'stderr':p.stderr.decode(errors='replace'),'stdoutBytes':len(p.stdout)})
    assert p.returncode==0 and not p.stderr,(tag,p.returncode,p.stderr)
    return p.stdout
save();failure=None
def tools():
    return json.loads(run('/usr/bin/python3',['-c','import hashlib,json; print(json.dumps({p:hashlib.sha256(open(p,"rb").read()).hexdigest() for p in ["/usr/bin/ffmpeg","/usr/bin/ffprobe","/usr/bin/timeout"]}))'],'tool-identities'))
try:
    assert tools()==frozen['toolsSha256']
    for variant,path in frozen['variants'].items():
        media=REPO/path; linux='/mnt/c/'+media.relative_to(Path('C:/')).as_posix()
        raw=run('/usr/bin/ffprobe',['-v','error','-ignore_editlist','1','-show_format','-show_streams','-show_packets','-show_data_hash','sha256','-of','json',linux],variant+'-probe')
        (out/(variant+'-ignore-editlist-probe.json')).write_bytes(raw)
        data=json.loads(raw);vs=next(s for s in data['streams'] if s['codec_type']=='video');ats=next(s for s in data['streams'] if s['codec_type']=='audio')
        video=run('/usr/bin/ffmpeg',['-nostdin','-v','error','-ignore_editlist','1','-i',linux,'-map','0:v:0','-vsync','0','-pix_fmt','yuv420p','-threads:v','1','-f','rawvideo','pipe:1'],variant+'-video')
        audio=run('/usr/bin/ffmpeg',['-nostdin','-v','error','-ignore_editlist','1','-i',linux,'-map','0:a:0','-acodec','pcm_s16le','-f','s16le','pipe:1'],variant+'-audio')
        vp=[p for p in data['packets'] if p['stream_index']==vs['index']];ap=[p for p in data['packets'] if p['stream_index']==ats['index']]
        record['cases'][variant]={'ignoreEditlist':True,'decodedVideoFrames':len(video)//(int(vs['width'])*int(vs['height'])*3//2),
            'decodedVideoSha256':hashlib.sha256(video).hexdigest(),'decodedAudioSamples':len(audio)//(int(ats['channels'])*2),
            'decodedAudioSha256':hashlib.sha256(audio).hexdigest(),'videoPacketCount':len(vp),'audioPacketCount':len(ap),
            'firstVideo':vp[0],'lastVideo':vp[-1],'firstAudio':ap[0],'lastAudio':ap[-1],
            'videoSideData':[p.get('side_data_list') for p in vp if p.get('side_data_list')],
            'audioSideData':[p.get('side_data_list') for p in ap if p.get('side_data_list')],
            'videoStream':vs,'audioStream':ats};save()
except BaseException as e:failure=e;record['failure']=repr(e)
finally:
    record['inputsUnchanged']=all(sha(REPO/p)==h for p,h in frozen['files'].items())
    record['producerUnchanged']=sha(__file__)==frozen['producerSha256']
    record['toolsUnchanged']=tools()==frozen['toolsSha256']
    record.update(status='terminal',success=failure is None and record['inputsUnchanged'] and record['producerUnchanged'] and record['toolsUnchanged'],ended=time.time());save()
print(json.dumps({'success':record['success'],'cases':{k:{x:v[x] for x in ('decodedVideoFrames','decodedVideoSha256','decodedAudioSamples','decodedAudioSha256')} for k,v in record['cases'].items()}},indent=2))
assert record['success']
