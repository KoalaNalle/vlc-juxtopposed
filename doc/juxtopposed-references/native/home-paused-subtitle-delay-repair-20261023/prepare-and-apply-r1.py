from pathlib import Path
from hashlib import sha256
import ast, json, shutil, subprocess

work = Path(__file__).resolve().parent
repo = work.parent.parent
old = repo/'build-windows/beta-subtitle-preferences-20261023'
read = lambda p: json.loads(p.read_text(encoding='utf-8-sig'))
digest = lambda p: sha256(p.read_bytes()).hexdigest()


def write(p, value):
    p.write_text(json.dumps(value, indent=2)+'\n', encoding='utf-8', newline='\n')


assert not (work/'application-r1.json').exists()
previous = read(old/'final-verification-r8.json')
assert previous['passed'] and previous['privateCandidatePassed']
assert previous['nativeNormalExits'] == 48 and previous['targetedPrivatePasses'] == 11
assert previous['legacyPrivatePasses'] == 24 and not previous['productionDelayRepairApplied']
frozen = read(old/'inputs-boundary-r6.json')
for name, h in frozen['files'].items():
    assert digest(Path(frozen['sourceSnapshot'])/name) == h
    assert digest(repo/name) == previous['onlyPostFreezeChanges'].get(name, h)
candidate = read(old/'candidate-r4.json')
assert all(digest(repo/p) == h for p, h in candidate['preimages'].items())
assert all(digest(old/'candidate-r4'/p) == h for p, h in candidate['files'].items())
receipt = repo/'doc/juxtopposed-references/native/home-paused-subtitle-delay-investigation-20261023'
assert digest(receipt/'checkpoint.json') == 'd45c6e24cfd53faf3cde0fa2cf1a53a5c79560167af4f66982b35365c25e95f9'
assert all(digest(receipt/p) == h for p, h in read(receipt/'checkpoint.json')['artifacts'].items())
assert subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=repo, text=True).strip() == previous['gitHead']
assert not subprocess.check_output(['git', 'diff', '--cached', '--name-only'], cwd=repo, text=True).strip()

# Keep the previous immutable worker/result scopes; adapt only the ordinary
# runtime and phase guards for the same maintained legacy regression workers.
parent = (old/'run-regressions-r5.py').read_text(encoding='utf-8-sig')


def replace_parent(before, after):
    global parent
    assert parent.count(before) == 1, before[:100]
    parent = parent.replace(before, after)


replace_parent("old = repo/'build-windows/beta-tracks-subtitles-20261023'",
               "old = repo/'build-windows/beta-tracks-subtitles-20261023'\n"
               "investigation = repo/'build-windows/beta-subtitle-preferences-20261023'")
replace_parent("manifest = work/'inputs-candidate-r4.json'", "manifest = work/'inputs-ordinary-r1.json'")
replace_parent("assert read(work/'paired-frames-r4.json')['allPairedSeekFramesMatch']",
               "assert read(investigation/'paired-frames-r4.json')['allPairedSeekFramesMatch']")
replace_parent("assert read(work/'paired-frames-r4.json')['allCandidateChecksPass']",
               "assert read(investigation/'paired-frames-r4.json')['allCandidateChecksPass']\n"
               "ordinary = read(work/'native-ordinary-r2/processes.json')\n"
               "assert ordinary['passed'] and len(ordinary['records']) == 11")
replace_parent("summary = read(work/('native-delay-candidate-'+batch+'-r4')/'processes.json')",
               "summary = read(investigation/('native-delay-candidate-'+batch+'-r4')/'processes.json')")
replace_parent("base = work/'runtime'", "base = investigation/'runtime'")
replace_parent("protected = read(work/'reopen-r2/verification.json')['runtimeSha256']",
               "protected = read(investigation/'reopen-r2/verification.json')['runtimeSha256']")
replace_parent("root = work/'regressions-candidate-r5'", "root = work/'regressions-ordinary-r3'")
replace_parent("shutil.copyfile(work/'candidate-r4-bin/libvlccore.dll', runtime/'libvlccore.dll')",
               "shutil.copyfile(work/'ordinary-core-r1/libvlccore.dll', runtime/'libvlccore.dll')")
replace_parent("privateCore=True, privateGui=False, fullBetaAccepted=False,",
               "privateCore=False, privateGui=False, fullBetaAccepted=False,")
ast.parse(parent)
target = work/'run-regressions-r3.py'
assert not target.exists()
target.write_text(parent, encoding='utf-8', newline='\n')

backup = work/'production-preimage-r1'
assert not backup.exists()
backup.mkdir()
for name, h in candidate['preimages'].items():
    target = backup/name
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(repo/name, target)
    assert digest(target) == h
for name, h in candidate['files'].items():
    shutil.copyfile(old/'candidate-r4'/name, repo/name)
    assert digest(repo/name) == h
write(work/'application-r1.json', dict(productionApplied=True, fullBetaAccepted=False,
      files=candidate['files'], preimages=candidate['preimages'],
      privateCoreSha256=previous['privateCoreSha256'],
      review='Exact previously tested seven-file candidate. Decoder delay update signals '
             'its FIFO; positive subtitle seek preroll retains earlier cue data while '
             'video/audio keep the requested display boundary. Track delay overrides '
             'category delay. The shared actual-frame paused refill retains existing '
             'seekability/recording/current-input gates and pending-target behavior. '
             'Only an appended private ES query and a private player declaration are '
             'added; existing enum values, public structures and exported API are unchanged.',
      required='Ordinary compilation, exact-binary native timing/regressions and actual desktop synchronization checks.'))
names = set(frozen['files'])
names.update((old/p).relative_to(repo).as_posix() for p in ['publish-investigation-r7.py', 'finalize-investigation-r8.py'])
names.update((work/p).relative_to(repo).as_posix() for p in [
    'prepare-and-apply-r1.py', 'build-ordinary-r1.sh', 'run-native-r2.py',
    'run-regressions-r3.py', 'run-ui-r4.py'])
forbidden = {'test/libvlc/native_soxr_reference.py',
             'test/modules/audio_filter/soxr_lifecycle_trace.c',
             'test/modules/audio_filter/soxr_native_reference.c',
             'test/modules/audio_filter/soxr_pcm_trace.c'}
assert not names.intersection(forbidden)
snapshot = work/'source-ordinary-r1'
assert not snapshot.exists()
snapshot.mkdir()
inputs = {name: digest(repo/name) for name in sorted(names)}
for name, h in inputs.items():
    target = snapshot/name
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(repo/name, target)
    assert digest(target) == h
write(work/'inputs-ordinary-r1.json', dict(files=inputs, sourceCount=len(inputs),
      sourceSnapshot=str(snapshot), productionApplied=True, fullBetaAccepted=False))
subprocess.run(['git', 'diff', '--check'], cwd=repo, check=True)
print('Applied exact tested candidate;', len(inputs), 'actual ordinary input copies', flush=True)
