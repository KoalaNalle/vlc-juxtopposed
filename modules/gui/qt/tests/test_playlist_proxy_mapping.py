# Copyright (C) 2026 VLC authors and VideoLAN
# Test code licensed under GPLv2 or later.
"""Exercise the queue's real QML callbacks with Qt's real filter proxy.

Run with PySide6-Essentials 6.8.3 or later; a VLC binary is not required.
The small harness replaces the native playback controller, not the mapping
implementation, and extracts callbacks from their production QML files.
"""
import os
from pathlib import Path
import re
import subprocess

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
os.environ["QML_DISABLE_DISK_CACHE"] = "1"

import PySide6
from PySide6.QtCore import (QAbstractProxyModel, QCoreApplication, QObject,
                           Property, QSortFilterProxyModel, Signal, Slot, Qt, QUrl)
from PySide6.QtGui import QStandardItem, QStandardItemModel
from PySide6.QtQml import (QQmlComponent, QQmlEngine, QQmlExpression,
                          qmlRegisterType, qmlRegisterUncreatableType)


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
    def __init__(self):
        super().__init__()
        self.activations = []
        self.insertions = []

    @Slot(int, bool)
    def goTo(self, index, start_playing):
        self.activations.append((index, start_playing))

    @Slot(int, "QVariantList", bool)
    def insert(self, index, items, start_playing):
        self.insertions.append((index, list(items), start_playing))


class DragObject(QObject):
    pass


app = QCoreApplication([])
qmlRegisterUncreatableType(QAbstractProxyModel, "QueueTests", 1, 0,
                          "AbstractProxyModel", "Provided by the test")
qmlRegisterType(DragObject, "QueueTests.Widgets", 1, 0, "DragItem")
engine = QQmlEngine()
source = PlaylistSource()
source.reset()
proxy = QSortFilterProxyModel()
proxy.setSourceModel(source)
proxy.setFilterFixedString("Visible")
controller = PlaybackController()
engine.rootContext().setContextProperty("testSource", source)
engine.rootContext().setContextProperty("testProxy", proxy)
engine.rootContext().setContextProperty("MainPlaylistController", controller)

qt_directory = Path(__file__).resolve().parents[1]
pane_path = qt_directory / "playlist/qml/PlaylistPane.qml"
pane = pane_path.read_text(encoding="utf-8")
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

mapping_functions = "\n".join(qml_function(pane, name) for name in
                              ["mapViewToSource", "mapSourceToView", "mappedSelectedIndexes",
                               "isSourceDropAcceptable", "acceptSourceDrop"])
footer_validation = qml_block(pane, r"\bisDropAcceptableFunc:\s*function\(")[1]
footer_drop = qml_block(pane, r"\bacceptDropFunc:\s*function\(")[1]
activation = qml_block(pane, r"\bonActionAtIndex:\s*\(index\)\s*=>")[1]
deletion = qml_block(pane, r"\bKeys.onDeletePressed:\s*")[1]
touch_handler = delegate[delegate.index("DelegateTouchTapHandler {"):]
touch_activation = qml_block(touch_handler, r"\bonSingleTapped:\s*\(eventPoint, button\)\s*=>")[1]
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

print("Passed: real proxy row mapping, keyboard/touch activation, sorted removal, row reorder, footer/end drops, hidden rows, empty filter and empty queue.")
