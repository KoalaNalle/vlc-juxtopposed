from pathlib import Path
from hashlib import file_digest
import json
import os
import sys
import xml.etree.ElementTree as ET

root = Path(__file__).resolve().parent
repo = root.parents[1]
sys.path.insert(0, str(repo / 'test/libvlc'))
import native_http_runtime as helper
from native_home_activity import SUITES

def digest(path):
    with Path(path).open('rb') as stream:
        return file_digest(stream, 'sha256').hexdigest()

build = root / 'history-actions-test-build-r2'
assert (build / 'exit.txt').read_text().strip() == '0'
inputs = {line.split('  ', 1)[1]: line.split('  ', 1)[0]
          for line in (build / 'inputs.sha256').read_text().splitlines()}
assert all(digest(repo / name) == sha for name, sha in inputs.items())
built = {Path(line.split('  ', 1)[1]).name: line.split('  ', 1)[0]
         for line in (build / 'binaries.sha256').read_text().splitlines()}
assert all(digest(build / name) == sha for name, sha in built.items())
plugin = root / 'ml-lock-review/dependency-plugin-r1/candidate/libmedialibrary_plugin.dll'
plugin_sha = 'e834a41a700bbc6ffeaa83bfde895f5b60f35c2e11f08cd6526d5122afe5af3f'
assert digest(plugin) == plugin_sha
base = root / 'ordinary_home_r2/runtime'
base_manifest = json.loads((base.parent / 'verification.json').read_text())['runtimeSha256']
assert all(digest(base / name) == sha for name, sha in base_manifest.items())
fixture = repo / 'build-windows/home-native-fixtures-20261014'
fixtures = {p.name: digest(p) for p in fixture.iterdir() if p.is_file()}
producer = digest(__file__)
out = root / 'history-actions-native-r2'
out.mkdir(exist_ok=False)
runtime = out / 'runtime'
stage = helper.stage_runtime(base, runtime)
for name, source in [('libvlccore.dll', build / 'libvlccore.dll'),
                     ('plugins/gui/libqt_plugin.dll', build / 'libqt_plugin.dll'),
                     ('plugins/misc/libmedialibrary_plugin.dll', plugin)]:
    (runtime / name).unlink()
    os.link(source, runtime / name)
exe = build / 'test_qt_home_activity.exe'
os.link(exe, runtime / exe.name)
original_environment = helper.environment
def environment(runtime, output):
    env = original_environment(runtime, output)
    env.pop('VLC_PLUGIN_PATH', None)
    env.update(VLC_HOME_FIXTURE_DIR=str(fixture), VLC_TEST_LOCAL_ML='1')
    return env
helper.environment = environment
helper.regenerate_cache(runtime, out / 'cache')
manifest = {name: digest(runtime / name) for name in base_manifest}
record = dict(status='running', scope='Full Home native suite, including real scoped history edits and active-input Stop/Close. No QML/desktop claim.',
              sourceSha256=inputs, runtimeSha256=manifest, fixtureSha256=fixtures,
              producerSha256=producer, testSha256=built[exe.name], stage=stage,
              expectedCases=SUITES['home-activity']['cases'])
def save():
    (out / 'verification.json').write_text(json.dumps(record, indent=2) + '\n')
save()
record.update(helper.run_owned([runtime / exe.name, '-o', str(out / 'result.xml') + ',xml',
                                '-o', str(out / 'result.txt') + ',txt'], runtime, out, timeout=240))
record.update(status='terminal', sourceUnchanged=all(digest(repo / n) == h for n, h in inputs.items()),
              fixtureUnchanged=all(digest(fixture / n) == h for n, h in fixtures.items()),
              runtimeUnchanged=all(digest(runtime / n) == h for n, h in manifest.items()),
              baselineUnchanged=all(digest(base / n) == h for n, h in base_manifest.items()),
              testUnchanged=digest(exe) == built[exe.name] and digest(runtime / exe.name) == built[exe.name],
              producerUnchanged=digest(__file__) == producer)
save()
try:
    tree = ET.parse(out / 'result.xml').getroot()
    record['functions'] = [dict(name=f.get('name'), incidents=[dict(type=i.get('type'), description=i.findtext('Description', ''))
                            for i in f.findall('Incident')]) for f in tree.findall('TestFunction')]
except (ET.ParseError, FileNotFoundError) as error:
    record['resultParseError'] = str(error)
save()
print(json.dumps({k: v for k, v in record.items() if k not in ('sourceSha256', 'runtimeSha256', 'fixtureSha256')}, indent=2))
assert record['normalExit'] and not record.get('resultParseError')
assert all(v for k, v in record.items() if k.endswith('Unchanged'))
assert {f['name'] for f in record['functions']} == set(record['expectedCases'])
assert all(i['type'] == 'pass' for f in record['functions'] for i in f['incidents'])
