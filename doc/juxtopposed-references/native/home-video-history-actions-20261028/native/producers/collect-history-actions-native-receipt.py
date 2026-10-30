"""Copy reviewed native evidence; never run a build, test, or runtime."""
from pathlib import Path
from hashlib import sha256
from datetime import datetime, timezone
import difflib
import json
import re
import shutil
import subprocess

repo = Path(__file__).resolve().parents[2]
work = Path(__file__).resolve().parent
out = repo / 'doc/juxtopposed-references/native/home-video-history-actions-20261028/native'
out.mkdir(parents=True, exist_ok=False)
copies = []

def digest(path):
    return sha256(Path(path).read_bytes()).hexdigest()

def copy(source, target):
    source, target = Path(source), out / target
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(source, target)
    assert source.read_bytes() == target.read_bytes()
    copies.append(dict(source=str(source.relative_to(repo)).replace('\\', '/'),
                       destination=target.relative_to(out).as_posix(),
                       size=target.stat().st_size, sha256=digest(target), byteIdentical=True))
    return target

records = {}
summaries = []
for revision in ('r1', 'r2'):
    run = work / f'history-actions-native-{revision}'
    record = json.loads((run / 'verification.json').read_text())
    records[revision] = record
    assert record['status'] == 'terminal' and not record['forcedCleanupUsed']
    guards = {key: value for key, value in record.items() if key.endswith('Unchanged')}
    assert len(guards) == 6 and all(guards.values())
    for filename in ('verification.json', 'result.txt', 'result.xml'):
        copy(run / filename, f'runs/{revision}/{filename}')
    producer = work / f'run-history-actions-native-{revision}.py'
    assert digest(producer) == record['producerSha256']
    copy(producer, f'producers/{producer.name}')
    incidents = [i for function in record['functions'] for i in function['incidents']]
    summaries.append(dict(revision=revision, status=record['status'],
        exitCode=record['exitCode'], normalExit=record['normalExit'],
        forcedCleanupUsed=record['forcedCleanupUsed'], guards=guards,
        passed=sum(i['type'] == 'pass' for i in incidents),
        failed=sum(i['type'] == 'fail' for i in incidents),
        skipped=sum(i['type'] == 'skip' for i in incidents),
        expectedCases=record['expectedCases'], testSha256=record['testSha256'],
        coreSha256=record['runtimeSha256']['libvlccore.dll'],
        qtSha256=record['runtimeSha256']['plugins/gui/libqt_plugin.dll'],
        medialibrarySha256=record['runtimeSha256']['plugins/misc/libmedialibrary_plugin.dll']))
assert summaries[0]['passed'] == 14 and summaries[0]['failed'] == 2
assert summaries[1]['passed'] == 16 and summaries[1]['failed'] == 0

for revision, builddir, producername in (
    ('r1', 'history-actions-build-r1', 'build-history-actions-r1.sh'),
    ('r2', 'history-actions-test-build-r2', 'build-history-actions-tests-r2.sh')):
    build = work / builddir
    assert (build / 'exit.txt').read_text().strip() == '0'
    producer = work / producername
    assert digest(producer) == (build / 'producer.sha256').read_text().split()[0]
    copy(producer, f'producers/{producername}')
    for filename in ('exit.txt', 'producer.sha256', 'inputs.sha256', 'binaries.sha256'):
        copy(build / filename, f'builds/{revision}/{filename}')
copy(work / 'history-actions-build-r1/commit.txt', 'builds/r1/commit.txt')
copy(work / 'history-actions-build-r1/source.patch', 'builds/r1/source.patch')
copy(work / 'history-actions-test-build-r2/unchanged-inputs.sha256', 'builds/r2/unchanged-inputs.sha256')

common = ['include/vlc_player.h', 'src/libvlccore.sym', 'src/player/player.h',
    'src/player/input.c', 'src/player/medialib.c',
    'modules/gui/qt/medialibrary/medialib.hpp',
    'modules/gui/qt/medialibrary/mlrecentvideomodel.hpp',
    'modules/gui/qt/medialibrary/mlrecentvideomodel.cpp',
    'modules/gui/qt/medialibrary/mlvideohistory.hpp']
for filename in common:
    actual = digest(repo / filename)
    assert records['r1']['sourceSha256'][filename] == actual
    assert records['r2']['sourceSha256'][filename] == actual
    copy(repo / filename, f'snapshots/common/{filename}')

testpath = 'modules/gui/qt/tests/test_home_activity.cpp'
assert digest(repo / testpath) == records['r2']['sourceSha256'][testpath]
r2test = copy(repo / testpath, f'snapshots/r2/{testpath}')
runnerpath = 'test/libvlc/native_home_activity.py'
assert digest(repo / runnerpath) == records['r1']['sourceSha256'][runnerpath]
assert digest(repo / runnerpath) == records['r2']['sourceSha256'][runnerpath]
copy(repo / runnerpath, f'producers/{Path(runnerpath).name}')

# Recover r1 from its actual HEAD + saved unified-diff section. Accept the
# reconstruction ONLY if its full-file SHA-256 matches the r1 run manifest.
commit = (work / 'history-actions-build-r1/commit.txt').read_text().strip()
base = subprocess.check_output(['git', 'show', f'{commit}:{testpath}'], cwd=repo).splitlines()
patch = (work / 'history-actions-build-r1/source.patch').read_bytes().splitlines()
marker = f'diff --git a/{testpath} b/{testpath}'.encode()
start = patch.index(marker)
end = next((i for i in range(start+1, len(patch)) if patch[i].startswith(b'diff --git ')), len(patch))
section = patch[start:end]
rebuilt = []
position = 0
index = 0
while index < len(section):
    match = re.match(rb'@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@', section[index])
    if not match:
        index += 1
        continue
    hunkstart = int(match[1]) - 1
    assert hunkstart >= position
    rebuilt.extend(base[position:hunkstart])
    position = hunkstart
    index += 1
    while index < len(section) and not section[index].startswith(b'@@ '):
        line = section[index]
        if line[:1] in (b' ', b'-'):
            assert position < len(base) and base[position] == line[1:]
            position += 1
        if line[:1] in (b' ', b'+'):
            rebuilt.append(line[1:])
        else:
            assert line[:1] in (b'-', b'\\')
        index += 1
rebuilt.extend(base[position:])
expected = records['r1']['sourceSha256'][testpath]
candidates = [separator.join(rebuilt) + ending
    for separator in (b'\n', b'\r\n') for ending in (separator, b'')]
matching = [value for value in candidates if sha256(value).hexdigest() == expected]
assert matching, 'Do not publish an invented r1 source reconstruction'
r1test = out / 'snapshots/r1' / testpath
r1test.parent.mkdir(parents=True)
r1test.write_bytes(matching[0])
assert digest(r1test) == expected
correction = ''.join(difflib.unified_diff(r1test.read_text().splitlines(True),
    r2test.read_text().splitlines(True), fromfile='r1/test_home_activity.cpp', tofile='r2/test_home_activity.cpp'))
(out / 'snapshots/test-corrections.patch').write_text(correction, encoding='utf-8', newline='\n')

recovery = dict(file=testpath, method='git show frozen commit plus saved source.patch file section',
    commit=commit, patch='builds/r1/source.patch',
    r1Sha256=digest(r1test), r1MatchesOriginalManifest=True,
    r2Sha256=digest(r2test), r2MatchesOriginalManifest=True,
    commonSourceSnapshotsMatchBothRuns=True)
(out / 'source-recovery.json').write_text(json.dumps(recovery, indent=2)+'\n')
(out / 'summary.json').write_text(json.dumps(dict(scope='Native backend tests only; QML worker and ordinary GUI validation are separate',
    runs=summaries), indent=2)+'\n')
copy(__file__, 'producers/collect-history-actions-native-receipt.py')
(out / 'copy-verification.json').write_text(json.dumps(dict(copiedFiles=copies,
    noBinariesOrRuntimeLogsCopied=True, preservedOriginalBytes=True), indent=2)+'\n')
print(json.dumps(dict(destination=str(out), copiedFiles=len(copies),
    r1Recovered=True, r1Sha256=digest(r1test), r2Sha256=digest(r2test), runs=summaries), indent=2))
