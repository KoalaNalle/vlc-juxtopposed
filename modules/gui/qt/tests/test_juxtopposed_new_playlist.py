"""Exercise production New Playlist QML with supplied controller state.
This checks presentation/actions, not native playlist saving or persistence.
Original visual design by Juxtopposed; GPLv2 or later test code.
"""
import argparse
import json
import os
from pathlib import Path
os.environ['QT_QPA_PLATFORM'] = 'offscreen'
os.environ['QT_QUICK_BACKEND'] = 'software'
os.environ['QML_DISABLE_DISK_CACHE'] = '1'
repo = Path(__file__).resolve().parents[4]
os.environ['QT_QPA_FONTDIR'] = str(repo / 'modules/gui/qt/pixmaps/juxtopposed')
from PySide6.QtCore import QObject, Property, Signal, Slot, Qt, QUrl, QEvent, qInstallMessageHandler
from PySide6.QtGui import QGuiApplication, QImage, QColor, QKeyEvent
from PySide6.QtQml import QQmlApplicationEngine, QQmlExpression, qmlContext
from PySide6.QtQuick import QQuickItem, QQuickWindow
from PySide6.QtTest import QSignalSpy, QTest

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output-dir', type=Path, default=Path('ui-previews/new-playlist'))
args = parser.parse_args()
args.output_dir.mkdir(parents=True, exist_ok=True)
messages = []
qInstallMessageHandler(lambda kind, context, message: messages.append(message))

class Controller(QObject):
    changed = Signal()
    finished = Signal(bool)
    def __init__(self):
        super().__init__()
        self._name, self._error, self._files = '', '', []
        self._thumbnail = QUrl()
        self._busy, self._available, self._partial = False, True, False
        self.saves, self.resets, self.moves, self.removals = 0, 0, [], []
    def set_name(self, name):
        self._name, self._error = name, ''
        self.changed.emit()
    def set_thumbnail(self, thumbnail):
        self._thumbnail = thumbnail
        self.changed.emit()
    name = Property(str, lambda s: s._name, set_name, notify=changed)
    thumbnail = Property(QUrl, lambda s: s._thumbnail, set_thumbnail, notify=changed)
    files = Property('QVariantList', lambda s: s._files, notify=changed)
    available = Property(bool, lambda s: s._available, notify=changed)
    busy = Property(bool, lambda s: s._busy, notify=changed)
    partialSave = Property(bool, lambda s: s._partial, notify=changed)
    savedPlaylist = Property('QVariantMap', lambda s: {'id': 73, 'type': 'playlist'}, notify=changed)
    error = Property(str, lambda s: s._error if s._available else 'Enable the media library to save playlists.', notify=changed)
    canSave = Property(bool, lambda s: bool(s._name.strip()) and s._available and not s._busy and not s._partial, notify=changed)
    @Slot()
    def reset(self):
        self.resets += 1
        self._name, self._error, self._files, self._thumbnail = '', '', [], QUrl()
        self._busy, self._partial = False, False
        self.changed.emit()
    @Slot()
    def save(self):
        self.saves += 1
        self._busy, self._error = True, ''
        self.changed.emit()
    @Slot('QVariantList', result=bool)
    def appendFiles(self, urls):
        if self._busy or self._partial:
            return False
        urls = [QUrl(url) for url in urls]
        if any(not url.isLocalFile() for url in urls):
            self._error = 'Select readable local files.'
            self.changed.emit()
            return False
        self._files += [{'url': url, 'title': url.fileName(), 'path': url.toLocalFile()} for url in urls]
        self.changed.emit()
        return True
    @Slot(int)
    def removeFile(self, index):
        self.removals.append(index)
        self._files.pop(index)
        self.changed.emit()
    @Slot(int, int)
    def moveFile(self, index, destination):
        self.moves.append((index, destination))
        self._files.insert(destination, self._files.pop(index))
        self.changed.emit()
    def complete(self, success, partial=False):
        self._busy, self._partial = False, partial
        self._error = '' if success else 'A playlist was saved incompletely. Open it before making another copy.' if partial else 'Could not save this playlist.'
        self.changed.emit()
        self.finished.emit(success)

app = QGuiApplication([])
controller = Controller()
engine = QQmlApplicationEngine()
engine.rootContext().setContextProperty('playlistController', controller)
qml_path = QUrl.fromLocalFile(str(repo / 'modules/gui/qt/dialogs/dialogs/qml')).toString()
asset_path = QUrl.fromLocalFile(str(repo / 'modules/gui/qt/pixmaps/juxtopposed') + '/').toString()
driver = f'''import QtQuick
import QtQuick.Controls.Basic as C
import "{qml_path}" as Dialogs
C.ApplicationWindow {{
    id: window
    visible: true; width: 1050; height: 760; color: "#111111"
    C.Button {{ id: opener; objectName: "opener"; text: "New Playlist"; x: 10; y: 10; onClicked: dialog.showDialog(opener) }}
    Dialogs.JuxtopposedNewPlaylistDialog {{ id: dialog; parent: C.Overlay.overlay; controller: playlistController; assetBase: "{asset_path}" }}
}}'''
engine.loadData(driver.encode(), QUrl.fromLocalFile(str(args.output_dir.resolve() / 'driver.qml')))
assert engine.rootObjects(), messages
window = engine.rootObjects()[0]
dialog = window.findChild(QObject, 'juxtopposedNewPlaylistDialog')
assert dialog, messages

def item(name):
    result = window.findChild(QObject, name)
    if result: return result
    pending = [window.contentItem(), dialog.property('contentItem')]
    while pending:
        candidate = pending.pop()
        if candidate.objectName() == name: return candidate
        pending.extend(candidate.childItems())
    raise AssertionError(name)

def click(name):
    obj = item(name)
    QTest.mouseClick(window, Qt.LeftButton, Qt.NoModifier, obj.mapToScene(obj.boundingRect().center()).toPoint())
    QTest.qWait(20)

def expression(code):
    expr = QQmlExpression(qmlContext(dialog), dialog, code)
    value, _ = expr.evaluate()
    assert not expr.hasError(), expr.error().toString()
    return value.toVariant() if hasattr(value, 'toVariant') else value

files_signal = QSignalSpy(dialog.filesRequested)
thumbnail_signal = QSignalSpy(dialog.thumbnailRequested)
playlist_signal = QSignalSpy(dialog.playlistRequested)
QTest.qWait(80)
click('opener')
assert dialog.property('visible') and item('newPlaylistName').hasActiveFocus()
assert not item('newPlaylistSave').property('enabled')
assert window.grabWindow().save(str(args.output_dir / 'new-playlist-empty-component.png'))
for character in 'Fav Episodes':
    modifiers = Qt.ShiftModifier if character.isupper() else Qt.NoModifier
    key = Qt.Key(ord(character.upper()))
    QGuiApplication.sendEvent(window, QKeyEvent(QEvent.KeyPress, key, modifiers, character))
    QGuiApplication.sendEvent(window, QKeyEvent(QEvent.KeyRelease, key, modifiers, character))
assert controller._name == 'Fav Episodes'
assert item('newPlaylistSave').property('enabled'), 'An empty named playlist must be valid'
QTest.keyClick(window, Qt.Key_Tab)
assert item('newPlaylistThumbnail').hasActiveFocus()
QTest.keyClick(window, Qt.Key_Tab)
assert item('newPlaylistSelectFiles').hasActiveFocus()
QTest.keyClick(window, Qt.Key_Return)
assert files_signal.count() == 1
click('newPlaylistThumbnail')
assert thumbnail_signal.count() == 1
cover_path = args.output_dir.resolve() / 'supplied-thumbnail.png'
cover = QImage(200, 200, QImage.Format_RGB32)
cover.fill(QColor('#74422e'))
assert cover.save(str(cover_path))
controller.set_thumbnail(QUrl.fromLocalFile(str(cover_path)))
QTest.qWait(25)
assert item('newPlaylistClearThumbnail').property('visible')
click('newPlaylistClearThumbnail')
assert controller._thumbnail.isEmpty()
assert expression('appendDroppedFiles(["file:///C:/fixtures/video.mp4", "file:///C:/fixtures/song.flac", "file:///C:/fixtures/video.mp4"])') is True
QTest.qWait(40)
assert [row['title'] for row in controller._files] == ['video.mp4', 'song.flac', 'video.mp4']
click('newPlaylistFiles')
assert files_signal.count() == 2
click('newPlaylistDown0')
assert controller.moves == [(0, 1)]
assert [row['title'] for row in controller._files] == ['song.flac', 'video.mp4', 'video.mp4']
assert expression('appendDroppedFiles(["https://example.invalid/stream"])') is False
assert len(controller._files) == 3 and item('newPlaylistError').property('visible')
click('newPlaylistRemove1')
assert controller.removals == [1]
controller.appendFiles([QUrl('file:///C:/fixtures/song.flac')])
controller.set_thumbnail(QUrl.fromLocalFile(str(cover_path)))
assert window.grabWindow().save(str(args.output_dir / 'new-playlist-selected-component.png'))
click('newPlaylistSave')
assert controller.saves == 1
assert not item('newPlaylistName').property('enabled') and not item('newPlaylistThumbnail').property('enabled')
assert not item('newPlaylistCancel').property('enabled') and not item('newPlaylistClose').property('enabled')
QTest.keyClick(window, Qt.Key_Escape)
assert dialog.property('visible')
controller.complete(False)
QTest.qWait(25)
assert dialog.property('visible') and item('newPlaylistError').property('visible') and item('newPlaylistSave').property('enabled')
assert controller._name == 'Fav Episodes' and len(controller._files) == 3
click('newPlaylistSave')
assert controller.saves == 2
controller.complete(False, partial=True)
QTest.qWait(25)
assert not item('newPlaylistName').property('enabled') and not item('newPlaylistFiles').property('enabled')
assert item('newPlaylistSave').property('text') == 'View playlist'
assert expression('appendDroppedFiles(["file:///C:/fixtures/extra.mp4"])') is False
click('newPlaylistSave')
assert controller.saves == 2 and playlist_signal.count() == 1
assert not dialog.property('visible') and item('opener').hasActiveFocus()
click('opener')
assert controller._name == '' and controller._files == [] and not controller._partial
controller.set_name('Keyboard save')
QTest.qWait(20)
QTest.keyClick(window, Qt.Key_Return, Qt.ControlModifier)
assert controller.saves == 3
controller.complete(True)
QTest.qWait(25)
assert playlist_signal.count() == 2 and not dialog.property('visible') and item('opener').hasActiveFocus()
geometry = []
for width, height, scale in [(1050, 760, 1), (640, 420, 1), (960, 600, 1.5), (1280, 720, 2)]:
    window.resize(width, height)
    dialog.setProperty('uiScale', scale)
    click('opener')
    QTest.qWait(40)
    assert dialog.property('width') <= width and dialog.property('height') <= height
    for name in ['newPlaylistClose', 'newPlaylistCancel', 'newPlaylistSave']:
        obj = item(name)
        point = obj.mapToScene(obj.boundingRect().topLeft())
        assert point.x() >= 0 and point.y() >= 0 and point.x() + obj.width() <= width + .5 and point.y() + obj.height() <= height + .5, (name, width, height, scale)
    QTest.keyClick(window, Qt.Key_Tab)
    assert item('newPlaylistThumbnail').hasActiveFocus()
    thumb = item('newPlaylistThumbnail')
    point = thumb.mapToScene(thumb.boundingRect().topLeft())
    footer_y = item('newPlaylistCancel').mapToScene(item('newPlaylistCancel').boundingRect().topLeft()).y()
    assert point.y() >= 0 and point.y() + thumb.height() <= footer_y, (width, height, scale, point.y(), footer_y)
    geometry.append({'width': width, 'height': height, 'scale': scale, 'dialogWidth': dialog.property('width'), 'dialogHeight': dialog.property('height')})
    if width == 640: assert window.grabWindow().save(str(args.output_dir / 'new-playlist-short-component.png'))
    QTest.keyClick(window, Qt.Key_Escape)
    QTest.qWait(20)
    assert not dialog.property('visible')
controller._available = False
controller.changed.emit()
click('opener')
assert not item('newPlaylistSave').property('enabled') and item('newPlaylistError').property('visible')
click('newPlaylistCancel')
controller._available = True
controller.changed.emit()
window.resize(1050, 760)
dialog.setProperty('uiScale', 1)
click('opener')
controller.set_name('Large selection')
controller.appendFiles([QUrl(f'file:///C:/fixtures/clip-{index}.mp4') for index in range(128)])
QTest.qWait(40)
selected = item('newPlaylistSelectedFiles')
pending, rendered = [selected], 0
while pending:
    candidate = pending.pop()
    rendered += candidate.objectName().startswith('newPlaylistRemove')
    pending.extend(candidate.childItems())
assert 0 < rendered < 50, rendered
expression('focusFile(127, Qt.TabFocusReason, true)')
QTest.qWait(30)
assert item('newPlaylistRemove127').hasActiveFocus()
expression('focusFile(0, Qt.TabFocusReason, false)')
QTest.qWait(30)
assert item('newPlaylistDown0').hasActiveFocus()
click('newPlaylistCancel')
assert not messages, messages
(args.output_dir / 'new-playlist-component-verification.json').write_text(json.dumps({
    'scope': 'Actual production QML, supplied controller state, offscreen software renderer. No native save/file-picker/restart proof.',
    'checks': ['required name and valid empty playlist', 'keyboard file-picker and thumbnail actions', 'ordered repeated mixed file display', 'dropped URL dispatch and rejection', 'reorder and remove file', 'thumbnail removal', 'busy close guard', 'failure retains edits and allows retry', 'partial-save freezes duplicate creation and exposes actual route', 'successful route and focus return', 'Ctrl+Enter', 'short/high-scale focus visibility', 'unavailable library'],
    'geometry': geometry, 'virtualizedSelection': {'suppliedCount': 128, 'renderedRows': rendered}, 'qmlMessages': messages
}, indent=2), encoding='utf-8')
print('New Playlist production-QML component checks passed; native save/restart acceptance is separate.')
