from pathlib import Path
from hashlib import sha256
from decimal import Decimal
import json
work=Path(__file__).resolve().parent;media=work/'media-r8'
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
video=read(work/'video-packet-probes-r7/original.json')['packets'];audio=read(media/'original-audio.json')['packets']
records=[];rejected=[]
for name,expected in [('preserve','0.000000'),('plus1ms','0.001000'),('plus21ms','0.021000'),('minus1ms','-0.001000')]:
    streams=read(media/(name+'-probe.json'))['streams']
    assert [s['codec_type'] for s in streams]==['video','audio','audio','subtitle','subtitle']
    assert [s.get('tags',{}).get('language') for s in streams[1:]]==['eng','fin','eng','fin']
    row=dict(name=name,mediaSha256=digest(media/(name+'.mkv')),streams=5,packetComparisons={})
    for kind,original in [('video',video),('audio',audio)]:
        path=media/(name+'-'+kind+'.json');packets=read(path)['packets']
        assert len(packets)==len(original)
        assert [p['data_hash'] for p in packets]==[p['data_hash'] for p in original]
        missing=[i for i,p in enumerate(packets) if 'pts_time' not in p]
        deltas=sorted({str(Decimal(p['pts_time'])-Decimal(q['pts_time'])) for p,q in zip(packets,original) if 'pts_time' in p and 'pts_time' in q})
        assert deltas==[expected],(name,kind,deltas)
        row['packetComparisons'][kind]=dict(packets=len(packets),payloadsMatchOriginal=True,ptsShiftSecondsForTimestampedPackets=deltas,missingPtsPacketIndexes=missing,firstOriginalPts=original[0]['pts_time'],firstControlPts=packets[0].get('pts_time'),probeSha256=digest(path))
    if name=='minus1ms':
        assert row['packetComparisons']['video']['missingPtsPacketIndexes']==[0]
        assert not row['packetComparisons']['audio']['missingPtsPacketIndexes']
        row['rejectionReason']='First video packet has no reported PTS after muxing; this is not an exact timestamp-offset control. Retain but do not use in the matched native comparison.';rejected.append(row)
    else:
        assert all(not v['missingPtsPacketIndexes'] for v in row['packetComparisons'].values());records.append(row)
report=dict(inputsValidated=True,playbackAccepted=False,producerSha256=digest(Path(__file__)),fixtureProducerSha256=digest(work/'generate-timestamp-controls-r8.sh'),originalMediaSha256=digest(work/'media-r1/multi-language.mkv'),records=records,rejectedControls=rejected,preflight='Original comparer raised on missing minus1ms video PTS; its source is preserved as compare-timestamp-controls-r8-preflight.py. No accepted input record was overwritten.')
assert not (work/'timestamp-control-inputs-r8b.json').exists()
(work/'timestamp-control-inputs-r8b.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps(dict(inputsValidated=True,matchedControls=[r['name'] for r in records],rejected=[r['name'] for r in rejected])))
