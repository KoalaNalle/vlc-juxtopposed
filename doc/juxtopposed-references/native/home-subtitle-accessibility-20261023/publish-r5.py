from pathlib import Path
from hashlib import sha256
import difflib,json,re,shutil,subprocess

work=Path(__file__).resolve().parent
repo=work.parent.parent
old=repo/'build-windows/beta-subtitle-delay-repair-20261023'
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
def write(p,value):p.write_text(json.dumps(value,indent=2)+'\n',encoding='utf-8',newline='\n')
inputs=read(work/'inputs-r3.json')
assert all(digest(repo/p)==h and digest(Path(inputs['sourceCopies'][p]))==h for p,h in inputs['files'].items())
component=read(work/'component-r3/verification.json')
assert component['passed'] and component['caseCount']==15 and not component['messages']
assert (work/'build-ordinary-r3-exit.txt').read_text().strip()=='0'
observation=read(work/'ui-observations-r4.json')
records=[]
for name in ['frame-marker-ass.mkv','frame-marker.mp4']:
    run=read(work/('ui-'+name+'-r3')/'verification.json')
    assert run['status']=='terminal' and run['normalExit'] and run['exitCode']==0
    assert all(run[k] for k in ['sourceInputsUnchanged','runtimeUnchanged','fixturesUnchanged'])
    assert not run['privateGui'] and not run['privateCore'] and not run['forcedCleanupUsed']
    assert digest(work/('ui-'+name+'-r3/runtime/plugins/gui/libqt_plugin.dll'))==run['runtimeSha256']['plugins/gui/libqt_plugin.dll']
    records.append(run)
assert not records[1]['diagnostics']
assert records[0]['diagnostics'] and all(re.match(r'(wasapi|mmdevice|main) error:',line) for line in records[0]['diagnostics'])
ass_log=work/'ui-frame-marker-ass.mkv-r3/vlc.log'
text=ass_log.read_text(encoding='utf-8',errors='replace')
assert 'session disconnected: device removed' in text and 'default device changed: (disabled)' in text
assert '0x88890004' in text and 'EOF reached' in text
assert all('sub-delay=12' in (work/('ui-'+name+'-r3/config-after.txt')).read_text(encoding='utf-8') for name in ['frame-marker-ass.mkv','frame-marker.mp4'])
previous=read(old/'final-verification-r6.json')
assert all(row['runtimeSha256']['libvlccore.dll']==previous['runtimeSha256']['libvlccore.dll'] for row in records)
assert records[0]['runtimeSha256']==records[1]['runtimeSha256']
head=subprocess.check_output(['git','rev-parse','HEAD'],cwd=repo,text=True).strip()
assert head==previous['gitHead']
assert not subprocess.check_output(['git','diff','--cached','--name-only'],cwd=repo,text=True).strip()
subprocess.run(['git','diff','--check'],cwd=repo,check=True)
receipt=repo/'doc/juxtopposed-references/native/home-subtitle-accessibility-20261023'
assert not receipt.exists()
receipt.mkdir(parents=True)
def copy(source,relative):
    target=receipt/relative
    target.parent.mkdir(parents=True,exist_ok=True)
    shutil.copyfile(source,target)
    assert digest(target)==digest(source)
for name in ['preparation-r3.json','preflight-r1.json','probe-qt-bounds-r1.py','probe-qt-bounds-r2.py',
             'qt-bounds-control-r2.json','inputs-r3.json','ordinary-r3.mk','build-ordinary-r3.sh',
             'build-ordinary-r3.log','build-ordinary-r3-exit.txt','prepare-r3.py','freeze-r3.py',
             'run-ui-r3.py','capture-qt-source-r4.py','qt-source-r4.json','ui-observations-r4.json',
             'publish-r5.py','finalize-r6.py']:
    copy(work/name,name)
for root in ['preimages-r3','qt-source-r4']:
    for path in (work/root).rglob('*'):
        if path.is_file():copy(path,path.relative_to(work))
for name in read(work/'preparation-r3.json')['preimages']:
    copy(repo/name,Path('files')/name)
for path in (work/'component-r3').iterdir():
    if path.is_file():copy(path,Path('component-r3')/path.name)
for name in ['frame-marker-ass.mkv','frame-marker.mp4']:
    root='ui-'+name+'-r3'
    for leaf in ['verification.json','config-after.txt','stdout.txt','stderr.txt']:
        copy(work/root/leaf,Path(root)/leaf)
    # Raw local logs retain hardware identifiers. The portable receipt keeps
    # exact-source hashes and a redacted device/error sequence instead.
    log=work/root/'vlc.log'
    selected=[]
    for number,line in enumerate(log.read_text(encoding='utf-8',errors='replace').splitlines(),1):
        if re.search(r'error:|session disconnected|default device changed|state changed: (unplugged|not present|active)|EOF reached|exiting|QApp exec',line):
            line=re.sub(r'\{[^}]+\}', '[endpoint]',line)
            selected.append(dict(line=number,text=line))
    write(receipt/root/'log-review.json',dict(sourceSha256=digest(log),sourcePath=str(log),
          redacted=True,lines=selected))
patch=[]
for name in read(work/'preparation-r3.json')['preimages']:
    before=(work/'preimages-r3'/name).read_text(encoding='utf-8-sig').splitlines(keepends=True)
    after=(repo/name).read_text(encoding='utf-8-sig').splitlines(keepends=True)
    patch.extend(difflib.unified_diff(before,after,fromfile='a/'+name,tofile='b/'+name))
(receipt/'change.patch').write_text(''.join(patch),encoding='utf-8',newline='\n')
readme='''# Subtitle control accessibility and saved-delay help — 23 October 2026

The ordinary GUI now exposes SpinBoxExt numeric bounds through the property
names read by pinned Qt 6.8.3 accessibility. The old Windows value route could
silently ignore nonzero requests because missing bounds became zero. Individual
primary-delay, secondary-delay and speed controls now have accessible names.
The saved default-delay row uses its existing info affordance and backend help
to explain that it applies to the next media. Native player code is unchanged.

## Evidence and limits

- The retained old ordinary desktop observation reports a nonzero write no-op:
  [previous receipt](../home-paused-subtitle-delay-repair-20261023/ui-observations-r4.json).
- Read-only pinned Qt sources show property lookup, Windows range validation and
  the string provider's silent out-of-range return. The isolated stock Qt control
  records missing bounds before the aliases and -10000/10000 after them. It is
  not VLC or Windows UIA. The first no-window preflight failed and is retained.
- Fifteen actual Subtitles/shared-host QML component cases pass without captured
  messages, including clicking the real help control, checking next-media text,
  and proving no settings request is generated. Its supplied model and scale
  variants do not establish native persistence, Windows DPI or visual acceptance.
- Two actual ordinary desktop processes use the same newly compiled GUI and the
  previously accepted ordinary Core. ASS accepts 0/+1200/-1200 ms through Windows
  accessibility with expected paused captions; Reset, 50 ms keyboard adjustment
  and an unchanged value after an excessive +10050 ms request are observed.
  All three controls expose individual names. Actual saved-default help appears
  and its editor stays at1200 while current timing is50; native sub-delay stays12.
- The ASS run exits0 but is NOT a clean audio/playback regression pass. Its log
  records real endpoint removal/default-output disappearance and audio errors.
  It reaches EOF and Home, then quits normally. Physical cause and full device-loss
  recovery are unproven. The failed audio result is not discarded or waived.
- The subsequent MP4 run accepts 0/+1200 ms, changes the held caption correctly,
  resumes with captions at25 seconds and quits directly during playing. It exits0
  with no checked runtime errors. No audibility or endpoint-loss proof is inferred.
- Source, runtime and generated fixtures remain unchanged during both runs.
  Visible held-frame patterns/coarse elapsed are retained; normalized progress
  reconciles by about1us in some adjustments, so exact GUI numeric conservation
  is not accepted. Secondary timing, subtitle speed and screen-reader use remain
  separate unfinished workflows. The unchanged Core's35 prior native passes keep
  their earlier scope; they were not rerun or counted as new GUI passes.

## Build and continuation

The ordinary overlay target recompiles all three changed QML units and links the
regular plugin object list without private hooks or QtTest. Existing static-library
portability warnings are retained. Runtime identity is recorded in both GUI parent
results: GUI9dd58c64 / Core7ab80750 / MKVf46d8d15 / WASAPI4e4e.

The2407-input freeze verifies every current source and its actual snapshot copy.
It reuses2394 unchanged immutable copies and writes only13 new copies to avoid
duplicating whole source snapshots on C:. The four changed source/test preimages
and final files are included here. Raw local logs remain in the ignored workspace;
portable log reviews retain their hashes and redact endpoint identifiers.

Continue per-track/secondary delay, subtitle FPS/speed and estimator contracts,
then the remaining settings/workflows,32-surface design/accessibility/OS-DPI pass,
consolidated media, physical-output checks, independent review and clean package.
The FPS control's initialization and secondary-delay notification use different
scale directions; verify the intended native contract before editing it.

The controlled hover hold passed previously; its original historical loss still
has no confirmed cause. Full thirteen-category/32-surface beta remains ACTIVE.
Windows10 is separately deferred. No install, commit/push, release/upload or shutdown.
'''
(receipt/'README.md').write_text(readme,encoding='utf-8',newline='\n')
artifacts={p.relative_to(receipt).as_posix():digest(p) for p in sorted(receipt.rglob('*')) if p.is_file()}
write(receipt/'checkpoint.json',dict(artifacts=artifacts,fullBetaAccepted=False,
      guiRuntimeSha256=records[1]['runtimeSha256'],sourceManifestSha256=digest(work/'inputs-r3.json'),
      componentCases=15,desktopNormalExits=2,cleanPlaybackDesktopPasses=1,
      retainedInterruptedAudioRuns=1))
publication=dict(path=receipt.relative_to(repo).as_posix(),checkpointSha256=digest(receipt/'checkpoint.json'),
                 artifactCount=len(artifacts),producerSha256=digest(Path(__file__)))
write(work/'publication-r5.json',publication)
for name in ['PROJECT_HANDOFF.md','doc/juxtopposed-beta-roadmap.md','doc/juxtopposed-settings-consumer-audit.md']:
    path=repo/name
    text=path.read_text(encoding='utf-8-sig')
    assert '<!-- subtitle-accessibility-status:start -->' not in text
    link=('doc/' if name=='PROJECT_HANDOFF.md' else '')+'juxtopposed-references/native/home-subtitle-accessibility-20261023/README.md'
    block=f'''<!-- subtitle-accessibility-status:start -->
Latest selected repair (23 October): [Subtitle control accessibility]({link})
is applied and accepted in its stated control/help scope. The shared spinner
exposes Qt's numeric bounds, and all three synchronization controls have names.
Ordinary Windows accessibility writes now update primary timing and the paused
caption: ASS0/+1200/-1200ms, MP4 0/+1200ms. Reset, focused50ms Up and ignored
excessive+10050ms input retain their recorded ASS scope. Saved-delay help now
explains next-media application through the existing authored info affordance.
Its actual editor stays1200 while current timing is50; native sub-delay stays12.

Fifteen component cases pass with no captured messages. Two ordinary desktop
processes exit0 on GUI9dd58c64 / Core7ab80750 / MKVf46d8d15 / WASAPI4e4e. The ASS
run retains endpoint-removal/default-output disappearance and audio errors; it
is NOT a clean playback/audio pass. The subsequent MP4 run resumes with captions
and quits while playing with no checked runtime errors. No device-loss recovery
or audibility claim follows. Previous35 native Core passes retain their earlier
scope and were not repeated for this GUI-only change.

Current runtime: `build-windows/beta-subtitle-accessibility-20261023/ui-frame-marker.mp4-r3/runtime`.
The2407-input freeze reuses2394 actual immutable copies and adds13 changed/helper
copies, avoiding another full C: snapshot. Both runs retain source/runtime/fixture
guards. Exact GUI numeric-position conservation remains unqualified; visible held
frame patterns/coarse elapsed stay fixed while some values reconcile by about1us.

Next: per-track/secondary timing, FPS/speed and estimators. The FPS control's
initialization and secondary-delay notification use differing scale directions;
verify its native contract. Continue all remaining consumers/workflows,32-surface
Figma/design/accessibility/OS-DPI, consolidated media, physical outputs, review
and clean packaging. Full beta remains ACTIVE; Windows10 stays separately deferred.
The prior controlled hover hold passed; original historical hover loss remains
unconfirmed. Older status blocks below are historical. No install, commit/push,
release/upload or shutdown occurred; both owned desktop processes are closed.
<!-- subtitle-accessibility-status:end -->
'''
    heading,body=text.split('\n\n',1)
    path.write_text(heading+'\n\n'+block+'\n'+body,encoding='utf-8',newline='\n')
print('Published',len(artifacts),'immutable artifacts and updated three current-status documents.',flush=True)
