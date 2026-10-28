from pathlib import Path
from hashlib import sha256
import json
import subprocess

root = Path(__file__).resolve().parent
source = root / 'media-record-flac/frame-marker.mkv'
digest = lambda path: sha256(path.read_bytes()).hexdigest()
def command(args):
    proc = subprocess.run(args, capture_output=True, check=True)
    assert not proc.stderr, proc.stderr.decode(errors='replace')
    return proc.stdout
def probe(path):
    return json.loads(command(['ffprobe','-v','error','-show_streams','-show_packets','-of','json',str(path)]))
def audio(path):
    return command(['ffmpeg','-v','error','-threads','1','-i',str(path),'-map','0:a:0','-f','s16le','-'])
def srt(path, index):
    return command(['ffmpeg','-v','error','-i',str(path),'-map',f'0:s:{index}','-c:s','copy','-f','srt','-'])
original, original_pcm = probe(source), audio(source)
reports = {}
for variant in ('baseline','candidate'):
    run = root / ('forced_srt_matroska_clean_' + variant)
    record = json.loads((run/'verification.json').read_text())
    assert record['normalExit'] and all(record[k] for k in ('sourceUnchanged','fixtureUnchanged','producerUnchanged','runtimeUnchanged','baselineUnchanged'))
    output = run / 'output.mkv'
    assert digest(source)==record['fixtureSha256'] and digest(output)==record['outputSha256']
    muxed = probe(output)
    tracks = []
    for index in range(2):
        before=[s for s in original['streams'] if s['codec_type']=='subtitle'][index]
        after=[s for s in muxed['streams'] if s['codec_type']=='subtitle'][index]
        assert before['codec_name']==after['codec_name']=='subrip'
        source_packets=[p for p in original['packets'] if p['stream_index']==before['index']]
        new_packets=[p for p in muxed['packets'] if p['stream_index']==after['index']]
        assert len(source_packets)==len(new_packets)>0
        checks=[]
        for old,new in zip(source_packets,new_packets):
            start,duration=float(new['pts_time']),float(new.get('duration_time',0))
            checks.append(dict(start=start,duration=duration,expectedStart=float(old['pts_time']),expectedDuration=float(old['duration_time']),
                timingPassed=abs(start-float(old['pts_time']))<=.002 and abs(duration-float(old['duration_time']))<=.002))
        old_text,new_text=srt(source,index),srt(output,index)
        tracks.append(dict(index=index,events=checks,exactSrtExport=old_text==new_text,exportSha256=sha256(new_text).hexdigest()))
    video=command(['ffmpeg','-v','error','-threads','1','-i',str(output),'-map','0:v:0',
        '-vf','crop=960:40:0:0','-fps_mode','passthrough','-pix_fmt','rgb24','-f','rawvideo','-'])
    stride=960*40*3
    assert len(video)%stride==0
    markers=[sum(1<<bit for bit in range(11) if video[offset+(20*960+32+64*bit)*3]>128)
             for offset in range(0,len(video),stride)]
    pcm=audio(output)
    log=(run/'vlc.log').read_text(errors='replace')
    reports[variant]=dict(outputSha256=digest(output),avcodecSha256=record['runtimeSha256']['plugins/codec/libavcodec_plugin.dll'],
        verificationSha256=digest(run/'verification.json'),subtitles=tracks,
        fullVideoMarkers=markers==list(range(1500)),videoFrames=len(markers),
        audioSamples=len(pcm)//2,sourceAudioSamples=len(original_pcm)//2,exactAudioPcm=pcm==original_pcm,
        audioSha256=sha256(pcm).hexdigest(),sourceAudioSha256=sha256(original_pcm).hexdigest(),
        diagnostics=[s for s in log.splitlines() if ' error:' in s],
        selectedLibavformat='using sout mux module "avcodec"' in log,
        module603='plug-ins loaded: 603 modules' in log)
result=dict(producerSha256=digest(Path(__file__)),sourceSha256=digest(source),runs=reports,
    equalBaselineCandidateAudio=reports['baseline']['audioSha256']==reports['candidate']['audioSha256'],
    scope='Compare subtitle-duration repair with unchanged r7 avcodec using forced Matroska mux. Audio completeness is independently reported, not inferred from equal baseline output.')
(root/'forced-srt-comparison.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
assert all(r['fullVideoMarkers'] and r['selectedLibavformat'] and r['module603'] and not r['diagnostics'] for r in reports.values())
assert all(t['exactSrtExport'] and all(e['timingPassed'] for e in t['events']) for t in reports['candidate']['subtitles'])
assert any(not e['timingPassed'] for t in reports['baseline']['subtitles'] for e in t['events'])
