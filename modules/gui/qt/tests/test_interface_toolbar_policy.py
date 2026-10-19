# SPDX-License-Identifier: GPL-2.0-or-later
"""Exercise actual toolbar FSM sources with a supplied MainCtx/Player boundary.

This shared-Qt component test does not launch VLC or accept native preferences,
input routing, or desktop/video integration. Source hashes are evidence only;
the worker always imports the selected checkout's current QML bytes.
"""
from pathlib import Path
import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
import xml.etree.ElementTree as ET


HERE = Path(__file__).resolve().parent
REPO = HERE.parents[3]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo-root', type=Path, default=REPO)
    parser.add_argument('--output-dir', type=Path, required=True)
    args = parser.parse_args()
    repo = args.repo_root.resolve()
    output = args.output_dir.resolve()
    output.mkdir(parents=True, exist_ok=False)
    fixture = output / 'qml'
    imports = fixture / 'imports'
    identities = {}

    def stage(relative, destination):
        source = repo / relative
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, destination)
        source_hash = digest(source)
        staged_hash = digest(destination)
        if source_hash != staged_hash:
            raise RuntimeError(f'Staging changed source bytes: {relative}')
        identities[relative] = dict(sourceSha256=source_hash, stagedSha256=staged_hash)

    qt = 'modules/gui/qt/'
    stage(qt + 'player/qml/PlayerToolbarVisibilityFSM.qml',
          imports / 'VLC/PlayerPolicyFixture/PlayerToolbarVisibilityFSM.qml')
    for name in ('FSM.qml', 'FSMState.qml', 'Helpers.qml'):
        stage(qt + 'util/qml/' + name, imports / 'VLC/Util' / name)
    for name in ('MainCtx.qml', 'Player.qml'):
        stage(qt + 'tests/fixtures/interface_toolbar/' + name,
              imports / 'VLC/MainInterface' / name)
    stage(qt + 'tests/tst_InterfaceToolbarPolicy.qml', fixture / 'tst_InterfaceToolbarPolicy.qml')

    modules = {
        'VLC/MainInterface': 'module VLC.MainInterface\nsingleton MainCtx 1.0 MainCtx.qml\nsingleton Player 1.0 Player.qml\n',
        'VLC/Util': 'module VLC.Util\nFSM 1.0 FSM.qml\nFSMState 1.0 FSMState.qml\nsingleton Helpers 1.0 Helpers.qml\n',
        'VLC/PlayerPolicyFixture': 'module VLC.PlayerPolicyFixture\nPlayerToolbarVisibilityFSM 1.0 PlayerToolbarVisibilityFSM.qml\n',
    }
    for module, text in modules.items():
        (imports / module / 'qmldir').write_text(text, encoding='utf-8', newline='\n')
    (output / 'source-identities.json').write_text(
        json.dumps(identities, indent=2) + '\n', encoding='utf-8')

    env = dict(os.environ, QT_QPA_PLATFORM='offscreen', QT_QUICK_BACKEND='software',
               QML_DISABLE_DISK_CACHE='1', PYTHONDONTWRITEBYTECODE='1')
    # Run a separate Qt application so actual QML teardown and exit are included.
    worker = ("import sys; from PySide6.QtQuickTest import QUICK_TEST_MAIN; "
              "sys.exit(QUICK_TEST_MAIN('InterfaceToolbarPolicy', argv=sys.argv, dir=sys.argv[2]))")
    command = [sys.executable, '-c', worker, '-input', str(fixture), '-import', str(imports),
               '-o', str(output / 'qtest.txt') + ',txt',
               '-o', str(output / 'qtest.xml') + ',junitxml']
    report = dict(
        scope='Actual production toolbar FSM, FSMState and Helpers; supplied mutable MainCtx/Player boundary; shared Qt QuickTest software/offscreen.',
        nativeApplicationAccepted=False, nativeConsumerAccepted=False,
        sourceIdentities=identities, expectedTests=41, timeoutSeconds=45,
        timeout=False, exitCode=None, passed=False)
    try:
        result = subprocess.run(command, env=env, capture_output=True, timeout=45,
                                creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
        stdout, stderr = result.stdout, result.stderr
        report['exitCode'] = result.returncode
    except subprocess.TimeoutExpired as error:
        stdout, stderr = error.stdout or b'', error.stderr or b''
        report['timeout'] = True
    (output / 'stdout.txt').write_bytes(stdout)
    (output / 'stderr.txt').write_bytes(stderr)

    xml_path = output / 'qtest.xml'
    text_path = output / 'qtest.txt'
    try:
        suite = ET.parse(xml_path).getroot()
        if suite.tag == 'testsuites':
            suites = suite.findall('testsuite')
            if len(suites) != 1:
                raise ValueError('Expected exactly one Qt test suite')
            suite = suites[0]
        if suite.tag != 'testsuite' or suite.get('name') != 'InterfaceToolbarPolicy':
            raise ValueError('Unexpected Qt test suite')
        report['qtest'] = suite.attrib
        cases = suite.findall('testcase')
        report['cases'] = [case.get('name') for case in cases]
        report['qtProperties'] = {
            item.get('name'): item.get('value') for item in suite.findall('properties/property')}
        qtest_text = text_path.read_text(encoding='utf-8')
        report['warningLines'] = [line for line in qtest_text.splitlines()
                                  if line.startswith(('QWARN ', 'QFATAL '))]
        report['passed'] = (
            not report['timeout'] and report['exitCode'] == 0
            and int(suite.get('tests', -1)) == 41 and len(cases) == 41
            and int(suite.get('failures', -1)) == 0
            and int(suite.get('errors', -1)) == 0
            and int(suite.get('skipped', -1)) == 0
            and not suite.findall('.//failure') and not suite.findall('.//error')
            and not suite.findall('.//skipped') and not report['warningLines'])
    except (OSError, ValueError, ET.ParseError) as error:
        report['resultError'] = str(error)
    (output / 'verification.json').write_text(
        json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({key: report.get(key) for key in
                      ('passed', 'exitCode', 'timeout', 'qtest', 'resultError')}, indent=2))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
