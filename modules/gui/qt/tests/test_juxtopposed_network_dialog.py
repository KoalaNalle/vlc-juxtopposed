# SPDX-License-Identifier: GPL-2.0-or-later
# Original design by Juxtopposed; supplied-controller component checks only.
import argparse
import json
import os
from pathlib import Path
import subprocess
os.environ['QT_QPA_PLATFORM'] = 'offscreen'
os.environ['QT_QUICK_BACKEND'] = 'software'
os.environ['QML_DISABLE_DISK_CACHE'] = '1'
os.environ['QT_QPA_FONTDIR'] = str(Path(__file__).resolve().parents[4] / 'modules/gui/qt/pixmaps/juxtopposed')
import PySide6
from PySide6.QtCore import QObject, Property, Signal, Slot, Qt, QUrl, QEvent, qInstallMessageHandler
from PySide6.QtGui import QGuiApplication, QKeyEvent
from PySide6.QtQml import QQmlApplicationEngine, qmlRegisterSingletonInstance
from PySide6.QtQuick import QQuickItem, QQuickWindow
from PySide6.QtTest import QTest

repo = Path(__file__).resolve().parents[4]
parser = argparse.ArgumentParser(description='Construct and test the production network stream dialog.')
parser.add_argument('--output-dir', type=Path, default=Path('ui-previews/network'))
args = parser.parse_args()
args.output_dir.mkdir(parents=True, exist_ok=True)
messages = []
qInstallMessageHandler(lambda kind, context, message: messages.append(message))


class Controller(QObject):
    stateChanged = Signal()
    finished = Signal(bool)
    def __init__(self):
        super().__init__()
        self._text, self._name, self._error = '', '', ''
        self._busy, self._available = False, True
        self.submits, self.resets = 0, 0
    def set_text(self, text):
        self._text, self._error = text, ''
        self.stateChanged.emit()
    def set_name(self, name):
        self._name, self._error = name, ''
        self.stateChanged.emit()
    urlText = Property(str, lambda s: s._text, set_text, notify=stateChanged)
    playlistName = Property(str, lambda s: s._name, set_name, notify=stateChanged)
    error = Property(str, lambda s: s._error, notify=stateChanged)
    busy = Property(bool, lambda s: s._busy, notify=stateChanged)
    playlistAvailable = Property(bool, lambda s: s._available, notify=stateChanged)
    canSubmit = Property(bool, lambda s: bool(s._text.strip()) and not s._busy, notify=stateChanged)
    @Slot()
    def reset(self):
        self.resets += 1
        self._text, self._name, self._error = '', '', ''
        self.stateChanged.emit()
    @Slot()
    def submit(self):
        self.submits += 1
        self._busy = True
        self._error = ''
        self.stateChanged.emit()
    def complete(self, success):
        self._busy = False
        self._error = '' if success else 'Could not save this playlist.'
        self.stateChanged.emit()
        self.finished.emit(success)


class ShortcutState(QObject):
    changed = Signal()
    def __init__(self):
        super().__init__()
        self.allowed = True
    useGlobalShortcuts = Property(bool, lambda s: s.allowed, notify=changed)


class DialogState(QObject):
    changed = Signal()
    def __init__(self):
        super().__init__()
        self.visible = False
        self.input_focused = False
    def set_visible(self, visible):
        self.visible = visible
        self.changed.emit()
    qmlDialogVisible = Property(bool, lambda s: s.visible, set_visible, notify=changed)
    def set_input_focused(self, focused):
        self.input_focused = focused
        self.changed.emit()
    qmlInputFocused = Property(bool, lambda s: s.input_focused, set_input_focused, notify=changed)


app = QGuiApplication([])
from juxtopposed_palette_fixture import register_juxtopposed_palette
register_juxtopposed_palette()
shortcut_state, dialog_state = ShortcutState(), DialogState()
qmlRegisterSingletonInstance(ShortcutState, 'VLC.MainInterface', 1, 0, 'MainCtx', shortcut_state)
qmlRegisterSingletonInstance(DialogState, 'VLC.Dialogs', 1, 0, 'DialogsProvider', dialog_state)
controller = Controller()
engine = QQmlApplicationEngine()
engine.rootContext().setContextProperty('networkController', controller)
qml_dir = repo / 'modules/gui/qt/dialogs/dialogs/qml'
driver = f'''
import QtQuick
import QtQuick.Controls.Basic as C
import VLC.MainInterface
import VLC.Dialogs
import "{QUrl.fromLocalFile(str(repo / 'modules/gui/qt/menus/qml')).toString()}" as Menus
import "{QUrl.fromLocalFile(str(qml_dir)).toString()}" as Network
import "{QUrl.fromLocalFile(str(repo / 'modules/gui/qt/widgets/qml')).toString()}" as Widgets
C.ApplicationWindow {{
    id: testWindow
    visible: true; width: 1100; height: 700; color: "#111111"
    property int interceptedPaste: 0
    Menus.ShortcutExt {{ objectName: "conflictingShortcut"; sequence: StandardKey.Paste
        onActivated: testWindow.interceptedPaste++
    }}
    C.Button {{ id: opener; objectName: "opener"; text: "Open stream"; x: 10; y: 10
        onClicked: dialog.showDialog(opener)
    }}
    C.TextField {{ objectName: "outsideEditor"; x: 200; y: 10; width: 180 }}
    C.TextArea {{ objectName: "outsideMultiline"; x: 400; y: 10; width: 180; height: 60 }}
    C.Button {{ objectName: "outsideButton"; text: "Return focus"; x: 600; y: 10 }}
    Widgets.TextInputFocus {{ id: inputFocus; focusItem: testWindow.activeFocusItem }}
    Binding {{ target: DialogsProvider; property: "qmlInputFocused"; value: inputFocus.editing }}
    Network.JuxtopposedNetworkDialog {{ id: dialog; parent: C.Overlay.overlay
        controller: networkController
        assetBase: "{QUrl.fromLocalFile(str(repo / 'modules/gui/qt/pixmaps/juxtopposed') + '/').toString()}"
    }}
    Binding {{ target: DialogsProvider; property: "qmlDialogVisible"; value: dialog.visible
        restoreMode: Binding.RestoreBindingOrValue
    }}
}}
'''
engine.loadData(driver.encode(), QUrl.fromLocalFile(str(args.output_dir.resolve() / 'driver.qml')))
assert engine.rootObjects(), messages
window = engine.rootObjects()[0]
dialog = window.findChild(QObject, 'juxtopposedNetworkDialog')
assert dialog, messages


def item(name):
    found = window.findChild(QObject, name)
    assert found, name
    return found


def center(obj):
    return obj.mapToScene(obj.boundingRect().center()).toPoint()


def click(name):
    QTest.mouseClick(window, Qt.LeftButton, Qt.NoModifier, center(item(name)))
    QTest.qWait(20)


def type_text(text):
    for character in text:
        modifiers = Qt.ShiftModifier if character.isupper() else Qt.NoModifier
        key = Qt.Key(ord(character.upper()))
        QGuiApplication.sendEvent(window, QKeyEvent(QEvent.KeyPress, key, modifiers, character))
        QGuiApplication.sendEvent(window, QKeyEvent(QEvent.KeyRelease, key, modifiers, character))


QTest.qWait(100)
click('opener')
urls, name = item('networkUrls'), item('networkPlaylistName')
assert dialog.property('visible') and urls.hasActiveFocus(), 'Open/focus'
assert not item('networkSave').property('enabled'), 'Empty Save must be disabled'
assert controller.submits == 0
shortcut_state.allowed = True
shortcut_state.changed.emit() # Another editor losing focus must not unblock it.
QTest.qWait(15)
assert not item('conflictingShortcut').property('enabled'), 'Application shortcut escaped the modal guard'
assert window.grabWindow().save(str(args.output_dir / 'network-dialog-empty.png'))
type_text('https://example.invalid/first')
QTest.keyClick(window, Qt.Key_Return)
type_text('https://example.invalid/second')
assert controller._text == 'https://example.invalid/first\nhttps://example.invalid/second', controller._text
assert controller.submits == 0, 'Plain Enter in URL field must add a line'
QTest.keyClick(window, Qt.Key_Tab)
assert name.hasActiveFocus(), 'Tab to optional name'
type_text('Favorite TV Channels')
assert controller._name == 'Favorite TV Channels', (controller._name, name.property('text'), messages[-4:])
assert item('networkSave').property('enabled')
before = controller.submits
click('networkSave')
assert controller.submits == before + 1
assert not urls.property('enabled') and not name.property('enabled')
assert not item('networkCancel').property('enabled') and not item('networkClose').property('enabled')
QTest.keyClick(window, Qt.Key_Escape)
assert dialog.property('visible'), 'Pending save closed'
controller.complete(False)
QTest.qWait(30)
assert dialog.property('visible') and item('networkError').property('visible')
assert urls.property('text') == controller._text and name.property('text') == controller._name, 'Failure lost fields'
assert item('networkSave').property('enabled'), 'Recoverable failure did not allow retry'
click('networkSave')
assert controller.submits == before + 2, 'Retry activation'
controller.complete(False)
QTest.qWait(20)
click('networkCancel')
assert not dialog.property('visible') and item('opener').hasActiveFocus(), 'Cancel/focus return'
assert item('conflictingShortcut').property('enabled'), 'Shortcut guard did not restore'
click('opener')
assert urls.property('text') == '' and name.property('text') == '', 'Reopen/reset'
controller.set_text('https://example.invalid/keyboard')
QTest.qWait(20)
QTest.keyClick(window, Qt.Key_Return, Qt.ControlModifier)
assert controller.submits == before + 3, 'Ctrl+Enter submit'
controller.complete(True)
QTest.qWait(30)
assert not dialog.property('visible') and item('opener').hasActiveFocus(), 'Save/focus return'

geometry = []
for width, height, scale in [(1100, 700, 1), (640, 420, 1), (960, 600, 1.5), (1280, 720, 2)]:
    window.resize(width, height)
    dialog.setProperty('uiScale', scale)
    click('opener')
    QTest.qWait(40)
    assert dialog.property('width') <= width and dialog.property('height') <= height
    for element in ['networkClose', 'networkCancel', 'networkSave']:
        obj = item(element)
        p = obj.mapToScene(obj.boundingRect().topLeft())
        assert p.x() >= 0 and p.y() >= 0 and p.x() + obj.width() <= width + .5 and p.y() + obj.height() <= height + .5, (element, width, height, scale)
    QTest.keyClick(window, Qt.Key_Tab)
    assert name.hasActiveFocus(), 'Narrow keyboard focus'
    p = name.mapToScene(name.boundingRect().topLeft())
    footer_y = item('networkCancel').mapToScene(item('networkCancel').boundingRect().topLeft()).y()
    assert p.y() >= 0 and p.y() + name.height() <= footer_y, ('Focused field clipped', width, height, scale, p.y(), footer_y)
    geometry.append({'width': width, 'height': height, 'scale': scale,
                     'dialogWidth': dialog.property('width'), 'dialogHeight': dialog.property('height')})
    if width == 640:
        assert window.grabWindow().save(str(args.output_dir / 'network-dialog-short.png'))
    QTest.keyClick(window, Qt.Key_Escape)
    QTest.qWait(20)
    assert not dialog.property('visible')
controller._available = False
controller.stateChanged.emit()
click('opener')
assert not name.property('enabled')
QTest.keyClick(window, Qt.Key_Tab)
assert item('networkCancel').hasActiveFocus(), 'Unavailable name was not skipped'
click('networkCancel')
window.resize(1100, 700)
QGuiApplication.clipboard().setText('chaptered')
for editor_name in ['outsideEditor', 'outsideMultiline']:
    click(editor_name)
    assert item(editor_name).hasActiveFocus()
    assert not item('conflictingShortcut').property('enabled'), 'Editable focus left application Paste enabled'
    QTest.keyClick(window, Qt.Key_V, Qt.ControlModifier)
    QTest.qWait(20)
    assert item(editor_name).property('text') == 'chaptered', 'Paste escaped the editor'
    assert window.property('interceptedPaste') == 0
click('outsideButton')
assert item('conflictingShortcut').property('enabled'), 'Application shortcut stayed disabled after leaving the editor'
QTest.keyClick(window, Qt.Key_V, Qt.ControlModifier)
QTest.qWait(20)
assert window.property('interceptedPaste') == 1, 'Non-editor shortcut was not restored'
errors = [m for m in messages if any(word in m.lower() for word in ['error', 'cannot', 'unable', 'binding loop'])]
assert not errors, errors
qmlformat = Path(PySide6.__file__).parent / ('qmlformat.exe' if os.name == 'nt' else 'qmlformat')
for source in [qml_dir / 'JuxtopposedNetworkDialog.qml', repo / 'modules/gui/qt/maininterface/qml/MainInterface.qml']:
    result = subprocess.run([str(qmlformat), '--ignore-settings', str(source)], stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True)
    assert result.returncode == 0, result.stderr
(args.output_dir / 'network-dialog-verification.json').write_text(json.dumps({'geometry': geometry, 'messages': messages, 'submits': controller.submits, 'resets': controller.resets}, indent=2) + '\n', newline='\n')
print('Passed: production dialog construction, multiline entry, optional name, Save/keyboard, busy/error/reset/focus return, unavailable library, short/scaled layouts and editable-focus Paste suppression/restoration.')
print('Actual native queue/HTTP/persistence and desktop acceptance are separate checks.')
