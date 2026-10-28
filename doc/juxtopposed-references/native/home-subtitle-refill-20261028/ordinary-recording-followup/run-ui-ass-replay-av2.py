from hashlib import file_digest
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time

repo = Path(__file__).resolve().parents[2]
work = repo / 'build-windows/beta-follow-20261027'
label = sys.argv[1]
mode = sys.argv[2]
assert mode in ['redesigned', 'classic']
out = repo / 'build-windows/beta-fps-20261028' / label
out.mkdir(exist_ok=False)
sys.path.insert(0, str(repo / 'test/libvlc'))
from native_http_runtime import stage_runtime, regenerate_cache, environment

def digest(path):
    with path.open('rb') as stream: return file_digest(stream, 'sha256').hexdigest()
def write(value):
    (out / 'verification.json').write_text(json.dumps(value, indent=2)+'\n')

package = work / 'runtime-r7'
manifest = json.loads((work / 'runtime-r7-manifest.json').read_text())
assert all(digest(package / p) == v['sha256'] for p, v in manifest['files'].items())
runtime = out / 'runtime'
stage_runtime(package, runtime)
candidate = repo / 'build-windows/beta-fps-20261028/core-r5/libvlccore.dll'
assert digest(candidate) == 'f8fc980bbe03da57908f79045f266710dd27a16c6ee66b6c45d60cd1f9b93598'
(runtime / 'libvlccore.dll').unlink()
shutil.copyfile(candidate, runtime / 'libvlccore.dll')
for relative, built, expected in [('plugins/codec/liblibass_plugin.dll', 'ass-plugin-r2/liblibass_plugin.dll', 'b622871c205345373343a463f193271b12cf875853e7c913c707acadb16e9b40'), ('plugins/codec/libavcodec_plugin.dll', 'avcodec-plugin-r2/libavcodec_plugin.dll', '545f715418c4d20659dce5d6c81ee33b292b21c87243fafa62036b9ffd739e1f'), ('plugins/demux/libsubtitle_plugin.dll', 'subtitle-plugin-r1/libsubtitle_plugin.dll', '1e8abb4593f82b5badf9dc9d103f655d4a0f30334c28f89e24b102b301139ea8')]:
    replacement = repo / 'build-windows/beta-fps-20261028' / built
    assert digest(replacement) == expected
    (runtime / relative).unlink()
    shutil.copyfile(replacement, runtime / relative)
regenerate_cache(runtime, out / 'cache')
config = out / 'vlcrc'
policy = 0
config.write_text('metadata-network-access=0\nml-network-discovery=0\none-instance=0\nqt-updates-notif=0\nqt-remember-window-geometry=0\nqt-controls-auto-hide='+str(policy)+'\nqt-controls-hide-timeout=1000\nqt-timeline-thumbnails=0\nsub-autodetect-file=0\n', encoding='utf-8')
recordings = out / 'recordings'
recordings.mkdir()
with config.open('a', encoding='utf-8') as f: f.write('input-record-path='+str(recordings)+'\nsub-fps=0\n')
ini = runtime / 'portable/vlc/vlc-qt-interface.ini'
ini.parent.mkdir(parents=True, exist_ok=True)
ini.write_text('[General]\ngraphics-api=4\nfiledialog-path='+(work / 'media').as_uri()+'\n[MainWindow]\ncolor-scheme-index=2\ninterface-scale=1\n', encoding='utf-8')
shutil.copyfile(ini, out / 'settings-before.ini')
snapshots = out / 'snapshots'
snapshots.mkdir()
media, = (repo / 'build-windows/beta-fps-20261028/ui_ass_record_av2/recordings').glob('*.mkv')
external = repo / 'build-windows/beta-fps-20261028/media-formats/subtitle-style.ass'
fixture_paths = [media, external]
fixtures = {str(p): digest(p) for p in fixture_paths}
human = [Path(os.environ['APPDATA']) / 'vlc' / name for name in ['vlcrc', 'vlc-qt-interface.ini']]
human_before = {str(p): digest(p) if p.is_file() else None for p in human}
command = [runtime / 'vlc.exe', '--intf=qt', '--no-one-instance', '--no-media-library',
           '--config='+str(config), '--no-crashdump', '--no-qt-privacy-ask', '--no-qt-updates-notif',
           '--no-metadata-network-access', '--no-ml-network-discovery', '--restore-playback-pos=0',
           '--no-qt-video-autoresize', '--no-qt-remember-window-geometry',
           '--file-logging', '--verbose=2', '--log-verbose=2', '--logfile='+str(out / 'vlc.log'),
           '--snapshot-path='+str(snapshots), '--snapshot-format=png',
           '--qt-juxtopposed-controls' if mode == 'redesigned' else '--no-qt-juxtopposed-controls']
if mode == 'redesigned':
    command += ['--start-paused', '--start-time=0', '--rate=1', media]
env = environment(runtime, out)
env.pop('VLC_PLUGIN_PATH', None)
binary = {p: digest(runtime / p) for p, v in manifest['files'].items() if p.endswith(('.dll', '.exe'))}
owned_sources = [repo / p for p in ['src/input/input.c', 'src/input/es_out.c', 'src/input/es_out.h', 'modules/codec/libass.c', 'modules/demux/subtitle.c', 'modules/demux/avformat/mux.c', 'modules/stream_out/record.c']]
source_snapshot = {str(p): digest(p) for p in owned_sources}
report = dict(sourceSnapshotSha256=source_snapshot, status='starting', scope='Ordinary r7 GUI/resources with core r5, libass r2, text demux r1 and avcodec r2; replaying the generated ASS recording with embedded subtitles only, no external attachment and normal FPS; isolated profile, actual Windows desktop; no final-package qualification.',
              privateHooks=False, experimentalCoreSha256=digest(candidate), sourceCommit=manifest['sourceCommit'], sourcePatchSha256=manifest['sourcePatchSha256'], producerSha256=digest(Path(__file__)),
              runtimeSha256=binary, mediaSha256=digest(media), externalSha256=digest(external),
              fixtureSha256=fixtures, humanBeforeSha256=human_before,
              mode=mode, autoHidePolicy=policy, command=[str(p) for p in command], started=time.time())
with (out / 'stdout.txt').open('wb') as stdout, (out / 'stderr.txt').open('wb') as stderr:
    child = subprocess.Popen([str(p) for p in command], cwd=runtime, env=env, stdout=stdout,
                             stderr=stderr, creationflags=subprocess.CREATE_NO_WINDOW)
    report.update(status='running', pid=child.pid)
    write(report)
    print(label, 'owned VLC PID', child.pid, flush=True)
    try:
        code = child.wait(timeout=1200)
    except subprocess.TimeoutExpired:
        report.update(status='timeout-process-left-running')
        write(report)
        raise SystemExit('Inspect the recorded owned PID; no automatic restart or cleanup.')
logs = '\n'.join((out / p).read_text(encoding='utf-8', errors='replace') for p in ['vlc.log', 'stderr.txt'])
report.update(sourceUnchanged=all(digest(Path(p)) == h for p,h in source_snapshot.items()), status='terminal', exitCode=code, normalExit=code == 0,
    runtimeUnchanged=all(digest(runtime / p) == h for p, h in binary.items()),
    packageUnchanged=all(digest(package / p) == v['sha256'] for p, v in manifest['files'].items()),
    fixturesUnchanged=all(digest(Path(p)) == h for p, h in fixtures.items()),
    humanConfigUnchanged=all((digest(Path(p)) if Path(p).is_file() else None) == h for p, h in human_before.items()),
    producerUnchanged=digest(Path(__file__)) == report['producerSha256'],
    moduleCounts=re.findall(r'plug-ins loaded: (\d+) modules', logs),
    diagnostics=[line for line in logs.splitlines() if re.search(r'\berror:|Assertion failed|Binding loop|ReferenceError|TypeError|Svg Image Provider:|SVGColorizer: can not open', line)],
    ended=time.time())
shutil.copyfile(ini, out / 'settings-after.ini')
write(report)
print(json.dumps({k: report[k] for k in ['status','pid','exitCode','normalExit','runtimeUnchanged','packageUnchanged','fixturesUnchanged','humanConfigUnchanged','producerUnchanged','moduleCounts','diagnostics']}), flush=True)
assert report['normalExit'] and all(report[k] for k in ['runtimeUnchanged','packageUnchanged','fixturesUnchanged','humanConfigUnchanged','producerUnchanged'])
assert not report['diagnostics'], 'Review ordinary desktop diagnostics before acceptance.'
