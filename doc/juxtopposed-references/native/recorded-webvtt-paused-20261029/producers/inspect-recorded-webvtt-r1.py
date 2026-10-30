"""Read sample timing directly from generated MP4s; do not alter their bytes."""
import hashlib
import json
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'build-windows/beta-record-transition-20261029/mp4-timing-r1'


def boxes(data, start=0, stop=None):
    stop = len(data) if stop is None else stop
    while start < stop:
        assert start + 8 <= stop
        size, kind = struct.unpack_from('>I4s', data, start)
        header = 8
        if size == 1:
            size = struct.unpack_from('>Q', data, start+8)[0]
            header = 16
        elif size == 0:
            size = stop-start
        assert size >= header and start+size <= stop
        yield kind.decode('ascii'), start+header, start+size
        start += size


def inspect(path):
    data = path.read_bytes()
    top = {kind:(start,end) for kind,start,end in boxes(data)}
    moov = {kind:(start,end) for kind,start,end in boxes(data,*top['moov']) if kind != 'trak'}
    mvhd = data[slice(*moov['mvhd'])]
    movie_scale = struct.unpack_from('>I', mvhd, 20 if mvhd[0] else 12)[0]
    result = []
    for kind,start,end in boxes(data,*top['moov']):
        if kind != 'trak':
            continue
        trak = {k:(a,b) for k,a,b in boxes(data,start,end)}
        mdia = {k:(a,b) for k,a,b in boxes(data,*trak['mdia'])}
        minf = {k:(a,b) for k,a,b in boxes(data,*mdia['minf'])}
        stbl = {k:(a,b) for k,a,b in boxes(data,*minf['stbl'])}
        stsd = data[slice(*stbl['stsd'])]
        types = [k for k,_,_ in boxes(stsd,8)]
        if 'wvtt' not in types:
            continue
        mdhd = data[slice(*mdia['mdhd'])]
        scale = struct.unpack_from('>I',mdhd,20 if mdhd[0] else 12)[0]
        duration = struct.unpack_from('>Q' if mdhd[0] else '>I',mdhd,24 if mdhd[0] else 16)[0]
        stts = data[slice(*stbl['stts'])]
        entries = [struct.unpack_from('>II',stts,8+8*i) for i in range(struct.unpack_from('>I',stts,4)[0])]
        durations = [delta for count,delta in entries for _ in range(count)]
        stsz = data[slice(*stbl['stsz'])]
        size,count = struct.unpack_from('>II',stsz,4)
        sizes = [size]*count if size else list(struct.unpack_from('>'+str(count)+'I',stsz,12))
        offset_kind = 'stco' if 'stco' in stbl else 'co64'
        offsets = data[slice(*stbl[offset_kind])]
        offset_count = struct.unpack_from('>I',offsets,4)[0]
        chunks = struct.unpack_from('>'+str(offset_count)+('I' if offset_kind == 'stco' else 'Q'),offsets,8)
        stsc = data[slice(*stbl['stsc'])]
        chunk_map = [struct.unpack_from('>III',stsc,8+12*i) for i in range(struct.unpack_from('>I',stsc,4)[0])]
        samples=[]; index=0; dts=0
        for chunk_number,offset in enumerate(chunks,1):
            per_chunk = [r for r in chunk_map if r[0] <= chunk_number][-1][1]
            for _ in range(per_chunk):
                payload = data[offset:offset+sizes[index]]
                payload_types = [k for k,_,_ in boxes(payload)]
                texts=[]
                for k,a,b in boxes(payload):
                    if k in ['vttc','vttx']:
                        texts += [payload[x:y].decode('utf-8') for t,x,y in boxes(payload,a,b) if t == 'payl']
                samples.append(dict(dtsUnits=dts,durationUnits=durations[index],
                                    startSeconds=dts/scale,endSeconds=(dts+durations[index])/scale,
                                    boxes=payload_types,text=texts))
                dts+=durations[index];offset+=sizes[index];index+=1
        assert index == count == len(durations)
        edits=[]
        if 'edts' in trak:
            elst = {k:(a,b) for k,a,b in boxes(data,*trak['edts'])}
            payload=data[slice(*elst['elst'])]
            for i in range(struct.unpack_from('>I',payload,4)[0]):
                segment,media_time=struct.unpack_from('>Qq' if payload[0] else '>Ii',payload,8+i*(20 if payload[0] else 12))
                edits.append(dict(segmentMovieUnits=segment,segmentSeconds=segment/movie_scale,
                                  mediaTimeUnits=media_time,mediaTimeSeconds=media_time/scale))
        result.append(dict(timescale=scale,mdhdDuration=duration,sumSampleDuration=dts,
                           samples=samples,edits=edits))
    assert result
    return dict(file=path.name,sha256=hashlib.sha256(data).hexdigest(),wvttTracks=result)


def main():
    assert not OUT.exists()
    OUT.mkdir()
    paths = [ROOT / 'build-windows/beta-resume-20261029/gui-r11/media/recorded-wvtt.mp4',
             *sorted((ROOT / 'build-windows/beta-resume-20261029/gui-r13/recordings').glob('*.mp4'))]
    result = [inspect(p) for p in paths]
    (OUT / 'sample-tables.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))


if __name__ == '__main__':
    main()
