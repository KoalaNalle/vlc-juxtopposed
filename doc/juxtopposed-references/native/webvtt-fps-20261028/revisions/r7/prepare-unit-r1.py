"""Freeze standalone actual WebVTT demux tests; no shared native build writes."""
from pathlib import Path
import ast,hashlib,json,shutil,subprocess
REPO=Path(r'C:\Modding\vlc-juxtopposed');PREP=Path(__file__).resolve().parent
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
assert not (PREP/'unit-config-r1.json').exists()
files=[p for p in (REPO/'include').rglob('*') if p.is_file()]
files += [REPO/p for p in ('modules/demux/webvtt.c','modules/codec/webvtt/webvtt.c','modules/codec/webvtt/webvtt.h','test/modules/demux/webvtt_fps.c')]
files += list((REPO/'test/modules/demux/fixtures/webvtt-fps').glob('*.vtt'))
files += [PREP/'source-preimage/webvtt.c']
source={p.relative_to(REPO).as_posix():sha(p) for p in files}
code=r'''
from pathlib import Path
import hashlib,json,shlex
base=Path('/root/vlc-juxtopposed-build/win64');mods=base/'modules';deps={}
def h(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
for rel in ['demux/.deps/libwebvtt_plugin_la-webvtt.Plo','codec/webvtt/.deps/libwebvtt_plugin_la-webvtt.Plo']:
    file=mods/rel;raw=file.read_text().split('\n\n',1)[0].replace('\\\n',' ')
    tail=raw.split(':',1)[1]
    for token in shlex.split(tail):
        if token.endswith(':'):continue
        path=Path(token);path=path if path.is_absolute() else mods/path;path=path.resolve()
        if path.is_file():deps[str(path)]=h(path)
for path in [base/'config.h',base/'src/.libs/libvlccore.dll.a',base/'compat/.libs/libcompat.a',Path('/usr/lib/gcc/x86_64-w64-mingw32/13-posix/libssp.a')]:deps[str(path)]=h(path)
print(json.dumps(dict(dependencies=deps,compiler='/usr/bin/x86_64-w64-mingw32-gcc',compilerSha256=h('/usr/bin/x86_64-w64-mingw32-gcc'))))
'''
p=subprocess.run(['wsl.exe','-d','VLC-Juxtopposed-Home-20261018','--','/usr/bin/python3','-c',code],capture_output=True,timeout=45,creationflags=subprocess.CREATE_NO_WINDOW)
assert p.returncode==0 and not p.stderr,p.stderr.decode(errors='replace')
config=json.loads(p.stdout)
# The old .Plo records the baseline demux hash; replace only that now-owned input.
config['dependencies']['/mnt/c/Modding/vlc-juxtopposed/modules/demux/webvtt.c']=source['modules/demux/webvtt.c']
config.update(sourceSha256=source,testCore='build-windows/beta-fps-20261028/forced_srt_matroska_clean_candidate/runtime/libvlccore.dll',
    testCoreSha256='f8fc980bbe03da57908f79045f266710dd27a16c6ee66b6c45d60cd1f9b93598',
    storageBudgetBytes=64*1024**2,stopByUnix=1793222700,
    scope='Standalone actual parser/Control/Demux/packet boundaries only; no decoder/render/player/UI/full-format acceptance.')
assert sha(REPO/config['testCore'])==config['testCoreSha256']
for f in ('prepare-unit-r1.py','build-unit-r1.py','run-unit-r1.py'):ast.parse((PREP/f).read_text(),filename=f)
config['producerSha256']={f:sha(PREP/f) for f in ('prepare-unit-r1.py','build-unit-r1.py','run-unit-r1.py')}
(PREP/'unit-config-r1.json').write_text(json.dumps(config,indent=2)+'\n')
print(json.dumps(dict(status='prepared-not-executed',sourceInputs=len(source),readOnlyDependencies=len(config['dependencies']),
    configSha256=sha(PREP/'unit-config-r1.json'),budgetBytes=config['storageBudgetBytes'],noCompilerOrApplicationExecuted=True),indent=2))
