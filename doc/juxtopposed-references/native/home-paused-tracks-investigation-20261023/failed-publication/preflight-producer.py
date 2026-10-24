from pathlib import Path
from hashlib import sha256
import json,re,shutil
from PySide6.QtGui import QImage
repo=Path('C:/Modding/vlc-juxtopposed');work=Path(__file__).resolve().parent
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
inputs=read(work/'inputs-r4.json');assert len(inputs)==2139
assert all(digest(repo/p)==h and digest(work/'source-r4'/p)==h for p,h in inputs.items())
out=repo/'doc/juxtopposed-references/native/home-paused-tracks-investigation-20261023';assert not out.exists();out.mkdir();artifacts={}
def copy(path,name):
    dest=out/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(path,dest);artifacts[name]=digest(dest)
matrix=[];pictures=0
for revision in ['r5c','r6']:
    group=work/('native-startup-'+revision);processes=read(group/'processes.json')
    assert processes['allProcessesNormal'] and processes['investigationOnly'] and not processes['passed']
    copy(group/'processes.json',revision+'/processes.json')
    for process in processes['records']:
        folder=group/process['case'];value=read(folder/'investigation.json')
        assert process['normalExit'] and process['exitCode']==0 and not process['forcedCleanupUsed']
        assert process['sourceInputsUnchanged'] and process['runtimeUnchanged'] and process['producerUnchanged']
        assert value['teardownComplete'] and value['stopReached'] and value['stopReturn']==0
        assert value['initialPauseReached'] and value['playedPauseReached'] and value['resumeReached']
        assert value['producerSha256']==process['producerSha256']==digest(work/('native-startup-'+revision+'.py'))
        assert process['parentProducerSha256']==digest(work/('run-native-startup-'+revision+'.py'))
        observations={e['step']:e for e in value['observations']};initial=observations['initial-paused'];played=observations['played-paused']
        assert initial['state']==3 and played['state']==3 and initial['vouts']==1 and played['snapshotAvailable']
        audio=not '--no-audio' in value['options'];subtitle='--no-spu' not in value['options']
        assert initial['snapshotAvailable']==(not audio)
        lines=(folder/'native.log').read_text(encoding='utf-8',errors='replace').splitlines()
        assert any('using vout display module "vdummy"' in line for line in lines)
        assert any('using vout window module "window"' in line for line in lines)
        assert ('main error: buffer deadlock prevented' in value['diagnostics'])==audio
        expectedErrors=(['main error: buffer deadlock prevented'] if audio else [])+['main error: window closed']
        assert value['diagnostics']==expectedErrors
        index=lines.index('main error: window closed')
        assert any('stop free vout' in line for line in lines[max(0,index-18):index])
        assert any('main debug: exiting'==line for line in lines[index+1:index+12])
        actualAudio='none'
        if '--aout=adummy' in value['options']:
            assert any('using audio output module "adummy"' in line for line in lines);actualAudio='dummy'
        elif audio:
            assert value['actualWasapiSelected'];actualAudio='WASAPI'
        else:assert not value['actualWasapiSelected']
        if '--avcodec-threads=1' in value['options']:
            assert 'avcodec debug: allowing 1 thread(s) for decoding' in value['actualVideoDecoderLines']
        else:assert 'avcodec debug: using frame thread mode with 6 threads' in value['actualVideoDecoderLines']
        if subtitle:
            assert observations['played-paused']['brightPixels']>1000 and observations['seek-middle-paused']['brightPixels']>1000
            for step in ['seek-first-paused','finnish-selected-paused','external-added-selected-paused','external-explicit-selected-paused']:
                assert observations[step]['brightPixels']==0 and observations[step]['state']==3
            assert value['finnishSelected'] and value['externalAddReturn']==0 and value['externalTrackAdded'] and value['externalExplicitSelected']
            added=observations['external-added-selected-paused']['subtitleTracks'];explicit=observations['external-explicit-selected-paused']['subtitleTracks']
            assert [t['language'] for t in added if t['selected']]==['fin']
            assert len([t for t in explicit if t['selected'] and t['language'] is None])==1
            assert observations['external-after-resume']['brightPixels']>1000
        for entry in value['observations']:
            if not entry.get('snapshotAvailable'):continue
            imagePath=folder/entry['snapshot'];assert digest(imagePath)==entry['snapshotSha256']
            im=QImage(str(imagePath)).convertToFormat(QImage.Format_RGBA8888);pixels=bytes(im.bits())
            bright=sum(1 for i in range(0,len(pixels),4) if pixels[i]>160 and pixels[i+1]>160 and pixels[i+2]>160)
            assert bright==entry['brightPixels'] and im.width()==960 and im.height()==540
            copy(imagePath,process['case']+'/'+entry['snapshot']);pictures+=1
        for name in ['process.json','investigation.json','native.log','stdout.txt','stderr.txt']:copy(folder/name,process['case']+'/'+name)
        matrix.append(dict(case=process['case'],audio=actualAudio,subtitles=subtitle,threads=1 if '--avcodec-threads=1' in value['options'] else 6,initialSnapshotAvailable=initial['snapshotAvailable'],startupBufferDeadlock=audio,backwardCueMissing=subtitle,externalCueAfterExplicitSelectAndResume=subtitle,
            delayedInitialFileExists=(folder/'initial-paused.png').exists() if not initial['snapshotAvailable'] else False,
            closeNotifyAtTeardownVerified=True,normalExit=True,pid=process['pid']))
assert len(matrix)==9 and sum(m['initialSnapshotAvailable'] for m in matrix)==3 and pictures==36
copy(work/'inputs-r4.json','source-inputs.json')
for revision in ['r5c','r6']:
    for prefix in ['native-startup-','run-native-startup-']:copy(work/(prefix+revision+'.py'),'producers/'+prefix+revision+'.py')
for name in ['generate-fixtures-r1.sh']:copy(work/name,'producers/'+name)
for name in ['multi-probe.json','plain-probe.json','embedded-english.srt','embedded-finnish.srt','manual-external.srt']:copy(work/'media-r1'/name,'fixtures/'+name)
for revision in ['r5','r5b']:
    folder=work/('native-startup-'+revision)/'multi-spu-noaudio'
    for name in ['process.json','stderr.txt','stdout.txt']:
        copy(folder/name,'failed-'+revision+'/'+name)
    copy(work/('native-startup-'+revision+'.py'),'failed-'+revision+'/producer.py')
review=dict(passed=False,investigationComplete=True,repairAccepted=False,fullBetaAccepted=False,
    findings=['All six audio-enabled cases lack an initial snapshot during the bounded paused observation and log buffer deadlock prevented. Three no-audio cases produce a paused picture. Dummy audio reproduces it too; plain one-audio/no-subtitle Matroska reproduces with WASAPI. The necessary factor in this matrix is active audio, not subtitles, multiple tracks or the six-thread decoder setting.',
        'After a25s request, snapshots show ENGLISH MIDDLE CUE. A5s backward request then has no subtitle text in all four subtitle-enabled cases, including no-audio and dummy-audio controls. Current renderer/decoder paths can produce text, but this paused transition fails below the QML menu.',
        'Adding external SRT with the public API select flag true leaves manually chosen Finnish selected. Explicit selection chooses Track3, but its cue is absent while paused; resume produces EXTERNAL FIRST CUE. Sticky track-ID selection precedence needs tracing before choosing a repair.'],
    visualInspection='The WASAPI snapshots were directly inspected: seek-middle-paused shows ENGLISH MIDDLE CUE, seek-first-paused is plain gray, external-after-resume shows EXTERNAL FIRST CUE. Pixel counts corroborate presence/absence, not text recognition or arbitrary subtitle correctness.',
    scope=['Nine owned native LibVLC runs with actual Matroska/H264/AAC/SRT media, vdummy display and the native window provider named window. No wdummy/headless-window claim: the first unsupported --window=wdummy attempt is preserved as setup failure.',
        'All nine processes stop/release and exit0 with source/runtime/producer identities unchanged. Each retains one window-closed diagnostic verified at vout teardown before instance exit; six retain the startup error. Cache-missing, subtitle-codec fallback, size and occasional lateness warnings stay in original logs.',
        'A snapshot call returning0 queues a request. The six missing initial files were unavailable during the two-second wait after one-second paused settling; files that appear later after resume are not initial-paused evidence and are excluded from the36 qualified snapshot artifacts.',
        'Native get_time output-clock readback can remain stale while paused. Requested25s/5s values name operations, not exact frame-PTS acceptance. This investigation does not replace the earlier Qt pending-seek repair or native color-frame regression.',
        'The native worker/header/layout/source inputs are recorded against existing source-r4 and GUI716862a1/Core4422/WASAPI4e4e. No production source or installed dependency changes in this investigation; no independent compiler provenance beyond earlier build receipts is invented.',
        'R5 failed before initialization because window is not a command-line option. R5b executed media/Stop/release but failed report finalization on a missing Python import; its incomplete original receipt stays unchanged. R5c corrects the producer and adds explicit external-track selection. These are test-producer failures, not additional app crashes.',
        'No whole workflow, exact target-frame timestamp, physical audibility, all formats, real service, Windows10, design/accessibility/DPI or beta/package acceptance.'],
    next='Trace category-specific decoder Wait/first-picture readiness and pause transitions in a private Core build, using active-audio and no-audio controls. Separately trace subtitle packet/cue delivery through backward seek and track replacement. Keep ordinary Core unchanged until a cause and paired regression are established.')
write(out/'review.json',review);artifacts['review.json']=digest(out/'review.json')
write(out/'matrix.json',dict(passed=False,investigationOnly=True,records=matrix));artifacts['matrix.json']=digest(out/'matrix.json')
write(out/'checkpoint.json',dict(passed=False,investigationComplete=True,repairAccepted=False,fullBetaAccepted=False,date='2026-10-23',nativeRuns=9,normalExits=9,qualifiedSnapshots=36,sourceInputs=2139,sourceFreeze='build-windows/beta-tracks-subtitles-20261023/source-r4',sourceManifestSha256=digest(work/'inputs-r4.json'),artifacts=artifacts))
rows=['| Case | Audio | Subtitles | Threads | Initial paused picture |','| --- | --- | --- | --- | --- |']
for m in matrix:rows.append('| '+m['case']+' | '+m['audio']+' | '+('on' if m['subtitles'] else 'off')+' | '+str(m['threads'])+' | '+('available' if m['initialSnapshotAvailable'] else 'missing')+' |')
(out/'README.md').write_text('''# Paused video/subtitle native investigation, 23 October 2026

This investigation reproduces two open playback failures below the QML menu.
It accepts no repair or whole workflow. The ordinary application remains on
GUI716862a1/Core4422fbb0/WASAPI4e4e, with the selected menu-input repair intact.

All audio-enabled controls lack an initial paused picture, including dummy
audio, one decoder thread, disabled subtitles and a one-audio/no-subtitle file.
No-audio controls produce the paused picture. The next trace should follow the
shared audio/video buffering and pause path; this is not a WASAPI-only finding.

'''+ '\n'.join(rows)+'''

All four subtitle-enabled cases render the middle cue after a25s request but
lose text after a5s backward request while paused. Explicitly selecting the
external track also leaves no visible paused cue; resume shows its first cue.
The public add-subtitle API's select flag leaves the manually selected Finnish
track active, so explicit track-ID precedence is a further investigation point.

The36 qualified native PNGs include positive and missing-text controls. Direct
inspection confirms ENGLISH MIDDLE CUE, an empty gray frame after backward seek,
and EXTERNAL FIRST CUE after resume. Native output-clock readback can stay stale
while paused; this does not claim exact video PTS or replace prior seek evidence.

Nine processes stop/release and exit normally0 with no identity drift. Every
run retains a window-close diagnostic at vout teardown; six also retain the
startup buffer-deadlock error. Cache/fallback/size/lateness warnings are retained.
The native window provider is named window and the display is vdummy. No
windowless, clean-log, physical-audibility or whole-beta claim is made.

Initial snapshot calls queue successfully but six files do not arrive in the
bounded paused observation. Files appearing later after resume are excluded
from initial-paused evidence. Earlier producer setup/report-finalization errors
are preserved separately; no production code changed to make this matrix run.

Next: a private category-specific decoder wait/pause trace with active-audio
and no-audio controls, then separate subtitle packet/cue/selection tracing.
Keep the ordinary Core unchanged until paired evidence supports a repair.
The full13-category/32-surface beta roadmap remains open.

- [Checkpoint and artifact hashes](checkpoint.json)
- [Comparison matrix](matrix.json)
- [Findings, source boundaries, failures and limitations](review.json)
- [WASAPI native observations](multi-spu-wasapi/investigation.json)
- [WASAPI source log](multi-spu-wasapi/native.log)
- [Middle cue after25s request](multi-spu-wasapi/seek-middle-paused.png)
- [Missing cue after5s backward request](multi-spu-wasapi/seek-first-paused.png)
- [External first cue after resume](multi-spu-wasapi/external-after-resume.png)
- [No-audio positive startup](multi-spu-noaudio/initial-paused.png)
- [Dummy-audio comparison](multi-spu-dummy/investigation.json)
- [Frozen source inputs](source-inputs.json)
''',encoding='utf-8',newline='\n')
print(json.dumps(dict(investigationComplete=True,repairAccepted=False,runs=9,normalExits=9,qualifiedSnapshots=pictures,artifacts=len(artifacts))))
