"""One-use normal incremental Core build, preserving the tested shared outputs."""
import fcntl
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess

ROOT = Path('/mnt/c/Modding/vlc-juxtopposed')
BASE = Path('/root/vlc-juxtopposed-build')
SRC = BASE / 'win64/src'
OUT = ROOT / 'build-windows/beta-record-transition-20261029/core-build-r3'
BACKUP = BASE / 'record-transition-r3-original'
OWNED = ['libvlccore.la', '.libs/libvlccore.dll', '.libs/libvlccore.dll.a',
         '.libs/libvlccore.la', '.libs/libvlccore.lai', '.libs/libvlccore.def',
         '.libs/libvlccore.dll.def', 'player/libvlccore_la-input.lo',
         'player/libvlccore_la-input.o', 'player/.libs/libvlccore_la-input.o',
         'player/.deps/libvlccore_la-input.Plo']


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def snapshot(path):
    if path.is_symlink():
        return dict(kind='symlink', target=os.readlink(path))
    return dict(kind='file', size=path.stat().st_size, sha256=sha(path))


def write(name, data):
    (OUT / name).write_text(json.dumps(data, indent=2) + '\n')


def main():
    assert not OUT.exists() and not BACKUP.exists()
    assert shutil.disk_usage(ROOT).free >= 399864390452 + 512 * 1024**2
    OUT.mkdir(parents=True)
    with (BASE / '.vlc-build.lock').open('a') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        assert sha(SRC / '.libs/libvlccore.dll') == 'f09c45fe4075a02836807f00cab03163cdba1ec79e37ba35a0328f7cb6d5462d'
        assert sha(ROOT / 'src/player/input.c') == 'f85d9bfa8801096b0853c25875ae9e937454bbd3588560f2a931d133bdbde20e'
        assert (SRC / 'player/.deps/libvlccore_la-input.Plo').stat().st_size > 20000
        sources = {str(p): sha(p) for name in ['src', 'include', 'compat']
                   for p in (ROOT / name).rglob('*')
                   if p.is_file() and p.suffix in ('.c', '.h', '.in', '.am', '.sym')}
        sources[str(Path(__file__))] = sha(Path(__file__))
        tree = {str(p.relative_to(SRC)): snapshot(p) for p in SRC.rglob('*')
                if p.is_file() or p.is_symlink()}
        baseline = ROOT / 'build-windows/beta-resume-20261029/gui-r9/runtime'
        fallback = {str(p): sha(p) for p in baseline.rglob('*')
                    if p.is_file() and 'portable' not in p.relative_to(baseline).parts}
        write('before.json', dict(sources=sources, sharedTree=tree, fallback=fallback,
                                   owned=OWNED, cFree=shutil.disk_usage(ROOT).free))
        assert sum(tree[n].get('size', 0) for n in OWNED if n in tree) < 64 * 1024**2
        BACKUP.mkdir()
        moved = []
        result = dict(status='not-built', forcedCleanupUsed=False)
        try:
            for name in OWNED:
                source, target = SRC / name, BACKUP / name
                assert source.parent.resolve().is_relative_to(SRC.resolve())
                if source.exists() or source.is_symlink():
                    target.parent.mkdir(parents=True, exist_ok=True)
                    os.rename(source, target)
                    moved.append(name)
                    if name != '.libs/libvlccore.dll':
                        shutil.copy2(target, source, follow_symlinks=False)
            env = os.environ.copy()
            env.pop('CC', None); env.pop('CXX', None)
            env['PATH'] = str(BASE / 'extras/tools/build/bin') + ':' + str(BASE / 'contrib/bin') + ':' + env['PATH']
            command = ['make', '-C', str(SRC), '-j2', 'V=1', 'libvlccore.la']
            write('command.json', command)
            with (OUT / 'build.log').open('w') as log:
                child = subprocess.Popen(command, env=env, stdout=log, stderr=subprocess.STDOUT)
                write('running.json', dict(pid=child.pid))
                code = child.wait()
            (OUT / 'exit.txt').write_text(str(code) + '\n')
            assert code == 0, 'Preserve failed log'
            candidate = OUT / 'candidate'
            candidate.mkdir()
            shutil.copy2(SRC / '.libs/libvlccore.dll', candidate / 'libvlccore.dll')
            shutil.copy2(SRC / 'player/.deps/libvlccore_la-input.Plo', candidate / 'input.Plo')
            result.update(status='built-not-qualified', candidateSha256=sha(candidate / 'libvlccore.dll'),
                          candidateSize=(candidate / 'libvlccore.dll').stat().st_size)
        finally:
            for name in OWNED:
                if name in tree and name not in moved:
                    continue  # Preserve originals if backup preparation failed early.
                path = SRC / name
                if path.exists() or path.is_symlink():
                    path.unlink()
                if name in moved:
                    os.rename(BACKUP / name, path)
            after = {str(p.relative_to(SRC)): snapshot(p) for p in SRC.rglob('*')
                     if p.is_file() or p.is_symlink()}
            changed = [n for n in tree.keys() | after.keys() if tree.get(n) != after.get(n)]
            result.update(sharedTreeRestored=not changed, changedSharedFiles=changed,
                          sourcesUnchanged=all(sha(p) == h for p, h in sources.items()),
                          fallbackUnchanged=all(sha(p) == h for p, h in fallback.items()),
                          cFreeAfter=shutil.disk_usage(ROOT).free)
            write('verification.json', result)
            print(json.dumps(result), flush=True)
        assert result['status'] == 'built-not-qualified' and result['sharedTreeRestored']
        assert result['sourcesUnchanged'] and result['fallbackUnchanged']


if __name__ == '__main__':
    main()
