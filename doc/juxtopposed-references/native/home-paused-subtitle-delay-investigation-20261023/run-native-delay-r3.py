from pathlib import Path
from hashlib import sha256
import argparse, ctypes, json, shutil, subprocess, sys

work = Path(__file__).resolve().parent
repo = work.parent.parent
parser = argparse.ArgumentParser()
parser.add_argument('--batch', choices=['ass-wasapi', 'formats'], required=True)
args = parser.parse_args()
read = lambda p: json.loads(p.read_text(encoding='utf-8-sig'))
digest = lambda p: sha256(p.read_bytes()).hexdigest()


def write(p, value):
    p.write_text(json.dumps(value, indent=2)+'\n', encoding='utf-8', newline='\n')


sys.path.insert(0, str(repo/'test/libvlc'))
from native_http_runtime import environment

prior = read(work/'inputs-r2.json')
for phase in ['initial', 'reopen']:
    check = read(work/(phase+'-r2')/'verification.json')
    assert check['status'] == 'terminal' and check['normalExit']
    assert check['sourceInputsUnchanged'] and check['runtimeUnchanged'] and check['fixturesUnchanged']
runtime = work/'runtime'
identities = read(work/'reopen-r2/verification.json')['runtimeSha256']
assert all(digest(runtime/p) == h for p, h in identities.items())
worker = repo/'test/libvlc/native_paused_subtitle_delay.py'
manifest = work/'inputs-native-r3.json'
if not manifest.exists():
    names = set(prior['files'])
    names.update(['test/libvlc/native_paused_subtitle_delay.py',
                  'test/libvlc/native_paused_subtitle_delay.md',
                  Path(__file__).relative_to(repo).as_posix()])
    forbidden = {'test/libvlc/native_soxr_reference.py',
                 'test/modules/audio_filter/soxr_lifecycle_trace.c',
                 'test/modules/audio_filter/soxr_native_reference.c',
                 'test/modules/audio_filter/soxr_pcm_trace.c'}
    assert not names.intersection(forbidden)
    changed = [p for p, h in prior['files'].items() if digest(repo/p) != h]
    assert changed == ['test/Makefile.am'], changed
    source = work/'source-native-r3'
    assert not source.exists()
    source.mkdir()
    inputs = {p: digest(repo/p) for p in sorted(names)}
    for name, h in inputs.items():
        destination = source/name
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(repo/name, destination)
        assert digest(destination) == h
    write(manifest, dict(files=inputs, sourceCount=len(inputs), sourceSnapshot=str(source),
                         fullBetaAccepted=False))
inputs = read(manifest)['files']
assert all(digest(repo/p) == h for p, h in inputs.items())
media = repo/'build-windows/beta-external-subtitle-acceptance-20261023/media-r1'
fixtures = {name: digest(media/name) for name in ['frame-marker.mkv',
            'frame-marker-ass.mkv', 'frame-marker.mp4']}
if args.batch == 'ass-wasapi':
    cases = [('frame-marker-ass.mkv', 'wasapi', delay) for delay in [0, 1200]]
else:
    cases = [(m, a, delay) for m, a in [('frame-marker.mkv', 'wasapi'),
                                      ('frame-marker.mp4', 'wasapi'),
                                      ('frame-marker-ass.mkv', 'none')]
             for delay in [0, 1200]]
root = work/('native-delay-'+args.batch+'-r3')
assert not root.exists()
root.mkdir()
records = []
kernel = ctypes.WinDLL('kernel32', use_last_error=True)
kernel.SetErrorMode.argtypes = [ctypes.c_uint]
kernel.SetErrorMode.restype = ctypes.c_uint
previous = kernel.SetErrorMode(0x8003)
try:
    for media_name, audio, delay in cases:
        name = media_name+'-'+audio+'-'+str(delay)
        out = root/name
        out.mkdir()
        probe = out/'probe'
        probe.mkdir()
        command = [sys.executable, str(worker), '--runtime', str(runtime),
                   '--fixture-root', str(media), '--media', media_name,
                   '--audio', audio, '--delay-ms', str(delay), '--output', str(probe)]
        record = dict(case=name, status='starting', passed=False,
                      forcedCleanupUsed=False, privateCore=False, privateGui=False,
                      sourceManifestSha256=digest(manifest), runtimeSha256=identities,
                      fixtureSha256=fixtures, producerSha256=digest(worker),
                      parentProducerSha256=digest(Path(__file__)), fullBetaAccepted=False)
        with (out/'stdout.txt').open('wb') as stdout, (out/'stderr.txt').open('wb') as stderr:
            child = subprocess.Popen(command, cwd=runtime, env=environment(runtime, out),
                                     stdout=stdout, stderr=stderr,
                                     creationflags=subprocess.CREATE_NO_WINDOW)
            record.update(status='running', pid=child.pid)
            write(out/'process.json', record)
            print(name, 'PID', child.pid, flush=True)
            try:
                code = child.wait(timeout=180)
            except subprocess.TimeoutExpired:
                record.update(status='observation-timeout-process-left-running')
                write(out/'process.json', record)
                raise SystemExit('Inspect saved PID; no automatic cleanup/restart')
        result = read(probe/'result.json') if (probe/'result.json').exists() else None
        record.update(status='terminal', exitCode=code, normalExit=code == 0,
                      passed=bool(result and result.get('passed')), result=result,
                      sourceInputsUnchanged=all(digest(repo/p) == h for p, h in inputs.items()),
                      runtimeUnchanged=all(digest(runtime/p) == h for p, h in identities.items()),
                      fixturesUnchanged=all(digest(media/p) == h for p, h in fixtures.items()))
        write(out/'process.json', record)
        records.append(record)
        print(name, 'exit', code, 'failed',
              [n for n, v in (result or {}).get('checks', {}).items() if not v], flush=True)
        if code != 0 or not result or result.get('status') != 'terminal':
            raise SystemExit('Inspect execution failure before proceeding')
        assert record['sourceInputsUnchanged'] and record['runtimeUnchanged'] and record['fixturesUnchanged']
finally:
    kernel.SetErrorMode(previous)
write(root/'processes.json', dict(records=records,
      allProcessesNormal=all(r['normalExit'] for r in records),
      allChecksPassed=all(r['passed'] for r in records), fullBetaAccepted=False))
