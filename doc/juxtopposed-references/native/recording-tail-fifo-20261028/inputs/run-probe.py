"""One explicitly reviewed headless observation; never repair acceptance."""
import argparse
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time
sys.dont_write_bytecode = True
from probe_support import digest, safe_digest, manifest, manifest_matches, stage_hardlinks, storage_state

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--execute-reviewed-probe', action='store_true', required=True)
parser.parse_args()
prep = Path(__file__).resolve().parent
repo = Path(r'C:\Modding\vlc-juxtopposed')
out = prep / 'run'
assert not out.exists(), 'Fresh reviewed output required; preserve prior failures'
out.mkdir()
errors, failure = [], None
record = dict(status='running', phase='preflight', started=time.time(), inspectionErrors=errors,
              scope='Generated AV/two-SRT metadata observation only; no queue mutation or repair.')
config = expected = producers = identities = runtime_hashes = human = None
runtime = out / 'runtime'
base = previous_file = fixture = build_file = None
def save():
    temporary = out / 'verification.next.json'
    temporary.write_text(json.dumps(record, indent=2) + '\n')
    os.replace(temporary, out / 'verification.json')
save()
try:
    config = json.loads((prep / 'preflight.json').read_text(encoding='utf-8-sig'))
    producers = json.loads((prep / 'producer-identities.json').read_text(encoding='utf-8-sig'))
    assert all(digest(prep / name) == value for name, value in producers.items())
    expected = json.loads((prep / 'build-inputs.json').read_text(encoding='utf-8-sig'))
    assert len(expected) == 535 and len(config['dependencyInputs']) == 355
    assert all(digest(repo / name) == value for name, value in expected.items()), 'Review drift; never refresh guards'
    build_file = prep / 'build/verification.json'
    build = json.loads(build_file.read_text())
    assert build['status'] == 'terminal' and build['exitCode'] == 0 and not build.get('failure')
    assert build['success'] and build['writerCleanupConfirmed'] and build['writerLockReleased']
    assert not any(build[key] for key in ('restoreErrors', 'verificationErrors', 'cleanupErrors', 'receiptWriteErrors'))
    events = [item['event'] for item in build['events']]
    assert events.count('artifact-restored') == 6 and events.count('source-restored-last') == 1
    assert events.index('writer-cleanup') < events.index('artifact-restored')
    assert max(index for index, event in enumerate(events) if event == 'artifact-restored') < events.index('source-restored-last')
    assert all(build[key] for key in ('sourceRestored', 'sharedBuildArtifactsRestored',
        'allProductionInputsRestored', 'producerUnchanged', 'buildInputGuards'))
    plugin = prep / 'build/libavcodec_plugin.dll'
    assert digest(plugin) == build['instrumentedPluginSha256']
    assert plugin.stat().st_size <= config['storageBudget']['candidatePluginMaxBytes']
    previous_run = repo / config['preservedRun']
    previous_file = previous_run / 'verification.json'
    assert digest(previous_file) == config['preservedVerificationSha256']
    previous = json.loads(previous_file.read_text())
    identities = config['preservedRuntimeInventory']
    assert len(identities) == 533
    base = previous_run / 'runtime'
    assert manifest(base) == identities, 'Any runtime/support file changed'
    assert all(identities[name]['sha256'] == value for name, value in previous['runtimeSha256'].items())
    assert all(identities[name]['sha256'] == value for name, value in config['preservedTuple'].items())
    fixture = repo / config['fixture']
    assert digest(fixture) == config['fixtureSha256']
    assert digest(previous_run / 'output.mkv') == previous['outputSha256']
    helper_path = repo / 'test/libvlc/native_http_runtime.py'
    assert digest(helper_path) == config['runtimeHelperSha256']
    sys.path.insert(0, str(helper_path.parent))
    import native_http_runtime as helper
    original_environment = helper.environment
    def isolated_environment(runtime, output):
        env = original_environment(runtime, output)
        env.pop('VLC_PLUGIN_PATH', None)
        return env
    helper.environment = isolated_environment
    human = {str(Path(os.environ['APPDATA']) / 'vlc' / name): None for name in ('vlcrc', 'vlc-qt-interface.ini')}
    human = {name: digest(name) if Path(name).is_file() else None for name in human}
    record.update(sourceSha256=expected, humanBeforeSha256=human, fixtureSha256=config['fixtureSha256'],
        buildVerificationSha256=digest(build_file), previousVerificationSha256=config['preservedVerificationSha256'],
        producerSha256=digest(__file__), preservedRuntimeInventory=identities,
        storageBeforeStage=storage_state(repo, config['runtimeStorageBudget']))
    save()
    assert record['storageBeforeStage']['fits'], 'Worst peak exceeds C headroom before80percent'
    record['phase'] = 'staging'
    record['stage'] = stage_hardlinks(base, runtime, identities)
    target = runtime / 'plugins/codec/libavcodec_plugin.dll'
    target.unlink()  # Break the source hardlink before replacing the DLL.
    shutil.copyfile(plugin, target)
    record['phase'] = 'cache'
    save()
    record['cacheProcess'] = helper.run_owned([runtime / 'vlc-cache-gen.exe', runtime / 'plugins'], runtime, out / 'cache')
    assert record['cacheProcess']['normalExit']
    runtime_manifest = manifest(runtime)
    runtime_hashes = {name: digest(runtime / name) for name in identities}
    assert set(runtime_manifest) == set(identities), 'Staged support-file set differs'
    for name, value in identities.items():
        if name != 'plugins/codec/libavcodec_plugin.dll' and Path(name).name != 'plugins.dat':
            assert runtime_hashes[name] == value['sha256'], name
    assert runtime_hashes['plugins/codec/libavcodec_plugin.dll'] == build['instrumentedPluginSha256']
    record['runtimeSha256'] = runtime_hashes
    output = out / 'output.mkv'
    command = [str(runtime / 'vlc.exe'), '--ignore-config', '--intf=dummy', '--no-one-instance', '--no-media-library',
        '--no-save-recentplay', '--no-metadata-network-access', '--no-ml-network-discovery', '--no-auto-preparse',
        '--extraintf=', '--control=', '--no-crashdump', '--play-and-exit', '--file-logging', '--verbose=2',
        '--log-verbose=2', '--logfile=' + str(out / 'vlc.log'), '--sout-all',
        '--sout=#standard{access=file{no-overwrite},mux=avformat{mux=matroska},dst=' + output.as_posix() + '}', str(fixture)]
    record.update(command=command, phase='vlc')
    save()
    record.update(helper.run_owned(command, runtime, out, timeout=85))
    assert record['normalExit']
    log = (out / 'vlc.log').read_text(errors='replace')
    stderr = (out / 'stderr.txt').read_text(errors='replace')
    record.update(moduleCounts=re.findall(r'plug-ins loaded: (\d+) modules', log),
        selectedLibavformat='using sout mux module "avcodec"' in log,
        diagnostics=[line for line in (log + '\n' + stderr).splitlines()
            if re.search(r'\berror:|Assertion failed|Binding loop|ReferenceError|TypeError', line)],
        outputSha256=digest(output), outputBytes=output.stat().st_size)
    assert record['outputBytes'] <= config['storageBudget']['outputMaxBytes']
    assert record['moduleCounts'] == ['603'] and record['selectedLibavformat'] and not record['diagnostics']
except BaseException as error:
    failure = error
    record['failure'] = repr(error)
finally:
    try:
        if expected is not None:
            record['sourceUnchanged'] = all([safe_digest(repo / name, errors) == value for name, value in expected.items()])
        if config is not None and fixture is not None:
            record['fixtureUnchanged'] = safe_digest(fixture, errors) == config['fixtureSha256']
        if producers is not None:
            record['producersUnchanged'] = all([safe_digest(prep / name, errors) == value for name, value in producers.items()])
        if build_file is not None and 'buildVerificationSha256' in record:
            record['buildVerificationUnchanged'] = safe_digest(build_file, errors) == record['buildVerificationSha256']
        if previous_file is not None and config is not None:
            record['previousVerificationUnchanged'] = safe_digest(previous_file, errors) == config['preservedVerificationSha256']
        if identities is not None and base is not None:
            record['preservedRuntimeUnchanged'] = manifest_matches(base, identities, errors)
        if runtime_hashes is not None:
            record['runtimeUnchanged'] = all([safe_digest(runtime / name, errors) == value for name, value in runtime_hashes.items()])
        if human is not None:
            record['humanConfigUnchanged'] = all([(safe_digest(name, errors) if Path(name).is_file() else None) == value
                                                 for name, value in human.items()])
    except BaseException as inspection_error:
        errors.append('final inspection: ' + repr(inspection_error))
    finally:
        record.update(status='terminal', ended=time.time())
        save()
if failure is not None:
    raise failure
assert not errors and all(value for key, value in record.items() if key.endswith('Unchanged'))
analysis = ['wsl.exe', '-d', 'VLC-Juxtopposed-Home-20261018', '--', '/usr/bin/timeout', '--signal=TERM', '--kill-after=10s', '110s', '/usr/bin/python3',
    '/mnt/c/Modding/vlc-juxtopposed/' + (prep / 'analyze-probe.py').relative_to(repo).as_posix()]
with (out / 'analysis.stdout.log').open('wb') as stdout, (out / 'analysis.stderr.log').open('wb') as stderr:
    completed = subprocess.run(analysis, stdout=stdout, stderr=stderr, timeout=125)
assert completed.returncode == 0, 'Observation analyzer failed; preserve artifacts'
print('Observation exited normally. Read analysis.json for diagnosis, never repair acceptance.')
