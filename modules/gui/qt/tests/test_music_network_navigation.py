# SPDX-License-Identifier: GPL-2.0-or-later
"""Music/network property and writer contracts with actual loader QML and supplied dependencies."""
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
    # Keep copied QML and hashes byte-exact, but parse either Git checkout's
    # line endings. Windows CI may materialize these sources with CRLF.
    return data.decode('utf-8').replace('\r\n', '\n')
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


from PySide6.QtQuick import QQuickItem

def qml_block(text, opening):
    start = text.index('{', opening)
    depth, quote, escape, line_comment, block_comment = 0, None, False, False, False
    for i in range(start, len(text)):
        c, nxt = text[i], text[i:i+2]
        if line_comment:
            if c == '\n': line_comment = False
            continue
        if block_comment:
            if text[i-1:i+1] == '*/': block_comment = False
            continue
        if quote:
            if escape: escape = False
            elif ord(c) == 92: escape = True
            elif c == quote: quote = None
            continue
        if nxt == '//': line_comment = True; continue
        if nxt == '/*': block_comment = True; continue
        if c in ('"', "'"): quote = c; continue
        if c == '{': depth += 1
        elif c == '}':
            depth -= 1
            if depth == 0: return text[opening:i+1]
    raise AssertionError('Unbalanced QML source block')

snapshots = {}
for name in ['medialibrary/qml/MusicAlbumsDisplay.qml', 'medialibrary/qml/MusicArtistsDisplay.qml',
        'medialibrary/qml/MusicGenresDisplay.qml', 'medialibrary/qml/MusicPlaylistsDisplay.qml',
        'network/qml/BrowseDisplay.qml', 'network/qml/BrowseDeviceView.qml',
        'network/qml/BrowseTreeDisplay.qml', 'network/qml/ServicesHomeDisplay.qml']:
    relative = 'modules/gui/qt/' + name
    snapshots[name] = save_source(relative, output / 'source-snapshots' / name)

extractions = []
def exact_alias(file, target):
    text = snapshots[file]
    matches = list(re.finditer(r'^\s*property alias initialIndex: ' + re.escape(target) + r'\.initialIndex\s*$', text, re.M))
    assert len(matches) <= 1
    if not matches: return ''
    match = matches[0]
    extractions.append(dict(file=file, line=text[:text.index('property alias', match.start())].count('\n')+1, source=match.group().strip()))
    return match.group().strip()

def exact_writer(file, signal, ordinal, component_id=None):
    text = snapshots[file]
    origin = 0
    if component_id:
        marker = re.search(r'^\s*id: ' + re.escape(component_id) + r'\s*$', text, re.M)
        assert marker, component_id
        opening = text.rfind('    Component {', 0, marker.start())
        assert opening >= 0, component_id
        body = qml_block(text, opening)
        origin = opening
        text = body
    matches = list(re.finditer(r'^\s*' + re.escape(signal) + r':', text, re.M))
    if ordinal >= len(matches): return ''
    match = matches[ordinal]
    opening = text.index(signal, match.start())
    tail = text[match.end():].lstrip()
    if tail.startswith('{'): expression = qml_block(text, opening)
    else: expression = text[opening:text.index('\n', opening)]
    extractions.append(dict(file=file, line=snapshots[file][:origin+opening].count('\n')+1, source=expression))
    return expression

fixtures = output / 'fixtures'
write(fixtures / 'Cursor.qml', '''import QtQuick
FocusScope {
    property var model: null
    property var modelAtSet: null
    property var modelAtPosition: null
    property int countAtSet: -1
    property int currentIndex: -1
    property int positionCount: 0
    function setCurrentItem(index) { modelAtSet = model; countAtSet = model?.count ?? -1; currentIndex = index }
    function positionViewAtIndex(index, mode) { modelAtPosition = model; positionCount += 1 }
    function setCurrentItemFocus(reason) { forceActiveFocus(reason) }
}
''')

routes = [
    dict(key='musicAlbums', file='medialibrary/qml/MusicAlbumsDisplay.qml', prefix=['music'], leaf='albums', direct=True,
         child='albumsView', fields={}, signal='onCurrentIndexChanged', ordinal=0),
    dict(key='artistsAll', file='medialibrary/qml/MusicArtistsDisplay.qml', prefix=['music','artists'], leaf='all',
         child='artistsView', component='allArtistsComponent', fields={}, signal='onCurrentIndexChanged', ordinal=0),
    dict(key='artistAlbums', file='medialibrary/qml/MusicArtistsDisplay.qml', prefix=['music','artists'], leaf='albums',
         child='albumSubView', artist=True, fields={'artistId':41,'initialAlbumIndex':3},
         signal='onCurrentAlbumIndexChanged', ordinal=0),
    dict(key='genresAll', file='medialibrary/qml/MusicGenresDisplay.qml', prefix=['music','genres'], leaf='all',
         child='genresView', component='genresComponent', fields={}, signal='onCurrentIndexChanged', ordinal=0),
    dict(key='genreAlbums', file='medialibrary/qml/MusicGenresDisplay.qml', prefix=['music','genres'], leaf='albums',
         child='albumsView', component='albumGenreComponent', fields={'parentId':29,'genreName':'Fixture genre'}, signal='onCurrentIndexChanged', ordinal=0),
    dict(key='musicPlaylistsAll', file='medialibrary/qml/MusicPlaylistsDisplay.qml', prefix=['music','playlists'], leaf='all',
         child='playlistView', fields={}, signal='onCurrentIndexChanged', ordinal=0),
    dict(key='musicPlaylistList', file='medialibrary/qml/MusicPlaylistsDisplay.qml', prefix=['music','playlists'], leaf='list',
         child='playlistView', existing=True, fields={'parentId':37,'title':'Fixture playlist'}, signal='onCurrentIndexChanged', ordinal=1),
    dict(key='networkFolders', file='network/qml/BrowseDisplay.qml', prefix=['network'], leaf='folders',
         child='viewFolders', device=True, fields={'title':'Fixture folders'}, signal='onCurrentIndexChanged', ordinal=0),
    dict(key='networkDevice', file='network/qml/BrowseDisplay.qml', prefix=['network'], leaf='device',
         child='viewDevice', device=True, fields={'sd_source':2,'title':'Fixture devices'}, signal='onCurrentIndexChanged', ordinal=1),
    dict(key='networkTree', file='network/qml/BrowseDisplay.qml', prefix=['network'], leaf='browse',
         child='loader', tree=True, fields={'tree':{'fixtureNode':19}}, signal='onCurrentIndexChanged', ordinal=2),
    dict(key='serviceRoot', file='network/qml/ServicesHomeDisplay.qml', prefix=['discover','services'], leaf='source_root',
         child='loader', tree=True, fields={'source_name':'fixture-source'}, signal='onCurrentIndexChanged', ordinal=0),
    dict(key='serviceBrowse', file='network/qml/ServicesHomeDisplay.qml', prefix=['discover','services'], leaf='source_browse',
         child='loader', tree=True, fields={'tree':{'fixtureNode':23},'source_name':'fixture-source'}, signal='onCurrentIndexChanged', ordinal=1),
]

device_text = snapshots['network/qml/BrowseDeviceView.qml']
start = device_text.find('    // Saved row restoration')
restoration = device_text[start:device_text.index('    // Private', start)] if start >= 0 else ''
loader_start = device_text.index('    Loader {\n        id: loader')
device_loader = qml_block(device_text, loader_start)
extractions.append(dict(file='network/qml/BrowseDeviceView.qml', source=restoration, purpose='Entire new restoration mechanism, or absent baseline'))
extractions.append(dict(file='network/qml/BrowseDeviceView.qml', source=device_loader, purpose='Unchanged Loader plus new onLoaded hook'))
write(fixtures / 'DeviceConsumer.qml', '''import QtQuick
import QtQml.Models
import VLC.MainInterface
import VLC.Util
FocusScope {
    id: root
    required property BaseModel model
    readonly property int currentIndex: _currentView?.currentIndex ?? -1
    property alias _currentView: loader.item
    property alias selection: modelSelect
    RESTORATION
    ListSelectionModel { id: modelSelect; model: root.model }
    LOADER
    Component { id: grid; Cursor { model: root.model } }
    Component { id: list; Cursor { model: root.model } }
    function testSelect(index) {
        modelSelect.select(model.index(index, 0), ItemSelectionModel.ClearAndSelect)
        _currentView.setCurrentItem(index)
    }
}
'''.replace('RESTORATION', restoration).replace('LOADER', device_loader))

for spec in routes:
    child = spec['child']
    if spec.get('tree'): alias = exact_alias('network/qml/BrowseTreeDisplay.qml', 'loader')
    elif spec.get('existing'): alias = 'property alias initialIndex: playlistView.initialIndex'
    elif spec.get('artist'): alias = 'property int initialIndex: 0\nproperty int initialAlbumIndex: 0\nproperty var artistId: undefined'
    else: alias = exact_alias(spec['file'], child)
    writer = exact_writer(spec['file'], spec['signal'], spec['ordinal'], spec.get('component'))
    fields = spec['fields']
    declarations = ''
    for key in fields:
        if key in ['title','artistId','initialAlbumIndex']: continue
        if key == 'parentId': declarations += f'property alias parentId: {child}.parentId\n'
        elif key == 'sd_source': declarations += 'required property int sd_source\n'
        elif key == 'genreName': declarations += 'property string genreName: ""\n'
        elif key == 'tree': declarations += 'property alias tree: metadata.tree\n'
        elif key == 'source_name': declarations += 'property alias source_name: metadata.source_name\n'
    probe = '{' + ','.join(f'{key}: {key}' for key in fields) + '}'
    child_body = (f'''DeviceConsumer {{ id: {child}; objectName: "cursorOwner"; model: BaseModel {{}} {writer} }}'''
        if spec.get('device') else f'''MainViewLoader {{
            id: {child}; objectName: "cursorOwner"
            property var parentId: -1
            readonly property int currentIndex: currentItem?.currentIndex ?? -1
            model: BaseModel {{}}
            grid: Component {{ Cursor {{}} }}
            list: Component {{ Cursor {{}} }}
            emptyLabel: Component {{ Item {{}} }}
            CHILD_WRITER
        }}'''.replace('CHILD_WRITER', '' if spec.get('tree') or spec.get('artist') or spec.get('existing') else writer))
    leaf_writer = writer if spec.get('tree') or spec.get('artist') or spec.get('existing') else ''
    if spec.get('artist'):
        leaf_writer += '\n' + exact_writer(spec['file'], 'onArtistIdChanged', 0)
        current = f'property alias currentAlbumIndex: {child}.currentIndex'
        child_body = child_body.replace('CHILD_WRITER','')
    else: current = f'readonly property int currentIndex: {child}.currentIndex'
    leaf = f'''T.Page {{
        id: {'root' if spec.get('direct') else 'leaf'}
        property var pagePrefix: []
        property string constructionValues: ""
        {alias}
        {declarations}
        {current}
        QtObject {{ id: metadata; property var tree; property string source_name: "" }}
        Component.onCompleted: constructionValues = JSON.stringify({probe})
        {child_body}
        {leaf_writer}
        function testSetIndex(index) {{ {child + '.testSelect(index)' if spec.get('device') else child + '.currentItem.setCurrentItem(index)'} }}
        function setCurrentItemFocus(reason) {{ {child}.setCurrentItemFocus(reason) }}
        function testSetArtist(value) {{ artistId = value }}
    }}'''
    # Artist metadata/reset implementation is supplied; only its exact parent
    # writer bodies and unchanged distinct property values are under test.
    owner_id = 'routeOwner' if spec.get('direct') else 'root'
    content = f'''import QtQuick
import QtQuick.Templates as T
import VLC.MainInterface
import VLC.Util
import VLC.Widgets as Widgets
Widgets.PageLoader {{
    id: {owner_id}
    property bool testLoaded: false
    pagePrefix: {json.dumps(spec['prefix'])}
    pageModel: [{{name: "{spec['leaf']}", default: true, component: routeLeaf}}]
    Component {{ id: routeLeaf; {leaf} }}
    function testLoad() {{ testLoaded = loadView(["{spec['leaf']}"], History.viewProp, Qt.OtherFocusReason) }}
}}\n'''
    write(fixtures / (spec['key'] + '.qml'), content)

engine = QQmlApplicationEngine(); engine.addImportPath(str(modules))
objects = []
owned_models = []

def owned_model():
    model=BaseModelAdapter()
    owned_models.append(model)
    return model
def settle():
    for _ in range(5): app.processEvents()
def component(filename, initial=None):
    from PySide6.QtQml import QQmlComponent
    from PySide6.QtCore import QUrl
    comp = QQmlComponent(engine, QUrl.fromLocalFile(str(fixtures / filename)))
    if comp.isError(): raise AssertionError([error.toString() for error in comp.errors()])
    obj = comp.createWithInitialProperties(initial or {})
    assert obj, [error.toString() for error in comp.errors()]
    objects.append((comp,obj)); settle()
    return obj
def invoke(obj, function, value=None):
    from PySide6.QtCore import QMetaObject, Q_ARG
    if value is None: result = QMetaObject.invokeMethod(obj, function)
    else: result = QMetaObject.invokeMethod(obj, function, Q_ARG('QVariant',value))
    assert result, function
    settle()
def load(spec, initial):
    obj=component(spec['key']+'.qml')
    history.push(spec['prefix']+[spec['leaf']], initial)
    invoke(obj,'testLoad')
    leaf=obj.property('currentItem')
    observed['loaderReturned']=obj.property('testLoaded')
    observed['initialPropertyExists']=bool(leaf and leaf.metaObject().indexOfProperty('initialIndex')>=0)
    assert obj.property('testLoaded'), 'Actual PageLoader returned false'
    settle(); assert leaf
    return obj,leaf

cases=[]
observed={}
def check(name, function):
    global observed
    observed={}
    before=len(messages)
    try: function(); error=None
    except Exception: error=traceback.format_exc()
    new_messages=messages[before:]
    cases.append(dict(name=name,passed=error is None and not new_messages,error=error,messages=new_messages,observed=observed))
    for comp,obj in objects: obj.deleteLater()
    objects.clear()
    from PySide6.QtCore import QCoreApplication, QEvent
    QCoreApplication.sendPostedEvents(None,QEvent.DeferredDelete)
    settle()

def restoration_case(spec):
    obj,leaf=load(spec,dict(spec['fields'],initialIndex=4))
    owner=leaf.findChild(QObject,'cursorOwner')
    assert leaf.metaObject().indexOfProperty('initialIndex')>=0
    assert leaf.property('initialIndex')==4 and owner.property('currentIndex')==4
    selection=owner.findChild(QItemSelectionModel)
    assert selection
    observed.update(actualIndex=owner.property('currentIndex'),selectedRows=[index.row() for index in selection.selectedIndexes()],
        constructorValues=json.loads(leaf.property('constructionValues')),initialIndex=leaf.property('initialIndex'))
    assert [index.row() for index in selection.selectedIndexes()]==[4]
    assert json.loads(leaf.property('constructionValues'))==spec['fields']

for spec in routes:
    if not spec.get('artist') and not spec.get('existing'):
        check(spec['key']+'ActualAliasRestoresRow',lambda spec=spec:restoration_case(spec))

def guarded_case(spec, artist=False):
    obj,leaf=load(spec,spec['fields'])
    field='artistId' if artist else 'initialAlbumIndex' if spec.get('artist') else 'initialIndex'
    invoke(leaf,'testSetArtist' if artist else 'testSetIndex',83 if artist else 6)
    observed.update(field=field,ownRoute=history.viewPath,ownValue=history.viewProp.value(field))
    assert history.viewProp.value(field)==(83 if artist else 6)
    history.push(['home','base'])
    invoke(leaf,'testSetArtist' if artist else 'testSetIndex',84 if artist else 7)
    observed.update(foreignRoute=history.viewPath,foreignHasField=history.viewProp.contains(field),foreignValue=history.viewProp.value(field))
    assert not history.viewProp.contains(field)
    assert not history.viewProp.contains('initialIndex') and not history.viewProp.contains('initialAlbumIndex')
    assert not history.viewProp.contains('artistId')

for spec in routes:
    check(spec['key']+'OwnRouteThenForeignRouteWriter',lambda spec=spec:guarded_case(spec))
    if spec.get('artist'): check('artistIdOwnRouteThenForeignRouteWriter',lambda spec=spec:guarded_case(spec,True))

def device_ready():
    model=owned_model()
    obj=component('DeviceConsumer.qml',{'model':model,'initialIndex':4})
    observed.update(initialPropertyExists=obj.metaObject().indexOfProperty('initialIndex')>=0,actualIndex=obj.property('currentIndex'))
    assert obj.property('currentIndex')==4
    assert [x.row() for x in obj.findChild(QItemSelectionModel).selectedIndexes()]==[4]
    assert not obj.property('_pendingInitialIndexReset')
    invoke(obj,'testSelect',7); context.gridView=not context.gridView;settle()
    assert obj.property('currentIndex')==7
    assert [x.row() for x in obj.findChild(QItemSelectionModel).selectedIndexes()]==[7]
    assert obj.property('initialIndex')==4
    observed.update(consumedOnce=True,laterUserIndex=obj.property('currentIndex'),selectedRows=[x.row() for x in obj.findChild(QItemSelectionModel).selectedIndexes()])

def device_loading():
    model=owned_model(); model.loading=True;model.size=0
    obj=component('DeviceConsumer.qml',{'model':model,'initialIndex':4})
    observed.update(initialPropertyExists=obj.metaObject().indexOfProperty('initialIndex')>=0,
        viewLoaded=bool(obj.property('_currentView')),pendingBeforeCount=obj.property('_pendingInitialIndexReset'))
    assert obj.property('_currentView') and obj.property('_pendingInitialIndexReset')
    model.size=12;settle()
    assert obj.property('_pendingInitialIndexReset') and obj.property('currentIndex')!=4
    model.loading=False;settle()
    assert not obj.property('_pendingInitialIndexReset') and obj.property('currentIndex')==4
    assert [x.row() for x in obj.findChild(QItemSelectionModel).selectedIndexes()]==[4]
    observed.update(restoredAfterLoadingFalse=obj.property('currentIndex'),selectedRows=[x.row() for x in obj.findChild(QItemSelectionModel).selectedIndexes()])

def device_replacement():
    model=owned_model();obj=component('DeviceConsumer.qml',{'model':model,'initialIndex':4})
    observed.update(initialPropertyExists=obj.metaObject().indexOfProperty('initialIndex')>=0)
    replacement=owned_model();replacement.size=3
    assert obj.setProperty('model',replacement);settle()
    selection=obj.findChild(QItemSelectionModel)
    view=obj.property('_currentView')
    observed.update(selectionModelIsReplacement=selection.model()==replacement,replacementCursor=obj.property('currentIndex'),
        replacementSelectedRows=[x.row() for x in selection.selectedIndexes()],
        viewModelAtSetIsReplacement=view.property('modelAtSet')==replacement,
        viewModelAtPositionIsReplacement=view.property('modelAtPosition')==replacement,viewCountAtSet=view.property('countAtSet'))
    assert selection.model()==replacement
    assert view.property('modelAtSet')==replacement and view.property('modelAtPosition')==replacement
    assert view.property('countAtSet')==3
    assert not obj.property('_pendingInitialIndexReset') and obj.property('currentIndex')==0
    assert [x.row() for x in selection.selectedIndexes()]==[0]

def device_no_request():
    model=owned_model();obj=component('DeviceConsumer.qml',{'model':model})
    observed.update(initialPropertyExists=obj.metaObject().indexOfProperty('initialIndex')>=0)
    assert obj.metaObject().indexOfProperty('initialIndex')>=0
    assert obj.property('initialIndex')==-1 and not obj.property('_pendingInitialIndexReset')
    assert not obj.findChild(QItemSelectionModel).hasSelection()
    invoke(obj,'testSelect',8);context.gridView=not context.gridView;settle()
    assert obj.property('currentIndex')==8 and obj.property('initialIndex')==-1
    observed.update(noRequest=True,laterUserIndex=obj.property('currentIndex'),selectedRows=[x.row() for x in obj.findChild(QItemSelectionModel).selectedIndexes()])

for name,function in [('deviceConstructorBeforeLoaded',device_ready),('deviceCountBeforeLoadingFalse',device_loading),
        ('deviceReadyModelReplacement',device_replacement),('deviceNoRequestAndUserViewSwitch',device_no_request)]:check(name,function)

failed=[case['name'] for case in cases if not case['passed']]
def expected_baseline(case):
    name,obs,msg=case['name'],case['observed'],case['messages']
    if case['passed'] or not case['error'] or 'AssertionError' not in case['error']: return False
    if name.endswith('ActualAliasRestoresRow'):
        return (obs==dict(loaderReturned=False,initialPropertyExists=False) and len(msg)==2
            and msg[0].endswith('Error: Cannot assign to non-existent property "initialIndex"')
            and msg[1].startswith('unknown initial property initialIndex of view '))
    if name.endswith('OwnRouteThenForeignRouteWriter'):
        if msg or obs.get('loaderReturned') is not True: return False
        if name in ['artistsAllOwnRouteThenForeignRouteWriter','genresAllOwnRouteThenForeignRouteWriter']:
            return obs.get('ownValue') is None and 'foreignRoute' not in obs
        return (obs.get('ownValue')==(83 if name.startswith('artistId') else 6)
            and obs.get('foreignHasField') is True and obs.get('foreignValue')==(84 if name.startswith('artistId') else 7)
            and obs.get('foreignRoute')==['home','base'])
    if name=='deviceNoRequestAndUserViewSwitch':
        return obs.get('initialPropertyExists') is False and not msg
    if name in ['deviceConstructorBeforeLoaded','deviceCountBeforeLoadingFalse','deviceReadyModelReplacement']:
        return (obs.get('initialPropertyExists') is False and len(msg)==1
            and msg[0].endswith('Setting initial properties failed: DeviceConsumer does not have a property called initialIndex'))
    return False

negative_reproduced=(len(cases)==27 and all(expected_baseline(case) for case in cases) and len(messages)==23)
record=dict(passed=not failed,mode=args.mode,cases=cases,failedCases=failed,messages=messages,
    sourceSha256=sources,helperSha256=digest(Path(__file__).read_bytes()),extractions=extractions,
    negativeBaselineReproduced=args.mode=='baseline' and negative_reproduced,
    scope='Actual unmodified PageLoader/StackViewExt/MainViewLoader; exact production aliases/writer bodies and entire device restoration/Loader blocks extracted into supplied History/model/style/view fixtures. Real Qt property maps, model indices and item selection model; no native ML/network models, mounted desktop, pointer/focus, same-route generations or design fidelity acceptance.',
    fullNativeIntegrationAccepted=False)
(output/'verification.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8',newline='\n')
(output/'messages.txt').write_text('\n'.join(messages)+'\n',encoding='utf-8',newline='\n')
print(json.dumps({'passed':record['passed'],'negativeBaselineReproduced':record['negativeBaselineReproduced'],
    'failedCases':failed,'caseCount':len(cases),'messageCount':len(messages)},indent=2))
if len(cases)!=27 or (args.mode=='baseline' and not negative_reproduced):raise SystemExit(2)
if failed or messages:raise SystemExit(1)
