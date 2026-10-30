"""Copy only selected frozen generated-data evidence; never run VLC or modify inputs."""
from hashlib import sha256
import json
from pathlib import Path
import shutil

prep = Path(__file__).resolve().parent
repo = prep.parents[2]
out = repo / 'doc/juxtopposed-references/native/recording-tail-drain-20261028'
assert not out.exists(), 'Frozen receipts are never overwritten'
out.mkdir()
copied = []
def digest(p): return sha256(Path(p).read_bytes()).hexdigest()
def copy(source, relative):
    target = out / relative
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(source, target)
    assert digest(source) == digest(target)
    copied.append(dict(source=str(source.relative_to(repo)), target=relative, sha256=digest(target)))
def sanitized(source, relative):
    value = json.loads(source.read_text())
    value.pop('humanBeforeSha256', None)
    value.pop('priorProfileSha256', None)
    target = out / relative
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(value, indent=2)+'\n')

config = json.loads((prep / 'runtime-config.json').read_text())
assert all(digest(prep / p) == h for p,h in config['producerSha256'].items())
for name,h in config['sourceSha256'].items():
    source = prep / 'adapter-guards-r1/candidate' / name
    assert digest(source) == h
    copy(source, 'sources/' + name)
copy(prep / 'source-delta.patch', 'sources/queue-implementation.patch')
copy(prep / 'adapter-guards-r1/adapter-only.patch', 'sources/adapter-guards.patch')
copy(prep / 'adapter-guards-r1/source-identities.json', 'sources/source-identities.json')
for name in config['producerSha256']:
    copy(prep / name, 'runtime-producers/' + name)
copy(prep / 'runtime-config.json', 'runtime-producers/runtime-config.json')
for variant in ('control','candidate','reversed','header-error','writer-error'):
    run = prep / ('runtime-' + variant + '-r1')
    record = json.loads((run / 'verification.json').read_text())
    required = ('sourceUnchanged','fixtureUnchanged','producersUnchanged','configUnchanged',
        'buildReceiptUnchanged','unitReceiptUnchanged','preservedRuntimeUnchanged','runtimeUnchanged','humanConfigUnchanged')
    assert record['status'] == 'terminal' and record['normalExit'] and record['exitCode'] == 0
    assert not record.get('failure') and not record['inspectionErrors'] and not record['forcedCleanupUsed']
    assert all(record.get(k) is True for k in required)
    assert record['moduleCounts'] == ['603'] and record['selectedLibavformat']
    if variant in ('control','candidate','reversed'):
        analysis = json.loads((run / 'analysis.json').read_text())
        assert analysis['accepted'] and analysis['verificationSha256'] == digest(run / 'verification.json')
        copy(run / 'analysis.json', 'runs/' + variant + '/analysis.json')
        assert record['diagnostics'] == []
    else:
        assert record['expectedIoFailureObserved'] and not record['unexpectedFailureDiagnostics']
    sanitized(run / 'verification.json', 'runs/' + variant + '/verification-sanitized.json')

native = prep / 'native-qualification-r1'
for name in ('prepare-inputs.py','qualify-build.py','run-unit.py','build-inputs.json',
    'dependency-inputs.json','preflight.json','producer-identities.json','reviewed-probe-preflight.json'):
    copy(native / name, 'native/' + name)
for name in ('unit-build/verification.json','unit-build/build.log','unit-build/compiled.d',
    'unit/verification.json','unit/stdout.txt','unit/stderr.txt','build/verification.json','build/build.log'):
    copy(native / name, 'native/' + name)
unit = json.loads((native / 'unit/verification.json').read_text())
build = json.loads((native / 'build/verification.json').read_text())
assert unit['success'] and unit['cases'] == 41 and unit['checks'] == 5772 and unit['failures'] == 0
assert build['success'] and build['writerLockReleased'] and build['pluginSha256'] == config['candidatePluginSha256']

(out / '.gitattributes').write_text('* -text\n')
(out / 'README.md').write_text('''# Recording tail repair — 28 October 2026

This frozen candidate repairs committed packets discarded when an input ends.
The earlier [FIFO observation](../recording-tail-fifo-20261028/README.md) found the
missing 619-byte/528-sample FLAC tail still queued at input deletion.

| Evidence | Actual result |
| --- | --- |
| Production queue helper | 41 native cases, 5,772 checks, zero failures; all 24 A/V/S/S deletion orders, ownership, live FIFO gaps/flush, header/write/trailer failures and cleanup. |
| Unchanged codec control | Reproduces 625/626 audio packets and 2,880,000/2,880,528 samples, exact source prefix; video/subtitles/timing remain intact. This is the required negative control. |
| Repaired real Matroska writer | All 626 packet payloads and 2,880,528 decoded audio samples exactly match the source. All 1,500 frame markers, both exact SRT exports/durations and audio/video timelines pass. |
| Opposite actual A/V deletion order | The remapped fixture produces audio-before-video deletion instead of video-before-audio; the same complete output checks pass. Existing Core add/remove logs identify order without another observation build. |
| Incompatible MP4 header | Expected native header failure and failed finalization, normal process exit, no forced cleanup. This deliberately incompatible subtitle fixture is a negative test. |
| Closed owned output pipe | Reader accepts 4,096 bytes then closes only its own sink. Native Broken pipe/frame failure and failed finalization are observed; child and reader finish normally, with no assertion/forced cleanup. |

Every runtime run loads 603 modules, selects the actual avcodec/avformat mux and
passes nine explicit preservation guards. Positive/control media runs have no
checked diagnostics; the two deliberate failure cases retain expected errors.
Unit, module build, runtime and independent decoding have distinct scopes.

The source snapshot is mux `d06d150d3fdb92c80fce3afe47af6bf7933876d81900a2dac5d5becea7422f35`,
queue helper `37eaa3c00753f5dc34ba43632ceb928492bd04f1277d9a7c125cc5e3893701d2`,
test `34a467dedff11259b4612b52646b2a20cd506f2887e29d9b8fac359658699e2b`.
The rebuilt codec is `79a565a9398756aaf22ceb194771a9599c499771971638bfdfb06e9fc531882f`.
Runtime Core remains r5 `f8fc980bbe03da57908f79045f266710dd27a16c6ee66b6c45d60cd1f9b93598`;
the full support/renderer/demux identities are in the frozen runtime configuration.
The ordinary Home candidate uses another Core and needs integrated qualification.

The module owns only queues detached at Delete; active FIFOs remain Core-owned,
with original readiness/flush semantics. Packets are merged by original DTS;
only a successful complete drain permits trailer finalization. Header/write
failures are sticky. ASS mapping and subtitle durations stay preserved.
Additional guards handle failed pre-keyframe flush/header I/O/allocation.
The allocation branch is source-reviewed, not an actual forced-OOM test.

The sister [container smoke receipt](../home-subtitle-refill-20261028/avformat-writer-smoke/)
has exact MPEG-TS and short-startup Matroska results. MP4 default presentation
remains open: ignoring edit lists recovers complete payload, while default
playback crops the ending. A later duration repair is a separate candidate.
This receipt does not close ordinary Record-toggle/UI, all format/hardware,
accessibility/DPI, integrated-current-tuple or final-package beta gates.

Frozen producers are one-use local records: review/adapt into fresh directories
before reuse. Native source/header, exact input inventories, compile/link logs,
unit results and selected runtime/decoder results are included. Personal
configuration hashes, raw VLC logs, media, binaries, databases and dumps are not.
The generated remap command and content/timing proof are retained without media.
The inventory preserves bytes across Git newline conversion.
''', newline='\n')
copy(Path(__file__), 'collect-main-receipt.py')
(out / 'copy-verification.json').write_text(json.dumps(dict(byteIdenticalCopies=len(copied), files=copied),indent=2)+'\n')
entries = [(p.relative_to(out).as_posix(),digest(p)) for p in sorted(out.rglob('*')) if p.is_file()]
(out / 'SHA256SUMS').write_text(''.join(h+'  '+p+'\n' for p,h in entries),newline='\n')
assert all(digest(out / p) == h for p,h in entries)
print(json.dumps({'receipt': str(out), 'files': len(entries), 'bytes': sum(p.stat().st_size for p in out.rglob('*') if p.is_file())},indent=2))
