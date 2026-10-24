from pathlib import Path
from hashlib import sha256
import gzip,json,shutil,statistics
repo=Path('C:/Modding/vlc-juxtopposed');work=Path(__file__).resolve().parent
public=repo/'doc/juxtopposed-references/native/home-timeline-thumbnails-20261021/resource-r95'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def digest(p):return sha256(p.read_bytes()).hexdigest()
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
assert not public.exists()
sources=read(public.parent/'scrollbar-r93/checkpoint.json')['sourceBindings'];assert all(digest(repo/p)==h for p,h in sources.items())
native=read(work/'backend-startup-new-full-r96/verification.json');assert native['passed'] and native['normalExit'] and native['counts']==dict(tests=26,failures=0,errors=0,skipped=0)
ordinary={t:read(work/f'ordinary-resource-{t}-r95/verification.json') for t in ['A1','B1']}
assert ordinary['A1']['identities']==ordinary['B1']['identities']
for field in ['libvlccore.dll','plugins/gui/libqt_plugin.dll','plugins/demux/libmp4_plugin.dll']:assert native['identities'][field]==ordinary['A1']['identities'][field]
assert ordinary['A1']['frameLatencyEnvironment']=='2' and not ordinary['A1']['candidateDefaultApplied']
assert ordinary['B1']['frameLatencyEnvironment'] is None and ordinary['B1']['candidateDefaultApplied']
summaries={};memoryBounds={}
for t,r in ordinary.items():
 assert r['status']=='terminal' and r['passed'] and r['normalExit'] and r['exitCode']==0 and r['sourceInputsUnchanged'] and r['identitiesUnchanged'] and not r['forcedCleanupUsed'] and not r['diagnostics'] and r['previewHoverStayedActive']
 assert r['sourceInputsSha256']==digest(work/'inputs-resource-r95.json')
 gate=r['hoverAtPreviewGate'];end=r['hoverAtMeasurementEnd'];assert gate['transitions'][-1]['after'] and end['transitions'][-1]['after'] and all(v['after'] for v in end['transitions'][len(gate['transitions']):]) and len(gate['deactivations'])==len(end['deactivations'])
 root=work/f'ordinary-resource-{t}-r95'
 initial=read(root/'initial-inspection.json');assert initial['primaryCaption']=='00:00:41' and initial['initialWarmupCardObserved'] and initial['baselinePointerMovedToTitlebar'] and not initial['picturePreviewObserved']
 assert read(root/'measurement-end-inspection.json')['picturePreviewObserved'] and read(root/'after-preview-inspection.json')['actualMotionObserved']
 q=r['quitLogOrder'];assert q['wait']<q['stopped']<q['teardown']
 summaries[t]={}
 for phase,count in [('playing',25),('paused',12),('playing-preview',20),('paused-preview',120)]:
  d=read(root/(phase+'-metrics.json'));s=d['samples'];p=r['phases'][phase];assert d['status']=='terminal' and len(s)==count
  delta={k:p['after'][k]-p['before'][k] for k in ['frames displayed','frames late','frames lost','buffers played','buffers lost']};assert delta['frames late']==delta['frames lost']==delta['buffers lost']==0
  fps=delta['frames displayed']/p['durationSeconds']
  if phase.startswith('paused'):assert all(v==0 for v in delta.values()) and p['timeBefore']==p['timeAfter']
  else:assert 48<fps<52
  watts=[x['boardPowerW'] for sample in s for x in sample.get('board',{}).get('devices',[]) if isinstance(x.get('boardPowerW'),(float,int))]
  summaries[t][phase]=dict(samples=len(s),durationSeconds=p['durationSeconds'],counterDelta=delta,displayedFps=fps,meanCpuMachinePercent=statistics.mean(x['cpuMachinePercent'] for x in s),firstPrivateMB=s[0]['privateBytes']/1e6,lastPrivateMB=s[-1]['privateBytes']/1e6,minPrivateMB=min(x['privateBytes'] for x in s)/1e6,maxPrivateMB=max(x['privateBytes'] for x in s)/1e6,meanWholeBoardPowerW=statistics.mean(watts))
  if phase=='paused-preview':
   blocks=[statistics.mean(x['privateBytes']/1e6 for x in s[i:i+30]) for i in range(0,120,30)]
   summaries[t][phase]['privateMBThirtySecondBlocks']=blocks
   memoryBounds[t]=bool(s[-1]['privateBytes']<=s[0]['privateBytes']+5000000 and max(blocks[1:])-min(blocks[1:])<=5)
assert all(memoryBounds.values()),'Selected paused-preview memory bound did not pass'
public.mkdir()
for t in ordinary:
 root=work/f'ordinary-resource-{t}-r95'
 for p in root.iterdir():
  if not p.is_file() or p.suffix not in ['.json','.txt','.log']:continue
  target=public/'ordinary'/t/p.name;target.parent.mkdir(parents=True,exist_ok=True)
  if p.suffix in ['.txt','.log']:target.with_suffix(p.suffix+'.gz').write_bytes(gzip.compress(p.read_bytes(),mtime=0))
  else:shutil.copyfile(p,target)
for name in ['run-resource-r95.py','inputs-resource-r95.json','resource_metrics_r45.py','build-ordinary-r94.sh','ordinary-r94.mk','qt-scrollbar-dependencies-r93.json','record-resource-r95.py']:
 target=public/'probes'/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(work/name,target)
artifacts={p.relative_to(public).as_posix():digest(p) for p in sorted(public.rglob('*')) if p.is_file()}
write(public/'checkpoint.json',dict(date='2026-10-22',scope='Repaired ordinary GUI matching candidate/default and explicit2 resource pair: selected CPU/private-memory/frame throughput; GPU board data remains contextual.',sourceBindings=sources,artifacts=artifacts,identities=ordinary['B1']['identities'],resourceSummary=summaries,selectedPausedPreviewMemoryBoundAccepted=memoryBounds,
 nativeHoverContinuityDuringMeasuredPreviewAccepted=True,selectedPlaybackPauseResumeQuitAccepted=True,selectedOrdinaryMemoryStabilityAccepted=True,resourceMeasurementIntegrityAccepted=True,continuousPixelRecordingClaimed=False,universalResourceCostAccepted=False,processGpuPowerAttributionAccepted=False,frameWaitWorkaroundPromoted=False,backdropBlurAccepted=False,fullBetaAccepted=False,
 notes=[
 'Both runs deliberately seek paused primary41 and inspect picture41, then close the card before baseline. Control also starts with a stationary-pointer warmup card over primary0 before that deliberate seek. No universal cold-footprint claim.',
 'Both preview intervals begin with actual paused primary41/card41, native named hover true and no intervening window deactivation. Actual endpoint cards remain visible after120s paused measurement; only endpoint pixels are inspected.',
 'Each measured playing interval is approximately50 displayed frames/sec, with no newly late/lost frames or lost audio buffers. Paused clocks/counters stay unchanged. Both resume actual motion and quit normally through CtrlQ with clean logs and unchanged source/binaries.',
 'Selected memory bound: no more5MB net private growth across120s paused preview and no more5MB spread among the final three30-second means. This distinguishes repaired bounded behavior from prior sustained growth; it does not prove every long-run allocation path leak-free.',
 'Qt6.8.3 narrow scrollbar UniqueConnection repair is included. No UIA-request blocker, activation wrapper, object observer or private journey hook is linked. Every accessibility request/notification remains enabled.',
 'CPU means and full memory/board traces are reported per phase. Whole-board power includes other applications and capture; it is not process energy attribution. Earlier97W stays preserved/unattributed. Selected memory/throughput acceptance is not a universal performance budget or production workaround promotion.',
 'Matching latest native26 and direct scrollbar7/ordinary idle180s receipts qualify separate scopes. Full design/settings/layout/DPI/accessibility/review/package roadmap and real backdrop blur remain open.'
 ]))
print('Saved repaired ordinary pair',len(sources),'sources',len(artifacts),'artifacts')
print(json.dumps(summaries,indent=2))
