from datetime import datetime, timezone
from hashlib import file_digest
import json
from pathlib import Path
import subprocess
import sys
import fcntl

repo = Path(__file__).resolve().parents[2]
work = Path(__file__).resolve().parent
base = Path('/root/vlc-juxtopposed-build').resolve()
build = (base / 'win64').resolve()
names = subprocess.check_output(['git', '-C', str(repo), 'diff', '--name-only',
    'codex/alpha-fallback-20261027..HEAD', '--', 'include'], text=True).splitlines()
headers = [repo / name for name in names if (repo / name).is_file()]
assert headers
cutoff = max(path.stat().st_mtime for path in headers)
paths = []
dummy = []
for group in ['compat', 'src', 'lib', 'bin', 'modules']:
    folder = build / group
    assert folder.resolve().is_relative_to(build)
    for path in folder.rglob('*'):
        if path.is_file() and path.suffix in ['.Plo', '.Po'] and path.read_text(errors='replace').strip() == '# dummy':
            dummy.append(path.relative_to(build).as_posix())
        if path.is_file() and path.suffix in ['.o', '.lo'] and path.stat().st_mtime < cutoff:
            assert path.resolve().is_relative_to(build)
            paths.append(path)
report = dict(cutoff=datetime.fromtimestamp(cutoff, timezone.utc).isoformat(),
    changedHeaders=[p.relative_to(repo).as_posix() for p in headers],
    objectCount=len(paths), bytes=sum(p.stat().st_size for p in paths),
    dummyDependencyFiles=dummy, staleObjects=[p.relative_to(build).as_posix() for p in paths])
(work / 'stale-object-audit.json').write_text(json.dumps(report, indent=2)+'\n')
print(json.dumps({key: report[key] for key in ['cutoff','objectCount','bytes']}, indent=2))
print('Dummy dependency files:', len(dummy))
if '--apply' not in sys.argv:
    raise SystemExit(0)
lock = (base / '.vlc-build.lock').open('w')
fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
target = base / 'owned-upstream-stale-objects-20261027'
assert target.resolve().is_relative_to(base) and not target.exists()
target.mkdir()
identities = {}
for path in paths:
    name = path.relative_to(build).as_posix()
    destination = target / name
    assert destination.resolve().is_relative_to(target) and not destination.exists()
    with path.open('rb') as stream:
        identities[name] = file_digest(stream, 'sha256').hexdigest()
    destination.parent.mkdir(parents=True, exist_ok=True)
    path.rename(destination)
    with destination.open('rb') as stream:
        assert file_digest(stream, 'sha256').hexdigest() == identities[name]
report.update(quarantine=str(target), quarantinedSha256=identities)
(work / 'stale-object-quarantine.json').write_text(json.dumps(report, indent=2)+'\n')
print('Quarantined stale production objects; freshly rebuilt Qt/Core objects are retained.')
