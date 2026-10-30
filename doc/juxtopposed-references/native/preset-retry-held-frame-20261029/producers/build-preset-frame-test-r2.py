"""One-use cross compilation of the maintained real-player notification test."""
import fcntl
import hashlib
import json
from pathlib import Path
import shlex
import shutil
import subprocess

ROOT = Path('/mnt/c/Modding/vlc-juxtopposed')
TREE = Path('/root/vlc-juxtopposed-build/win64')
OUT = ROOT / 'build-windows/beta-preset-frame-20261029/test-build-r2'


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    assert not OUT.exists()
    assert shutil.disk_usage(ROOT).free >= 399864390452 + 256 * 1024**2
    OUT.mkdir(parents=True)
    with Path('/root/vlc-juxtopposed-build/.vlc-build.lock').open('a') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        compiler = Path(shutil.which('x86_64-w64-mingw32-gcc')).resolve()
        source = ROOT / 'test/src/player/subtitle_fps.c'
        imports = [TREE / 'src/.libs/libvlccore.dll.a', TREE / 'lib/.libs/libvlc.dll.a']
        flags = ['-std=gnu11', '-DHAVE_CONFIG_H', '-DTOP_BUILDDIR="."',
                 '-I' + str(TREE), '-I' + str(TREE / 'include'),
                 '-I' + str(ROOT / 'include'), '-I' + str(ROOT / 'src'),
                 '-I' + str(ROOT), '-O1', '-g', '-Wall', '-Wextra']

        def run(name, command):
            (OUT / (name + '.command.json')).write_text(json.dumps(command, indent=2) + '\n')
            with (OUT / (name + '.log')).open('w') as log:
                result = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, timeout=90)
            (OUT / (name + '.exit.txt')).write_text(str(result.returncode) + '\n')
            assert result.returncode == 0, name + ' failed; preserve and inspect log'

        run('dependencies', [str(compiler), *flags, '-M', '-MT', 'subtitle-fps-test',
                             '-MF', str(OUT / 'dependencies.d'), str(source)])
        dep_text = (OUT / 'dependencies.d').read_text().replace('\\\n', ' ')
        deps = [Path(p) for p in shlex.split(dep_text.split(':', 1)[1])]
        inputs = {str(p): sha(p) for p in [*deps, *imports, compiler, Path(__file__),
                                         ROOT / 'test/Makefile.am']}
        (OUT / 'inputs.json').write_text(json.dumps(inputs, indent=2) + '\n')
        run('compile', [str(compiler), *flags, '-c', str(source), '-o', str(OUT / 'subtitle-fps.o'),
                        '-MMD', '-MF', str(OUT / 'actual.d')])
        run('link', [str(compiler), '-static', '-o', str(OUT / 'subtitle-fps.exe'),
                     str(OUT / 'subtitle-fps.o'), *map(str, imports)])
        assert all(sha(p) == value for p, value in inputs.items())
        result = dict(passed=True, dependencyCount=len(deps), inputsUnchanged=True,
                      compilerSha256=sha(compiler), binarySha256=sha(OUT / 'subtitle-fps.exe'),
                      sourceSha256=sha(source), scope='Actual Core startup single/batched subtitle timing and snapshots; no Qt or final-package claim')
        (OUT / 'verification.json').write_text(json.dumps(result, indent=2) + '\n')
        print(json.dumps(result), flush=True)


if __name__ == '__main__':
    main()
