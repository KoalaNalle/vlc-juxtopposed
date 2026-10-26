# SPDX-License-Identifier: GPL-2.0-or-later
"""Production conversion QML with supplied state; native conversion is separate."""
import argparse
from enum import IntEnum
import hashlib
import json
import os
from pathlib import Path
import subprocess

os.environ['QT_QPA_PLATFORM'] = 'offscreen'
os.environ['QT_QUICK_BACKEND'] = 'software'
os.environ['QML_DISABLE_DISK_CACHE'] = '1'
repo = Path(__file__).resolve().parents[4]
os.environ['QT_QPA_FONTDIR'] = str(repo / 'modules/gui/qt/pixmaps/juxtopposed')
import PySide6
from PySide6.QtCore import QObject, Property, Signal, Slot, QEnum, Qt, QUrl, QPointF, qInstallMessageHandler
from PySide6.QtGui import QGuiApplication
from PySide6.QtQml import QQmlApplicationEngine, QQmlExpression, qmlContext, qmlRegisterSingletonInstance, qmlRegisterUncreatableType
from PySide6.QtQuick import QQuickItem, QQuickWindow
from PySide6.QtTest import QTest

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output-dir', type=Path, required=True)
args = parser.parse_args()
args.output_dir.mkdir(parents=True, exist_ok=False)
messages = []
qInstallMessageHandler(lambda kind, context, message: messages.append(message))

class Controller(QObject):
    class State(IntEnum):
        Idle, Starting, Encoding, Validating, Publishing, Cancelling, Succeeded, Failed, Cancelled = range(9)
    QEnum(State)
    changed = Signal()
    def __init__(self):
        super().__init__()
        self._source, self._destination, self._output = QUrl(), QUrl(), QUrl()
        self._state, self._progress, self._error = self.State.Idle, -1., ''
        self._time, self._duration = 0, 0
        self.starts, self.resets, self.cancels, self.source_changes, self.destination_changes = 0, 0, 0, 0, 0
        self.publication_won = False
        self.next_start_error = ''
    def is_busy(self):
        return self._state in [self.State.Starting, self.State.Encoding, self.State.Validating, self.State.Publishing, self.State.Cancelling]
    def set_source(self, value):
        if self.is_busy(): return
        self._source = value
        self.source_changes += 1
        self.reset()
    def set_destination(self, value):
        if self.is_busy(): return
        self._destination = value
        self.destination_changes += 1
        self.reset()
    sourceUrl = Property(QUrl, lambda s: s._source, set_source, notify=changed)
    destinationUrl = Property(QUrl, lambda s: s._destination, set_destination, notify=changed)
    outputUrl = Property(QUrl, lambda s: s._output, notify=changed)
    profileName = Property(str, lambda s: 'Audio - FLAC', constant=True)
    state = Property(int, lambda s: int(s._state), notify=changed)
    busy = Property(bool, is_busy, notify=changed)
    canStart = Property(bool, lambda s: not s.is_busy() and not s._source.isEmpty() and not s._destination.isEmpty(), notify=changed)
    progress = Property(float, lambda s: s._progress, notify=changed)
    error = Property(str, lambda s: s._error, notify=changed)
    sourceTimeMs = Property(int, lambda s: s._time, notify=changed)
    sourceDurationMs = Property(int, lambda s: s._duration, notify=changed)
    @Slot()
    def reset(self):
        if self.is_busy(): return
        self.resets += 1
        self._state, self._progress, self._error, self._output = self.State.Idle, -1., '', QUrl()
        self._time, self._duration = 0, 0
        self.changed.emit()
    @Slot(result=bool)
    def start(self):
        if self.is_busy(): return False
        self.starts += 1
        if self.next_start_error:
            # Local-file validation can fail synchronously, before a worker
            # starts. This has a different layout/focus order from its result.
            self._state, self._error = self.State.Failed, self.next_start_error
            self.next_start_error = ''
            self.changed.emit()
            return False
        self._state, self._error, self._output = self.State.Starting, '', QUrl()
        self.changed.emit()
        return True
    @Slot()
    def cancel(self):
        if not self.is_busy(): return
        self.cancels += 1
        if self.publication_won: return
        self._state = self.State.Cancelling
        self.changed.emit()
    def phase(self, state, progress=-1.):
        self._state, self._progress = state, progress
        self._time, self._duration = 1000, 2000
        self.changed.emit()
    def finish(self, state, error=''):
        self._state, self._error = state, error
        self._output = self._destination if state == self.State.Succeeded else QUrl()
        self._progress = 1. if state == self.State.Succeeded else self._progress
        self.changed.emit()

class ShortcutState(QObject):
    changed = Signal()
    useGlobalShortcuts = Property(bool, lambda s: True, notify=changed)

class DialogState(QObject):
    changed = Signal()
    def __init__(self):
        super().__init__()
        self._visible = False
    def set_visible(self, value):
        self._visible = value
        self.changed.emit()
    qmlDialogVisible = Property(bool, lambda s: s._visible, set_visible, notify=changed)
    qmlInputFocused = Property(bool, lambda s: False, notify=changed)

app = QGuiApplication([])
from juxtopposed_palette_fixture import register_juxtopposed_palette
register_juxtopposed_palette()
controller, shortcut_state, dialog_state = Controller(), ShortcutState(), DialogState()
qmlRegisterUncreatableType(Controller, 'VLC.Dialogs', 1, 0, 'ConversionController', 'Supplied-state controller enum')
qmlRegisterSingletonInstance(DialogState, 'VLC.Dialogs', 1, 0, 'DialogsProvider', dialog_state)
qmlRegisterSingletonInstance(ShortcutState, 'VLC.MainInterface', 1, 0, 'MainCtx', shortcut_state)
engine = QQmlApplicationEngine()
engine.rootContext().setContextProperty('conversionController', controller)
qml_dir = repo / 'modules/gui/qt/dialogs/dialogs/qml'
driver = f'''import QtQuick
import QtQuick.Controls.Basic as C
import VLC.Dialogs
import "{QUrl.fromLocalFile(str(qml_dir)).toString()}" as Dialogs
import "{QUrl.fromLocalFile(str(repo / 'modules/gui/qt/menus/qml')).toString()}" as Menus
C.ApplicationWindow {{
    id: window
    visible: true; width: 1050; height: 760; color: "#111111"
    property int sourceRequests: 0
    property int destinationRequests: 0
    property int profileRequests: 0
    property int transportShortcuts: 0
    Menus.ShortcutExt {{ objectName: "transportShortcut"; sequence: "Space"; onActivated: window.transportShortcuts++ }}
    C.Button {{ id: opener; objectName: "opener"; text: "Convert"; x: 10; y: 10; onClicked: dialog.showDialog(opener) }}
    Dialogs.JuxtopposedConversionDialog {{ id: dialog; parent: C.Overlay.overlay; controller: conversionController
        assetBase: "{QUrl.fromLocalFile(str(repo / 'modules/gui/qt/pixmaps/juxtopposed') + '/').toString()}"
        onSourceRequested: window.sourceRequests++
        onDestinationRequested: window.destinationRequests++
        onAllProfilesRequested: window.profileRequests++
    }}
    Binding {{ target: DialogsProvider; property: "qmlDialogVisible"; value: dialog.visible; restoreMode: Binding.RestoreBindingOrValue }}
}}'''
engine.loadData(driver.encode(), QUrl.fromLocalFile(str(args.output_dir.resolve() / 'driver.qml')))
assert engine.rootObjects(), messages
window = engine.rootObjects()[0]
dialog = window.findChild(QObject, 'juxtopposedConversionDialog')
assert dialog, messages

def item(name):
    result = window.findChild(QObject, name)
    assert result, name
    return result

def click(name):
    obj = item(name)
    QTest.mouseClick(window, Qt.LeftButton, Qt.NoModifier, obj.mapToScene(obj.boundingRect().center()).toPoint())
    QTest.qWait(20)

def evaluate(code):
    expression = QQmlExpression(qmlContext(window), window, code)
    value, _ = expression.evaluate()
    assert not expression.hasError(), expression.error().toString()
    QTest.qWait(20)
    return value

def screenshot(name):
    assert window.grabWindow().save(str(args.output_dir / name))

QTest.qWait(100)
click('opener')
assert dialog.property('visible') and item('conversionSourceBrowse').hasActiveFocus()
assert not item('conversionConvert').property('enabled') and controller.starts == 0
assert not item('transportShortcut').property('enabled'), 'Transport shortcut escaped modal'
item('conversionSource').forceActiveFocus(Qt.TabFocusReason)
QTest.keyClick(window, Qt.Key_Space)
assert window.property('transportShortcuts') == 0
screenshot('conversion-empty-component.png')
click('conversionSourceBrowse')
click('conversionDestinationBrowse')
assert window.property('sourceRequests') == 1 and window.property('destinationRequests') == 1
source = QUrl.fromLocalFile("C:/Audio/quoted 'source, ä.wav")
destination = QUrl.fromLocalFile("C:/Audio/quoted 'output, ä.flac")
controller.set_source(source)
controller.set_destination(destination)
QTest.qWait(20)
assert item('conversionSource').property('text') == source.toLocalFile()
assert item('conversionDestination').property('text') == destination.toLocalFile()
assert item('conversionSource').property('readOnly') and item('conversionDestination').property('readOnly')
assert item('conversionProfile').property('text') == 'Audio - FLAC'
assert item('conversionConvert').property('enabled')
QTest.keyClick(window, Qt.Key_Return, Qt.ControlModifier)
assert controller.starts == 1 and controller.is_busy()
assert item('conversionCancel').hasActiveFocus()
for name in ['conversionSource', 'conversionSourceBrowse', 'conversionDestination', 'conversionDestinationBrowse', 'conversionAllProfiles', 'conversionConvert']:
    assert not item(name).property('enabled'), name
resets = controller.resets
evaluate('dialog.showDialog(opener)')
assert controller.resets == resets and controller.starts == 1, 'Opening a running job reset/restarted it'
assert dialog.property('visible') and not item('transportShortcut').property('enabled')
controller.phase(controller.State.Encoding, .5)
QTest.qWait(20)
assert item('conversionProgress').property('visible') and item('conversionProgress').property('value') == .5
assert item('conversionTiming').property('text') == '50% · 0:01 / 0:02'
screenshot('conversion-encoding-component.png')
for phase, text in [(controller.State.Validating, 'Checking the converted audio…'), (controller.State.Publishing, 'Saving the converted file…')]:
    controller.phase(phase)
    QTest.qWait(20)
    assert item('conversionStatus').property('text') == text
    assert not item('conversionProgress').property('visible') and item('conversionBusy').property('visible')
controller.finish(controller.State.Failed, 'The destination already exists. Choose a new filename.')
QTest.qWait(30)
assert dialog.property('visible') and item('conversionError').property('visible')
assert item('conversionError').property('text') == controller._error
assert item('conversionSource').property('text') == source.toLocalFile()
assert item('conversionDestination').property('text') == destination.toLocalFile()
assert item('conversionConvert').hasActiveFocus() and item('conversionConvert').property('enabled')
screenshot('conversion-error-component.png')
click('conversionConvert')
assert controller.starts == 2
click('conversionCancel')
assert controller.cancels == 1 and dialog.property('visible')
assert not dialog.property('closeAfterCancel') and not item('conversionCancel').property('enabled')
controller.finish(controller.State.Cancelled)
QTest.qWait(20)
assert dialog.property('visible') and item('conversionStatus').property('text') == 'Conversion cancelled'
click('conversionConvert')
assert controller.starts == 3
QTest.keyClick(window, Qt.Key_Escape)
QTest.qWait(20)
assert controller.cancels == 2 and dialog.property('visible') and dialog.property('closeAfterCancel')
assert not item('transportShortcut').property('enabled'), 'Shortcuts restored before worker completion'
controller.finish(controller.State.Cancelled)
QTest.qWait(20)
assert not dialog.property('visible') and item('opener').hasActiveFocus()
assert item('transportShortcut').property('enabled')

click('opener')
assert controller._source == source and controller._destination == destination, 'Reopening lost chosen files'
click('conversionConvert')
controller.phase(controller.State.Encoding, .999)
QTest.qWait(20)
assert item('conversionTiming').property('text').startswith('99%'), 'Unpublished output was shown as 100%'
controller.finish(controller.State.Succeeded)
QTest.qWait(30)
assert dialog.property('visible') and item('conversionStatus').property('text') == 'Conversion complete'
assert item('conversionOutput').property('visible') and item('conversionOutput').property('text') == destination.toLocalFile()
assert item('conversionConvert').property('text') == 'Done'
screenshot('conversion-complete-component.png')
click('conversionConvert')
assert not dialog.property('visible') and item('opener').hasActiveFocus()
click('opener')
click('conversionAllProfiles')
assert window.property('profileRequests') == 1 and not dialog.property('visible'), 'Native all-profile route lost'

geometry = []
terminal_geometry = []
for width, height, scale in [(1050, 760, 1), (602, 421, 1), (640, 360, 1), (960, 600, 1.5), (1280, 720, 2)]:
    window.resize(width, height)
    dialog.setProperty('uiScale', scale)
    click('opener')
    QTest.qWait(30)
    assert dialog.property('width') <= width and dialog.property('height') <= height
    for name in ['conversionClose', 'conversionCancel', 'conversionConvert']:
        obj = item(name)
        point = obj.mapToScene(QPointF(0, 0))
        assert point.x() >= 0 and point.y() >= 0 and point.x() + obj.width() <= width + .5 and point.y() + obj.height() <= height + .5, (name, width, height, scale)
    for name in ['conversionSource', 'conversionDestination', 'conversionAllProfiles']:
        obj = item(name)
        obj.forceActiveFocus(Qt.TabFocusReason)
        QTest.qWait(25)
        # TextField.boundingRect() is its padded text bounds. Measure the full
        # control from its actual origin when checking fixed-footer clipping.
        point = obj.mapToScene(QPointF(0, 0))
        footer_top = dialog.property('footer').mapToScene(QPointF(0, 0)).y()
        assert obj.hasActiveFocus() and point.y() >= dialog.property('y') + 50 * scale - .5
        if point.y() + obj.height() > footer_top + .5:
            screenshot('conversion-focus-failure-component.png')
            scroll = item('conversionScroll')
            flick = scroll.property('contentItem')
            footer = dialog.property('footer')
            print('Focus diagnostic:', name, 'fieldHeight', obj.height(), 'scroll', scroll.y(), scroll.height(), 'flick', flick.y(), flick.height(),
                'contentY', flick.property('contentY'), 'contentHeight', flick.property('contentHeight'), 'footer', footer.y(), footer.height(),
                'footerSceneY', footer.mapToScene(footer.boundingRect().topLeft()).y(), 'assertedFooterTop', footer_top)
            print('QML messages:', messages)
        assert point.y() + obj.height() <= footer_top + .5, ('Focused control clipped by fixed footer', name, width, height, scale, point.y(), footer_top)
    # A shorter second error previously settled below the fixed footer after
    # the first reveal. Measure the actual text bounds after each layout, not
    # just its visible property, including the synchronous validation path.
    for number, error in enumerate([
        'The audio could not be converted. Check the source and available codecs.',
        'Choose a readable local audio file in a supported format.',
        'Could not create conversion storage in the destination folder. Choose a writable folder and a new filename.',
    ]):
        item('conversionSourceBrowse').forceActiveFocus(Qt.TabFocusReason)
        controller.set_source(source)
        QTest.qWait(30)
        if number == 0:
            click('conversionConvert')
            controller.finish(controller.State.Failed, error)
        else:
            controller.next_start_error = error
            click('conversionConvert')
        QTest.qWait(60)
        assert item('conversionConvert').hasActiveFocus()
        footer_top = dialog.property('footer').mapToScene(QPointF(0, 0)).y()
        for name in ['conversionStatus', 'conversionError']:
            obj = item(name)
            point = obj.mapToScene(QPointF(0, 0))
            bounds = {'window': [width, height], 'scale': scale, 'errorCase': number,
                'item': name, 'top': point.y(), 'bottom': point.y() + obj.height(), 'footerTop': footer_top}
            terminal_geometry.append(bounds)
            if point.y() < dialog.property('y') + 50 * scale - .5 or point.y() + obj.height() > footer_top + .5:
                screenshot('conversion-terminal-clipped-component.png')
                print('Terminal geometry:', bounds, 'QML messages:', messages)
            assert obj.property('visible') and point.y() >= dialog.property('y') + 50 * scale - .5, bounds
            assert point.y() + obj.height() <= footer_top + .5, ('Terminal result clipped', bounds)
        if width == 602:
            screenshot(f'conversion-short-error-{number}-component.png')
    geometry.append({'window': [width, height], 'scale': scale, 'dialog': [dialog.property('width'), dialog.property('height')]})
    if width == 640: screenshot('conversion-short-component.png')
    QTest.keyClick(window, Qt.Key_Escape)
    QTest.qWait(20)
    assert not dialog.property('visible') and item('opener').hasActiveFocus()

window.resize(1050, 760)
dialog.setProperty('uiScale', 1)
click('opener')
click('conversionConvert')
controller.publication_won = True
click('conversionClose')
assert dialog.property('visible') and controller.is_busy(), 'Close detached a publishing worker'
controller.finish(controller.State.Succeeded)
QTest.qWait(30)
assert not dialog.property('visible') and item('opener').hasActiveFocus()
assert controller._output == destination
assert window.property('transportShortcuts') == 0
errors = [message for message in messages if any(word in message.lower() for word in ['error', 'cannot', 'unable', 'binding loop'])]
assert not errors, errors
qmlformat = Path(PySide6.__file__).parent / ('qmlformat.exe' if os.name == 'nt' else 'qmlformat')
result = subprocess.run([str(qmlformat), '--ignore-settings', str(qml_dir / 'JuxtopposedConversionDialog.qml')], stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True, timeout=20)
assert result.returncode == 0, result.stderr
verification = {'passed': True, 'geometry': geometry, 'terminalGeometry': terminal_geometry, 'messages': messages,
    'starts': controller.starts, 'cancelRequests': controller.cancels,
    'nativePickerDispatch': {'source': window.property('sourceRequests'), 'destination': window.property('destinationRequests'), 'allProfiles': window.property('profileRequests')},
    'sourceQmlSha256': hashlib.sha256((qml_dir / 'JuxtopposedConversionDialog.qml').read_bytes()).hexdigest(),
    'scope': 'Production conversion QML, supplied controller state, offscreen software render. Picker signals, state/focus/layout/shortcut behavior only; no native conversion, primary playback, desktop picker or original Figma form acceptance.',
    'adaptation': True, 'fullConversionOrDesignAcceptance': False}
(args.output_dir / 'verification.json').write_text(json.dumps(verification, indent=2) + '\n', encoding='utf-8', newline='\n')
print('Passed production conversion QML: file-picker signals, real-controller state contract, phase progress/errors, retry/cancel, delayed close, successful output, native-profile dispatch, modal shortcut guard, focus return and short/scaled layouts.')
print('Supplied-state component checks only; native conversion/picker/primary/Figma acceptance remains separate.')
