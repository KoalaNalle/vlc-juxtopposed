"""Check actual Home QML with supplied models; this is not native ML evidence.
Original design/artwork: Juxtopposed. Test code: GPLv2 or later.
"""
import argparse
import json
import os
import re
import tempfile
import shutil
from pathlib import Path

repo = Path(__file__).resolve().parents[4]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--repo-root', type=Path, default=repo)
parser.add_argument('--output-dir', type=Path, default=Path('ui-previews/home'))
args = parser.parse_args()
repo = args.repo_root.resolve()
os.environ['QT_QPA_PLATFORM'] = 'offscreen'
os.environ['QT_QUICK_BACKEND'] = 'software'
os.environ['QML_DISABLE_DISK_CACHE'] = '1'
os.environ['QT_QPA_FONTDIR'] = str(repo / 'modules/gui/qt/pixmaps/juxtopposed')
from PySide6.QtCore import QAbstractListModel, QModelIndex, Property, Qt, QUrl, qInstallMessageHandler
from PySide6.QtGui import QGuiApplication, QFontDatabase, QImage, QColor
from PySide6.QtQml import QQmlComponent, QQmlExpression, qmlContext
from PySide6.QtQuick import QQuickItem, QQuickView
from PySide6.QtTest import QSignalSpy, QTest

args.output_dir.mkdir(parents=True, exist_ok=True)
messages = []
qInstallMessageHandler(lambda kind, context, message: messages.append(message))
app = QGuiApplication([])
from juxtopposed_palette_fixture import register_juxtopposed_palette
register_juxtopposed_palette()
font_id = QFontDatabase.addApplicationFont(str(repo / 'modules/gui/qt/pixmaps/VLCIcons.ttf'))
assert font_id >= 0
font_family = QFontDatabase.applicationFontFamilies(font_id)[0]
asset_base = QUrl.fromLocalFile(str(repo / 'modules/gui/qt/pixmaps/juxtopposed') + '/')
icons_source = (repo / 'modules/gui/qt/style/vlcicons.hpp').read_text(encoding='utf-8')
def icon(name):
    match = re.search(r"QChar " + name + r"\{ u'\\u([a-f0-9]{4})' \}", icons_source)
    assert match, name
    return chr(int(match[1], 16))

class SuppliedMediaModel(QAbstractListModel):
    def __init__(self, rows):
        super().__init__()
        self.rows = rows
        keys = sorted({key for row in rows for key in row})
        self.roles = {Qt.UserRole + i: key for i, key in enumerate(keys)}
    def rowCount(self, parent=QModelIndex()):
        return 0 if parent.isValid() else len(self.rows)
    def roleNames(self):
        return {role: key.encode() for role, key in self.roles.items()}
    def data(self, index, role=Qt.DisplayRole):
        if not index.isValid() or index.row() >= len(self.rows):
            return None
        return self.rows[index.row()].get(self.roles.get(role, ''))

# Use the production ImageExt dependency; the software renderer exercises its
# documented Image fallback, not the native RHI rounded-image shader.
widget_module = tempfile.TemporaryDirectory(prefix='vlc-home-widgets-')
widget_dir = Path(widget_module.name) / 'VLC/Widgets'
widget_dir.mkdir(parents=True)
shutil.copyfile(repo / 'modules/gui/qt/widgets/qml/ImageExt.qml', widget_dir / 'ImageExt.qml')
(widget_dir / 'qmldir').write_text('module VLC.Widgets\nImageExt 1.0 ImageExt.qml\n', encoding='utf-8')

def load(name, properties, width, height):
    view = QQuickView()
    view.engine().addImportPath(widget_module.name)
    view.setResizeMode(QQuickView.SizeRootObjectToView)
    view.setInitialProperties(dict(properties, assetBase=asset_base, iconFontFamily=font_family))
    view.setSource(QUrl.fromLocalFile(str(repo / 'modules/gui/qt/medialibrary/qml' / name)))
    assert not view.errors(), [error.toString() for error in view.errors()]
    view.resize(width, height)
    view.show()
    QTest.qWait(80)
    return view, view.rootObject()

def find(root, name):
    pending = [root]
    while pending:
        item = pending.pop()
        if item.objectName() == name:
            return item
        pending.extend(item.childItems())
    raise AssertionError(name)

def expr(root, code):
    expression = QQmlExpression(qmlContext(root), root, code)
    value, _ = expression.evaluate()
    assert not expression.hasError(), expression.error().toString()
    return value.toVariant() if hasattr(value, 'toVariant') else value

def entry_id(value):
    if hasattr(value, 'toVariant'):
        value = value.toVariant()
    return value.get('id') if isinstance(value, dict) else value.property('id')

def click(view, item):
    QTest.mouseClick(view, Qt.LeftButton, Qt.NoModifier, item.mapToScene(item.boundingRect().center()).toPoint())

def context_click(view, item):
    QTest.mouseClick(view, Qt.RightButton, Qt.NoModifier, item.mapToScene(item.boundingRect().center()).toPoint())

def context_key(view, item, key=Qt.Key_F10, modifiers=Qt.ShiftModifier):
    item.forceActiveFocus(Qt.TabFocusReason)
    QTest.keyClick(view, key, modifiers)

art_path = args.output_dir / 'supplied-cover.png'
art = QImage(400, 400, QImage.Format_RGB32)
art.fill(QColor('#22554b'))
assert art.save(str(art_path))
art_uri = QUrl.fromLocalFile(str(art_path.resolve())).toString()
video_model = SuppliedMediaModel([
    {'id': 'video-12', 'title': 'RÃ©sumÃ©', 'fileName': 'RÃ©sumÃ©.mp4', 'small_cover': art_uri, 'progress': .4},
    {'id': 'video-8', 'title': 'Second', 'fileName': 'Second.mp4', 'small_cover': art_uri, 'progress': .2},
    {'id': 'video-31', 'title': 'Third', 'fileName': 'Third.mp4', 'small_cover': art_uri, 'progress': .1},
    {'id': 'video-17', 'title': 'Fourth', 'fileName': 'Fourth.mp4', 'small_cover': art_uri, 'progress': .2},
])
audio_model = SuppliedMediaModel([
    {'id': 'audio-44', 'title': 'Moonlight', 'main_artist': 'Artist Î±', 'small_cover': art_uri},
    {'id': 'audio-29', 'title': 'Black bird', 'main_artist': 'Artist Î²', 'small_cover': art_uri},
    {'id': 'audio-72', 'title': 'Untangle', 'main_artist': 'Artist Î±', 'small_cover': art_uri},
])
watched_model = SuppliedMediaModel([
    {'id': 'watched-1', 'title': 'Finished film', 'fileName': 'Finished.mp4', 'small_cover': art_uri, 'progress': -1},
])
folders = [{'id': 'folder-21', 'title': 'Shows'}, {'id': 'folder-22', 'title': 'Music'}, {'id': 'folder-23', 'title': 'Mixed media'}]
playlists = [{'id': 'playlist-56', 'title': 'Fav Episodes'}, {'id': 'playlist-83', 'title': 'Coolest Songs'}]
artists = [{'id': 'artist-15', 'title': 'Artist Î±', 'artwork': art_uri}]
properties = {
    'videoEntries': video_model, 'audioEntries': audio_model,
    'watchAgainEntries': watched_model,
    'folderEntries': folders, 'playlistEntries': playlists, 'artistEntries': artists,
    'arrowIcon': icon('ic_fluent_chevron_right_24'), 'videoIcon': icon('topbar_video'),
    'audioIcon': icon('topbar_music'), 'folderIcon': icon('topbar_discover'),
    'playlistIcon': icon('playlist'), 'artistIcon': icon('topbar_music'),
}
view, root = load('JuxtopposedHomeDashboard.qml', properties, 1700, 970)
# Exercise the same reactive sorting adapter imported by the three native
# Home pages. Default history stays recent-first; explicit menu order applies.
sorting_url = QUrl.fromLocalFile(str(repo / 'modules/gui/qt/medialibrary/qml/JuxtopposedHomeSorting.js')).toString()
sorting_component = QQmlComponent(view.engine())
sorting_component.setData(('import QtQuick\nimport "' + sorting_url + '" as HomeSorting\nQtObject {\n'
    'property string criteria: ""\nproperty int order: Qt.AscendingOrder\n'
    'property var rows: [{id:"frequent",title:"Zulu"},{id:"middle",title:"Alpha"},{id:"rare",title:"Beta"}]\n'
    'readonly property int effectiveOrder: HomeSorting.mediaOrder(criteria, order)\n'
    'readonly property string effectiveCriteria: HomeSorting.mediaCriteria(criteria)\n'
    'readonly property var sorted: HomeSorting.activityEntries(rows, "", criteria, order)\n'
    'readonly property var recent: HomeSorting.recentActivityEntries(rows, "", criteria, order)\n}').encode(), QUrl())
assert not sorting_component.errors(), [e.toString() for e in sorting_component.errors()]
sorting = sorting_component.create()
assert sorting.property('effectiveOrder') == Qt.DescendingOrder.value
assert expr(sorting, 'sorted.map(e => e.id).join(",")') == 'frequent,middle,rare'
sorting.setProperty('criteria', 'activity')
assert sorting.property('effectiveOrder') == Qt.AscendingOrder.value
assert sorting.property('effectiveCriteria') == ''
assert expr(sorting, 'sorted.map(e => e.id).join(",")') == 'rare,middle,frequent'
sorting.setProperty('order', Qt.DescendingOrder.value)
assert expr(sorting, 'sorted.map(e => e.id).join(",")') == 'frequent,middle,rare'
sorting.setProperty('criteria', 'title')
sorting.setProperty('order', Qt.AscendingOrder.value)
assert expr(sorting, 'sorted.map(e => e.id).join(",")') == 'middle,rare,frequent'
# A frequently visited older playlist must follow the latest visit in Recent.
sorting.setProperty('rows', [dict(id='older',title='Alpha',lastActivity=100,activityCount=99),dict(id='newer',title='Zulu',lastActivity=200,activityCount=1)])
sorting.setProperty('criteria', '')
assert expr(sorting, 'recent.map(e => e.id).join(",")') == 'newer,older'
sorting.setProperty('criteria', 'activity')
sorting.setProperty('order', Qt.AscendingOrder.value)
assert expr(sorting, 'recent.map(e => e.id).join(",")') == 'older,newer'
sorting.setProperty('criteria', 'title')
assert expr(sorting, 'recent.map(e => e.id).join(",")') == 'older,newer'
media = QSignalSpy(root.mediaRequested)
folder = QSignalSpy(root.folderRequested)
playlist = QSignalSpy(root.playlistRequested)
artist = QSignalSpy(root.artistRequested)
see_all = QSignalSpy(root.seeAllRequested)
expr(root, 'setCurrentItemFocus(Qt.TabFocusReason)')
assert find(root, 'homevideo0').hasActiveFocus()
assert view.grabWindow().save(str(args.output_dir / 'home-sections-component.png'))
click(view, find(root, 'homevideo0'))
assert media.count() == 1 and entry_id(media.at(0)[0]) == 'video-12' and media.at(0)[1] is True
click(view, find(root, 'homevideo1'))
assert media.count() == 2 and entry_id(media.at(1)[0]) == 'video-8' and media.at(1)[1] is True
watched_card = find(root, 'homeWatchedVideo0')
assert expr(watched_card, 'Accessible.description') == 'Watch again from the beginning'
watched_card.forceActiveFocus(Qt.TabFocusReason)
QTest.keyClick(view, Qt.Key_Return)
assert media.count() == 3 and entry_id(media.at(2)[0]) == 'watched-1' and media.at(2)[1] is False
find(root, 'homeaudio0').forceActiveFocus(Qt.TabFocusReason)
QTest.qWait(20)
click(view, find(root, 'homeaudio0'))
assert media.count() == 4 and entry_id(media.at(3)[0]) == 'audio-44' and media.at(3)[1] is False
click(view, find(root, 'homeSectionlistenAgain'))
assert see_all.count() == 1 and see_all.at(0)[0] == 'listenAgain'
assert view.grabWindow().save(str(args.output_dir / 'home-grid-component.png'))

for kind, spy, expected in [('folder', folder, 'folder-21'), ('playlist', playlist, 'playlist-56'), ('artist', artist, 'artist-15')]:
    item = find(root, 'home' + kind + '0')
    item.forceActiveFocus(Qt.TabFocusReason)
    QTest.qWait(20)
    p = item.mapToScene(item.boundingRect().center())
    assert 0 <= p.y() <= view.height(), (kind, p.y())
    QTest.keyClick(view, Qt.Key_Return)
    assert spy.count() == 1 and entry_id(spy.at(0)[0]) == expected, (kind, spy.count(), entry_id(spy.at(0)[0]) if spy.count() else None)

root.setProperty('gridMode', False)
find(root, 'homevideo0').forceActiveFocus(Qt.TabFocusReason)
root.setProperty('contentY', 0)
QTest.qWait(40)
assert find(root, 'homevideo0').height() == 44
assert 'Artist Î±' in find(root, 'homeaudio0').property('artist')
assert find(root, 'homevideo3').isVisible() is False
assert view.grabWindow().save(str(args.output_dir / 'home-list-component.png'))

layouts = []
for width, height, scale, grid in [(640, 420, 1, True), (640, 420, 1, False), (960, 600, 1.5, True), (1280, 720, 2, False)]:
    view.resize(width, height)
    root.setProperty('uiScale', scale)
    root.setProperty('gridMode', grid)
    root.setProperty('contentY', 0)
    QTest.qWait(40)
    first = find(root, 'homevideo0')
    assert first.width() <= width - 48 * scale
    last = find(root, 'homeartist0')
    last.forceActiveFocus(Qt.TabFocusReason)
    QTest.qWait(20)
    p = last.mapToScene(last.boundingRect().center())
    assert 0 <= p.y() <= height, (width, height, scale, grid, p.y())
    layouts.append({'width': width, 'height': height, 'scale': scale, 'grid': grid, 'focusedCenterY': p.y()})

empty_view, empty = load('JuxtopposedHomeDashboard.qml', {}, 700, 500)
assert 'Unfinished videos' in find(empty, 'homeEmptycontinueWatching').property('text')
assert 'Videos you finish' in find(empty, 'homeEmptywatchAgain').property('text')
empty.setProperty('historyLoading', True)
assert 'Loading' in find(empty, 'homeEmptycontinueWatching').property('text')
empty.setProperty('historyLoading', False)
empty.setProperty('searchPattern', 'missing')
assert 'No matching' in find(empty, 'homeEmptylistenAgain').property('text')
empty.setProperty('searchPattern', '')
empty.setProperty('mediaEmpty', True)
add = QSignalSpy(empty.addFolderRequested)
click(empty_view, find(empty, 'homeAddFolder'))
assert add.count() == 1
empty.setProperty('errorText', 'Supplied library failure')
retry = QSignalSpy(empty.retryRequested)
click(empty_view, find(empty, 'homeRetry'))
assert retry.count() == 1
empty.setProperty('folderEntries', [{'id': 'folder-offline', 'title': 'Disconnected drive', 'available': False}])
QTest.qWait(20)
offline = find(empty, 'homefolder0')
assert not offline.isEnabled() and offline.property('subtitle') == 'Unavailable'

many_folders = [{'id': f'folder-{i}', 'title': f'Folder {i}'} for i in range(250)]
empty.setProperty('folderEntries', many_folders)
QTest.qWait(20)
pending = [empty]
dashboard_folder_cards = 0
while pending:
    item = pending.pop()
    dashboard_folder_cards += bool(re.fullmatch(r'homefolder\d+', item.objectName()))
    pending.extend(item.childItems())
assert dashboard_folder_cards == 4, dashboard_folder_cards
collection_view, collection = load('JuxtopposedHomeCollectionView.qml', {'entries': many_folders, 'kind': 'folder', 'fallbackIcon': icon('topbar_discover')}, 900, 600)
assert collection.property('count') == 250
pending = [collection]
rendered_cards = 0
while pending:
    item = pending.pop()
    rendered_cards += item.objectName().startswith('homeCollectionItem')
    pending.extend(item.childItems())
assert 0 < rendered_cards < 100, rendered_cards
activated = QSignalSpy(collection.activated)
item = find(collection, 'homeCollectionItem0')
click(collection_view, item)
assert activated.count() == 1 and entry_id(activated.at(0)[0]) == 'folder-0'
collection.setProperty('gridMode', False)
QTest.qWait(30)
assert find(collection, 'homeCollectionItem0').height() == 44
assert collection_view.grabWindow().save(str(args.output_dir / 'home-collection-component.png'))

# Mixed folders must choose their fallback from the native media kind.
# Keep both positive audio and video controls, and exercise accessible names
# with Unicode metadata rather than merely searching source text.
mixed_rows = SuppliedMediaModel([
    {'id': 'mixed-audio', 'title': 'Track \u00c4\u00e4', 'main_artist': 'Artist \u65e5\u672c\u8a9e', 'isVideo': False},
    {'id': 'mixed-video', 'title': 'Clip', 'fileName': 'Clip.mp4', 'isVideo': True, 'progress': .4},
])
mixed_view, mixed = load('JuxtopposedHomeCollectionView.qml', {'entries': mixed_rows, 'kind': 'media', 'fallbackIcon': icon('topbar_video')}, 900, 600)
mixed.setProperty('audioFallbackIcon', icon('topbar_music'))
QTest.qWait(30)
audio_card = find(mixed, 'homeCollectionItem0')
video_card = find(mixed, 'homeCollectionItem1')
assert audio_card.property('kind') == 'audio' and audio_card.property('fallbackIcon') == icon('topbar_music'), 'Audio in mixed folder must use music fallback'
assert video_card.property('kind') == 'video' and video_card.property('fallbackIcon') == icon('topbar_video'), 'Video in mixed folder must retain video fallback'
assert expr(audio_card, 'Accessible.name') == 'Track \u00c4\u00e4 \u2014 Artist \u65e5\u672c\u8a9e', 'Audio accessibility separator must be valid Unicode'
assert expr(video_card, 'Accessible.name') == 'Clip.mp4'
mixed_actions = QSignalSpy(mixed.activated)
click(mixed_view, audio_card)
assert mixed_actions.count() == 1 and mixed_actions.at(0)[1] == 'audio'
click(mixed_view, video_card)
assert mixed_actions.count() == 2 and mixed_actions.at(1)[1] == 'video'
assert mixed_view.grabWindow().save(str(args.output_dir / 'home-mixed-fallback-component.png'))

# Context menus must carry the selected row and its history section, including
# unavailable files whose playback stays disabled. Use actual pointer/key input
# on production delegates rather than emitting their routing signals directly.
actions_view, actions = load('JuxtopposedHomeDashboard.qml', dict(properties,
    videoEntries=[dict(id='offline-video', title='Missing fixture', available=False, progress=.3)]), 1700, 970)
video_menus = QSignalSpy(actions.videoMenuRequested)
section_menus = QSignalSpy(actions.historyMenuRequested)
pin_menus = QSignalSpy(actions.pinMenuRequested)
play_actions = QSignalSpy(actions.mediaRequested)
offline_video = find(actions, 'homevideo0')
assert offline_video.isEnabled() and not offline_video.property('available')
context_click(actions_view, offline_video)
context_key(actions_view, offline_video)
assert video_menus.count() == 2
for n in range(2):
    assert entry_id(video_menus.at(n)[0]) == 'offline-video' and video_menus.at(n)[1] == 'continueWatching'
click(actions_view, offline_video)
QTest.keyClick(actions_view, Qt.Key_Return)
assert play_actions.count() == 0, 'Unavailable history must retain its menu without starting playback'
watched_item = find(actions, 'homeWatchedVideo0')
context_key(actions_view, watched_item, Qt.Key_Menu, Qt.NoModifier)
assert video_menus.count() == 3 and video_menus.at(2)[1] == 'watchAgain'
assert entry_id(video_menus.at(2)[0]) == 'watched-1'
for category in ['continueWatching', 'watchAgain']:
    actions.setProperty('contentY', 0)
    options = find(actions, 'homeHistoryOptions' + category)
    options.forceActiveFocus(Qt.TabFocusReason)
    QTest.qWait(20)
    click(actions_view, options)
assert section_menus.count() == 2 and [section_menus.at(n)[0] for n in range(2)] == ['continueWatching', 'watchAgain']
actions.setProperty('historyBusy', True)
assert not find(actions, 'homeHistoryOptionscontinueWatching').isEnabled()
assert not find(actions, 'homeHistoryOptionswatchAgain').isEnabled()
assert 'Updating' in find(actions, 'homeHistoryStatus').property('text')
actions.setProperty('historyBusy', False)
actions.setProperty('historyMessage', 'Supplied history failure')
assert find(actions, 'homeHistoryStatus').isVisible() and find(actions, 'homeHistoryStatus').property('text') == 'Supplied history failure'
context_key(actions_view, find(actions, 'homefolder0'))
assert pin_menus.count() == 1 and entry_id(pin_menus.at(0)[0]) == 'folder-21'

history_view, history_collection = load('JuxtopposedHomeCollectionView.qml', {
    'entries': [dict(id='offline-full', title='Missing fixture', available=False), dict(id='online-full', title='Available fixture')],
    'kind': 'video', 'historyActionsAvailable': True,
}, 900, 600)
full_menus = QSignalSpy(history_collection.contextMenuRequested)
full_play = QSignalSpy(history_collection.activated)
full_offline = find(history_collection, 'homeCollectionItem0')
assert full_offline.isEnabled() and not full_offline.property('available')
context_click(history_view, full_offline)
context_key(history_view, full_offline)
assert full_menus.count() == 2 and all(entry_id(full_menus.at(n)[0]) == 'offline-full' for n in range(2))
click(history_view, full_offline)
QTest.keyClick(history_view, Qt.Key_Return)
assert full_play.count() == 0
full_online = find(history_collection, 'homeCollectionItem1')
context_key(history_view, full_online)
assert full_menus.count() == 3 and entry_id(full_menus.at(2)[0]) == 'online-full'
click(history_view, full_online)
assert full_play.count() == 1 and entry_id(full_play.at(0)[0]) == 'online-full'

warnings = messages
assert not warnings, warnings
verification = {'scope': 'Production Home QML with supplied QAbstractListModel/array data; native ML/player/persistence not executed.',
                'mediaActions': media.count(), 'collectionActions': [folder.count(), playlist.count(), artist.count()],
                'seeAllActions': see_all.count(), 'fullCollectionCount': collection.property('count'), 'renderedCards': rendered_cards,
                'dashboardPreviewCardsFrom250Folders': dashboard_folder_cards,
                'mixedMediaFallbackChecks': 6,
                'historyContextRouting': {'dashboardItemMenus': video_menus.count(), 'sectionOptions': section_menus.count(),
                    'unavailablePlaybackActions': play_actions.count(), 'fullCollectionItemMenus': full_menus.count(),
                    'fullCollectionAvailablePlaybackActions': full_play.count(), 'pointerAndKeyboard': True},
                'layouts': layouts, 'warnings': warnings, 'imageDependency': 'Unmodified production Widgets.ImageExt', 'imageRenderer': 'Software Image fallback; native RHI rounding not tested'}
(args.output_dir / 'home-component-verification.json').write_text(json.dumps(verification, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
print('Home dashboard and collection component checks passed.')
