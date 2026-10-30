"""Confirm packet content/timing survives the generated fixture's stream remapping."""
from hashlib import sha256
import json
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parent
repo = Path('/mnt/c/Modding/vlc-juxtopposed')
before = repo / 'build-windows/beta-fps-20261028/media-record-flac/frame-marker.mkv'
after = root / 'frame-marker.mkv'
assert sha256(before.read_bytes()).hexdigest() == 'ff9c28f52e759fcfcfd657dda5af5d3115b29d0255363b208dbbc08c9ae16a21'
def read(path):
    result = subprocess.run(['/usr/bin/ffprobe','-v','error','-show_streams','-show_packets',
        '-show_data_hash','sha256','-of','json',str(path)], capture_output=True, check=True, timeout=30)
    assert not result.stderr
    return json.loads(result.stdout)
old, new = read(before), read(after)
assert [s['codec_type'] for s in new['streams']] == ['audio','video','subtitle','subtitle']
checks = []
for kind in ('audio','video','subtitle'):
    old_streams = [s for s in old['streams'] if s['codec_type'] == kind]
    new_streams = [s for s in new['streams'] if s['codec_type'] == kind]
    assert len(old_streams) == len(new_streams)
    for ordinal, (a,b) in enumerate(zip(old_streams,new_streams)):
        a_packets = [p for p in old['packets'] if p['stream_index'] == a['index']]
        b_packets = [p for p in new['packets'] if p['stream_index'] == b['index']]
        assert a['codec_name'] == b['codec_name'] and len(a_packets) == len(b_packets)
        assert [p['data_hash'] for p in a_packets] == [p['data_hash'] for p in b_packets]
        assert all(all(abs(float(p[k])-float(q[k])) <= .000001 for k in ('pts_time','duration_time'))
            for p,q in zip(a_packets,b_packets))
        checks.append(dict(type=kind, ordinal=ordinal, packets=len(a_packets), exactPayloads=True, exactTiming=True))
result = dict(status='passed', sourceSha256=sha256(before.read_bytes()).hexdigest(),
    remappedSha256=sha256(after.read_bytes()).hexdigest(), producerSha256=sha256(Path(__file__).read_bytes()).hexdigest(),
    command=['/usr/bin/ffmpeg','-nostdin','-v','error','-threads','1','-i',str(before),
        '-map','0:a:0','-map','0:v:0','-map','0:s:0','-map','0:s:1','-c','copy',str(after)],
    streamChecks=checks)
(root / 'generation-check.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
