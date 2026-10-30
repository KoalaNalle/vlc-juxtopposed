"""Direct source-compiled TTML parser/control tests, without changing shared objects."""
import fcntl
import hashlib
import json
from pathlib import Path
import shlex
import shutil
import subprocess

ROOT = Path('/mnt/c/Modding/vlc-juxtopposed')
TREE = Path('/root/vlc-juxtopposed-build/win64')
OUT = ROOT/'build-windows/beta-ttml-fps-20261029/unit-build-r3'


def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def main():
    assert not OUT.exists()
    assert shutil.disk_usage(ROOT).free >= 399864390452 + 128 * 1024**2
    OUT.mkdir(parents=True)
    with Path('/root/vlc-juxtopposed-build/.vlc-build.lock').open('a') as lock:
        fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
        compiler = Path(shutil.which('x86_64-w64-mingw32-gcc')).resolve()
        sources = [ROOT/'test/modules/demux/ttml_fps.c',ROOT/'modules/codec/ttml/ttml.c',ROOT/'modules/codec/ttml/genttml.c']
        imports = [TREE/'src/.libs/libvlccore.dll.a',TREE/'lib/.libs/libvlc.dll.a',TREE/'compat/.libs/libcompat.a']
        flags = ['-DMODULE_STRING="ttml_fps_test"','-DMODULE_NAME=ttml_fps_test','-DHAVE_CONFIG_H','-DTOP_BUILDDIR="."','-I'+str(TREE),'-I'+str(TREE/'include'),
                 '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT),
                 '-D__USE_MINGW_ANSI_STDIO=1','-O1','-g','-Wall','-Wextra']
        inputs = {str(p):sha(p) for p in [Path(__file__),compiler,*imports]}
        commands = []
        def run(label,command):
            commands.append(command)
            with (OUT/(label+'.log')).open('w') as log:
                code = subprocess.run(command,cwd=TREE,stdout=log,stderr=subprocess.STDOUT,timeout=90).returncode
            assert code == 0,label
        for i,source in enumerate(sources):
            run('dependencies-'+str(i),[str(compiler),*flags,'-M','-MT','ttml-unit','-MF',str(OUT/f'deps-{i}.d'),str(source)])
            data = (OUT/f'deps-{i}.d').read_text().replace('\\\n',' ')
            for p in shlex.split(data.split(':',1)[1]):
                path = Path(p).resolve()
                inputs[str(path)] = sha(path)
        (OUT/'inputs.json').write_text(json.dumps(inputs,indent=2)+'\n')
        for i,source in enumerate(sources):
            run('compile-'+str(i),[str(compiler),*flags,'-c',str(source),'-o',str(OUT/f'unit-{i}.o')])
        run('link',[str(compiler),'-static','-o',str(OUT/'ttml-fps.exe'),
                    *[str(OUT/f'unit-{i}.o') for i in range(3)],*map(str,imports)])
        assert all(sha(p)==h for p,h in inputs.items())
        record = dict(passed=True,inputsUnchanged=True,dependencyCount=len(inputs),
                      compilerSha256=sha(compiler),binarySha256=sha(OUT/'ttml-fps.exe'),
                      sources={str(p.relative_to(ROOT)):sha(p) for p in sources},commands=commands)
        (OUT/'verification.json').write_text(json.dumps(record,indent=2)+'\n')
        print(json.dumps({k:v for k,v in record.items() if k!='commands'}))


if __name__ == '__main__':
    main()
