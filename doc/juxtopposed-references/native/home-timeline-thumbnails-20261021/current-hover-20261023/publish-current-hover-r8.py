from pathlib import Path
from hashlib import sha256
import json, shutil

work = Path(__file__).resolve().parent
repo = work.parent.parent
target = repo / 'doc/juxtopposed-references/native/home-timeline-thumbnails-20261021/current-hover-20261023'
read = lambda p: json.loads(p.read_text(encoding='utf-8-sig'))
digest = lambda p: sha256(p.read_bytes()).hexdigest()
def write(p, value):
    p.write_text(json.dumps(value, indent=2)+'\n', encoding='utf-8', newline='\n')

assert not target.exists()
out = work / 'ui-current-hover-r7'
run = read(out / 'verification.json')
assert run['status']=='terminal' and run['normalExit'] and run['exitCode']==0
assert all(run[n] for n in ['sourceInputsUnchanged','runtimeUnchanged','fixtureUnchanged','realD3D11Opened'])
assert not any(run[n] for n in ['privateCore','privateGui','privateObserver','forcedCleanupUsed','diagnostics'])
assert all(digest(out / 'runtime' / n)==h for n,h in run['runtimeSha256'].items())
inputs = read(work / 'inputs-current-hover-r7.json')
assert len(inputs['files'])==inputs['sourceCount']==run['sourceInputCount']
assert all(digest(repo / n)==h and digest(Path(inputs['sourceSnapshot']) / n)==h for n,h in inputs['files'].items())
observations = read(work / 'hover-observations-r7.json')
samples = {s['label']:s for s in observations['samples']}
start, end = samples['hold-start'], samples['hold-end']
duration = end['wallMonotonic']-start['wallMonotonic']
assert duration>=300
for sample in [start,samples['hold-midpoint'],end]:
    assert sample['picturePreviewObserved'] and sample['pausedElapsed']=='00:00:41' and sample['previewLabel']=='00:00:41'
    assert sample['hoverTransitions']==start['hoverTransitions'] and sample['deactivations']==start['deactivations']
assert start['hoverTransitions'][-1]['after'] and not start['deactivations']
leave, reentry = samples['intentional-leave'], samples['intentional-reentry']
assert not leave['picturePreviewObserved'] and not leave['hoverTransitions'][-1]['after']
assert reentry['picturePreviewObserved'] and reentry['hoverTransitions'][-1]['after']
assert len(leave['hoverTransitions'])==len(start['hoverTransitions'])+1
assert len(reentry['hoverTransitions'])==len(leave['hoverTransitions'])+1
assert samples['resumed']['playingObserved'] and samples['resumed']['picturePreviewObserved']
assert samples['resumed']['elapsedSeconds']>41
assert observations['normalQuitRequested']
observations.update(controlledHoldSeconds=duration,holdStayedHovered=True,controlledExitAndReentryPassed=True,
                    resumedPlaybackPassed=True,originalLossCauseEstablished=False,fullBetaAccepted=False)
target.mkdir()
for source in [work/'run-current-hover-r7.py',Path(__file__),work/'inputs-current-hover-r7.json',work/'hover-observations-r7.json']:
    shutil.copyfile(source,target/source.name)
for name in ['verification.json','vlc.log','stderr.txt','stdout.txt','vlcrc']:
    shutil.copyfile(out/name,target/name)
write(target/'review.json',observations)
(target/'README.md').write_text(f'''# Current ordinary preview hover, 23 October 2026

The current ordinary build keeps the actual timeline preview visible through an
untouched paused hold of {duration:.3f} seconds. The main video stays paused at41
seconds and the card shows its41-second image at the reviewed start/middle/end.
Built-in named Qt hover logging has no intervening false transition and the
window-focus log has no deactivation during the hold. Pixels are reviewed at
those points; this is not continuous screen recording.

An intentional click on the title strip moves the pointer away and closes the
card, with a matching true-to-false hover transition. Returning to the timeline
restores the picture card with a matching false-to-true transition. The timeline
click also performs its ordinary seek; no special pointer helper is used.
Playback resumes and advances while the preview remains visible, then the app
quits normally with no checked GUI diagnostics. No private observer or GUI/Core
hooks are linked, and no functional hover change was required.

Tuple: Core0ef9dde2 / GUId0ff14c6 / MKVf46d8d15 / WASAPI4e4e. The
{inputs['sourceCount']:,} actual frozen input copies, runtime and fixture verify.
This refreshes the controlled hover result after the subtitle repairs.

The original historical disappearance remains unconfirmed: there was no cursor/
focus trace then, and the user later said they might have moved the mouse.
Both loss of hover and loss of window activity are explicit production preview
eligibility conditions. These observations establish expected behavior on this
run; they do not prove what happened during the earlier event. Retain that
uncertainty rather than inventing a timeout fix. Broader thumbnail visual,
accessibility/DPI, media/source-switch and full-beta gates remain separate.
No install, commit/push, release/upload or shutdown occurred.

- [Reviewed observations and scope](review.json)
- [Ordinary process/log verification](verification.json)
- [Source manifest](inputs-current-hover-r7.json)
- [Artifact hashes](checkpoint.json)
''',encoding='utf-8',newline='\n')
artifacts = {p.name:digest(p) for p in target.iterdir() if p.is_file()}
write(target/'checkpoint.json',dict(artifacts=artifacts,artifactCount=len(artifacts),
    sourceInputCount=inputs['sourceCount'],runtimeSha256=run['runtimeSha256'],controlledHoldSeconds=duration,
    controlledIdleHoverAccepted=True,controlledPointerLeaveReentryAccepted=True,normalExit=True,
    originalHoverLossCauseEstablished=False,privateObserver=False,fullFeatureAccepted=False,fullBetaAccepted=False))
assert all(digest(target/n)==h for n,h in artifacts.items())
write(work/'hover-publication-r8.json',dict(passed=True,path=target.relative_to(repo).as_posix(),
    checkpointSha256=digest(target/'checkpoint.json'),artifactCount=len(artifacts),controlledHoldSeconds=duration,
    originalHoverLossCauseEstablished=False,fullBetaAccepted=False))
print(json.dumps(read(work/'hover-publication-r8.json')))
