"""One-use native worker target only; requires root's released writer lane."""
from pathlib import Path
import argparse,fcntl,json,os,re,shlex,shutil,signal,subprocess,time
from worker_support import digest,save,storage,seconds_left,dependencies,check_map

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--execute-reviewed-build',action='store_true',required=True)
parser.parse_args()
prep=Path(__file__).resolve().parent
repo=Path('/mnt/c/Modding/vlc-juxtopposed')
base=Path('/root/vlc-juxtopposed-build')
qt=base/'win64/modules/gui/qt'
out=prep/'build'; out.mkdir(exist_ok=False)
record=dict(status='preflight',started=time.time(),scope='Actual production history-model worker test only; no GUI/Core rebuild',
    deadlineLocal='2026-10-28T23:35:00+02:00',writerLockAcquired=False,writerLockReleased=False,
    forcedBuildCleanupUsed=False,writerCleanupConfirmed=False,events=[],errors=[])
def write(): save(out/'verification.json',record)
write()
env=os.environ.copy()
env['PATH']=f'{base}/extras/tools/build/bin:{base}/contrib/bin:'+env['PATH']
env.pop('CC',None); env.pop('CXX',None)
(out/'tmp').mkdir(); env['TMPDIR']=str(out/'tmp')
lock=None

def owned(command,label):
    assert seconds_left()>180, '23:35 stop bound leaves no cleanup/run reserve'
    record['phase']=label; write()
    with (out/'build.log').open('ab') as log:
        log.write((label+' '+shlex.join(command)+'\n').encode()); log.flush()
        child=subprocess.Popen(command,cwd=qt,env=env,stdout=log,stderr=log,start_new_session=True)
        record['pid']=child.pid; write(); start=time.monotonic()
        try:
            while child.poll() is None:
                assert seconds_left()>180 and time.monotonic()-start<900, 'Owned build/deadline timeout'
                assert storage(repo,0)['fits'], 'C: reached owner 80% floor'
                try: child.wait(timeout=3)
                except subprocess.TimeoutExpired: pass
        finally:
            if child.poll() is None:
                record['forcedBuildCleanupUsed']=True
                os.killpg(child.pid,signal.SIGTERM)
                try: child.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    os.killpg(child.pid,signal.SIGKILL); child.wait(timeout=10)
        record['events'].append(dict(phase=label,exitCode=child.returncode,ended=time.time()))
        processes=subprocess.check_output(['ps','-eo','pid=,pgid=,stat='],text=True)
        remaining=[int(fields[0]) for line in processes.splitlines()
            if len(fields:=line.split())==3 and int(fields[1])==child.pid and not fields[2].startswith('Z')]
        record['remainingOwnedWriterPids']=remaining
        write(); assert not remaining, 'Owned process group still has live writers'
        assert child.returncode==0, f'{label}: exit {child.returncode}'

try:
    assert seconds_left()>1200, 'Need twenty minutes before the 23:35 stop bound'
    record['storageBefore']=storage(repo); assert record['storageBefore']['fits'], '384 MiB peak will cross 80%'
    identities=json.loads((prep/'producer-identities.json').read_text())
    assert not check_map(prep,identities), 'Frozen producer changed'
    initial=json.loads((prep/'initial-inputs.json').read_text())
    assert not check_map(repo,initial), 'Selected production/test source changed since preparation'
    linux=json.loads((prep/'linux-inputs.json').read_text())
    assert not check_map('/',linux), 'Configured toolchain/build inputs changed'
    lock=(base/'.vlc-build.lock').open('w')
    fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
    record['writerLockAcquired']=True; write()
    objects=out/'objects'; objects.mkdir()
    command=['make','-f','Makefile','-f',str(repo/'modules/gui/qt/tests/native_library/history-worker.mk'),
        '-j2','HISTORY_WORKER_OUT='+str(objects)]
    owned(command+['history-worker-config','history-worker-generated'],'generated')
    owned(command+['history-worker-dependencies'],'compiler-dependency-probes')
    probes=sorted((objects/'predeps').rglob('*.d')); assert len(probes)==35, len(probes)
    graph=dependencies(probes,qt)
    # Freeze exact compiler-selected headers/sources plus actual link archives.
    linking=shlex.split((objects/'config-link.txt').read_text())
    search=[]; queue=[]; cursor=0
    compiler=shlex.split((objects/'config-cxx.txt').read_text())[0]
    while cursor<len(linking):
        word=linking[cursor]
        if word=='-L': cursor+=1; search.append(Path(linking[cursor]))
        elif word.startswith('-L'): search.append(Path(word[2:]))
        elif word.endswith(('.a','.la')): queue.append((qt/word).resolve())
        elif word.startswith('-l'):
            name='lib'+word[2:]+'.a'
            candidates=[directory/name for directory in search]
            found=next((path for path in candidates if path.is_file()),None)
            if found is None:
                found=Path(subprocess.check_output([compiler,'-print-file-name='+name],env=env,text=True).strip())
            assert found.is_file(), 'Unresolved real link input: '+word
            queue.append(found.resolve())
        cursor+=1
    for name in ['libstdc++.a','libgcc.a','crtbegin.o','crtend.o']:
        found=Path(subprocess.check_output([compiler,'-print-file-name='+name],env=env,text=True).strip())
        assert found.is_file(), 'Missing implicit compiler link input: '+name
        queue.append(found.resolve())
    while queue:
        path=queue.pop()
        if str(path) in graph: continue
        assert path.is_file(), str(path)
        graph.add(str(path))
        if path.suffix=='.la':
            text=path.read_text()
            names=re.search(r"^old_library='([^']*)'",text,re.M)
            dll=re.search(r"^library_names='([^']*)'",text,re.M)
            for name in ((names[1].split() if names else [])+(dll[1].split() if dll else [])):
                choices=[path.parent/'.libs'/name,path.parent/name]
                found=next((p for p in choices if p.is_file()),None)
                assert found, 'Missing libtool link archive '+name
                queue.append(found.resolve())
    compiled={name:digest(name) for name in sorted(graph)}
    save(out/'compiler-inputs.json',compiled)
    record['compilerDependencyFiles']=len(probes); record['compilerInputCount']=len(compiled)
    record['compilerInputsSha256']=digest(out/'compiler-inputs.json'); write()
    assert not check_map(repo,initial) and not check_map('/',linux)
    owned(command+['history-worker-native'],'compile-and-link')
    actual=sorted(objects.rglob('*.o.d')); assert len(actual)==35, len(actual)
    actual_graph=dependencies(actual,qt)
    assert actual_graph.issubset(graph), 'Actual compiler introduced unguarded dependencies'
    save(out/'actual-compiler-dependencies.json',sorted(actual_graph))
    assert not check_map('/',compiled), 'Compiler-selected inputs changed during build'
    record['sourceUnchanged']=not check_map(repo,initial)
    record['toolchainUnchanged']=not check_map('/',linux)
    record['producersUnchanged']=not check_map(prep,identities)
    assert record['sourceUnchanged'] and record['toolchainUnchanged'] and record['producersUnchanged']
    candidates=[objects/'.libs/test_qt_history_worker.exe',objects/'test_qt_history_worker.exe']
    exe=next(path for path in candidates if path.is_file() and path.read_bytes()[:2]==b'MZ')
    assert exe.stat().st_size<=80*2**20, 'Stripped executable exceeds reviewed budget'
    sections=subprocess.check_output(['x86_64-w64-mingw32-objdump','-h',str(exe)],env=env,text=True)
    (out/'exe-sections.txt').write_text(sections)
    assert not re.search(r'^\s*\d+\s+\.debug',sections,re.M), 'Debug sections remain'
    record.update(status='terminal',success=True,exitCode=0,exe=str(exe),testSha256=digest(exe),
        exeBytes=exe.stat().st_size,actualDependenciesGuarded=True,compilerInputsUnchanged=True,
        strippedDebugVerified=True,writerCleanupConfirmed=True)
except BaseException as error:
    record.update(status='terminal',success=False,exitCode=1)
    record['errors'].append(repr(error))
finally:
    if lock is not None:
        fcntl.flock(lock,fcntl.LOCK_UN); lock.close(); record['writerLockReleased']=True
    record['storageAfter']=storage(repo,0)
    if not record['storageAfter']['fits']:
        record.update(success=False,exitCode=1); record['errors'].append('C: below owner 20% free floor at finish')
    record['ended']=time.time(); write()
print(json.dumps({key:value for key,value in record.items() if key!='events'},indent=2))
raise SystemExit(record['exitCode'])
