"""Read generated callback-test outputs through installed WSL FFmpeg; no VLC job."""
import hashlib,json,subprocess
from pathlib import Path
ROOT=Path('/mnt/c/Modding/vlc-juxtopposed')
OUT=ROOT/'build-windows/beta-record-notice-20261029/callbacks-r3'
SOURCE=ROOT/'build-windows/beta-fps-20261028/media-record-flac/frame-marker.mkv'
DEST=OUT/'output-analysis-r2.json'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def run(args):
    p=subprocess.run(args,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=90)
    assert p.returncode==0 and not p.stderr,(args,p.returncode,p.stderr.decode(errors='replace'))
    return p.stdout

def analyze(path):
    streams=json.loads(run(['ffprobe','-v','error','-show_streams','-of','json',str(path)]))['streams']
    packets=json.loads(run(['ffprobe','-v','error','-show_packets','-show_data_hash','sha256','-of','json',str(path)]))['packets']
    pcm=run(['ffmpeg','-v','error','-threads','1','-i',str(path),'-map','0:a:0','-f','s16le','-'])
    frames=run(['ffmpeg','-v','error','-threads','1','-i',str(path),'-map','0:v:0','-f','framemd5','-']).decode().splitlines()
    hashes=[line.split(',')[-1].strip() for line in frames if line and not line.startswith('#')]
    subtitles=[run(['ffmpeg','-v','error','-i',str(path),'-map','0:s:'+str(i),'-c:s','copy','-f','srt','-']) for i in range(2)]
    return dict(streams=streams,packets=packets,pcm=pcm,frameHashes=hashes,subtitles=subtitles)

assert not DEST.exists()
receipt=json.loads((OUT/'verification.json').read_text())
assert receipt['status']=='terminal'
assert receipt['sourcesUnchanged'] and receipt['fallbackUnchanged'] and receipt['runtimeUnchanged'] and receipt['humanConfigUnchanged']
qualified={r['variant']:r for r in receipt['cases'] if r['mode']=='success' and r.get('passed')}
assert set(qualified)=={'control','candidate'}
report=dict(status='running',scope='complete generated H264/FLAC/two-SRT passthrough outputs from the identified current Core and FFmpeg plugin; no arbitrary-media or package acceptance',outputs={})
try:
    source=analyze(SOURCE)
    assert sha(SOURCE)=='ff9c28f52e759fcfcfd657dda5af5d3115b29d0255363b208dbbc08c9ae16a21'
    for variant,row in qualified.items():
        path=OUT/(variant+'-success/output.mkv')
        assert sha(path)==row['outputSha256']
        a=analyze(path)
        streams=lambda x:[(s['codec_type'],s['codec_name']) for s in x['streams']]
        checks=dict(streams=sorted(streams(a))==sorted(streams(source)),pcmExact=a['pcm']==source['pcm'],allVideoFrames=a['frameHashes']==source['frameHashes'],subtitleTextExact=a['subtitles']==source['subtitles'])
        mappings=[]
        for kind in ('video','audio','subtitle'):
            old_streams=[s for s in source['streams'] if s['codec_type']==kind]
            new_streams=[s for s in a['streams'] if s['codec_type']==kind]
            assert len(old_streams)==len(new_streams)
            for ordinal,(before,after) in enumerate(zip(old_streams,new_streams)):
                mappings.append(dict(kind=kind,ordinal=ordinal,sourceIndex=before['index'],outputIndex=after['index']))
                old=[p for p in source['packets'] if p['stream_index']==before['index']]
                new=[p for p in a['packets'] if p['stream_index']==after['index']]
                checks[kind+str(ordinal)]=len(old)==len(new) and all(x['data_hash']==y['data_hash'] and abs(float(x['pts_time'])-float(y['pts_time']))<=.002 and abs(float(x.get('duration_time',0))-float(y.get('duration_time',0)))<=.002 for x,y in zip(old,new))
        result=dict(streamMapping=mappings,checks=checks,passed=all(checks.values()),fileSha256=sha(path),audioPcmSha256=hashlib.sha256(a['pcm']).hexdigest(),audioBytes=len(a['pcm']),videoFrames=len(a['frameHashes']),packetCounts=[sum(p['stream_index']==i for p in a['packets']) for i in range(4)])
        report['outputs'][variant]=result
        assert result['passed'],result
    report['passed']=True
finally:
    report['status']='terminal'
    report['analyzerSha256']=sha(__file__)
    DEST.write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
