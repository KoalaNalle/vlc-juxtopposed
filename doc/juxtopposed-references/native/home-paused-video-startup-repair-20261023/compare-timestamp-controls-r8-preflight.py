from pathlib import Path
from hashlib import sha256
from decimal import Decimal
import json
work=Path(__file__).resolve().parent;media=work/'media-r8'
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
video=read(work/'video-packet-probes-r7/original.json')['packets'];audio=read(media/'original-audio.json')['packets']
records=[]
for name,expected in [('preserve','0.000000'),('plus1ms','0.001000'),('plus21ms','0.021000'),('minus1ms','-0.001000')]:
    probe=read(media/(name+'-probe.json'));streams=probe['streams']
    assert [s['codec_type'] for s in streams]==['video','audio','audio','subtitle','subtitle']
    assert [s.get('tags',{}).get('language') for s in streams[1:]]==['eng','fin','eng','fin']
    row=dict(name=name,mediaSha256=digest(media/(name+'.mkv')),streams=5,packetComparisons={})
    for kind,original in [('video',video),('audio',audio)]:
        path=media/(name+'-'+kind+'.json');packets=read(path)['packets']
        assert len(packets)==len(original)
        assert [p['data_hash'] for p in packets]==[p['data_hash'] for p in original]
        deltas=sorted({str(Decimal(p['pts_time'])-Decimal(q['pts_time'])) for p,q in zip(packets,original)})
        assert deltas==[expected],(name,kind,deltas)
        row['packetComparisons'][kind]=dict(packets=len(packets),payloadsMatchOriginal=True,allPtsShiftSeconds=deltas,firstOriginalPts=original[0]['pts_time'],firstControlPts=packets[0]['pts_time'],probeSha256=digest(path))
    records.append(row)
report=dict(inputsValidated=True,playbackAccepted=False,producerSha256=digest(Path(__file__)),fixtureProducerSha256=digest(work/'generate-timestamp-controls-r8.sh'),originalMediaSha256=digest(work/'media-r1/multi-language.mkv'),records=records)
(work/'timestamp-control-inputs-r8.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps(report,indent=2))
