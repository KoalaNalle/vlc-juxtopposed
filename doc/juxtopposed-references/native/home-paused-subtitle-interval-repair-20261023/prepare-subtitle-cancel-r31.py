from pathlib import Path
import ast
work=Path(__file__).resolve().parent
original=(work/'native-mkv-interval-r25.py').read_text(encoding='utf-8')
prefix=original[:original.index('def image_mask(')]
tail=r'''
options=['--ignore-config','--no-media-library','--no-metadata-network-access','--no-ml-network-discovery','--no-osd','--no-video-title-show','--no-sub-autodetect-file','--vout=vdummy','--no-hw-dec','--audio-language=eng','--sub-language=eng','--verbose=2','--file-logging','--logfile='+str(out/'native.log'),'--aout=mmdevice','--mmdevice-backend=wasapi']
encoded=[v.encode() for v in options]
instance=new(len(encoded),(C.c_char_p*len(encoded))(*encoded));assert instance
player=playerNew(instance,None,None);assert player
media=mediaNew(str(mediaPath).encode());assert media
mediaOption(media,b':start-paused');setMedia(player,media)
result=dict(passed=False,candidateOnly=True,repairAccepted=False,configuration=cfg,
    producerSha256=digest(Path(__file__)),fixtureSha256=digest(mediaPath),
    coreSha256=digest(runtime/'libvlccore.dll'),mkvSha256=digest(runtime/'plugins/demux/libmkv_plugin.dll'),
    options=options,teardownComplete=False,indexCancellationIsolated=False,
    scope='Normal Stop request during a pending precise seek, not forced termination. Index-phase qualification additionally requires AV preroll anchors, increased own-process reads, and no final seek/preroll marker even after teardown. Own-process reads include probe/log activity and filesystem caches.')
log=out/'native.log'
def logtext():
    return log.read_text(encoding='utf-8',errors='replace') if log.exists() else ''
finished_marker='seek: preroll{ req: 55000001,'
anchor='seek: preroll{ track: 1, pts: 55000000,'
try:
    assert play(player)==0
    result['initialPauseReached']=wait(lambda:state(player)==3,10)
    assert result['initialPauseReached']
    time.sleep(.5)
    result['selectedEnglish']=any(t['language']=='eng' and t['selected'] for t in tracks())
    assert result['selectedEnglish'] and vouts(player)>0
    before_io=process_io();started=time.monotonic();result['seekReturn']=seek(player,55_000_000,False)
    assert result['seekReturn']==0
    end=time.monotonic()+10
    trigger=False
    while time.monotonic()<end:
        text=logtext();now_io=process_io()
        anchor_seen=anchor in text
        finished_seen=finished_marker in text
        delta=now_io['readBytes']-before_io['readBytes']
        if anchor_seen and not finished_seen and delta>2_000_000:
            trigger=True
            break
        if finished_seen:
            break
        time.sleep(.001)
    # Keep this observation in memory until after Stop to minimize the interval
    # between the observed stage and the ordinary application stop request.
    observation=dict(elapsedSeconds=time.monotonic()-started,anchorSeen=anchor_seen,
        finishedSeen=finished_seen,readBytesIncludingProbe=delta,triggered=trigger,state=state(player))
    stop_started=time.monotonic();result['stopReturn']=stop(player)
    result['stopReached']=wait(lambda:state(player)==4,10)
    result['stopToStoppedSeconds']=time.monotonic()-stop_started
    result['preStopObservation']=observation
    write('result.json',result)
finally:
    if not result.get('stopReached'):
        result['cleanupStopReturn']=stop(player)
        result['stopReached']=wait(lambda:state(player)==4,10)
    write('result.json',result)
    playerRelease(player);mediaRelease(media);release(instance)
    result['teardownComplete']=True
text=logtext()
result['finalSeekMarkerPresent']=finished_marker in text
result['indexCancellationIsolated']=bool(result.get('preStopObservation',{}).get('triggered') and not result['finalSeekMarkerPresent'])
result['normalStopPassed']=result['stopReturn']==0 and result['stopReached'] and result['teardownComplete']
result['passed']=result['normalStopPassed'] and result['indexCancellationIsolated']
result['diagnostics']=[l for l in text.splitlines() if re.search(r'\berror:|Assertion failed',l)]
write('result.json',result)
print(json.dumps({k:result[k] for k in ['passed','normalStopPassed','indexCancellationIsolated','preStopObservation','stopToStoppedSeconds','finalSeekMarkerPresent','teardownComplete']}),flush=True)
assert result['normalStopPassed']
'''
text=prefix+tail
ast.parse(text)
target=work/'native-subtitle-cancel-r31.py';assert not target.exists()
target.write_text(text,encoding='utf-8',newline='\n')

parent=(work/'run-native-mkv-headers-r30.py').read_text(encoding='utf-8')
start=parent.index('configs=[');end=parent.index('source=read(',start)
parent=parent[:start]+"configs=[dict(name='stop-pending-index',media='media-r31/cbr-overlap.mkv',language='eng',audio=True,steps=[])]\n"+parent[end:]
parent=parent.replace("worker=work/'native-mkv-interval-r25.py'", "worker=work/'native-subtitle-cancel-r31.py'")
parent=parent.replace("root=work/'native-mkv-headers-r30'", "root=work/'native-subtitle-cancel-r31'")
parent=parent.replace("fixtureProducerSha256=digest(work/'generate-subtitle-cost-r26.sh')", "fixtureProducerSha256=digest(work/'generate-subtitle-cancel-r31.sh')")
old="print(phase,cfg['name'],'normal terminal','passed',record['passed'],'steps',[(r['step'],r.get('passed'),r.get('brightPixels')) for r in value['observations']],flush=True)"
assert parent.count(old)==1
parent=parent.replace(old,"print(phase,cfg['name'],'normal terminal','passed',record['passed'],'indexCancellationIsolated',value.get('indexCancellationIsolated'),flush=True)")
ast.parse(parent)
target=work/'run-native-subtitle-cancel-r31.py';assert not target.exists()
target.write_text(parent,encoding='utf-8',newline='\n')
