# SPDX-License-Identifier: GPL-2.0-or-later
"""Actual production loader QML with supplied dependencies; no native GUI claim."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import sys
import traceback

os.environ['QT_QPA_PLATFORM'] = 'offscreen'
os.environ['QT_QUICK_CONTROLS_STYLE'] = 'Basic'
os.environ['QSG_RHI_BACKEND'] = 'software'
os.environ['QT_QUICK_BACKEND'] = 'software'
from PySide6.QtCore import QObject, Property, Signal, Slot, Qt, QAbstractListModel, QModelIndex, QItemSelectionModel, qInstallMessageHandler
from PySide6.QtGui import QGuiApplication
from PySide6.QtQml import (QQmlApplicationEngine, QQmlPropertyMap, QJSValue,
    QmlAttached, qmlRegisterType, qmlRegisterUncreatableType, qmlRegisterSingletonInstance)

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--repo-root', type=Path, required=True)
parser.add_argument('--overlay', type=Path)
parser.add_argument('--mode', choices=['baseline', 'fixed'], required=True)
parser.add_argument('--output-dir', type=Path, required=True)
args = parser.parse_args()
repo, output = args.repo_root.resolve(), args.output_dir.resolve()
overlay = args.overlay.resolve() if args.overlay else None
output.mkdir(parents=True, exist_ok=False)
messages = []
all_messages = []
expected_failures = []
def capture(kind, context, text):
    messages.append(text); all_messages.append(text)
qInstallMessageHandler(capture)
app = QGuiApplication([])
def digest(data): return hashlib.sha256(data).hexdigest()
def source(name):
    path = overlay / name if overlay and (overlay / name).is_file() else repo / name
    return path.read_bytes()
sources = {}
def save_source(name, target):
    data = source(name); sources[name] = digest(data)
    target.parent.mkdir(parents=True, exist_ok=True); target.write_bytes(data)
    return data.decode('utf-8')
def write(path, text):
    path.parent.mkdir(parents=True, exist_ok=True); path.write_text(text, encoding='utf-8', newline='\n')

class HistoryAdapter(QObject):
    changed = Signal()
    navigate = Signal(int)
    def __init__(self):
        super().__init__(); self.frames = []; self.forward = []
    @Property('QStringList', notify=changed)
    def viewPath(self): return self.frames[-1][0] if self.frames else []
    @Property(QObject, notify=changed)
    def viewProp(self): return self.frames[-1][1] if self.frames else None
    @Property(bool, notify=changed)
    def previousEmpty(self): return len(self.frames) < 2
    @Property(bool, notify=changed)
    def nextEmpty(self): return not self.forward
    @Slot('QStringList')
    def update(self, path):
        if not self.frames: self.frames.append((list(path), QQmlPropertyMap(self)))
        else: self.frames[-1] = (list(path), self.frames[-1][1])
        self.changed.emit()
    @Slot('QStringList', 'QStringList', result=bool)
    def exactMatch(self, left, right): return list(left) == list(right)
    def push(self, path, props=None, reason=Qt.OtherFocusReason):
        mapping = QQmlPropertyMap(self)
        for key, value in (props or {}).items(): mapping.insert(key, value)
        self.frames.append((list(path), mapping)); self.forward.clear()
        self.changed.emit(); self.navigate.emit(reason.value)
    def previous(self):
        assert len(self.frames) > 1
        self.forward.append(self.frames.pop()); self.changed.emit(); self.navigate.emit(Qt.OtherFocusReason.value)
    def next(self):
        assert self.forward
        self.frames.append(self.forward.pop()); self.changed.emit(); self.navigate.emit(Qt.OtherFocusReason.value)

class ContextAdapter(QObject):
    changed = Signal()
    def __init__(self): super().__init__(); self._grid = True; self._ml = True
    @Property(bool, notify=changed)
    def gridView(self): return self._grid
    @gridView.setter
    def gridView(self, value): self._grid = value; self.changed.emit()
    @Property(bool, notify=changed)
    def mediaLibraryAvailable(self): return self._ml
    @mediaLibraryAvailable.setter
    def mediaLibraryAvailable(self, value): self._ml = value; self.changed.emit()
    @Slot(QObject, int)
    def setCursor(self, item, cursor): pass
    @Slot(QObject)
    def unsetCursor(self, item): pass

class NavigationProperties(QObject):
    changed = Signal()
    def __init__(self, parent=None): super().__init__(parent); self._parent = None; self._cancel = QJSValue()
    @Property(QObject, notify=changed)
    def parentItem(self): return self._parent
    @parentItem.setter
    def parentItem(self, item): self._parent = item; self.changed.emit()
    @Property(QJSValue, notify=changed)
    def cancelAction(self): return self._cancel
    @cancelAction.setter
    def cancelAction(self, value): self._cancel = value; self.changed.emit()

@QmlAttached(NavigationProperties)
class NavigationAdapter(QObject):
    @staticmethod
    def qmlAttachedProperties(owner, target): return NavigationProperties(target)

class BaseModelAdapter(QAbstractListModel):
    changed = Signal()
    loadingChanged = Signal()
    def __init__(self, parent=None): super().__init__(parent); self._size = 12; self._loading = False
    @Property(int, notify=changed)
    def count(self): return self._size
    @Property(bool, notify=loadingChanged)
    def loading(self): return self._loading
    @loading.setter
    def loading(self, value): self._loading = value; self.loadingChanged.emit()
    @Property(int, notify=changed)
    def size(self): return self._size
    @size.setter
    def size(self, value):
        self.beginResetModel(); self._size = value; self.endResetModel(); self.changed.emit()
    def rowCount(self, parent=QModelIndex()): return 0 if parent.isValid() else self._size
    def data(self, index, role=Qt.DisplayRole): return str(index.row()) if index.isValid() else None

history, context = HistoryAdapter(), ContextAdapter()
qmlRegisterSingletonInstance(HistoryAdapter, 'VLC.MainInterface', 1, 0, 'History', history)
qmlRegisterSingletonInstance(ContextAdapter, 'VLC.MainInterface', 1, 0, 'MainCtx', context)
qmlRegisterUncreatableType(NavigationAdapter, 'VLC.Util', 1, 0, 'Navigation', 'Attached dependency adapter')
qmlRegisterType(BaseModelAdapter, 'VLC.Util', 1, 0, 'BaseModel')
modules = output / 'modules'
widgets, interface, util, style = [modules / 'VLC' / name for name in ['Widgets', 'MainInterface', 'Util', 'Style']]
write(widgets / 'qmldir', 'module VLC.Widgets\nPageLoader 1.0 PageLoader.qml\nStackViewExt 1.0 StackViewExt.qml\nProgressIndicator 1.0 ProgressIndicator.qml\n')
save_source('modules/gui/qt/widgets/qml/PageLoader.qml', widgets / 'PageLoader.qml')
save_source('modules/gui/qt/widgets/qml/StackViewExt.qml', widgets / 'StackViewExt.qml')
write(widgets / 'ProgressIndicator.qml', 'import QtQuick\nItem { property string text: "" }\n')
write(interface / 'qmldir', 'module VLC.MainInterface\nMainViewLoader 1.0 MainViewLoader.qml\n')
save_source('modules/gui/qt/maininterface/qml/MainViewLoader.qml', interface / 'MainViewLoader.qml')
write(util / 'qmldir', 'module VLC.Util\nsingleton Helpers 1.0 Helpers.qml\nListSelectionModel 1.0 ListSelectionModel.qml\n')
write(util / 'Helpers.qml', 'pragma Singleton\nimport QtQuick\nQtObject { function isArray(value) { return Array.isArray(value) } function enforceFocus(item, reason) { item.forceActiveFocus(reason) } }\n')
write(util / 'ListSelectionModel.qml', 'import QtQml.Models\nItemSelectionModel {}\n')
write(style / 'qmldir', 'module VLC.Style\nsingleton VLCStyle 1.0 VLCStyle.qml\nColorContext 1.0 ColorContext.qml\n')
write(style / 'VLCStyle.qml', 'pragma Singleton\nimport QtQuick\nQtObject { readonly property int duration_humanMoment: 5000 }\n')
write(style / 'ColorContext.qml', 'import QtQuick\nQtObject { enum Set { View } property int colorSet: 0 }\n')

for name in ['MainDisplay.qml', 'VideoDisplay.qml', 'MusicDisplay.qml', 'DiscoverDisplay.qml',
             'VideoAllSubDisplay.qml', 'VideoGroupDisplay.qml', 'VideoPlaylistsDisplay.qml']:
    directory = 'maininterface' if name == 'MainDisplay.qml' else 'network' if name == 'DiscoverDisplay.qml' else 'medialibrary'
    relative = f'modules/gui/qt/{directory}/qml/{name}'
    save_source(relative, output / 'source-snapshots' / name)

def nested(name, route):
    text = (output / 'source-snapshots' / name).read_text(encoding='utf-8')
    match = re.search(r'name:\s*"' + re.escape(route) + r'"\s*,(.*?)(?:url:|component:|\})', text, re.DOTALL)
    assert match, (name, route)
    flag = re.search(r'nested:\s*([^,\n]+)', match.group(1))
    return flag.group(1).strip() if flag else 'false'
def alias(name, child):
    text = (output / 'source-snapshots' / name).read_text(encoding='utf-8')
    matches = re.findall(r'^\s*property alias initialIndex:\s*' + child + r'\.initialIndex\s*$', text, re.MULTILINE)
    assert len(matches) <= 1
    return matches[0].strip() if matches else ''

fixtures = output / 'fixtures'
leaf = '''import QtQuick
import QtQuick.Templates as T
import VLC.MainInterface
import VLC.Util
T.Page {
    id: leaf
    property var pagePrefix: []
    property var parentId: -1
    property var constructionId: -1
    property string routeRole: "ROLE"
    ALIAS
    Component.onCompleted: constructionId = parentId
    function setCurrentItemFocus(reason) { CHILD.setCurrentItemFocus(reason) }
    MainViewLoader {
        id: CHILD
        objectName: "actualViewLoader"
        anchors.fill: parent
        model: BaseModel { size: COUNT }
        grid: Component { ViewFixture { selectionModel: CHILD.selectionModel } }
        list: Component { ViewFixture { selectionModel: CHILD.selectionModel } }
        emptyLabel: Component { Item {} }
    }
}
'''
write(fixtures / 'ViewFixture.qml', '''import QtQuick
FocusScope {
    property var selectionModel
    property int currentIndex: 0
    function setCurrentItem(index) { currentIndex = index }
    function setCurrentItemFocus(reason) { forceActiveFocus(reason) }
    function positionViewAtIndex(index, mode) {}
}
''')
for filename, role, alias_name, child, count in [
    ('VideoBase.qml', 'video-base', 'VideoAllSubDisplay.qml', 'videoAll', 12),
    ('VideoGroup.qml', 'video-group', 'VideoGroupDisplay.qml', 'videoAll', 12),
    ('PlaylistAll.qml', 'playlist-all', 'VideoPlaylistsDisplay.qml', 'playlistView', 12),
]:
    write(fixtures / filename, leaf.replace('ROLE', role).replace('ALIAS', alias(alias_name, child)).replace('CHILD', child).replace('COUNT', str(count)))
write(fixtures / 'PlaylistList.qml', leaf.replace('ROLE', 'playlist-list').replace('ALIAS', 'property alias initialIndex: view.initialIndex').replace('CHILD', 'view').replace('COUNT', '0'))
simple = '''import QtQuick
import QtQuick.Templates as T
T.Page { property var pagePrefix: []; property string routeRole: "ROLE"; function setCurrentItemFocus(reason) { forceActiveFocus(reason) } }
'''
write(fixtures / 'HomeLeaf.qml', simple.replace('ROLE', 'home-base'))
write(fixtures / 'NoMlHome.qml', simple.replace('ROLE', 'home-no-ml'))
write(fixtures / 'Failing.qml', simple.replace('ROLE', 'failing').replace('function setCurrentItemFocus', 'function loadView(path, properties, reason) { return false }\nfunction setCurrentItemFocus'))
write(fixtures / 'HomeNested.qml', 'import VLC.Widgets as Widgets\nWidgets.PageLoader { pageModel: [{name: "base", default: true, url: "HomeLeaf.qml"}] }\n')
write(fixtures / 'PlaylistNested.qml', 'import VLC.Widgets as Widgets\nWidgets.PageLoader { pageModel: [{name: "all", default: true, url: "PlaylistAll.qml"}, {name: "list", url: "PlaylistList.qml"}] }\n')
write(fixtures / 'VideoAllNested.qml', 'import VLC.Widgets as Widgets\nWidgets.PageLoader { pageModel: [{name: "base", default: true, url: "VideoBase.qml"}, {name: "group", url: "VideoGroup.qml"}] }\n')
write(fixtures / 'VideoNested.qml', 'import VLC.Widgets as Widgets\nWidgets.PageLoader { pageModel: [{name: "all", default: true, nested: ' + nested('VideoDisplay.qml', 'all') + ', url: "VideoAllNested.qml"}] }\n')
write(fixtures / 'Harness.qml', '''import QtQuick
import QtQuick.Window
import VLC.Widgets as Widgets
import VLC.MainInterface
Window {
    id: window
    width: 960; height: 540; visible: false
    property bool lastLoadSucceeded: false
    property var lastLoadResult: "unstarted"
    property var leafItem: null
    property var topItem: loader.currentItem
    function findLeaf(item) {
        while (item && typeof item.loadView === "function" && item.currentItem) item = item.currentItem;
        return item;
    }
    function reload(path, properties) {
        lastLoadSucceeded = false;
        lastLoadResult = "threw-or-incomplete";
        const result = loader.loadView(path, properties, Qt.OtherFocusReason);
        lastLoadResult = result;
        lastLoadSucceeded = result === true;
        leafItem = findLeaf(loader.currentItem);
    }
    Widgets.PageLoader {
        id: loader; anchors.fill: parent
        pageModel: [
            {name: "home", nested: HOME, url: MainCtx.mediaLibraryAvailable ? "HomeNested.qml" : "NoMlHome.qml"},
            {name: "video", nested: VIDEO, url: "VideoNested.qml"},
            {name: "playlists", nested: PLAYLISTS, url: "PlaylistNested.qml"},
            {name: "failing", nested: true, url: "Failing.qml"}
        ]
    }
    Connections { target: History; function onNavigate(reason) { window.reload(History.viewPath, History.viewProp) } }
}
'''.replace('HOME', nested('MainDisplay.qml', 'home')).replace('VIDEO', nested('MainDisplay.qml', 'video')).replace('PLAYLISTS', nested('MainDisplay.qml', 'playlists')))
for fixture in fixtures.glob('*.qml'):
    text = fixture.read_text(encoding='utf-8')
    # Production registries use absolute qrc URLs; fixture URLs must likewise
    # remain rooted in their declaration, not PageLoader's helper directory.
    text = re.sub(r'url:\s*"([^"]+)"', r'url: Qt.resolvedUrl("\1")', text)
    text = text.replace('url: MainCtx.mediaLibraryAvailable ? "HomeNested.qml" : "NoMlHome.qml"',
                        'url: Qt.resolvedUrl(MainCtx.mediaLibraryAvailable ? "HomeNested.qml" : "NoMlHome.qml")')
    fixture.write_text(text, encoding='utf-8', newline='\n')

engine = QQmlApplicationEngine(); engine.addImportPath(str(modules))
engine.load(str(fixtures / 'Harness.qml'))
completed = []
def settle():
    for _ in range(4): app.processEvents()
def loaded(role):
    settle(); assert window.property('lastLoadSucceeded'), (role, messages)
    item = window.property('leafItem'); assert item and item.property('routeRole') == role
    return item
def navigate(path, props=None): history.push(path, props); settle()
failed = None
try:
    assert len(engine.rootObjects()) == 1, messages
    window = engine.rootObjects()[0]
    navigate(['video', 'all', 'base'], {'initialIndex': 7})
    item = loaded('video-base')
    assert item.property('initialIndex') == 7
    view = item.findChild(QObject, 'actualViewLoader')
    assert view.property('currentItem').property('currentIndex') == 7
    assert [index.row() for index in view.property('selectionModel').selectedIndexes()] == [7]
    completed.append('expandedVideoLeafIndex')
    selection = view.property('selectionModel')
    selection.select(selection.model().index(9, 0), QItemSelectionModel.ClearAndSelect)
    view.property('currentItem').setProperty('currentIndex', 9)
    context.gridView = False; settle()
    assert view.property('currentItem').property('currentIndex') == 9
    assert [index.row() for index in selection.selectedIndexes()] == [9]
    completed.append('pendingInitialResetDoesNotReplayAfterGridSwitch')
    model = selection.model()
    model.loading = True
    view.setProperty('initialIndex', 8)
    model.size = 14  # countChanged while loading remains true, after onLoaded
    settle()
    assert view.property('_pendingInitialIndexReset')
    assert view.property('currentItem').property('currentIndex') == 9
    model.loading = False; settle()
    assert not view.property('_pendingInitialIndexReset')
    assert view.property('currentItem').property('currentIndex') == 8
    assert [index.row() for index in selection.selectedIndexes()] == [8]
    completed.append('countBeforeLoadingSettlesConsumesPendingIndex')
    navigate(['playlists'], {'initialIndex': 0})
    item = loaded('playlist-all'); assert item.property('initialIndex') == 0
    assert history.viewPath == ['playlists', 'all']
    completed.append('defaultPlaylistLeafZeroIndex')
    navigate(['playlists', 'list'], {'parentId': 29, 'title': 'Empty playlist', 'initialIndex': -1})
    item = loaded('playlist-list')
    assert item.property('parentId') == item.property('constructionId') == 29
    assert item.property('title') == 'Empty playlist' and item.property('initialIndex') == -1
    completed.append('genuineIdTitleBeforeConstruction')
    history.previous(); item = loaded('playlist-all'); assert item.property('initialIndex') == 0
    history.next(); item = loaded('playlist-list'); assert item.property('parentId') == 29
    completed.append('adapterBackForwardLeafRestoration')
    navigate(['home']); item = loaded('home-base')
    assert item.metaObject().indexOfProperty('initialIndex') == -1
    assert not history.viewProp.contains('initialIndex')
    completed.append('playlistToRealNoIndexHomeContract')
    navigate(['video', 'all', 'group'], {'parentId': 41, 'title': 'Group', 'initialIndex': 6})
    item = loaded('video-group'); assert item.property('parentId') == item.property('constructionId') == 41
    assert item.property('initialIndex') == 6
    completed.append('sameParentReloadAndGroupAlias')
    navigate(['failing', 'child']); assert not window.property('lastLoadSucceeded')
    navigate(['failing', 'child']); assert not window.property('lastLoadSucceeded')
    completed.append('explicitChildFalseNewAndReusedParent')
    start = len(messages)
    navigate(['home', 'base'], {'doesNotExist': 4})
    assert not window.property('lastLoadSucceeded')
    assert window.property('lastLoadResult') is False
    expected_unknown = messages[start:]
    assert any('doesNotExist' in text for text in expected_unknown), expected_unknown
    expected_failures.append(dict(case='unknownLeafProperty', messages=expected_unknown))
    completed.append('unknownLeafPropertyFailsWithoutDropping')
    messages[start:] = []
    context.mediaLibraryAvailable = False
    navigate(['home']); loaded('home-no-ml')
    assert history.viewPath == ['home']
    completed.append('conditionalNoMlLeaf')
    assert not messages, messages
except Exception:
    failed = traceback.format_exc()
finally:
    record = dict(passed=failed is None, mode=args.mode, completed=completed,
        failure=failed, messages=messages, allMessages=all_messages,
        expectedFailures=expected_failures, sourceSha256=sources,
        helperSha256=digest(Path(__file__).read_bytes()),
        scope='Unchanged production PageLoader/StackViewExt/MainViewLoader; exact wrapper alias declarations and nesting registry values extracted into dependency-adapted fixtures. Supplied Python History/Navigation/model/style adapters; real Qt property maps and item-selection model. No actual C++ NavigationHistory, native MediaLibrary wrappers, primary playback or desktop navigation acceptance.',
        fullNativeIntegrationAccepted=False)
    record['negativeBaselineReproduced'] = bool(args.mode == 'baseline' and failed and any('initialIndex' in text for text in messages))
    (output / 'verification.json').write_text(json.dumps(record, indent=2) + '\n', encoding='utf-8', newline='\n')
    (output / 'messages.txt').write_text('\n'.join(all_messages) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps({key: record[key] for key in ['passed', 'mode', 'completed', 'negativeBaselineReproduced', 'failure', 'messages']}, indent=2))
    if failed and not record['negativeBaselineReproduced']: raise SystemExit(1)
