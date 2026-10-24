"""Exercise the production settings popup with supplied controller state.

Presentation/adaptor evidence only; native equalizer output has a separate C++
test. Visual design by Juxtopposed. Test code licensed under GPLv2 or later.
"""
import argparse
import os
import re
from pathlib import Path
import tempfile

repo = Path(__file__).resolve().parents[4]
os.environ['QT_QPA_PLATFORM'] = 'offscreen'
os.environ['QT_QUICK_BACKEND'] = 'software'
os.environ['QML_DISABLE_DISK_CACHE'] = '1'
os.environ['QT_QPA_FONTDIR'] = str(repo / 'modules/gui/qt/pixmaps/juxtopposed')
from PySide6.QtCore import QAbstractListModel, QModelIndex, QObject, Property, Qt, QUrl, Signal, Slot, qInstallMessageHandler
from PySide6.QtGui import QGuiApplication, QFontDatabase
from PySide6.QtQml import QQmlExpression, qmlContext
from PySide6.QtQuick import QQuickView
from PySide6.QtTest import QTest

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output-dir', type=Path, default=Path('ui-previews/settings'))
args = parser.parse_args()
args.output_dir.mkdir(parents=True, exist_ok=True)
messages = []
qInstallMessageHandler(lambda kind, context, message: messages.append(message))
app = QGuiApplication([])
from juxtopposed_palette_fixture import register_juxtopposed_palette
register_juxtopposed_palette()
font_id = QFontDatabase.addApplicationFont(str(repo / 'modules/gui/qt/pixmaps/VLCIcons.ttf'))
assert font_id >= 0
icon_source = (repo / 'modules/gui/qt/style/vlcicons.hpp').read_text(encoding='utf-8')
def icon(name):
    match = re.search(r"QChar " + name + r"\{ u'\\u([a-f0-9]{4})' \}", icon_source)
    assert match, name
    return chr(int(match[1], 16))

def state_property(name, kind, notification):
    def get(self): return self.values[name]
    def set(self, value):
        self.writes.append((name, value))
        self.values[name] = value
        self.changed.emit()
    return Property(kind, get, set, notify=notification)

class State(QObject):
    def __init__(self, values):
        super().__init__()
        self.values = values
        self.writes = []
    def engine(self, name, value):
        self.values[name] = value
        self.changed.emit()

class Choices(QAbstractListModel):
    def __init__(self):
        super().__init__()
        self.rows = ['Default', 'Alternative']
        self.selected = 0
        self.writes = []
    def roleNames(self): return {int(Qt.DisplayRole): b'display', int(Qt.CheckStateRole): b'checked'}
    def rowCount(self, parent=QModelIndex()): return 0 if parent.isValid() else len(self.rows)
    def data(self, index, role=Qt.DisplayRole):
        if not index.isValid(): return None
        if role == Qt.DisplayRole: return self.rows[index.row()]
        if role == Qt.CheckStateRole: return self.selected == index.row()
    @Slot(int)
    def toggleIndex(self, index):
        self.writes.append(index)
        self.selected = index
        self.dataChanged.emit(self.index(0), self.index(len(self.rows) - 1), [int(Qt.CheckStateRole)])

class EqualizerBase(State):
    @Slot()
    def refresh(self): pass
    @Slot(int, float)
    def setBand(self, index, gain):
        self.writes.append(('band', index, gain))
        bands = self.values['bands'].copy()
        bands[index] = gain
        self.engine('bands', bands)
    @Slot(int)
    def setPreset(self, index):
        self.writes.append(('preset', index))
        self.engine('presetIndex', index)

eq_fields = {'available': bool, 'enabled': bool, 'twoPass': bool, 'preamp': float,
             'bands': 'QVariantList', 'frequencies': 'QVariantList', 'presets': 'QStringList', 'presetIndex': int}
eq_notification = Signal()
Equalizer = type('Equalizer', (EqualizerBase,), dict(changed=eq_notification,
    **{name: state_property(name, kind, eq_notification) for name, kind in eq_fields.items()}))
eq = Equalizer({'available': True, 'enabled': False, 'twoPass': False, 'preamp': 12.,
                'bands': [0.] * 10, 'frequencies': [60., 170., 310., 600., 1000., 3000., 6000., 12000., 14000., 16000.],
                'presets': ['Flat', 'Test preset'], 'presetIndex': 0})
transform_notification = Signal()
VideoTransform = type('VideoTransform', (State,), dict(changed=transform_notification,
    **{name: state_property(name, kind, transform_notification)
       for name, kind in {'available': bool, 'flip': int, 'rotation': float}.items()}))
transform = VideoTransform(dict(available=True, flip=0, rotation=0.))
class AppearanceBase(State):
    @Slot()
    def refresh(self): pass
appearance_notification = Signal()
appearance_fields = dict(available=bool, statusText=str, embeddedFormat=bool, fontFamily=str, textColor=str,
                         outlineColor=str, offset=int, sizeMode=int, fontSize=float,
                         fontSizeMinimum=float, fontSizeMaximum=float, outlineSize=float, backgroundOpacity=float)
appearance_fields.update(durationAvailable=bool, durationFactor=float, durationUnit=str, durationDescription=str)
Appearance = type('Appearance', (AppearanceBase,), dict(changed=appearance_notification,
    **{name: state_property(name, kind, appearance_notification) for name, kind in appearance_fields.items()}))
appearance = Appearance(dict(available=True, statusText='Select a text subtitle track to change its appearance.', embeddedFormat=True, fontFamily='Arial', textColor='#FFFFFF',
    outlineColor='#000000', offset=0, sizeMode=0, fontSize=6.25, fontSizeMinimum=0.1,
    fontSizeMaximum=50., outlineSize=4., backgroundOpacity=0., durationAvailable=True, durationFactor=0.,
    durationUnit='', durationDescription='Multiply cue duration; 0 disables.'))
class Tracks(QObject):
    @Property(int, constant=True)
    def count(self): return 2
tracks = Tracks()
choices = {name: Choices() for name in ['zoom', 'aspectRatio', 'crop', 'deinterlace', 'deinterlaceMode', 'audioStereoMode', 'audioMixMode']}
fields = dict(isStarted=bool, hasVideoOutput=bool, ratechangable=bool, rate=float,
              audioDelayMS=int, subtitleDelayMS=int, secondarySubtitleDelayMS=int, subtitleFPS=float,
              equalizer=QObject, videoTransform=QObject, subtitleAppearance=QObject, audioTracks=QObject, subtitleTracks=QObject, videoTracks=QObject)
fields.update({name: QObject for name in choices})
player_notification = Signal()
Player = type('Player', (State,), dict(changed=player_notification,
    **{name: state_property(name, kind, player_notification) for name, kind in fields.items()}))
player = Player(dict(isStarted=True, hasVideoOutput=True, ratechangable=True, rate=1.,
                    audioDelayMS=0, subtitleDelayMS=0, secondarySubtitleDelayMS=0, subtitleFPS=0.,
                    equalizer=eq, videoTransform=transform, subtitleAppearance=appearance, audioTracks=tracks, subtitleTracks=tracks, videoTracks=tracks, **choices))
class Dialogs(QObject):
    def __init__(self): super().__init__(); self.actions = []
    @Slot()
    def extendedDialog(self): self.actions.append('effects')
    @Slot()
    def synchroDialog(self): self.actions.append('sync')
    @Slot()
    def prefsDialog(self): self.actions.append('preferences')
dialogs = Dialogs()
class SubtitlePresets(QObject):
    changed = Signal()
    def __init__(self): super().__init__(); self.actions = []
    @Property('QVariantList', notify=changed)
    def entries(self): return [{'id': 'default', 'name': 'Default Preset', 'selected': True}]
    @Property(str, notify=changed)
    def selectedName(self): return 'Default Preset'
    @Property(str, notify=changed)
    def error(self): return ''
    @Property(str, notify=changed)
    def statusText(self): return '' if appearance.available else appearance.statusText
    @Property(bool, notify=changed)
    def canApply(self): return player.isStarted and appearance.available
    @Property(bool, notify=changed)
    def canSave(self): return self.canApply
    @Slot()
    def clearError(self): pass
    @Slot(str, result=bool)
    def apply(self, identity): self.actions.append(('apply', identity)); return True
    @Slot(str, result=bool)
    def save(self, name): self.actions.append(('save', name)); return True
subtitle_presets = SubtitlePresets()
appearance.changed.connect(subtitle_presets.changed)
player.changed.connect(subtitle_presets.changed)
class Style(QObject):
    changed = Signal()
    def __init__(self): super().__init__(); self.factor = 1.
    @Property(float, notify=changed)
    def scale(self): return self.factor
    @Property(float, constant=True)
    def margin_large(self): return 20.
    @Slot(float, float, result=float)
    def dp(self, value, scale): return value * scale
style = Style()

def descendants(item):
    yield item
    for child in item.childItems(): yield from descendants(child)
def accessible(item, name):
    for child in descendants(item):
        if child.property('title') == name or child.property('text') == name or child.objectName() == name: return child
    raise AssertionError(name)

with tempfile.TemporaryDirectory(prefix='vlc-settings-scene-', dir=repo) as temporary:
    folder = Path(temporary)
    assert folder.resolve().parent == repo.resolve()
    asset_base = (repo / 'modules/gui/qt/pixmaps/juxtopposed').as_uri() + '/'
    for relative in ['player/qml/PlaybackSettingsMenu.qml', 'player/qml/JuxtopposedSettingsPanel.qml', 'player/qml/JuxtopposedSubtitlePresets.qml',
                     'player/qml/JuxtopposedSubtitleTimelineToggle.qml',
                     'widgets/qml/JuxtopposedSettingChoice.qml', 'widgets/qml/JuxtopposedSettingSlider.qml']:
        path = repo / 'modules/gui/qt' / relative
        source = '\n'.join(line for line in path.read_text(encoding='utf-8').splitlines() if not line.startswith('import VLC.') or line == 'import VLC.Style')
        source = source.replace('Widgets.', '').replace('qrc:/juxtopposed/', asset_base)
        (folder / path.name).write_text(source, encoding='utf-8')
    (folder / 'ColorContext.qml').write_text('''import QtQuick
QtObject {
    enum ColorSet { Window }
    property int colorSet: ColorContext.Window
    property var palette: ({})
}
''')
    (folder / 'Scene.qml').write_text('''import QtQuick
Item {
    id: scene
    width: 400; height: 600
    PlaybackSettingsMenu { id: menu; objectName: "testedPopup"; parent: scene; maximumHeight: scene.height; Component.onCompleted: open() }
}
''')
    view = QQuickView()
    view.setResizeMode(QQuickView.SizeRootObjectToView)
    context = view.rootContext()
    for name, value in [('Player', player), ('VLCStyle', style), ('DialogsProvider', dialogs),
                        ('MainCtx', {'intfMainWindow': {'height': 600}, 'windowExtendedMargin': 0, 'subtitlePresets': subtitle_presets}),
                        ('VLCIcons', dict(fontFamily=QFontDatabase.applicationFontFamilies(font_id)[0],
                            **{name: icon(name) for name in ['play', 'tv', 'visualization', 'audiosub', 'add', 'ellipsis', 'check', 'chevron_down']}))]:
        context.setContextProperty(name, value)
    view.setSource(QUrl.fromLocalFile(str(folder / 'Scene.qml')))
    assert not view.errors(), [e.toString() for e in view.errors()]
    view.resize(400, 600)
    view.show()
    QTest.qWait(50)
    popup = view.rootObject().findChild(QObject, 'testedPopup')
    assert popup and popup.property('visible')
    def evaluate(code):
        expression = QQmlExpression(qmlContext(popup), popup, code)
        value, _ = expression.evaluate()
        assert not expression.hasError(), expression.error().toString()
        return value
    def content(): return popup.property('contentItem')
    def page(): return content().property('loadedPage')
    def click(item): QTest.mouseClick(view, Qt.LeftButton, Qt.NoModifier, item.mapToScene(item.boundingRect().center()).toPoint())
    def switch(index):
        click(accessible(content(), 'playerSettingsTab' + str(index)))
        QTest.qWait(30)
        assert content().property('currentTab') == index
    assert not player.writes and not eq.writes and not any(model.writes for model in choices.values())
    speed = accessible(page(), 'Speed')
    player.engine('rate', 1.75)
    QTest.qWait(20)
    assert speed.property('value') == 1.75 and not player.writes, (speed.property('value'), player.rate, player.writes, messages)
    speed.valueRequested.emit(1.25)
    assert player.writes == [('rate', 1.25)]
    editor = accessible(speed, 'settingValueEditor')
    click(editor)
    QTest.keyClick(view, Qt.Key_A, Qt.ControlModifier)
    for character in '1.50x': QTest.keyClick(view, Qt.Key(ord(character.upper())))
    QTest.keyClick(view, Qt.Key_Return)
    QTest.qWait(20)
    assert player.writes[-1] == ('rate', 1.5)
    before_invalid = list(player.writes)
    QTest.keyClick(view, Qt.Key_A, Qt.ControlModifier)
    for character in 'not a rate': QTest.keyClick(view, Qt.Key(ord(character.upper())))
    QTest.keyClick(view, Qt.Key_Return)
    QTest.qWait(20)
    assert player.writes == before_invalid and editor.property('text') == '1.50x'
    switch(1)
    assert not transform.writes
    transform.engine('flip', 1)
    transform.engine('rotation', 90.)
    QTest.qWait(20)
    assert accessible(page(), 'Horizontal').property('checked')
    buttons = [accessible(page(), name) for name in ['None', 'Horizontal', 'Vertical']]
    assert max(button.width() for button in buttons) - min(button.width() for button in buttons) <= 1, [button.width() for button in buttons]
    assert accessible(page(), 'Rotate').property('value') == 90. and not transform.writes
    click(accessible(page(), 'Vertical'))
    assert transform.writes == [('flip', 2)]
    rotation = accessible(page(), 'Rotate')
    rotation_editor = accessible(rotation, 'settingValueEditor')
    click(rotation_editor)
    QTest.keyClick(view, Qt.Key_A, Qt.ControlModifier)
    for character in '37 deg': QTest.keyClick(view, Qt.Key(ord(character.upper())))
    QTest.keyClick(view, Qt.Key_Return)
    QTest.qWait(20)
    assert transform.writes[-1] == ('rotation', 37.)
    before_rotation_refresh = list(transform.writes)
    transform.engine('rotation', 37.5)
    QTest.qWait(20)
    assert rotation_editor.property('text') == '37.5 deg' and transform.writes == before_rotation_refresh
    assert rotation_editor.property('height') - rotation_editor.property('topPadding') - rotation_editor.property('bottomPadding') >= rotation_editor.property('contentHeight')
    transform.engine('available', False)
    QTest.qWait(20)
    assert not rotation.isEnabled() and not accessible(page(), 'Horizontal').isEnabled()
    transform.engine('available', True)
    selector = accessible(accessible(page(), 'Aspect ratio'), 'settingChoiceSelector')
    selector.activated.emit(1)
    assert choices['aspectRatio'].writes == [1]
    switch(2)
    equalizer_switch = accessible(page(), 'Enable equalizer')
    eq.engine('enabled', True)
    QTest.qWait(20)
    assert equalizer_switch.property('checked') and not eq.writes
    click(equalizer_switch)
    assert eq.writes == [('enabled', False)]
    preamp = accessible(page(), 'Preamp')
    preamp.valueRequested.emit(-6.)
    assert eq.writes[-1] == ('preamp', -6.)
    assert len([item for item in descendants(page()) if str(item.property('title') or '').endswith((' Hz', ' kHz'))]) == 10
    delay = accessible(page(), 'Audio track delay')
    delay.valueRequested.emit(-0.125)
    assert player.writes[-1] == ('audioDelayMS', -125)
    switch(3)
    fps = accessible(page(), 'Subtitle frame rate')
    fps_editor = accessible(fps, 'settingValueEditor')
    before_fps_refresh = list(player.writes)
    player.engine('subtitleFPS', 23.976)
    QTest.qWait(20)
    assert fps_editor.property('text') == '23.976 fps' and player.writes == before_fps_refresh
    # Windows accessibility writes the text property without textEdited.
    # Apply that same property route, then commit through the real editor.
    fps_editor.setProperty('text', '29.97')
    click(fps_editor)
    QTest.keyClick(view, Qt.Key_Return)
    QTest.qWait(20)
    assert player.writes[-1] == ('subtitleFPS', 29.97), (player.writes, fps_editor.property('text'))
    before_fps_refresh = list(player.writes)
    player.engine('subtitleFPS', 50.)
    QTest.qWait(20)
    assert fps_editor.property('text') == '50.000 fps' and player.writes == before_fps_refresh
    accessible(page(), 'Subtitle track delay').valueRequested.emit(0.250)
    assert player.writes[-1] == ('subtitleDelayMS', 250)
    assert not appearance.writes
    duration = accessible(page(), 'Subtitle duration factor')
    appearance.engine('durationFactor', 2.5)
    QTest.qWait(20)
    assert duration.property('value') == 2.5 and not appearance.writes
    duration.valueRequested.emit(1.5)
    assert appearance.writes[-1] == ('durationFactor', 1.5)
    before_mode_refresh = list(appearance.writes)
    appearance.engine('durationUnit', ' s')
    appearance.engine('durationDescription', 'Add seconds; 0 disables.')
    QTest.qWait(20)
    assert duration.property('unit') == ' s' and duration.property('helpText') == 'Add seconds; 0 disables.'
    assert appearance.writes == before_mode_refresh
    appearance.engine('durationAvailable', False)
    QTest.qWait(20)
    assert not duration.isEnabled()
    appearance.engine('durationAvailable', True)
    appearance.writes.clear()
    appearance.engine('textColor', '#FF0000')
    QTest.qWait(20)
    color_editor = accessible(accessible(page(), 'Text color'), 'settingTextEditor')
    assert color_editor.property('text') == '#FF0000' and not appearance.writes
    # Scroll via the production focus handler before editing the lower fields.
    color_editor.forceActiveFocus(Qt.TabFocusReason)
    QTest.qWait(20)
    click(color_editor)
    QTest.keyClick(view, Qt.Key_A, Qt.ControlModifier)
    for character in '#00ff00': QTest.keyClick(view, Qt.Key(ord(character.upper())))
    QTest.keyClick(view, Qt.Key_Return)
    QTest.qWait(20)
    assert appearance.writes[-1] == ('textColor', '#00ff00'), (appearance.writes, color_editor.property('text'))
    before_invalid_color = list(appearance.writes)
    QTest.keyClick(view, Qt.Key_A, Qt.ControlModifier)
    for character in 'bad': QTest.keyClick(view, Qt.Key(ord(character.upper())))
    QTest.keyClick(view, Qt.Key_Return)
    QTest.qWait(20)
    assert appearance.writes == before_invalid_color and color_editor.property('text') == '#00ff00'
    pixel_button = accessible(page(), 'Pixels')
    pixel_button.forceActiveFocus(Qt.TabFocusReason)
    QTest.qWait(20)
    click(pixel_button)
    assert appearance.writes[-1] == ('sizeMode', 1)
    assert accessible(page(), 'Font size').property('unit') == ' px'
    accessible(page(), 'Font size').valueRequested.emit(24.)
    assert appearance.writes[-1] == ('fontSize', 24.)
    accessible(page(), 'Subtitle offset').valueRequested.emit(20.)
    assert appearance.writes[-1] == ('offset', 20)
    accessible(page(), 'Outline thickness').valueRequested.emit(3.)
    assert appearance.writes[-1] == ('outlineSize', 3.)
    accessible(page(), 'Background opacity').valueRequested.emit(50.)
    assert appearance.writes[-1] == ('backgroundOpacity', 50.)
    appearance.engine('available', False)
    QTest.qWait(20)
    assert not accessible(page(), 'Font size').isEnabled() and not color_editor.isEnabled()
    appearance.engine('available', True)
    # Loading/destroying tab controls must not make additional engine writes.
    before = (list(player.writes), list(eq.writes), list(transform.writes), list(appearance.writes))
    for index in range(4): switch(index)
    assert before == (player.writes, eq.writes, transform.writes, appearance.writes), (before, player.writes, eq.writes, transform.writes, appearance.writes)
    player.engine('isStarted', False)
    switch(0)
    assert not accessible(page(), 'Speed').isEnabled()
    player.engine('isStarted', True)
    for width, height, scale in [(400, 600, 1), (250, 300, 1), (600, 900, 1.5), (800, 1200, 2)]:
        style.factor = scale; style.changed.emit()
        view.resize(width, height)
        for index in range(4):
            switch(index)
            assert view.grabWindow().save(str(args.output_dir / f'settings-{index}-{width}-{scale}.png'))
    style.factor = 1.; style.changed.emit(); view.resize(400, 600)
    switch(0); switch(3)
    assert view.grabWindow().save(str(args.output_dir / 'subtitle-appearance-top.png'))
    accessible(accessible(page(), 'Background opacity'), 'settingValueEditor').forceActiveFocus(Qt.TabFocusReason)
    QTest.qWait(20)
    assert view.grabWindow().save(str(args.output_dir / 'subtitle-appearance-bottom.png'))
    QTest.keyClick(view, Qt.Key_Escape)
    QTest.qWait(20)
    assert not popup.property('visible')
    view.close()
assert not messages, messages
print('Passed: actual tabbed popup construction, rate/choice/delay/equalizer bindings, model-only synchronization, media availability, closure and scaled scenes.')
print('Native VLC desktop interaction and final visual acceptance remain separate gates.')
