# SPDX-License-Identifier: GPL-2.0-or-later
"""Setup component contracts with supplied services; not native app acceptance."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import tempfile
import xml.etree.ElementTree as ET

os.environ['QT_QPA_PLATFORM'] = 'offscreen'
os.environ['QT_QUICK_BACKEND'] = 'software'
os.environ['QML_DISABLE_DISK_CACHE'] = '1'
from PySide6.QtCore import QUrl, Qt, qInstallMessageHandler
from PySide6.QtGui import QColor, QGuiApplication, QFontDatabase
from PySide6.QtQml import QQmlExpression, qmlContext, qmlRegisterSingletonType
from PySide6.QtQuick import QQuickView
from PySide6.QtTest import QTest, QSignalSpy

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--repo-root', type=Path, required=True)
parser.add_argument('--output-dir', type=Path, required=True)
options = parser.parse_args()
repo = options.repo_root.resolve()
qt = repo / 'modules/gui/qt'
os.environ['QT_QPA_FONTDIR'] = str(qt / 'pixmaps/juxtopposed')
out = options.output_dir.resolve()
out.mkdir(parents=True, exist_ok=False)
messages = []
qInstallMessageHandler(lambda kind, context, message: messages.append(message))
app = QGuiApplication([])
assert QFontDatabase.addApplicationFont(str(qt / 'pixmaps/juxtopposed/InterVariable.ttf')) >= 0
qmlRegisterSingletonType(QUrl.fromLocalFile(str(qt / 'style/JuxtopposedPalette.qml')),
                         'VLC.Style', 1, 0, 'JuxtopposedPalette')
temporary = tempfile.TemporaryDirectory(prefix='vlc-setup-components-')
folder = Path(temporary.name)
assets = folder / 'assets'
assets.mkdir()
resources = ET.parse(qt / 'assets.qrc')
for entry in resources.findall(".//qresource[@prefix='/juxtopposed']/file"):
    destination = assets / entry.attrib.get('alias', Path(entry.text).name)
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(qt / entry.text, destination)
player = folder / 'VLC/Player'
player.mkdir(parents=True)
player_files = ['JuxtopposedPlaybackBar', 'JuxtopposedButton', 'JuxtopposedSeekBackground']
for name in player_files:
    shutil.copyfile(qt / f'player/qml/{name}.qml', player / f'{name}.qml')
(player / 'qmldir').write_text('module VLC.Player\n' + ''.join(f'{name} 1.0 {name}.qml\n' for name in player_files))
util = folder / 'VLC/Util'
util.mkdir()
(util / 'qmldir').write_text('module VLC.Util\nsingleton SVGColorImage 1.0 SVGColorImage.qml\n')
# Native recoloring is verified in app captures, not this supplied dependency.
(util / 'SVGColorImage.qml').write_text('pragma Singleton\nimport QtQml\nQtObject { function colorize(path) { return { any: function(colors) { return this }, uri: function() { return path } } } }\n')
scene = '''import QtQuick
import QtQuick.Controls.Basic
import VLC.Style
import "@DIALOGS@" as Authored
Item {
    id: root
    width: 1280; height: 900
    property alias dialog: setup
    property alias service: service
    property alias systemDark: nativePalette.isDark
    property alias opener: opener
    property alias testScale: setup.uiScale
    QtObject { id: nativePalette; property bool isDark: true }
    QtObject {
        id: colors
        property var bg: ({primary: "#FAFAFA"})
        property var fg: ({primary: "#212121", secondary: "#4D4D4D", negative: "#FF0000"})
        property color separator: "#E0E0E0"
        property color accent: "#017F89"
    }
    QtObject { id: windowColors; property var bg: ({primary: "#EDEDED"}) }
    QtObject {
        id: service
        property var values: ({theme: 2, accentIndex: 0})
        property string lastError: ""
        property int saves: 0
        property int reloads: 0
        property bool fail: false
        function reload() { reloads++ }
        function finishSetup(theme, accent) {
            saves++
            if (fail) { lastError = "Owned fixture save failure; retry is available."; return false }
            values = {theme: theme, accentIndex: accent}; lastError = ""; return true
        }
    }
    Rectangle { anchors.fill: parent; color: "#383838" }
    Button { id: opener; objectName: "opener"; text: "Open setup"; onClicked: setup.showDialog(opener) }
    Authored.JuxtopposedSetupDialog {
        id: setup; parent: root
        preferences: service; previewNativePalette: nativePalette
        previewViewColors: colors; previewWindowColors: windowColors
        assetBase: "@ASSETS@"
        activeShortcuts: ({"key-play-pause": "Space", "key-jump-extrashort": "Shift+Left",
            "key-jump+extrashort": "Shift+Right", "key-prev": "p", "key-next": "n",
            "key-frame-previous": "Shift+e", "key-frame-next": "e", "key-subtitle-track": "v", "key-audio-track": "b",
            "key-toggle-fullscreen": "f", "key-vol-up": "Ctrl+Up", "key-vol-down": "Ctrl+Down",
            "key-snapshot": "Shift+s", "key-record": "Shift+r"})
        interfaceShortcuts: ({queue: "Ctrl+L"})
    }
}
'''.replace('@DIALOGS@', (qt / 'dialogs/dialogs/qml').as_uri()).replace('@ASSETS@', assets.as_uri() + '/')
(folder / 'Scene.qml').write_text(scene, encoding='utf-8')
view = QQuickView()
view.engine().addImportPath(str(folder))
view.setResizeMode(QQuickView.SizeRootObjectToView)
view.setSource(QUrl.fromLocalFile(str(folder / 'Scene.qml')))
assert not view.errors(), [error.toString() for error in view.errors()]
root = view.rootObject()
view.resize(1280, 900)
view.show()
QTest.qWait(100)
records = []

def evaluate(source, obj=None):
    obj = obj or root
    expression = QQmlExpression(qmlContext(obj), obj, source)
    value, _ = expression.evaluate()
    assert not expression.hasError(), expression.error().toString()
    return value.toVariant() if hasattr(value, 'toVariant') else value

def check(name, source, expected):
    actual = evaluate(source)
    if isinstance(actual, QColor):
        actual = actual.name()
    if isinstance(expected, str) and expected.startswith('#') and isinstance(actual, str):
        actual = actual.lower()
    records.append(dict(name=name, expected=expected, actual=actual, passed=actual == expected))

def call(source):
    evaluate(source)
    QTest.qWait(70)

def find(name):
    # Popup items belong to Overlay rather than the root's visual subtree.
    queue = [view.contentItem()]
    while queue:
        item = queue.pop()
        if item.objectName() == name and item.isVisible():
            return item
        queue.extend(item.childItems())
    raise AssertionError('Missing visible authored item: ' + name)

def click(name):
    item = find(name)
    QTest.mouseClick(view, Qt.LeftButton, Qt.NoModifier, item.mapToScene(item.boundingRect().center()).toPoint())
    QTest.qWait(70)

def capture(name):
    assert view.grabWindow().save(str(out / (name + '.png')))

call('dialog.showDialog(opener)')
check('opens-page-one', 'dialog.visible && dialog.pageIndex === 0', True)
check('native-size-reference', 'dialog.width === 1090 && dialog.height === 759', True)
check('no-default-title-header', 'dialog.header === null && dialog.footer === null', True)
check('load-current-values', 'dialog.draftTheme === 2 && dialog.draftAccent === 0 && service.reloads === 1', True)
preview = find('juxtopposedSetupPreviewControls')
play_spy = QSignalSpy(preview.playPauseRequested)
volume_spy = QSignalSpy(preview.volumeRequested)
for name in ['juxtopposedPlay', 'juxtopposedQueue', 'juxtopposedAudio', 'juxtopposedSubtitles']:
    item = find(name)
    records.append(dict(name=name + '-preview-not-tabbable', passed=not item.property('activeFocusOnTab')))
click('juxtopposedPlay')
records.append(dict(name='preview-click-has-no-action', passed=play_spy.count() == 0 and volume_spy.count() == 0))
capture('01-dark-original-size')
for theme in [1, 0, 2]:
    click('setupTheme' + str(theme))
    check('theme-click-' + str(theme), 'dialog.draftTheme', theme)
    check('draft-does-not-save-' + str(theme), 'service.saves', 0)
for accent in range(9):
    click('setupAccent' + str(accent))
    check('accent-click-' + str(accent), 'dialog.draftAccent', accent)
    check('accent-keeps-global-palette-' + str(accent), 'JuxtopposedPalette.accentIndex', 0)
click('setupTheme1')
capture('02-light-gray-preview')
check('draft-leaves-saved-theme', 'service.values.theme', 2)
click('setupNext')
check('next-opens-tour', 'dialog.pageIndex', 1)
check('fullscreen-actual-native-key', 'dialog.keyTokens(["key-toggle-fullscreen"])[0].label', 'F')
check('queue-actual-front-end-key', 'dialog.keyTokens(["@queue"])[0].label', 'CTRL')
check('browser-unassigned-is-honest', 'dialog.keyTokens(["@browser"])[0].label', 'Unassigned')
check('aliases-use-primary-binding', 'dialog.keyTokens(["key-play-pause"])[0].label', 'Space')
check('previous-frame-actual-default', 'dialog.keyTokens(["key-frame-previous"]).map(token => token.label).join(",")', 'SHIFT,E')
check('seeking-groups-stay-complete', 'dialog.keyGroups(["key-jump-extrashort", "key-jump+extrashort"], 153).map(group => group.tokens.map(token => token.label).join("+")).join(";")', 'SHIFT+←;SHIFT+→')
group = find('setupBindingGroup1')
records.append(dict(name='native-size-seeking-chord-one-line', passed=group.height() == 42, actual=group.height()))
call('dialog.activeShortcuts = Object.assign({}, dialog.activeShortcuts, {"key-vol-up": "Ctrl++", "key-play-pause": "+"})')
check('literal-plus-with-modifier', 'dialog.keyTokens(["key-vol-up"]).map(token => token.label).join(",")', 'CTRL,+')
check('literal-plus-alone', 'dialog.keyTokens(["key-play-pause"])[0].label', '+')
call('dialog.activeShortcuts = Object.assign({}, dialog.activeShortcuts, {"key-vol-up": "Ctrl+Up", "key-play-pause": "Space"})')
capture('03-shortcut-tour')
click('setupBack')
check('back-retains-draft', 'dialog.pageIndex === 0 && dialog.draftTheme === 1 && dialog.draftAccent === 8', True)
call('dialog.draftTheme = 0; root.systemDark = false')
check('system-light-preview', 'dialog.tone("#0F0F0F")', '#fafafa')
call('root.systemDark = true')
check('system-dark-preview', 'dialog.tone("#0F0F0F")', '#0f0f0f')
QTest.keyClick(view, Qt.Key_Escape)
QTest.qWait(70)
check('escape-without-save', '!dialog.visible && service.saves === 0', True)
check('escape-restores-opener', 'opener.activeFocus', True)
call('dialog.showDialog(opener); dialog.draftTheme = 1; dialog.draftAccent = 5; dialog.pageIndex = 1; service.fail = true')
click('setupFinish')
check('failed-finish-stays-open', 'dialog.visible && !dialog.busy && service.saves === 1 && service.values.theme === 2', True)
check('failed-finish-error', 'dialog.saveError.length > 0', True)
capture('04-save-failure')
call('service.fail = false')
click('setupFinish')
check('retry-saves-and-closes', '!dialog.visible && service.saves === 2 && service.values.theme === 1 && service.values.accentIndex === 5', True)
check('finish-restores-opener', 'opener.activeFocus', True)
for width, height, scale in [(900, 680, 1), (640, 480, 1), (480, 360, 1), (1280, 900, 1.5)]:
    view.resize(width, height)
    root.setProperty('testScale', scale)
    call('service.lastError = ""; dialog.showDialog(opener)')
    label = f'{width}x{height}-{scale}'
    check('bounded-' + label, 'dialog.x >= 0 && dialog.y >= 0 && dialog.x + dialog.width <= root.width && dialog.y + dialog.height <= root.height', True)
    call('dialog.focusAccent(8)')
    item = find('setupAccent8')
    scroll = find('setupScroll')
    rect = item.mapRectToItem(scroll, item.boundingRect())
    records.append(dict(name='focused-accent-visible-' + label, passed=rect.top() >= -0.5 and rect.bottom() <= scroll.height() + 0.5,
                        actual=[rect.top(), rect.bottom(), scroll.height()]))
    capture('compact-' + label)
    click('setupNext')
    check('compact-tour-' + label, 'dialog.pageIndex', 1)
    scroll = find('setupScroll')
    scroll.setProperty('contentY', max(0, scroll.property('contentHeight') - scroll.height()))
    QTest.qWait(70)
    capture('tour-' + label)
    QTest.keyClick(view, Qt.Key_Escape)
    QTest.qWait(70)

view.close()
QTest.qWait(70)
source_files = [qt / 'dialogs/dialogs/qml/JuxtopposedSetupDialog.qml', qt / 'dialogs/dialogs/qml/JuxtopposedSetupPreview.qml',
                qt / 'style/JuxtopposedPalette.qml', *(qt / f'player/qml/{name}.qml' for name in player_files), Path(__file__)]
result = dict(passed=all(row['passed'] for row in records) and not messages,
              counts=dict(checks=len(records), failures=sum(not row['passed'] for row in records)),
              records=records, diagnostics=messages,
              sourceSha256={str(path.relative_to(repo)): hashlib.sha256(path.read_bytes()).hexdigest() for path in source_files},
              scope='Authored setup QML with supplied native colors, bindings and save service; offscreen component captures only.',
              nativeDesktopAccepted=False, fullBetaAccepted=False)
(out / 'verification.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
print(json.dumps({key: result[key] for key in ['passed', 'counts', 'diagnostics']}))
raise SystemExit(0 if result['passed'] else 1)
