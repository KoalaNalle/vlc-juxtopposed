"""Compile only fresh ignored standalone units, never make or mutate the shared tree."""
from pathlib import Path
import hashlib,json,os,shutil,signal,subprocess,time
PREP=Path(__file__).resolve().parent;REPO=Path('/mnt/c/Modding/vlc-juxtopposed');BASE=Path('/root/vlc-juxtopposed-build')
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
cfg=json.loads((PREP/'unit-config-r1.json').read_text());config_hash=sha(PREP/'unit-config-r1.json')
out=PREP/'unit-build-r1';assert not out.exists();out.mkdir();record={'status':'running','started':time.time(),'commands':[],'errors':[],'forcedCleanupUsed':False};child=None;failure=None
def save():(out/'verification.json').write_text(json.dumps(record,indent=2)+'\n')
def interrupted(signum,frame):raise InterruptedError(str(signum))
for sig in (signal.SIGINT,signal.SIGTERM,signal.SIGHUP):signal.signal(sig,interrupted)
def run(command,name):
    global child
    assert time.time()+120<cfg['stopByUnix']
    with (out/(name+'.log')).open('wb') as log:
        child=subprocess.Popen(command,stdout=log,stderr=subprocess.STDOUT,start_new_session=True,cwd=out,env=dict(os.environ,TMPDIR=str(out/'tmp')))
        item={'name':name,'command':command,'pid':child.pid};record['commands'].append(item);save()
        try:item['exitCode']=child.wait(timeout=90)
        finally:
            if child.poll() is None:record['forcedCleanupUsed']=True;os.killpg(child.pid,signal.SIGKILL);child.wait(timeout=10)
        assert item['exitCode']==0,(name,item['exitCode'])
save()
try:
    assert time.time()+300<cfg['stopByUnix'];space=shutil.disk_usage(REPO);reserve=(space.total+4)//5
    record['storage']={'freeBytes':space.free,'minimumFreeBytes':reserve,'budgetBytes':cfg['storageBudgetBytes']};assert space.free-reserve>=cfg['storageBudgetBytes']
    assert all(sha(REPO/p)==h for p,h in cfg['sourceSha256'].items())
    assert all(sha(p)==h for p,h in cfg['dependencies'].items())
    assert all(sha(PREP/p)==h for p,h in cfg['producerSha256'].items());assert sha(cfg['compiler'])==cfg['compilerSha256']
    (out/'tmp').mkdir()
    flags=['-std=gnu11','-DHAVE_CONFIG_H=1','-DMODULE_NAME=webvtt_fps_test','-D__MSVCRT_VERSION__=0x700','-D_WIN32_WINNT=0x0601','-DWINVER=0x0601','-D__USE_MINGW_ANSI_STDIO=1',
        '-I'+str(BASE/'win64'),'-I'+str(REPO/'include'),'-I'+str(BASE/'win64/include'),'-I'+str(BASE/'contrib/x86_64-w64-mingw32/include'),'-I'+str(REPO/'modules/demux'),
        '-I'+str(REPO/'compat/stdbit'),'-I'+str(REPO/'compat/stdckdint'),'-O1','-g','-mms-bitfields','-Wall','-Wextra','-Werror=implicit-function-declaration','-Werror=incompatible-pointer-types',
        '-fstack-protector-strong','-fdata-sections','-ffunction-sections','-static-libgcc']
    run([cfg['compiler'],*flags,'-MMD','-MF',str(out/'parser.d'),'-c',str(REPO/'modules/codec/webvtt/webvtt.c'),'-o',str(out/'parser.o')],'parser-compile')
    for variant in ('baseline','candidate'):
        dest=out/variant;dest.mkdir();extra=[]
        if variant=='baseline':extra=['-DWEBVTT_DEMUX_SOURCE="'+str(PREP/'source-preimage/webvtt.c')+'"']
        run([cfg['compiler'],*flags,*extra,'-MMD','-MF',str(dest/'test.d'),'-c',str(REPO/'test/modules/demux/webvtt_fps.c'),'-o',str(dest/'test.o')],variant+'-compile')
        run([cfg['compiler'],str(dest/'test.o'),str(out/'parser.o'),str(BASE/'win64/src/.libs/libvlccore.dll.a'),str(BASE/'win64/compat/.libs/libcompat.a'),
            '/usr/lib/gcc/x86_64-w64-mingw32/13-posix/libssp.a','-static-libgcc','-Wl,--gc-sections,--nxcompat,--dynamicbase,--high-entropy-va','-o',str(dest/'test_webvtt_fps.exe')],variant+'-link')
    record['executables']={v:sha(out/v/'test_webvtt_fps.exe') for v in ('baseline','candidate')}
except BaseException as e:failure=e;record['failure']=repr(e)
finally:
    if child is not None and child.poll() is None:
        record['forcedCleanupUsed']=True;os.killpg(child.pid,signal.SIGKILL);child.wait(timeout=10)
    for key,mapping,root in [('sourceUnchanged',cfg['sourceSha256'],REPO),('dependenciesUnchanged',cfg['dependencies'],None),('producersUnchanged',cfg['producerSha256'],PREP)]:
        try:record[key]=all(sha((root/p) if root else p)==h for p,h in mapping.items())
        except BaseException as e:record[key]=False;record['errors'].append(repr(e))
    record['configUnchanged']=sha(PREP/'unit-config-r1.json')==config_hash
    record.update(status='terminal',ended=time.time(),success=failure is None and not record['errors'] and not record['forcedCleanupUsed'] and all(v for k,v in record.items() if k.endswith('Unchanged')));save()
print(json.dumps({k:record.get(k) for k in ('status','success','failure','executables','sourceUnchanged','dependenciesUnchanged','producersUnchanged','configUnchanged','forcedCleanupUsed')},indent=2));assert record['success']
