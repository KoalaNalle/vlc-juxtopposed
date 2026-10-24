from pathlib import Path
from hashlib import sha256
import ctypes, json, shutil, subprocess, sys

work = Path(__file__).resolve().parent
repo = work.parent.parent
old = repo/'build-windows/beta-tracks-subtitles-20261023'
investigation = repo/'build-windows/beta-subtitle-preferences-20261023'
read = lambda p: json.loads(p.read_text(encoding='utf-8-sig'))
digest = lambda p: sha256(p.read_bytes()).hexdigest()


def write(p, value):
    p.write_text(json.dumps(value, indent=2)+'\n', encoding='utf-8', newline='\n')


sys.path.insert(0, str(repo/'test/libvlc'))
from native_http_runtime import stage_runtime, environment

manifest = work/'inputs-r7.json'
inputs = read(manifest)['files']
assert all(digest(repo/p) == h for p, h in inputs.items())
assert read(investigation/'paired-frames-r4.json')['allPairedSeekFramesMatch']
assert read(investigation/'paired-frames-r4.json')['allCandidateChecksPass']
ordinary = read(repo/'build-windows/beta-subtitle-delay-repair-20261023/native-ordinary-r2/processes.json')
assert ordinary['passed'] and len(ordinary['records']) == 11
for batch in ['ass-wasapi', 'formats']:
    summary = read(investigation/('native-delay-candidate-'+batch+'-r4')/'processes.json')
    assert summary['allChecksPassed'] and summary['allProcessesNormal']
base = work/'native-after-r6/runtime'
protected = read(work/'native-after-r6/verification.json')['runtimeSha256']
assert all(digest(base/p) == h for p, h in protected.items())
root = work/'regressions-ordinary-r7'
assert not root.exists()
root.mkdir()
shutil.copyfile(Path(__file__), root/'parent.py')
parent_hash = digest(Path(__file__))
runtime = root/'runtime'
stage_runtime(base, runtime)
(runtime/'libvlccore.dll').unlink()
shutil.copyfile(work/'ordinary-r3-bin/libvlccore.dll', runtime/'libvlccore.dll')
identities = {p: digest(runtime/p) for p in protected}
media_root = repo/'build-windows/beta-external-subtitle-acceptance-20261023/media-r1'
abi = repo/'build-windows/beta-subtitle-frame-feedback-20261023/layout-r1.json'
provenance = read(abi.parent/'layout-provenance-r1.json')
assert provenance['headerSha256'] == digest(repo/'include/vlc/libvlc_media_player.h')
assert provenance['sourceSha256'] == digest(repo/'test/libvlc/native_player_callback_layout.c')
definitions = []
for p in sorted((old/'native-startup-ordinary-r14').glob('*/config.json')):
    cfg = read(p)
    assert all(v == '--no-spu' for v in cfg['extraOptions'])
    definitions.append(('startup', cfg['name'], p, cfg))
assert len(definitions) == 9
for p in sorted((old/'native-subtitle-ordinary-r36c/after').glob('*/scenario.json')):
    cfg = read(p)
    definitions.append(('subtitle', cfg['name'], p, cfg))
assert len(definitions) == 17
for media in ['frame-marker.mkv', 'frame-marker-ass.mkv', 'frame-marker.mp4']:
    for audio in ['wasapi', 'none']:
        definitions.append(('tracks', media+'-'+audio, None,
                            dict(media=media, audio=audio, external='external.srt', count=1)))
definitions.append(('tracks', 'frame-marker.mkv-wasapi-mks', None,
                    dict(media='frame-marker.mkv', audio='wasapi', external='multi-language.mks', count=2)))
assert len(definitions) == 24
records = []
kernel = ctypes.WinDLL('kernel32', use_last_error=True)
kernel.SetErrorMode.argtypes = [ctypes.c_uint]
kernel.SetErrorMode.restype = ctypes.c_uint
previous = kernel.SetErrorMode(0x8003)
try:
    for family, name, original, cfg in definitions:
        out = root/(family+'-'+name)
        out.mkdir()
        probe = out/'probe'
        probe.mkdir()
        config = out/'config.json'
        write(config, cfg)
        if family == 'tracks':
            worker = repo/'test/libvlc/native_paused_subtitle_tracks.py'
            fixtures = {p.name: digest(p) for p in media_root.iterdir() if p.is_file()}
            command = [sys.executable, str(worker), '--fixture-root', str(media_root),
                       '--media', cfg['media'], '--audio', cfg['audio'],
                       '--external-selection', 'automatic', '--external-policy', 'ids',
                       '--external-file', cfg['external'], '--external-track-count', str(cfg['count']),
                       '--frame-feedback-abi', str(abi)]
        else:
            worker = repo/('test/libvlc/native_paused_video_startup.py' if family == 'startup'
                           else 'test/libvlc/native_paused_subtitle_seek.py')
            media = Path(cfg['media']) if family == 'startup' else old/cfg['media']
            fixtures = {str(media): digest(media)}
            if family == 'subtitle':
                for step in cfg['steps']:
                    if 'referencePicture' in step:
                        picture = old/step['referencePicture']
                        fixtures[str(picture)] = digest(picture)
            command = [sys.executable, str(worker),
                       '--config' if family == 'startup' else '--scenario', str(config)]
            if family == 'subtitle':
                command += ['--fixture-root', str(old)]
        command += ['--runtime', str(runtime), '--output', str(probe)]
        record = dict(family=family, case=name, status='starting', passed=False,
                      privateCore=False, privateGui=False, fullBetaAccepted=False,
                      forcedCleanupUsed=False, sourceManifestSha256=digest(manifest),
                      runtimeSha256=identities, fixtureSha256=fixtures,
                      producerSha256=digest(worker), parentProducerSha256=parent_hash,
                      originalConfigSha256=digest(original) if original else None)
        with (out/'stdout.txt').open('wb') as stdout, (out/'stderr.txt').open('wb') as stderr:
            child = subprocess.Popen(command, cwd=runtime, env=environment(runtime, out),
                                     stdout=stdout, stderr=stderr,
                                     creationflags=subprocess.CREATE_NO_WINDOW)
            record.update(status='running', pid=child.pid)
            write(out/'process.json', record)
            print(family, name, 'PID', child.pid, flush=True)
            try:
                code = child.wait(timeout=180)
            except subprocess.TimeoutExpired:
                record.update(status='observation-timeout-process-left-running')
                write(out/'process.json', record)
                raise SystemExit('Inspect saved PID; no automatic cleanup/restart')
        result = read(probe/'result.json') if (probe/'result.json').exists() else None
        fixture_root = media_root if family == 'tracks' else Path()
        record.update(status='terminal', exitCode=code, normalExit=code == 0,
                      passed=bool(result and result.get('passed')), result=result,
                      sourceInputsUnchanged=all(digest(repo/p) == h for p, h in inputs.items()),
                      parentUnchanged=digest(Path(__file__)) == parent_hash == digest(root/'parent.py'),
                      runtimeUnchanged=all(digest(base/p) == h and digest(runtime/p) == identities[p]
                                           for p, h in protected.items()),
                      fixturesUnchanged=all(digest(fixture_root/p) == h for p, h in fixtures.items()))
        write(out/'process.json', record)
        records.append(record)
        print(family, name, 'exit', code, 'passed', record['passed'], flush=True)
        if code != 0 or not record['passed'] or not all(record[p] for p in [
                'sourceInputsUnchanged', 'parentUnchanged', 'runtimeUnchanged', 'fixturesUnchanged']):
            write(root/'processes.json', dict(status='stopped-on-failure', records=records,
                  passed=False, fullBetaAccepted=False))
            raise SystemExit('Inspect this regression before proceeding')
finally:
    kernel.SetErrorMode(previous)
write(root/'processes.json', dict(status='terminal', records=records,
      allProcessesNormal=all(r['normalExit'] for r in records),
      passed=all(r['passed'] for r in records), fullBetaAccepted=False))
