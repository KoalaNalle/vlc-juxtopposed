# SPDX-License-Identifier: GPL-2.0-or-later
"""Actual production backdrop: software alpha/geometry/lifecycle, not RHI blur proof."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import shutil

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[3]
os.environ.update(QT_QPA_PLATFORM='offscreen', QT_QUICK_BACKEND='software', QML_DISABLE_DISK_CACHE='1',
                  QT_QPA_FONTDIR=str(REPO/'modules/gui/qt/pixmaps/juxtopposed'))
from PySide6.QtCore import QObject, QUrl, QEvent, QCoreApplication, qInstallMessageHandler
from PySide6.QtGui import QGuiApplication
from PySide6.QtQml import QQmlApplicationEngine, QQmlExpression, qmlContext
from PySide6.QtQuick import QQuickItem, QQuickWindow
from PySide6.QtTest import QTest

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output-dir', required=True, type=Path)
parser.add_argument('--repo-root', type=Path, default=REPO)
args = parser.parse_args()
REPO = args.repo_root.resolve()
os.environ['QT_QPA_FONTDIR'] = str(REPO / 'modules/gui/qt/pixmaps/juxtopposed')
args.output_dir.mkdir(parents=True, exist_ok=False)
runtime = args.output_dir / 'qml'
runtime.mkdir()
identities = {}
relative = 'modules/gui/qt/dialogs/dialogs/qml/'
for name in ['JuxtopposedSettingsBackdrop.qml', 'JuxtopposedSettingsBackdropEffect.qml', 'JuxtopposedSettingsBackdropShadow.qml']:
    source = REPO / relative / name
    identities[source.relative_to(REPO).as_posix()] = hashlib.sha256(source.read_bytes()).hexdigest()
    shutil.copyfile(source, runtime / name)

messages = []
qInstallMessageHandler(lambda kind, context, message: messages.append(message))
app = QGuiApplication([])
engine = QQmlApplicationEngine()
qml_url = QUrl.fromLocalFile(str(runtime.resolve())).toString()
driver = f'''import QtQuick
import QtQuick.Controls.Basic as C
import "{qml_url}" as Dialogs
C.ApplicationWindow {{
    id: window; visible: true; width: 760; height: 560; color: "white"
    property color underlayColor: "red"
    property color tintColor: "#0E0E0E"
    property real settingsScale: 1
    property real popupLeft: 140
    Item {{ id: underlay; objectName: "underlay"; anchors.fill: parent
        Rectangle {{ anchors.fill: parent; color: window.underlayColor }}
    }}
    C.Button {{ id: opener; objectName: "opener"; text: "Open"; onClicked: popup.open() }}
    C.Popup {{
        id: popup; objectName: "popup"; x: window.popupLeft; y: 100; width: 440; height: 340
        padding: 0; focus: true; modal: true; closePolicy: C.Popup.CloseOnEscape
        background: Dialogs.JuxtopposedSettingsBackdrop {{
            id: backdrop; sourceItem: underlay; active: popup.visible
            color: window.tintColor; uiScale: window.settingsScale
        }}
        contentItem: Item {{
            // A foreground item must never be part of the underlying source.
            Rectangle {{ x: 20; y: 20; width: 20; height: 20; color: "#00ff00" }}
        }}
        C.Overlay.modal: Rectangle {{ color: "transparent" }}
        onClosed: opener.forceActiveFocus(Qt.PopupFocusReason)
    }}
    function openSettings() {{ popup.open() }}
    function unsafeSource() {{ backdrop.sourceItem = window.contentItem }}
    function restoreSource() {{ backdrop.sourceItem = underlay }}
}}'''
(args.output_dir / 'driver.qml').write_text(driver, encoding='utf-8')
engine.loadData(driver.encode(), QUrl.fromLocalFile(str(args.output_dir.resolve() / 'driver.qml')))
assert engine.rootObjects(), messages
window = engine.rootObjects()[0]
checks = []

def evaluate(expression):
    expr = QQmlExpression(qmlContext(window), window, expression)
    value = expr.evaluate()
    assert not expr.hasError(), expr.error().toString()
    return value[0] if isinstance(value, tuple) else value

def check(name, value):
    assert value, (name, messages)
    checks.append(name)

def settle():
    QTest.qWait(120)
    app.processEvents()

def pixel(name):
    image = window.grabWindow()
    check(name + ':nonempty', not image.isNull())
    image.save(str(args.output_dir / (name + '.png')))
    point = evaluate('Qt.point(popup.x + 220, popup.y + 170)')
    return image.pixelColor(round(point.x() * window.devicePixelRatio()), round(point.y() * window.devicePixelRatio()))

def channels(color):
    return [color.red(), color.green(), color.blue()]

def close_to(actual, expected, tolerance=2):
    return all(abs(a - e) <= tolerance for a, e in zip(channels(actual), expected))

settle()
evaluate('window.openSettings()')
settle()
backdrop = window.findChild(QObject, 'globalSettingsBackdrop')
loader = window.findChild(QObject, 'globalSettingsBackdropEffectLoader')
check('actual-software-backend-honestly-has-no-RHI-effects', not backdrop.property('effectsAvailable'))
check('source-is-window-sibling-not-overlay-or-popup', backdrop.property('sourceIsSafe'))
check('software-does-not-load-native-effect', not loader.property('active') and not backdrop.property('blurReady'))
check('exact-Figma-25px-radius', backdrop.property('blurRadius') == 25)
check('95-percent-fill-preserved', backdrop.property('fillOpacity') == .95)
check('rounded-14px-fill', backdrop.property('radius') == 14)
rect = backdrop.property('captureRect')
region = backdrop.property('regionInCapture')
check('padded-region-captures-real-source', rect.x() == 65 and rect.y() == 25 and rect.width() == 590 and rect.height() == 490)
check('crop-offset-preserves-popup-position', region.x() == 75 and region.y() == 75 and region.width() == 440)
red = pixel('01-dark-red-underlay')
check('pixels-retain-5-percent-underlay-dark', close_to(red, [26, 13, 13]))
window.setProperty('underlayColor', '#0000ff')
settle()
blue = pixel('02-dark-blue-underlay')
check('dirty-underlay-updates-without-reopen', close_to(blue, [13, 13, 26]) and blue != red)
window.setProperty('tintColor', '#f2f2f2')
settle()
light = pixel('03-light-blue-underlay')
check('live-light-theme-preserves-alpha', close_to(light, [230, 230, 243]))
window.setProperty('popupLeft', 200)
settle()
rect = backdrop.property('captureRect')
check('popup-movement-refreshes-source-crop', rect.x() == 125 and rect.width() == 590)
window.setProperty('width', 640)
window.setProperty('settingsScale', 1.5)
settle()
rect = backdrop.property('captureRect')
check('narrow-scaled-region-clamps-at-real-window-edge', rect.right() == 640 and rect.left() == 87 and rect.top() == 0)
check('scaled-radius-and-blur', backdrop.property('radius') == 21 and backdrop.property('blurRadius') == 37.5)
evaluate('window.unsafeSource()')
settle()
check('recursive-window-capture-rejected', not backdrop.property('sourceIsSafe') and not loader.property('active'))
evaluate('window.restoreSource()')
settle()
check('safe-source-can-be-restored', backdrop.property('sourceIsSafe'))
QTest.keyClick(window, __import__('PySide6.QtCore', fromlist=['Qt']).Qt.Key_Escape)
settle()
check('escape-still-closes-modal', not evaluate('popup.visible'))
check('closed-releases-capture-and-effect', not backdrop.property('sourceIsSafe') and not loader.property('active') and backdrop.property('capturedSource') is None)
check('close-restores-opener-focus', evaluate('opener.activeFocus'))
destroyed = []
engine.destroyed.connect(lambda: destroyed.append(True))
engine.deleteLater()
QCoreApplication.sendPostedEvents(None, QEvent.DeferredDelete)
QTest.qWait(40)
assert destroyed == [True], 'The actual QML engine must be destroyed before checking warnings'
check('no-Qt-warnings', not messages)
report = dict(scope='Actual production pure backdrop component; software pixels verify alpha/dirty underlay/live tint only. Existing native RHI blur, shadow and rounding need the supplied native fixture and app captures.',
              checks=checks, messages=messages, identities=identities,
              pixels=dict(darkRed=channels(red),darkBlue=channels(blue),lightBlue=channels(light)), nativeRhiAccepted=False)
(args.output_dir / 'verification.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
print(json.dumps(report, indent=2))
