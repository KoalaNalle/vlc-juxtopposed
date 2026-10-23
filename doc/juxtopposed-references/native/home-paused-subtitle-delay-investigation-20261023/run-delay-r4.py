from pathlib import Path
from hashlib import sha256
import argparse, ctypes, json, shutil, subprocess, sys

work = Path(__file__).resolve().parent
repo = work.parent.parent
parser = argparse.ArgumentParser()
parser.add_argument('--variant', choices=['before', 'candidate'], required=True)
parser.add_argument('--batch', choices=['ass-wasapi', 'formats'], required=True)
args = parser.parse_args()
read = lambda p: json.loads(p.read_text(encoding='utf-8-sig'))
digest = lambda p: sha256(p.read_bytes()).hexdigest()


def write(p, value):
    p.write_text(json.dumps(value, indent=2)+'\n', encoding='utf-8', newline='\n')


sys.path.insert(0, str(repo/'test/libvlc'))
from native_http_runtime import stage_runtime, environment

manifest = work/'inputs-candidate-r4.json'
inputs = read(manifest)['files']
assert all(digest(repo/p) == h for p, h in inputs.items())
base = work/'runtime'
protected = read(work/'reopen-r2/verification.json')['runtimeSha256']
assert all(digest(base/p) == h for p, h in protected.items())
media = repo/'build-windows/beta-external-subtitle-acceptance-20261023/media-r1'
fixtures = {p.name: digest(p) for p in media.iterdir() if p.is_file()}
if args.variant == 'candidate':
    assert (work/'build-candidate-r4-exit.txt').read_text().strip() == '0'
    assert all(digest(repo/p) == h for p, h in read(work/'candidate-r4.json')['preimages'].items())
cases = [('frame-marker-ass.mkv', 'wasapi', n) for n in [0, 1200]]
if args.batch == 'formats':
    cases = [(m, a, n) for m, a in [('frame-marker.mkv', 'wasapi'),
                                   ('frame-marker.mp4', 'wasapi'),
                                   ('frame-marker-ass.mkv', 'none')]
             for n in [0, 1200]]
root = work/('native-delay-'+args.variant+'-'+args.batch+'-r4')
assert not root.exists()
root.mkdir()
runtime = root/'runtime'
stage_runtime(base, runtime)
if args.variant == 'candidate':
    (runtime/'libvlccore.dll').unlink()
    shutil.copyfile(work/'candidate-r4-bin/libvlccore.dll', runtime/'libvlccore.dll')
identities = {p: digest(runtime/p) for p in protected}
worker = repo/'test/libvlc/native_paused_subtitle_delay.py'
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
        record = dict(case=name, variant=args.variant, status='starting', passed=False,
                      privateCore=args.variant == 'candidate', privateGui=False,
                      forcedCleanupUsed=False, fullBetaAccepted=False,
                      runtimeSha256=identities, fixtureSha256=fixtures,
                      sourceManifestSha256=digest(manifest), producerSha256=digest(worker),
                      parentProducerSha256=digest(Path(__file__)))
        with (out/'stdout.txt').open('wb') as stdout, (out/'stderr.txt').open('wb') as stderr:
            child = subprocess.Popen(command, cwd=runtime, env=environment(runtime, out),
                                     stdout=stdout, stderr=stderr,
                                     creationflags=subprocess.CREATE_NO_WINDOW)
            record.update(status='running', pid=child.pid)
            write(out/'process.json', record)
            print(args.variant, name, 'PID', child.pid, flush=True)
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
                      runtimeUnchanged=all(digest(base/p) == h and digest(runtime/p) == identities[p]
                                           for p, h in protected.items()),
                      fixturesUnchanged=all(digest(media/p) == h for p, h in fixtures.items()))
        write(out/'process.json', record)
        records.append(record)
        print(args.variant, name, 'exit', code, 'failed',
              [n for n, v in (result or {}).get('checks', {}).items() if not v], flush=True)
        if code != 0 or not result or result.get('status') != 'terminal':
            raise SystemExit('Inspect execution failure before proceeding')
        assert record['sourceInputsUnchanged'] and record['runtimeUnchanged'] and record['fixturesUnchanged']
finally:
    kernel.SetErrorMode(previous)
write(root/'processes.json', dict(records=records,
      allProcessesNormal=all(r['normalExit'] for r in records),
      allChecksPassed=all(r['passed'] for r in records), fullBetaAccepted=False))
