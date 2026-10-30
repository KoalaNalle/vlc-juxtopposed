# SPDX-License-Identifier: GPL-2.0-or-later
"""Exercise the production error notification with supplied dialog boundaries."""
import argparse
from hashlib import sha256
import json
import os
from pathlib import Path
import shutil

os.environ.update(QT_QPA_PLATFORM='offscreen', QT_QUICK_BACKEND='software',
                  QML_DISABLE_DISK_CACHE='1')

from PySide6.QtCore import QObject, QPoint, Qt, QUrl, qInstallMessageHandler
from PySide6.QtGui import QGuiApplication
from PySide6.QtQml import QQmlExpression, qmlContext
from PySide6.QtQuick import QQuickView, QQuickWindow
from PySide6.QtTest import QTest

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--repo-root', type=Path, default=Path(__file__).resolve().parents[4])
parser.add_argument('--output-dir', type=Path, required=True)
args = parser.parse_args()
repo, out = args.repo_root.resolve(), args.output_dir.resolve()
out.mkdir(parents=True, exist_ok=False)
os.environ['QT_QPA_FONTDIR'] = str(repo / 'modules/gui/qt/pixmaps/juxtopposed')

source = repo / 'modules/gui/qt/dialogs/dialogs/qml/Dialogs.qml'
drawer = repo / 'modules/gui/qt/widgets/qml/DrawerExt.qml'
code = source.read_text(encoding='utf-8')
startup = code[code.index('    Component.onCompleted: if (showErrorNotifications'):
               code.index('    Component.onDestruction:')]
connection_start = code.index('    Connections\n    {\n        target: DialogErrorModel')
connection = code[connection_start:code.index('    //---------------------------------------------------------------------------------------------', connection_start)]
popup_start = code.index('    Widgets.DrawerExt {\n        id: errorPopup')
popup = code[popup_start:code.index('    ModalDialog {', popup_start)]

def write(relative, content):
    path = out / relative
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding='utf-8', newline='\n')

write('imports/VLC/Style/qmldir', 'module VLC.Style\nsingleton VLCStyle 1.0 VLCStyle.qml\nsingleton VLCIcons 1.0 VLCIcons.qml\nColorContext 1.0 ColorContext.qml\n')
write('imports/VLC/Style/VLCStyle.qml', '''pragma Singleton
import QtQml
QtObject {
    property int margin_small: 8; property int margin_normal: 12
    property int margin_xsmall: 4; property int margin_xxsmall: 4
    property int fontHeight_normal: 16; property int fontSize_normal: 14
    property int icon_normal: 14; property int duration_short: 0
    property real scale: 1; property var palette: null
    function dp(value, scale) { return value * scale }
}
''')
write('imports/VLC/Style/VLCIcons.qml', 'pragma Singleton\nimport QtQml\nQtObject { property string info: "i"; property string clear: "x" }\n')
write('imports/VLC/Style/ColorContext.qml', '''import QtQml
QtObject {
    enum Set { Window, ButtonAccent }
    property var palette; property int colorSet: 0
    property var bg: ({negative: "#c62828", primary: "#181818"})
    property var fg: ({primary: "#ffffff"})
}
''')
write('imports/VLC/Dialogs/qmldir', 'module VLC.Dialogs\nsingleton DialogErrorModel 1.0 DialogErrorModel.qml\nsingleton DialogsProvider 1.0 DialogsProvider.qml\n')
write('imports/VLC/Dialogs/DialogErrorModel.qml', '''pragma Singleton
import QtQml
QtObject {
    property int repeatedMessageCount: 0
    property string notificationText: ""
    property int resets: 0
    signal countChanged()
    function pushError(text) { notificationText = text; repeatedMessageCount++; countChanged() }
    function resetRepeatedMessageCount() { resets++; repeatedMessageCount = 0 }
}
''')
write('imports/VLC/Dialogs/DialogsProvider.qml', '''pragma Singleton
import QtQml
QtObject {
    property var pages: []
    signal messageRequested(int page)
    function messagesDialog(page) { pages = pages.concat([page]); messageRequested(page) }
}
''')
write('imports/VLC/Widgets/qmldir', 'module VLC.Widgets\nDrawerExt 1.0 DrawerExt.qml\nIconLabel 1.0 IconLabel.qml\nTextToolButton 1.0 TextToolButton.qml\nIconToolButton 1.0 IconToolButton.qml\nRoundedRectangleShadow 1.0 RoundedRectangleShadow.qml\n')
shutil.copyfile(drawer, out / 'imports/VLC/Widgets/DrawerExt.qml')
write('imports/VLC/Widgets/IconLabel.qml', 'import QtQuick\nText {}\n')
write('imports/VLC/Widgets/TextToolButton.qml', '''import QtQuick
import QtQuick.Controls.Basic
import VLC.Style
Button { property ColorContext colorContext: ColorContext {} }
''')
write('imports/VLC/Widgets/IconToolButton.qml', '''import QtQuick
import QtQuick.Controls.Basic
Button {
    property string description: ""; property color color: "white"; property color backgroundColor: "black"
    property QtObject colorContext: QtObject {
        property var fg: ({primary: "white"}); property var bg: ({negative: "red"})
    }
}
''')
write('imports/VLC/Widgets/RoundedRectangleShadow.qml', 'import QtQuick\nItem { property real yOffset: 0; property real blurRadius: 0; property color color: "transparent" }\n')
write('NotificationHarness.qml', '''import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import QtQuick.Controls.Basic as C
import QtQuick.Window
import VLC.Dialogs
import VLC.Widgets as Widgets
import VLC.Style
FocusScope {
    id: outerScope
    width: 800; height: 300
Item {
    id: root
    objectName: "harnessRoot"
    anchors.fill: parent
    property bool showErrorNotifications: true
    property Item bgContent: outerScope
    property bool simulateMessages: false
    C.Button { id: backgroundButton; objectName: "backgroundFocus"; text: "Previous control"; activeFocusOnTab: true; focus: true }
    C.Button { id: mockMessage; objectName: "mockMessageFocus"; text: "Messages window"; visible: false }
    Connections {
        target: DialogsProvider
        function onMessageRequested(page) {
            if (root.simulateMessages) {
                mockMessage.visible = true
                mockMessage.forceActiveFocus(Qt.OtherFocusReason)
            }
        }
    }
''' + startup + connection + popup + '\n}\n}\n')

messages = []
qInstallMessageHandler(lambda kind, context, message: messages.append(message))
app = QGuiApplication([])
view = QQuickView()
view.engine().addImportPath(str(out / 'imports'))
view.setResizeMode(QQuickView.SizeRootObjectToView)
view.setSource(QUrl.fromLocalFile(str(out / 'NotificationHarness.qml')))
assert not view.errors(), [error.toString() for error in view.errors()]
view.resize(800, 300)
view.show()
QTest.qWait(150)
root = view.rootObject().findChild(QObject, 'harnessRoot')
assert root

def evaluate(code):
    expr = QQmlExpression(qmlContext(root), root, code)
    result = expr.evaluate()[0]
    assert not expr.hasError(), expr.error().toString()
    return result

def item(name):
    result = root.findChild(QObject, name)
    assert result, name
    return result

def state():
    return item('errorNotification').property('state')

def point(control):
    return control.mapToScene(control.boundingRect().center()).toPoint()

def push(text):
    evaluate('DialogErrorModel.pushError(' + json.dumps(text) + ')')
    QTest.qWait(100)
    assert state() == 'visible'

def move_out():
    QTest.mouseMove(view, QPoint(10, 10))
    QTest.qWait(100)

checks = []
move_out()
push('Recording failed')
QTest.qWait(5300)
assert state() == 'hidden' and evaluate('DialogErrorModel.repeatedMessageCount') == 0
checks.append('An unattended notice expires and resets its repeat count')

push('Recording failed again')
details = item('errorNotificationDetails')
QTest.mouseMove(view, point(details))
QTest.qWait(100)
assert item('errorNotification').property('interacting')
QTest.qWait(5300)
assert state() == 'visible'
QTest.mouseClick(view, Qt.LeftButton, Qt.NoModifier, point(details))
QTest.qWait(100)
assert state() == 'hidden' and evaluate('DialogsProvider.pages.join(",")') == '1'
assert evaluate('DialogErrorModel.repeatedMessageCount') == 0
checks.append('Hover holds the notice beyond five seconds; pointer Show Details opens the Errors page')

move_out()
push('Keyboard recording failure')
details = item('errorNotificationDetails')
item('backgroundFocus').forceActiveFocus(Qt.TabFocusReason)
QTest.keyClick(view, Qt.Key_Tab)
QTest.qWait(100)
assert details.property('activeFocus') and item('errorNotification').property('interacting')
QTest.qWait(5300)
assert state() == 'visible'
push('Another error while focused')
QTest.qWait(5300)
assert state() == 'visible' and details.property('activeFocus')
root.setProperty('simulateMessages', True)
QTest.keyClick(view, Qt.Key_Return)
QTest.qWait(100)
assert state() == 'hidden' and evaluate('DialogsProvider.pages.join(",")') == '1,1'
assert evaluate('DialogErrorModel.repeatedMessageCount') == 0
assert item('mockMessageFocus').property('activeFocus')
QTest.qWait(250)
assert item('mockMessageFocus').property('activeFocus')
checks.append('Keyboard focus and a repeated error hold the notice; Enter opens Errors without reclaiming focus')

item('mockMessageFocus').setProperty('visible', False)
root.setProperty('simulateMessages', False)
item('backgroundFocus').forceActiveFocus(Qt.OtherFocusReason)
move_out()
push('New error after Show Details')
assert not item('errorNotificationDetails').property('activeFocus')
assert item('backgroundFocus').property('activeFocus')
QTest.keyClick(view, Qt.Key_Tab)
assert item('errorNotificationDetails').property('activeFocus')
QTest.keyClick(view, Qt.Key_Return)
QTest.qWait(100)
assert state() == 'hidden' and item('backgroundFocus').property('activeFocus')
assert evaluate('DialogsProvider.pages.join(",")') == '1,1,1'
push('Another error after Show Details')
assert not item('errorNotificationDetails').property('activeFocus')
assert item('backgroundFocus').property('activeFocus')
QTest.qWait(5300)
assert state() == 'hidden'
checks.append('Show Details restores previous focus; a later notice does not steal focus and expires normally')

move_out()
push('Hover departure')
details = item('errorNotificationDetails')
item('backgroundFocus').forceActiveFocus(Qt.TabFocusReason)
QTest.qWait(100)
assert not details.property('activeFocus')
QTest.mouseMove(view, point(details))
QTest.qWait(100)
assert item('errorNotification').property('interacting')
move_out()
QTest.qWait(4000)
assert state() == 'visible'
QTest.qWait(1300)
assert state() == 'hidden'
checks.append('Leaving the notice starts a fresh five-second dismissal period')

push('Dismiss notice')
dismiss = item('errorNotificationDismiss')
item('backgroundFocus').forceActiveFocus(Qt.TabFocusReason)
QTest.keyClick(view, Qt.Key_Tab)
QTest.keyClick(view, Qt.Key_Tab)
QTest.qWait(100)
assert dismiss.property('activeFocus')
QTest.keyClick(view, Qt.Key_Space)
QTest.qWait(100)
assert state() == 'hidden' and evaluate('DialogErrorModel.repeatedMessageCount') == 0
assert evaluate('DialogsProvider.pages.join(",")') == '1,1,1'
assert item('backgroundFocus').property('activeFocus')
checks.append('Keyboard dismissal restores previous focus without opening Errors')

push('New error after Dismiss')
assert not item('errorNotificationDetails').property('activeFocus')
assert not dismiss.property('activeFocus')
assert item('backgroundFocus').property('activeFocus')
QTest.qWait(5300)
assert state() == 'hidden'
checks.append('A new notice after Dismiss does not steal focus and expires normally')

push('Previous control became unavailable')
item('backgroundFocus').forceActiveFocus(Qt.TabFocusReason)
QTest.keyClick(view, Qt.Key_Tab)
QTest.qWait(100)
assert item('errorNotificationDetails').property('activeFocus')
item('backgroundFocus').setProperty('enabled', False)
QTest.keyClick(view, Qt.Key_Return)
QTest.qWait(150)
assert state() == 'hidden' and not item('errorNotification').property('activeFocus')
push('New error after unavailable focus target')
assert not item('errorNotificationDetails').property('activeFocus')
assert not item('errorNotification').property('interacting')
QTest.qWait(5300)
assert state() == 'hidden'
push('Previous focus is the ancestor scope')
assert item('errorNotification').property('previousFocusItem') == view.rootObject()
item('errorNotificationDetails').forceActiveFocus(Qt.TabFocusReason)
QTest.qWait(100)
assert item('errorNotificationDetails').property('activeFocus')
QTest.keyClick(view, Qt.Key_Return)
QTest.qWait(100)
assert state() == 'hidden' and not item('errorNotification').property('activeFocus')
push('New error after ancestor scope focus')
assert not item('errorNotificationDetails').property('activeFocus')
assert not item('errorNotification').property('interacting')
item('errorNotificationDismiss').forceActiveFocus(Qt.TabFocusReason)
QTest.keyClick(view, Qt.Key_Space)
QTest.qWait(100)
assert state() == 'hidden' and not item('errorNotification').property('activeFocus')
checks.append('A previous ancestor FocusScope cannot retain and reactivate the hidden notice')
item('backgroundFocus').setProperty('enabled', True)
item('backgroundFocus').forceActiveFocus(Qt.OtherFocusReason)
checks.append('An unavailable previous control clears the notice focus before returning to the ancestor scope')

push('Notice before another window opens')
item('backgroundFocus').forceActiveFocus(Qt.TabFocusReason)
QTest.keyClick(view, Qt.Key_Tab)
QTest.qWait(100)
assert item('errorNotificationDetails').property('activeFocus')
other_window = QQuickWindow()
other_window.resize(200, 100)
other_window.show()
other_window.requestActivate()
QTest.qWait(200)
assert other_window.isActive() and not view.isActive()
root.setProperty('showErrorNotifications', False)
QTest.qWait(150)
assert state() == 'hidden' and not item('errorNotification').property('focus')
assert other_window.isActive() and not view.isActive()
root.setProperty('showErrorNotifications', True)
other_window.close()
view.requestActivate()
QTest.qWait(150)
assert view.isActive()
push('New notice after returning to the player')
assert not item('errorNotificationDetails').property('activeFocus')
assert not item('errorNotification').property('interacting')
QTest.qWait(5300)
assert state() == 'hidden'
checks.append('Hiding while another window is active clears stale focus without activating the player')

push('Notifications disabled')
root.setProperty('showErrorNotifications', False)
QTest.qWait(100)
assert state() == 'hidden'
evaluate('DialogErrorModel.pushError("Disabled")')
QTest.qWait(100)
assert state() == 'hidden'
checks.append('Disabling optional notifications hides the notice and prevents new notices')

view.close()
assert not messages, messages
report = dict(passed=True, checks=checks, qtMessages=messages,
              sourceSha256={str(p.relative_to(repo)): sha256(p.read_bytes()).hexdigest() for p in [source, drawer]},
              scope='Production notification and DrawerExt with supplied style, button and dialog boundaries. Native details dialog, desktop focus and screen reader remain separate.')
write('verification.json', json.dumps(report, indent=2)+'\n')
print(json.dumps(report, indent=2))
