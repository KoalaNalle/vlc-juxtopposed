# Copyright (C) 2026 VLC authors and VideoLAN
# Test code licensed under GPLv2 or later.
"""Exercise the queue's real QML callbacks with Qt's real filter proxy.

Run with PySide6-Essentials 6.8.3 or later; a VLC binary is not required.
The small harness replaces the native playback controller, not the mapping
implementation, and extracts callbacks from their production QML files.
"""
import os
from enum import IntEnum
from pathlib import Path
import re
import subprocess

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
os.environ["QML_DISABLE_DISK_CACHE"] = "1"

import PySide6
from PySide6.QtCore import (QAbstractProxyModel, QCoreApplication, QObject, QEnum, QItemSelectionModel, QModelIndex,
                           Property, QSortFilterProxyModel, Signal, Slot, Qt, QUrl)
from PySide6.QtGui import QStandardItem, QStandardItemModel
from PySide6.QtQml import (QQmlComponent, QQmlEngine, QQmlExpression,
                          qmlRegisterType, qmlRegisterUncreatableType)
from PySide6.QtTest import QSignalSpy


def qml_block(source, pattern):
    """Read a QML/JS block, respecting strings and both comment forms."""
    match = re.search(pattern, source)
    assert match, pattern
    start = source.index("{", match.start())
    depth, position = 1, start + 1
    while depth:
        assert position < len(source), pattern
        character = source[position]
        if source.startswith("//", position):
            end = source.find("\n", position)
            position = len(source) if end < 0 else end + 1
            continue
        if source.startswith("/*", position):
            end = source.find("*/", position + 2)
            assert end >= 0, pattern
            position = end + 2
            continue
        if character in "\"'`":
            quote = character
            position += 1
            while position < len(source):
                if source[position] == "\\":
                    position += 2
                elif source[position] == quote:
                    position += 1
                    break
                else:
                    position += 1
            continue
        depth += (character == "{") - (character == "}")
        position += 1
    return source[match.start():position], source[start + 1:position - 1]


def qml_function(source, name):
    return qml_block(source, rf"\bfunction\s+{name}\s*\(")[0]


class PlaylistSource(QStandardItemModel):
    countChanged = Signal()
    class Roles(IntEnum):
        DurationRole = Qt.UserRole + 1
        IsCurrentRole = Qt.UserRole + 2
    QEnum(Roles)

    def __init__(self):
        super().__init__()
        self.moves = []
        self.removals = []
        self.rowsInserted.connect(lambda *_: self.countChanged.emit())
        self.rowsRemoved.connect(lambda *_: self.countChanged.emit())
        self.modelReset.connect(self.countChanged.emit)

    @Property(int, notify=countChanged)
    def count(self):
        return self.rowCount()

    def reset(self):
        self.clear()
        for title in ["Hidden before", "Visible A", "Hidden middle",
                      "Visible B", "Hidden tail"]:
            self.appendRow(QStandardItem(title))
        self.moves.clear()
        self.removals.clear()

    def reset_same_rows(self):
        # Native playlist/bookmark resets invalidate selection without emitting
        # selectionChanged; clearing this test model would emit that signal.
        self.beginResetModel()
        self.endResetModel()

    @Slot("QVariantList", int)
    def moveItemsPre(self, indexes, target):
        self.moves.append((list(indexes), target))
        # Model the playlist API's pre-move insertion position.
        items = [self.takeRow(index) for index in reversed(indexes)]
        destination = target - sum(index < target for index in indexes)
        for offset, item in enumerate(reversed(items)):
            self.insertRow(destination + offset, item)

    @Slot("QVariantList")
    def removeItems(self, indexes):
        self.removals.append(list(indexes))
        for index in reversed(indexes):
            self.removeRow(index)

    def titles(self):
        return [self.item(index).text() for index in range(self.rowCount())]


class PlaybackController(QObject):
    class PlaybackRepeat(IntEnum):
        PLAYBACK_REPEAT_NONE = 0
        PLAYBACK_REPEAT_CURRENT = 1
        PLAYBACK_REPEAT_ALL = 2
    QEnum(PlaybackRepeat)

    def __init__(self, source):
        super().__init__()
        self.source = source
        self.activations = []
        self.insertions = []

    @Property(int)
    def count(self):
        return self.source.rowCount()

    @Slot(int, bool)
    def goTo(self, index, start_playing):
        self.activations.append((index, start_playing))

    @Slot(int, "QVariantList", bool)
    def insert(self, index, items, start_playing):
        self.insertions.append((index, list(items), start_playing))


class DragObject(QObject):
    pass


class Duration(QObject):
    def __init__(self, seconds, parent, valid=True):
        super().__init__(parent)
        self.seconds = seconds
        self.known = valid

    @Slot(result=bool)
    def valid(self):
        return self.known

    @Slot(result=int)
    def toSeconds(self):
        return self.seconds

    @Slot(result=int)
    def toMinutes(self):
        return self.seconds // 60

    @Slot(result=int)
    def toHours(self):
        return self.seconds // 3600

    @Slot("QVariantList", result=QObject)
    def sum(self, values):
        return Duration(sum(value.toSeconds() for value in values), self.parent())


app = QCoreApplication([])
qmlRegisterUncreatableType(QAbstractProxyModel, "QueueTests", 1, 0,
                          "AbstractProxyModel", "Provided by the test")
qmlRegisterType(DragObject, "QueueTests.Widgets", 1, 0, "DragItem")
qmlRegisterUncreatableType(PlaylistSource, "QueueTests", 1, 0, "PlaylistListModel", "Provided by the test")
qmlRegisterUncreatableType(PlaybackController, "QueueTests", 1, 0, "PlaylistController", "Provided by the test")
engine = QQmlEngine()
source = PlaylistSource()
source.reset()
proxy = QSortFilterProxyModel()
proxy.setSourceModel(source)
proxy.setFilterFixedString("Visible")
controller = PlaybackController(source)
engine.rootContext().setContextProperty("testSource", source)
engine.rootContext().setContextProperty("testProxy", proxy)
engine.rootContext().setContextProperty("MainPlaylistController", controller)

qt_directory = Path(__file__).resolve().parents[1]
pane_path = qt_directory / "playlist/qml/PlaylistPane.qml"
pane = pane_path.read_text(encoding="utf-8")
queue = qml_block(pane, r"Widgets\.ListViewExt\s*\{\s*id:\s*listView\b")[0]
delegate_path = qt_directory / "playlist/qml/PlaylistDelegate.qml"
delegate = delegate_path.read_text(encoding="utf-8")
helpers = (qt_directory / "util/qml/Helpers.qml").read_text(encoding="utf-8")

# Parse both complete production components as well as the extracted harness.
formatter = Path(PySide6.__file__).parent / ("qmlformat.exe" if os.name == "nt" else "qmlformat")
for path in [pane_path, delegate_path]:
    result = subprocess.run([str(formatter), "--ignore-settings", str(path)],
                            stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True)
    assert result.returncode == 0, (path.name, result.stderr)


def create_component(text):
    component = QQmlComponent(engine)
    component.setData(text.encode("utf-8"), QUrl())
    assert not component.isError(), [error.toString() for error in component.errors()]
    instance = component.create()
    assert instance, [error.toString() for error in component.errors()]
    return component, instance


helper_functions = "\n".join(qml_function(helpers, name) for name in
                             ["isSortedIntegerArrayConsecutive", "itemsMovable",
                              "isValidInstanceOf", "isArray"])
helper_component, helper_object = create_component("import QtQml\nQtObject {\n" + helper_functions + "\n}")
engine.rootContext().setContextProperty("Helpers", helper_object)

mapping_functions = "\n".join(qml_function(queue, name) for name in
                              ["mapViewToSource", "mapSourceToView", "mappedSelectedIndexes",
                               "isSourceDropAcceptable", "acceptSourceDrop"])
footer_validation = qml_block(queue, r"\bisDropAcceptableFunc:\s*function\(")[1]
footer_drop = qml_block(queue, r"\bacceptDropFunc:\s*function\(")[1]
activation = qml_block(queue, r"\bonActionAtIndex:\s*\(index\)\s*=>")[1]
deletion = qml_block(queue, r"\bKeys.onDeletePressed:\s*")[1]
touch_handler = delegate[delegate.index("DelegateTouchTapHandler {"):]
touch_activation = qml_block(touch_handler, r"\bonSingleTapped:\s*\(eventPoint, button\)\s*=>")[1]
accessible_activation = qml_block(delegate, r"\bAccessible.onPressAction:\s*")[1]
component, harness = create_component("""
import QtQml
import QueueTests
import QueueTests.Widgets as Widgets
QtObject {
    id: listView
    property var model: testProxy
    property int count: model.rowCount()
    property var root: QtObject { property var model: testSource }
    property var dragItem: Widgets.DragItem { }
    property var selectionModel: QtObject { property var selectedIndexesFlat: [] }
    function forceActiveFocus() { }
""" + mapping_functions + "\n" +
    "property var rowDelegate: QtObject { id: delegate; property var view: listView; property int index: 0;\n" +
    qml_function(delegate, "mappedIndex") + "\n}\n" +
    "function footerAccepts(drop, index) {" + footer_validation + "}\n" +
    "function footerDrop(index, drop) {" + footer_drop + "}\n" +
    "function activate(index) {" + activation + "}\n" +
    "function touchActivate(eventPoint, button) {" + touch_activation + "}\n" +
    "function accessibleActivate() {" + accessible_activation + "}\n" +
    "function deleteSelection() {" + deletion + "}\n" + "}")


def evaluate(code):
    expression = QQmlExpression(engine.rootContext(), harness, code)
    result, _ = expression.evaluate()
    assert not expression.hasError(), expression.error().toString()
    app.processEvents()
    # A real ListView updates count on model signals; QtObject does not.
    harness.setProperty("count", harness.property("model").rowCount())
    return result.toVariant() if hasattr(result, "toVariant") else result


def reset(filter_text="Visible", filtered=True):
    source.reset()
    proxy.setFilterFixedString(filter_text)
    harness.setProperty("model", proxy if filtered else source)
    harness.setProperty("count", (proxy if filtered else source).rowCount())
    evaluate("selectionModel.selectedIndexesFlat = []")
    controller.activations.clear()
    controller.insertions.clear()


assert evaluate("[mapViewToSource(0), mapViewToSource(1)]") == [1, 3]
assert evaluate("[mapSourceToView(1), mapSourceToView(3)]") == [0, 1]
assert evaluate("[mapSourceToView(0), mapSourceToView(4), mapViewToSource(-1), mapViewToSource(2)]") == [-1] * 4
evaluate("activate(1); activate(-1); activate(2)")
assert controller.activations == [(3, True)], controller.activations
evaluate("rowDelegate.index = 1; touchActivate(null, null)")
assert controller.activations == [(3, True), (3, True)], controller.activations
evaluate("accessibleActivate(); rowDelegate.index = -1; accessibleActivate()")
assert controller.activations == [(3, True)] * 3, controller.activations

# Reversed click order must still satisfy the source playlist's ordered API.
evaluate("selectionModel.selectedIndexesFlat = [1, 0]")
assert evaluate("mappedSelectedIndexes()") == [1, 3]
evaluate("deleteSelection()")
assert source.removals == [[1, 3]], source.removals
assert source.titles() == ["Hidden before", "Hidden middle", "Hidden tail"]

# Row drop targets arrive already mapped by VerticalDropAreaLayout. Verify
# they are used once, rather than treating a source row as another proxy row.
reset()
evaluate("selectionModel.selectedIndexesFlat = [1]")
assert evaluate("isSourceDropAcceptable({source: dragItem}, 1)")
evaluate("acceptSourceDrop(1, {source: dragItem})")
assert source.moves == [([3], 1)], source.moves
assert source.titles() == ["Hidden before", "Visible B", "Visible A", "Hidden middle", "Hidden tail"]

# A no-op move is rejected; footer validation and execution agree on the
# complete queue's end, including its hidden tail.
reset()
evaluate("selectionModel.selectedIndexesFlat = [1]")
assert not evaluate("isSourceDropAcceptable({source: dragItem}, 4)")
assert evaluate("footerAccepts({source: dragItem}, count)")
evaluate("footerDrop(count, {source: dragItem})")
assert source.moves == [([3], 5)], source.moves
assert source.titles() == ["Hidden before", "Visible A", "Hidden middle", "Hidden tail", "Visible B"]

# External drops append when all rows are filtered out, and leave playback
# stopped. The native drop must be accepted with IgnoreAction to avoid a
# second insertion by the backend.
reset("No matches")
assert evaluate("count") == 0
assert evaluate("footerAccepts({hasUrls: true}, 0)")
assert evaluate("""(function() {
    let accepted = -1;
    footerDrop(0, {hasUrls: true, urls: ['file:///one.mp4', 'file:///two.mp4'],
                  accept: function(action) { accepted = action; }});
    return accepted === Qt.IgnoreAction;
})()""")
assert controller.insertions == [(5, ["file:///one.mp4", "file:///two.mp4"], False)]
assert not evaluate("isSourceDropAcceptable({hasUrls: true}, -1)")
assert not evaluate("isSourceDropAcceptable({hasUrls: true}, 6)")

reset(filtered=False)
assert evaluate("[mapViewToSource(0), mapSourceToView(4)]") == [0, 4]
evaluate("activate(4)")
assert controller.activations == [(4, True)]
source.clear()
app.processEvents()
harness.setProperty("count", 0)
evaluate("footerDrop(0, {hasUrls: true, urls: ['file:///first.mp4'], accept: function() {}})")
assert controller.insertions == [(0, ["file:///first.mp4"], False)]


class Selection(QObject):
    changed = Signal()

    def __init__(self, model):
        super().__init__()
        self.qt_selection = QItemSelectionModel(model, self)
        self.qt_selection.selectionChanged.connect(lambda *_: self.changed.emit())

    @Property("QVariantList", notify=changed)
    def selectedIndexesFlat(self):
        return [index.row() for index in self.qt_selection.selectedIndexes()]

    @Slot(int, int)
    def select(self, row, flags):
        self.qt_selection.select(self.qt_selection.model().index(row, 0), QItemSelectionModel.SelectionFlags(flags))


# Real source/proxy dataChanged notifications must update the displayed
# aggregate even when the count stays fixed; engine-current roles do not.
reset()
for index, seconds in enumerate([0, 600, 0, 120, 0]):
    source.item(index).setData(Duration(seconds, source), PlaylistSource.Roles.DurationRole)
selection = Selection(source)
engine.rootContext().setContextProperty("testSelection", selection)
formatting_functions = "\n".join(qml_function(pane, name) for name in
                                 ["queueDuration", "formatQueueDuration", "formatChapterTime", "queueSummary", "formatQueueRepeatMode"])
selection_count_body = qml_block(pane, r"readonly property int queueSelectionCount:\s*")[1]
metadata_component, metadata = create_component("""
import QtQml
import QueueTests
QtObject {
    id: root
    property var model: testSource
    property var selectionModel: testSelection
    property int durationRevision: 0
    property int queueSelectionRevision: 0
    property var listView: QtObject { property var model: testProxy; property int count: 2; property int currentIndex: 0 }
    property string summary: queueSummary()
""" + "readonly property int queueSelectionCount: {" + selection_count_body + "}\n" +
    formatting_functions + "\nproperty var watcher: Connections { target: root.listView.model;\n" +
    qml_function(queue, "onDataChanged") + "\n" + qml_function(queue, "onModelReset") + "\n}\n}")
assert metadata.property("summary") == "2 items - 12m 0s", metadata.property("summary")
source.item(1).setData(Duration(7380, source), PlaylistSource.Roles.DurationRole)
app.processEvents()
assert proxy.rowCount() == 2
assert metadata.property("summary") == "2 items - 2h 5m", metadata.property("summary")
revision = metadata.property("durationRevision")
source.item(1).setData(True, PlaylistSource.Roles.IsCurrentRole)
app.processEvents()
assert metadata.property("durationRevision") == revision

# The same aggregation path handles an unfiltered queue, avoiding VLC's
# duration cache, which can lag behind preparsing metadata updates.
metadata.property("listView").setProperty("model", source)
metadata.property("listView").setProperty("count", source.rowCount())
source.item(3).setData(Duration(240, source), PlaylistSource.Roles.DurationRole)
app.processEvents()
assert metadata.property("summary") == "5 items - 2h 7m", metadata.property("summary")


def evaluate_on(object, code):
    expression = QQmlExpression(engine.rootContext(), object, code)
    value, _ = expression.evaluate()
    assert not expression.hasError(), expression.error().toString()
    app.processEvents()
    return value.toVariant() if hasattr(value, "toVariant") else value


for seconds, queue_text, chapter_text in [
    (0, "0s", "00:00:00"), (5, "5s", "00:00:05"),
    (135, "2m 15s", "00:02:15"), (3661, "1h 1m", "01:01:01"),
    (360000, "100h 0m", "100:00:00"),
]:
    engine.rootContext().setContextProperty("testDuration", Duration(seconds, source))
    assert evaluate_on(metadata, "formatQueueDuration(testDuration)") == queue_text
    assert evaluate_on(metadata, "formatChapterTime(testDuration)") == chapter_text
engine.rootContext().setContextProperty("testDuration", Duration(0, source, valid=False))
assert evaluate_on(metadata, "formatQueueDuration(testDuration)") == ""
assert evaluate_on(metadata, "formatChapterTime(testDuration)") == "—"
for mode, expected in [("NONE", "Repeat off"), ("ALL", "Repeat all"), ("CURRENT", "Repeat current item")]:
    value = getattr(PlaybackController.PlaybackRepeat, "PLAYBACK_REPEAT_" + mode).value
    assert evaluate_on(metadata, f"formatQueueRepeatMode({value})") == expected

selection.qt_selection.select(source.index(1, 0), QItemSelectionModel.ClearAndSelect)
assert metadata.property("queueSelectionCount") == 1
queue_selection_spy = QSignalSpy(selection.changed)
source.reset_same_rows()
app.processEvents()
assert queue_selection_spy.count() == 0, "Reset-only fixture emitted selection change"
assert source.rowCount() > 0, "Reset-only fixture must retain rows"
assert metadata.property("queueSelectionCount") == 0, "Queue selection count stayed stale after reset without selection change"
source.clear()
app.processEvents()
assert metadata.property("queueSelectionCount") == 0, "Queue selection count stayed stale after model reset"


class BookmarkActions(PlaylistSource):
    readinessChanged = Signal()

    def __init__(self):
        super().__init__()
        self.ready = False
        self.added = 0
        self.selected = []
        self.removed = []
        self.appendRow(QStandardItem("One"))
        self.appendRow(QStandardItem("Two"))

    @Property(bool, notify=readinessChanged)
    def canAdd(self):
        return self.ready

    @Slot()
    def add(self):
        self.added += 1

    @Slot("QModelIndexList")
    def remove(self, indexes):
        self.removed.append([index.row() for index in indexes if index.isValid() and index.model() is self])

    @Slot(QModelIndex)
    def select(self, index):
        if index.isValid() and index.model() is self:
            self.selected.append(index.row())


class PlayerState(QObject):
    changed = Signal()
    def __init__(self):
        super().__init__()
        self.started = False
        self.can_seek = False

    @Property(bool, notify=changed)
    def isStarted(self):
        return self.started

    @Property(bool, notify=changed)
    def seekable(self):
        return self.can_seek


bookmarks = BookmarkActions()
bookmark_selection = Selection(bookmarks)
player_state = PlayerState()
engine.rootContext().setContextProperty("testBookmarks", bookmarks)
engine.rootContext().setContextProperty("testBookmarkSelection", bookmark_selection)
engine.rootContext().setContextProperty("Player", player_state)
bookmark_source = (qt_directory / "playlist/qml/JuxtopposedBookmarkList.qml").read_text(encoding="utf-8")
can_add_expression = re.search(r"readonly property bool canAdd:\s*([^\n]+)", bookmark_source).group(1)
bookmark_selection_body = qml_block(bookmark_source, r"readonly property int selectionCount:\s*")[1]
bookmark_functions = "\n".join(qml_function(bookmark_source, name) for name in
                               ["formatTimeMs", "addBookmark", "removeSelected", "activate", "selectAndActivate"])
bookmark_component, bookmark_harness = create_component("""
import QtQml
import QtQml.Models
QtObject {
    id: root
    property var bookmarkModel: testBookmarks
    property var selectionModel: testBookmarkSelection
    property int count: 2
    property int currentIndex: 0
    property int selectionRevision: 0
    function forceActiveFocus() { }
""" + "readonly property bool canAdd: " + can_add_expression + "\n" +
    "readonly property int selectionCount: {" + bookmark_selection_body + "}\n" + bookmark_functions +
    "\nproperty var watcher: Connections { target: root.bookmarkModel;\n" +
    qml_function(bookmark_source, "onModelReset") + "\n}\n}")
for milliseconds, expected in [(0, "00:00:00:000"), (42023, "00:00:42:023"),
                               (44125, "00:00:44:125"), (360000001, "100:00:00:001")]:
    assert evaluate_on(bookmark_harness, f"formatTimeMs({milliseconds})") == expected
assert evaluate_on(bookmark_harness, "formatTimeMs(-1)") == "—"
evaluate_on(bookmark_harness, "addBookmark()")
assert bookmarks.added == 0
bookmarks.ready = True
bookmarks.readinessChanged.emit()
evaluate_on(bookmark_harness, "addBookmark()")
assert bookmarks.added == 0, "Readiness alone must not enable adding before playback"
player_state.started = True
player_state.changed.emit()
evaluate_on(bookmark_harness, "addBookmark()")
assert bookmarks.added == 0, "An unseekable input must not expose Add"
player_state.can_seek = True
player_state.changed.emit()
evaluate_on(bookmark_harness, "addBookmark(); selectAndActivate(1); removeSelected()")
assert bookmarks.added == 1
assert bookmarks.selected == [1]
assert bookmarks.removed == [[1]]
assert bookmark_harness.property("selectionCount") == 1
bookmark_selection_spy = QSignalSpy(bookmark_selection.changed)
bookmarks.reset_same_rows()
app.processEvents()
assert bookmark_selection_spy.count() == 0, "Reset-only bookmark fixture emitted selection change"
assert bookmarks.rowCount() == 2
assert bookmark_harness.property("selectionCount") == 0, "Bookmark selection count stayed stale after reset without selection change"
bookmarks.clear()
app.processEvents()
assert bookmark_harness.property("selectionCount") == 0
assert bookmark_harness.property("currentIndex") == -1

print("Passed: real proxy mapping, keyboard/touch/accessibility activation, queue drops, duration notifications/formatting, reset selection counts and bookmark readiness/selection actions.")
