# SPDX-License-Identifier: GPL-2.0-or-later
"""Exercise the exact production topbar queue bindings during loader teardown.

Supplied loader state reproduces Ready with a null item; native window lifetime
and ordinary docking still require desktop validation.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re

os.environ['QT_QPA_PLATFORM'] = 'offscreen'
from PySide6.QtCore import QObject, QUrl, qInstallMessageHandler
from PySide6.QtGui import QGuiApplication
from PySide6.QtQml import QQmlComponent, QQmlEngine

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--repo-root', type=Path, default=Path(__file__).resolve().parents[4])
parser.add_argument('--output-dir', type=Path, required=True)
parser.add_argument('--expect-original-failures', action='store_true')
args = parser.parse_args()
assert not args.output_dir.exists()
args.output_dir.mkdir(parents=True)
messages = []
qInstallMessageHandler(lambda kind, context, message: messages.append(message))
app = QGuiApplication([])
engine = QQmlEngine()
cases = []
sources = {}
for path, expected in [('modules/gui/qt/player/qml/Player.qml', 1),
                       ('modules/gui/qt/maininterface/qml/MainDisplay.qml', 2)]:
    source_path = args.repo_root / path
    source = source_path.read_text(encoding='utf-8')
    sources[path] = hashlib.sha256(source_path.read_bytes()).hexdigest()
    bindings = re.findall(r'\bplListView:\s*\{([^{}]*)\}', source)
    assert len(bindings) == expected, path
    for index, body in enumerate(bindings):
        component = QQmlComponent(engine)
        component.setData(('''import QtQuick
QtObject {
    id: root
    property QtObject detachedView: QtObject { objectName: "detached" }
    property QtObject dockedView: QtObject { objectName: "docked" }
    property QtObject detachedShell: QtObject { property var playlistView: root.detachedView }
    property QtObject detachedLoader: QtObject { property int status: Loader.Ready; property var item: root.detachedShell }
    property var playlistWindowLoader: detachedLoader
    property QtObject playlistLoader: QtObject { property bool active: false; property var item: root.dockedView }
    property var playlistpopup: playlistLoader
    property var plListView: {''' + body + '''}
}''').encode('utf-8'), QUrl())
        assert not component.isError(), [e.toString() for e in component.errors()]
        root = component.create()
        assert root, [e.toString() for e in component.errors()]
        detached = root.property('detachedLoader')
        docked = root.property('playlistLoader')

        def check(name, expected_name):
            app.processEvents()
            value = root.property('plListView')
            actual = value.objectName() if isinstance(value, QObject) else None
            cases.append(dict(binding=path + ':' + str(index), phase=name,
                              actual=actual, expected=expected_name,
                              diagnostics=list(messages), passed=actual == expected_name and not messages))
            messages.clear()

        check('ready', 'detached')
        detached.setProperty('item', None)
        check('ready-null-item', None)
        detached.setProperty('item', root.property('detachedShell'))
        check('restored', 'detached')
        docked.setProperty('active', True)
        detached.setProperty('item', None)
        check('docked-wins', 'docked')
        detached.setProperty('status', 2)  # Loader.Loading
        docked.setProperty('active', False)
        check('loading', None)
        root.setProperty('playlistWindowLoader', None)
        check('loader-absent', None)
        root.deleteLater()
        app.processEvents()

failed = [(c['binding'], c['phase']) for c in cases if not c['passed']]
expected_negative = [(p + ':' + str(i), phase)
                     for p, n in [('modules/gui/qt/player/qml/Player.qml', 1), ('modules/gui/qt/maininterface/qml/MainDisplay.qml', 2)]
                     for i in range(n) for phase in (['ready-null-item'] if n == 1 else ['ready-null-item', 'loader-absent'])]
result = dict(passed=not failed, expectedNegativeVerified=args.expect_original_failures and failed == expected_negative,
              cases=cases, sourceSha256=sources,
              scope='Exact production binding bodies in a real Qt engine with supplied docked/detached loader states. Not actual native window or desktop acceptance.')
(args.output_dir / 'verification.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
print(json.dumps(dict(passed=result['passed'], expectedNegativeVerified=result['expectedNegativeVerified'], cases=len(cases), failed=failed), indent=2))
assert result['expectedNegativeVerified'] if args.expect_original_failures else result['passed']
