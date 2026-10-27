# SPDX-License-Identifier: GPL-2.0-or-later
"""Production no-library Home/Open File routing with supplied native boundaries."""
import argparse
from hashlib import sha256
import json
import os
from pathlib import Path
import shutil
import subprocess

os.environ.update(QT_QPA_PLATFORM='offscreen', QT_QUICK_BACKEND='software', QML_DISABLE_DISK_CACHE='1')
import PySide6
from PySide6.QtCore import QObject, Property, Signal, Slot, QUrl, Qt, QResource, qInstallMessageHandler
from PySide6.QtGui import QGuiApplication, QFontDatabase, QFont
from PySide6.QtQml import QmlAttached, qmlRegisterType, qmlRegisterSingletonInstance, QQmlExpression, qmlContext
from PySide6.QtQuick import QQuickView
from PySide6.QtTest import QTest

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--repo-root', type=Path, default=Path(__file__).resolve().parents[4])
parser.add_argument('--output-dir', type=Path, required=True)
args = parser.parse_args()
repo, out = args.repo_root.resolve(), args.output_dir.resolve()
out.mkdir(parents=True, exist_ok=False)
os.environ['QT_QPA_FONTDIR'] = str(repo / 'modules/gui/qt/pixmaps/juxtopposed')
messages = []
qInstallMessageHandler(lambda kind, context, text: messages.append(text))
app = QGuiApplication([])
font_id = QFontDatabase.addApplicationFont(str(repo / 'modules/gui/qt/pixmaps/juxtopposed/InterVariable.ttf'))
assert font_id >= 0
app.setFont(QFont(QFontDatabase.applicationFontFamilies(font_id)[0]))

class NavigationProperties(QObject):
    changed = Signal()
    def __init__(self, parent=None):
        super().__init__(parent)
        self.values = {}
    def field(name, notification):
        return Property(QObject, lambda self: self.values.get(name),
                        lambda self, value: self.set_field(name, value),
                        notify=notification)
    def set_field(self, name, value):
        self.values[name] = value
        self.changed.emit()
    parentItem = field('parentItem', changed)
    leftItem = field('leftItem', changed)
    rightItem = field('rightItem', changed)

@QmlAttached(NavigationProperties)
class Navigation(QObject):
    @staticmethod
    def qmlAttachedProperties(owner, target): return NavigationProperties(target)

class IdentifierProperties(QObject):
    changed = Signal()
    def get_id(self): return self.parent().objectName()
    def set_id(self, value): self.parent().setObjectName(value); self.changed.emit()
    id = Property(str, get_id, set_id, notify=changed)

@QmlAttached(IdentifierProperties)
class AccessibleCompat(QObject):
    @staticmethod
    def qmlAttachedProperties(owner, target): return IdentifierProperties(target)

qmlRegisterType(Navigation, 'VLC.Util', 1, 0, 'Navigation')
qmlRegisterType(AccessibleCompat, 'VLC.Util', 1, 0, 'AccessibleCompat')

def write(relative, text):
    path = out / relative
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding='utf-8')

write('imports/VLC/Util/qmldir', 'module VLC.Util\n')
write('imports/VLC/Style/qmldir', 'module VLC.Style\nsingleton VLCStyle 1.0 VLCStyle.qml\nsingleton VLCIcons 1.0 VLCIcons.qml\nColorContext 1.0 ColorContext.qml\n')
write('imports/VLC/Style/VLCStyle.qml', '''pragma Singleton
import QtQml
QtObject { property real margin_large: 24; property int fontSize_xxlarge: 24; function colWidth(n) { return 120*n } }
''')
write('imports/VLC/Style/VLCIcons.qml', 'pragma Singleton\nimport QtQml\nQtObject { property string ic_fluent_folder_open_24_regular: "" }')
write('imports/VLC/Style/ColorContext.qml', 'import QtQml\nQtObject { enum Set { View }\n property int colorSet: 0 }')
write('imports/VLC/MainInterface/qmldir', 'module VLC.MainInterface\nsingleton MainCtx 1.0 MainCtx.qml\n')
write('imports/VLC/MainInterface/MainCtx.qml', '''pragma Singleton
import QtQml
QtObject { property bool juxtopposedControls: false; property bool mediaLibraryAvailable: false
 signal intfDevicePixelRatioChanged()
 function effectiveDevicePixelRatio(window) { return 1 }
 function useXmasCone() { return false }
}
''')
write('imports/VLC/Dialogs/qmldir', 'module VLC.Dialogs\n')
class Dialogs(QObject):
    changed = Signal()
    def __init__(self): super().__init__(); self.actions = []
    @Property('QStringList', notify=changed)
    def calls(self): return self.actions
    def note(self, name): self.actions.append(name); self.changed.emit()
    @Slot()
    def simpleOpenDialog(self): self.note('simple')
    @Slot()
    def openFileDialog(self): self.note('multi')
    @Slot()
    def PLOpenDir(self): self.note('directory')
    @Slot()
    def openDiscDialog(self): self.note('disc')
    @Slot()
    def openNetDialog(self): self.note('network')
    @Slot()
    def openCaptureDialog(self): self.note('capture')
    @Slot()
    def openUrlDialog(self): self.note('clipboard')
    @Slot()
    def addMediaLibraryFolder(self): self.note('addFolder')
dialogs = Dialogs()
qmlRegisterSingletonInstance(Dialogs, 'VLC.Dialogs', 1, 0, 'DialogsProvider', dialogs)
write('imports/VLC/Widgets/qmldir', 'module VLC.Widgets\nSplitButton 1.0 SplitButton.qml\nActionButtonPrimary 1.0 ActionButtonPrimary.qml\n')
write('imports/VLC/Widgets/SplitButton.qml', '''import QtQuick.Controls.Basic
Button { property string iconTxt: ""; property real iconSize: 0; property var model: []; signal menuSelected(int index) }
''')
write('imports/VLC/Widgets/ActionButtonPrimary.qml', 'import QtQuick.Controls.Basic\nButton {}')
sources = {}
for name in ['NoMedialibHome.qml', 'OpenFileSplitButton.qml']:
    source = repo / 'modules/gui/qt/maininterface/qml' / name
    target = out / name
    shutil.copyfile(source, target)
    sources[source.relative_to(repo).as_posix()] = sha256(source.read_bytes()).hexdigest()

cone = repo / 'modules/gui/qt/pixmaps/misc/cone.svg'
write('logo.qrc', '<RCC><qresource prefix="/misc"><file alias="cone.svg">'+cone.as_posix()+'</file></qresource></RCC>')
subprocess.run([str(Path(PySide6.__file__).parent / 'rcc.exe'), '--binary', str(out / 'logo.qrc'), '-o', str(out / 'logo.rcc')], check=True)
assert QResource.registerResource(str(out / 'logo.rcc'))

def expression(item, code):
    value = QQmlExpression(qmlContext(item), item, code)
    result = value.evaluate()[0]
    assert not value.hasError(), value.error().toString()
    return result

cases = []
view = QQuickView()
view.engine().addImportPath(str(out / 'imports'))
view.setResizeMode(QQuickView.SizeRootObjectToView)
view.setSource(QUrl.fromLocalFile(str(out / 'NoMedialibHome.qml')))
assert not view.errors(), [error.toString() for error in view.errors()]
root = view.rootObject()
for redesigned, library in [(False, False), (False, True), (True, False), (True, True)]:
    dialogs.actions.clear(); dialogs.changed.emit()
    expression(root, f'MainCtx.juxtopposedControls={str(redesigned).lower()}; MainCtx.mediaLibraryAvailable={str(library).lower()}')
    view.resize(900, 600)
    view.show()
    QTest.qWait(50)
    primary = root.findChild(QObject, 'openFileButton')
    folder = root.findChild(QObject, 'addLibraryFolderButton')
    assert primary and primary.isVisible()
    assert bool(folder and folder.isVisible()) == (redesigned and library)
    assert expression(primary, 'model.length') == 6
    assert expression(primary, 'model.map(entry => typeof entry === "string" ? entry : entry.text).join("|")') == (
        'Open Multiple Files|Open Directory|Open Disc|Open Network Stream|'
        'Open Capture Device|Open Location from clipboard')
    QTest.mouseClick(view, Qt.LeftButton, Qt.NoModifier, primary.mapToScene(primary.boundingRect().center()).toPoint())
    for index in range(6):
        expression(primary, f'menuSelected({index})')
    expected = 'simple,multi,directory,disc,network,capture,clipboard'
    assert dialogs.actions == expected.split(',')
    if folder and folder.isVisible():
        assert primary.x() + primary.width() <= folder.x()
        QTest.mouseClick(view, Qt.LeftButton, Qt.NoModifier, folder.mapToScene(folder.boundingRect().center()).toPoint())
        expected += ',addFolder'
        assert dialogs.actions == expected.split(',')
    cases.append(dict(redesigned=redesigned, library=library, folderVisible=bool(folder and folder.isVisible()), calls=expected.split(',')))
view.close()
assert not messages, messages
report = dict(passed=True, cases=cases, sourceSha256=sources, messages=messages,
              scope='Actual NoMedialibHome/OpenFileSplitButton. Supplied style, navigation, buttons, menu and native-dialog boundaries; not native picker/menu, artwork or desktop acceptance.')
(out / 'result.json').write_text(json.dumps(report, indent=2)+'\n')
print(json.dumps(report, indent=2))
