"""Freeze a fresh runtime comparison after the reviewed native build is terminal."""
from pathlib import Path
from hashlib import sha256
import ast
import json

prep = Path(__file__).resolve().parent
repo = prep.parents[2]
assert repo == Path(r'C:\Modding\vlc-juxtopposed')
reference = repo / 'build-windows/beta-fps-20261028/fifo-tail-probe-preparation-r4'
digest = lambda p: sha256(Path(p).read_bytes()).hexdigest()
build_path = prep / 'native-qualification-r1/build/verification.json'
unit_path = prep / 'native-qualification-r1/unit/verification.json'
build = json.loads(build_path.read_text())
unit = json.loads(unit_path.read_text())
assert build['status'] == 'terminal' and build['success'] and build['exitCode'] == 0
assert unit['normalExit'] and unit['cases'] == 41 and unit['failures'] == 0
unit_guards = ['allSourceInputsUnchanged', 'allProducersUnchanged', 'buildVerificationUnchanged',
    'unitBuildExeUnchanged', 'runtimeExeUnchanged', 'preservedCoreUnchanged', 'runtimeCoreUnchanged', 'runtimeFileSetUnchanged']
assert unit['success'] and not unit['inspectionErrors'] and all(unit[k] for k in unit_guards)
assert build['unitVerificationSha256'] == digest(unit_path)
config_path = prep / 'runtime-config.json'
assert not config_path.exists(), 'Never replace a frozen runtime configuration'
support = (reference / 'probe_support.py').read_bytes()
if (prep / 'runtime_support.py').exists():
    assert (prep / 'runtime_support.py').read_bytes() == support
else:
    (prep / 'runtime_support.py').write_bytes(support)

# Reuse the reviewed streaming decoder, dropping instrumentation-only parsing
# and replacing its diagnostic acceptance with explicit positive/negative gates.
analyzer = (reference / 'analyze-probe.py').read_text()
analyzer = analyzer.replace('import time\n', 'import time\nimport sys\n', 1)
assert "out = prep / 'run'" in analyzer
analyzer = analyzer.replace("out = prep / 'run'", "variant = sys.argv[1]\nassert variant in ('control', 'candidate', 'reversed')\nout = prep / ('runtime-' + variant + '-r1')", 1)
analyzer = analyzer.replace("prep / 'preflight.json'", "prep / 'runtime-config.json'", 1)
needle = "    assert record['status'] == 'terminal' and record['normalExit']"
assert needle in analyzer
analyzer = analyzer.replace(needle, """    assert record['status'] == 'terminal' and record['normalExit'] and record['exitCode'] == 0
    assert record['variant'] == variant
    assert not record.get('failure') and not record.get('inspectionErrors')
    assert record['diagnostics'] == [] and record['moduleCounts'] == ['603'] and record['selectedLibavformat']
    required_guards = ('sourceUnchanged', 'fixtureUnchanged', 'producersUnchanged', 'configUnchanged',
        'buildReceiptUnchanged', 'unitReceiptUnchanged', 'preservedRuntimeUnchanged', 'runtimeUnchanged', 'humanConfigUnchanged')
    assert all(record.get(key) is True for key in required_guards)
    assert sha256((prep / 'runtime-config.json').read_bytes()).hexdigest() == record['configSha256']
    assert all(sha256((prep / p).read_bytes()).hexdigest() == h for p,h in config['producerSha256'].items())""", 1)
start = analyzer.index("    log = (out / 'vlc.log').read_text(errors='replace')")
end = analyzer.index('    def run(arguments):', start)
analyzer = analyzer[:start] + analyzer[end:]
start = analyzer.index('    tail_present = bool(tail_rows)')
end = analyzer.index("    result = {'status': 'observation-terminal'", start)
analyzer = analyzer[:start] + analyzer[end:]
analyzer = analyzer.replace("{'status': 'observation-terminal', 'scope': 'Instrumentation-only diagnosis, not repair acceptance.',\n        'events': events, 'matching619Byte60sAudioRows': tail_rows, 'tailMetadataPresentAtDelete': tail_present,\n        'diagnosis': diagnosis,", "{'status': 'expected-control-tail-loss' if variant == 'control' else 'passed',\n        'scope': 'Strict real-writer Matroska comparison; separate from ordinary recording-toggle/UI or final package.',\n        'variant': variant, 'esRegistrationCodecs': record['esRegistrationCodecs'], 'esDeletionOrder': record['esDeletionOrder'],\n        'producerSha256': sha256(Path(__file__).read_bytes()).hexdigest(),", 1)
analyzer = analyzer.replace("source = repo / config['fixture']", "source = repo / record['fixture']")
analyzer = analyzer.replace("config['fixtureSha256']", "record['fixtureSha256']")
needle = "    assert len(source_packets) == 626 and result['sourceAudioSamples'] == 2880528\n    return result"
assert needle in analyzer
analyzer = analyzer.replace(needle, """    assert len(source_packets) == 626 and result['sourceAudioSamples'] == 2880528
    def packet_times_match(before, after):
        return len(after) <= len(before) and all(
            all(key in old and key in new and abs(float(old[key]) - float(new[key])) <= .002
                for key in ('pts_time', 'duration_time'))
            for old, new in zip(before, after))
    source_video = next(s['index'] for s in source_streams if s['codec_type'] == 'video')
    output_video = next(s['index'] for s in streams if s['codec_type'] == 'video')
    old_video_packets = [p for p in all_source_packets if p['stream_index'] == source_video]
    new_video_packets = [p for p in all_packets if p['stream_index'] == output_video]
    result['audioPrefixTimelinePreserved'] = packet_times_match(source_packets, packets)
    result['videoTimelinePreserved'] = (len(old_video_packets) == len(new_video_packets) == 1500
        and packet_times_match(old_video_packets, new_video_packets))
    result['accepted'] = (result['audioPrefixTimelinePreserved'] and result['videoTimelinePreserved']
        and (strict_audio and len(packets) == 626 and output_samples == 2880528
             if variant != 'control' else known_tail_loss))
    assert sha256(source.read_bytes()).hexdigest() == record['fixtureSha256']
    assert sha256(output.read_bytes()).hexdigest() == record['outputSha256']
    return result""", 1)
analyzer = analyzer.replace('    result = analyze_observation()\n', "    result = analyze_observation()\n    assert result['accepted'], 'Strict comparison failed; inspect retained metrics'\n", 1)
analyzer = analyzer.replace("'observation-failed'", "'analysis-failed'")
analyzer = analyzer.replace("'Instrumentation-only observation, never repair acceptance.'", "'Real-writer repair comparison; failed analysis is not acceptance.'")
assert 'tail_rows' not in analyzer and 'diagnosis' not in analyzer and "events': events" not in analyzer
ast.parse(analyzer)
(prep / 'analyze-runtime.py').write_text(analyzer, newline='\n')

old = json.loads((reference / 'preflight.json').read_text(encoding='utf-8-sig'))
relative = lambda p: p.relative_to(repo).as_posix()
plugin = build_path.parent / 'libavcodec_plugin.dll'
assert digest(plugin) == build['pluginSha256']
sources = json.loads((prep / 'adapter-guards-r1/source-identities.json').read_text())
assert all(digest(repo / p) == h for p, h in sources.items())
guards = ['sourceUnchanged', 'buildInputGuards', 'dependencyInputGuards', 'producerUnchanged',
    'originalBackupsUnchanged', 'untouchedBuildBinariesUnchanged', 'writerLockReleased',
    'actualCompileObserved', 'actualLinkObserved', 'writerCleanupConfirmed', 'sharedCandidatePluginUnchanged']
assert all(build[k] for k in guards)
assert not build['cleanupErrors'] and not build['verificationErrors']
assert not build['forcedBuildCleanupUsed'] and not build['remainingOwnedWriterPids']
config = {k: old[k] for k in ('preservedRun', 'preservedRuntimeInventory', 'fixture', 'fixtureSha256',
    'runtimeHelperSha256', 'runtimeStorageBudget')}
config['runtimeStorageBudget']['componentsBytes']['stagedReplacementDllMaximum'] = 0
config['runtimeStorageBudget']['peakAdditionalBytes'] = sum(config['runtimeStorageBudget']['componentsBytes'].values())
config['runtimeStorageBudget']['stagingPolicy'] = 'hardlink-only-fail-closed-including-frozen-candidate'
reversed_fixture = prep / 'media-reversed/frame-marker.mkv'
assert reversed_fixture.is_file()
generation = json.loads((prep / 'media-reversed/generation-check.json').read_text())
assert generation['status'] == 'passed' and generation['remappedSha256'] == digest(reversed_fixture)
config['alternateFixtures'] = {'reversed': {'fixture': relative(reversed_fixture), 'fixtureSha256': digest(reversed_fixture)}}
config.update(sourceSha256=sources, buildReceipt=relative(build_path), buildReceiptSha256=digest(build_path),
    unitReceipt=relative(unit_path), unitReceiptSha256=digest(unit_path), requiredBuildGuards=guards, requiredUnitGuards=unit_guards,
    candidatePlugin=relative(plugin), candidatePluginSha256=digest(plugin),
    controlPluginSha256=old['preservedTuple']['plugins/codec/libavcodec_plugin.dll'],
    producerSha256={p: digest(prep / p) for p in ('prepare-runtime.py','run-runtime.py','analyze-runtime.py','runtime_support.py','pipe_runtime.py',
        'media-reversed/verify-source.py','media-reversed/generation-check.json')})
config_path.write_text(json.dumps(config, indent=2) + '\n')
print(json.dumps({'config': str(config_path), 'pluginSha256': digest(plugin), 'sourceFiles': len(sources)}, indent=2))
