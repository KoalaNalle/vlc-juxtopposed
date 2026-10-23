from pathlib import Path
from hashlib import sha256
import json, os

os.environ['QT_QPA_PLATFORM'] = 'offscreen'
os.environ['QT_QUICK_CONTROLS_STYLE'] = 'Basic'
from PySide6 import __version__
from PySide6.QtCore import QUrl
from PySide6.QtGui import QAccessible, QGuiApplication
from PySide6.QtQml import QQmlComponent, QQmlEngine

work = Path(__file__).resolve().parent
output = work/'qt-bounds-control-r1.json'
assert not output.exists()
app = QGuiApplication([])
QAccessible.setActive(True)
engine = QQmlEngine()
rows = []
objects = []
for aliases in [False, True]:
    compatibility = ('readonly property int minimumValue: Math.min(from, to);'
                     'readonly property int maximumValue: Math.max(from, to);') if aliases else ''
    source = ('import QtQuick\nimport QtQuick.Controls.Basic\nSpinBox {'
              'from:-10000;to:10000;value:0;stepSize:50;'
              'property int changes:0; onValueChanged:changes += 1;'+compatibility+'}').encode()
    component = QQmlComponent(engine)
    component.setData(source, QUrl('file:///owned-spin-bounds-control.qml'))
    item = component.create()
    assert item, component.errorString()
    objects.append((component, item))
    app.processEvents()
    interface = QAccessible.queryAccessibleInterface(item)
    assert interface and interface.valueInterface()
    value = interface.valueInterface()
    minimum, maximum = value.minimumValue(), value.maximumValue()
    row = dict(compatibilityProperties=aliases, role=str(interface.role()),
               fromValue=item.property('from'), toValue=item.property('to'),
               accessibleMinimum=minimum, accessibleMaximum=maximum,
               before=item.property('value'), beforeChanges=item.property('changes'))
    value.setCurrentValue(1200.0)
    app.processEvents()
    row.update(afterDirectQtWrite=item.property('value'), afterChanges=item.property('changes'))
    rows.append(row)
result = dict(qtVersion=__version__, privateMinimalControl=True,
              ordinaryVlcAccepted=False, windowsUiaTested=False, rows=rows,
              producerSha256=sha256(Path(__file__).read_bytes()).hexdigest(),
              scope='Offscreen pinned Qt SpinBox range/value interfaces, with and without compatibility properties; does not load the VLC widget or Windows UIA provider.')
output.write_text(json.dumps(result, indent=2)+'\n', encoding='utf-8', newline='\n')
print(json.dumps(result), flush=True)
assert rows[0]['accessibleMinimum'] is None and rows[0]['accessibleMaximum'] is None
assert rows[1]['accessibleMinimum'] == -10000 and rows[1]['accessibleMaximum'] == 10000
assert all(row['afterDirectQtWrite'] == 1200 for row in rows)
for component, item in objects:
    item.deleteLater()
app.processEvents()
