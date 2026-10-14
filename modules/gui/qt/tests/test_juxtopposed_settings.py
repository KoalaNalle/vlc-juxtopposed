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
class Tracks(QObject):
    @Property(int, constant=True)
    def count(self): return 2
tracks = Tracks()
choices = {name: Choices() for name in ['zoom', 'aspectRatio', 'crop', 'deinterlace', 'deinterlaceMode', 'audioStereoMode', 'audioMixMode']}
fields = dict(isStarted=bool, hasVideoOutput=bool, ratechangable=bool, rate=float,
              audioDelayMS=int, subtitleDelayMS=int, secondarySubtitleDelayMS=int, subtitleFPS=float,
              equalizer=QObject, audioTracks=QObject, subtitleTracks=QObject)
fields.update({name: QObject for name in choices})
player_notification = Signal()
Player = type('Player', (State,), dict(changed=player_notification,
    **{name: state_property(name, kind, player_notification) for name, kind in fields.items()}))
player = Player(dict(isStarted=True, hasVideoOutput=True, ratechangable=True, rate=1.,
                    audioDelayMS=0, subtitleDelayMS=0, secondarySubtitleDelayMS=0, subtitleFPS=0.,
                    equalizer=eq, audioTracks=tracks, subtitleTracks=tracks, **choices))
class Dialogs(QObject):
    def __init__(self): super().__init__(); self.actions = []
    @Slot()
    def extendedDialog(self): self.actions.append('effects')
    @Slot()
    def synchroDialog(self): self.actions.append('sync')
dialogs = Dialogs()
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
    for relative in ['player/qml/PlaybackSettingsMenu.qml', 'player/qml/JuxtopposedSettingsPanel.qml',
                     'widgets/qml/JuxtopposedSettingChoice.qml', 'widgets/qml/JuxtopposedSettingSlider.qml']:
        path = repo / 'modules/gui/qt' / relative
        source = '\n'.join(line for line in path.read_text(encoding='utf-8').splitlines() if not line.startswith('import VLC.'))
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
                        ('MainCtx', {'intfMainWindow': {'height': 600}, 'windowExtendedMargin': 0}),
                        ('VLCIcons', dict(fontFamily=QFontDatabase.applicationFontFamilies(font_id)[0],
                            **{name: icon(name) for name in ['play', 'tv', 'visualization', 'audiosub']}))]:
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
    accessible(page(), 'Subtitle track delay').valueRequested.emit(0.250)
    assert player.writes[-1] == ('subtitleDelayMS', 250)
    # Loading/destroying tab controls must not make additional engine writes.
    before = (list(player.writes), list(eq.writes))
    for index in range(4): switch(index)
    assert before == (player.writes, eq.writes), (before, player.writes, eq.writes)
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
    QTest.keyClick(view, Qt.Key_Escape)
    QTest.qWait(20)
    assert not popup.property('visible')
    view.close()
assert not messages, messages
print('Passed: actual tabbed popup construction, rate/choice/delay/equalizer bindings, model-only synchronization, media availability, closure and scaled scenes.')
print('Native VLC desktop interaction and final visual acceptance remain separate gates.')
