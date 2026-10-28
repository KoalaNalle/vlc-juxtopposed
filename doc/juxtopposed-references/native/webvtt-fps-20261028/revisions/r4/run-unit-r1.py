"""Run actual demux units against preserved Core; baseline failures stay visible."""
from pathlib import Path
import ctypes,hashlib,json,os,shutil,subprocess,time
PREP=Path(__file__).resolve().parent;REPO=Path(r'C:\Modding\vlc-juxtopposed')
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
cfg=json.loads((PREP/'unit-config-r1.json').read_text());cfgsha=sha(PREP/'unit-config-r1.json');build=PREP/'unit-build-r1/verification.json';b= json.loads(build.read_text());buildsha=sha(build)
assert b['status']=='terminal' and b['success'] and all(b[k] for k in ('sourceUnchanged','dependenciesUnchanged','producersUnchanged','configUnchanged'))
out=PREP/'unit-run-r1';assert not out.exists();out.mkdir();errors=[];failure=None;child=None
record={'status':'running','started':time.time(),'scope':cfg['scope'],'variants':[],'errors':errors,'forcedCleanupUsed':False}
def save():(out/'verification.json').write_text(json.dumps(record,indent=2)+'\n')
kernel=ctypes.WinDLL('kernel32',use_last_error=True);kernel.SetErrorMode.argtypes=[ctypes.c_uint];kernel.SetErrorMode.restype=ctypes.c_uint;previous=kernel.SetErrorMode(0x8003)
save()
try:
    assert time.time()+120<cfg['stopByUnix'];space=shutil.disk_usage(REPO);assert space.free-(space.total+4)//5>=cfg['storageBudgetBytes']
    assert all(sha(REPO/p)==h for p,h in cfg['sourceSha256'].items());assert all(sha(PREP/p)==h for p,h in cfg['producerSha256'].items())
    assert sha(REPO/cfg['testCore'])==cfg['testCoreSha256']
    for variant in ('baseline','candidate'):
        dest=out/variant;dest.mkdir();exe=PREP/'unit-build-r1'/variant/'test_webvtt_fps.exe';assert sha(exe)==b['executables'][variant]
        shutil.copyfile(exe,dest/exe.name);os.link(REPO/cfg['testCore'],dest/'libvlccore.dll')
        item={'variant':variant};record['variants'].append(item);save()
        with (dest/'stdout.txt').open('wb') as stdout,(dest/'stderr.txt').open('wb') as stderr:
            child=subprocess.Popen([str(dest/exe.name),str(REPO/'test/modules/demux/fixtures/webvtt-fps')],cwd=dest,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
            item['pid']=child.pid;save()
            try:item['exitCode']=child.wait(timeout=30)
            finally:
                if child.poll() is None:record['forcedCleanupUsed']=True;child.kill();child.wait(timeout=5)
        rows=[json.loads(x) for x in (dest/'stdout.txt').read_text().splitlines()];item['cases']=[x for x in rows if 'case' in x];item['summary']=rows[-1]
        assert len(item['cases'])==8 and item['summary']['cases']==8 and item['summary']['checks']>0
        if variant=='candidate':assert item['exitCode']==0 and item['summary']['failures']==0 and not (dest/'stderr.txt').read_bytes()
        else:
            assert item['exitCode']==1 and item['summary']['failures']>0
            named={x['case']:x['passed'] for x in item['cases']}
            assert named['actual-stream-mode-rejects-rate-and-preserves-inline-times']
            assert named['actual-demux-invalid-rates-atomic']
            assert not named['actual-demux-containing-cue-refill'] and not named['actual-demux-rate-inline-overlap-and-immutable-source']
        item['runtimeExeUnchanged']=sha(dest/exe.name)==b['executables'][variant];item['runtimeCoreUnchanged']=sha(dest/'libvlccore.dll')==cfg['testCoreSha256'];save()
except BaseException as e:failure=e;record['failure']=repr(e)
finally:
    if child is not None and child.poll() is None:record['forcedCleanupUsed']=True;child.kill();child.wait(timeout=5)
    kernel.SetErrorMode(previous)
    for key,mapping,root in [('sourceUnchanged',cfg['sourceSha256'],REPO),('producersUnchanged',cfg['producerSha256'],PREP)]:
        try:record[key]=all(sha(root/p)==h for p,h in mapping.items())
        except BaseException as e:record[key]=False;errors.append(repr(e))
    record['configUnchanged']=sha(PREP/'unit-config-r1.json')==cfgsha;record['buildUnchanged']=sha(build)==buildsha;record['preservedCoreUnchanged']=sha(REPO/cfg['testCore'])==cfg['testCoreSha256']
    record.update(status='terminal',ended=time.time(),success=failure is None and not errors and not record['forcedCleanupUsed'] and all(v for k,v in record.items() if k.endswith('Unchanged')) and all(v.get('runtimeExeUnchanged') and v.get('runtimeCoreUnchanged') for v in record['variants']));save()
print(json.dumps({k:record.get(k) for k in ('status','success','failure','variants','forcedCleanupUsed')},indent=2));assert record['success']
