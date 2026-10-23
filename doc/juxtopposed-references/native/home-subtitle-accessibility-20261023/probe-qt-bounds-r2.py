from pathlib import Path
from hashlib import sha256
import json, os

os.environ.update(QT_QPA_PLATFORM='offscreen', QT_QUICK_CONTROLS_STYLE='Basic',
                  QT_QUICK_BACKEND='software')
from PySide6 import __version__
from PySide6.QtCore import QUrl
from PySide6.QtGui import QAccessible, QGuiApplication
from PySide6.QtQml import QQmlComponent
from PySide6.QtQuick import QQuickView

work = Path(__file__).resolve().parent
output = work/'qt-bounds-control-r2.json'
assert not output.exists()
app = QGuiApplication([])
rows, owned = [], []
for aliases in [False, True]:
    view = QQuickView()
    component = QQmlComponent(view.engine())
    compatibility = ('readonly property int minimumValue: Math.min(from, to);'
                     'readonly property int maximumValue: Math.max(from, to);') if aliases else ''
    source = ('import QtQuick\nimport QtQuick.Controls.Basic\nSpinBox {'
              'width:200;height:40;from:-10000;to:10000;value:0;stepSize:50;'
              'property int changes:0; onValueChanged:changes += 1;'+compatibility+'}').encode()
    url = QUrl('file:///owned-spin-bounds-control.qml')
    component.setData(source, url)
    item = component.create()
    assert item, component.errorString()
    view.setContent(url, component, item)
    view.show()
    QAccessible.setActive(True)
    app.processEvents()
    owned.append((view, component))
    interface = QAccessible.queryAccessibleInterface(item)
    row = dict(compatibilityProperties=aliases, accessibilityActive=QAccessible.isActive(),
               hasInterface=bool(interface), role=str(interface.role()) if interface else None,
               fromValue=item.property('from'), toValue=item.property('to'))
    value = interface.valueInterface() if interface else None
    row['hasValueInterface'] = bool(value)
    if value:
        row.update(accessibleMinimum=value.minimumValue(), accessibleMaximum=value.maximumValue(),
                   before=item.property('value'), beforeChanges=item.property('changes'))
        value.setCurrentValue(1200.0)
        app.processEvents()
        row.update(afterDirectQtWrite=item.property('value'), afterChanges=item.property('changes'))
    rows.append(row)
result = dict(qtVersion=__version__, privateMinimalControl=True, windowsUiaTested=False,
              ordinaryVlcAccepted=False, rows=rows,
              producerSha256=sha256(Path(__file__).read_bytes()).hexdigest(),
              scope='Offscreen Qt Quick windows initialize the accessibility factory; pinned Qt SpinBox bounds/value interfaces only. Does not load VLC or Windows UIA.')
output.write_text(json.dumps(result, indent=2)+'\n', encoding='utf-8', newline='\n')
print(json.dumps(result), flush=True)
for view, component in owned:
    view.close()
app.processEvents()
assert all(row['hasValueInterface'] for row in rows)
assert rows[0]['accessibleMinimum'] is None and rows[0]['accessibleMaximum'] is None
assert rows[1]['accessibleMinimum'] == -10000 and rows[1]['accessibleMaximum'] == 10000
assert all(row['afterDirectQtWrite'] == 1200 for row in rows)
