"""Check the production subtitle preset footer/menu with supplied controller state.

This is presentation evidence. Native save/restart/apply and rendered subtitle
pixels are checked separately by test_subtitle_presets.cpp.
"""
import argparse
import json
import os
import re
import sys
import traceback
sys.excepthook = lambda kind, value, tb: traceback.print_exception(kind, value, tb, file=sys.stderr)
from pathlib import Path

repo = Path(__file__).resolve().parents[4]
os.environ['QT_QPA_PLATFORM'] = 'offscreen'
os.environ['QT_QUICK_BACKEND'] = 'software'
os.environ['QML_DISABLE_DISK_CACHE'] = '1'
os.environ['QT_QPA_FONTDIR'] = str(repo / 'modules/gui/qt/pixmaps/juxtopposed')
from PySide6.QtCore import QObject, Property, QPointF, Qt, QUrl, Signal, Slot, qInstallMessageHandler
from PySide6.QtGui import QFontDatabase, QGuiApplication
from PySide6.QtQml import QQmlExpression, qmlContext
from PySide6.QtQuick import QQuickItem, QQuickView, QQuickWindow
from PySide6.QtTest import QTest

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output-dir', type=Path, default=Path('ui-previews/subtitle-presets'))
args = parser.parse_args()
args.output_dir.mkdir(parents=True, exist_ok=True)
messages = []
def qt_message(kind, context, message):
    messages.append(message)
    with (args.output_dir / 'qml-messages.txt').open('a', encoding='utf-8') as output: output.write(message + '\n')
qInstallMessageHandler(qt_message)
app = QGuiApplication([])
from juxtopposed_palette_fixture import register_juxtopposed_palette
register_juxtopposed_palette()
font_id = QFontDatabase.addApplicationFont(str(repo / 'modules/gui/qt/pixmaps/juxtopposed/InterVariable.ttf'))
assert font_id >= 0
icon_font_id = QFontDatabase.addApplicationFont(str(repo / 'modules/gui/qt/pixmaps/VLCIcons.ttf'))
assert icon_font_id >= 0
icon_font = QFontDatabase.applicationFontFamilies(icon_font_id)[0]
icon_source = (repo / 'modules/gui/qt/style/vlcicons.hpp').read_text(encoding='utf-8')
def icon(name):
    match = re.search(r"QChar " + name + r"\{ u'\\u([a-f0-9]{4})' ", icon_source)
    assert match, name
    return chr(int(match[1], 16))

class Presets(QObject):
    changed = Signal()
    def __init__(self):
        super().__init__()
        self.rows = [{'id': 'default', 'name': 'Default Preset'}, {'id': 'series', 'name': 'Series subtitles'}]
        self.selected = 'default'
        self.values = dict(canSave=True, canApply=True, edited=False, restoring=False, statusText='', error='')
        self.actions = []
    @Property('QVariantList', notify=changed)
    def entries(self):
        return [dict(row, selected=row['id'] == self.selected and not self.values['edited']
                     and not self.values['restoring']) for row in self.rows]
    @Property(bool, notify=changed)
    def canSave(self): return self.values['canSave']
    @Property(bool, notify=changed)
    def canApply(self): return self.values['canApply']
    @Property(bool, notify=changed)
    def edited(self): return self.values['edited']
    @Property(bool, notify=changed)
    def restoring(self): return self.values['restoring']
    @Property(str, notify=changed)
    def statusText(self): return self.values['statusText']
    @Property(str, notify=changed)
    def error(self): return self.values['error']
    @Property(str, notify=changed)
    def selectedId(self): return self.selected
    @Property(str, notify=changed)
    def selectedName(self):
        if self.values['edited']: return 'Custom settings'
        return next(row['name'] for row in self.rows if row['id'] == self.selected)
    def update(self, **values): self.values.update(values); self.changed.emit()
    @Slot()
    def clearError(self): self.update(error='')
    @Slot(str, result=bool)
    def save(self, name):
        self.actions.append(('save', name))
        name = name.strip()
        if any(row['name'].lower() == name.lower() for row in self.rows):
            self.update(error='A subtitle preset with this name already exists.')
            return False
        self.selected = 'new-' + str(len(self.rows))
        self.rows.append({'id': self.selected, 'name': name})
        self.update(edited=False, error='')
        return True
    @Slot(str, result=bool)
    def apply(self, identity):
        self.actions.append(('apply', identity)); self.selected = identity
        self.update(edited=False, error=''); return True
    @Slot(str)
    def action(self, action): self.actions.append((action,))

controller = Presets()
view = QQuickView()
view.setResizeMode(QQuickView.SizeRootObjectToView)
view.rootContext().setContextProperty('testPresets', controller)
view.rootContext().setContextProperty('iconFamily', icon_font)
view.rootContext().setContextProperty('presetIcons', {name: icon(name) for name in ['add', 'ellipsis', 'check', 'chevron_down']})
component = repo / 'modules/gui/qt/player/qml/JuxtopposedSubtitlePresets.qml'
(args.output_dir / component.name).write_text(component.read_text(encoding='utf-8'), encoding='utf-8', newline='\n')
driver = args.output_dir / 'subtitle-presets-driver.qml'
driver.write_text('''import QtQuick
import QtQuick.Controls.Basic as C
import "%s"
Rectangle {
    color: "#111111"
    JuxtopposedSubtitlePresets {
        id: footer
        anchors.centerIn: parent
        width: Math.min(parent.width - 32, 360 * uiScale)
        controller: testPresets
        iconFontFamily: iconFamily
        addIcon: presetIcons.add
        moreIcon: presetIcons.ellipsis
        checkIcon: presetIcons.check
        downIcon: presetIcons.chevron_down
        property real testScale: 1
        uiScale: testScale
        onAdvancedRequested: testPresets.action("advanced")
        onMoveFocusRequested: (current, direction) => {
            const controls = footer.keyboardControls.filter(item => item.enabled && item.visible);
            const index = controls.indexOf(current);
            controls[(index + direction + controls.length) %% controls.length].forceActiveFocus(Qt.TabFocusReason);
        }
    }
}''' % args.output_dir.resolve().as_uri(), encoding='utf-8', newline='\n')
view.resize(520, 700)
view.setSource(QUrl.fromLocalFile(str(driver.resolve())))
assert view.status() == QQuickView.Ready, view.errors()
view.show(); QTest.qWait(100)

def descendants(item):
    yield item
    for child in item.childItems(): yield from descendants(child)
def item(name):
    for child in descendants(view.contentItem()):
        if child.objectName() == name: return child
    raise AssertionError(name)
def click(target):
    center = target.mapToScene(QPointF(target.width() / 2, target.height() / 2))
    QTest.mouseClick(view, Qt.LeftButton, Qt.NoModifier, center.toPoint())
    QTest.qWait(40)
def evaluate(target, expression):
    value = QQmlExpression(qmlContext(target), target, expression)
    result = value.evaluate()
    assert not value.hasError(), value.error().toString()
    return result
def naming():
    click(item('subtitlePresetSelector'))
    click(item('newSubtitlePreset'))
    assert item('subtitlePresetName').hasActiveFocus()
    assert not item('subtitlePresetSave').isEnabled()

selector = item('subtitlePresetSelector')
click(selector)
click(item('subtitlePresetEntry_series'))
assert controller.actions[-1] == ('apply', 'series')
assert selector.property('text') == 'Series subtitles'
naming()
editor = item('subtitlePresetName')
editor.setProperty('text', 'Series subtitles')
click(item('subtitlePresetSave'))
assert editor.property('text') == 'Series subtitles'
assert item('subtitlePresetStatus').property('text').startswith('A subtitle preset')
assert editor.isVisible()
editor.setProperty('text', '  My subtitles  ')
editor.forceActiveFocus(Qt.TabFocusReason)
QTest.keyClick(view, Qt.Key_Return); QTest.qWait(60)
assert controller.selectedName == 'My subtitles'
assert selector.hasActiveFocus()
assert not editor.isVisible()
controller.update(edited=True)
assert selector.property('text') == 'Custom settings'
click(selector)
click(item('subtitlePresetEntry_default'))
assert controller.selectedId == 'default' and not controller.edited
naming()
editor.setProperty('text', 'Keep unsaved')
QTest.keyClick(view, Qt.Key_Escape); QTest.qWait(50)
assert not editor.isVisible() and selector.hasActiveFocus()
assert not any(action == ('save', 'Keep unsaved') for action in controller.actions)
naming()
editor.setProperty('text', 'Do not save while unavailable')
controller.update(canSave=False, canApply=False, statusText='Select a text subtitle track to change its appearance.')
assert not item('subtitlePresetSave').isEnabled()
assert item('subtitlePresetStatus').property('text').startswith('Select a text subtitle')
click(item('subtitlePresetCancel'))
controller.update(canSave=True, canApply=True, statusText='')
click(item('subtitlePresetAdvanced'))
assert controller.actions[-1] == ('advanced',)

footer = selector.parentItem()
captures = []
for width, height, scale in [(520, 700, 1), (320, 480, 1), (720, 900, 1.5)]:
    view.resize(width, height)
    footer.setProperty('testScale', scale)
    QTest.qWait(80)
    assert footer.width() <= width - 32 + .1
    assert selector.width() > 0
    click(selector)
    QTest.qWait(60)
    path = args.output_dir / ('subtitle-presets-%dx%d-%g.png' % (width, height, scale))
    assert view.grabWindow().save(str(path))
    captures.append(str(path.name))
    QTest.keyClick(view, Qt.Key_Escape); QTest.qWait(40)
    naming()
    editor.setProperty('text', 'A named subtitle preset')
    QTest.qWait(60)
    bounds = editor.mapToScene(QPointF(0, 0))
    assert bounds.x() >= 0 and bounds.y() >= 0
    assert bounds.x() + editor.width() <= width + .1
    assert bounds.y() + editor.height() <= height + .1
    name_path = args.output_dir / ('subtitle-preset-name-%dx%d-%g.png' % (width, height, scale))
    assert view.grabWindow().save(str(name_path))
    captures.append(str(name_path.name))
    click(item('subtitlePresetCancel'))

unexpected = [message for message in messages if any(token in message.lower()
    for token in ['referenceerror', 'typeerror', 'unable to assign', 'binding loop', 'is not defined', 'cannot assign'])]
assert not unexpected, unexpected
(args.output_dir / 'subtitle-presets-verification.json').write_text(json.dumps({
    'productionComponent': str(component.relative_to(repo)), 'evidence': 'supplied-controller component only',
    'checks': ['named/default actions', 'empty disabled save', 'error retains name and retry', 'Enter saves trimmed name',
               'selected/edited states', 'Escape cancels and restores focus', 'unavailable state', 'advanced route', 'three sizes/scales'],
    'captures': captures, 'actions': controller.actions, 'qmlMessages': messages,
}, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
view.close()
print('PASS: subtitle preset menu, naming/retry, state, focus, sizes and routes')
