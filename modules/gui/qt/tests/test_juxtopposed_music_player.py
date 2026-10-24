"""Check the production music presentation with supplied media-model data.
Native filesystem/metadata/queue playback and RHI blur remain separate gates.
Original design/artwork: Juxtopposed. Test code: GPLv2 or later.
"""
import argparse
import json
import os
import re
import shutil
import tempfile
from pathlib import Path

repo = Path(__file__).resolve().parents[4]
os.environ['QT_QPA_PLATFORM'] = 'offscreen'
os.environ['QT_QUICK_BACKEND'] = 'software'
os.environ['QML_DISABLE_DISK_CACHE'] = '1'
os.environ['QT_QPA_FONTDIR'] = str(repo / 'modules/gui/qt/pixmaps/juxtopposed')
from PySide6.QtCore import QAbstractListModel, QModelIndex, QPointF, Qt, QUrl, qInstallMessageHandler
from PySide6.QtGui import QColor, QFontDatabase, QGuiApplication, QImage, QPainter
from PySide6.QtQml import QQmlExpression, qmlContext
from PySide6.QtQuick import QQuickView
from PySide6.QtTest import QSignalSpy, QTest

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output-dir', type=Path, default=Path('ui-previews/music'))
args = parser.parse_args()
args.output_dir.mkdir(parents=True, exist_ok=True)
messages = []
qInstallMessageHandler(lambda kind, context, message: messages.append(message))
app = QGuiApplication([])
from juxtopposed_palette_fixture import register_juxtopposed_palette
register_juxtopposed_palette()
font_id = QFontDatabase.addApplicationFont(str(repo / 'modules/gui/qt/pixmaps/VLCIcons.ttf'))
assert font_id >= 0
asset_base = QUrl.fromLocalFile(str(repo / 'modules/gui/qt/pixmaps/juxtopposed') + '/')
icon_source = (repo / 'modules/gui/qt/style/vlcicons.hpp').read_text(encoding='utf-8')

def icon(name):
    match = re.search(r"QChar " + name + r"\{ u'\\u([a-f0-9]{4})' \}", icon_source)
    assert match, name
    return chr(int(match[1], 16))

class MediaModel(QAbstractListModel):
    names = ['title', 'artist', 'artwork', 'url']
    def __init__(self, rows):
        super().__init__()
        self.rows = rows
    def rowCount(self, parent=QModelIndex()):
        return 0 if parent.isValid() else len(self.rows)
    def roleNames(self):
        return {Qt.UserRole + i + 1: name.encode() for i, name in enumerate(self.names)}
    def data(self, index, role):
        if not index.isValid() or role < Qt.UserRole + 1:
            return None
        return self.rows[index.row()][self.names[role - Qt.UserRole - 1]]
    def replace(self, rows):
        self.beginResetModel()
        self.rows = rows
        self.endResetModel()

def find(root, name):
    pending = [root]
    while pending:
        item = pending.pop()
        if item.objectName() == name:
            return item
        pending.extend(item.childItems())
    raise AssertionError(name)

def expression(root, code):
    expr = QQmlExpression(qmlContext(root), root, code)
    result, _ = expr.evaluate()
    assert not expr.hasError(), expr.error().toString()
    return result.toVariant() if hasattr(result, 'toVariant') else result

def click(view, item):
    QTest.mouseClick(view, Qt.LeftButton, Qt.NoModifier, item.mapToScene(item.boundingRect().center()).toPoint())

# Copy the actual QML modules; no replacement implementation for artwork/rows/nav.
with tempfile.TemporaryDirectory(prefix='music-scene-') as directory:
    path = Path(directory)
    for module, sources in {'Widgets': ['ImageExt', 'JuxtopposedMediaRow'],
                            'MainInterface': ['JuxtopposedNavigation']}.items():
        target = path / 'VLC' / module
        target.mkdir(parents=True)
        entries = ['module VLC.' + module]
        for name in sources:
            original = repo / 'modules/gui/qt' / ('widgets' if module == 'Widgets' else 'maininterface') / 'qml' / (name + '.qml')
            shutil.copyfile(original, target / original.name)
            entries.append(name + ' 1.0 ' + original.name)
        (target / 'qmldir').write_text('\n'.join(entries) + '\n', encoding='utf-8', newline='\n')
    # SVG colorization is a native C++ singleton. Supply the same component-test
    # adapter used by the standalone navigation checks; native artwork fidelity
    # is covered separately by the compiled application journeys.
    util = path / 'VLC' / 'Util'
    util.mkdir()
    (util / 'qmldir').write_text('module VLC.Util\nsingleton SVGColorImage 1.0 SVGColorImage.qml\n', encoding='utf-8')
    (util / 'SVGColorImage.qml').write_text('pragma Singleton\nimport QtQml\nQtObject { function colorize(path) { return { any: function(colors) { return this }, uri: function() { return path } } } }\n', encoding='utf-8')
    # Generated fixture artwork, intentionally unrelated to the author's cover.
    cover_path = path / 'fixture-cover.png'
    image = QImage(500, 500, QImage.Format_ARGB32)
    image.fill(QColor('#26394b'))
    painter = QPainter(image)
    painter.fillRect(50, 50, 200, 200, QColor('#de881a'))
    painter.fillRect(250, 250, 200, 200, QColor('#74b39c'))
    painter.end()
    assert image.save(str(cover_path))
    cover = QUrl.fromLocalFile(str(cover_path))
    rows = [dict(title='Generated first track', artist='Fixture artist', artwork=cover, url=QUrl('file:///first.wav')),
            dict(title='Generated second track', artist='', artwork=QUrl(), url=QUrl('file:///second.wav')),
            dict(title='Generated third track', artist='Different artist', artwork=QUrl(), url=QUrl('file:///third.wav'))]
    model = MediaModel(rows)
    view = QQuickView()
    view.setColor(QColor('#000000'))
    view.engine().addImportPath(str(path))
    view.setResizeMode(QQuickView.SizeRootObjectToView)
    view.setInitialProperties(dict(assetBase=asset_base, artwork=cover, fallbackArtwork=cover,
        sourceModel=model, itemCount=3, currentIndex=0, folderAvailable=True,
        folderName='Generated source folder', title='Generated first track', artist='Fixture artist',
        iconFontFamily=QFontDatabase.applicationFontFamilies(font_id)[0],
        backIcon=icon('back'), moreIcon=icon('more'), addIcon=icon('add'),
        searchIcon=icon('search'), preferencesIcon=icon('settings'),
        navigationRoutes=[dict(key='music', text='Music', icon=icon('topbar_music'), uri=['music'])]))
    view.setSource(QUrl.fromLocalFile(str(repo / 'modules/gui/qt/player/qml/JuxtopposedMusicPlayer.qml')))
    assert not view.errors(), [error.toString() for error in view.errors()]
    view.resize(2048, 1056)
    view.show()
    QTest.qWait(150)
    root = view.rootObject()
    assert root is not None
    activation = QSignalSpy(root.activationRequested)
    add = QSignalSpy(root.addRequested)
    back = QSignalSpy(root.backRequested)
    search = QSignalSpy(root.searchRequested)
    sort = QSignalSpy(root.sortRequested)
    controls = QSignalSpy(root.controlsFocusRequested)
    row = find(root, 'musicSourceRow1')
    click(view, row)
    assert activation.count() == 0, 'Selection does not start a track'
    QTest.keyClick(view, Qt.Key_Return)
    assert activation.count() == 1 and activation.at(0)[0] == 1
    QTest.keyClick(view, Qt.Key_Down)
    assert find(root, 'musicSourceRow2').hasActiveFocus()
    QTest.keyClick(view, Qt.Key_Return)
    assert activation.count() == 2 and activation.at(1)[0] == 2
    QTest.keyClick(view, Qt.Key_Right)
    assert controls.count() == 1
    click(view, find(root, 'musicAdd'))
    assert add.count() == 1
    click(view, find(root, 'musicBack'))
    assert back.count() == 1
    click(view, find(root, 'musicFolderSort'))
    assert sort.count() == 1
    click(view, find(root, 'musicSearchToggle'))
    field = find(root, 'musicSearch')
    assert field.hasActiveFocus()
    QTest.keyClick(view, Qt.Key_A)
    assert search.count() == 1 and search.at(0)[0] == 'a'
    QTest.keyClick(view, Qt.Key_Escape)
    assert search.at(search.count() - 1)[0] == ''
    root.setProperty('currentIndex', 2)
    root.setProperty('title', 'Updated title')
    root.setProperty('artist', 'Updated artist')
    QTest.qWait(30)
    assert find(root, 'musicTrackLabel').property('text') == 'Updated title - Updated artist'
    assert find(find(root, 'musicSourceRow2'), 'libraryRowTitle').property('color').name() == '#d37918'
    root.setProperty('displayFolder', False)
    assert not root.property('folderMode')
    assert not find(root, 'musicFolderSort').isVisible()
    root.setProperty('displayFolder', True)
    root.setProperty('busy', True)
    QTest.qWait(20)
    assert find(root, 'musicSourceStatus').property('text') == 'Reading audio metadata…'
    root.setProperty('busy', False)
    root.setProperty('error', 'The selected file is no longer readable.')
    assert find(root, 'musicSourceStatus').property('text') == 'The selected file is no longer readable.'
    root.setProperty('error', '')
    results = []
    for width, height, scale in [(2048, 1056, 1), (1100, 620, 1), (640, 420, 1), (1440, 900, 1.5), (1280, 840, 2)]:
        root.setProperty('uiScale', scale)
        view.resize(width, height)
        QTest.qWait(60)
        frame = find(root, 'musicCoverFrame')
        panel = find(root, 'musicSourcePanel')
        label = find(root, 'musicTrackLabel')
        frame_position = frame.mapToScene(QPointF(0, 0))
        assert frame.width() > 0 and abs(frame.width() - frame.height()) < .1
        assert frame_position.x() >= panel.x() + panel.width() + 23 * scale
        assert frame_position.x() + frame.width() <= root.width() - 15 * scale
        label_position = label.mapToScene(QPointF(0, 0))
        assert label_position.y() + label.height() <= root.height()
        if width == 2048:
            assert abs(frame_position.x() + frame.width() / 2 - width / 2) < 1
            assert 780 < frame.width() < 800
        filename = f'music-{width}-{height}-{scale}.png'
        assert view.grabWindow().save(str(args.output_dir / filename))
        results.append(dict(width=width, height=height, scale=scale,
            coverSize=frame.width(), coverX=frame_position.x(), coverY=frame_position.y()))
    model.replace([])
    root.setProperty('itemCount', 0)
    root.setProperty('currentIndex', -1)
    root.setProperty('folderAvailable', False)
    root.setProperty('error', 'A readable local audio file is needed to browse its source folder.')
    expression(root, 'setCurrentItemFocus(Qt.TabFocusReason)')
    assert find(root, 'musicAdd').hasActiveFocus()
    assert find(root, 'musicSourceStatus').property('text').startswith('A readable local audio')
    assert not messages, messages
    (args.output_dir / 'music-player-verification.json').write_text(json.dumps(dict(
        component='JuxtopposedMusicPlayer', layouts=results,
        nativeFilesystemMetadataAndPlayback=False, nativeRhiBlurAndRounding=False), indent=2) + '\n', encoding='utf-8', newline='\n')
    view.close()
    view.setSource(QUrl())
    app.processEvents()
print('Passed: source list selection/keyboard activation, current-track metadata, Add/back/search/sort, queue fallback, loading/error/empty states and scaled square-cover geometry.')
print('Native source-folder discovery/metadata/playback, RHI blur/rounding and whole-app acceptance remain separate gates.')
