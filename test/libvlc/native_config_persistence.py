"""Opt-in Windows real-Core peers: shared config persistence, failures and exit.
No desktop, audio output or final package acceptance.
"""
from pathlib import Path
import argparse,ctypes,hashlib,json,os,queue,re,shutil,subprocess,threading,time
from native_http_runtime import stage_runtime,environment,regenerate_cache

def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()

parser=argparse.ArgumentParser(description=__doc__)
for name in ('package','core','probe','output-dir'):parser.add_argument('--'+name,type=Path,required=True)
parser.add_argument('--overlap-iterations',type=int,default=15)
args=parser.parse_args()
assert 1<=args.overlap_iterations<=10000
root=Path(__file__).resolve().parents[2];build=root/'build-windows'
package,core,probe,out=(p.resolve() for p in (args.package,args.core,args.probe,args.output_dir))
assert os.name=='nt' and all(p.is_relative_to(build) for p in (package,core,probe,out))
out.mkdir(exist_ok=False);runtime=out/'runtime';stage_runtime(package,runtime)
target=runtime/'libvlccore.dll';target.unlink();shutil.copyfile(core,target)
shutil.copyfile(probe,runtime/'persistence-probe.exe');regenerate_cache(runtime,out/'cache')
report=dict(passed=False,scope=__doc__,fullBetaAccepted=False,coreSha256=digest(core),probeSha256=digest(probe),
    packageCoreUnchanged=False,cases=[],processes=[],checks=[])
baseline=digest(package/'libvlccore.dll')
peers=[]
def save():(out/'verification.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8',newline='\n')
def check(value,name):
    report['checks'].append(dict(name=name,passed=bool(value)));save();assert value,name
    print('PASS',name,flush=True)
def fixture(name,data=None):
    folder=out/name;folder.mkdir()
    path=folder/'vlcrc'
    if data is not None:path.write_bytes(data)
    return path
seed=b'qt-notification-duration=777\nqt-remember-window-geometry=1\n'
def options(path):
    result={}
    for line in path.read_text(encoding='utf-8-sig').splitlines():
        if line.startswith('#'):continue
        if '=' in line:
            key,value=line.split('=',1);result[key]=value
    return result
def effective(path,key,default):return options(path).get(key,str(default))

class Peer:
    def __init__(self,path,*extra):
        self.index=len(peers);self.closed=False;self.path=path;self.events=[]
        folder=out/('peer-%02d'%self.index);folder.mkdir()
        self.log=(folder/'stderr.txt').open('wb')
        self.env=environment(runtime,folder);self.env['VLC_USERDATA_PATH']=str(folder/'userdata')
        self.p=subprocess.Popen([str(runtime/'persistence-probe.exe'),str(path),*extra],cwd=runtime,env=self.env,
            stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=self.log,creationflags=subprocess.CREATE_NO_WINDOW)
        self.lines=queue.Queue()
        def reader():
            for line in self.p.stdout:self.lines.put(line.decode('utf-8').rstrip('\r\n'))
            self.lines.put(None)
        threading.Thread(target=reader,daemon=True).start()
        peers.append(self)
        self.record=dict(pid=self.p.pid,folder=folder.name,config=str(path.relative_to(out)),normalExit=False,forcedCleanupUsed=False,events=self.events)
        report['processes'].append(self.record);save()
        assert self.lines.get(timeout=20)=='READY'
    def command(self,text):
        self.p.stdin.write((text+'\n').encode('utf-8'));self.p.stdin.flush()
        response=self.lines.get(timeout=20);self.events.append(dict(command=text,response=response));return response
    def ok(self,text):assert self.command(text)=='RESULT 0',self.events[-1]
    def fail(self,text):assert self.command(text)=='RESULT -1',self.events[-1]
    def integer(self,key):return int(self.command('geti '+key).split()[1])
    def close(self):
        if self.closed:return
        response=self.command('quit');assert response=='CLOSED',response
        code=self.p.wait(timeout=20);self.log.close();self.closed=True
        logs=Path(self.log.name).read_text(encoding='utf-8',errors='replace')
        errors=[line.split(' error: ',1)[1] for line in logs.splitlines() if ' error: ' in line]
        case=self.path.parent.name
        failure=any(e['response']=='RESULT -1' for e in self.events)
        expected=[]
        if failure and case in ('read-denial','pending-rollback','failed-only-exit','batch-rollback'):
            expected=['cannot read configuration file: Permission denied']
        elif failure and case=='replacement-denial':
            expected=['cannot replace configuration file (Windows error 5)']
        elif failure and case=='read-only':expected=['configuration file is read-only']
        self.record.update(exitCode=code,normalExit=code==0,nativeErrors=errors,expectedNativeErrors=expected,
                           unexpectedNativeErrors=[] if errors==expected else errors)
        save();assert code==0 and errors==expected,(case,errors,expected)

kernel=ctypes.WinDLL('kernel32',use_last_error=True)
kernel.CreateFileW.argtypes=[ctypes.c_wchar_p,ctypes.c_uint32,ctypes.c_uint32,ctypes.c_void_p,ctypes.c_uint32,ctypes.c_uint32,ctypes.c_void_p]
kernel.CreateFileW.restype=ctypes.c_void_p
kernel.CloseHandle.argtypes=[ctypes.c_void_p];kernel.CloseHandle.restype=ctypes.c_int
kernel.SetFileAttributesW.argtypes=[ctypes.c_wchar_p,ctypes.c_uint32];kernel.SetFileAttributesW.restype=ctypes.c_int
def lock(path,share):
    handle=kernel.CreateFileW(str(path),0x80000000,share,None,3,0,None)
    assert handle not in (None,ctypes.c_void_p(-1).value),ctypes.get_last_error()
    return handle

try:
    for order in ('ab','ba'):
        path=fixture('different-keys-'+order,seed);a=Peer(path);b=Peer(path)
        a.ok('seti qt-notification-duration 5000');b.ok('seti qt-remember-window-geometry 0')
        check(effective(path,'qt-notification-duration',3000)=='5000' and effective(path,'qt-remember-window-geometry',True)=='0',
              'Different-key edits survive both commits: '+order)
        check(a.integer('qt-remember-window-geometry')==1 and b.integer('qt-notification-duration')==777,
              'External saved defaults do not overwrite active process values: '+order)
        a.ok('save');b.ok('save')
        for p in ([a,b] if order=='ab' else [b,a]):p.close()
        check(effective(path,'qt-notification-duration',3000)=='5000' and effective(path,'qt-remember-window-geometry',True)=='0',
              'Explicit no-op saves and either close order retain both edits: '+order)
    path=fixture('same-key',seed);a=Peer(path);b=Peer(path)
    a.ok('seti qt-notification-duration 5000');b.ok('seti qt-notification-duration 6000');a.ok('save')
    check(effective(path,'qt-notification-duration',3000)=='6000','No-op older process cannot replay its last successful edit')
    a.ok('seti qt-notification-duration 5000')
    check(effective(path,'qt-notification-duration',3000)=='5000','Intentional same-value edit is a new serialized commit')
    b.close();a.close()
    check(effective(path,'qt-notification-duration',3000)=='5000','Same-key last successful commit survives both exits')

    path=fixture('overlap',seed+b'qt-max-volume=125\nqt-controls-hide-timeout=1000\n')
    keys=['qt-notification-duration','qt-max-volume','qt-controls-hide-timeout'];actors=[Peer(path) for _ in keys]
    barrier=threading.Barrier(len(keys));errors=[]
    def writes(actor,key,base):
        try:
            barrier.wait(timeout=10)
            for i in range(args.overlap_iterations):actor.ok('seti %s %d'%(key,base+i%30))
        except BaseException as error:errors.append(repr(error))
    workers=[threading.Thread(target=writes,args=(p,k,v)) for p,k,v in zip(actors,keys,[5000,150,2000])]
    for t in workers:t.start()
    for t in workers:t.join(timeout=50)
    report['overlapErrors']=errors
    check(not errors and all(not t.is_alive() for t in workers),'Overlapping multi-process file transactions terminate successfully')
    for p in reversed(actors):p.close()
    final=[base+(args.overlap_iterations-1)%30 for base in [5000,150,2000]]
    check(all(effective(path,k,-1)==str(v) for k,v in zip(keys,final)),
          'All%d overlapping distinct-key commits retain independent final values and exits'%(3*args.overlap_iterations))
    events=[]
    for actor in actors:
        for line in Path(actor.log.name).read_text(encoding='utf-8',errors='replace').splitlines():
            match=re.search(r'configuration writer (acquired|releasing): pid=(\d+) tick=(\d+) lock=(.*)',line)
            if match:events.append(dict(stage=match[1],pid=int(match[2]),tick=int(match[3]),identity=match[4]))
    active=None;overlaps=[]
    for event in sorted(events,key=lambda e:e['tick']):
        if event['stage']=='acquired':
            if active:overlaps.append((active,event))
            active=event
        elif active and event['pid']==active['pid']:active=None
        else:overlaps.append(('unmatched release',event))
    report['overlapLockEvents']=sorted(events,key=lambda e:e['tick'])
    check(len(events)==6*args.overlap_iterations and len({e['identity'] for e in events})==1 and not overlaps and active is None,
          'Actual lock identity and acquisition/release intervals prove one writer domain without overlapping critical sections')

    for name,share in [('read-denial',0),('replacement-denial',3)]:
        path=fixture(name,seed);a=Peer(path);before=path.read_bytes();handle=lock(path,share)
        try:
            a.fail('seti qt-notification-duration 5000')
            check(a.integer('qt-notification-duration')==777,'Failed '+name+' rolls back actual native value')
        finally:assert kernel.CloseHandle(handle)
        check(path.read_bytes()==before,'Failed '+name+' preserves destination bytes')
        a.ok('seti qt-notification-duration 5000');a.close()
        check(effective(path,'qt-notification-duration',3000)=='5000','Retry after '+name+' and normal exit persist correctly')

    path=fixture('transient-replacement',seed);a=Peer(path);handle=lock(path,3)
    released=threading.Event()
    def release():
        time.sleep(.035);assert kernel.CloseHandle(handle);released.set()
    release_thread=threading.Thread(target=release);release_thread.start()
    a.ok('seti qt-notification-duration 5000');release_thread.join(timeout=2);a.close()
    log=Path(a.log.name).read_text(encoding='utf-8',errors='replace')
    check(released.is_set() and effective(path,'qt-notification-duration',3000)=='5000'
          and 'configuration replacement recovered after' in log,
          'Bounded internal replacement retry succeeds after real temporary sharing denial without error or rollback')

    path=fixture('pending-rollback',seed);a=Peer(path);b=Peer(path)
    assert a.command('stagei qt-notification-duration 2000')=='STAGED'
    handle=lock(path,0)
    try:a.fail('seti qt-notification-duration 3000');assert a.integer('qt-notification-duration')==2000
    finally:assert kernel.CloseHandle(handle)
    b.ok('seti qt-remember-window-geometry 0');b.close();a.close()
    check(effective(path,'qt-notification-duration',3000)=='2000' and effective(path,'qt-remember-window-geometry',True)=='0',
          'Failed checked write restores previous pending edit; autosave merges it with another process commit')

    path=fixture('failed-only-exit',seed);a=Peer(path);b=Peer(path);handle=lock(path,0)
    try:a.fail('seti qt-notification-duration 5000')
    finally:assert kernel.CloseHandle(handle)
    b.ok('seti qt-notification-duration 6000');b.close();before=path.read_bytes();a.close()
    check(path.read_bytes()==before,'Failed-only process exit cannot replay rejected or previous values')

    path=fixture('batch-rollback',seed+b'audio-language=English\nrate=1.000000\n');a=Peer(path)
    before=path.read_bytes();a.fail('badbatch')
    check(a.command('gets audio-language')=='STRING '+b'English'.hex() and a.integer('qt-notification-duration')==777
          and path.read_bytes()==before,'Late invalid batch member cannot publish earlier string or float changes')
    assert a.command('stages audio-language '+b'Pending'.hex())=='STAGED'
    handle=lock(path,0)
    try:
        a.fail('triplet')
        check(a.command('gets audio-language')=='STRING '+b'Pending'.hex() and a.integer('qt-notification-duration')==777
              and float.fromhex(a.command('getf rate').split()[1])==1.,'Failed mixed-type batch restores all values and previous pending string')
    finally:assert kernel.CloseHandle(handle)
    a.ok('triplet');a.close();a=Peer(path)
    check(a.command('gets audio-language')=='STRING '+b'Suomi'.hex() and a.integer('qt-notification-duration')==5000
          and float.fromhex(a.command('getf rate').split()[1])==1.25,'Mixed-type batch retry persists every member and reloads normally')
    a.close()

    path=fixture('format',b'\xef\xbb\xbf# owned header\r\nqt-notification-duration=111\r\n'
        b'#qt-notification-duration=3000\r\nqt-notification-duration=777\r\n[unknown]\r\n'
        +('unknown-option='+'x'*32768+' Ää\r\n').encode('utf-8')+b'unknown-last=untouched')
    before=path.read_bytes();a=Peer(path);a.ok('seti qt-notification-duration 5000');a.close();after=path.read_bytes()
    expected=before.replace(b'qt-notification-duration=111\r\n',b'qt-notification-duration=5000\r\n').replace(b'#qt-notification-duration=3000\r\n',b'').replace(b'qt-notification-duration=777\r\n',b'')
    check(after==expected,'BOM CRLF long unknown lines Unicode and final unterminated line preserved; all duplicates removed')
    path=fixture('last-option',b'qt-notification-duration=4321');a=Peer(path)
    check(a.integer('qt-notification-duration')==4321,'Loader retains final option digit without a newline')
    a.close()

    path=fixture('types',seed);a=Peer(path);before=path.read_bytes()
    for command in ['duplicate','setf qt-notification-duration 1','seti no-such-owned-option 1','seti qt-notification-duration -1','setf rate nan','setf rate 1.1234567','sets audio-language 610a62']:
        a.fail(command)
    check(path.read_bytes()==before and a.integer('qt-notification-duration')==777,'Invalid duplicate/type/range/nonfinite/precision/line-break changes are atomic no-ops')
    a.ok('setf rate 1.25');check(float.fromhex(a.command('getf rate').split()[1])==1.25,'Exact native float transaction round trip')
    text='Suomi Ää 日本語';a.ok('sets audio-language '+text.encode('utf-8').hex())
    check(a.command('gets audio-language')=='STRING '+text.encode('utf-8').hex(),'Actual native UTF-8 string transaction round trip')
    a.close();a=Peer(path)
    check(float.fromhex(a.command('getf rate').split()[1])==1.25 and a.command('gets audio-language')=='STRING '+text.encode('utf-8').hex(),
          'Float and UTF-8 values reload in a fresh process')
    a.close()

    path=fixture('read-only',seed);a=Peer(path);assert kernel.SetFileAttributesW(str(path),1)
    try:a.fail('seti qt-notification-duration 5000');assert a.integer('qt-notification-duration')==777
    finally:assert kernel.SetFileAttributesW(str(path),0x80)
    a.ok('seti qt-notification-duration 5000');a.close()
    check(effective(path,'qt-notification-duration',3000)=='5000','Read-only destination rejects write, restores memory, then permits retry')
    path=fixture('first-file');a=Peer(path);a.ok('seti qt-notification-duration 5000');a.close()
    check(path.read_bytes().startswith(b'\xef\xbb\xbf###') and effective(path,'qt-notification-duration',3000)=='5000',
          'First save uses complete default snapshot and requested edit')
    path=fixture('cli',seed);a=Peer(path,'--qt-notification-duration=3333')
    check(a.integer('qt-notification-duration')==777 and a.command('inherit qt-notification-duration')=='INT 3333',
          'Command-line override remains distinct from saved native defaults')
    a.ok('seti qt-remember-window-geometry 0');a.close()
    check(effective(path,'qt-notification-duration',3000)=='777','Unrelated save never persists command-line override')
    path=fixture('reset',seed);a=Peer(path);a.ok('seti qt-notification-duration 5000');a.ok('reset');default=a.integer('qt-notification-duration');a.close()
    check(effective(path,'qt-notification-duration',default)==str(default) and ('#qt-notification-duration='+str(default)).encode() in path.read_bytes(),
          'Explicit reset persists compiled default with normal default commenting')
    path=fixture('startup-reset',seed);a=Peer(path,'--reset-config');startup_default=a.integer('qt-notification-duration');a.close()
    check(startup_default==default and effective(path,'qt-notification-duration',default)==str(default),
          'Startup --reset-config saves defaults before loading the previous file')
    check(all(p.record['normalExit'] and not p.record['forcedCleanupUsed'] for p in peers),'All owned console peers exit normally')
    report['packageCoreUnchanged']=digest(package/'libvlccore.dll')==baseline
    check(report['packageCoreUnchanged'] and digest(runtime/'libvlccore.dll')==digest(core),'Original package Core preserved; staged repaired Core unchanged')
    report['passed']=True;save()
finally:
    for p in peers:
        if not p.closed:
            try:p.close()
            except BaseException as error:
                p.record['cleanupError']=repr(error)
                if p.p.poll() is None:
                    p.p.kill();p.p.wait(timeout=10);p.record['forcedCleanupUsed']=True
                p.log.close()
    save()
