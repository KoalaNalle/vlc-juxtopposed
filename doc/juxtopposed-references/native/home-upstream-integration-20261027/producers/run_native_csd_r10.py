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
root = work / 'native-csd-r10'
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
unit_runtime = root / 'unit-runtime'
stage_runtime(package, unit_runtime)
for name in ['test_qt_csd_button_model.exe']:
    shutil.copyfile(work / 'csd-r10' / name, unit_runtime / name)
regenerate_cache(unit_runtime, root / 'unit-cache')
csd = run_owned([unit_runtime / 'test_qt_csd_button_model.exe', '-platform', 'offscreen',
                 '-o', str(root / 'csd.xml')+',junitxml', '-o', str(root / 'csd.txt')+',txt'],
                unit_runtime, root / 'csd-unit', timeout=40)
csd['name'] = 'csd-unit'
(root / 'process.json').write_text(json.dumps(csd, indent=2)+'\n'); print(json.dumps(csd), flush=True)
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

guard()
(root / 'summary.json').write_text(json.dumps(dict(passed=True, testMethods=sorted(expected), cases=len(actual), packageUnchanged=True, sourceCommit='7c54e67517971ef1d4daa9bf3226776c3b943b79', scope='Actual upstream CSD model native QtTest with static Windows offscreen platform; ordinary window behavior remains separate.'), indent=2)+'\n')
print('Native CSD checks passed.', flush=True)
