# SPDX-License-Identifier: GPL-2.0-or-later
"""Exact production routing bodies with Qt QML signal inheritance and supplied dependencies.

Windowless QCore fixture. It does not deliver native mouse/key/accessibility events,
render a grid, construct the ML engine, or establish primary-player lifetime.
"""
import argparse
import hashlib
import json
import re
from pathlib import Path

from PySide6.QtCore import QCoreApplication, QObject, Property, Signal, Slot, Qt, QUrl, QEvent, qInstallMessageHandler
from PySide6.QtQml import QQmlEngine, QQmlComponent, QQmlExpression, qmlContext, QJSValue

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--repo-root', type=Path, required=True)
parser.add_argument('--overlay', type=Path)
parser.add_argument('--mode', choices=['baseline', 'fixed'], required=True)
parser.add_argument('--output-dir', type=Path, required=True)
args = parser.parse_args()
repo, output = args.repo_root.resolve(), args.output_dir.resolve()
overlay = args.overlay.resolve() if args.overlay else None
output.mkdir(parents=True, exist_ok=False)
messages, all_messages, source_hashes, cases = [], [], {}, []
def record_message(kind, context, text):
    messages.append(text)
    all_messages.append(text)
qInstallMessageHandler(record_message)
app = QCoreApplication([])


def source(name):
    selected = overlay / name if overlay and (overlay / name).is_file() else repo / name
    data = selected.read_bytes()
    source_hashes[name] = hashlib.sha256(data).hexdigest()
    target = output / 'source-snapshots' / name
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(data)
    return data.decode('utf-8')


def block(text, pattern, start=0):
    match = re.search(pattern, text[start:])
    assert match, pattern
    first = start + match.end() - 1
    assert text[first] == '{'
    depth, quote, escape, line_comment, long_comment = 0, None, False, False, False
    for i in range(first, len(text)):
        char, following = text[i], text[i + 1:i + 2]
        if line_comment:
            if char == '\n': line_comment = False
            continue
        if long_comment:
            if char == '*' and following == '/': long_comment = False
            continue
        if quote:
            if escape: escape = False
            elif char == '\\': escape = True
            elif char == quote: quote = None
            continue
        if char in "\"'`": quote = char
        elif char == '/' and following == '/': line_comment = True
        elif char == '/' and following == '*': long_comment = True
        elif char == '{': depth += 1
        elif char == '}':
            depth -= 1
            if depth == 0: return text[first + 1:i], first, i + 1
    raise AssertionError('Unterminated production body: ' + pattern)


grid = source('modules/gui/qt/widgets/qml/GridItem.qml')
video = source('modules/gui/qt/medialibrary/qml/VideoGridItem.qml')
playlists = source('modules/gui/qt/medialibrary/qml/PlaylistMediaList.qml')
routes = source('modules/gui/qt/medialibrary/qml/VideoPlaylistsDisplay.qml')
play_body = block(video, r'function\s+play\(\)\s*\{')[0]
parent_play_handler = re.search(r'^\s*(onPlayClicked:\s*.+)$', video, re.MULTILINE)
assert parent_play_handler
playlist_delegate = block(playlists, r'delegate:\s*VideoGridItem\s*\{')[0]
child_play_handler = re.search(r'^\s*(onPlayClicked:\s*.+)$', playlist_delegate, re.MULTILINE)
# The repaired delegate inherits the single base handler without a second request.
double_handler = re.search(r'^\s*onItemDoubleClicked:\s*(.+)$', playlist_delegate, re.MULTILINE)
assert double_handler
accessible_body = re.search(r'^\s*Accessible.onPressAction:\s*(.+)$', grid, re.MULTILINE)
assert accessible_body
pointer_double_body = block(grid, r'onDoubleTapped:\s*\(eventPoint, button\)\s*=>\s*\{')[0]
cover_body = block(grid, r'onPlayIconClicked:\s*\(point\)\s*=>\s*\{')[0]
single_selection_body = block(playlists, r'function\s+_actionAtIndex\(\)\s*\{')[0]
show_list_body = block(routes, r'onShowList:\s*\(model, reason\)\s*=>\s*\{')[0]
enabled_property = re.search(r'^\s*(property bool playbackEnabled:\s*.+)$', video, re.MULTILINE)
enabled_binding = re.search(r'^\s*(playbackEnabled:\s*.+)$', playlist_delegate, re.MULTILINE)
assert bool(enabled_property) == bool(enabled_binding)


def write(name, text):
    (output / name).write_text(text, encoding='utf-8', newline='\n')


base = '''import QtQml
QtObject {
    id: root
    property var model: ({})
    signal playClicked()
    signal itemClicked(int modifier, bool select)
    signal itemDoubleClicked(int modifier)
    function play() { PLAY_BODY }
    PARENT_HANDLER
    ENABLED_PROPERTY
    function accessiblePress() { ACCESSIBLE_BODY }
    function coverPress() {
        const point = {modifiers: Qt.NoModifier}
        COVER_BODY
    }
    function pointerDoubleTap(button) {
        const point = {modifiers: Qt.NoModifier}
        const eventPoint = null
        POINTER_DOUBLE_BODY
    }
}
'''
for key, value in [('PLAY_BODY', play_body), ('PARENT_HANDLER', parent_play_handler.group(1)),
                   ('ENABLED_PROPERTY', enabled_property.group(1) if enabled_property else ''),
                   ('ACCESSIBLE_BODY', accessible_body.group(1)), ('COVER_BODY', cover_body),
                   ('POINTER_DOUBLE_BODY', pointer_double_body)]:
    base = base.replace(key, value)
write('Base.qml', base)
child = '''import QtQml
Base {
    id: root
    property var pagePrefix: ["main", "playlists"]
    signal showList(var model, int reason)
    CHILD_HANDLER
    ENABLED_BINDING
    onItemDoubleClicked: DOUBLE_HANDLER
    onShowList: (model, reason) => { SHOW_LIST_BODY }
}
'''
for key, value in [('CHILD_HANDLER', child_play_handler.group(1) if child_play_handler else ''),
                   ('ENABLED_BINDING', enabled_binding.group(1) if enabled_binding else ''),
                   ('DOUBLE_HANDLER', double_handler.group(1)), ('SHOW_LIST_BODY', show_list_body)]:
    child = child.replace(key, value)
write('Child.qml', child)
collection = '''import QtQml
QtObject {
    id: root
    property var pagePrefix: ["main", "playlists"]
    property var model: FixtureModel
    property var selectionModel: ({selectedIndexes: []})
    signal showList(var model, int reason)
    function _actionAtIndex() { SINGLE_SELECTION_BODY }
    onShowList: (model, reason) => { SHOW_LIST_BODY }
}
'''.replace('SINGLE_SELECTION_BODY', single_selection_body).replace('SHOW_LIST_BODY', show_list_body)
write('Collection.qml', collection)


def variant(value): return value.toVariant() if isinstance(value, QJSValue) else value


class ContextAdapter(QObject):
    changed = Signal()
    def __init__(self): super().__init__(); self.value = False; self.assignments = []
    @Property(bool, notify=changed)
    def playerView(self): return self.value
    @playerView.setter
    def playerView(self, value): self.value = value; self.assignments.append(value); self.changed.emit()
    def reset(self): self.value = False; self.assignments.clear(); self.changed.emit()


class MediaAdapter(QObject):
    def __init__(self): super().__init__(); self.calls = []
    @Slot('QVariant')
    def addAndPlay(self, value): self.calls.append(variant(value))


class HistoryAdapter(QObject):
    def __init__(self): super().__init__(); self.calls = []
    @Slot('QVariant', 'QVariant', int)
    def push(self, path, data, reason): self.calls.append([variant(path), variant(data), reason])


class ModelAdapter(QObject):
    def __init__(self): super().__init__(); self.calls = []; self.entry = dict(id=42, name='Empty checkpoint', count=0)
    @Slot('QVariant', result='QVariantMap')
    def getDataAt(self, value): return dict(self.entry)
    @Slot('QVariant')
    def addAndPlay(self, value): self.calls.append(variant(value))


context, media, history, model = ContextAdapter(), MediaAdapter(), HistoryAdapter(), ModelAdapter()
engine = QQmlEngine()
for name, adapter in [('MainCtx', context), ('MediaLib', media), ('History', history), ('FixtureModel', model)]:
    engine.rootContext().setContextProperty(name, adapter)
components = []


def create(name):
    component = QQmlComponent(engine, QUrl.fromLocalFile(str(output / name)))
    assert not component.isError(), [error.toString() for error in component.errors()]
    item = component.create()
    assert item, [error.toString() for error in component.errors()]
    components.append(component)
    return item


child_item, raw_item, collection_item = create('Child.qml'), create('Base.qml'), create('Collection.qml')
item_clicks = []
child_item.itemClicked.connect(lambda modifiers, select: item_clicks.append([modifiers, select]))


def evaluate(item, expression):
    code = QQmlExpression(qmlContext(item), item, expression)
    value, _ = code.evaluate()
    assert not code.hasError(), code.error().toString()
    return variant(value)


def reset(item, entry):
    context.reset(); media.calls.clear(); history.calls.clear(); model.calls.clear(); item_clicks.clear(); messages.clear()
    evaluate(item, 'model = ' + json.dumps(entry))
    app.processEvents()


def check(name, task):
    try:
        observation = task()
        assert not messages, messages
        cases.append(dict(name=name, passed=True, observation=observation))
    except Exception as error:
        cases.append(dict(name=name, passed=False, error=str(error),
            observedMediaCalls=list(media.calls), observedPlayerView=context.playerView,
            observedViewAssignments=list(context.assignments), observedHistoryCalls=list(history.calls),
            messages=list(messages)))


def empty_play(method):
    reset(child_item, dict(id=42, name='Empty checkpoint', count=0))
    evaluate(child_item, method + '()')
    assert media.calls == [] and not context.playerView and context.assignments == [], (
        'Known empty Play must preserve the library view; got calls/view/assignments',
        media.calls, context.playerView, context.assignments)
    assert history.calls == [], 'An explicit Play action is not an Open action'
    assert item_clicks == ([[Qt.NoModifier.value, True]] if method == 'coverPress' else []), item_clicks
    return dict(nativePointerDelivery=False, mediaCalls=[], playerView=False,
        coverSelectionSignals=list(item_clicks))


def open_empty(button):
    reset(child_item, dict(id=42, name='Empty checkpoint', count=0))
    evaluate(child_item, 'pointerDoubleTap(' + button + ')')
    expected = [] if button == 'Qt.RightButton' else [[['main', 'playlists', 'list'],
        dict(parentId=42, title='Empty checkpoint'), Qt.MouseFocusReason.value]]
    assert history.calls == expected, history.calls
    assert not media.calls and not context.playerView and not context.assignments
    return dict(historyCalls=list(history.calls), nativePointerDelivery=False)


def keep_play(entry, raw=False, cover=False):
    item = raw_item if raw else child_item
    reset(item, entry)
    evaluate(item, 'coverPress()' if cover else 'accessiblePress()')
    expected = [42]
    assert media.calls == expected and context.playerView and context.assignments == [True], (
        media.calls, context.playerView, context.assignments)
    if cover:
        assert item_clicks == [[Qt.NoModifier.value, True]], item_clicks
    return dict(mediaCalls=list(media.calls), playerView=True,
        coverSelectionSignals=list(item_clicks), nativeMediaEngineConstructed=False)


def selection_open(selected):
    context.reset(); media.calls.clear(); history.calls.clear(); model.calls.clear(); messages.clear()
    evaluate(collection_item, 'selectionModel = ({selectedIndexes: ' + json.dumps(selected) + '})')
    evaluate(collection_item, '_actionAtIndex()')
    if len(selected) == 1:
        assert history.calls == [[['main', 'playlists', 'list'],
            dict(parentId=42, title='Empty checkpoint'), Qt.TabFocusReason.value]]
        assert not model.calls
    else:
        assert not history.calls
        assert model.calls == ([selected] if selected else [])
    assert not media.calls and not context.playerView and not context.assignments
    return dict(historyCalls=list(history.calls), selectionPlayCalls=list(model.calls), nativeKeyDelivery=False)


check('knownZeroAccessiblePlayKeepsLibraryView', lambda: empty_play('accessiblePress'))
check('knownZeroCoverPlayKeepsLibraryView', lambda: empty_play('coverPress'))
check('knownZeroLeftDoubleTapBodyOpensDetails', lambda: open_empty('Qt.LeftButton'))
check('rightDoubleTapBodyDoesNotOpenDetails', lambda: open_empty('Qt.RightButton'))
check('filledPlaylistAccessibleDispatchesOnce', lambda: keep_play(dict(id=42, count=3)))
check('filledPlaylistCoverDispatchesOnce', lambda: keep_play(dict(id=42, count=3), cover=True))
check('unknownCountDispatchesOnce', lambda: keep_play(dict(id=42)))
check('nullCountDispatchesOnce', lambda: keep_play(dict(id=42, count=None)))
check('nonNumericZeroDispatchesOnce', lambda: keep_play(dict(id=42, count='0')))
check('nonCollectionVideoRetainsDefaultPlay', lambda: keep_play(dict(id=42, count=0), raw=True))
check('oneSelectionKeyboardBodyOpensEmptyDetails', lambda: selection_open([0]))
check('multipleSelectionKeyboardBodyRetainsModelPlay', lambda: selection_open([0, 1]))
check('noSelectionKeyboardBodyDoesNothing', lambda: selection_open([]))
# Finish all explicitly owned QML objects before freezing diagnostics.
for item in [child_item, raw_item, collection_item]: item.deleteLater()
engine.deleteLater()
QCoreApplication.sendPostedEvents(None, QEvent.DeferredDelete)
app.processEvents()
qInstallMessageHandler(None)
passed = all(case['passed'] for case in cases) and not all_messages
expected_baseline_failures = ['knownZeroAccessiblePlayKeepsLibraryView', 'knownZeroCoverPlayKeepsLibraryView',
    'filledPlaylistAccessibleDispatchesOnce', 'filledPlaylistCoverDispatchesOnce',
    'unknownCountDispatchesOnce', 'nullCountDispatchesOnce', 'nonNumericZeroDispatchesOnce']
verification = dict(passed=passed, mode=args.mode, cases=cases,
    failureNames=[case['name'] for case in cases if not case['passed']],
    expectedBaselineFailureNames=expected_baseline_failures,
    baselineShapeMatches=(not passed and [case['name'] for case in cases if not case['passed']] == expected_baseline_failures),
    sourceSha256=source_hashes,
    generatedQmlSha256={name: hashlib.sha256((output / name).read_bytes()).hexdigest()
        for name in ['Base.qml', 'Child.qml', 'Collection.qml']},
    fixtureSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
    scope='Actual production routing bodies and normal Qt QML base/derived signal connections with supplied dependencies. No window, native input delivery, actual ML/primary player, rendering, lifetime or Figma acceptance.',
    messages=list(messages), allMessages=list(all_messages),
    diagnosticsScope='QCore startup, QML creation, all actions and explicit QML-object/engine teardown; subsequent process stderr is checked by the launcher.')
write('verification.json', json.dumps(verification, ensure_ascii=False, indent=2) + '\n')
print(json.dumps(dict(passed=passed, failureNames=verification['failureNames'],
    baselineShapeMatches=verification['baselineShapeMatches'], totalCases=len(cases))))
raise SystemExit(0 if passed else 1)
