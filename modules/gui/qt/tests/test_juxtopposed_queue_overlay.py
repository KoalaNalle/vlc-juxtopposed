# Visual design by Juxtopposed; test code licensed under GPLv2 or later.
"""Exercise the production Player queue anchors in an actual Qt scene."""
import os
from pathlib import Path
import re

os.environ["QT_QPA_PLATFORM"] = "offscreen"
os.environ["QT_QUICK_BACKEND"] = "software"
os.environ["QML_DISABLE_DISK_CACHE"] = "1"

from PySide6.QtCore import QObject, Property, Signal, Slot, QUrl
from PySide6.QtGui import QGuiApplication
from PySide6.QtQml import QQmlComponent, QQmlEngine, qmlRegisterSingletonInstance


class Scale(QObject):
    changed = Signal()

    def __init__(self):
        super().__init__()
        self._scale = 1.0

    @Property(float, notify=changed)
    def scale(self):
        return self._scale

    def set_scale(self, value):
        self._scale = value
        self.changed.emit()

    @Slot(float, float, result=float)
    def dp(self, value, scale):
        return value * scale


app = QGuiApplication([])
style = Scale()
qmlRegisterSingletonInstance(Scale, "Test.QueueOverlay", 1, 0, "VLCStyle", style)
player_file = Path(__file__).resolve().parents[1] / "player/qml/Player.qml"
player = player_file.read_text(encoding="utf-8")
anchors = re.search(r"id: playlistpopup\s*(anchors \{[\s\S]*?\n        \})", player).group(1)
engine = QQmlEngine()
component = QQmlComponent(engine)
component.setData(("""
import QtQuick
import Test.QueueOverlay 1.0
Item {
    id: rootPlayer
    width: 1920 * VLCStyle.scale
    height: 1080 * VLCStyle.scale
    property bool _controlsUnderVideo: false
    property bool redesigned: true
    property real headerHeight: 40
    property real positionSliderY: height - 120 * VLCStyle.scale
    Rectangle {
        id: topBar
        width: parent.width
        height: rootPlayer.headerHeight * VLCStyle.scale
        property real reservedHeight: height
    }
    Rectangle {
        id: controlBar
        anchors.bottom: parent.bottom
        width: parent.width
        height: 54 * VLCStyle.scale
        property bool redesigned: rootPlayer.redesigned
    }
    Loader {
        id: playlistpopup
        objectName: "overlay"
        width: 400 * VLCStyle.scale
""" + anchors + "\n    }\n}\n").encode(), QUrl())
assert not component.isError(), [error.toString() for error in component.errors()]
root = component.create()
assert root, [error.toString() for error in component.errors()]
overlay = root.findChild(QObject, "overlay")


def check(expected):
    app.processEvents()
    actual = [overlay.property(key) for key in ["x", "y", "width", "height"]]
    assert actual == expected, (actual, expected)


for scale in [1, 1.5, 2]:
    style.set_scale(scale)
    root.setProperty("redesigned", True)
    root.setProperty("_controlsUnderVideo", False)
    check([value * scale for value in [1500, 60, 400, 946]])
    root.setProperty("_controlsUnderVideo", True)
    check([value * scale for value in [1500, 60, 400, 946]])
    # The legacy queue keeps its full right edge and slider-relative bottom.
    root.setProperty("redesigned", False)
    check([value * scale for value in [1520, 40, 400, 920]])
    root.setProperty("_controlsUnderVideo", False)
    check([value * scale for value in [1520, 0, 400, 960]])

style.set_scale(1)
root.setProperty("redesigned", True)
root.setProperty("headerHeight", 80)
check([1500, 80, 400, 926])
print("Passed: production queue overlay anchors, canonical 1920x1080 placement, pinned controls, taller header, 100/150/200-percent scaling and legacy opt-out.")
