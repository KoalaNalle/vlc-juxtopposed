"""Archive frozen R4 observation records only; never run a build or media test."""
from pathlib import Path
from hashlib import sha256
import json
import re
import shutil

repo = Path(__file__).resolve().parents[2]
source = Path(__file__).resolve().parent / 'fifo-tail-probe-preparation-r4'
target = repo / 'doc/juxtopposed-references/native/recording-tail-fifo-20261028'
target.mkdir(parents=True, exist_ok=False)
copied = []

def digest(path):
    return sha256(Path(path).read_bytes()).hexdigest()

def copy(path, relative):
    destination = target / relative
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(path, destination)
    assert Path(path).read_bytes() == destination.read_bytes()
    copied.append(dict(source=Path(path).relative_to(repo).as_posix(),
        destination=relative, sha256=digest(destination), size=destination.stat().st_size,
        byteIdentical=True))

identities = json.loads((source / 'producer-identities.json').read_text())
assert len(identities) == 17
for name, expected in identities.items():
    # Only frozen preparation files, never their current production counterparts.
    assert digest(source / name) == expected
    copy(source / name, f'inputs/{name}')
copy(source / 'producer-identities.json', 'producer-identities.json')

build = json.loads((source / 'build/verification.json').read_text())
runtime = json.loads((source / 'run/verification.json').read_text())
analysis = json.loads((source / 'run/analysis.json').read_text())
assert build['status'] == runtime['status'] == 'terminal'
assert build['success'] and build['exitCode'] == runtime['exitCode'] == 0
assert not build['forcedBuildCleanupUsed'] and not runtime['forcedCleanupUsed']
build_guards = ['writerLockAcquired', 'writerLockReleased', 'writerCleanupConfirmed',
    'sourceRestored', 'sharedBuildArtifactsRestored', 'allProductionInputsRestored',
    'dependencyInputsRestored', 'producerUnchanged', 'buildInputGuards']
assert all(build[key] for key in build_guards)
assert not build['remainingOwnedWriterPids'] and not build['defunctOwnedGroupPids']
assert all(not value for key, value in build.items() if key.endswith('Errors'))
assert build['ended'] < runtime['started']
runtime_guards = {key: value for key, value in runtime.items() if key.endswith('Unchanged')}
assert len(runtime_guards) == 8 and all(runtime_guards.values())
assert runtime['normalExit'] and runtime['moduleCounts'] == ['603'] and not runtime['diagnostics']
assert runtime['selectedLibavformat']
assert build['producerSha256'] == identities['build-probe.py']
assert runtime['producerSha256'] == identities['run-probe.py']
assert build['sourceOriginalSha256'] == identities['mux-original.c']
assert build['sourceInstrumentedSha256'] == identities['mux-instrumented.c']
assert runtime['runtimeSha256']['plugins/codec/libavcodec_plugin.dll'] == build['instrumentedPluginSha256']
assert analysis['verificationSha256'] == digest(source / 'run/verification.json')
assert analysis['status'] == 'observation-terminal' and analysis['tailMetadataPresentAtDelete']
assert analysis['audioPackets'] == 625 and analysis['sourceAudioPackets'] == 626
assert analysis['audioSamples'] == 2880000 and analysis['sourceAudioSamples'] == 2880528
assert analysis['packetPayloadsExactSourcePrefix'] and analysis['pcmExactSourcePrefix']
assert analysis['known528SampleTailLoss'] and not analysis['strictAudioCompleteness']
assert analysis['videoFrames'] == 1500 and analysis['all1500Markers']
assert len(analysis['subtitleChecks']) == 2
assert all(item['timingPreserved'] and item['exactSrtExport'] for item in analysis['subtitleChecks'])
assert analysis['matching619Byte60sAudioRows'] == [dict(stream=3, ordinal=0,
    size=619, pts=60000001, dts=60000001, duration=11000)]
copy(source / 'build/verification.json', 'build/verification.json')
copy(source / 'build/build.log', 'build/build.log')
copy(source / 'run/analysis.json', 'run/analysis.json')

# Preserve the public guard results, never the personal configuration hashes.
omitted = [key for key in ('humanBeforeSha256', 'priorProfileSha256') if key in runtime]
public = {key: value for key, value in runtime.items() if key not in omitted}
public['sanitization'] = dict(originalSha256=digest(source / 'run/verification.json'),
    omittedFields=omitted)
(target / 'run/verification.json').write_text(json.dumps(public, indent=2)+'\n',
    encoding='utf-8', newline='\n')

# Retain ONLY these exact scalar instrumentation lines from the private raw log.
selected = []
line_numbers = []
for number, line in enumerate((source / 'run/vlc.log').read_bytes().splitlines(keepends=True), 1):
    if b'tail_probe ' not in line:
        continue
    assert re.fullmatch(rb'avcodec debug: tail_probe (begin|row|end) [A-Za-z0-9_= \-]+\r?\n?', line)
    selected.append(line)
    line_numbers.append(number)
assert len(selected) == 9
(target / 'run/tail-probe-lines.txt').write_bytes(b''.join(selected))
copy(__file__, 'collect-fifo-tail-receipt-r4.py')
(target / '.gitattributes').write_text('* -text\n', encoding='utf-8', newline='\n')
summary = dict(status='observation-terminal', scope='Retiring-FIFO diagnosis only; no repair/completeness acceptance',
    frozenInputsVerified=17, buildExitCode=build['exitCode'], runExitCode=runtime['exitCode'],
    forcedCleanupUsed=False, buildGuards={key:build[key] for key in build_guards},
    runtimeGuards=runtime_guards, restoredBuildEnded=build['ended'], runtimeStarted=runtime['started'],
    restoredBeforeRuntime=True, currentProductionFilesReadOrRehashed=False,
    probePluginSha256=build['instrumentedPluginSha256'], originalMuxSha256=identities['mux-original.c'],
    probeMuxSha256=identities['mux-instrumented.c'],
    sourcePackets=626, outputPackets=625, sourceSamples=2880528, outputSamples=2880000,
    missingSamples=528, exactPacketAndPcmSourcePrefix=True, strictAudioCompleteness=False,
    observedTail=analysis['matching619Byte60sAudioRows'][0], videoMarkers=1500,
    bothSrtExportsAndTimingsPreserved=True, mediaBinariesSettingsAndRawLogsCopied=False)
(target / 'summary.json').write_text(json.dumps(summary, indent=2)+'\n', encoding='utf-8', newline='\n')
proof = dict(byteIdenticalCopies=copied,
    sanitizedRuntimeRecord=dict(originalSha256=digest(source / 'run/verification.json'),
        publicSha256=digest(target / 'run/verification.json'), omittedFields=omitted),
    scalarLogExcerpt=dict(source='run/vlc.log', destination='run/tail-probe-lines.txt',
        exactOriginalLines=True, originalLineNumbers=line_numbers, count=len(selected),
        sha256=digest(target / 'run/tail-probe-lines.txt')),
    noCurrentProductionValidationPerformed=True)
(target / 'copy-verification.json').write_text(json.dumps(proof, indent=2)+'\n', encoding='utf-8', newline='\n')
print(json.dumps(dict(destination=str(target), frozenInputsVerified=17,
    originalCopiesVerified=len(copied), exactScalarLogLines=len(selected),
    originalRuntimeHashOmittedFields=omitted, restoredBeforeRuntime=True,
    sourceHashesFromFrozenEvidenceOnly=True)))
