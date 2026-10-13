# Test the real settings choice binding with a native Qt list model.
# Test code licensed under GPLv2 or later.
import os
from pathlib import Path
import tempfile

os.environ.setdefault('QT_QPA_PLATFORM', 'offscreen')
os.environ.setdefault('QT_QUICK_BACKEND', 'software')
os.environ.setdefault('QML_DISABLE_DISK_CACHE', '1')

from PySide6.QtCore import QAbstractListModel, QModelIndex, QObject, Qt, QUrl, Slot
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
source = (repo / 'modules/gui/qt/player/qml/PlaybackSettingsMenu.qml').read_text()
marker = '    component ChoiceSetting: RowLayout {'
assert source.count(marker) == 1
# Exercise the production component and updateSelection implementation directly.
# Only replace native VLC presentation types and attached navigation properties;
# these require a compiled VLC plugin and do not affect choice-model behavior.
choice = source[source.index(marker):].rsplit('\n}', 1)[0]
choice = choice.replace('Widgets.CaptionLabel', 'C.Label')
choice = choice.replace('Widgets.ComboBoxExt', 'C.ComboBox')
choice = choice.replace('VLCStyle.margin_small', '8')
choice = choice.replace('theme.fg.primary', '"white"')
choice = '\n'.join(line for line in choice.splitlines()
                   if 'Navigation.' not in line and 'AccessibleCompat.' not in line)

harness = '''import QtQuick
import QtQuick.Controls as C
import QtQuick.Layouts
import QtQml.Models
Item {
    id: root
    width: 448
    height: 100
    required property var nativeChoices
    ChoiceSetting {
        id: tested
        objectName: "choiceUnderTest"
        width: parent.width
        title: "Aspect ratio"
        accessibleId: "choiceUnderTest"
        choices: root.nativeChoices
    }
''' + choice + '\n}\n'

app = QGuiApplication([])
model = Choices()
with tempfile.TemporaryDirectory(prefix='vlc-settings-choices-', dir=repo) as directory:
    assert Path(directory).resolve().parent == repo.resolve()
    path = Path(directory) / 'ChoiceHarness.qml'
    path.write_text(harness)
    view = QQuickView()
    view.setInitialProperties({'nativeChoices': model})
    view.setSource(QUrl.fromLocalFile(str(path)))
    assert not view.errors(), [error.toString() for error in view.errors()]
    view.show()
    QTest.qWait(30)
    root = view.rootObject()
    field = root.findChild(QObject, 'choiceUnderTest')
    assert field

    def evaluate(code):
        expression = QQmlExpression(qmlContext(field), field, code)
        result = expression.evaluate()
        assert not expression.hasError(), expression.error().toString()
        return result[0]

    assert evaluate('selector.currentIndex') == 0
    assert evaluate('selector.displayText') == 'Default'
    assert not model.activations, 'Initial binding wrote to VLC'

    model.engine_select(2)
    QTest.qWait(20)
    assert evaluate('selector.currentIndex') == 2
    assert evaluate('selector.displayText') == '4:3'
    assert not model.activations, 'Engine selection emitted a user change'

    # A QML activation invokes the same production onActivated handler used by
    # pointer and keyboard actions, without relying on platform popup geometry.
    evaluate('selector.activated(1)')
    QTest.qWait(20)
    assert model.activations == [1], model.activations
    assert evaluate('selector.currentIndex') == 1

    model.engine_select(-1)
    QTest.qWait(20)
    assert evaluate('selector.currentIndex') == -1
    assert evaluate('selector.displayText') == 'Custom'
    assert model.activations == [1]

    model.engine_reset([])
    QTest.qWait(20)
    assert not field.property('enabled'), 'Missing model rows must disable the choice'
    assert evaluate('selector.displayText') == 'Unavailable'
    assert model.activations == [1], 'Model reset wrote to VLC'

    model.engine_reset(['New default', '21:9'], 1)
    QTest.qWait(20)
    assert field.property('enabled')
    assert evaluate('selector.currentIndex') == 1
    assert evaluate('selector.displayText') == '21:9'
    assert model.activations == [1], 'Restored model wrote to VLC'

    field.setProperty('available', False)
    QTest.qWait(20)
    assert not field.property('enabled'), 'Unavailable output must disable the choice'
    assert model.activations == [1]
    view.close()

print('Native checked-model choices pass selection, activation, reset and availability checks.')
