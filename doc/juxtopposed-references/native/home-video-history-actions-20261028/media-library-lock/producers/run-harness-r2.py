from pathlib import Path
from hashlib import file_digest
import json
import subprocess
import time

root = Path(__file__).resolve().parent
build = root / 'build-r2'
assert (build / 'exit.txt').read_text().strip() == '0'
out = root / 'results-r2'
out.mkdir(exist_ok=False)

def sha(path):
    with path.open('rb') as stream:
        return file_digest(stream, 'sha256').hexdigest()

binaries = {Path(line.split('  ', 1)[1]).name: line.split('  ', 1)[0]
            for line in (build / 'binaries.sha256').read_text().splitlines()}
assert all(sha(build / name) == identity for name, identity in binaries.items())
producer = sha(Path(__file__))
sources = {str(path.relative_to(root)): sha(path) for path in root.rglob('*')
           if path.is_file() and path.parts[len(root.parts)] not in ('build', 'build-r2', 'results', 'results-r2')}
record = dict(status='running', producerSha256=producer, sourceSha256=sources,
              binarySha256=binaries, runs=[], started=time.time(),
              scope='Actual SWMRLock logic; deterministic legal wake selection and real Windows standard-CV stress. No VLC/library rebuild.')

def save():
    (out / 'verification.json').write_text(json.dumps(record, indent=2) + '\n')

save()
for label, trials in [('baseline-deterministic', 1), ('candidate-deterministic', 100),
                      ('baseline-native', 500), ('candidate-native', 500)]:
    run = dict(label=label, trials=trials, started=time.time(), forcedCleanupUsed=False)
    with (out / (label + '.stdout.txt')).open('wb') as stdout, (out / (label + '.stderr.txt')).open('wb') as stderr:
        child = subprocess.Popen([str(build / (label + '.exe')), str(trials)],
                                 cwd=out, stdout=stdout, stderr=stderr,
                                 creationflags=subprocess.CREATE_NO_WINDOW)
        run['pid'] = child.pid
        print('Running', label, 'PID', child.pid, flush=True)
        try:
            run['exitCode'] = child.wait(timeout=60)
        except subprocess.TimeoutExpired:
            run['forcedCleanupUsed'] = True
            child.kill()
            run['exitCode'] = child.wait(timeout=5)
    run.update(ended=time.time(), stdout=(out / (label + '.stdout.txt')).read_text(errors='replace'),
               stderr=(out / (label + '.stderr.txt')).read_text(errors='replace'))
    record['runs'].append(run)
    save()
    print(json.dumps(run), flush=True)
record.update(status='terminal', ended=time.time(),
              producerUnchanged=sha(Path(__file__)) == producer,
              sourceUnchanged=all(sha(root / name) == identity for name, identity in sources.items()),
              binariesUnchanged=all(sha(build / name) == identity for name, identity in binaries.items()))
save()
assert all(record[key] for key in ('producerUnchanged', 'sourceUnchanged', 'binariesUnchanged'))
runs = {item['label']: item for item in record['runs']}
assert runs['baseline-deterministic']['exitCode'] == 1
assert not any(item['forcedCleanupUsed'] for item in record['runs'])
assert all(runs[label]['exitCode'] == 0 for label in ('candidate-deterministic', 'candidate-native'))
assert runs['baseline-native']['exitCode'] in (0, 1), 'A native baseline pass means adverse wake choice was not observed.'
