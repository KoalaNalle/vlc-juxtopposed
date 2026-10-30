"""Create only small generated fixtures and freeze the independent writer smoke lane."""
from pathlib import Path
import hashlib, json, os, shutil, subprocess, sys, time
sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parent
REPO = Path(r'C:\Modding\vlc-juxtopposed')
PARENT = ROOT.parent
DISTRO = 'VLC-Juxtopposed-Home-20261018'
def sha(path):
    h=hashlib.sha256()
    with Path(path).open('rb') as f:
        for b in iter(lambda:f.read(1048576),b''): h.update(b)
    return h.hexdigest()
def linux(path): return '/mnt/c/' + Path(path).relative_to(Path('C:/')).as_posix()
assert sys.argv[1:] == ['--prepare-generated-only']
assert not (ROOT/'smoke-config.json').exists()
frozen=json.loads((ROOT/'preparation-producer-identities.json').read_text())
assert all(sha(ROOT/p)==h for p,h in frozen.items()), 'Preparation producer changed after freeze'
budget={'componentsBytes':{'fixturesAndOutputs':16*1024**2,'diagnosticAndProbeLogs':16*1024**2,
    'cacheAndMetadata':16*1024**2,'pagingAndSnapshotAllowance':48*1024**2},
    'peakAdditionalBytes':96*1024**2,'reserveFraction':.2}
disk=shutil.disk_usage(REPO); reserve=(disk.total+4)//5
assert disk.free-reserve >= budget['peakAdditionalBytes'], 'C peak exceeds owner80% target'
cfg=json.loads((PARENT/'runtime-config.json').read_text())
record={'status':'running','started':time.time(),'commands':[],'errors':[],
    'storageBefore':{'freeBytes':disk.free,'minimumFreeBytes':reserve,'headroom':disk.free-reserve,'budgetBytes':budget['peakAdditionalBytes']}}
def save(): (ROOT/'generation.json').write_text(json.dumps(record,indent=2)+'\n')
save()
try:
    fixtures=ROOT/'fixtures'; fixtures.mkdir()
    cases=[('mp4','2.4','libx264','aac','mp4'),('mpegts','2.4','mpeg2video','mp2','ts'),
           ('short-matroska','0.12','libx264','flac','mkv')]
    generated=[]
    for name,duration,video,audio,ext in cases:
        path=fixtures/(name+'.mkv')
        cmd=['wsl.exe','-d',DISTRO,'--','/usr/bin/timeout','--kill-after=2s','25s','/usr/bin/ffmpeg',
            '-nostdin','-v','error','-f','lavfi','-i','testsrc2=size=160x96:rate=25',
            '-f','lavfi','-i','sine=frequency=997:sample_rate=48000','-t',duration,
            '-map','0:v:0','-map','1:a:0','-c:v',video,'-threads:v','1','-g','12','-bf','0',
            '-pix_fmt','yuv420p','-c:a',audio,'-ac','1','-metadata','title=Generated writer smoke '+name]
        if video=='libx264': cmd += ['-preset','ultrafast','-crf','23']
        if audio in ('aac','mp2'): cmd += ['-b:a','128k']
        cmd += ['-f','matroska',linux(path)]
        p=subprocess.run(cmd,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=35,creationflags=subprocess.CREATE_NO_WINDOW)
        record['commands'].append({'name':name,'command':cmd,'exitCode':p.returncode,'stderr':p.stderr.decode(errors='replace')}); save()
        assert p.returncode==0 and not p.stderr, record['commands'][-1]
        assert path.stat().st_size < 2*1024**2
        generated.append({'name':name,'durationSeconds':float(duration),'fixture':path.relative_to(REPO).as_posix(),
            'fixtureSha256':sha(path),'fixtureBytes':path.stat().st_size,'mux':{'mpegts':'mpegts','short-matroska':'matroska'}.get(name,name),
            'outputExtension':ext,'expectedCodecs':[{'mpeg2video':'mpeg2video'}.get(video,'h264'),audio],
            'muxCachingMs':1500 if name=='short-matroska' else 300,'expectedVideoFrames':3 if name=='short-matroska' else 60})
    support=ROOT/'runtime_support.py'; shutil.copyfile(PARENT/'runtime_support.py',support)
    toolcmd=['wsl.exe','-d',DISTRO,'--','/usr/bin/python3','-c',
        'import hashlib,json; print(json.dumps({p:hashlib.sha256(open(p,"rb").read()).hexdigest() for p in ["/usr/bin/ffmpeg","/usr/bin/ffprobe","/usr/bin/timeout"]}))']
    tools=json.loads(subprocess.check_output(toolcmd,timeout=10,creationflags=subprocess.CREATE_NO_WINDOW))
    config={k:cfg[k] for k in ('preservedRun','preservedRuntimeInventory','sourceSha256','runtimeHelperSha256',
        'candidatePlugin','candidatePluginSha256','controlPluginSha256','buildReceipt','buildReceiptSha256')}
    config.update(cases=generated,storageBudget=budget,toolsSha256=tools,distro=DISTRO,
        outputMaxBytes=2*1024**2,runTimeoutSeconds=30,stopByUnix=1793223300,
        parentConfigSha256=sha(PARENT/'runtime-config.json'),
        producers={p:sha(ROOT/p) for p in ('prepare-smoke.py','run-smoke.py','runtime_support.py')})
    (ROOT/'smoke-config.json').write_text(json.dumps(config,indent=2)+'\n')
    assert all(sha(ROOT/p)==h for p,h in frozen.items()), 'Preparation producer changed while running'
    record.update(status='terminal',success=True,fixtureCount=len(generated),fixtureTotalBytes=sum(x['fixtureBytes'] for x in generated),
        configSha256=sha(ROOT/'smoke-config.json'),producerSha256=config['producers'],ended=time.time()); save()
except BaseException as e:
    record.update(status='terminal',success=False,failure=repr(e),ended=time.time()); save(); raise
print(json.dumps({k:record[k] for k in ('status','success','fixtureCount','fixtureTotalBytes','configSha256')},indent=2))
