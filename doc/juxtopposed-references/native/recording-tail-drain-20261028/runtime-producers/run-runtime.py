"""Frozen, bounded real-writer comparison for the local avformat tail repair."""
import argparse
import json
import os
from pathlib import Path
import re
import shutil
import sys
import time
sys.dont_write_bytecode = True
from runtime_support import digest, safe_digest, manifest, manifest_matches, stage_hardlinks, storage_state

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--case', choices=('control', 'candidate', 'reversed', 'header-error', 'writer-error'), required=True)
args = parser.parse_args()
prep = Path(__file__).resolve().parent
repo = Path(r'C:\Modding\vlc-juxtopposed')
config_digest = digest(prep / 'runtime-config.json')
config = json.loads((prep / 'runtime-config.json').read_text())
out = prep / ('runtime-' + args.case + '-r1')
assert not out.exists(), 'Preserve prior results; use a freshly reviewed producer/output'
out.mkdir()
runtime = out / 'runtime'
record = dict(status='running', started=time.time(), phase='preflight', variant=args.case,
    configSha256=config_digest,
    scope='Generated H264/FLAC/two-SRT real avformat Matroska writer; no desktop, recording toggle or final-package acceptance.')
errors, failure = [], None
record['inspectionErrors'] = errors
baseline = repo / config['preservedRun'] / 'runtime'
fixture_info = config['alternateFixtures']['reversed'] if args.case == 'reversed' else config
fixture = repo / fixture_info['fixture']
identities = config['preservedRuntimeInventory']
expected = config['sourceSha256']
runtime_hashes = runtime_inventory = human = None

def save():
    temp = out / 'verification.next.json'
    temp.write_text(json.dumps(record, indent=2) + '\n')
    os.replace(temp, out / 'verification.json')

save()
try:
    producers = config['producerSha256']
    assert all(digest(prep / p) == h for p, h in producers.items()), 'Frozen producer changed'
    assert all(digest(repo / p) == h for p, h in expected.items()), 'Source changed after review'
    build_path = repo / config['buildReceipt']
    assert digest(build_path) == config['buildReceiptSha256']
    build = json.loads(build_path.read_text())
    assert build['status'] == 'terminal' and build['success'] and build['exitCode'] == 0
    assert all(build[k] for k in config['requiredBuildGuards'])
    assert not build['cleanupErrors'] and not build['verificationErrors']
    unit_path = repo / config['unitReceipt']
    assert digest(unit_path) == config['unitReceiptSha256']
    unit = json.loads(unit_path.read_text())
    assert unit['normalExit'] and unit['exitCode'] == 0 and unit['cases'] == 41 and unit['failures'] == 0
    assert unit['success'] and not unit['inspectionErrors'] and all(unit[k] for k in config['requiredUnitGuards'])
    assert build['unitVerificationSha256'] == config['unitReceiptSha256']
    plugin = repo / config['candidatePlugin']
    assert digest(plugin) == build['pluginSha256'] == config['candidatePluginSha256']
    assert manifest(baseline) == identities
    assert digest(fixture) == fixture_info['fixtureSha256']
    helper_path = repo / 'test/libvlc/native_http_runtime.py'
    assert digest(helper_path) == config['runtimeHelperSha256']
    sys.path.insert(0, str(helper_path.parent))
    import native_http_runtime as helper
    original_environment = helper.environment
    def isolated_environment(runtime_path, output_path):
        env = original_environment(runtime_path, output_path)
        env.pop('VLC_PLUGIN_PATH', None)
        return env
    helper.environment = isolated_environment
    human = {str(Path(os.environ['APPDATA']) / 'vlc' / p): None for p in ('vlcrc', 'vlc-qt-interface.ini')}
    human = {p: digest(p) if Path(p).is_file() else None for p in human}
    record.update(sourceSha256=expected, fixture=fixture_info['fixture'], fixtureSha256=fixture_info['fixtureSha256'],
        producerSha256=producers, buildReceiptSha256=config['buildReceiptSha256'],
        unitReceiptSha256=config['unitReceiptSha256'], humanBeforeSha256=human,
        storageBeforeStage=storage_state(repo, config['runtimeStorageBudget']))
    save()
    assert record['storageBeforeStage']['fits'], 'C: peak would exceed owner80percent limit'
    record['stage'] = stage_hardlinks(baseline, runtime, identities)
    if args.case != 'control':
        target = runtime / 'plugins/codec/libavcodec_plugin.dll'
        target.unlink()  # Break the immutable baseline hardlink before replacement.
        os.link(plugin, target)  # Frozen candidate stays immutable; no full DLL copy.
    record.update(phase='cache')
    save()
    record['cacheProcess'] = helper.run_owned([runtime / 'vlc-cache-gen.exe', runtime / 'plugins'], runtime, out / 'cache')
    assert record['cacheProcess']['normalExit']
    runtime_inventory = manifest(runtime)
    runtime_hashes = {p: item['sha256'] for p, item in runtime_inventory.items()}
    assert set(runtime_inventory) == set(identities)
    for p, entry in identities.items():
        if p != 'plugins/codec/libavcodec_plugin.dll' and Path(p).name != 'plugins.dat':
            assert runtime_hashes[p] == entry['sha256'], p
    expected_plugin = config['controlPluginSha256'] if args.case == 'control' else config['candidatePluginSha256']
    assert runtime_hashes['plugins/codec/libavcodec_plugin.dll'] == expected_plugin
    record.update(runtimeSha256=runtime_hashes, runtimeInventory=runtime_inventory)
    output = out / 'output.mkv'
    sout = '#standard{access=file{no-overwrite},mux=avformat{mux=matroska},dst=' + output.as_posix() + '}'
    if args.case == 'header-error':
        # Matroska's SRT tracks are deliberately incompatible with MP4 passthrough.
        sout = '#standard{access=file{no-overwrite},mux=avformat{mux=mp4},dst=' + output.as_posix() + '}'
    elif args.case == 'writer-error':
        sout = '#standard{access=fd,mux=avformat{mux=matroska},dst=1}'
    command = [str(runtime / 'vlc.exe'), '--ignore-config', '--intf=dummy', '--no-one-instance', '--no-media-library',
        '--no-save-recentplay', '--no-metadata-network-access', '--no-ml-network-discovery', '--no-auto-preparse',
        '--extraintf=', '--control=', '--no-crashdump', '--play-and-exit', '--file-logging', '--verbose=2',
        '--log-verbose=2', '--logfile=' + str(out / 'vlc.log'), '--sout-all',
        '--sout=' + sout, str(fixture)]
    record.update(command=command, phase='vlc')
    save()
    if args.case == 'writer-error':
        from pipe_runtime import run_closed_sink
        record.update(run_closed_sink(command, runtime, out, helper.environment(runtime, out), timeout=85))
        output = out / 'stdout-prefix.bin'
    else:
        record.update(helper.run_owned(command, runtime, out, timeout=85))
    assert record['normalExit']
    log = (out / 'vlc.log').read_text(errors='replace')
    stderr = (out / 'stderr.txt').read_text(errors='replace')
    record.update(moduleCounts=re.findall(r'plug-ins loaded: (\d+) modules', log),
        selectedLibavformat='using sout mux module "avcodec"' in log,
        diagnostics=[line for line in (log + '\n' + stderr).splitlines()
            if re.search(r'\berror:|Assertion failed|Binding loop|ReferenceError|TypeError', line)],
        outputSha256=digest(output), outputBytes=output.stat().st_size)
    assert record['outputBytes'] <= config['runtimeStorageBudget']['outputMaxBytes']
    assert record['moduleCounts'] == ['603'] and record['selectedLibavformat']
    registrations = re.findall(r'adding an output ES for `([^`]+)` \(([^)]+)\)', log)
    removals = re.findall(r'removing an output ES \(([^)]+)\)', log)
    registered = {pointer: (ordinal, codec) for ordinal, (codec, pointer) in enumerate(registrations)}
    record['esRegistrationCodecs'] = [codec for codec, pointer in registrations]
    record['esDeletionOrder'] = [dict(registration=registered[p][0], codec=registered[p][1])
        for p in removals if p in registered]
    if args.case in ('control', 'candidate', 'reversed'):
        assert not record['diagnostics']
        assert len(registrations) == len(removals) == len(record['esDeletionOrder']) == 4
    else:
        wanted = ('could not write header',) if args.case == 'header-error' else (
            'could not write frame', 'could not flush buffered mux output', 'could not flush mux header')
        record['expectedIoFailureObserved'] = any(any(text in line for text in wanted) for line in record['diagnostics'])
        record['unexpectedFailureDiagnostics'] = [line for line in record['diagnostics']
            if re.search(r'Assertion failed|Binding loop|ReferenceError|TypeError', line)]
        assert record['expectedIoFailureObserved'] and not record['unexpectedFailureDiagnostics']
except BaseException as error:
    failure = error
    record['failure'] = repr(error)
finally:
    try:
        record['sourceUnchanged'] = all([safe_digest(repo / p, errors) == h for p, h in expected.items()])
        record['fixtureUnchanged'] = safe_digest(fixture, errors) == fixture_info['fixtureSha256']
        record['producersUnchanged'] = all([safe_digest(prep / p, errors) == h for p, h in config['producerSha256'].items()])
        record['configUnchanged'] = safe_digest(prep / 'runtime-config.json', errors) == config_digest
        record['buildReceiptUnchanged'] = safe_digest(repo / config['buildReceipt'], errors) == config['buildReceiptSha256']
        record['unitReceiptUnchanged'] = safe_digest(repo / config['unitReceipt'], errors) == config['unitReceiptSha256']
        record['preservedRuntimeUnchanged'] = manifest_matches(baseline, identities, errors)
        if runtime_inventory is not None:
            record['runtimeUnchanged'] = manifest_matches(runtime, runtime_inventory, errors)
        if human is not None:
            record['humanConfigUnchanged'] = all([(safe_digest(p, errors) if Path(p).is_file() else None) == h for p, h in human.items()])
    except BaseException as inspection_error:
        errors.append(repr(inspection_error))
    record.update(status='terminal', ended=time.time())
    save()
if failure is not None:
    raise failure
assert not errors and all(v for k, v in record.items() if k.endswith('Unchanged'))
print(json.dumps({k:record[k] for k in ('status','variant','exitCode','normalExit','moduleCounts','diagnostics','outputBytes')}, indent=2))
