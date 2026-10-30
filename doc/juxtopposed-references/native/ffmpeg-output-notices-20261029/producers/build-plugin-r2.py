"""One locked ordinary merged FFmpeg target; preserve and restore every owned shared artifact."""
import fcntl
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import signal
import subprocess
import time

ROOT = Path('/mnt/c/Modding/vlc-juxtopposed')
TREE = Path('/root/vlc-juxtopposed-build/win64')
M = TREE / 'modules'
OUT = ROOT / 'build-windows/beta-record-notice-20261029/plugin-build-r2'
SOURCES = ['codec/avcodec/'+n+'.c' for n in ('video','subtitle','audio','va','avcodec','encoder')]
SOURCES += ['packetizer/av1_obu.c','demux/avformat/demux.c','access/avio.c','packetizer/avparser.c','demux/avformat/mux.c']
COMMON = ['codec/avcodec/fourcc.c','codec/avcodec/chroma.c']

def artifact(name, extension):
    p = Path(name)
    group = 'libavcodec_common_la-' if name in COMMON else 'libavcodec_plugin_la-'
    stem = group+p.stem
    if extension == 'lo': return p.parent/(stem+'.lo')
    if extension == 'o': return p.parent/'.libs'/(stem+'.o')
    if extension == 'Plo': return p.parent/'.deps'/(stem+'.Plo')
    raise ValueError(extension)



def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def deps(p):
    text = Path(p).read_text().replace('\\\n', ' ')
    return [(Path(n) if Path(n).is_absolute() else M / n).resolve()
            for n in shlex.split(text.split(':', 1)[1].split('\n', 1)[0])]


def main():
    assert not OUT.exists()
    assert shutil.disk_usage(ROOT).free >= 399864390452 + 1024 * 1024**2
    OUT.mkdir(parents=True)
    lock = Path('/root/vlc-juxtopposed-build/.vlc-build.lock').open('a')
    fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
    compiler = Path(shutil.which('x86_64-w64-mingw32-gcc')).resolve()
    record = dict(status='running',scope='Ordinary merged FFmpeg plugin build only, not native callback/GUI/output acceptance',
                  started=time.time(),writerLockAcquired=True,forcedCleanupUsed=False,
                  commands=[],restoreErrors=[],errors=[])
    child = None
    backups = []
    changed = False
    failure = None

    def save():
        (OUT / 'verification.json').write_text(json.dumps(record, indent=2) + '\n')

    def active_group():
        if child is None:
            return []
        active = []
        for p in Path('/proc').iterdir():
            if p.name.isdigit():
                try:
                    fields = (p / 'stat').read_text().rsplit(')',1)[1].split()
                    if fields[0] != 'Z' and int(fields[2]) == child.pid:
                        active.append(int(p.name))
                except (FileNotFoundError,ProcessLookupError,PermissionError):
                    pass
        return active

    def interrupted(signum, frame):
        raise InterruptedError(str(signum))

    for sig in [signal.SIGINT,signal.SIGTERM,signal.SIGHUP]:
        signal.signal(sig,interrupted)
    save()
    try:
        flags = ['-DHAVE_CONFIG_H','-DVLC_DYNAMIC_PLUGIN','-DMERGE_FFMPEG','-DMODULE_STRING="avcodec"',
                 '-D__MSVCRT_VERSION__=0x700','-D_WIN32_WINNT=0x0601','-DWINVER=0x0601',
                 '-D__USE_MINGW_ANSI_STDIO=1', '-I'+str(M),'-I'+str(ROOT/'modules'),
                 '-I'+str(TREE),'-I'+str(ROOT/'modules/access'),'-I'+str(ROOT/'modules/codec'),
                 '-I'+str(ROOT/'include'),'-I'+str(TREE/'include'),
                 '-I/root/vlc-juxtopposed-build/contrib/x86_64-w64-mingw32/include',
                 '-I'+str(ROOT/'compat/stdbit'),'-I'+str(ROOT/'compat/stdckdint')]
        unit_receipt = ROOT/'build-windows/beta-record-notice-20261029/unit-run-r1/verification.json'
        unit = json.loads(unit_receipt.read_text())
        assert unit['status']=='passed' and unit['inputsUnchanged']
        assert unit['variants']['candidate']['summary']==dict(cases=51,checks=6062,failures=0)
        inputs = {str(compiler):sha(compiler),str(Path(__file__)):sha(__file__),str(unit_receipt):sha(unit_receipt)}
        for i, name in enumerate(SOURCES+COMMON):
            command = [str(compiler),*flags,'-M','-MP','-MT',str(artifact(name,'lo')),
                       '-MF',str(OUT/f'deps-{i}.d'),str(ROOT/'modules'/name)]
            record['commands'].append(command)
            with (OUT/f'deps-{i}.log').open('w') as log:
                assert subprocess.run(command,cwd=M,stdout=log,stderr=subprocess.STDOUT,timeout=45).returncode == 0
            for p in deps(OUT/f'deps-{i}.d'):
                inputs[str(p)] = sha(p)
        for directory in [Path('/root/vlc-juxtopposed-build/contrib/x86_64-w64-mingw32/lib'), TREE/'_win32/lib']:
            for p in directory.glob('*.a'):
                inputs[str(p.resolve())] = sha(p)
        untouched = {str(p):sha(p) for p in [M/'Makefile',TREE/'src/.libs/libvlccore.dll',
            TREE/'src/.libs/libvlccore.dll.a',TREE/'src/libvlccore.la',TREE/'compat/.libs/libcompat.a',
            TREE/'compat/libcompat.la',M/'module.rc.lo',M/'.libs/module.rc.o',
            M/'gui/qt/.libs/libqt_plugin.dll',M/'.libs/libwebvtt_plugin.dll',M/'.libs/libttml_plugin.dll',
            TREE/'libtool',TREE/'config.status',TREE/'config.h',
            TREE/'_win32/lib/libvlccore.la']}
        fallback = ROOT/'build-windows/beta-resume-20261029/gui-r28/runtime'
        untouched.update({str(p):sha(p) for p in fallback.rglob('*')
                          if p.is_file() and p.name != 'plugins.dat' and 'portable' not in p.relative_to(fallback).parts})
        names = ['libavcodec_plugin.la','.libs/libavcodec_plugin.dll','.libs/libavcodec_plugin.lai',
                 '.libs/libavcodec_plugin.dll.a','.libs/libavcodec_plugin.dll.def','.libs/libavcodec_plugin.exp',
                 'libavcodec_common.la','.libs/libavcodec_common.a']
        for name in SOURCES+COMMON:
            names += [str(artifact(name,ext)) for ext in ('lo','o','Plo')]
        owned = []
        for name in names:
            p = M/name
            st = p.stat()
            assert p.is_file() and not p.is_symlink() and st.st_nlink == 1
            owned.append(dict(path=str(p),sha256=sha(p),bytes=st.st_size,mtimeNs=st.st_mtime_ns,atimeNs=st.st_atime_ns))
        for rel in ['modules/demux/Makefile.am','test/modules/demux/avformat_mux_queue.c']:
            inputs[str(ROOT/rel)]=sha(ROOT/rel)
        record['sourceFiles'] = {str(ROOT/'modules'/n):sha(ROOT/'modules'/n) for n in SOURCES+COMMON}
        config = dict(inputs=inputs,untouched=untouched,owned=owned,compilerSha256=sha(compiler))
        (OUT/'inputs.json').write_text(json.dumps(config,indent=2)+'\n')
        config_hash = sha(OUT/'inputs.json')
        record.update(dependencyCount=len(inputs)-2,originalPluginSha256=sha(M/'.libs/libavcodec_plugin.dll'))
        original = OUT/'original'
        original.mkdir()
        for i, item in enumerate(owned):
            p = Path(item['path'])
            assert sha(p) == item['sha256']
            backup = original/(str(i)+'-'+p.name)
            shutil.copy2(p,backup)
            assert sha(backup) == item['sha256']
            backups.append((p,backup,item))
        assert len(backups) == len(owned)
        changed = True
        for name in SOURCES+COMMON:
            (M/artifact(name,'lo')).unlink()
        command = ['make','-C',str(M),'-o','Makefile','-j2','V=1']
        for p in untouched:
            if Path(p).is_relative_to(TREE):
                command += ['-o',os.path.relpath(p,M)]
        command += ['libavcodec_plugin.la']
        record['commands'].append(command)
        env = dict(os.environ)
        env.pop('CC',None)
        env.pop('CXX',None)
        with (OUT/'build.log').open('w') as log:
            child = subprocess.Popen(command,cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
            record['pid'] = child.pid
            save()
            print('OWNED_FFMPEG_BUILD '+str(child.pid),flush=True)
            record['exitCode'] = child.wait(timeout=300)
        assert record['exitCode'] == 0
        actual = set()
        for name in SOURCES+COMMON:
            f = Path(name)
            closure = deps(M/artifact(name,'Plo'))
            assert closure
            actual.update(closure)
        (OUT/'actual-dependencies.json').write_text(json.dumps({str(p):sha(p) for p in sorted(actual)},indent=2)+'\n')
        record['unqualifiedDependencies'] = sorted(str(p) for p in actual if str(p) not in inputs)
        assert not record['unqualifiedDependencies']
        record['actualDependencyCount'] = len(actual)
        assert all(sha(p)==inputs[str(p)] for p in actual)
        built = M/'.libs/libavcodec_plugin.dll'
        shutil.copyfile(built,OUT/built.name)
        record['pluginSha256'] = sha(OUT/built.name)
        assert record['pluginSha256'] == sha(built) != record['originalPluginSha256']
        record['inputsUnchanged'] = all(sha(p)==h for p,h in inputs.items())
        record['untouchedUnchanged'] = all(sha(p)==h for p,h in untouched.items())
        record['configUnchanged'] = sha(OUT/'inputs.json') == config_hash
        assert record['inputsUnchanged'] and record['untouchedUnchanged'] and record['configUnchanged']
    except BaseException as error:
        failure = error
        record['errors'].append(repr(error))
    finally:
        for sig in [signal.SIGINT,signal.SIGTERM,signal.SIGHUP]:
            signal.signal(sig,signal.SIG_IGN)
        if child is not None and (child.poll() is None or active_group()):
            record['forcedCleanupUsed'] = True
            try:
                os.killpg(child.pid,signal.SIGTERM)
            except ProcessLookupError:
                pass
            until = time.monotonic()+3
            while active_group() and time.monotonic()<until:
                time.sleep(.05)
            if active_group():
                os.killpg(child.pid,signal.SIGKILL)
            child.wait(timeout=10)
        record['remainingOwnedPids'] = active_group()
        record['writerCleanupConfirmed'] = (child is None or child.poll() is not None) and not record['remainingOwnedPids']
        if changed:
            if not record['writerCleanupConfirmed']:
                record['restoreErrors'].append('Writer remains; cannot restore loaded artifacts')
            else:
                for p,backup,item in backups:
                    try:
                        assert p.is_relative_to(M) and sha(backup)==item['sha256']
                        assert not p.is_symlink() and (not p.exists() or p.stat().st_nlink==1)
                        shutil.copyfile(backup,p)
                        os.utime(p,ns=(item['atimeNs'],item['mtimeNs']))
                        assert sha(p)==item['sha256']
                    except BaseException as error:
                        record['restoreErrors'].append(str(p)+': '+repr(error))
                record['ownedArtifactsRestored'] = all(sha(p)==item['sha256'] for p,backup,item in backups)
        record['inputsUnchanged'] = 'inputs' in locals() and all(sha(p)==h for p,h in inputs.items())
        record['untouchedUnchanged'] = 'untouched' in locals() and all(sha(p)==h for p,h in untouched.items())
        fcntl.flock(lock,fcntl.LOCK_UN)
        lock.close()
        record['writerLockReleased'] = True
        record.update(status='terminal',ended=time.time(),success=failure is None and not record['restoreErrors']
                      and not record['forcedCleanupUsed'] and record['writerCleanupConfirmed']
                      and record.get('ownedArtifactsRestored',False) and record['inputsUnchanged'] and record['untouchedUnchanged'])
        save()
    print(json.dumps({k:v for k,v in record.items() if k!='commands'},indent=2))
    assert record['success']


if __name__ == '__main__':
    main()
