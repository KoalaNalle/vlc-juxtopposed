from hashlib import file_digest
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import xml.etree.ElementTree as ET
import PySide6

work = Path(__file__).resolve().parent
repo = work.parents[1]
package = work / 'runtime-r2'
root = work / 'native-r1'
root.mkdir(exist_ok=False)
base_python = Path(sys._base_executable)
os.environ['PYTHONPATH'] = str(Path(PySide6.__file__).parent.parent)
os.environ['QT_QPA_FONTDIR'] = str(repo / 'modules/gui/qt/pixmaps/juxtopposed')
os.environ['QT_QPA_PLATFORM'] = 'offscreen'
sys.path.insert(0, str(repo / 'test/libvlc'))
from native_http_runtime import stage_runtime, regenerate_cache, run_owned

def digest(path):
    with path.open('rb') as stream: return file_digest(stream, 'sha256').hexdigest()

manifest = json.loads((work / 'runtime-r2-manifest.json').read_text())
def guard():
    assert all(digest(package / name) == value['sha256'] for name, value in manifest['files'].items())
guard()
results = []
def save():
    (root / 'results.json').write_text(json.dumps(results, indent=2)+'\n')

def command(name, argv, limit=80):
    result = run_owned(argv, package, root / name, timeout=limit)
    result['name'] = name
    results.append(result)
    save()
    print(name, json.dumps(result), flush=True)
    assert result['normalExit'], name
    return result

for label, script, extra in [
    ('mmdevice-unit', 'native_mmdevice_stop.py', ['--probe', str(work / 'build-r5/probes/test_mmdevice_stop.exe')]),
    ('mmdevice-lifecycle', 'native_mmdevice_lifecycle.py', []),
]:
    command(label, [base_python, repo / 'test/libvlc' / script, '--package', package,
                    '--output', root / (label + '-evidence'), *extra],
            limit=140 if label == 'mmdevice-lifecycle' else 90)
    result_path = root / (label + '-evidence') / ('result.json' if label == 'mmdevice-unit' else 'lifecycle.json')
    # The lifecycle parent writes result.json and retains the child's lifecycle.json.
    if not result_path.exists(): result_path = root / (label + '-evidence') / 'result.json'
    evidence = json.loads(result_path.read_text())
    assert evidence['passed'], evidence

unit_runtime = root / 'unit-runtime'
stage_runtime(package, unit_runtime)
for name in ['test_record_start.exe', 'test_qt_csd_button_model.exe']:
    shutil.copyfile(work / 'build-r5/probes' / name, unit_runtime / name)
regenerate_cache(unit_runtime, root / 'unit-cache')
unit = run_owned([unit_runtime / 'test_record_start.exe'], unit_runtime, root / 'record-unit', timeout=30)
unit['name'] = 'record-unit'
results.append(unit); save()
assert unit['normalExit'], unit
record_summary = json.loads((root / 'record-unit/stdout.txt').read_text().splitlines()[-1])
assert record_summary == {'checks': 22, 'failures': 0}, record_summary
csd = run_owned([unit_runtime / 'test_qt_csd_button_model.exe', '-platform', 'offscreen',
                 '-o', str(root / 'csd.xml')+',junitxml', '-o', str(root / 'csd.txt')+',txt'],
                unit_runtime, root / 'csd-unit', timeout=40)
csd['name'] = 'csd-unit'
results.append(csd); save()
assert csd['normalExit'], csd
suite = ET.parse(root / 'csd.xml').getroot()
if suite.tag == 'testsuites':
    suites = suite.findall('testsuite'); assert len(suites) == 1
    suite = suites[0]
expected = set(re.findall(r'^    void (test_\w+)\(',
    (repo / 'modules/gui/qt/tests/test_qt_csd_button_model.cpp').read_text(), re.MULTILINE))
actual = {case.get('name').split('::')[-1] for case in suite.findall('testcase')}
assert actual == expected | {'initTestCase', 'cleanupTestCase'}, actual
assert not suite.findall('.//failure') and not suite.findall('.//error') and not suite.findall('.//skipped')
assert not re.search(r'^(QWARN|QFATAL)', (root / 'csd.txt').read_text(), re.MULTILINE)
command('recording-consumers', [base_python, repo / 'test/libvlc/native_recording_consumers.py',
    '--package', package, '--video-fixture', work / 'media/colors.mp4',
    '--output-dir', root / 'recording-consumers-evidence'], limit=100)
assert json.loads((root / 'recording-consumers-evidence/verification.json').read_text())['passed']
command('paused-rgb', [base_python, repo / 'test/libvlc/native_paused_seek.py',
                       '--package', package, '--media', work / 'media/colors.mp4',
                       '--output-dir', root / 'paused-rgb-evidence'], limit=45)

old = repo / 'build-windows/beta-single-frame-20261025-night'
fixtures = repo / 'build-windows/beta-subtitle-fps-20261024/media-r1'
for label, media, selection in [
    ('mp4-english', fixtures / 'frame-marker.mp4', 'eng'),
    ('mp4-bframes', old / 'mp4-media-r234/b4.mp4', 'eng'),
    ('mkv-dual', work / 'media/frame-marker.mkv', 'both'),
]:
    evidence = root / (label + '-evidence')
    evidence.mkdir()
    command(label, [base_python, repo / 'test/libvlc/native_mp4_subtitle_seek.py',
                   package, evidence, media, fixtures / 'external-fps.srt', '1.0', 'wasapi', '12', selection], limit=60)
    assert json.loads((evidence / 'result.json').read_text())['passed'], label
guard()
(root / 'summary.json').write_text(json.dumps(dict(passed=True, results=results,
    packageUnchanged=True, sourceCommit=manifest['sourceCommit'],
    scope='Controlled native units and actual LibVLC playback on generated local media; visible app and broader beta acceptance are separate.'), indent=2)+'\n')
print('All selected native checks passed; package identities are unchanged.', flush=True)
