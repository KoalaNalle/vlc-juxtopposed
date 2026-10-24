"""Exercise the shell component's route, focus and state contracts.

Native VLC routes, folder indexing and whole-app fidelity require separate
Windows verification. Visual design and original artwork: Juxtopposed.
Test code licensed under GPLv2 or later.
"""
import argparse
import os
import tempfile
import shutil
from pathlib import Path

os.environ['QT_QPA_PLATFORM'] = 'offscreen'
os.environ['QT_QUICK_BACKEND'] = 'software'
os.environ['QML_DISABLE_DISK_CACHE'] = '1'
os.environ['QT_QPA_FONTDIR'] = str(Path(__file__).resolve().parents[4] / 'modules/gui/qt/pixmaps/juxtopposed')

from PySide6.QtCore import Qt, QUrl, QObject, qInstallMessageHandler
from PySide6.QtGui import QGuiApplication, QFontDatabase
from PySide6.QtQml import QQmlExpression, qmlContext
from PySide6.QtQuick import QQuickView
from PySide6.QtTest import QSignalSpy, QTest

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--qml-file', type=Path)
parser.add_argument('--output-dir', type=Path, default=Path('ui-previews/navigation'))
args = parser.parse_args()
args.output_dir.mkdir(parents=True, exist_ok=True)
repo = Path(__file__).resolve().parents[4]
messages = []
qInstallMessageHandler(lambda kind, context, message: messages.append(message))
app = QGuiApplication([])
from juxtopposed_palette_fixture import register_juxtopposed_palette
register_juxtopposed_palette()
icon_font_id = QFontDatabase.addApplicationFont(str(repo / 'modules/gui/qt/pixmaps/VLCIcons.ttf'))
assert icon_font_id >= 0
icon_family = QFontDatabase.applicationFontFamilies(icon_font_id)[0]
module = tempfile.TemporaryDirectory(prefix='vlc-navigation-widgets-')
widgets = Path(module.name) / 'VLC/Widgets'
widgets.mkdir(parents=True)
shutil.copyfile(repo / 'modules/gui/qt/widgets/qml/ImageExt.qml', widgets / 'ImageExt.qml')
(widgets / 'qmldir').write_text('module VLC.Widgets\nImageExt 1.0 ImageExt.qml\n', encoding='utf-8')
util = widgets.parent / 'Util'
util.mkdir()
(util / 'qmldir').write_text('module VLC.Util\nsingleton SVGColorImage 1.0 SVGColorImage.qml\n', encoding='utf-8')
(util / 'SVGColorImage.qml').write_text('pragma Singleton\nimport QtQml\nQtObject { function colorize(path) { return { any: function(colors) { return this }, uri: function() { return path } } } }\n', encoding='utf-8')
view = QQuickView()
view.engine().addImportPath(module.name)
view.setResizeMode(QQuickView.SizeRootObjectToView)
routes = [
    {'key': 'home', 'text': 'Home', 'icon': '\ue01f', 'uri': ['home'], 'enabled': True},
    {'key': 'video', 'text': 'Videos', 'icon': '\ue047', 'uri': ['video', 'all'], 'enabled': True},
    {'key': 'music', 'text': 'Music', 'icon': '\ue044', 'uri': ['music', 'tracks'], 'enabled': True},
    {'key': 'network', 'text': 'Network', 'icon': '\ue045', 'uri': ['network'], 'enabled': True},
    {'key': 'playlists', 'text': 'Playlists', 'icon': '\ue02f', 'uri': ['playlists'], 'enabled': True},
    {'key': 'recent', 'text': 'Recent', 'icon': '\ue01e', 'uri': ['recent'], 'enabled': True},
]
view.setInitialProperties({
    'assetBase': QUrl.fromLocalFile(str(repo / 'modules/gui/qt/pixmaps/juxtopposed') + '/'),
    'routes': routes, 'activeRoute': 'video', 'iconFontFamily': icon_family, 'preferencesIcon': '\ue039',
})
view.setSource(QUrl.fromLocalFile(str(args.qml_file or repo / 'modules/gui/qt/maininterface/qml/JuxtopposedNavigation.qml')))
assert not view.errors(), [error.toString() for error in view.errors()]
root = view.rootObject()
assert root
view.resize(50, 600)
view.show()
QTest.qWait(50)

def item(name):
    pending = [root]
    while pending:
        candidate = pending.pop()
        if candidate.objectName() == name:
            return candidate
        pending.extend(candidate.childItems())
    raise AssertionError(name)

def click(button):
    position = button.mapToScene(button.boundingRect().center()).toPoint()
    QTest.mouseClick(view, Qt.LeftButton, Qt.NoModifier, position)

def expression(code):
    evaluation = QQmlExpression(qmlContext(root), root, code)
    value, _ = evaluation.evaluate()
    assert not evaluation.hasError(), evaluation.error().toString()
    return value

spy = QSignalSpy(root.routeRequested)
for route in routes:
    button = item('juxtopposedNavigation' + route['key'])
    assert button.width() == 34 and button.height() == 32, "Original compact row must be 34 by 32 logical pixels"
    click(button)
    assert spy.count() == routes.index(route) + 1
    received = spy.at(spy.count() - 1)[0]
    if hasattr(received, 'toVariant'):
        received = received.toVariant()
    assert received == route['uri'], received

# History/engine state updates must not navigate a second time.
before = spy.count()
root.setProperty('activeRoute', 'music')
assert item('juxtopposedNavigationmusic').property('checked')
assert not item('juxtopposedNavigationvideo').property('checked')
assert spy.count() == before

video = item('juxtopposedNavigationvideo')
music = item('juxtopposedNavigationmusic')
network = item('juxtopposedNavigationnetwork')
video.forceActiveFocus()
QTest.keyClick(view, Qt.Key_Down)
assert music.hasActiveFocus(), 'Down navigates to Music'
QTest.keyClick(view, Qt.Key_Space)
assert spy.count() == before + 1, 'Keyboard route activation'
focus_spy = QSignalSpy(root.contentFocusRequested)
QTest.keyClick(view, Qt.Key_Right)
assert focus_spy.count() == 1, 'Right returns focus to content'

# Without a media library, dependent routes stay inactive while Network works.
unavailable = [dict(route, enabled=route['key'] in ('home', 'network')) for route in routes]
root.setProperty('routes', unavailable)
root.setProperty('libraryAvailable', False)
root.setProperty('expanded', True)
view.resize(217, 600)
QTest.qWait(20)
assert item('juxtopposedLibraryUnavailable').isVisible()
before = spy.count()
click(item('juxtopposedNavigationvideo'))
assert spy.count() == before
network = item('juxtopposedNavigationnetwork')
network.forceActiveFocus()
QTest.keyClick(view, Qt.Key_Down)
assert item('juxtopposedSidebarAdd').hasActiveFocus(), 'Unavailable routes are skipped before Add item'
QTest.keyClick(view, Qt.Key_Down)
assert item('juxtopposedNavigationhome').hasActiveFocus(), 'Add item wraps to Home'
QTest.keyClick(view, Qt.Key_Down)
assert network.hasActiveFocus(), 'Navigation wraps to enabled route'
click(network)
assert spy.count() == before + 1

root.setProperty('routes', routes)
root.setProperty('libraryAvailable', True)
for scale in (1, 1.5, 2):
    root.setProperty('uiScale', scale)
    for expanded, width in ((False, 50), (True, 217)):
        root.setProperty('expanded', expanded)
        view.resize(round(width * scale), round(600 * scale))
        QTest.qWait(30)
        assert root.property('implicitWidth') == width * scale
        button = item('juxtopposedNavigationvideo')
        assert abs(button.height() - 32 * scale) < 0.1
        assert button.width() > 0 and button.width() <= root.width()
        assert view.grabWindow().save(str(args.output_dir / f'layout-{width}-{scale}.png'))

root.setProperty('libraryBusy', True)
assert item('juxtopposedLibraryScan').isVisible()
root.setProperty('libraryBusy', False)
assert not item('juxtopposedLibraryScan').isVisible()
assert not messages, messages
print('Passed: route payloads, active state, keyboard/focus, unavailable-library paths, scanning and scaled shell layouts.')
print('Native routes/indexing and original navigation icon fidelity remain separate acceptance gates.')
