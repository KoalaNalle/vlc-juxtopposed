from pathlib import Path
from hashlib import sha256
import json,re,shutil
work=Path(__file__).resolve().parent;repo=work.parent.parent
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
source=read(work/'inputs-ordinary-r3.json');assert source['sourceCount']==2232
protected={'test/libvlc/native_soxr_reference.py','test/modules/audio_filter/soxr_lifecycle_trace.c','test/modules/audio_filter/soxr_native_reference.c','test/modules/audio_filter/soxr_pcm_trace.c'}
assert not protected.intersection(source['files'])
assert all(digest(repo/p)==h and digest(work/'source-ordinary-r3'/p)==h for p,h in source['files'].items())
before=read(work/'native-before-r1b/process.json');after=read(work/'native-after-r1b/process.json')
assert before['normalExit'] and not before['passed'] and before['journey']['error']=='accessibility or pointer action reaches native seek'
assert after['normalExit'] and after['passed'] and len(after['journey']['checks'])==39 and not after['diagnostics']
assert all(r['nativeSeekCount']==1 for r in after['journey']['requests'])
gui=read(work/'ordinary-ui-r4/verification.json')
assert gui['normalExit'] and gui['exitCode']==0 and gui['runtimeUnchanged'] and gui['sourceInputsUnchanged'] and gui['mediaUnchanged'] and not gui['privateGui']
assert all(digest(work/'ordinary-ui-r4/runtime'/p)==h for p,h in gui['runtimeSha256'].items())
assert read(work/'owned-process-audit-r5.json')==[]
dll=(work/'ordinary-r3-bin/libqt_plugin.dll').read_bytes()
assert b'VLC_OWNED_UI_REPORT' not in dll and b'VLC_OWNED_HOVER_TRACE' not in dll
log=(work/'build-ordinary-r3.log').read_text(encoding='utf-8',errors='replace')
links=[l for l in log.splitlines() if '--mode=link' in l and 'libqt_plugin.la' in l]
assert len(links)==1 and 'native-ui-journey.o' not in links[0] and 'libQt6Test' not in links[0]
assert gui['runtimeSha256']['plugins/gui/libqt_plugin.dll']==digest(work/'ordinary-r3-bin/libqt_plugin.dll')
oldroot=repo/'doc/juxtopposed-references/native/home-paused-subtitle-interval-repair-20261023'
old=read(oldroot/'checkpoint.json');assert all(digest(oldroot/p)==h for p,h in old['artifacts'].items())
desktop=dict(selectedAccessibleProgressSeekAccepted=True,fullAccessibilityAccepted=False,captionClearanceAccepted=False,fullBetaAccepted=False,
    method='Live Computer Use desktop accessibility value-setting with observed video colors and accessibility text. Visual checks are not exact frame-PTS or exported pixel-mask comparisons.',
    runtimeSha256=gui['runtimeSha256'],windowId=11666784,pid=gui['pid'],
    observations=[
        dict(step='startup',progress=.0003554256010396361,elapsed='00:00:00',color='red',button='Play'),
        dict(step='accessible-forward',requested=.42,progress=.42020847703303843,elapsed='00:00:25',color='blue',button='Play'),
        dict(step='accessible-backward',requested=.09,progress=.09032393662218223,elapsed='00:00:05',color='red',button='Play'),
        dict(step='accessible-further-forward',requested=.75,progress=.7500930174438947,elapsed='00:00:45',color='green',button='Play'),
        dict(step='physical-progress-click',progress=.7420958164642375,elapsed='00:00:44',button='Play'),
        dict(step='right-key',progress=.9093706036220656,elapsed='00:00:54',button='Play'),
        dict(step='left-key',progress=.7434286832941803,elapsed='00:00:44',button='Play'),
        dict(step='accessible-after-keyboard',requested=.26,progress=.260264457439896,elapsed='00:00:15',color='green',button='Play'),
        dict(step='resume',progress=.3343723030272738,elapsed='00:00:20',button='Pause'),
        dict(step='pause-again',progress=.46468585994901784,elapsed='00:00:27',button='Play'),
        dict(step='later-paused-observation',progress=.46468585994901784,elapsed='00:00:27',button='Play'),
        dict(step='quit',exitCode=0,forcedCleanup=False),
    ],
    limits=['Physical arrow keys followed the observed ten-second global hotkey route. UIA continued reporting title text as focused; this does not qualify slider-specific focus, short-step routing or screen-reader speech.','Captions remain covered by controls in the small window. No caption layout change is included.','The raw GUI wrapper retains passed:false/observationsRequired:true. This separate reviewed observation record qualifies only the selected progress-value repair.'],questionsForMorning=[])
write(work/'desktop-observations-r5.json',desktop)
caption=dict(implemented=False,findings=['Player.qml anchors VideoSurface across the overlay viewport when controls are unpinned. The bottom bar then covers the lower caption.','The redesigned playback bar supplies its own background but the outer controlBar.background can be invisible, allowing the old bottom fade to darken captions above it.','sub-margin is a saved appearance/preset setting and is expressed in destination pixels; changing it for temporary controls would affect user settings semantics.','libass renders absolute TOP|LEFT regions, including normally styled ASS. A generic relative-bottom margin adjustment alone cannot qualify positioned ASS or preserve all of its layouts.'],
    sourceSha256={p:digest(repo/p) for p in ['modules/gui/qt/player/qml/Player.qml','modules/gui/qt/player/qml/ControlBar.qml','modules/codec/libass.c','src/video_output/vout_subpictures.c']},
    next='Check the design/video reference and runtime safe-area options while preserving pin-controls policy, saved offsets, overlap geometry and positioned subtitles. Remove redundant redesigned bottom shading as a separate bounded change if confirmed.')
write(work/'caption-investigation-r5.json',caption)
draft=work/'accessibility-r5-draft';assert not draft.exists();draft.mkdir()
target=repo/'doc/juxtopposed-references/native/home-progress-accessibility-repair-20261023';assert not target.exists()
def copy(relative,name=None):
    p=work/relative;dest=draft/(name if name else relative);dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,dest);assert digest(p)==digest(dest)
for p in sorted(work.iterdir()):
    if p.is_file() and p.suffix in {'.py','.sh','.mk','.json','.log','.txt','.cpp'}:copy(p.name)
for folder in ['native-before-r1','native-before-r1b','native-after-r1b','ordinary-ui-r4']:
    for p in sorted((work/folder).iterdir()):
        if p.is_file() and p.suffix in {'.json','.log','.txt'}:copy(p.relative_to(work).as_posix())
for phase,manifest in [('before','inputs-before-r1b.json'),('after','inputs-ordinary-r3.json')]:
    value=read(work/manifest)
    assert all(digest(Path(value['sourceSnapshot'])/p)==h for p,h in value['files'].items())
    for relative in ['modules/gui/qt/player/qml/SliderBar.qml','modules/gui/qt/Makefile.am']:
        copy(Path(value['sourceSnapshot']).relative_to(work).as_posix()+'/'+relative,'production-'+phase+'/'+relative)
for relative in ['modules/gui/qt/tests/native_ui/seek_accessibility_journey.cpp','modules/gui/qt/tests/native_ui/seek_accessibility_journey.md']:
    copy('source-ordinary-r3/'+relative,'maintained-tests/'+relative)
review=dict(selectedAccessibleProgressSeekAccepted=True,fullAccessibilityAccepted=False,captionClearanceAccepted=False,fullBetaAccepted=False,
    ordinaryIdentities=gui['runtimeSha256'],sourceInputs=2232,sourceManifestSha256=digest(work/'inputs-ordinary-r3.json'),
    ordinaryLinkCommandSha256=sha256(links[0].encode()).hexdigest(),ordinaryLinkExcludesPrivateQaAndQtTest=True,
    qtValueInterfaceSource=dict(path='/root/vlc-juxtopposed-build/contrib/contrib-win64/qtdeclarative/src/quick/accessible/qaccessiblequickitem.cpp',sha256='15fbdc474ec930e3f40c0b678939282677403a06904f5bac1e4ce1267d5f093e',version='6.8.3',lines='689-706',finding='setCurrentValue writes the QML value property; it does not request VLC playback seeking.'),
    repair='Guard internal engine/drag display writes. An external accessible value write in the released state follows the existing seek function once, with normalized bounds. Preserve held/wrong-input protection, chapter/pointer paths, native seek-speed policy and existing keyboard/step handlers.',
    evidence=['Corrected unchanged build has a working video, pause and real accessible value interface but no native seek after setCurrentValue.','Candidate passes39 actual-player checks, with one native request per accessible/pointer action and no feedback requests from playing-position updates.','Ordinary desktop value-setting reaches blue25s, red5s, green45s and green15s segments while paused. Pointer/global arrow routing and resume/Pause/normal Quit also work.','Legacy/designed seek-handle hover/focus/chapter appearance smoke passes.'],
    corrections=['Initial private test failed its combined cold-paused readiness condition before reaching accessibility; it is retained as a setup result, not an accessibility negative. No per-condition breakdown established which readiness predicate was late. Corrected setup follows existing seek tests: normal video/statistics publication, then pause.','Before compiling R1b, its byte-identical phase-named make include was added and the preflight manifest retained before extension.','The appended test distribution line initially used CRLF inside an LF file. Only its final newline was normalized before the ordinary source freeze; whitespace checks pass.','Paused player time can lag a seek. Native picture counts and requested controller positions alone are insufficient to prove exact content; ordinary desktop coarse colors supply separate content observations. No exact PTS claim.'],
    previousImmutableReceiptSha256=digest(oldroot/'checkpoint.json'),remaining=['Caption clearance and redundant bottom fade; preserve saved offsets and authored ASS layout.','Slider-specific focus/keyboard routes and screen-reader speech, wider accessibility/DPI.','Remaining track/subtitle workflows and full13-category/32-surface roadmap.'],questionsForMorning=[],
    sideEffects=dict(installs=False,commit=False,push=False,release=False,upload=False,shutdown=False))
write(draft/'review.json',review)
(draft/'README.md').write_text('''# Progress accessibility repair, 23 October 2026

Direct accessibility value-setting now commits a seek in the ordinary player.
Qt's value interface writes the QML value property; the earlier progress bar
displayed that property but did not send it to VLC. The repair guards internal
display updates and routes an external value write through the existing seek
function once. Pointer/chapter state and the native seek-speed policy remain.

The unchanged build reproduces the missing native request. The candidate
passes39 actual-player checks, including forward/backward values, normalized
bounds, identical-value suppression, pointer compatibility, resume and no seek
feedback during normal playback updates. The private QtTest fixture stays out
of the ordinary DLL. Its first cold-pause setup failure remains separately.

Ordinary desktop accessibility requests show the expected blue25s, red5s and
green45s/15s video segments while paused. Pointer input, the observed global
arrow-key route, resume/Pause and Quit0 work. Final paused position stays exact.
These are coarse visual observations, not exact frame PTS or speech/focus tests.

Current runtime: `build-windows/beta-caption-accessibility-20261023/ordinary-ui-r4/runtime`.
Tuple: GUId368d5ec / Core000f1159 / MKVf46d8d15 / WASAPI4e4e.
The2,232-input freeze has actual copies in source-ordinary-r3. It is not a
translation-unit count. Three private native jobs and one ordinary GUI job
close normally; earlier successful receipts remain immutable.

Caption clearance is still open. The small-window bar covers the lowest line,
and redundant overlay shading darkens fullscreen captions. Saved subtitle
offsets and positioned ASS require care; no caption layout change is included.
Full accessibility/DPI, remaining track workflows and the full beta remain open.
No installs, commit/push, release/upload or shutdown occurred.

- [Checkpoint and hashes](checkpoint.json)
- [Review and remaining work](review.json)
- [Ordinary desktop observations](desktop-observations-r5.json)
- [Native candidate39 checks](native-after-r1b/journey.json)
- [Unchanged-build failure](native-before-r1b/journey.json)
- [Caption source investigation](caption-investigation-r5.json)
''',encoding='utf-8',newline='\n')
artifacts={p.relative_to(draft).as_posix():digest(p) for p in sorted(draft.rglob('*')) if p.is_file()}
checkpoint=dict(selectedAccessibleProgressSeekAccepted=True,fullAccessibilityAccepted=False,captionClearanceAccepted=False,fullBetaAccepted=False,date='2026-10-23',
    ordinaryIdentities=gui['runtimeSha256'],sourceInputs=2232,nativePrivateProcessesNormal=3,ordinaryGuiProcessesNormal=1,candidateNativeChecks=39,
    previousImmutableReceiptSha256=digest(oldroot/'checkpoint.json'),artifacts=artifacts)
write(draft/'checkpoint.json',checkpoint);assert all(digest(draft/p)==h for p,h in artifacts.items())
shutil.copytree(draft,target)
assert digest(target/'checkpoint.json')==digest(draft/'checkpoint.json') and all(digest(target/p)==h for p,h in artifacts.items())
result=dict(publicationVerified=True,target=str(target),checkpointSha256=digest(target/'checkpoint.json'),artifacts=len(artifacts),candidateNativeChecks=39,selectedAccessibleProgressSeekAccepted=True,fullBetaAccepted=False)
write(work/'publication-r5.json',result);print(json.dumps(result,indent=2))
