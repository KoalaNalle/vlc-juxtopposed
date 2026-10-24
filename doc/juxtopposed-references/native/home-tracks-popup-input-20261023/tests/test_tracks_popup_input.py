# SPDX-License-Identifier: GPL-2.0-or-later
"""Check the complete production TracksMenu's popup input ownership in real Qt.

Track pages, palette and native models are supplied. This exercises popup focus,
Escape, outside-click delivery and reopening, not track selection or decoding.
An ordinary VLC replay must separately check its real pages and hotkey routing.
"""
import argparse
from hashlib import sha256
import json
import os
from pathlib import Path
import shutil

os.environ['QT_QPA_PLATFORM'] = 'offscreen'
os.environ['QT_QUICK_BACKEND'] = 'software'
os.environ['QML_DISABLE_DISK_CACHE'] = '1'
os.environ['QT_QPA_FONTDIR'] = str(Path(__file__).resolve().parents[1]/'pixmaps/juxtopposed')
from PySide6.QtCore import QObject, QPoint, Qt, QUrl, qInstallMessageHandler
from PySide6.QtGui import QGuiApplication
from PySide6.QtQuick import QQuickView
from PySide6.QtTest import QTest

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--menu-source', type=Path, default=Path(__file__).resolve().parents[1]/'player/qml/TracksMenu.qml')
parser.add_argument('--output-dir', type=Path, required=True)
args = parser.parse_args()
assert not args.output_dir.exists()
args.output_dir.mkdir(parents=True)
out = args.output_dir.resolve()

def write(path, text):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding='utf-8', newline='\n')

shutil.copyfile(args.menu_source, out/'TracksMenu.qml')
for module in ['Widgets', 'Util']:
    write(out/f'imports/VLC/{module}/qmldir', f'module VLC.{module}\nFixture 1.0 Fixture.qml\n')
    write(out/f'imports/VLC/{module}/Fixture.qml', 'import QtQml\nQtObject {}\n')
write(out/'imports/VLC/Style/qmldir', 'module VLC.Style\nsingleton VLCStyle 1.0 VLCStyle.qml\nColorContext 1.0 ColorContext.qml\n')
write(out/'imports/VLC/Style/VLCStyle.qml', '''pragma Singleton
import QtQml
QtObject {
    property real scale: 1
    property int duration_veryShort: 0
    property int duration_long: 0
    property int margin_xxxsmall: 1
    function dp(value, scale) { return value * scale }
}
''')
write(out/'imports/VLC/Style/ColorContext.qml', '''import QtQuick
QtObject {
    enum ColorSet { Window }
    property int colorSet: ColorContext.Window
    property var palette: ({ isDark: true })
    property var bg: ({ primary: { alpha: function(value) { return "#333333" } } })
    property color border: "#777777"
}
''')
write(out/'TracksListPage.qml', '''import QtQuick
Item {
    id: root
    property real preferredWidth: 400
    property var trackMenuController
    focus: true
    Rectangle { id: subtitles; objectName: "subtitleFocus"; width: 180; height: 100; color: "#444444"; focus: true }
    Rectangle { id: audio; objectName: "audioFocus"; x: 200; width: 180; height: 100; color: "#555555"; focus: true }
    function focusTrack(column) {
        (column === 1 ? audio : subtitles).forceActiveFocus(Qt.PopupFocusReason)
    }
}
''')
write(out/'scene.qml', '''import QtQuick
import QtQuick.Controls.Basic as C
Item {
    id: root
    width: 640; height: 600
    property int playbackClicks: 0
    property int closes: 0
    property alias menuVisible: tracks.visible
    property alias menuFocus: tracks.activeFocus
    property string focusName: Window.window && Window.window.activeFocusItem ? Window.window.activeFocusItem.objectName : ""
    MouseArea {
        anchors.fill: parent
        onClicked: root.playbackClicks += 1
    }
    C.Button {
        id: origin
        objectName: "origin"
        x: 10; y: 540; width: 100; height: 40
        text: "Tracks"
    }
    TracksMenu {
        id: tracks
        x: 200; y: 200
        onClosed: { root.closes += 1; origin.forceActiveFocus(Qt.PopupFocusReason) }
    }
    function showTracks(column) { origin.forceActiveFocus(); tracks.showTracks(column) }
}
''')

messages = []
qInstallMessageHandler(lambda kind, context, message: messages.append(message))
app = QGuiApplication([])
view = QQuickView()
view.engine().addImportPath(str(out/'imports'))
view.setSource(QUrl.fromLocalFile(str(out/'scene.qml')))
assert not view.errors(), [e.toString() for e in view.errors()]
root = view.rootObject()
view.show()
view.requestActivate()
QTest.qWait(80)

cases = []
for column, expected_focus in [(0, 'subtitleFocus'), (1, 'audioFocus')]:
    for dismissal in ['escape', 'outside-click']:
        root.showTracks(column)
        QTest.qWait(80)
        before = dict(visible=root.property('menuVisible'), activeFocus=root.property('menuFocus'), focusName=root.property('focusName'))
        clicks = root.property('playbackClicks')
        closes = root.property('closes')
        if dismissal == 'escape':
            QTest.keyClick(view, Qt.Key_Escape)
        else:
            QTest.mouseClick(view, Qt.LeftButton, Qt.NoModifier, QPoint(100, 100))
        QTest.qWait(80)
        checks = dict(opened=bool(before['visible']), columnFocused=before['focusName'] == expected_focus,
                      closed=not root.property('menuVisible'), closedOnce=root.property('closes') == closes + 1,
                      playbackClickNotDelivered=root.property('playbackClicks') == clicks,
                      originFocusRestored=root.property('focusName') == 'origin')
        cases.append(dict(column=column, dismissal=dismissal, before=before, checks=checks, passed=all(checks.values())))
        # An original failure must not contaminate the next independent opening.
        if root.property('menuVisible'):
            QTest.mouseClick(view, Qt.LeftButton, Qt.NoModifier, QPoint(620, 550))
            QTest.qWait(50)
view.close()
app.processEvents()
report = dict(passed=all(c['passed'] for c in cases) and not messages, cases=cases, diagnostics=messages,
              sourceSha256=sha256(args.menu_source.read_bytes()).hexdigest(),
              producerSha256=sha256(Path(__file__).read_bytes()).hexdigest(), scope=__doc__)
write(out/'verification.json', json.dumps(report, indent=2)+'\n')
print(json.dumps(report, indent=2))
raise SystemExit(0 if report['passed'] else 1)
