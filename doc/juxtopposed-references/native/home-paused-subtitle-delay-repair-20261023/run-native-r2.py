from pathlib import Path
from hashlib import sha256
import ctypes, json, shutil, subprocess, sys

work = Path(__file__).resolve().parent
repo = work.parent.parent
old = repo/'build-windows/beta-subtitle-preferences-20261023'
read = lambda p: json.loads(p.read_text(encoding='utf-8-sig'))
digest = lambda p: sha256(p.read_bytes()).hexdigest()


def write(p, value):
    p.write_text(json.dumps(value, indent=2)+'\n', encoding='utf-8', newline='\n')


sys.path.insert(0, str(repo/'test/libvlc'))
from native_http_runtime import stage_runtime, environment

manifest = work/'inputs-ordinary-r1.json'
inputs = read(manifest)['files']
assert all(digest(repo/p) == h for p, h in inputs.items())
application = read(work/'application-r1.json')
assert application['productionApplied']
assert all(digest(repo/p) == h for p, h in application['files'].items())
assert (work/'build-ordinary-r1-exit.txt').read_text().strip() == '0'
base = old/'runtime'
protected = read(old/'reopen-r2/verification.json')['runtimeSha256']
assert all(digest(base/p) == h for p, h in protected.items())
root = work/'native-ordinary-r2'
assert not root.exists()
root.mkdir()
runtime = root/'runtime'
stage_runtime(base, runtime)
(runtime/'libvlccore.dll').unlink()
shutil.copyfile(work/'ordinary-core-r1/libvlccore.dll', runtime/'libvlccore.dll')
identities = {p: digest(runtime/p) for p in protected}
media = repo/'build-windows/beta-external-subtitle-acceptance-20261023/media-r1'
fixtures = {p.name: digest(p) for p in media.iterdir() if p.is_file()}
worker = repo/'test/libvlc/native_paused_subtitle_delay.py'
cases = []
for folder in ['native-delay-candidate-ass-wasapi-r4', 'native-delay-candidate-formats-r4',
               'native-boundary-candidate-r6']:
    for reference in sorted((old/folder).glob('*/probe/result.json')):
        cfg = read(reference)
        assert cfg['passed'] and all(cfg['checks'].values())
        cases.append((reference, cfg))
assert len(cases) == 11
records = []
kernel = ctypes.WinDLL('kernel32', use_last_error=True)
kernel.SetErrorMode.argtypes = [ctypes.c_uint]
kernel.SetErrorMode.restype = ctypes.c_uint
previous = kernel.SetErrorMode(0x8003)
try:
    for index, (reference, cfg) in enumerate(cases):
        mode = cfg.get('seekMode', 'time')
        boundary = cfg.get('boundarySeconds', 20.5)
        name = str(index+1).zfill(2)+'-'+reference.parent.parent.name+'-'+mode
        out = root/name
        out.mkdir()
        probe = out/'probe'
        probe.mkdir()
        command = [sys.executable, str(worker), '--runtime', str(runtime),
                   '--fixture-root', str(media), '--media', cfg['media'],
                   '--audio', cfg['audio'], '--delay-ms', str(cfg['defaultDelayMs']),
                   '--seek-mode', mode, '--boundary-seconds', str(boundary), '--output', str(probe)]
        record = dict(case=name, status='starting', passed=False, privateCore=False,
                      privateGui=False, forcedCleanupUsed=False, fullBetaAccepted=False,
                      runtimeSha256=identities, fixtureSha256=fixtures,
                      reference=str(reference.relative_to(repo)), referenceSha256=digest(reference),
                      sourceManifestSha256=digest(manifest), producerSha256=digest(worker),
                      parentProducerSha256=digest(Path(__file__)))
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
        comparison = {}
        if result and 'observations' in result:
            before_rows = {r['step']: r for r in cfg['observations']}
            current_rows = {r['step']: r for r in result['observations']}
            comparison = {n: dict(candidate=before_rows[n]['frameMarker'],
                                  ordinary=current_rows[n]['frameMarker'])
                          for n in ['reference-first', 'reference-middle',
                                    'boundary-seek', 'boundary-settled']}
        matched = bool(comparison) and all(r['candidate'] == r['ordinary'] for r in comparison.values())
        record.update(status='terminal', exitCode=code, normalExit=code == 0,
                      result=result, pairedFrames=comparison, pairedFramesMatch=matched,
                      passed=bool(result and result.get('passed') and matched),
                      sourceInputsUnchanged=all(digest(repo/p) == h for p, h in inputs.items()),
                      runtimeUnchanged=all(digest(base/p) == h and digest(runtime/p) == identities[p]
                                           for p, h in protected.items()),
                      fixturesUnchanged=all(digest(media/p) == h for p, h in fixtures.items()))
        write(out/'process.json', record)
        records.append(record)
        print(name, 'exit', code, 'passed', record['passed'], 'failed',
              [n for n, v in (result or {}).get('checks', {}).items() if not v], flush=True)
        if code != 0 or not record['passed'] or not all(record[p] for p in [
                'sourceInputsUnchanged', 'runtimeUnchanged', 'fixturesUnchanged']):
            write(root/'processes.json', dict(status='stopped-on-failure', records=records,
                  passed=False, fullBetaAccepted=False))
            raise SystemExit('Inspect this ordinary case before proceeding')
finally:
    kernel.SetErrorMode(previous)
write(root/'processes.json', dict(status='terminal', records=records,
      allProcessesNormal=all(r['normalExit'] for r in records),
      passed=all(r['passed'] for r in records), fullBetaAccepted=False))
