# Visual design and original artwork by Juxtopposed.
# Test code licensed under GPLv2 or later.
"""Render the production queue row and verify its measured Figma geometry."""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import xml.etree.ElementTree as ET

os.environ["QT_QPA_PLATFORM"] = "offscreen"
os.environ["QT_QUICK_BACKEND"] = "software"
os.environ["QML_DISABLE_DISK_CACHE"] = "1"
os.environ["QT_QPA_FONTDIR"] = str(Path(__file__).resolve().parents[1] / "pixmaps/juxtopposed")

import PySide6
from PySide6.QtCore import QObject, QRectF, QUrl, Qt, qInstallMessageHandler
from PySide6.QtGui import QColor, QFont, QFontDatabase, QGuiApplication
from PySide6.QtQml import QQmlExpression, qmlContext
from PySide6.QtQuick import QQuickView, QQuickWindow
from PySide6.QtTest import QTest, QSignalSpy

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--output-dir", type=Path, default=Path("ui-previews"))
args = parser.parse_args()
args.output_dir.mkdir(parents=True, exist_ok=True)

qt_directory = Path(__file__).resolve().parents[1]
assets = qt_directory / "pixmaps/juxtopposed"
component = qt_directory / "playlist/qml/JuxtopposedQueueRow.qml"
panel_component = qt_directory / "playlist/qml/JuxtopposedQueuePanel.qml"
move_asset = assets / "queue-move.svg"
assert move_asset.is_file() and move_asset.stat().st_size > 0, move_asset

# Only inspect the export's dimensions, never its vector paths.
svg = ET.parse(move_asset).getroot()
move_width = float(svg.attrib["width"])
move_height = float(svg.attrib["height"])

formatter = Path(PySide6.__file__).parent / ("qmlformat.exe" if os.name == "nt" else "qmlformat")
for source in [component, panel_component, component.parent / "JuxtopposedBookmarkList.qml",
               component.parent / "PlaylistPane.qml", component.parent / "PlaylistDelegate.qml"]:
    result = subprocess.run([str(formatter), "--ignore-settings", str(source)],
                            stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True)
    assert result.returncode == 0, (source.name, result.stderr)

messages = []
qInstallMessageHandler(lambda kind, context, message: messages.append(message))
app = QGuiApplication([])
QQuickWindow.setDefaultAlphaBuffer(True)
font_id = QFontDatabase.addApplicationFont(str(assets / "InterVariable.ttf"))
assert font_id >= 0
app.setFont(QFont(QFontDatabase.applicationFontFamilies(font_id)[0]))
view = QQuickView()
view.setColor(QColor("#0E0E0E"))
view.setResizeMode(QQuickView.SizeRootObjectToView)
view.setInitialProperties({
    "title": "The Wand_S01E11.mp4",
    "durationText": "23:06",
    "assetBase": QUrl.fromLocalFile(str(assets) + "/"),
})
view.setSource(QUrl.fromLocalFile(str(component)))
assert not view.errors(), [error.toString() for error in view.errors()]
root = view.rootObject()
assert root
view.resize(360, 32)
view.show()
QTest.qWait(120)


def item(name):
    child = root.findChild(QObject, name)
    # Delegates created by Repeater/ListView have visual parents without
    # necessarily sharing the root's QObject ownership tree.
    pending = list(root.childItems())
    while not child and pending:
        candidate = pending.pop()
        if candidate.objectName() == name:
            child = candidate
        else:
            pending.extend(candidate.childItems())
    assert child, name
    return child


def expression(object, code):
    evaluation = QQmlExpression(qmlContext(object), object, code)
    value, _ = evaluation.evaluate()
    assert not evaluation.hasError(), evaluation.error().toString()
    return value


def bounds(object):
    # Text.boundingRect() measures painted glyphs rather than its layout slot.
    rectangle = object.mapRectToScene(QRectF(0, 0, object.width(), object.height()))
    return [rectangle.x(), rectangle.y(), rectangle.width(), rectangle.height()]


title = item("queueRowTitle")
duration = item("queueRowDuration")
move = item("queueRowMove")
assert expression(move, "status === Image.Ready"), move.property("source")
assert move.width() == move_width and move.height() == move_height
assert title.property("color") == QColor("#969696")
assert duration.property("color") == QColor("#6B6B6B")
assert expression(title, "font.pixelSize === 14 && font.weight === Font.Normal")

# Figma row 463:848 has text at x42/y7.5 with a 265x17 title slot,
# and its duration ends at x356 after the four-pixel right padding.
expected_title = [42, 7.5, 265, 17]
actual_title = bounds(title)
assert all(abs(actual - expected) <= 0.6 for actual, expected in zip(actual_title, expected_title)), actual_title
duration_bounds = bounds(duration)
assert abs(duration_bounds[0] + duration_bounds[2] - 356) <= 0.6, duration_bounds
assert not root.property("titleElided")
assert view.grabWindow().save(str(args.output_dir / "queue-row-preview.png"))

geometry = []
for width, scale, title_value, duration_value in [
    (200, 1, "A very long media title that cannot fit into this queue row.mp4", "123:45:56"),
    (540, 1.5, "The Wand_S01E11.mp4", "23:06"),
    (720, 2, "The Wand_S01E11.mp4", "23:06"),
    (360, 1, "Live stream", "—"),
]:
    root.setProperty("uiScale", scale)
    root.setProperty("title", title_value)
    root.setProperty("durationText", duration_value)
    view.resize(width, round(32 * scale))
    QTest.qWait(40)
    title_bounds = bounds(title)
    duration_bounds = bounds(duration)
    move_bounds = bounds(move)
    assert abs(title_bounds[0] - 42 * scale) <= 0.6
    assert abs(duration_bounds[0] + duration_bounds[2] - (width - 4 * scale)) <= 0.6
    assert title_bounds[0] + title_bounds[2] <= duration_bounds[0] - 10 * scale + 0.6
    assert abs(move_bounds[2] - move_width * scale) <= 0.6
    assert abs(move_bounds[3] - move_height * scale) <= 0.6
    for object_bounds in [title_bounds, duration_bounds, move_bounds]:
        assert object_bounds[0] >= -0.6 and object_bounds[1] >= -0.6, object_bounds
        assert object_bounds[0] + object_bounds[2] <= width + 0.6, object_bounds
        assert object_bounds[1] + object_bounds[3] <= view.height() + 0.6, object_bounds
    if width == 200:
        assert root.property("titleElided")
        assert view.grabWindow().save(str(args.output_dir / "queue-row-narrow.png"))
    geometry.append({"width": width, "scale": scale, "title": title_bounds,
                     "duration": duration_bounds, "move": move_bounds})

errors = [message for message in messages if any(term in message.lower()
          for term in ["error", "unable", "cannot", "binding loop"])]
assert not errors, errors

# Render the same production panel with a replaceable view slot. The literal
# summary and four filenames come from Figma node 463:831; they are reference
# fixtures, not a substituted implementation of the live playlist model.
fixture = args.output_dir / "queue-panel-fixture.qml"
fixture.write_text("""
import QtQuick
import QtQuick.Controls
import """ + '"' + QUrl.fromLocalFile(str(component.parent)).toString() + '"' + """ as Queue
Queue.JuxtopposedQueuePanel {
    id: panel
    summary: "4 items - 2h 3m"
    onTabRequested: (index) => activeTab = index
    bodyComponent: Component {
        ListView {
            spacing: 12 * panel.uiScale
            model: ListModel {
                ListElement { name: "The Wand_S01E11.mp4"; time: "23:06" }
                ListElement { name: "The Wand_S01E12.mp4"; time: "24:18" }
                ListElement { name: "The Wand_S01E13.mp4"; time: "25:00" }
                ListElement { name: "The Wand_S01E14.mp4"; time: "22:35" }
            }
            delegate: Queue.JuxtopposedQueueRow {
                required property int index
                required property string name
                required property string time
                objectName: "queuePreviewRow" + index
                width: ListView.view.width
                uiScale: panel.uiScale
                assetBase: panel.assetBase
                showMove: panel.activeTab === 0
                chapterNumber: panel.activeTab === 2 ? index + 1 : 0
                title: name
                durationText: time
            }
        }
    }
}
""", encoding="utf-8")
row_view = view
row_view.hide()
view = QQuickView()
view.setColor(QColor("transparent"))
view.setFlags(Qt.FramelessWindowHint)
surface_format = view.format()
surface_format.setAlphaBufferSize(8)
view.setFormat(surface_format)
view.setResizeMode(QQuickView.SizeRootObjectToView)
view.setInitialProperties({"assetBase": QUrl.fromLocalFile(str(assets) + "/")})
view.setSource(QUrl.fromLocalFile(str(fixture)))
assert not view.errors(), [error.toString() for error in view.errors()]
root = view.rootObject()
assert root
view.resize(400, 946)
view.show()
QTest.qWait(120)

assert abs(expression(root, "background.color.a") - 0.95) <= 0.005
assert expression(root, "background.radius") == 10
assert bounds(item("queueBody")) == [20, 120, 360, 826]
assert bounds(item("queueTabs")) == [0, 0, 400, 50]
assert bounds(item("queueToolbar")) == [0, 51, 400, 68]
assert item("queueSummary").property("text") == "4 items - 2h 3m"
assert item("queueSummary").property("color") == QColor("#6B6B6B")

expected_actions = {
    "queueSort": [200, 73, 24, 24],
    "queueRepeat": [236, 71, 30, 28],
    "queueShuffle": [278, 71, 30, 28],
    "queueRemove": [320, 73, 24, 24],
    "queueAdd": [356, 73, 24, 24],
}
for name, expected in expected_actions.items():
    actual = bounds(item(name))
    assert all(abs(a - b) <= 0.6 for a, b in zip(actual, expected)), (name, actual)
    icon = item(name + "Icon")
    assert expression(icon, "status === Image.Ready"), icon.property("source")
    svg = ET.parse(assets / icon.property("source").fileName()).getroot()
    assert icon.width() == float(svg.attrib["width"])
    assert icon.height() == float(svg.attrib["height"])
    assert bounds(icon) == expected, (name, bounds(icon), expected)

for index in range(3):
    tab = item("queueTab" + str(index))
    assert abs(tab.width() - 128.666667) <= 0.7
    icon = item("queueTabIcon" + str(index))
    assert expression(icon, "status === Image.Ready"), icon.property("source")
    assert icon.width() == 20 and icon.height() == 20
    assert item("queueTabUnderline" + str(index)).isVisible() == (index == 0)

for index in range(4):
    row = item("queuePreviewRow" + str(index))
    assert bounds(row) == [20, 120 + index * 44, 360, 32], (index, bounds(row))
panel_preview = view.grabWindow()
capture_has_alpha = panel_preview.hasAlphaChannel()
if capture_has_alpha:
    assert abs(panel_preview.pixelColor(200, 900).alpha() - 242) <= 1
    assert panel_preview.pixelColor(0, 0).alpha() == 0
else:
    # The Windows software/offscreen backend captures RGB32 even when the
    # window requests an alpha surface. It composites the real 95% fill on
    # black; verify that result and the cleared corner without altering PNGs.
    assert panel_preview.pixelColor(200, 900) == QColor("#0D0D0D")
    assert panel_preview.pixelColor(0, 0) == QColor("#000000")
assert panel_preview.save(str(args.output_dir / "queue-panel-preview.png"))

# Engine-active repeat/shuffle states must provide visible feedback and state
# descriptions while preserving the measured unchecked reference appearance.
for name, active_property, description_property, expected_on, expected_off in [
    ("queueRepeat", "repeatActive", "repeatDescription", "Repeat all", "Repeat off"),
    ("queueShuffle", "shuffleActive", "shuffleDescription", "Shuffle on", "Shuffle off"),
]:
    button = item(name)
    assert button.property("description") == expected_off
    assert expression(button, "background.color.a") == 0
    assert expression(button, "background.border.width") == 0
    root.setProperty(active_property, True)
    assert button.property("checked")
    assert button.property("description") == expected_on
    assert expression(button, "background.color.a") > 0
    assert expression(button, "background.border.width") == 1
    assert expression(button, "Accessible.checked")
    if name == "queueRepeat":
        root.setProperty(description_property, "Repeat current item")
        assert button.property("description") == "Repeat current item"
        assert expression(button, "Accessible.name") == "Repeat current item"
    root.setProperty(active_property, False)
    assert not button.property("checked")
    assert expression(button, "background.color.a") == 0

for name, signal in [
    ("queueSort", "sortRequested"), ("queueRepeat", "repeatRequested"),
    ("queueShuffle", "shuffleRequested"), ("queueRemove", "removeRequested"),
    ("queueAdd", "addRequested"),
]:
    spy = QSignalSpy(getattr(root, signal))
    QTest.mouseClick(view, Qt.LeftButton, Qt.NoModifier,
                     item(name).mapToScene(item(name).boundingRect().center()).toPoint())
    QTest.qWait(10)
    assert spy.count() == 1, (name, spy.count())

tab_spy = QSignalSpy(root.tabRequested)
QTest.mouseClick(view, Qt.LeftButton, Qt.NoModifier,
                 item("queueTab1").mapToScene(item("queueTab1").boundingRect().center()).toPoint())
assert tab_spy.count() == 1 and tab_spy.at(0)[0] == 1
assert root.property("activeTab") == 1
add = item("queueAdd")
add.forceActiveFocus()
add_spy = QSignalSpy(root.addRequested)
QTest.keyClick(view, Qt.Key_Space)
assert add_spy.count() == 1, "Keyboard add files"
root.setProperty("removeEnabled", False)
remove_spy = QSignalSpy(root.removeRequested)
QTest.mouseClick(view, Qt.LeftButton, Qt.NoModifier,
                 item("queueRemove").mapToScene(item("queueRemove").boundingRect().center()).toPoint())
assert remove_spy.count() == 0, "Disabled remove action"

root.setProperty("activeTab", 0)
root.setProperty("removeEnabled", True)
for width, height, scale in [(340, 700, 1), (600, 1419, 1.5), (800, 1892, 2)]:
    root.setProperty("uiScale", scale)
    view.resize(width, height)
    QTest.qWait(40)
    for name in expected_actions:
        action_bounds = bounds(item(name))
        assert action_bounds[0] >= 0 and action_bounds[0] + action_bounds[2] <= width, (name, action_bounds)
    body_bounds = bounds(item("queueBody"))
    assert body_bounds == [20 * scale, 120 * scale, width - 40 * scale, height - 120 * scale]


def render_variant(name, tab, summary, records, toolbar_content_height, show_toolbar):
    text = fixture.read_text(encoding="utf-8")
    replacement = ("summary: " + json.dumps(summary) + "\n" +
                   "activeTab: " + str(tab) + "\n" +
                   "showSort: false\nshowRepeat: false\nshowShuffle: false\n" +
                   "showToolbar: " + str(show_toolbar).lower() + "\n" +
                   "toolbarContentHeight: " + str(toolbar_content_height) + "\n" +
                   "addDescription: " + json.dumps("Add bookmark" if tab == 1 else "Add files"))
    text = text.replace('summary: "4 items - 2h 3m"', replacement)
    elements = "\n".join("ListElement { name: " + json.dumps(title, ensure_ascii=True) +
                          "; time: " + json.dumps(time) + " }" for title, time in records)
    text = re.sub(r"model: ListModel \{.*?\n\s*\}", lambda _: "model: ListModel {\n" + elements + "\n}",
                  text, count=1, flags=re.DOTALL)
    variant = args.output_dir / (name + "-fixture.qml")
    variant.write_text(text, encoding="utf-8")
    view.setInitialProperties({"assetBase": QUrl.fromLocalFile(str(assets) + "/")})
    view.setSource(QUrl.fromLocalFile(str(variant)))
    assert not view.errors(), [error.toString() for error in view.errors()]
    variant_root = view.rootObject()
    view.resize(400, 946)
    QTest.qWait(60)
    return variant_root


root = render_variant("bookmarks-panel", 1, "2 Bookmarks",
                      [("DON\u2019T BLINK 1", "00:00:42:023"), ("DON\u2019T BLINK 2", "00:00:44:125")],
                      24, True)
assert bounds(item("queueToolbar")) == [0, 51, 400, 64]
assert bounds(item("queueBody")) == [20, 116, 360, 830]
assert bounds(item("queueRemove")) == [320, 71, 24, 24]
assert bounds(item("queueAdd")) == [356, 71, 24, 24]
assert expression(item("queueAdd"), "Accessible.name") == "Add bookmark"
for index in range(2):
    row = item("queuePreviewRow" + str(index))
    assert bounds(row) == [20, 116 + index * 44, 360, 32]
    assert row.property("title") == "DON\u2019T BLINK " + str(index + 1), row.property("title")
    title = row.findChild(QObject, "queueRowTitle")
    duration = row.findChild(QObject, "queueRowDuration")
    assert bounds(title)[0] == 20
    assert abs(bounds(duration)[0] + duration.width() - 376) <= 0.6
for index in range(3):
    assert expression(item("queueTabIcon" + str(index)), "status === Image.Ready")
    assert item("queueTabUnderline" + str(index)).isVisible() == (index == 1)
assert view.grabWindow().save(str(args.output_dir / "bookmarks-panel-preview.png"))

root = render_variant("chapters-panel", 2, "",
                      [("Winter", "00:02:15"), ("Spring", "00:29:45"), ("Summer", "00:51:30"),
                       ("Fall", "01:13:50"), ("Winter (5 Years Later)", "01:44:10")], 0, False)
assert not item("queueToolbar").isVisible()
assert bounds(item("queueBody")) == [20, 71, 360, 875]
for index in range(5):
    row = item("queuePreviewRow" + str(index))
    assert bounds(row) == [20, 71 + index * 44, 360, 32]
    number = row.findChild(QObject, "queueRowNumber")
    title = row.findChild(QObject, "queueRowTitle")
    duration = row.findChild(QObject, "queueRowDuration")
    assert bounds(number)[0] == 24 and number.property("text") == str(index + 1)
    assert expression(number, "font.weight === Font.Medium")
    assert number.property("color") == QColor("#D37918")
    assert bounds(title)[0] == 68
    assert abs(bounds(duration)[0] + duration.width() - 376) <= 0.6
for index in range(3):
    assert expression(item("queueTabIcon" + str(index)), "status === Image.Ready")
    assert item("queueTabUnderline" + str(index)).isVisible() == (index == 2)
assert view.grabWindow().save(str(args.output_dir / "chapters-panel-preview.png"))

# Execute the production adapter's parent/anchor bindings with a real
# ListView/ColumnLayout. Changing the style must retain the view and current
# row, then release its anchors so the legacy layout can size it again.
pane_source = (component.parent / "PlaylistPane.qml").read_text(encoding="utf-8")
queue_prefix = re.search(r"Widgets\.ListViewExt\s*\{\s*id:\s*listView\b(.*?)selectionModel:",
                         pane_source, re.DOTALL).group(1)
parent_binding = re.search(r"^\s*parent:.*$", queue_prefix, re.MULTILINE).group().strip()
anchor_binding = re.search(r"^\s*anchors\.fill:.*$", queue_prefix, re.MULTILINE).group().strip()
adapter_fixture = args.output_dir / "queue-adapter-fixture.qml"
adapter_fixture.write_text("""
import QtQuick
import QtQuick.Controls as C
import QtQuick.Layouts
import """ + '"' + QUrl.fromLocalFile(str(component.parent)).toString() + '"' + """ as Queue
C.Pane {
    id: root
    width: 400
    height: 946
    property bool juxtopposed: true
    property url assetBase
    padding: 0
    contentItem: ColumnLayout {
        spacing: 0
        Item {
            visible: !root.juxtopposed
            Layout.fillWidth: true
            Layout.preferredHeight: 50
        }
        Queue.JuxtopposedQueuePanel {
            id: queuePanel
            visible: root.juxtopposed
            Layout.fillWidth: true
            Layout.fillHeight: true
            assetBase: root.assetBase
        }
        Item {
            id: legacyQueueHost
            visible: !root.juxtopposed
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
        ListView {
            id: listView
            objectName: "adapterQueue"
            property int retainedMarker: 123
""" + parent_binding + "\n" + anchor_binding + "\n" + """
            model: 4
            delegate: Rectangle { height: 32; width: ListView.view.width }
        }
    }
}
""", encoding="utf-8")
view.setInitialProperties({"assetBase": QUrl.fromLocalFile(str(assets) + "/")})
view.setSource(QUrl.fromLocalFile(str(adapter_fixture)))
assert not view.errors(), [error.toString() for error in view.errors()]
root = view.rootObject()
view.resize(400, 946)
QTest.qWait(60)
retained_view = item("adapterQueue")
retained_view.setProperty("currentIndex", 2)
assert bounds(retained_view) == [20, 120, 360, 826]
root.setProperty("juxtopposed", False)
QTest.qWait(60)
assert item("adapterQueue") == retained_view
assert retained_view.property("currentIndex") == 2
assert retained_view.property("retainedMarker") == 123
assert bounds(retained_view) == [0, 50, 400, 896], bounds(retained_view)
root.setProperty("juxtopposed", True)
QTest.qWait(60)
assert item("adapterQueue") == retained_view
assert retained_view.property("currentIndex") == 2
assert bounds(retained_view) == [20, 120, 360, 826], bounds(retained_view)

errors = [message for message in messages if any(term in message.lower()
          for term in ["error", "unable", "cannot", "binding loop"])]
assert not errors, errors
(args.output_dir / "queue-row-verification.json").write_text(
    json.dumps({"geometry": geometry, "captureHasAlpha": capture_has_alpha,
                "messages": messages}, indent=2), encoding="utf-8")
print("Passed: original assets, measured panel/row geometry, Inter typography, long media labels/durations, header actions, retained native view across opt-out and 100/150/200-percent scaling.")
