# Copyright (C) 2026 VLC authors and VideoLAN
# Test code licensed under GPLv2 or later.
"""Exercise SliderBar's production handle states without the VLC engine."""
import os
import re
from pathlib import Path

os.environ["QT_QPA_PLATFORM"] = "offscreen"
os.environ["QT_QUICK_BACKEND"] = "software"
os.environ["QML_DISABLE_DISK_CACHE"] = "1"

from PySide6.QtCore import QObject, Qt, QUrl
from PySide6.QtGui import QGuiApplication
from PySide6.QtQml import QQmlComponent
from PySide6.QtQuick import QQuickView
from PySide6.QtTest import QTest

qt = Path(__file__).resolve().parents[1]
source = (qt / "player/qml/SliderBar.qml").read_text(encoding="utf-8")
match = re.search(r"Rectangle\s*\{\s*id:\s*sliderHandle\b", source)
assert match
start = source.index("{", match.start())
depth, end = 1, start + 1
while depth:
    depth += (source[end] == "{") - (source[end] == "}")
    end += 1
handle = source[match.start():end].replace(
    "id: sliderHandle", 'id: sliderHandle\nobjectName: "nativeHandle"', 1)

# Use the real paint/state block; replace only unrelated engine and style
# context. The old implementation expands this handle to 84 pixels when the
# redesigned 28-pixel timeline is hovered.
app = QGuiApplication([])
view = QQuickView()
view.engine().rootContext().setContextProperty("VLCStyle", {
    "margin_small": 4, "duration_short": 10,
})
view.engine().rootContext().setContextProperty("Player", {
    "playingState": 2, "PLAYING_STATE_PLAYING": 2, "hasChapters": False,
})
view.engine().rootContext().setContextProperty("fsm", {
    "_state": "released", "fsmHeld": "held",
})
fixture = """
import QtQuick
import QtQuick.Templates as T
T.ProgressBar {
    id: control
    width: 600
    height: 28
    padding: 0
    value: 0.5
    activeFocusOnTab: true
    property int barHeight: 28
    property bool showHandle: true
    property real _hoveredScalingFactor: 1.8
    property bool _currentChapterHovered: false
    property bool testHover: false
    property var hoverHandler: ({hovered: testHover})
    property var theme: ({fg: {primary: "white"}})
""" + handle + "\n}"
component = QQmlComponent(view.engine())
component.setData(fixture.encode("utf-8"), QUrl())
assert not component.isError(), [e.toString() for e in component.errors()]
root = component.create()
assert root, [e.toString() for e in component.errors()]
view.setContent(QUrl(), component, root)
view.resize(600, 28)
view.show()
item = root.findChild(QObject, "nativeHandle")
assert item

root.setProperty("testHover", True)
QTest.qWait(80)
assert item.isVisible() and item.width() == 84, "Legacy handle remains available"

root.setProperty("showHandle", False)
QTest.qWait(80)
assert not item.isVisible() and item.width() == 0, "Redesigned handle is hidden"
root.setProperty("_currentChapterHovered", True)
root.forceActiveFocus(Qt.TabFocusReason)
QTest.qWait(80)
assert not item.isVisible() and item.width() == 0, "Chapter/focus states stay hidden"

root.setProperty("showHandle", True)
root.setProperty("barHeight", 5)
root.setProperty("_currentChapterHovered", False)
QTest.qWait(80)
assert item.isVisible() and item.width() == 15, "Traditional timeline unchanged"

adapter = (qt / "player/qml/ControlBar.qml").read_text(encoding="utf-8")
assert "showHandle: false" in adapter
print("Passed: native seek handle hover, focus, chapters, and legacy appearance.")
