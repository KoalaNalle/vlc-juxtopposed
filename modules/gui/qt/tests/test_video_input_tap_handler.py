# SPDX-License-Identifier: GPL-2.0-or-later
"""Owned Qt Quick pointer tests; no external desktop automation or GPU."""
import argparse
import hashlib
import json
import os
from pathlib import Path
os.environ.update(QT_QPA_PLATFORM='offscreen', QT_QUICK_BACKEND='software', QML_DISABLE_DISK_CACHE='1')
from PySide6.QtCore import QObject, QUrl, QPoint, Qt, Slot, qInstallMessageHandler
from PySide6.QtGui import QGuiApplication
from PySide6.QtQml import QQmlApplicationEngine
from PySide6.QtTest import QTest

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--repo-root', type=Path, required=True)
parser.add_argument('--output-dir', type=Path, required=True)
args = parser.parse_args()
root = args.repo_root.resolve(); output = args.output_dir.resolve(); output.mkdir(parents=True, exist_ok=False)
class Provider(QObject):
    def __init__(self): super().__init__(); self.calls = []
    @Slot(float, float)
    def onMouseMoved(self, x, y): self.calls.append(['move', x, y])
    @Slot(int)
    def onMousePressed(self, button): self.calls.append(['press', button])
    @Slot(int)
    def onMouseReleased(self, button): self.calls.append(['release', button])
    @Slot(int)
    def onMouseDoubleClick(self, button): self.calls.append(['double', button])
    @Slot()
    def onMouseCancelled(self): self.calls.append(['cancel'])
messages = []; qInstallMessageHandler(lambda kind, context, message: messages.append(message))
app = QGuiApplication([]); provider = Provider(); engine = QQmlApplicationEngine()
engine.rootContext().setContextProperty('ownedProvider', provider)
source = '''import QtQuick
import "@PLAYER@" as Player
Window {
 id: window; visible: true; width: 400; height: 300
 property int touchTaps: 0
 Rectangle {
  x: 20; y: 20; width: 300; height: 220; color: "black"
  Player.VideoInputTapHandler {
   objectName: "tap"; videoSurfaceProvider: ownedProvider; pixelRatio: 2
   dragging: drag.active; onTouchTapped: ++window.touchTaps
  }
  DragHandler { id: drag; target: null; dragThreshold: 0; grabPermissions: PointerHandler.CanTakeOverFromAnything }
 }
}'''.replace('@PLAYER@', (root / 'modules/gui/qt/player/qml').as_uri())
engine.loadData(source.encode(), QUrl.fromLocalFile(str(output / 'driver.qml')))
assert engine.rootObjects(), messages
window = engine.rootObjects()[0]; tap = window.findChild(QObject, 'tap'); QTest.qWait(50)
checks = []
def done(name): checks.append(name); print('PASS', name, flush=True)
def reset():
    QTest.qWait(app.styleHints().mouseDoubleClickInterval() + 30); provider.calls.clear()
point = QPoint(70, 80)
reset(); QTest.mouseClick(window, Qt.LeftButton, Qt.NoModifier, point); QTest.qWait(25)
assert ['press', 0] in provider.calls and ['release', 0] in provider.calls, provider.calls
assert ['move', 100.0, 120.0] in provider.calls, provider.calls
assert not any(c[0] in ('cancel', 'double') for c in provider.calls), provider.calls
done('A normal desktop tap relays one native press/release with surface pixel scaling')
reset(); QTest.mouseDClick(window, Qt.LeftButton, Qt.NoModifier, point); QTest.qWait(25)
assert provider.calls.count(['double', 0]) == 1, provider.calls
assert provider.calls[-1] == ['release', 0], provider.calls
done('A desktop double tap forwards native classification and releases its second press')
reset(); QTest.mousePress(window, Qt.LeftButton, Qt.NoModifier, point)
QTest.mouseMove(window, QPoint(130, 120)); QTest.qWait(25)
QTest.mouseRelease(window, Qt.LeftButton, Qt.NoModifier, QPoint(130, 120)); QTest.qWait(25)
assert ['cancel'] in provider.calls and not any(c[0] == 'double' for c in provider.calls), provider.calls
done('Dragging explicitly cancels the native gesture')
reset(); QTest.mousePress(window, Qt.LeftButton, Qt.NoModifier, point)
tap.setProperty('enabled', False); QTest.qWait(25)
QTest.mouseRelease(window, Qt.LeftButton, Qt.NoModifier, point); tap.setProperty('enabled', True)
assert ['cancel'] in provider.calls, provider.calls
reset(); QTest.mouseClick(window, Qt.LeftButton, Qt.NoModifier, point); QTest.qWait(25)
assert ['press', 0] in provider.calls and ['release', 0] in provider.calls and ['cancel'] not in provider.calls, provider.calls
done('Losing a grab cancels it and leaves the next ordinary tap usable')
touch_tested = hasattr(QTest, 'createTouchDevice')
if touch_tested:
    reset(); device = QTest.createTouchDevice()
    QTest.touchEvent(window, device).press(0, point, window).commit(); QTest.qWait(25)
    QTest.touchEvent(window, device).release(0, point, window).commit(); QTest.qWait(25)
    assert window.property('touchTaps') == 1 and not provider.calls, (window.property('touchTaps'), provider.calls)
    done('Touch preserves its control-visibility action without native mouse playback gestures')
errors = [m for m in messages if any(term in m for term in ('ReferenceError', 'TypeError', 'is not a type', 'Cannot assign', 'Unable to assign'))]
assert not errors, errors
report = dict(passed=True, checks=checks, touchTested=touch_tested, qmlMessages=messages,
    nativeCoreAccepted=False, desktopDpiAccepted=False,
    sourceSha256={name:hashlib.sha256((root / name).read_bytes()).hexdigest() for name in (
        'modules/gui/qt/player/qml/VideoInputTapHandler.qml', 'modules/gui/qt/player/qml/PIPPlayer.qml')})
(output / 'verification.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8', newline='\n')
window.close(); engine.deleteLater(); app.processEvents()
