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
root = work / 'native-r2'
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
    scope='Actual LibVLC recording/playback continuation on generated media; earlier unit/device results and visible-app acceptance are separate.'), indent=2)+'\n')
print('All selected native checks passed; package identities are unchanged.', flush=True)
