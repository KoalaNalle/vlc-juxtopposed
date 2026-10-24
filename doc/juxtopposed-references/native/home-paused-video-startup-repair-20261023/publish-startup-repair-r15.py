from pathlib import Path
from hashlib import sha256
import json,shutil,re
from PySide6.QtGui import QImage

repo=Path('C:/Modding/vlc-juxtopposed');work=Path(__file__).resolve().parent
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
inputs=read(work/'inputs-r14.json');assert len(inputs)==2147
assert all(digest(repo/p)==h and digest(work/'source-r14'/p)==h for p,h in inputs.items())
old=read(work/'inputs-r4.json')
assert [p for p,h in old.items() if digest(repo/p)!=h]==['src/input/decoder.c','test/Makefile.am']
assert all(digest(work/'source-r4'/p)==h for p,h in old.items())
ordinary=read(work/'ordinary-ui-r14/verification.json')
assert ordinary['normalExit'] and ordinary['exitCode']==0 and ordinary['sourceInputsUnchanged'] and ordinary['runtimeUnchanged'] and ordinary['mediaUnchanged']
identities=ordinary['runtimeSha256'];assert identities['libvlccore.dll']=='000f1159b7d190d83edaba83739b97b869669ca2eec2ba97bc35d6c51f9bb9cb'
assert all(digest(work/'ordinary-ui-r14/runtime'/p)==h for p,h in identities.items())
assert b'OWNED_DECODER_TRACE' not in (work/'core-r14/libvlccore.dll').read_bytes()
assert digest(work/'ordinary-ui-r4/runtime/libvlccore.dll')=='4422fbb02fb22857c257340d486a0310c923526d6d276127258566d3ac2671de'
assert (work/'build-core-r14-exit.txt').read_text().strip()=='0'
controls=read(work/'timestamp-control-inputs-r8b.json');assert controls['inputsValidated']
preserve=controls['records'][0];assert preserve['name']=='preserve' and preserve['streams']==5
assert all(v['payloadsMatchOriginal'] and v['ptsShiftSecondsForTimestampedPackets']==['0.000000'] for v in preserve['packetComparisons'].values())
assert preserve['packetComparisons']['video']['packets']==1500 and preserve['packetComparisons']['audio']['packets']==2814
groups=['native-startup-r8','native-core-trace-r9c','native-startup-candidate-r10','native-tracks-r15']
all_records=[]
for group in groups:
    data=read(work/group/'processes.json');assert data['allProcessesNormal']
    for row in data['records']:
        assert row['normalExit'] and row['exitCode']==0 and row['runtimeUnchanged'] and row['sourceInputsUnchanged'] and row['producerUnchanged']
        result=read(work/group/row['case']/'investigation.json');assert result['teardownComplete'] and result['stopReached']
        all_records.append(dict(group=group,case=row['case'],pid=row['pid']))
for group in ['native-startup-edges-r12','native-startup-edges-r13','native-startup-ordinary-r14']:
    data=read(work/group/'comparison.json');assert data['allProcessesNormal'] and data['sourceRuntimeFixturesProducersUnchanged']
    for row in data['records']:
        assert row['normalExit'] and row['exitCode']==0 and row['runtimeUnchanged']
        result=read(work/group/row['case']/row['variant']/'result.json');assert result['teardownComplete'] and result['stopReached']
        all_records.append(dict(group=group,case=row['case'],variant=row['variant'],pid=row['pid']))
final=read(work/'native-startup-ordinary-r14/comparison.json');assert len(final['records'])==9 and all(r['passed'] for r in final['records'])
paired=read(work/'native-startup-edges-r12/comparison.json')
assert {r['case'] for r in paired['records'] if r['variant']=='original' and not r['passed']}=={'original-wasapi','original-dummy','original-nospu-wasapi','single-picture','audio-only-wasapi'}
assert {r['case'] for r in paired['records'] if r['variant']=='candidate' and not r['passed']}=={'audio-only-wasapi'}
corrected=read(work/'native-startup-edges-r13/comparison.json')
assert all(r['passed'] for r in corrected['records'] if r['variant']=='candidate')
assert all(r['passed'] for r in corrected['records'] if r['case']=='audio-only-wasapi')
for row in final['records']:
    result=read(work/'native-startup-ordinary-r14'/row['case']/'ordinary/result.json')
    if row['case']!='single-picture':
        held=[r for r in result['observations'] if r['step'].startswith('held-paused')]
        resumed=next(r for r in result['observations'] if r['step']=='resumed')
        assert resumed['state']==2 and resumed['timeUs']>held[-1]['timeUs']+200_000
audio=read(work/'native-audio-r15/verification.json');bookmarks=read(work/'native-bookmarks-r15/verification.json');video=read(work/'native-video-r15/process.json')
assert audio['passed'] and audio['counts']==dict(passed=7,failed=0,skipped=0) and audio['actualWasapiLogs']==5 and not audio['diagnostics']
assert bookmarks['passed'] and bookmarks['counts']==dict(passed=9,failed=0,skipped=0) and bookmarks['shutdownContextVerified']
assert video['passed'] and video['runtimeUnchanged'] and video['sourceUnchanged'] and video['mediaUnchanged']
for name in ['original-wasapi','preserve-wasapi','original-noaudio']:
    a=read(work/'native-core-trace-r9c'/name/'investigation.json')['observations']
    b=read(work/'native-tracks-r15'/name/'investigation.json')['observations']
    assert [(r['step'],r.get('brightPixels',0)>1000) for r in a[1:]]==[(r['step'],r.get('brightPixels',0)>1000) for r in b[1:]]
trace=(work/'native-core-trace-r9c/original-wasapi/native.log').read_text(encoding='utf-8',errors='replace').splitlines()
first=lambda pattern:next(i for i,l in enumerate(trace) if pattern in l)
assert first('Stream buffering done')<first('WAIT_IDLE_EMPTY cat=1')<first('CHANGE_PAUSE_AFTER cat=1 id=video/1 paused=1')<first('ENQUEUE cat=1')
positive=(work/'native-core-trace-r9c/preserve-wasapi/native.log').read_text(encoding='utf-8',errors='replace').splitlines()
assert next(i for i,l in enumerate(positive) if 'ENQUEUE cat=1' in l)<next(i for i,l in enumerate(positive) if 'Stream buffering done' in l)

draft=work/'startup-checkpoint-r15-draft';assert not draft.exists();draft.mkdir()
target=repo/'doc/juxtopposed-references/native/home-paused-video-startup-repair-20261023';assert not target.exists()
def copy(source,name=None):
    source=work/source;dest=draft/(name if name else source.relative_to(work));dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(source,dest);assert digest(source)==digest(dest)
qualified=[]
def picture(source,name,expected_hash):
    assert digest(work/source)==expected_hash
    im=QImage(str(work/source));assert not im.isNull() and im.width()>0 and im.height()>0
    copy(source,name);qualified.append(name)
for name in ['inputs-r4.json','inputs-r14.json','scope-r14.json','build-core-r14.sh','build-core-r14.log','build-core-r14-exit.txt','core-r14/libvlccore_la-decoder.Plo','prepare-r14.py','prepare-regressions-r15.py','regression-producers-r15.json','run-native-video-r15.py','publish-startup-repair-r15.py','generate-timestamp-controls-r8.sh','compare-timestamp-controls-r8-preflight.py','compare-timestamp-controls-r8b.py','timestamp-control-inputs-r8b.json','generate-startup-edges-r12.sh','prepare-startup-fixture-generator-r14.py','prepare-maintained-startup-test-r13.py','prepare-native-startup-r10.py','native-startup-r10-producers.json']:
    copy(name)
for version in ['r9c','r10','r11','r11b']:
    prefix='core-trace-'+version if version=='r9c' else 'startup-candidate-'+version
    for name in ['source-record.json','decoder.c','decoder-original.c']:copy(prefix+'-source/'+name)
    producer='prepare-'+prefix+'.py';copy(producer)
    if version!='r11':
        for name in [prefix+'.mk','build-'+prefix+'.sh','build-'+prefix+'.log','build-'+prefix+'-exit.txt']:copy(name)
copy('prepare-startup-candidate-r11b-producer.py')
for group in groups:
    copy(group+'.py');copy('run-'+group+'.py');copy(group+'/processes.json')
    for row in read(work/group/'processes.json')['records']:
        root=group+'/'+row['case']
        for name in ['process.json','investigation.json','native.log','stdout.txt','stderr.txt']:copy(root+'/'+name)
        if group!='native-startup-candidate-r10':
            for item in read(work/root/'investigation.json')['observations']:
                if item.get('snapshotAvailable'):
                    picture(root+'/'+item['snapshot'],root+'/'+item['snapshot'],item['snapshotSha256'])
for group,worker,parent in [('native-startup-edges-r12','native-startup-edges-r12.py','run-native-startup-edges-r12.py'),('native-startup-edges-r13','test/libvlc/native_paused_video_startup.py','run-native-startup-edges-r13.py'),('native-startup-ordinary-r14','test/libvlc/native_paused_video_startup.py','run-native-startup-ordinary-r14.py')]:
    if worker.startswith('test/'):
        dest=draft/group/'probe.py';dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(repo/worker,dest)
    else:copy(worker)
    copy(parent);copy(group+'/comparison.json')
    for row in read(work/group/'comparison.json')['records']:
        base=group+'/'+row['case'];root=base+'/'+row['variant'];copy(base+'/config.json') if not (draft/base/'config.json').exists() else None
        for name in ['process.json','result.json','native.log','stdout.txt','stderr.txt']:copy(root+'/'+name)
        result=read(work/root/'result.json');initial=result['observations'][0]
        if initial.get('snapshotAvailableWhilePaused'):
            picture(root+'/initial-paused.png',root+'/initial-paused.png',initial['snapshotSha256'])
for folder in ['ordinary-ui-r14','native-audio-r15','native-bookmarks-r15','native-video-r15']:
    for source in sorted((work/folder).rglob('*')):
        relative=source.relative_to(work/folder)
        if not source.is_file() or relative.parts[0] in ['runtime','userdata','appdata','localappdata','temp','cache'] or source.suffix.lower() not in ['.txt','.json','.log','.xml','.ini','.png'] or source.name=='vlcrc':continue
        copy(str(source.relative_to(work)))
for name in ['run-ordinary-r14.py','run-native-audio-r15.py','run-native-bookmarks-r15.py']:copy(name)
for name in ['native_paused_video_startup.py','native_paused_video_startup.md','generate_paused_video_startup.sh','native_paused_seek.py']:
    dest=draft/'maintained-producers'/name;dest.parent.mkdir(exist_ok=True);shutil.copyfile(repo/'test/libvlc'/name,dest)
copy('source-r4/src/input/decoder.c','production-decoder-before.c');copy('source-r14/src/input/decoder.c','production-decoder-after.c')
ui=dict(selectedStartupObserved=True,fullTrackWorkflowAccepted=False,method='Computer Use screenshots and accessibility observations in this conversation; no standalone desktop screenshot files are claimed.',pid=ordinary['pid'],runtimeSha256=identities,steps=[dict(step='cold-start-paused',visible='Gray video surface and Play control; music placeholder absent; English subtitle text absent.',elapsed='00:00:00',progress=0.0003554256010396361),dict(step='open-subtitle-menu',visible='English subtitle and audio checked; both Finnish tracks and video Track1 listed.',elapsed='00:00:00',progress=0.0003554256010396361),dict(step='escape-menu',visible='Popup closed; keyboard outline on subtitle button; playback remains paused.',elapsed='00:00:00',progress=0.0003554256010396361),dict(step='play',visible='Pause control and elapsed00:00:05 observed; subtitle text absent in this desktop observation.'),dict(step='pause-again',visible='Play symbol and elapsed00:00:29 observed; immediate accessibility capture still named Pause, so no exact additional paused-hold claim.'),dict(step='normal-quit',visible='Ctrl+Q; first window listing transiently retained the closing window, next state capture reported no foreground process, fresh listing returned no VLC window; parent terminal exit0.')],limits=['Initial-picture identity is established by the native PNG probes; the desktop gray surface is visual corroboration only.','No ordinary desktop subtitle-rendering acceptance, exact interaction-latency or warning-free claim.'])
write(draft/'ui-observations.json',ui)
review=dict(passed=True,selectedPausedVideoStartupAccepted=True,fullTrackWorkflowAccepted=False,fullBetaAccepted=False,sourceInputs=2147,ordinaryCoreSha256=identities['libvlccore.dll'],conclusions=['Timestamp-preserving remux starts paused correctly with unchanged A/V packet payloads and PTS: the21ms offset is not a sufficient explanation.','Private trace finds startup buffering ends with audio queued and no video packet received; deferred pause then prevents the first video picture. Remux/no-audio controls receive video before that pause. Logging is diagnostic and can affect timing.','A paused video decoder awaiting its first picture now consumes/requests input independently of explicit user frame-step counts. No audio clock, subtitle demux/selection, decoder layout or GUI change is part of this repair.','Nine ordinary native controls pass. Four old-video failures are retained: original with WASAPI, dummy audio, subtitles disabled, and a single-picture file. Audio-only producer expectation is corrected and paired on actual selected-track APIs.','Native post-resume/seek subtitle pixel-presence pattern matches the earlier trace controls; paused backward-seek and new-track/external-selection gaps remain. Ordinary desktop observations also retain absent subtitle text.'],qualityCorrections=['R10 permits a first video via explicit frame countdown, but static review found that route also emits user frame-step notifications/OSD; retained as a candidate, not accepted. R11b uses first-picture state independently.','R11 generated C contains literal newline-prefix plus characters; inspection caught this before compiling it. R11b corrects the producer and leaves the uncompiled preflight intact.','R12 assumes audio-only has no vout resource. Both builds can retain an empty vout. R13 checks actual selected video tracks instead; both audio-only controls pass. R12 failed records remain unchanged.','The ordinary raw compiledSourceCount is the source-manifest size including headers/producers, not a claim that2147 translation units were compiled.','The earlier audio executable is reused unchanged. Its old source record differs only in two EXTRA_DIST lines in modules/gui/qt/Makefile.am; compiled controller/test sources remain checked.'],retainedDiagnostics=['Empty-buffer prevention remains in original-container and single-picture startup logs; it is not hidden or renamed.','Native window-close messages are retained; audio test has no diagnostics, bookmark2 and video1 closes are checked at teardown.','Build deprecated-FIFO/delay/libtool/host-path warnings and cache/parser/codec/size/lateness warnings remain in raw logs.'],nextSteps=['Trace Matroska subtitle seek points that precede video preroll: the backward5s request emits no SPU packet, unlike the original-container25s request.','Trace selected-track changes and associated subtitle demux while paused, including manual selection precedence and the UI/native display distinction.','Continue full13-category/32-surface beta roadmap: ordinary remaining workflows, Figma/design fidelity, accessibility/DPI, physical outputs, Windows10, review and packaging.'],questionsForMorning=[],sideEffects=dict(installs=False,commit=False,push=False,release=False,upload=False,shutdown=False))
write(draft/'review.json',review)
readme='''# Paused video startup repair, 23 October 2026

A selected video track can now deliver its first picture while playback stays
paused, even when audio fills the initial buffer before any video packet arrives.
The decoder uses its first-picture state independently of user frame-step counts.
Audio clocks, subtitle demux/selection, and the GUI are unchanged by this repair.

The timestamp-preserving remux succeeds with identical A/V payloads and PTS;
the earlier21ms shift was not a sufficient explanation. A separate diagnostic
Core shows the original file pausing with no video packet queued, while the
matching remux receives video before that pause. The diagnostic DLL is not the
ordinary acceptance Core.

Ordinary Core000f1159 with GUI716862a1 passes nine native startup controls,
including real WASAPI, dummy/no audio, disabled subtitles, plain/remuxed video,
audio-only, a single-picture file, and delayed video. Four old video failures
remain recorded. The same corrected probe also checks selected video tracks.
Existing real-controller audio7, bookmark/chapter9, and paused video6 checks pass.
The visible app opens with its video surface, remains paused during track-menu
open/Escape, resumes/Pause, and quits normally. Desktop subtitle text remains
absent in these observations; that workflow is not accepted.

The earlier paused subtitle failures remain: backward seeking and switching or
loading a subtitle while paused miss cues; resume supplies the external cue in
native probes. Post-start subtitle pixel-presence patterns match the earlier
controls. The old buffer warning remains, as do documented native teardown and
build/parser/cache warnings. This is selected startup acceptance, not a clean-log,
all-format, Windows10, physical-output, design/DPI or full-beta acceptance.

R10's frame-count candidate, R11's uncompiled malformed generated source, and
R12's incorrect no-vout audio-only assumption are retained and explained in the
review. R13 corrects that assumption using selected tracks. No scripts or old
receipts were rewritten to turn a failure into a pass. Only snapshots available
while paused qualify; files delivered after resume remain excluded.

- [Checkpoint and artifact hashes](checkpoint.json)
- [Review, corrections and remaining work](review.json)
- [Ordinary nine-control comparison](native-startup-ordinary-r14/comparison.json)
- [Corrected paired probe](native-startup-edges-r13/comparison.json)
- [Original paired edge cases](native-startup-edges-r12/comparison.json)
- [Private decoder trace](native-core-trace-r9c/processes.json)
- [Timestamp controls](timestamp-control-inputs-r8b.json)
- [Ordinary UI observations](ui-observations.json)
- [Native audio replay](native-audio-r15/verification.json)
- [Native bookmark replay](native-bookmarks-r15/verification.json)
- [Native video replay](native-video-r15/process.json)
- [Current source inputs](inputs-r14.json)

The source freeze holds2147 actual copies, including headers and producers.
Historical receipts retain their own binary scopes. Full13-category/32-surface
beta work continues; no installs, commit/push, release/upload or shutdown.
Nonurgent questions stay for morning; none is required for the next trace.
'''
(draft/'README.md').write_text(readme,encoding='utf-8',newline='\n')
artifacts={p.relative_to(draft).as_posix():digest(p) for p in sorted(draft.rglob('*')) if p.is_file()}
checkpoint=dict(passed=True,selectedPausedVideoStartupAccepted=True,subtitleRepairAccepted=False,fullTrackWorkflowAccepted=False,fullBetaAccepted=False,date='2026-10-23',sourceInputs=2147,identities=identities,ordinaryStartupControls=9,pairedOldVideoFailures=4,nativeAudioCases=7,nativeBookmarkCases=9,nativeVideoPictures=6,investigationProcessesNormal=len(all_records),qualifiedInvestigationPictures=len(qualified),qualifiedPicturePaths=qualified,artifacts=artifacts)
write(draft/'checkpoint.json',checkpoint)
assert all(digest(draft/p)==h for p,h in artifacts.items())
shutil.copytree(draft,target)
assert digest(draft/'checkpoint.json')==digest(target/'checkpoint.json') and all(digest(target/p)==h for p,h in artifacts.items())
write(work/'publication-verification-r15.json',dict(passed=True,target=str(target),checkpointSha256=digest(target/'checkpoint.json'),artifacts=len(artifacts),allArtifactHashesVerified=True,sourceInputsVerified=2147,investigationProcessesNormal=len(all_records),selectedStartupAccepted=True,fullBetaAccepted=False))
print(json.dumps(read(work/'publication-verification-r15.json'),indent=2))
