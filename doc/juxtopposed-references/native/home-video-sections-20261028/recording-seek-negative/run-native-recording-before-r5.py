from pathlib import Path
from hashlib import file_digest
import json
import os
import shutil
import sys
import xml.etree.ElementTree as ET

root=Path(__file__).resolve().parent
repo=root.parents[1]
sys.path.insert(0,str(repo/'test/libvlc'))
import native_http_runtime as helper
def digest(path):
    with path.open('rb') as stream: return file_digest(stream,'sha256').hexdigest()
base=repo/'build-windows/beta-fps-20261028/ui_ass_replay_av2/runtime'
manifest=json.loads((base.parent/'verification.json').read_text())['runtimeSha256']
assert all(digest(base/name)==h for name,h in manifest.items())
build=root/'native-r12'
assert (build/'exit.txt').read_text().strip()=='0'
inputs={line.split('  ',1)[1]:line.split('  ',1)[0] for line in (build/'inputs.sha256').read_text().splitlines()}
assert all(digest(repo/name)==h for name,h in inputs.items())
exe=build/'test_qt_home_activity.exe'
binary=digest(exe)
assert binary==(build/'binary.sha256').read_text().split()[0]
fixture=repo/'build-windows/home-native-fixtures-20261014'
fixtures={p.name:digest(p) for p in fixture.iterdir() if p.is_file()}
producer=digest(Path(__file__))
out=root/'native-recording-before-r5';out.mkdir(exist_ok=False)
runtime=out/'runtime'
stage=helper.stage_runtime(base,runtime)
os.link(exe,runtime/exe.name)
core=root/'native-r11/libvlccore.dll'
core_sha=digest(core)
assert core_sha==(root/'native-r11/core.sha256').read_text().split()[0]
(runtime/'libvlccore.dll').unlink()
os.link(core,runtime/'libvlccore.dll')
runtime_manifest=dict(manifest, **{'libvlccore.dll':core_sha})
original_environment=helper.environment
def environment(runtime,output):
    env=original_environment(runtime,output);env.pop('VLC_PLUGIN_PATH',None)
    env.update(VLC_HOME_FIXTURE_DIR=str(fixture),VLC_TEST_LOCAL_ML='1')
    return env
helper.environment=environment
helper.regenerate_cache(runtime,out/'cache')
record=dict(status='running',scope='Retained r11 Core negative control for recording seek guard; source guards identify test build.',sourceSha256=inputs,runtimeSha256=runtime_manifest,producerSha256=producer,
    fixtureSha256=fixtures,testSha256=binary,stage=stage)
def save(): (out/'verification.json').write_text(json.dumps(record,indent=2)+'\n')
save()
record.update(helper.run_owned([runtime/exe.name,'nativeVideoHistoryRejectedRecordingSeek','-o',str(out/'result.xml')+',xml',
    '-o',str(out/'result.txt')+',txt'],runtime,out,timeout=180))
record.update(status='terminal',sourceUnchanged=all(digest(repo/n)==h for n,h in inputs.items()),
    fixtureUnchanged=all(digest(fixture/n)==h for n,h in fixtures.items()),
    runtimeUnchanged=all(digest(runtime/n)==h for n,h in runtime_manifest.items() if not n.endswith('plugins.dat')) and all(digest(base/n)==h for n,h in manifest.items()),
    testUnchanged=digest(exe)==binary and digest(runtime/exe.name)==binary,producerUnchanged=digest(Path(__file__))==producer)
if (out/'result.xml').exists():
    tree=ET.parse(out/'result.xml').getroot()
    record['functions']=[dict(name=f.get('name'),incidents=[dict(type=i.get('type'),description=i.findtext('Description','')) for i in f.findall('Incident')]) for f in tree.findall('TestFunction')]
save()
print(json.dumps({k:v for k,v in record.items() if k not in ('runtimeSha256','sourceSha256')},indent=2))
assert record['normalExit'] and all(record[k] for k in ('sourceUnchanged','fixtureUnchanged','runtimeUnchanged','testUnchanged','producerUnchanged'))
