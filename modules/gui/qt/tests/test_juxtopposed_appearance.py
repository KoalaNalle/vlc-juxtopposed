# SPDX-License-Identifier: GPL-2.0-or-later
"""Exercise actual authored QML against changing supplied native providers.

These are component/provider-contract checks, not native preferences, System
theme notification, renderer blur, restart persistence, or desktop acceptance.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import tempfile
import re
import xml.etree.ElementTree as ET

os.environ['QT_QPA_PLATFORM'] = 'offscreen'
os.environ['QT_QUICK_BACKEND'] = 'software'
os.environ['QML_DISABLE_DISK_CACHE'] = '1'
from PySide6.QtCore import QObject, QUrl, Qt, qInstallMessageHandler
from PySide6.QtGui import QColor, QGuiApplication, QFontDatabase
from PySide6.QtQml import QQmlExpression, qmlContext, qmlRegisterSingletonType
from PySide6.QtQuick import QQuickView
from PySide6.QtTest import QTest, QSignalSpy

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--repo-root', type=Path, required=True)
parser.add_argument('--palette-root', type=Path)
parser.add_argument('--output-dir', type=Path, required=True)
options = parser.parse_args()
repo = options.repo_root.resolve()
palette_repo = (options.palette_root or repo).resolve()
qt = repo / 'modules/gui/qt'
os.environ['QT_QPA_FONTDIR'] = str(qt / 'pixmaps/juxtopposed')
out = options.output_dir.resolve()
out.mkdir(parents=True, exist_ok=False)
messages = []
qInstallMessageHandler(lambda kind, context, message: messages.append(message))
app = QGuiApplication([])
font = QFontDatabase.addApplicationFont(str(qt / 'pixmaps/juxtopposed/InterVariable.ttf'))
assert font >= 0
palette_source = palette_repo / 'modules/gui/qt/style/JuxtopposedPalette.qml'
qmlRegisterSingletonType(QUrl.fromLocalFile(str(palette_source)), 'VLC.Style', 1, 0, 'JuxtopposedPalette')

module = tempfile.TemporaryDirectory(prefix='vlc-appearance-components-')
folder = Path(module.name)
widgets = folder / 'VLC/Widgets'
widgets.mkdir(parents=True)
shutil.copyfile(qt / 'widgets/qml/ImageExt.qml', widgets / 'ImageExt.qml')
(widgets / 'qmldir').write_text('module VLC.Widgets\nImageExt 1.0 ImageExt.qml\n')
scene = '''import QtQuick
import VLC.Style
import "@PLAYER@" as NativePlayer
import "@LIBRARY@" as NativeLibrary
import "@MAIN@" as NativeMain
import "@WIDGETS@" as NativeWidgets
Item {
    id: root
    width: 1920; height: 650
    property int theme: 2
    property int accentIndex: 0
    property bool systemDark: true
    property alias player: player
    property alias navigation: navigation
    property alias card: card
    property alias setting: setting
    property alias categories: categories
    property alias customAccent: nativeView.accent
    QtObject { id: nativePalette; property bool isDark: root.theme === 2 || (root.theme === 0 && root.systemDark) }
    QtObject { id: settings; property var values: ({theme: root.theme, accentIndex: root.accentIndex}) }
    QtObject {
        id: nativeView
        property var bg: ({primary: "#FAFAFA"})
        property var fg: ({primary: "#212121", secondary: "#4D4D4D", negative: "#FF0000"})
        property color separator: "#E0E0E0"
        property color accent: "#017F89"
    }
    QtObject { id: nativeWindow; property var bg: ({primary: "#EDEDED"}) }
    Binding { target: JuxtopposedPalette; property: "nativePalette"; value: nativePalette }
    Binding { target: JuxtopposedPalette; property: "nativeView"; value: nativeView }
    Binding { target: JuxtopposedPalette; property: "nativeWindow"; value: nativeWindow }
    Binding { target: JuxtopposedPalette; property: "preferences"; value: settings }
    Rectangle { anchors.fill: parent; color: nativePalette.isDark ? "#0F0F0F" : "#FAFAFA" }
    NativePlayer.JuxtopposedPlaybackBar {
        id: player; width: parent.width; height: 54; assetBase: "@ASSETS@"
        elapsed: "00:02:10"; duration: "00:25:40"; position: 0.25; volume: 0.5; seekable: true
    }
    NativeMain.JuxtopposedNavigation {
        id: navigation; x: 0; y: 80; width: 204; height: 460; expanded: true; assetBase: "@ASSETS@"
        activeRoute: "Home"
        routes: [{key: "Home", text: "Home", icon: "H", uri: ["home"]},
                 {key: "Music", text: "Music", icon: "M", uri: ["music"]}]
    }
    NativeLibrary.JuxtopposedHomeCard {
        id: card; x: 250; y: 100; width: 320; height: 250
        kind: "video"; entry: ({title: "Generated test media", progress: 0.4})
    }
    NativeWidgets.JuxtopposedSettingSlider {
        id: setting; x: 630; y: 100; width: 360; height: 100; assetBase: "@ASSETS@"
        title: "Generated native setting"; value: 0.5; from: 0; to: 1
    }
    NativeWidgets.JuxtopposedCategoryList {
        id: categories; x: 630; y: 250; width: 360; height: 200; assetBase: "@ASSETS@"
        routes: [{key: "Albums", text: "Albums", icon: "A", uri: ["albums"]}]
    }
}
'''
for key, value in {'PLAYER': 'player/qml', 'LIBRARY': 'medialibrary/qml',
                   'MAIN': 'maininterface/qml', 'WIDGETS': 'widgets/qml'}.items():
    scene = scene.replace('@' + key + '@', (qt / value).as_uri())
scene = scene.replace('@ASSETS@', (qt / 'pixmaps/juxtopposed').as_uri() + '/')
(folder / 'Scene.qml').write_text(scene, encoding='utf-8')

def create(initial=None):
    view = QQuickView()
    view.engine().addImportPath(str(folder))
    view.setResizeMode(QQuickView.SizeRootObjectToView)
    view.setInitialProperties(initial or {})
    view.setSource(QUrl.fromLocalFile(str(folder / 'Scene.qml')))
    assert not view.errors(), [error.toString() for error in view.errors()]
    view.resize(1920, 650)
    view.show()
    QTest.qWait(80)
    return view, view.rootObject()

records = []
views = []
def evaluate(root, source):
    expression = QQmlExpression(qmlContext(root), root, source)
    result, _ = expression.evaluate()
    assert not expression.hasError(), expression.error().toString()
    return result.toVariant() if hasattr(result, 'toVariant') else result

def check(root, name, expression, expected):
    actual = evaluate(root, expression)
    if isinstance(actual, QColor):
        actual = actual.name(QColor.HexArgb) if actual.alpha() != 255 else actual.name()
    actual = str(actual).lower()
    expected = str(expected).lower()
    records.append(dict(name=name, expression=expression, expected=expected,
                        actual=actual, passed=actual == expected))

def named(root, name):
    queue = [root]
    while queue:
        item = queue.pop()
        if item.objectName() == name:
            return item
        queue.extend(item.childItems())
    raise AssertionError('Missing actual QML item: ' + name)

view, root = create()
views.append(view)
check(root, 'default-dark-player-background', 'player.background.color', '#1B1B1B')
check(root, 'default-dark-home-text', 'card.textColor', '#969696')
check(root, 'default-dark-navigation-accent', 'navigation.accentColor', '#D37918')
check(root, 'default-dark-setting-editor', 'setting.editor.color', '#969696')
assert view.grabWindow().save(str(out / '01-authored-dark-default.png'))

root.setProperty('accentIndex', 5)
QTest.qWait(30)
check(root, 'dark-blue-home-progress', 'card.accentColor', '#185DD3')
check(root, 'dark-blue-navigation', 'navigation.accentColor', '#185DD3')
seek_background = evaluate(root, 'player.seekItem.background')
head = seek_background.childItems()[-1]
check(head, 'dark-blue-seek-head', 'color', '#185DD3')
assert view.grabWindow().save(str(out / '02-dark-blue.png'))

root.setProperty('theme', 1)
QTest.qWait(30)
check(root, 'light-player-surface-reference', 'player.background.color', '#BFD5D5D5')
check(root, 'light-player-timeline-reference', 'player.seekItem.background.color', '#DEDEDE')
check(root, 'light-home-text-adaptation', 'card.textColor', '#2E2E2E')
check(root, 'light-setting-editor-adaptation', 'setting.editor.color', '#2E2E2E')
check(root, 'light-retains-blue-accent', 'navigation.accentColor', '#185DD3')
check(root, 'light-audio-original-asset', 'player.audioButton.source.toString().split("/").pop()', 'audio-light.svg')
for object_name in ['juxtopposedMute', 'juxtopposedAudio', 'juxtopposedSubtitles', 'juxtopposedQueue']:
    button = named(root, object_name)
    image = button.property('contentItem').childItems()[0]
    check(image, object_name + '-light-image-width', 'width', '24.0')
    check(image, object_name + '-light-image-height', 'height', '24.0')
play_spy = QSignalSpy(evaluate(root, 'player').playPauseRequested)
play = named(root, 'juxtopposedPlay')
QTest.mouseClick(view, Qt.LeftButton, Qt.NoModifier,
                 play.mapToScene(play.boundingRect().center()).toPoint())
records.append(dict(name='light-player-action-once', passed=play_spy.count() == 1,
                    expected=1, actual=play_spy.count()))
assert view.grabWindow().save(str(out / '03-light-blue.png'))

root.setProperty('accentIndex', 0)
QTest.qWait(30)
check(root, 'light-default-timeline-accent-reference', 'card.accentColor', '#E8851A')
check(named(root, 'juxtopposedPlay'), 'light-default-play-retains-authored-fill', 'fillColor', '#D37918')
check(seek_background.childItems()[0], 'light-default-played-track-reference', 'color', '#D8BA9A')
assert view.grabWindow().save(str(out / '04-light-authored-default.png'))

root.setProperty('theme', 0)
root.setProperty('systemDark', True)
QTest.qWait(30)
check(root, 'system-dark-live-player', 'player.background.color', '#1B1B1B')
check(root, 'system-dark-live-asset', 'player.audioButton.source.toString().split("/").pop()', 'audio.svg')
root.setProperty('systemDark', False)
QTest.qWait(30)
check(root, 'system-light-live-player', 'player.background.color', '#BFD5D5D5')
check(root, 'system-light-live-asset', 'player.audioButton.source.toString().split("/").pop()', 'audio-light.svg')

root.setProperty('accentIndex', -1)
QTest.qWait(30)
check(root, 'custom-existing-accent-retained', 'navigation.accentColor', '#017F89')
root.setProperty('customAccent', QColor('#5927AA'))
QTest.qWait(30)
check(root, 'custom-existing-accent-live-provider', 'card.accentColor', '#5927AA')

# Recreate the entire engine with already-restored provider inputs. This proves
# binding initialization, not the native model's on-disk persistence semantics.
view2, restored = create({'theme': 1, 'accentIndex': 6})
views.append(view2)
check(restored, 'new-engine-restored-theme-provider', 'player.background.color', '#BFD5D5D5')
check(restored, 'new-engine-restored-accent-provider', 'card.accentColor', '#8E42FF')
check(restored, 'new-engine-restored-control-provider', 'navigation.accentColor', '#8E42FF')
assert view2.grabWindow().save(str(out / '05-recreated-provider-light-purple.png'))

# Exercise the real opt-out expression; the native controller/legacy UI itself
# is outside this supplied-provider test. Keep its original branch intact.
controlbar = (qt / 'player/qml/ControlBar.qml').read_text(encoding='utf-8')
gate = re.search(r'readonly property bool redesigned:\s*(.*?)\n\s*readonly property real sliderY:',
                 controlbar, re.S)
assert gate
records.append(dict(name='legacy-opt-out-production-gate-retained', passed=(
    'MainCtx.juxtopposedControls' in gate[1]
    and 'identifier !== PlayerControlbarModel.Miniplayer' in gate[1]
    and 'sourceComponent: root.redesigned ? redesignedControls : traditionalControls' in controlbar),
    expected='native option and miniplayer select original traditional component'))
context = view.rootContext()
context.setContextProperty('PlayerControlbarModel', {'Miniplayer': 7})
for enabled, identifier, expected in [(False, -1, False), (True, -1, True), (True, 7, False)]:
    context.setContextProperty('MainCtx', {'juxtopposedControls': enabled})
    context.setContextProperty('identifier', identifier)
    check(root, 'actual-controlbar-expression-' + str(enabled) + '-' + str(identifier),
          gate[1], expected)

resources = ET.parse(qt / 'assets.qrc')
manifest_files = {x.attrib.get('alias'): x.text for x in resources.findall(".//qresource[@prefix='/juxtopposed']/file")}
for name in ['volume-light.svg', 'audio-light.svg', 'subtitles-light.svg', 'queue-light.svg', 'fullscreen-light.svg']:
    file = qt / 'pixmaps/juxtopposed' / name
    # Baseline intentionally lacks these new reference assets; report a normal
    # contract failure, not a failed fixture launch or missing dependency error.
    exists = file.is_file() and file.stat().st_size > 0
    geometry = False
    if exists:
        svg = ET.parse(file).getroot()
        geometry = svg.attrib.get('width') == svg.attrib.get('height') == '24'
    records.append(dict(name=name + '-resource-original-root-geometry',
                        passed=exists and geometry and manifest_files.get(name) == 'pixmaps/juxtopposed/' + name,
                        expected='nonempty registered original24x24SVG'))

for item in views:
    item.close()
    item.setSource(QUrl())
    item.deleteLater()
QTest.qWait(30)
assert not messages, messages[:20]
result = dict(scope=__doc__.strip(), count=len(records), passed=all(x['passed'] for x in records),
              sourceRoot=str(repo), paletteSha256=hashlib.sha256(palette_source.read_bytes()).hexdigest(),
              qtMessages=messages, cases=records)
(out / 'verification.json').write_text(json.dumps(result, indent=2) + '\n')
print(json.dumps({'passed': result['passed'], 'count': len(records),
                  'failures': [x['name'] for x in records if not x['passed']]}))
raise SystemExit(0 if result['passed'] else 1)
