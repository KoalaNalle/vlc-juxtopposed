# SPDX-License-Identifier: GPL-2.0-or-later
"""Production Home/History/PageExt routing with explicit native-boundary adapters.

This exercises QML input and handlers, not native menus, dialogs, ML or disk
persistence. Supplied models record calls without implementing history edits.
"""
import argparse
import faulthandler
import hashlib
import json
import os
import subprocess
from pathlib import Path

os.environ['QT_QPA_PLATFORM'] = 'offscreen'
os.environ['QT_QUICK_BACKEND'] = 'software'
os.environ['QT_QUICK_CONTROLS_STYLE'] = 'Basic'
os.environ['QML_DISABLE_DISK_CACHE'] = '1'
import PySide6
from PySide6.QtCore import QObject, Property, QResource, QUrl, Qt, qInstallMessageHandler
from PySide6.QtGui import QGuiApplication
from PySide6.QtQml import (QQmlExpression, qmlContext, QmlAttached, QJSValue,
    qmlRegisterUncreatableType)
from PySide6.QtQuick import QQuickView
from PySide6.QtTest import QTest

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--repo-root', type=Path, required=True)
parser.add_argument('--output-dir', type=Path, required=True)
args = parser.parse_args()
faulthandler.dump_traceback_later(60, exit=True)
repo, output = args.repo_root.resolve(), args.output_dir.resolve()
os.environ['QT_QPA_FONTDIR'] = str(repo / 'modules/gui/qt/pixmaps/juxtopposed')
output.mkdir(parents=True, exist_ok=False)
messages = []
qInstallMessageHandler(lambda kind, context, message: messages.append(message))
app = QGuiApplication([])
sources = {}
modules = output / 'supplied-modules'
qml = output / 'production-qml'

def write(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(value, encoding='utf-8', newline='\n')

def freeze(relative, target):
    path = repo / relative
    data = path.read_bytes()
    sources[relative] = hashlib.sha256(data).hexdigest()
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(data)

for name in ['JuxtopposedHomePage.qml', 'JuxtopposedHomeHistoryPage.qml',
             'JuxtopposedHomeDashboard.qml', 'JuxtopposedHomeCollectionView.qml',
             'JuxtopposedHomeCard.qml', 'JuxtopposedHomeSorting.js']:
    freeze('modules/gui/qt/medialibrary/qml/' + name, qml / name)
widgets = modules / 'VLC/Widgets'
for name in ['PageExt.qml', 'ImageExt.qml']:
    freeze('modules/gui/qt/widgets/qml/' + name, widgets / name)
freeze('modules/gui/qt/style/JuxtopposedPalette.qml', modules / 'VLC/Style/JuxtopposedPalette.qml')
freeze('modules/gui/qt/pixmaps/juxtopposed/InterVariable.ttf', output / 'assets/InterVariable.ttf')
freeze('modules/gui/qt/tests/test_juxtopposed_home_history_actions.py', output / 'reviewed-test.py')
# Supply the real font at its authored qrc URL. A Python URL interceptor can
# deadlock against threaded font loading, so package this test resource with
# the already-installed Qt resource utility (no VLC/native rebuild).
write(output / 'assets.qrc', '<RCC><qresource prefix="/juxtopposed"><file alias="InterVariable.ttf">assets/InterVariable.ttf</file></qresource></RCC>\n')
resource_file = output / 'assets.rcc'
qt_package = Path(PySide6.__file__).parent
resource_tools = [qt_package / ('rcc.exe' if os.name == 'nt' else 'rcc'), qt_package / 'Qt/libexec/rcc']
resource_tool = next((path for path in resource_tools if path.is_file()), None)
assert resource_tool, 'The installed Qt resource utility is required for the supplied font resource'
subprocess.run([str(resource_tool), '--binary',
    str(output / 'assets.qrc'), '-o', str(resource_file)], check=True, timeout=20, capture_output=True)
assert QResource.registerResource(str(resource_file))

# These adapters expose the native boundary. Production handlers, views, card
# delegates, collection forwarding and PageExt's menu override stay unchanged.
write(widgets / 'qmldir', '''module VLC.Widgets
PageExt 1.0 PageExt.qml
ImageExt 1.0 ImageExt.qml
JuxtopposedLibraryHeader 1.0 JuxtopposedLibraryHeader.qml
GridSortFilterControls 1.0 GridSortFilterControls.qml
SubtitleLabel 1.0 SubtitleLabel.qml
''')
write(widgets / 'JuxtopposedLibraryHeader.qml', '''import QtQuick
Item {
    implicitHeight: 48
    property real uiScale; property string iconFontFamily; property string backIcon
    property string viewIcon; property string searchIcon; property string moreIcon
    property bool canGoBack; property bool canGoForward; property bool sortAvailable
    property bool searchAvailable; property bool viewAvailable; property string searchPattern
    property int itemCount; property bool countAvailable; property var breadcrumbs
    signal backRequested(); signal forwardRequested(); signal breadcrumbRequested(var uri)
    signal viewRequested(); signal searchRequested(string pattern); signal contentFocusRequested()
    signal sortRequested(var position); signal moreRequested(var position)
    function setCurrentItemFocus(reason) {}
    function showSearch() {}
}
''')
write(widgets / 'GridSortFilterControls.qml', 'import QtQuick\nItem { property var sortMenu; property var search; property var sort }\n')
write(widgets / 'SubtitleLabel.qml', 'import QtQuick\nText {}\n')
style = modules / 'VLC/Style'
write(style / 'qmldir', '''module VLC.Style
singleton JuxtopposedPalette 1.0 JuxtopposedPalette.qml
singleton VLCStyle 1.0 VLCStyle.qml
singleton VLCIcons 1.0 VLCIcons.qml
ColorContext 1.0 ColorContext.qml
''')
write(style / 'VLCStyle.qml', '''pragma Singleton
import QtQuick
QtObject { property real scale: 1; property url noArtArtist: ""; function dp(value, scale) { return value * scale } }
''')
write(style / 'VLCIcons.qml', '''pragma Singleton
import QtQuick
QtObject {
    property string fontFamily: ""; property string ic_fluent_chevron_right_24: ">"
    property string more: "..."; property string topbar_video: "V"; property string topbar_music: "A"
    property string topbar_discover: "F"; property string playlist: "P"
    property string back: "<"; property string list: "L"; property string grid: "G"; property string search: "S"
}
''')
write(style / 'ColorContext.qml', 'import QtQuick\nQtObject { enum Set { View } property int colorSet: 0 }\n')
interface = modules / 'VLC/MainInterface'
write(interface / 'qmldir', '''module VLC.MainInterface
singleton MainCtx 1.0 MainCtx.qml
singleton History 1.0 History.qml
SearchCtx 1.0 SearchCtx.qml
SortCtx 1.0 SortCtx.qml
''')
write(interface / 'MainCtx.qml', '''pragma Singleton
import QtQuick
QtObject {
    property bool gridView: true; property bool playerView: false
    property bool hasGridListMode: false; property bool juxtopposedControls: true
    property QtObject sidebarPins: QtObject {
        property bool canRetry: false; property var pinned: []
        function pinItem(id) { pinned = [...pinned, id] }
    }
    function settingValue(key, fallback) { return fallback }
    function setSettingValue(key, value) {}
}
''')
write(interface / 'History.qml', '''pragma Singleton
import QtQuick
QtObject {
    property var viewPath: []; property QtObject viewProp: QtObject { property string historySearchPattern: "" }
    property bool previousEmpty: true; property bool nextEmpty: true
    function exactMatch(left, right) { return JSON.stringify(left) === JSON.stringify(right) }
    function previous(reason) {}
    function next(reason) {}
    function push(uri, reason) {}
}
''')
write(interface / 'SearchCtx.qml', 'import QtQuick\nQtObject { property string pattern: ""; property bool available: false; signal askShow() }\n')
write(interface / 'SortCtx.qml', 'import QtQuick\nQtObject { property string criteria: ""; property int order: Qt.DescendingOrder; property var model: []; property bool available: false }\n')
menus = modules / 'VLC/Menus'
write(menus / 'qmldir', '''module VLC.Menus
StringListMenu 1.0 StringListMenu.qml
SortMenu 1.0 SortMenu.qml
QmlGlobalMenu 1.0 QmlGlobalMenu.qml
''')
write(menus / 'StringListMenu.qml', '''import QtQuick
QtObject {
    property var ctx; property int popupCount: 0; property var actions: []; property point lastPosition
    signal selected(int index, var options)
    function popup(position, entries) { lastPosition = position; actions = entries; popupCount++ }
}
''')
write(menus / 'SortMenu.qml', 'import QtQuick\nQtObject { property var ctx; signal selected(int index); function popup(position, above, model) {} }\n')
write(menus / 'QmlGlobalMenu.qml', 'import QtQuick\nQtObject { objectName: "suppliedGlobalMenu"; property var ctx; property int popupCount: 0; function popup(position) { popupCount++ } }\n')
dialogs = modules / 'VLC/Dialogs'
write(dialogs / 'qmldir', 'module VLC.Dialogs\nsingleton DialogsProvider 1.0 DialogsProvider.qml\n')
write(dialogs / 'DialogsProvider.qml', '''pragma Singleton
import QtQuick
QtObject {
    property bool answer: false; property var questions: []; property int addedFolders: 0
    function questionDialog(text) { questions = [...questions, text]; return answer }
    function addMediaLibraryFolder() { addedFolders++ }
}
''')
library = modules / 'VLC/MediaLibrary'
write(library / 'qmldir', '''module VLC.MediaLibrary
singleton MediaLib 1.0 MediaLib.qml
MLRecentVideoModel 1.0 MLRecentVideoModel.qml
MLRecentAudioModel 1.0 MLRecentAudioModel.qml
''')
write(library / 'MediaLib.qml', '''pragma Singleton
import QtQuick
QtObject {
    property bool idle: true; property var playCalls: []
    property QtObject homeActivity: QtObject {
        property int mediaCount: 4; property bool loading: false; property string error: ""
        property var folders: []; property var playlists: []; property var artists: []
        function refresh() {}
    }
    function addAndPlay(id, options) { playCalls = [...playCalls, {id: id, options: options}] }
}
''')
write(library / 'MLRecentVideoModel.qml', '''import QtQuick
ListModel {
    enum HistoryMode { All, Unfinished, Completed }
    property var ml; property int historyMode: 0; property int limit: 0; property string searchPattern: ""
    property string sortCriteria: ""; property int sortOrder: Qt.DescendingOrder
    property bool loading: false; property bool historyBusy: false
    property var clearCalls: []; property var removeCalls: []
    signal historyEditFinished(bool success, string error)
    function clearHistory() { clearCalls = [...clearCalls, {mode: historyMode, search: searchPattern}] }
    function removeFromHistory(id) { removeCalls = [...removeCalls, id] }
    Component.onCompleted: {
        append({id: "offline-" + historyMode, title: "Missing fixture", fileName: "fixture.mkv", isVideo: true, available: false, progress: .3})
        append({id: "online-" + historyMode, title: "Available fixture", fileName: "fixture.mkv", isVideo: true, available: true, progress: .3})
    }
}
''')
write(library / 'MLRecentAudioModel.qml', '''import QtQuick
ListModel {
    property var ml; property int limit: 0; property string searchPattern: ""; property string sortCriteria: ""
    property int sortOrder: Qt.DescendingOrder; property bool loading: false
    Component.onCompleted: append({id: "audio", title: "Audio fixture", isVideo: false})
}
''')
util = modules / 'VLC/Util'
write(util / 'qmldir', 'module VLC.Util\nsingleton Helpers 1.0 Helpers.qml\n')
write(util / 'Helpers.qml', 'pragma Singleton\nimport QtQuick\nQtObject { function isArray(value) { return Array.isArray(value) } }\n')

class NavigationProperties(QObject):
    parentItem = Property(QObject, lambda self: None, lambda self, item: None)
    upItem = Property(QObject, lambda self: None, lambda self, item: None)
    navigable = Property(bool, lambda self: True, lambda self, value: None)
    downAction = Property(QJSValue, lambda self: QJSValue(), lambda self, value: None)

@QmlAttached(NavigationProperties)
class NavigationAdapter(QObject):
    @staticmethod
    def qmlAttachedProperties(owner, target): return NavigationProperties(target)

qmlRegisterUncreatableType(NavigationAdapter, 'VLC.Util', 1, 0, 'Navigation', 'Supplied navigation boundary')

views = []
def load(name, properties=None):
    view = QQuickView()
    view.engine().addImportPath(str(modules))
    view.setResizeMode(QQuickView.SizeRootObjectToView)
    view.setInitialProperties(properties or {})
    view.setSource(QUrl.fromLocalFile(str(qml / name)))
    assert not view.errors(), [e.toString() for e in view.errors()]
    view.resize(1700, 1100)
    view.show()
    QTest.qWait(60)
    views.append(view)
    return view, view.rootObject()

def expr(root, code):
    expression = QQmlExpression(qmlContext(root), root, code)
    value, _ = expression.evaluate()
    assert not expression.hasError(), expression.error().toString()
    return value.toVariant() if hasattr(value, 'toVariant') else value

def find(root, name):
    pending = [root]
    while pending:
        item = pending.pop()
        if item.objectName() == name: return item
        pending.extend(item.childItems())
    raise AssertionError(name)

def right_click(view, item):
    QTest.mouseClick(view, Qt.RightButton, Qt.NoModifier, item.mapToScene(item.boundingRect().center()).toPoint())

def key_menu(view, item):
    item.forceActiveFocus(Qt.TabFocusReason)
    QTest.keyClick(view, Qt.Key_F10, Qt.ShiftModifier)

checks = []
home_view, home = load('JuxtopposedHomePage.qml')
dashboard = home.property('contentItem')
assert expr(home, 'videos.historyMode') == 1 and expr(home, 'watchedVideos.historyMode') == 2
assert expr(home, 'videos.limit') == 4 and expr(home, 'watchedVideos.limit') == 4
expr(home, 'search.pattern = "visible fixture"')
for prefix, model, category, mode in [('homevideo', 'videos', 'continueWatching', 1), ('homeWatchedVideo', 'watchedVideos', 'watchAgain', 2)]:
    card = find(dashboard, prefix + '0')
    assert card.isEnabled() and not card.property('available')
    key_menu(home_view, card)
    assert expr(home, 'pinMenu.actions.join("|")') == 'Pin to sidebar|Remove from this list'
    assert expr(home, 'pinMenu.itemId') == 'offline-' + str(mode)
    expr(home, 'pinMenu.selected(1, null)')
    assert expr(home, model + '.removeCalls.join(",")') == 'offline-' + str(mode)
    # A menu opened before another operation starts must recheck busy on use.
    expr(home, model + '.historyBusy = true')
    expr(home, 'pinMenu.selected(1, null)')
    assert expr(home, model + '.removeCalls.length') == 1
    assert not find(dashboard, 'homeHistoryOptions' + category).isEnabled()
    expr(home, model + '.historyBusy = false')
    expr(dashboard, 'historyMenuRequested("' + category + '", Qt.point(5, 8))')
    assert expr(home, 'historyMenu.actions.join("|")') == 'Clear all items'
    expr(home, 'DialogsProvider.answer = false; historyMenu.selected(0, null)')
    assert expr(home, model + '.clearCalls.length') == 0
    prompt = expr(home, 'DialogsProvider.questions.slice(-1)[0]')
    assert ('Watch Again' if mode == 2 else 'Continue Watching') in prompt
    assert 'hidden by search' in prompt and 'files will be kept' in prompt
    expr(home, 'DialogsProvider.answer = true; historyMenu.selected(0, null)')
    assert expr(home, model + '.clearCalls.length') == 1
    assert expr(home, model + '.clearCalls[0].mode') == mode
    assert expr(home, model + '.clearCalls[0].search') == 'visible fixture'
    checks.append('Home ' + category + ': unavailable removal, busy recheck, clear cancel/confirm and category target')
expr(home, 'videos.historyEditFinished(false, "Supplied removal failure")')
assert home.property('historyMessage') == 'Supplied removal failure'
dashboard.setProperty('contentY', 0)
QTest.qWait(20)
assert home_view.grabWindow().save(str(output / 'home-history-error-component.png'))
expr(home, 'watchedVideos.historyEditFinished(true, "")')
assert home.property('historyMessage') == ''
# Retryable sidebar failure suppresses only Pin, retaining an unambiguous Remove.
expr(home, 'MainCtx.sidebarPins.canRetry = true')
key_menu(home_view, find(dashboard, 'homevideo0'))
assert expr(home, 'pinMenu.actions.join("|")') == 'Remove from this list'
expr(home, 'pinMenu.selected(0, null)')
assert expr(home, 'videos.removeCalls.length') == 2
expr(home, 'MainCtx.sidebarPins.canRetry = false')
key_menu(home_view, find(dashboard, 'homevideo0'))
expr(home, 'pinMenu.selected(0, null)')
assert expr(home, 'MainCtx.sidebarPins.pinned.join(",")') == 'offline-1'
checks.append('Home Pin/Remove indices follow the actual available menu entries')

for completed in [False, True]:
    view, page = load('JuxtopposedHomeHistoryPage.qml', {'completedVideos': completed})
    collection = page.property('contentItem')
    mode = 2 if completed else 1
    expr(page, 'search.pattern = "one visible result"')
    card = find(collection, 'homeCollectionItem0')
    right_click(view, card)
    assert expr(page, 'itemMenu.actions.join("|")') == 'Remove from this list'
    key_menu(view, card)
    assert expr(page, 'itemMenu.popupCount') == 2
    expr(page, 'itemMenu.selected(0, null)')
    assert expr(page, 'videos.removeCalls.join(",")') == 'offline-' + str(mode)
    # Actual PageExt handler must dispatch the history override from its header.
    expr(page, 'header.item.moreRequested(Qt.point(10, 20))')
    assert expr(page, 'clearMenu.popupCount') == 1
    assert page.findChild(QObject, 'suppliedGlobalMenu').property('popupCount') == 0
    expr(page, 'DialogsProvider.answer = false; clearMenu.selected(0, null)')
    assert expr(page, 'videos.clearCalls.length') == 0
    expr(page, 'DialogsProvider.answer = true; clearMenu.selected(0, null)')
    assert expr(page, 'videos.clearCalls[0].mode') == mode
    assert expr(page, 'videos.clearCalls[0].search') == 'one visible result'
    expr(page, 'videos.historyBusy = true')
    popup_count = expr(page, 'itemMenu.popupCount')
    key_menu(view, card)
    assert expr(page, 'itemMenu.popupCount') == popup_count
    expr(page, 'itemMenu.selected(0, null); clearMenu.selected(0, null); header.item.moreRequested(Qt.point(10, 20))')
    assert expr(page, 'videos.removeCalls.length') == 1 and expr(page, 'videos.clearCalls.length') == 1
    assert expr(page, 'clearMenu.popupCount') == 1
    status = find(page, 'homeHistoryStatus')
    assert status.isVisible() and 'Updating' in status.property('text')
    assert view.grabWindow().save(str(output / ('watch-again-busy-component.png' if completed else 'continue-watching-busy-component.png')))
    expr(page, 'videos.historyBusy = false; videos.historyEditFinished(false, "Supplied edit failure")')
    assert status.isVisible() and status.property('text') == 'Supplied edit failure'
    expr(page, 'videos.historyEditFinished(true, "")')
    assert not status.isVisible()
    assert expr(page, 'MediaLib.playCalls.length') == 0
    # Available playback retains each section's existing resume policy.
    online = find(collection, 'homeCollectionItem1')
    online.forceActiveFocus(Qt.TabFocusReason)
    QTest.keyClick(view, Qt.Key_Return)
    assert expr(page, 'MediaLib.playCalls.slice(-1)[0].options[0]') == (':restore-playback-pos=0' if completed else ':restore-playback-pos=2')
    checks.append(('Watch Again' if completed else 'Continue Watching') + ' full page: right-click/Shift+F10, override, cancel/confirm, busy/error, resume policy')

audio_view, audio_page = load('JuxtopposedHomeHistoryPage.qml', {'isVideo': False})
assert expr(audio_page, 'libraryMoreMenu === null')
expr(audio_page, 'header.item.moreRequested(Qt.point(10, 20))')
assert audio_page.findChild(QObject, 'suppliedGlobalMenu').property('popupCount') == 1 and expr(audio_page, 'clearMenu.popupCount') == 0
key_menu(audio_view, find(audio_page.property('contentItem'), 'homeCollectionItem0'))
assert expr(audio_page, 'itemMenu.popupCount') == 0
checks.append('Listen Again retains global header menu and does not expose video-history removal')

guards = {name: hashlib.sha256((repo / name).read_bytes()).hexdigest() == digest for name, digest in sources.items()}
verification = {
    'scope': 'Unmodified production HomePage, HomeHistoryPage, PageExt, dashboard, collection and card with supplied native-boundary modules; offscreen software rendering only.',
    'notExecuted': ['native medialibrary query/mutation', 'native menu/dialog presentation', 'disk persistence', 'ordinary desktop'],
    'searchScope': 'QML confirmation explicitly includes hidden items and invokes the selected model without a search argument. Native category-wide selection remains a separate backend/native requirement.',
    'checks': checks, 'productionSha256': sources, 'sourceGuards': guards, 'qmlWarnings': messages,
}
write(output / 'verification.json', json.dumps(verification, indent=2, ensure_ascii=False) + '\n')
assert all(guards.values()), guards
assert not messages, messages
for view in views: view.close()
QTest.qWait(20)
faulthandler.cancel_dump_traceback_later()
print('Production Home history action routing checks passed (supplied native boundaries).')
