"""Preparation only: read/guard existing inputs and write new ignored artifacts."""
from pathlib import Path
from hashlib import sha256
import difflib
import json
import re
import subprocess

prep = Path(__file__).resolve().parent
repo = Path(r'C:\Modding\vlc-juxtopposed')
root = repo / 'build-windows/beta-fps-20261028'
digest = lambda path: sha256(Path(path).read_bytes()).hexdigest()
assert not (prep / 'preflight.json').exists(), 'Do not rewrite a frozen preparation'
source = repo / 'modules/demux/avformat/mux.c'
original = source.read_bytes()
assert digest(source) == 'd1de834e1108e03565b3a347830f30debf9948b64315355ba5ae57e0acef777a'
newline = '\r\n' if b'\r\n' in original else '\n'
old = original.decode('utf-8')
anchor = 'static void DelStream( sout_mux_t *p_mux, sout_input_t *p_input )' + newline + '{' + newline
assert old.count(anchor) == 1
helper = (prep / 'probe-helper.c.txt').read_text().replace('\r\n', '\n').replace('\n', newline)
new = old.replace(anchor, helper + anchor, 1)
call = '    msg_Dbg( p_mux, "removing input" );' + newline + '    free( p_input->p_sys );'
assert new.count(call) == 1
new = new.replace(call, '    TraceRetiringInput( p_mux, p_input );' + newline + call, 1)
instrumented = new.encode('utf-8')
(prep / 'mux-original.c').write_bytes(original)
(prep / 'mux-instrumented.c').write_bytes(instrumented)
patch = ''.join(difflib.unified_diff(old.splitlines(keepends=True), new.splitlines(keepends=True),
    fromfile='a/modules/demux/avformat/mux.c', tofile='b/modules/demux/avformat/mux.c'))
(prep / 'instrumentation-only.patch').write_bytes(patch.encode('utf-8'))
assert 'vlc_fifo_Dequeue' not in helper and 'vlc_fifo_Queue' not in helper and 'MuxBlock(' not in helper

required = {
    'libvlccore.dll': 'f8fc980bbe03da57908f79045f266710dd27a16c6ee66b6c45d60cd1f9b93598',
    'plugins/codec/liblibass_plugin.dll': 'b622871c205345373343a463f193271b12cf875853e7c913c707acadb16e9b40',
    'plugins/demux/libsubtitle_plugin.dll': '1e8abb4593f82b5badf9dc9d103f655d4a0f30334c28f89e24b102b301139ea8',
    'plugins/codec/libavcodec_plugin.dll': '545f715418c4d20659dce5d6c81ee33b292b21c87243fafa62036b9ffd739e1f',
}
previous_run = root / 'forced_srt_matroska_clean_candidate'
previous_file = previous_run / 'verification.json'
previous = json.loads(previous_file.read_text())
assert previous['status'] == 'terminal' and previous['normalExit']
assert all(previous['runtimeSha256'][name] == value for name, value in required.items())
runtime_checks = {name: digest(previous_run / 'runtime' / name) == value for name, value in previous['runtimeSha256'].items()}
assert all(runtime_checks.values())
assert digest(previous_run / 'output.mkv') == previous['outputSha256']
fixture = root / 'media-record-flac/frame-marker.mkv'
assert digest(fixture) == 'ff9c28f52e759fcfcfd657dda5af5d3115b29d0255363b208dbbc08c9ae16a21'

inputs = {}
prior_inputs = {}
allowed = ('src/', 'include/', 'modules/codec/avcodec/', 'modules/demux/avformat/', 'modules/access/avio.c', 'modules/packetizer/avparser.c')
for line in (root / 'avcodec-plugin-r2/inputs.sha256').read_text().splitlines():
    name = line.split(None, 1)[1].lstrip('*')
    assert name.startswith(allowed), name
    prior_inputs[name] = line.split(None, 1)[0]
    inputs[name] = digest(repo / name)
for name in ['modules/codec/Makefile.am', 'modules/demux/Makefile.am', 'modules/packetizer/av1_obu.c',
             'modules/packetizer/av1_obu.h', 'modules/packetizer/av1.h', 'test/libvlc/native_http_runtime.py']:
    inputs[name] = digest(repo / name)
(prep / 'build-inputs.json').write_text(json.dumps(inputs, indent=2) + '\n')
wsl = r'''
from pathlib import Path
from hashlib import sha256
import json
b=Path('/root/vlc-juxtopposed-build/win64/modules')
p=b/'Makefile';t=p.read_text();lines=t.splitlines()
ranges=list(range(14357,14371))+list(range(28776,28779))
matches=[{'line':i+1,'text':lines[i]} for i in ranges]
assert 'demux/avformat/libavcodec_plugin_la-mux.lo' in t and '-DMERGE_FFMPEG' in t
names=['libavcodec_plugin.la','.libs/libavcodec_plugin.dll','.libs/libavcodec_plugin.lai',
       '.libs/libavcodec_plugin.dll.a','demux/avformat/.libs/libavcodec_plugin_la-mux.o',
       'demux/avformat/libavcodec_plugin_la-mux.lo']
artifacts=[]
for name in names:
    f=b/name
    assert f.is_file() and f.stat().st_nlink==1
    artifacts.append({'path':str(f),'sha256':sha256(f.read_bytes()).hexdigest(),'bytes':f.stat().st_size,'hardlinks':f.stat().st_nlink})
dependency_inputs={}
repo=Path('/mnt/c/Modding/vlc-juxtopposed')
for relative in ['codec/avcodec','demux/avformat','access','packetizer']:
    directory=b/relative/'.deps'
    for dependency in directory.glob('libavcodec*_la-*.Plo'):
        first=dependency.read_text().replace(chr(92)+chr(10),chr(32)).split(chr(10),1)[0]
        for token in first.split(':',1)[1].split():
            f=Path(token)
            if not f.is_absolute(): f=b/f
            f=f.resolve()
            assert f.is_file(),str(f)
            dependency_inputs[str(f)]=sha256(f.read_bytes()).hexdigest()
print(json.dumps({'configuredMakefile':{'path':str(p),'sha256':sha256(p.read_bytes()).hexdigest(),'excerpt':matches},'sharedBuildArtifacts':artifacts,'dependencyInputs':dependency_inputs},indent=2))
'''
result = subprocess.run(['wsl.exe', '-d', 'VLC-Juxtopposed-Home-20261018', '--', '/usr/bin/python3', '-c', wsl], capture_output=True, timeout=30)
assert result.returncode == 0, result.stderr.decode(errors='replace')
assert not result.stderr, result.stderr.decode(errors='replace')
configured = json.loads(result.stdout)
prefix = '/mnt/c/Modding/vlc-juxtopposed/'
project_dependencies = {name[len(prefix):]:value for name,value in configured['dependencyInputs'].items() if name.startswith(prefix)}
changed_dependencies = [name for name,value in project_dependencies.items() if name in prior_inputs and value != prior_inputs[name]]
assert not changed_dependencies, 'Merged plugin dependencies changed since preserved r2: ' + repr(changed_dependencies)
for name,value in project_dependencies.items():
    assert not name.startswith('test/'), name
    inputs[name] = value
(prep / 'build-inputs.json').write_text(json.dumps(inputs, indent=2) + '\n')
assert next(item['sha256'] for item in configured['sharedBuildArtifacts'] if item['path'].endswith('.dll')) == required['plugins/codec/libavcodec_plugin.dll']
config = dict(configured, status='prepared-only-not-executed', sourceOriginalSha256=digest(source),
    sourceInstrumentedSha256=sha256(instrumented).hexdigest(), preservedTuple=required,
    preservedRun=previous_run.relative_to(repo).as_posix(), preservedVerificationSha256=digest(previous_file),
    preservedRuntimeFiles=len(runtime_checks), fixture=fixture.relative_to(repo).as_posix(), fixtureSha256=digest(fixture),
    runtimeHelperSha256=digest(repo/'test/libvlc/native_http_runtime.py'),
    projectDependencyChangesSinceR2=changed_dependencies,
    sourceGuardAfterPreparation=digest(source)==sha256(original).hexdigest(),
    allCurrentBuildInputGuards=all(digest(repo/name)==value for name,value in inputs.items()))
assert config['sourceGuardAfterPreparation'] and config['allCurrentBuildInputGuards']
(prep / 'preflight.json').write_text(json.dumps(config, indent=2) + '\n')
print(json.dumps({'status':config['status'],'runtimeFilesChecked':len(runtime_checks),'currentBuildInputs':len(inputs),
    'plugin':'plugins/codec/libavcodec_plugin.dll','target':'libavcodec_plugin.la','sourceUnchanged':config['sourceGuardAfterPreparation']},indent=2))
