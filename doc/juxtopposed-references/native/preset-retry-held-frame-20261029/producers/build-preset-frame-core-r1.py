"""Normal Core rebuild with full small shared-src backup and exact restoration."""
import fcntl
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess

ROOT = Path('/mnt/c/Modding/vlc-juxtopposed')
BASE = Path('/root/vlc-juxtopposed-build')
SRC = BASE / 'win64/src'
OUT = ROOT / 'build-windows/beta-preset-frame-20261029/core-build-r1'
BACKUP = BASE / 'preset-frame-r1-original-src'
FALLBACK = ROOT / 'build-windows/beta-resume-20261029/gui-r24/runtime'


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def snapshot(path):
    if path.is_symlink():
        return {'kind': 'symlink', 'target': os.readlink(path)}
    return {'kind': 'file', 'size': path.stat().st_size, 'sha256': sha(path)}


def tree(path):
    return {str(item.relative_to(path)): snapshot(item) for item in path.rglob('*')
            if item.is_file() or item.is_symlink()}


def dependencies(path):
    line = path.read_text().replace('\\\n', ' ').splitlines()[0]
    return {str((SRC / name).resolve()) for name in shlex.split(line.split(':', 1)[1])}


def main():
    assert not OUT.exists() and not BACKUP.exists()
    assert SRC.resolve().is_relative_to(BASE.resolve())
    assert shutil.disk_usage(ROOT).free >= 399864390452 + 1024**3
    OUT.mkdir(parents=True)

    def write(name, value):
        (OUT / name).write_text(json.dumps(value, indent=2) + '\n')

    with (BASE / '.vlc-build.lock').open('a') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        assert sha(SRC / '.libs/libvlccore.dll') == 'f09c45fe4075a02836807f00cab03163cdba1ec79e37ba35a0328f7cb6d5462d'
        before = tree(SRC)
        assert sum(item.get('size', 0) for item in before.values()) < 128 * 1024**2
        affected = [path for path in SRC.rglob('*.Plo') if 'player/player.h' in path.read_text()]
        assert len(affected) == 21
        deps = set().union(*(dependencies(path) for path in affected))
        compiler = Path(shutil.which('x86_64-w64-mingw32-gcc')).resolve()
        inputs = {name: sha(name) for name in deps | {str(compiler), str(Path(__file__))}}
        sources = {str(path): sha(path) for name in ('src', 'include', 'compat')
                   for path in (ROOT / name).rglob('*')
                   if path.is_file() and path.suffix in ('.c', '.h', '.in', '.am', '.sym')}
        fallback = json.loads((FALLBACK.parent / 'verification.json').read_text())['runtimeSha256']
        assert all(sha(FALLBACK / name) == value for name, value in fallback.items())
        write('before.json', {'sharedTree': before, 'sources': sources, 'compilerInputs': inputs,
                              'fallback': fallback, 'affectedDependencyRecords': [str(path.relative_to(SRC)) for path in affected]})
        shutil.copytree(SRC, BACKUP, symlinks=True)
        assert tree(BACKUP) == before
        result = {'status': 'not-built', 'forcedCleanupUsed': False}
        try:
            # Preserve any runtime hardlinks to the original shared DLL.
            (SRC / '.libs/libvlccore.dll').unlink()
            env = os.environ.copy()
            env.pop('CC', None)
            env.pop('CXX', None)
            env['PATH'] = str(BASE / 'extras/tools/build/bin') + ':' + str(BASE / 'contrib/bin') + ':' + env['PATH']
            command = ['make', '-C', str(SRC), '-j2', 'V=1', 'libvlccore.la']
            write('command.json', command)
            with (OUT / 'build.log').open('w') as log:
                child = subprocess.Popen(command, env=env, stdout=log, stderr=subprocess.STDOUT)
                write('running.json', {'pid': child.pid})
                code = child.wait()
            (OUT / 'exit.txt').write_text(str(code) + '\n')
            assert code == 0
            candidate = OUT / 'candidate'
            candidate.mkdir()
            shutil.copy2(SRC / '.libs/libvlccore.dll', candidate / 'libvlccore.dll')
            actual = set().union(*(dependencies(path) for path in affected))
            assert actual <= inputs.keys()
            for path in affected:
                target = OUT / 'actual-dependencies' / path.relative_to(SRC)
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(path, target)
            result.update(status='built-not-qualified', candidateSha256=sha(candidate / 'libvlccore.dll'),
                          compilerSha256=sha(compiler), affectedObjects=len(affected), dependencyCount=len(actual))
        finally:
            built = tree(SRC)
            changed = [name for name in before.keys() | built.keys() if before.get(name) != built.get(name)]
            for name in changed:
                target = SRC / name
                assert target.parent.resolve().is_relative_to(SRC.resolve())
                if target.exists() or target.is_symlink():
                    target.unlink()
                if name in before:
                    target.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copy2(BACKUP / name, target, follow_symlinks=False)
            after = tree(SRC)
            result.update(sharedTreeRestored=after == before, changedAndRestored=changed,
                          inputsUnchanged=all(sha(name) == value for name, value in inputs.items()),
                          sourcesUnchanged=all(sha(name) == value for name, value in sources.items()),
                          fallbackUnchanged=all(sha(FALLBACK / name) == value for name, value in fallback.items()))
            write('verification.json', result)
            print(json.dumps(result), flush=True)
        assert result['status'] == 'built-not-qualified' and result['sharedTreeRestored']
        assert result['inputsUnchanged'] and result['sourcesUnchanged'] and result['fallbackUnchanged']


if __name__ == '__main__':
    main()
