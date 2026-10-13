"""Check library presentation contracts separately from native VLC models.
Original design/artwork: Juxtopposed. Test code: GPLv2 or later.
"""
import argparse
import os
import re
from pathlib import Path
repo = Path(__file__).resolve().parents[4]
os.environ['QT_QPA_PLATFORM'] = 'offscreen'
os.environ['QT_QUICK_BACKEND'] = 'software'
os.environ['QML_DISABLE_DISK_CACHE'] = '1'
os.environ['QT_QPA_FONTDIR'] = str(repo / 'modules/gui/qt/pixmaps/juxtopposed')
from PySide6.QtCore import Qt, QUrl, QPoint, qInstallMessageHandler
from PySide6.QtGui import QGuiApplication, QFontDatabase
from PySide6.QtQml import QQmlExpression, qmlContext
from PySide6.QtQuick import QQuickView
from PySide6.QtTest import QSignalSpy, QTest

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output-dir', type=Path, default=Path('ui-previews/library'))
args = parser.parse_args()
args.output_dir.mkdir(parents=True, exist_ok=True)
messages = []
qInstallMessageHandler(lambda kind, context, message: messages.append(message))
app = QGuiApplication([])
font_id = QFontDatabase.addApplicationFont(str(repo / 'modules/gui/qt/pixmaps/VLCIcons.ttf'))
assert font_id >= 0
asset_base = QUrl.fromLocalFile(str(repo / 'modules/gui/qt/pixmaps/juxtopposed') + '/')
icon_source = (repo / 'modules/gui/qt/style/vlcicons.hpp').read_text(encoding='utf-8')
def icon(name):
    match = re.search(r"QChar " + name + r"\{ u'\\u([a-f0-9]{4})' \}", icon_source)
    assert match, name
    return chr(int(match[1], 16))

def load(name, properties, width, height):
    view = QQuickView()
    view.setResizeMode(QQuickView.SizeRootObjectToView)
    view.setInitialProperties(dict(properties, assetBase=asset_base))
    view.setSource(QUrl.fromLocalFile(str(repo / 'modules/gui/qt/widgets/qml' / name)))
    assert not view.errors(), [e.toString() for e in view.errors()]
    view.resize(width, height)
    view.show()
    QTest.qWait(50)
    return view, view.rootObject()

def find(root, name):
    pending = [root]
    while pending:
        item = pending.pop()
        if item.objectName() == name:
            return item
        pending.extend(item.childItems())
    raise AssertionError(name)

def click(view, item, modifiers=Qt.NoModifier):
    QTest.mouseClick(view, Qt.LeftButton, modifiers, item.mapToScene(item.boundingRect().center()).toPoint())

def expression(view, root, code):
    expr = QQmlExpression(qmlContext(root), root, code)
    value, _ = expr.evaluate()
    assert not expr.hasError(), expr.error().toString()
    return value.toVariant() if hasattr(value, 'toVariant') else value

header_view, header = load('JuxtopposedLibraryHeader.qml', {
    'breadcrumbs': [{'text': 'Videos', 'uri': ['video']}, {'text': 'Fixture folder', 'uri': ['video', 'all', 'group']}],
    'itemCount': 15, 'countAvailable': True, 'canGoBack': True,
    'sortAvailable': True, 'viewAvailable': True, 'searchAvailable': True,
    'iconFontFamily': QFontDatabase.applicationFontFamilies(font_id)[0],
    'backIcon': icon('back'), 'viewIcon': icon('list'), 'searchIcon': icon('search'), 'moreIcon': icon('more'),
}, 1200, 72)
back = QSignalSpy(header.backRequested)
forward = QSignalSpy(header.forwardRequested)
expression(header_view, header, 'setCurrentItemFocus(Qt.TabFocusReason)')
assert find(header, 'libraryBack').hasActiveFocus()
header.setProperty('canGoBack', False)
expression(header_view, header, 'setCurrentItemFocus(Qt.TabFocusReason)')
assert find(header, 'libraryBreadcrumb0').hasActiveFocus()
header.setProperty('canGoBack', True)
click(header_view, find(header, 'libraryBack'))
assert back.count() == 1
click(header_view, find(header, 'libraryForward'))
assert forward.count() == 0
header.setProperty('canGoForward', True)
click(header_view, find(header, 'libraryForward'))
assert forward.count() == 1
crumb = QSignalSpy(header.breadcrumbRequested)
click(header_view, find(header, 'libraryBreadcrumb0'))
received = crumb.at(0)[0]
assert (received.toVariant() if hasattr(received, 'toVariant') else received) == ['video']
for button_name, signal_name in [('librarySort', 'sortRequested'), ('libraryView', 'viewRequested'), ('libraryMore', 'moreRequested')]:
    spy = QSignalSpy(getattr(header, signal_name))
    click(header_view, find(header, button_name))
    assert spy.count() == 1, button_name

search = QSignalSpy(header.searchRequested)
header.searchRequested.connect(lambda value: header.setProperty('searchPattern', value))
header.setProperty('searchPattern', 'Waveform')
QTest.qWait(20)
assert search.count() == 0, 'Model synchronization must not trigger a search action'
assert find(header, 'librarySearch').property('text') == 'Waveform'
click(header_view, find(header, 'librarySearchClear'))
assert search.count() == 1 and search.at(0)[0] == ''
QTest.keyClick(header_view, Qt.Key_A)
assert search.count() == 2 and search.at(1)[0] == 'a'
header_view.resize(500, 72)
QTest.qWait(30)
click(header_view, find(header, 'librarySearchToggle'))
assert find(header, 'librarySearch').isVisible()
focus = QSignalSpy(header.contentFocusRequested)
QTest.keyClick(header_view, Qt.Key_Escape)
assert focus.count() == 1 and not header.property('searchExpanded')

for width, scale in [(1200, 1), (500, 1), (1800, 1.5), (1000, 2)]:
    header.setProperty('uiScale', scale)
    header_view.resize(width, round(72 * scale))
    QTest.qWait(30)
    assert header_view.grabWindow().save(str(args.output_dir / f'header-{width}-{scale}.png'))
    # All visible controls must fit even when the final breadcrumb elides.
    for name in ['libraryBack', 'libraryForward', 'libraryBreadcrumb0', 'libraryBreadcrumb1',
                 'librarySort', 'libraryView', 'librarySearch', 'librarySearchToggle', 'libraryMore']:
        control = find(header, name)
        if control.isVisible():
            pos = control.mapToScene(QPoint(0, 0))
            assert pos.x() >= 0 and pos.x() + control.width() <= header.width(), name

row_view, row = load('JuxtopposedMediaRow.qml', {'title': 'Generated chaptered video', 'durationText': '00:00:18', 'showDuration': True}, 380, 44)
selected = QSignalSpy(row.selectionRequested)
activated = QSignalSpy(row.activationRequested)
click(row_view, row, Qt.ControlModifier)
assert selected.count() == 1 and selected.at(0)[0] & Qt.ControlModifier.value
row.forceActiveFocus()
QTest.keyClick(row_view, Qt.Key_Return)
assert activated.count() == 1 and activated.at(0)[0] is True
context = QSignalSpy(row.contextRequested)
QTest.mouseDClick(row_view, Qt.RightButton, Qt.NoModifier, QPoint(80, 22))
assert context.count() >= 1 and activated.count() == 1, 'Right double-click must not play'
QTest.qWait(app.styleHints().mouseDoubleClickInterval() + 20)
QTest.mouseDClick(row_view, Qt.LeftButton, Qt.NoModifier, QPoint(80, 22))
assert activated.count() == 2 and activated.at(1)[0] is False, 'Left double-click activates the item'
drag = QSignalSpy(row.dragRequested)
QTest.mousePress(row_view, Qt.LeftButton, Qt.ControlModifier, QPoint(80, 22))
QTest.mouseMove(row_view, QPoint(130, 22), 20)
QTest.mouseMove(row_view, QPoint(160, 22), 20)
QTest.mouseRelease(row_view, Qt.LeftButton, Qt.ControlModifier, QPoint(160, 22))
assert drag.count() >= 2 and drag.at(0)[0] is True and drag.at(drag.count() - 1)[0] is False
row.setProperty('current', True)
QTest.qWait(20)
assert find(row, 'libraryRowTitle').property('color').name() == '#d37918'
for width, scale in [(380, 1), (180, 1), (570, 1.5), (760, 2)]:
    row.setProperty('uiScale', scale)
    row.setProperty('title', 'Long generated video title that should elide without covering the duration or thumbnail')
    row_view.resize(width, round(44 * scale))
    QTest.qWait(30)
    title = find(row, 'libraryRowTitle')
    assert title.width() > 0 and title.x() + title.width() <= row.width()
    assert row_view.grabWindow().save(str(args.output_dir / f'row-{width}-{scale}.png'))

# Use the actual music adapter's destination declarations; the backend remains
# a separate native gate. Assert their known real routes rather than tautologies.
music_source = (repo / 'modules/gui/qt/medialibrary/qml/MusicCategoriesDisplay.qml').read_text(encoding='utf-8')
routes_source = re.search(r'\broutes:\s*(\[[\s\S]*?\n        \])', music_source)[1]
header_view.rootContext().setContextProperty('VLCIcons', {
    name: icon(name) for name in ['topbar_music', 'album_cover', 'visualization']
})
routes = expression(header_view, header, routes_source)
expected_routes = [['music', name] for name in ['tracks', 'albums', 'artists', 'genres']]
assert [route['uri'] for route in routes] == expected_routes
category_view, categories = load('JuxtopposedCategoryList.qml', {
    'routes': routes, 'iconFontFamily': QFontDatabase.applicationFontFamilies(font_id)[0], 'addIcon': icon('add')
}, 380, 300)
add = QSignalSpy(categories.addRequested)
route = QSignalSpy(categories.routeRequested)
click(category_view, find(categories, 'musicCategoryAdd'))
assert add.count() == 1
for name, destination in zip(['tracks', 'albums', 'artists', 'genres'], expected_routes):
    click(category_view, find(categories, 'musicCategory' + name))
    received = route.at(route.count() - 1)[0]
    assert (received.toVariant() if hasattr(received, 'toVariant') else received) == destination
categories.setProperty('addAvailable', False)
expression(category_view, categories, 'setCurrentItemFocus(Qt.TabFocusReason)')
assert find(categories, 'musicCategorytracks').hasActiveFocus()
QTest.keyClick(category_view, Qt.Key_Down)
assert find(categories, 'musicCategoryalbums').hasActiveFocus()
QTest.keyClick(category_view, Qt.Key_Return)
assert route.count() == 5
for width, height, scale in [(380, 300, 1), (200, 100, 1), (570, 450, 1.5), (760, 600, 2)]:
    categories.setProperty('uiScale', scale)
    category_view.resize(width, height)
    QTest.qWait(30)
    assert category_view.grabWindow().save(str(args.output_dir / f'categories-{width}-{scale}.png'))
assert not messages, messages
print('Passed: header/history/search focus, row selection/activation/drag/current state, music category destinations and scaled layouts.')
print('Actual native library adapters and whole-app visual acceptance remain separate gates.')
