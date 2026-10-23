# SPDX-License-Identifier: GPL-2.0-or-later
"""Check production queue key handlers against real Qt window shortcuts.

This offscreen component fixture substitutes the native Navigation fallback and
search action only. Actual VLC QAction/panel interaction needs an ordinary run.
"""
import argparse
from hashlib import sha256
import json
import os
from pathlib import Path
import re

os.environ["QT_QPA_PLATFORM"] = "offscreen"
os.environ["QT_QUICK_BACKEND"] = "software"
os.environ["QML_DISABLE_DISK_CACHE"] = "1"

from PySide6.QtCore import QObject, Qt, QUrl, qInstallMessageHandler
from PySide6.QtGui import QGuiApplication
from PySide6.QtQuick import QQuickView
from PySide6.QtTest import QTest

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--source", type=Path, default=Path(__file__).resolve().parents[1] / "playlist/qml/PlaylistPane.qml")
parser.add_argument("--output-dir", type=Path, required=True)
args = parser.parse_args()
assert not args.output_dir.exists(), "Use a fresh output directory"
args.output_dir.mkdir(parents=True)
source = args.source.read_text(encoding="utf-8")
section = source[source.index("    Keys.priority: Keys.AfterItem"):]


def handler(name, required=True):
    match = re.search(r"Keys\." + name + r"\s*:\s*(?:\(event\)|event)\s*=>\s*\{", section)
    if not match:
        assert not required, name
        return ""
    # These production key handlers have no braced strings or block comments.
    depth, end = 1, match.end()
    while depth:
        assert end < len(section), name
        depth += (section[end] == "{") - (section[end] == "}")
        end += 1
    return section[match.start():end]


pressed = handler("onPressed").replace("root.Navigation.defaultKeyAction(event)", "harness.fallbackCalls += 1")
override = handler("onShortcutOverride", required=False)
scene = """import QtQuick
Item {
    id: harness
    width: 400; height: 240
    property int searches: 0
    property int folders: 0
    property int otherShortcuts: 0
    property int fallbackCalls: 0
    Shortcut { sequence: "Ctrl+F"; context: Qt.WindowShortcut; onActivated: harness.folders += 1 }
    Shortcut { sequence: "Ctrl+O"; context: Qt.WindowShortcut; onActivated: harness.otherShortcuts += 1 }
    FocusScope {
        id: root
        objectName: "queueScope"
        width: 200; height: 240
        property bool juxtopposed: true
        property int currentTab: 0
        QtObject { id: searchBox; function expandAndFocus() { harness.searches += 1 } }
        Item { objectName: "queueChild"; focus: true }
        Keys.priority: Keys.AfterItem
""" + override + "\n" + pressed + """
    }
    Item { objectName: "outsideQueue"; x: 200; width: 200; height: 240 }
}
"""
scene_path = args.output_dir / "queue-shortcut.qml"
scene_path.write_text(scene, encoding="utf-8", newline="\n")
messages = []
qInstallMessageHandler(lambda kind, context, message: messages.append(message))
app = QGuiApplication([])
view = QQuickView()
view.setSource(QUrl.fromLocalFile(str(scene_path.resolve())))
assert not view.errors(), [e.toString() for e in view.errors()]
root = view.rootObject()
assert root
queue = root.findChild(QObject, "queueScope")
child = root.findChild(QObject, "queueChild")
outside = root.findChild(QObject, "outsideQueue")
assert queue and child and outside
view.show()
view.requestActivate()
QTest.qWait(150)
cases = []
for name, redesigned, tab, inside, key, expected in [
    ("focusedQueueFind", True, 0, True, Qt.Key_F, (1, 0, 0)),
    ("bookmarksKeepsGlobalFolder", True, 1, True, Qt.Key_F, (0, 1, 0)),
    ("chaptersKeepsGlobalFolder", True, 2, True, Qt.Key_F, (0, 1, 0)),
    ("legacyKeepsGlobalFolder", False, 0, True, Qt.Key_F, (0, 1, 0)),
    ("outsideQueueKeepsGlobalFolder", True, 0, False, Qt.Key_F, (0, 1, 0)),
    ("otherShortcutUnchanged", True, 0, True, Qt.Key_O, (0, 0, 1)),
    ("queueFindAfterFocusReturn", True, 0, True, Qt.Key_F, (1, 0, 0)),
]:
    for field in ["searches", "folders", "otherShortcuts", "fallbackCalls"]:
        root.setProperty(field, 0)
    queue.setProperty("juxtopposed", redesigned)
    queue.setProperty("currentTab", tab)
    focus_item = child if inside else outside
    focus_item.forceActiveFocus(Qt.OtherFocusReason)
    app.processEvents()
    assert focus_item.hasActiveFocus(), name
    QTest.keyClick(view, key, Qt.ControlModifier)
    QTest.qWait(60)
    actual = tuple(root.property(field) for field in ["searches", "folders", "otherShortcuts"])
    case = dict(name=name, expected=list(expected), actual=list(actual), passed=actual == expected)
    cases.append(case)
    print(("PASS " if case["passed"] else "FAIL ") + name + ": " + str(actual))
view.close()
app.processEvents()
result = dict(passed=all(c["passed"] for c in cases) and not messages,
              cases=cases, diagnostics=messages, sourceSha256=sha256(args.source.read_bytes()).hexdigest(),
              scope="Offscreen real Qt shortcut dispatch using extracted production key handlers; native Navigation/search actions replaced by counters. No VLC playlist or Windows UI acceptance.")
(args.output_dir / "verification.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8", newline="\n")
assert result["passed"], result
