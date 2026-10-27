# SPDX-License-Identifier: GPL-2.0-or-later
"""Exercise production tile/startup callbacks with owned routing dependencies."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import sys

os.environ['QT_QPA_PLATFORM'] = 'offscreen'
os.environ['QT_QUICK_BACKEND'] = 'software'
from PySide6.QtCore import QObject, Property, Signal, Slot, QUrl, Qt, qInstallMessageHandler
from PySide6.QtGui import QGuiApplication
from PySide6.QtQml import QQmlComponent, QQmlEngine, QQmlExpression, QQmlPropertyMap, qmlContext

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--repo-root', type=Path, required=True)
parser.add_argument('--output-dir', type=Path, required=True)
parser.add_argument('--mode', choices=['baseline', 'fixed'], required=True)
args = parser.parse_args()
repo, out = args.repo_root.resolve(), args.output_dir.resolve()
out.mkdir(parents=True, exist_ok=False)
sources = {}
def source(name):
    data = (repo / name).read_bytes()
    sources[name] = hashlib.sha256(data).hexdigest()
    return data.decode('utf-8')

def block(text, anchor):
    start = text.index('{', text.index(anchor))
    depth, quote, escaped = 0, None, False
    for end in range(start, len(text)):
        ch = text[end]
        if quote:
            if escaped: escaped = False
            elif ch == '\\': escaped = True
            elif ch == quote: quote = None
        elif ch in ['"', "'"]: quote = ch
        elif ch == '{': depth += 1
        elif ch == '}':
            depth -= 1
            if depth == 0: return text[start:end + 1]
    raise AssertionError('Unterminated production callback')

startup = block(source('modules/gui/qt/maininterface/qml/MainDisplay.qml'), 'Component.onCompleted:')
tile = block(source('modules/gui/qt/player/qml/ControlBar.qml'), 'onTileRequested: (tile, item) =>')
header = source('modules/gui/qt/player/control_list_model.hpp')
body = header.split('enum ControlType')[1].split('{', 1)[1].split('};', 1)[0]
body = re.sub(r'//[^\n]*', '', body)
ids, value = {}, -1
for entry in body.split(','):
    entry = entry.strip()
    if not entry: continue
    if '=' in entry:
        name, raw = entry.split('=', 1); value = int(raw.strip(), 0)
    else: name = entry; value += 1
    ids[name.strip()] = value

class HistoryAdapter(QObject):
    changed = Signal()
    def __init__(self): super().__init__(); self.frames = []
    def seed(self, path, props=None):
        self.frames = []
        if path is not None: self.push(path, props or {}, Qt.OtherFocusReason.value)
        else: self.changed.emit()
    @Property('QStringList', notify=changed)
    def viewPath(self): return self.frames[-1][0] if self.frames else []
    @Property(QObject, notify=changed)
    def viewProp(self): return self.frames[-1][1] if self.frames else None
    @Property(bool, notify=changed)
    def previousEmpty(self): return len(self.frames) < 2
    @Slot('QStringList')
    def update(self, path):
        if not self.frames: self.push(path, {}, Qt.OtherFocusReason.value)
        else: self.frames[-1] = (list(path), self.frames[-1][1]); self.changed.emit()
    @Slot('QStringList', 'QVariantMap', int)
    def push(self, path, props, reason):
        mapping = QQmlPropertyMap(self)
        for key, value in props.items(): mapping.insert(key, value)
        self.frames.append((list(path), mapping)); self.changed.emit()

class ContextAdapter(QObject):
    changed = Signal()
    def __init__(self): super().__init__(); self._player = True
    @Property(bool, constant=True)
    def canShowVideoPIP(self): return False
    @Property(bool, constant=True)
    def hasEmbededVideo(self): return False
    @Property(bool, notify=changed)
    def playerView(self): return self._player
    @playerView.setter
    def playerView(self, value): self._player = value; self.changed.emit()

class ProviderAdapter(QObject):
    def __init__(self): super().__init__(); self.quit_calls = 0; self.stop_calls = 0
    @Slot()
    def quit(self): self.quit_calls += 1
    @Slot()
    def stop(self): self.stop_calls += 1

messages = []
qInstallMessageHandler(lambda kind, context, text: messages.append(text))
app = QGuiApplication([])
engine = QQmlEngine()
history, context, provider = HistoryAdapter(), ContextAdapter(), ProviderAdapter()
for name, value in [('History', history), ('MainCtx', context), ('DialogsProvider', provider),
                    ('MainPlaylistController', provider), ('ControlListModel', ids)]:
    engine.rootContext().setContextProperty(name, value)
components, roots = [], []
def create(callback=False):
    text = '''import QtQuick
Item {
    property var loadedPath: []
    property bool actionsEnabled: true
    property bool actionsBusy: false
    function tileState(tile) { return ({enabled: actionsEnabled, busy: actionsBusy}) }
    function loadCurrentHistoryView(reason) { loadedPath = [...History.viewPath] }
    function activateTile(tile, item) TILE
    STARTUP
}
'''.replace('TILE', tile).replace('STARTUP', 'Component.onCompleted: ' + startup if callback else '')
    component = QQmlComponent(engine)
    component.setData(text.encode('utf-8'), QUrl('file:///OwnedCustomizerRoutes.qml'))
    root = component.create()
    assert root is not None, [error.toString() for error in component.errors()]
    components.append(component); roots.append(root); app.processEvents()
    return root

cases = []
def check(name, passed, detail=''):
    cases.append(dict(name=name, passed=bool(passed), detail=detail))
def loaded(root): return root.property('loadedPath').toVariant()
def activate(root, name):
    expression = QQmlExpression(qmlContext(root), root, f'activateTile({ids[name]}, null)')
    expression.evaluate()
    return expression.error().toString() if expression.hasError() else ''

history.seed(None)
root = create(True)
check('emptyHistoryDefaultsToHome', history.viewPath == ['home'] and loaded(root) == ['home'])
for name, path, props in [
    ('firstPlaylistsPreserved', ['playlists'], {}),
    ('firstPlaylistDetailPreserved', ['playlists', 'list'], {'parentId': 29, 'title': 'Owned collection'}),
    ('firstArtistDetailPreserved', ['music', 'artists', 'albums'], {'artistId': 13}),
    ('firstHomePreserved', ['home'], {})]:
    history.seed(path, props); original = history.viewProp
    assert history.previousEmpty
    root = create(True)
    check(name, history.viewPath == path and loaded(root) == path and history.viewProp is original
          and all(original.value(key) == value for key, value in props.items()), str(history.viewPath))

history.seed(None); context.playerView = True
root = create()
error = activate(root, 'JUXTO_PLAYLISTS')
destination = create(True)
check('firstPlaylistsTileSurvivesLibraryConstruction', not error and not context.playerView
      and history.viewPath == ['playlists'] and loaded(destination) == ['playlists'], error or str(history.viewPath))

history.seed(['playlists']); context.playerView = True
root = create(); error = activate(root, 'JUXTO_GO_HOME'); destination = create(True)
check('homeTileReturnsFromAnotherRoute', not error and not context.playerView and loaded(destination) == ['home'], error)

root = create(); before = provider.quit_calls; error = activate(root, 'QUIT_BUTTON')
check('quitTileCallsNativeQuit', not error and provider.quit_calls == before + 1, error)
for name, key in [('disabledTileCannotQuit', 'actionsEnabled'), ('busyTileCannotQuit', 'actionsBusy')]:
    root = create(); root.setProperty(key, key == 'actionsBusy')
    before = provider.quit_calls; error = activate(root, 'QUIT_BUTTON')
    check(name, not error and provider.quit_calls == before, error)

expected = {'firstPlaylistsPreserved', 'firstPlaylistDetailPreserved', 'firstArtistDetailPreserved',
            'firstPlaylistsTileSurvivesLibraryConstruction', 'quitTileCallsNativeQuit'}
failed = {case['name'] for case in cases if not case['passed']}
record = dict(passed=not failed and not messages, mode=args.mode, cases=cases, messages=messages,
    negativeBaselineReproduced=args.mode == 'baseline' and failed == expected and not messages,
    sourceSha256=sources, helperSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
    scope='Extracted production MainDisplay completion and ControlBar tile callbacks with supplied History/MainCtx/provider/enum adapters. Real Qt property maps and callback execution; no native navigation, playback, full GUI or final-package acceptance.',
    fullBetaAccepted=False)
(out/'verification.json').write_text(json.dumps(record, indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps(record, indent=2), flush=True)
raise SystemExit(0 if record['passed'] or record['negativeBaselineReproduced'] else 1)
