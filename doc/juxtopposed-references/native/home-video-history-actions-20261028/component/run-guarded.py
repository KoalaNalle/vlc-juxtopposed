import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

repo = Path(r'C:\Modding\vlc-juxtopposed')
run = Path(__file__).resolve().parent
assert sys.executable.lower().startswith(str(repo / 'build-windows/homecoming-20261018/tools').lower())
names = [
    'modules/gui/qt/tests/test_juxtopposed_home.py',
    'modules/gui/qt/tests/test_juxtopposed_home_history_actions.py',
    'modules/gui/qt/tests/juxtopposed_palette_fixture.py',
    'modules/gui/qt/medialibrary/qml/JuxtopposedHomePage.qml',
    'modules/gui/qt/medialibrary/qml/JuxtopposedHomeHistoryPage.qml',
    'modules/gui/qt/medialibrary/qml/JuxtopposedHomeDashboard.qml',
    'modules/gui/qt/medialibrary/qml/JuxtopposedHomeCard.qml',
    'modules/gui/qt/medialibrary/qml/JuxtopposedHomeCollectionView.qml',
    'modules/gui/qt/medialibrary/qml/JuxtopposedHomeSorting.js',
    'modules/gui/qt/widgets/qml/PageExt.qml',
    'modules/gui/qt/widgets/qml/ImageExt.qml',
    'modules/gui/qt/style/JuxtopposedPalette.qml',
    'modules/gui/qt/style/vlcicons.hpp',
    'modules/gui/qt/pixmaps/VLCIcons.ttf',
    'modules/gui/qt/pixmaps/juxtopposed/InterVariable.ttf',
]
def digest(path): return hashlib.sha256(path.read_bytes()).hexdigest()
before = {name: digest(repo / name) for name in names}
producer_names = ['reviewed-components-test.py', 'reviewed-history-actions-test.py', 'run-guarded.py']
producer_before = {name: digest(run / name) for name in producer_names}
assert producer_before[producer_names[0]] == before[names[0]]
assert producer_before[producer_names[1]] == before[names[1]]
(run / 'source-identities-before.json').write_text(json.dumps(before, indent=2) + '\n')
(run / 'producer-identities-before.json').write_text(json.dumps(producer_before, indent=2) + '\n')
env = dict(os.environ, PYTHONPATH=str(repo / 'modules/gui/qt/tests'), PYTHONDONTWRITEBYTECODE='1')
results = {}
for name, script, destination, extra in [
    ('components', 'reviewed-components-test.py', 'components', []),
    ('pages', 'reviewed-history-actions-test.py', 'pages', []),
]:
    command = [sys.executable, str(run / script), '--repo-root', str(repo), '--output-dir', str(run / destination), *extra]
    completed = subprocess.run(command, cwd=repo, env=env, capture_output=True, text=True, timeout=45)
    (run / (name + '.stdout.log')).write_text(completed.stdout, encoding='utf-8')
    (run / (name + '.stderr.log')).write_text(completed.stderr, encoding='utf-8')
    results[name] = {'command': command, 'exitCode': completed.returncode, 'timeoutSeconds': 45}
after = {name: digest(repo / name) for name in names}
guards = {name: after[name] == value for name, value in before.items()}
producer_guards = {name: digest(run / name) == value for name, value in producer_before.items()}
result = {'scope': 'Bounded offscreen component tests only; no native VLC, desktop, build, install or Git write.',
    'results': results, 'sourceGuards': guards, 'producerGuards': producer_guards,
    'sourceSha256': before, 'producerSha256': producer_before}
(run / 'guarded-verification.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
assert all(guards.values()) and all(producer_guards.values())
assert all(item['exitCode'] == 0 for item in results.values()), results
print('Both component suites passed; all source/producer guards unchanged; all child jobs exited.')
