# Test the real settings choice binding with a native Qt list model.
# Test code licensed under GPLv2 or later.
import os
from pathlib import Path
import tempfile

os.environ.setdefault('QT_QPA_PLATFORM', 'offscreen')
os.environ.setdefault('QT_QUICK_BACKEND', 'software')
os.environ.setdefault('QML_DISABLE_DISK_CACHE', '1')
os.environ.setdefault('QT_QPA_FONTDIR', str(Path(__file__).resolve().parents[1] / 'pixmaps/juxtopposed'))

from PySide6.QtCore import QAbstractListModel, QModelIndex, QObject, Property, Qt, QUrl, Signal, Slot, qInstallMessageHandler
from PySide6.QtGui import QGuiApplication
from PySide6.QtQml import QQmlExpression, qmlContext
from PySide6.QtQuick import QQuickView
from PySide6.QtTest import QTest


class Choices(QAbstractListModel):
    """The display/checked role contract exposed by VLCVarChoiceModel."""

    def __init__(self):
        super().__init__()
        self.rows = ['Default', '16:9', '4:3']
        self.selected = 0
        self.activations = []

    def roleNames(self):
        return {int(Qt.DisplayRole): b'display', int(Qt.CheckStateRole): b'checked'}

    def rowCount(self, parent=QModelIndex()):
        return 0 if parent.isValid() else len(self.rows)

    def data(self, index, role=Qt.DisplayRole):
        if not index.isValid() or not 0 <= index.row() < len(self.rows):
            return None
        if role == Qt.DisplayRole:
            return self.rows[index.row()]
        if role == Qt.CheckStateRole:
            return index.row() == self.selected
        return None

    @Slot(int)
    def toggleIndex(self, index):
        self.activations.append(index)
        self.engine_select(index)

    def engine_select(self, index):
        self.selected = index
        if self.rows:
            self.dataChanged.emit(self.index(0), self.index(len(self.rows) - 1),
                                  [int(Qt.CheckStateRole)])

    def engine_reset(self, rows, selected=-1):
        self.beginResetModel()
        self.rows = rows
        self.selected = selected
        self.endResetModel()


repo = Path(__file__).resolve().parents[4]
messages = []
qInstallMessageHandler(lambda kind, context, message: messages.append(message))
app = QGuiApplication([])
model = Choices()
view = QQuickView()
view.setResizeMode(QQuickView.SizeRootObjectToView)
view.setInitialProperties({
    'choices': model, 'title': 'Aspect ratio',
    'assetBase': QUrl.fromLocalFile(str(repo / 'modules/gui/qt/pixmaps/juxtopposed') + '/'),
})
view.setSource(QUrl.fromLocalFile(str(repo / 'modules/gui/qt/widgets/qml/JuxtopposedSettingChoice.qml')))
assert not view.errors(), [error.toString() for error in view.errors()]
view.resize(400, 44)
view.show()
QTest.qWait(30)
field = view.rootObject()

def evaluate(code):
    expression = QQmlExpression(qmlContext(field), field, code)
    result = expression.evaluate()
    assert not expression.hasError(), expression.error().toString()
    return result[0]

assert evaluate('selector.currentIndex') == 0
assert not model.activations, 'Initial synchronization must not write to VLC'
evaluate('selector.popup.open()')
QTest.qWait(20)
assert evaluate('selector.popup.visible')
assert evaluate('selector.popup.contentItem.count') == 3
evaluate('selector.popup.close()')
model.engine_select(2)
QTest.qWait(20)
assert evaluate('selector.currentIndex') == 2
assert evaluate('selector.displayText') == '4:3'
assert not model.activations
evaluate('selector.activated(1)')
QTest.qWait(20)
assert model.activations == [1] and evaluate('selector.currentIndex') == 1
model.engine_select(-1)
QTest.qWait(20)
assert evaluate('selector.currentIndex') == -1 and evaluate('selector.displayText') == 'Custom'
model.engine_reset([])
QTest.qWait(20)
assert not field.property('enabled') and evaluate('selector.displayText') == 'Unavailable'
model.engine_reset(['New default', '21:9'], 1)
QTest.qWait(20)
assert field.property('enabled') and evaluate('selector.currentIndex') == 1
field.setProperty('available', False)
QTest.qWait(20)
assert not field.property('enabled') and model.activations == [1]
view.close()
print('Production settings choice passes actual Qt model selection, activation, reset and availability checks.')


class SpeedPlayer(QObject):
    """The writable rate and rateChanged contract used by PlaybackSpeed."""

    rateChanged = Signal()

    def __init__(self):
        super().__init__()
        self._rate = 1.5
        self.changes = []

    @Property(float, notify=rateChanged)
    def rate(self):
        return self._rate

    @rate.setter
    def rate(self, value):
        self.changes.append(value)
        self.engine_set_rate(value)

    def engine_set_rate(self, value):
        self._rate = value
        self.rateChanged.emit()


class SpeedStyle(QObject):
    @Property(float, constant=True)
    def scale(self):
        return 1

    @Property(int, constant=True)
    def margin_normal(self):
        return 12

    @Property(int, constant=True)
    def margin_small(self):
        return 8

    @Property(int, constant=True)
    def fontSize_normal(self):
        return 12

    @Slot(float, float, result=float)
    def dp(self, value, scale):
        return value * scale


# Load the full production speed control, including its Qt imports, slider,
# preset delegates and ButtonGroup. Replace only native VLC presentation and
# attached navigation/accessibility types. Retain the real Qt Templates import
# so unqualified or unsupported types fail exactly as they do in native VLC.
speed_source = (repo / 'modules/gui/qt/player/qml/PlaybackSpeed.qml').read_text()
speed_source = '\n'.join(line for line in speed_source.splitlines()
                         if not line.startswith('import VLC.')
                         and 'Navigation.' not in line and 'AccessibleCompat.' not in line)
speed_source = speed_source.replace('import QtQuick\n', 'import QtQuick\nimport QtQuick.Controls.Basic as C\n', 1)
for label_type in ['SubtitleLabel', 'CaptionLabel', 'ListLabel']:
    speed_source = speed_source.replace('Widgets.' + label_type, 'C.Label')
speed_source = speed_source.replace('Widgets.SliderExt', 'SpeedSlider')
speed_source = speed_source.replace('Widgets.RadioButtonExt', 'C.RadioButton')
speed_player = SpeedPlayer()
speed_style = SpeedStyle()

with tempfile.TemporaryDirectory(prefix='vlc-settings-speed-', dir=repo) as directory:
    assert Path(directory).resolve().parent == repo.resolve()
    directory = Path(directory)
    (directory / 'ColorContext.qml').write_text('''import QtQuick
QtObject {
    enum ColorSet { Window }
    property int colorSet: ColorContext.Window
    property color accent: "orange"
    property var fg: ({primary: "white", negative: "red"})
}
''')
    (directory / 'SpeedSlider.qml').write_text('''import QtQuick.Controls.Basic as C
C.Slider {
    property var toolTipTextProvider
    property bool toolTipFollowsMouse: false
    readonly property var colorContext: ({fg: {primary: "white", negative: "red"}})
}
''')
    path = directory / 'PlaybackSpeed.qml'
    path.write_text(speed_source)
    speed_view = QQuickView()
    speed_view.engine().rootContext().setContextProperty('Player', speed_player)
    speed_view.engine().rootContext().setContextProperty('VLCStyle', speed_style)
    speed_view.setSource(QUrl.fromLocalFile(str(path)))
    assert not speed_view.errors(), [error.toString() for error in speed_view.errors()]
    speed_root = speed_view.rootObject()
    assert speed_root
    speed_view.resize(448, 320)
    speed_view.show()
    QTest.qWait(30)

    def evaluate_speed(code):
        expression = QQmlExpression(qmlContext(speed_root), speed_root, code)
        result = expression.evaluate()
        assert not expression.hasError(), expression.error().toString()
        return result[0]

    assert not speed_player.changes, 'Initial speed binding wrote to VLC'
    assert evaluate_speed('buttonGroup.buttons.length') == 10
    assert abs(evaluate_speed('sliderToSpeed(slider.value)') - 1.5) < 0.001
    assert evaluate_speed('buttonGroup.checkedButton.modelData') == 1.5

    speed_player.engine_set_rate(2)
    QTest.qWait(20)
    assert evaluate_speed('buttonGroup.checkedButton.modelData') == 2
    assert not speed_player.changes, 'Engine speed update emitted a user change'

    # Invoke the same real Qt button signal used by pointer/keyboard activation.
    evaluate_speed('radioButtonRepeater.itemAt(1).clicked()')
    QTest.qWait(20)
    assert speed_player.changes == [0.5], speed_player.changes
    assert evaluate_speed('buttonGroup.checkedButton.modelData') == 0.5
    assert abs(evaluate_speed('sliderToSpeed(slider.value)') - 0.5) < 0.001

    evaluate_speed('slider.value = 17')
    QTest.qWait(20)
    assert speed_player.rate == 2
    assert speed_player.changes == [0.5, 2], speed_player.changes
    assert evaluate_speed('buttonGroup.checkedButton.modelData') == 2
    speed_view.close()

print('Production playback speed passes Qt Templates loading, grouped presets, rate synchronization and slider activation.')
errors = [message for message in messages if any(marker in message
          for marker in ['TypeError', 'ReferenceError', 'Unable', 'Cannot'])]
assert not errors, errors
