from pathlib import Path
from hashlib import file_digest
import json
import sys
import os
import shutil
import time

root = Path(__file__).resolve().parent
repo = root.parents[1]
sys.path.insert(0, str(repo / 'test/libvlc'))
import native_http_runtime as runtime_helper
from native_http_runtime import stage_runtime, regenerate_cache, run_owned
_original_environment = runtime_helper.environment
def isolated_environment(runtime, output):
    env = _original_environment(runtime, output)
    env.pop('VLC_PLUGIN_PATH', None)
    return env
runtime_helper.environment = isolated_environment
variant = sys.argv[1]
assert variant in ('candidate', 'baseline')

def digest(path):
    with path.open('rb') as stream:
        return file_digest(stream, 'sha256').hexdigest()

baseline = root / 'ui_ass_replay_av2/runtime'
previous = json.loads((baseline.parent / 'verification.json').read_text())
assert previous['status'] == 'terminal' and previous['exitCode'] == 0
identities = previous['runtimeSha256']
assert all(digest(baseline / name) == value for name, value in identities.items())
source = repo / 'modules/demux/avformat/mux.c'
source_hash = digest(source)
assert source_hash == previous['sourceSnapshotSha256'][str(source)]
fixture = root / 'media-record-flac/frame-marker.mkv'
fixture_hash = digest(fixture)
assert fixture_hash == 'ff9c28f52e759fcfcfd657dda5af5d3115b29d0255363b208dbbc08c9ae16a21'
producer = digest(Path(__file__))
out = root / ('forced_srt_matroska_clean_' + variant)
out.mkdir(exist_ok=False)
runtime = out / 'runtime'
stage = stage_runtime(baseline, runtime)
if variant == 'baseline':
    original_plugin = repo / 'build-windows/beta-follow-20261027/runtime-r7/plugins/codec/libavcodec_plugin.dll'
    target = runtime / 'plugins/codec/libavcodec_plugin.dll'
    target.unlink()
    shutil.copyfile(original_plugin, target)
    identities = dict(identities)
    identities['plugins/codec/libavcodec_plugin.dll'] = digest(original_plugin)
regenerate_cache(runtime, out / 'cache')
output = out / 'output.mkv'
command = [str(runtime / 'vlc.exe'), '--ignore-config', '--intf=dummy',
    '--no-one-instance', '--no-media-library', '--no-save-recentplay',
    '--no-metadata-network-access', '--no-ml-network-discovery', '--no-auto-preparse',
    '--extraintf=', '--control=', '--no-crashdump', '--play-and-exit',
    '--file-logging', '--verbose=2', '--log-verbose=2', '--logfile=' + str(out / 'vlc.log'),
    '--sout-all', '--sout=#standard{access=file{no-overwrite},mux=avformat{mux=matroska},dst=' + output.as_posix() + '}',
    str(fixture)]
record = dict(status='running', producerSha256=producer, sourceSha256=source_hash,
    fixtureSha256=fixture_hash, runtimeSha256=identities, stage=stage, command=command,
    started=time.time(), scope='Explicit libavformat Matroska passthrough of generated H264/FLAC/embedded SRT; no desktop interaction.')
def save():
    (out / 'verification.json').write_text(json.dumps(record, indent=2) + '\n')
save()
record.update(run_owned(command, runtime, out, timeout=85))
record.update(status='terminal', ended=time.time(), sourceUnchanged=digest(source)==source_hash,
    fixtureUnchanged=digest(fixture)==fixture_hash, producerUnchanged=digest(Path(__file__))==producer,
    baselineUnchanged=all(digest(baseline/name)==h for name,h in previous['runtimeSha256'].items()),
    variant=variant, runtimeUnchanged=all(digest(runtime/name)==h
                         for name,h in identities.items() if not name.endswith('plugins.dat')))
if output.exists():
    record.update(outputSha256=digest(output), outputBytes=output.stat().st_size)
save()
print(json.dumps({k:v for k,v in record.items() if k not in ('runtimeSha256','command')}, indent=2))
assert record['normalExit'] and all(record[k] for k in ('sourceUnchanged','fixtureUnchanged','producerUnchanged','runtimeUnchanged'))
