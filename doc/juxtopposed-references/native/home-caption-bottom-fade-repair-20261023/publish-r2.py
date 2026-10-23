from pathlib import Path
from hashlib import sha256
import json,shutil,subprocess
work=Path(__file__).resolve().parent;repo=work.parent.parent
target=repo/'doc/juxtopposed-references/native/home-caption-bottom-fade-repair-20261023'
assert not target.exists()
digest=lambda p:sha256(p.read_bytes()).hexdigest()
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
def write(p,value):p.write_text(json.dumps(value,indent=2)+'\n',encoding='utf-8',newline='\n')
before=read(work/'inputs-before-r1.json');after=read(work/'inputs-after-r1.json')
assert before['files'].keys()==after['files'].keys()
changed=[p for p,h in before['files'].items() if after['files'][p]!=h]
assert changed==['modules/gui/qt/player/qml/Player.qml'],changed
for phase,record in [('before',before),('after',after)]:
    assert record['sourceCount']==2242
    root=Path(record['sourceSnapshot'])
    assert all(digest(root/p)==h for p,h in record['files'].items())
assert all(digest(repo/p)==h for p,h in after['files'].items())
assert (work/'build-ordinary-r1-exit.txt').read_text().strip()=='0'
records={case:read(work/('ui-'+case+'-r1/verification.json')) for case in ['before','after','ass','legacy']}
for case,record in records.items():
    assert record['status']=='terminal' and record['normalExit'] and record['exitCode']==0
    assert not record['diagnostics'] and record['sourceInputsUnchanged'] and record['runtimeUnchanged'] and record['mediaUnchanged']
    assert not record['privateGui'] and not record['forcedCleanupUsed']
    assert all(digest(work/('ui-'+case+'-r1/runtime')/p)==h for p,h in record['runtimeSha256'].items())
assert records['before']['runtimeSha256']['plugins/gui/libqt_plugin.dll']=='d368d5ec28ea156b4d9c26c5c58820e4840cdc80b2a1369bc3137ebdfa9d8e57'
for case in ['after','ass','legacy']:
    assert records[case]['runtimeSha256']==records['after']['runtimeSha256']
assert digest(work/'ordinary-r1-bin/libqt_plugin.dll')=='d0ff14c66da4287a36a769754deead89c4ba616fb31fb8e1262c62a9a35e2678'
assert not subprocess.check_output(['git','diff','--cached','--name-only'],cwd=repo,text=True).strip()
assert subprocess.check_output(['git','rev-parse','HEAD'],cwd=repo,text=True).strip()=='3fe8c13bf11a251fe324da19fd0ef3d4eee87abb'
target.mkdir()
names=['prepare-r1.py','freeze-r1.py','run-ui-r1.py','ordinary-r1.mk','build-r1.sh','build-ordinary-r1.log','build-ordinary-r1-exit.txt','inputs-before-r1.json','inputs-after-r1.json','figma-player-247-445.png','figma-player-hidden-3328-11554.png','figma-player-layout-excerpts.txt','figma-hidden-layout-excerpts.txt','morning-question.md',Path(__file__).name]
for name in names:shutil.copyfile(work/name,target/name)
for phase,record in [('before',before),('after',after)]:
    folder=target/('source-'+phase);folder.mkdir()
    shutil.copyfile(Path(record['sourceSnapshot'])/'modules/gui/qt/player/qml/Player.qml',folder/'Player.qml')
for case in records:
    folder=target/('ui-'+case+'-r1');folder.mkdir()
    for name in ['verification.json','vlc.log','stdout.txt','stderr.txt','config-before.txt','config-after.txt']:
        shutil.copyfile(work/folder.name/name,folder/name)
observations=dict(method='Computer Use screenshots and actual Qt accessibility tree of ordinary Windows 11 GUI; reviewed observations, not automated image metrics.',screenshotsStored=False,display=dict(width=2560,height=1440),
    baseline=dict(initial='LONG OVERLAPPING CUE visibly darkened by bottom fade.',at31=dict(paused=True,position=0.5167552264663258,captions=['SECOND SHORT OVERLAY','LONG OVERLAPPING CUE'],lowerCaptionDarkened=True)),
    redesignedSubRip=dict(initial='LONG cue white on red, no redundant bottom gradient.',at31=dict(paused=True,position=0.5167552264663258,bothCaptionsBright=True),menuOpenEscape=dict(positionBefore=0.5167552264663258,positionAfter=0.5167552264663258),backward15=dict(position=0.2502262664290117,color='green',longCueBright=True),resume=dict(elapsedSecondsObserved=20,position=0.343114340923856,pauseButton=True),repaused=dict(elapsedSecondsObserved=25,position=0.42527890589861905,playButton=True)),
    redesignedASS=dict(initial='LONG cue white on red.',at31=dict(paused=True,position=0.5167552264663258,bothCaptionsBright=True,coarseVideoColor='red')),
    legacy=dict(initial='Legacy top/bottom gradients retained.',at31='Both overlap captions and 00:31 visible; legacy lower fade retained. No exact seek-time/focus claim.'),
    limitations=['No saved desktop screenshot artifacts; the original tool observations are in the chat transcript.','No caption placement/clearance change, precise frame PTS, screen-reader speech, DPI, physical audibility or full-beta acceptance.','Popup panel has its own intentional shading, separate from the repaired idle playback fade.'])
write(target/'desktop-observations.json',observations)
review=dict(selectedRepairAccepted=True,fullCaptionLayoutAccepted=False,fullAccessibilityAccepted=False,fullBetaAccepted=False,productionDelta='One visibility guard excludes redesigned controls from the old bottom FadeRectangle. Legacy condition, top fade, video bounds, saved subtitle offsets and libass positions are unchanged.',
    figma=dict(fileKey='zdNUYRVOH9lEtIxMYvh8DV',nodes=['247:445','3328:11554'],sessionId='5b7f2506-94c9-4bf2-a6d8-8f49a2034e61',readOnly=True,scope='Full-size media behind floating controls; no separate 206px bottom UI fade in context. Darker floor is part of the placeholder media, also present with controls hidden. Caption clearance is unspecified.',attribution='Original design and artwork: Juxtopposed. Reference images only; no screenshot is shipped in the application.'),
    build=dict(exitCode=0,ordinaryPlugin=True,privateTestObjects=False,recompiled='Player QML AOT and player resource; ordinary GUI relink.',warnings='Existing static-library portability linker warnings are retained in the full build log.'),
    processCount=4,allNormalExits=True,morningQuestion='morning-question.md',next='Initial/new/external subtitle and track selection, saved language/defaults and synchronization flows. Caption-clearance preference saved for morning.',authorization=dict(installs=False,commit=False,push=False,release=False,shutdown=False))
write(target/'review.json',review)
(target/'README.md').write_text('''# Caption bottom fade repair, 23 October 2026

The redesigned player no longer draws VLC's old bottom fade above its opaque
playback bar. The former fade darkened captions even when they were clear of
the controls. A single visibility guard scopes the change to the redesign.
Legacy controls keep their existing gradient; video bounds, saved subtitle
offsets, authored ASS positions and the top fade stay unchanged.

The unchanged ordinary build reproduces the dark lower line at 31 seconds.
The repaired ordinary build shows both overlapping SubRip and ASS captions
brightly in fullscreen. Backward SubRip seeking, paused menu open/Escape and
resume/Pause work. The legacy comparison retains its old fade. Four ordinary
GUI sessions exit normally with no checked runtime errors or byte drift.
These are reviewed desktop observations, not automated image metrics.

Read-only Figma context and screenshots for Player 247:445 and hidden controls
3328:11554 confirm media behind floating controls. Original design/artwork is
by Juxtopposed. Reference screenshots are not application assets. The design
does not settle caption clearance when controls cover a line in a small window;
that preference is saved for morning, with no subtitle-position change here.

Current runtime: `build-windows/beta-caption-layout-20261023/ui-after-r1/runtime`.
Tuple: GUId0ff14c6 / Core000f1159 / MKVf46d8d15 / WASAPI4e4e.
The before/after freezes each contain 2,242 actual input copies, not compiled
translation-unit counts. Only Player.qml differs between the two freezes.
Existing native subtitle seeking and accessible progress repair receipts retain
their original scopes. Full caption layout, accessibility/DPI, remaining track
flows, Windows 10, independent review and the full beta remain open.

- [Checkpoint and artifact hashes](checkpoint.json)
- [Reviewed desktop observations](desktop-observations.json)
- [Scope and remaining work](review.json)
- [Saved morning question](morning-question.md)
''',encoding='utf-8',newline='\n')
artifacts={p.relative_to(target).as_posix():digest(p) for p in sorted(target.rglob('*')) if p.is_file()}
checkpoint=dict(selectedRepairAccepted=True,fullBetaAccepted=False,artifacts=artifacts,artifactCount=len(artifacts),runtime=records['after']['runtimeSha256'],sourceManifestSha256=digest(work/'inputs-after-r1.json'),sourceCount=2242,onlyCodeChangeBetweenPairedFreezes=changed,allProcessesNormal=True,processCount=4)
write(target/'checkpoint.json',checkpoint)
assert all(digest(target/p)==h for p,h in artifacts.items())
write(work/'publication-r2.json',dict(passed=True,path=target.relative_to(repo).as_posix(),checkpointSha256=digest(target/'checkpoint.json'),artifactCount=len(artifacts)))
print(json.dumps(read(work/'publication-r2.json')))
