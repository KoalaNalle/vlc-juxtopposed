from pathlib import Path
from hashlib import sha256
import ast,json
work=Path(__file__).resolve().parent
digest=lambda p:sha256(p.read_bytes()).hexdigest()
text=(work/'native-mkv-edges-r20.py').read_text(encoding='utf-8')
before="class Stats(C.Structure):"
addition='''class ProcessIo(C.Structure):
    _fields_=[(name,C.c_uint64) for name in ['readOperations','writeOperations','otherOperations','readBytes','writeBytes','otherBytes']]
assert C.sizeof(ProcessIo)==48
kernel=C.WinDLL('kernel32',use_last_error=True)
kernel.GetCurrentProcess.restype=C.c_void_p;kernel.GetCurrentProcess.argtypes=[]
kernel.GetProcessIoCounters.restype=C.c_int;kernel.GetProcessIoCounters.argtypes=[C.c_void_p,C.POINTER(ProcessIo)]
currentProcess=kernel.GetCurrentProcess()
def process_io():
    value=ProcessIo();assert kernel.GetProcessIoCounters(currentProcess,C.byref(value))
    return {name:getattr(value,name) for name,_ in ProcessIo._fields_}
def image_mask(path):
    im=QImage(str(path)).convertToFormat(QImage.Format_RGBA8888);assert not im.isNull() and im.width()==960 and im.height()==540
    raw=bytes(im.bits());return bytes(1 if raw[i]>160 and raw[i+1]>160 and raw[i+2]>160 else 0 for i in range(0,len(raw),4))
references={r['referencePicture']:image_mask(work/r['referencePicture']) for r in cfg['steps'] if 'referencePicture' in r}

'''
assert text.count(before)==1;text=text.replace(before,addition+before)
before="        row['passed']=all(row['assertions'].values())"
addition='''        if 'referencePicture' in expected and row.get('snapshotAvailableWhilePaused'):
            reference=references[expected['referencePicture']]
            union=sum(a|b for a,b in zip(white,reference));intersection=sum(a&b for a,b in zip(white,reference))
            jaccard=intersection/union if union else 1.0
            row.update(referenceComparisonPerformed=True,referencePicture=expected['referencePicture'],referencePictureSha256=digest(work/expected['referencePicture']),captionMaskJaccard=jaccard)
            row['assertions']['referenceCaptions']=jaccard>=.985
        else:
            row['referenceComparisonPerformed']=False
            if 'referencePicture' in expected:row['assertions']['referenceCaptions']=False
        if not expected['subtitle']:
            row['assertions']['subtitlePresence']=row.get('brightPixels',9999)<=10
'''
assert text.count(before)==2;text=text.replace(before,addition+before,1)
before="        before=stats();started=time.monotonic();ret=seek(player,int(step['target']*1_000_000),False);assert ret==0"
after="        before=stats();before_io=process_io();before_cpu=time.process_time();started=time.monotonic();ret=seek(player,int(step['target']*1_000_000),False);assert ret==0"
assert text.count(before)==1;text=text.replace(before,after)
before="        row['statsBeforeSeek']=before"
after="        row['processCpuSecondsIncludingProbe']=time.process_time()-before_cpu\n        after_io=process_io();row['processIoDeltaIncludingProbe']={p:after_io[p]-before_io[p] for p in before_io}\n"+before
assert text.count(before)==1;text=text.replace(before,after)
text=text.replace("scope='Native coarse target colors, actual subtitle pixels/masks, frozen paused clock and resume; counters/times are observations, not an accepted performance budget or exact video PTS.'", "scope='Native coarse target colors, actual subtitle pixels and reference-caption comparisons where configured, frozen pause and resume. Process I/O/CPU includes the probe, generated PNG reads, font/library reads and filesystem-cache effects; it is not physical storage throughput, exclusive demux work, exact video PTS or a universal performance acceptance.'")
ast.parse(text);target=work/'native-mkv-interval-r25.py';assert not target.exists();target.write_text(text,encoding='utf-8',newline='\n')

parent=(work/'run-native-mkv-edges-r20.py').read_text(encoding='utf-8')
parent=parent.replace("worker=work/'native-mkv-edges-r20.py'", "worker=work/'native-mkv-interval-r25.py'")
parent=parent.replace("root=work/'native-mkv-edges-r20'", "root=work/'native-mkv-interval-r25'")
parent=parent.replace('mkv-spu-seek-r18b-source/source-record.json','mkv-interval-r23d-source/source-record.json')
parent=parent.replace("'mkv-spu-seek-'+phase+'-r18b-bin/libmkv_plugin.dll'", "'mkv-interval-'+phase+'-r23d-bin/libmkv_plugin.dll'")
position=parent.index("for kind,start,duration in ")
extra='''overlap=dict(name='overlapping-cues',media='media-r21/colored-overlap.mkv',language='eng',audio=True,steps=[step('overlap-first',11,1,True),step('long-alone',15,1,True),step('overlap-second',31,0,True),step('long-alone-again',35,0,True),step('long-at-first',5,0,True)])
refs=[
    'native-mkv-overlap-r21/after/overlapping-cues/seek-0-overlap-first.png',
    'native-overlap-reference-r22/before/normal-playback-reference/normal-playback-after-short-cue-ended.png',
    'native-overlap-mask-reference-r24/before/normal-playback-reference/normal-overlap-31s.png',
    'native-overlap-mask-reference-r24/before/normal-playback-reference/normal-long-alone-35s.png',
    'native-overlap-mask-reference-r24/before/normal-playback-reference/normal-long-alone-35s.png',
]
for item,reference in zip(overlap['steps'],refs):item['referencePicture']=reference
configs.append(overlap)
'''
parent=parent[:position]+extra+parent[position:]
before="assert all(digest(repo/p)==h for p,h in inputs.items())"
after=before+"\nassert source['sourceCount']==2196 and all(digest(work/'source-mkv-interval-r23d'/p)==h for p,h in inputs.items())\nfor phase,files in source['privateSources'].items():\n    assert all(digest(work/'mkv-interval-r23d-source'/phase/p)==h for p,h in files.items())\nassert (work/'build-mkv-interval-r23d-exit.txt').read_text().strip()=='0'"
assert parent.count(before)==1;parent=parent.replace(before,after)
ast.parse(parent);target=work/'run-native-mkv-interval-r25.py';assert not target.exists();target.write_text(parent,encoding='utf-8',newline='\n')
record=dict(generatorSha256=digest(Path(__file__)),preflightProducerSha256=digest(work/'prepare-native-interval-r25-preflight.py'),preflightCorrection='The pass-assignment occurs twice; only its first occurrence belongs to observe(). The producer assertion failed before creating a worker or launching native code.',workerSha256=digest(work/'native-mkv-interval-r25.py'),parentSha256=digest(target),references={p:digest(work/p) for p in refs},scope='New matched controls use full before/after Matroska rebuilds and compare overlap captions to retained independent normal-playback references;11s uses the earlier visually verified two-caption candidate picture. Production code/runtime remain unchanged.')
(work/'native-interval-producers-r25.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps(record,indent=2))
