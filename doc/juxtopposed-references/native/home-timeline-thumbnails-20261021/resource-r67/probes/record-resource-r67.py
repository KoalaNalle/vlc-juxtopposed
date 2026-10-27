from pathlib import Path
from hashlib import sha256
import gzip,json,shutil,statistics

repo=Path('C:/Modding/vlc-juxtopposed');work=Path(__file__).resolve().parent
public=repo/'doc/juxtopposed-references/native/home-timeline-thumbnails-20261021/resource-r67'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def digest(p):return sha256(p.read_bytes()).hexdigest()
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
assert not public.exists()
prior=read(public.parent/'renderer-r63/checkpoint.json')['sourceBindings']
sources={p:digest(repo/p) for p in prior}
changed=[p for p in sources if sources[p]!=prior[p]]
assert changed==['modules/gui/qt/player/qml/SliderBar.qml']
for p,h in sources.items():assert digest(work/'source-resource-r67'/p)==h,p
native=read(work/'backend-startup-new-full-r68/verification.json')
ordinary={t:read(work/f'ordinary-resource-{t}-r67/verification.json') for t in ['A1','B1']}
for result in [native,*ordinary.values()]:
    assert result['status']=='terminal' and result['passed'] and result['normalExit'] and result['exitCode']==0
    assert result['sourceInputsUnchanged'] and result['identitiesUnchanged'] and not result['forcedCleanupUsed']
assert native['counts']==dict(tests=26,failures=0,errors=0,skipped=0)
assert native['sourceInputsSha256']==digest(work/'inputs-tuple-r68.json')
assert ordinary['A1']['identities']==ordinary['B1']['identities']
for field in ['libvlccore.dll','plugins/gui/libqt_plugin.dll','plugins/demux/libmp4_plugin.dll']:
    assert native['identities'][field]==ordinary['A1']['identities'][field]
assert ordinary['A1']['frameLatencyEnvironment']=='2' and not ordinary['A1']['candidateDefaultApplied']
assert ordinary['B1']['frameLatencyEnvironment'] is None and ordinary['B1']['candidateDefaultApplied']
summaries={}
for t,r in ordinary.items():
    assert not r['diagnostics'] and r['previewHoverStayedActive']
    assert r['sourceInputsSha256']==digest(work/'inputs-resource-r67.json')
    gate=r['hoverAtPreviewGate'];end=r['hoverAtMeasurementEnd']
    assert gate['transitions'][-1]['after'] and end['transitions'][-1]['after']
    assert gate['transitions']==end['transitions'][:len(gate['transitions'])]
    assert all(v['after'] for v in end['transitions'][len(gate['transitions']):])
    assert len(gate['deactivations'])==len(end['deactivations'])
    assert read(work/f'ordinary-resource-{t}-r67/measurement-end-inspection.json')['picturePreviewObserved']
    order=r['quitLogOrder'];assert order['wait']<order['stopped']<order['teardown']
    summaries[t]={}
    for phase,count in [('playing',25),('paused',12),('playing-preview',20),('paused-preview',120)]:
        m=read(work/f'ordinary-resource-{t}-r67/{phase}-metrics.json');s=m['samples'];p=r['phases'][phase]
        assert m['status']=='terminal' and len(s)==count
        delta={k:p['after'][k]-p['before'][k] for k in ['frames displayed','frames late','frames lost','buffers played','buffers lost']}
        assert delta['frames late']==delta['frames lost']==delta['buffers lost']==0
        fps=delta['frames displayed']/p['durationSeconds']
        if phase.startswith('paused'):
            assert all(v==0 for v in delta.values()) and p['timeBefore']==p['timeAfter']
        else:assert 48<fps<52
        watts=[d['boardPowerW'] for x in s for d in x.get('board',{}).get('devices',[]) if isinstance(d.get('boardPowerW'),(int,float))]
        summaries[t][phase]=dict(samples=count,durationSeconds=p['durationSeconds'],counterDelta=delta,displayedFps=fps,
            meanCpuMachinePercent=statistics.mean(x['cpuMachinePercent'] for x in s),
            meanPrivateMB=statistics.mean(x['privateBytes']/1e6 for x in s),meanWholeBoardPowerW=statistics.mean(watts))
        if phase=='paused-preview':summaries[t][phase]['privateMBThirtySecondBlocks']=[statistics.mean(x['privateBytes']/1e6 for x in s[i:i+30]) for i in range(0,120,30)]
memory={}
for name in ['post-metric-memory.json','off-hover-memory.json']:
    d=read(work/'ordinary-resource-A1-r67'/name);s=d['samples'];assert d['status']=='terminal' and len(s)==37 and s[-1]['elapsedSeconds']>=180
    memory[name]=dict(samples=len(s),durationSeconds=s[-1]['elapsedSeconds'],firstPrivateMB=s[0]['privateBytes']/1e6,lastPrivateMB=s[-1]['privateBytes']/1e6)
debug=read(work/'memory-native-r69/verification.json')
stack=(work/'memory-native-r69/memory-stack.txt').read_text(encoding='utf-8',errors='replace')
assert debug['status']=='terminal' and debug['debuggerExitCode']==0 and 'Detached' in stack
heapAvailable='Invalid type information' not in stack
assert not heapAvailable
public.mkdir()
def copy(src,name):
    target=public/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(src,target)
def compress(src,name):
    target=public/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(gzip.compress(src.read_bytes(),mtime=0))
for t in ordinary:
    root=work/f'ordinary-resource-{t}-r67'
    for name in ['verification.json','initial-inspection.json','preview-inspection.json','measurement-end-inspection.json','after-preview-inspection.json','cli-observations.txt']:
        copy(root/name,f'ordinary/{t}-{name}')
    for phase in ['playing','paused','playing-preview','paused-preview']:copy(root/f'{phase}-metrics.json',f'ordinary/{t}-{phase}-metrics.json')
    for name in ['vlc.log','stderr.txt','board-observer-r67.json']:compress(root/name,f'ordinary/{t}-{name}.gz')
for name in ['post-metric-memory.json','off-hover-memory.json','off-hover-end-inspection.json']:copy(work/'ordinary-resource-A1-r67'/name,'extra-memory/'+name)
for name in ['verification.json','results.xml','results.txt','stdout.txt','stderr.txt']:copy(work/'backend-startup-new-full-r68'/name,'native/'+name)
for p in (work/'backend-startup-new-full-r68/runtime').glob('*native.log'):compress(p,'native/'+p.name+'.gz')
copy(work/'memory-native-r69/verification.json','diagnostic/verification.json')
compress(work/'memory-native-r69/memory-stack.txt','diagnostic/memory-stack.txt.gz')
for name in ['run-resource-r67.py','observe-board-r67.py','resource_metrics_r45.py','run-tuple-r68.py','inputs-resource-r67.json','inputs-tuple-r68.json','observe-post-metric-memory-r67.py','observe-off-hover-memory-r67.py','inspect-owned-memory-r69.py','build-ordinary-r67.sh','record-resource-r67.py']:
    copy(work/name,'probes/'+name)
artifacts={p.relative_to(public).as_posix():digest(p) for p in sorted(public.rglob('*')) if p.is_file()}
write(public/'checkpoint.json',dict(date='2026-10-22',scope='Ordinary Win11 logged-hover resource pair and native26 on the R67 tuple; separate post-metric memory/diagnostic observations retain their own bounds.',
    sourceBindings=sources,sourceSnapshot='build-windows/beta-timeline-thumbnails-20261021/source-resource-r67',artifacts=artifacts,
    identities=ordinary['B1']['identities'],ordinaryCases={t:f'ordinary/{t}-verification.json' for t in ordinary},nativeCase='native/verification.json',
    resourceSummary=summaries,extraMemorySummary=memory,deltaFromR63=changed,
    ordinarySelectedPlaybackPauseResumeQuitAccepted=True,nativeSelectedFormatsAccepted=True,nativeHoverContinuityDuringMeasuredPreviewAccepted=True,
    continuousPixelRecordingClaimed=False,resourceMeasurementIntegrityAccepted=True,resourceCostAccepted=False,memoryStabilityAccepted=False,
    powerAttributionResolved=False,originalHoverLossCauseEstablished=False,nativeHeapOwnershipEstablished=False,
    frameWaitWorkaroundPromoted=False,backdropBlurAccepted=False,physicalDpiAccepted=False,fullFeatureAccepted=False,fullBetaAccepted=False,
    notes=[
        'Only source delta from R63 is a stable objectName on the existing HoverHandler. Behavior is unchanged; ordinary Qt logging identifies its actual transitions without private observer hooks.',
        'Both measured preview intervals start/end with real picture41 visible. Named Qt hover has no intervening false transition; Qt Quick focus has no intervening window deactivation. Pixels were inspected at selected points, not continuously recorded.',
        'Both warmup observations show picture41 over paused primary0, then move pointer to title bar before baseline. Do not infer universal cold-start footprint or unrelated media cost.',
        'Both measured playing intervals have aggregate approximately50 displayed frames/second and no newly late/lost frames or lost audio buffers. Primary paused time/counters remain unchanged, followed by real resumed advancement and normal playing Quit.',
        'Both cases show growing paused private memory. In control A1 it continues494.0->583.5MB during an extra180 seconds with card open, and615.1->701.4MB during a separate180 seconds with card closed. Thus neither the candidate frame wait nor the visible card alone is established as its cause.',
        'Extra memory observers do not query GPU counters; the separate board observer still queries them. Monitoring effects are not excluded. R70 separates quiet/PDH/NVIDIA/quiet phases on an ordinary paused process.',
        'Native diagnostic runs only after all resource/memory intervals. CDB noninvasive snapshot returns0 and detaches; heap/type/address commands lack needed local OS symbols. Thread stacks are retained, but no heap ownership or allocation source is established.',
        'Whole GPU power includes other processes/capture. Prior97W remains preserved and unattributed. New power data and successful runner status do not close cost/promotion gates.',
        'Existing package/release unchanged. Memory stability, actual backdrop blur, legacy compositor primary composition, broader layout/DPI/accessibility and the complete beta roadmap remain open.'
    ]))
for p,h in artifacts.items():assert digest(public/p)==h,p
print('Saved',len(sources),'source bindings /',len(artifacts),'artifacts: ordinary2 + native26; hover continuity qualified, memory cost remains open.')
