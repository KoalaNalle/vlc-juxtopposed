# SPDX-License-Identifier: GPL-2.0-or-later
"""Exercise production Queue/filter layout and action hit targets in real Qt.

The complete panel and exact PlaylistPane popup/inset bindings are used. The
native playlist and SearchBox content are supplied; ordinary VLC must separately
verify the real filter, file picker, playback and keyboard behavior.
"""
import argparse
from hashlib import sha256
import json
import os
from pathlib import Path
import re

os.environ['QT_QPA_PLATFORM']='offscreen'
os.environ['QT_QUICK_BACKEND']='software'
os.environ['QML_DISABLE_DISK_CACHE']='1'
os.environ['QT_QPA_FONTDIR']=str(Path(__file__).resolve().parents[1]/'pixmaps/juxtopposed')
from PySide6.QtCore import QObject, Property, Signal, QPoint, QRectF, Qt, QUrl, qInstallMessageHandler
from PySide6.QtGui import QGuiApplication
from PySide6.QtQml import qmlRegisterSingletonInstance
from PySide6.QtQuick import QQuickView
from PySide6.QtTest import QTest
from juxtopposed_palette_fixture import register_juxtopposed_palette

qt=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--pane-source',type=Path,default=qt/'playlist/qml/PlaylistPane.qml')
parser.add_argument('--panel-source',type=Path,default=qt/'playlist/qml/JuxtopposedQueuePanel.qml')
parser.add_argument('--output-dir',type=Path,required=True)
args=parser.parse_args()
assert not args.output_dir.exists()
args.output_dir.mkdir(parents=True)
pane=args.pane_source.read_text(encoding='utf-8')
bindings=re.findall(r'    Binding \{\n        target: searchBox\.popup\n[\s\S]*?\n    \}',pane)
assert len(bindings)==3, len(bindings)
inset=re.search(r'^            bodyTopInset:.*$',pane,re.M)
panel=args.output_dir/'JuxtopposedQueuePanel.qml'
panel.write_bytes(args.panel_source.read_bytes())
scene='''import QtQuick
import QtQuick.Controls.Basic as C
import VLC.Style
Item {
    id: root
    width: 400; height: 946
    property bool juxtopposed: true
    property int adds: 0
    property int rowClicks: 0
    property alias expanded: searchBox.expanded
    QtObject {
        id: searchBox
        property bool expanded: false
        property var popup: filterPopup
    }
    C.Popup {
        id: filterPopup
        objectName: "filterPopup"
        width: 240 * VLCStyle.scale
        height: 38 * VLCStyle.scale
        padding: 0
        visible: searchBox.expanded
        closePolicy: C.Popup.NoAutoClose
        contentItem: Rectangle {
            color: "#444444"
            MouseArea { anchors.fill: parent }
        }
    }
    JuxtopposedQueuePanel {
        id: queuePanel
        objectName: "panel"
        anchors.fill: parent
        uiScale: VLCStyle.scale
        summary: "2 items - 1m"
        assetBase: "ASSET_BASE"
        onAddRequested: root.adds += 1
        INSET_BINDING
        bodyComponent: Component {
            Item {
                Rectangle {
                    objectName: "firstRow"
                    width: parent.width
                    height: 32 * VLCStyle.scale
                    color: "#555555"
                    MouseArea { anchors.fill: parent; onClicked: root.rowClicks += 1 }
                }
            }
        }
    }
POPUP_BINDINGS
}
'''.replace('ASSET_BASE',QUrl.fromLocalFile(str(qt/'pixmaps/juxtopposed')+'/').toString()).replace('INSET_BINDING',inset.group(0).strip() if inset else '').replace('POPUP_BINDINGS','\n'.join(bindings))
fixture=args.output_dir/'filter-layout.qml'
fixture.write_text(scene,encoding='utf-8',newline='\n')

class Style(QObject):
    changed=Signal()
    def __init__(self):super().__init__();self.value=1.0
    @Property(float,notify=changed)
    def scale(self):return self.value
    def set_scale(self,value):self.value=value;self.changed.emit()

messages=[]
qInstallMessageHandler(lambda kind,context,message:messages.append(message))
app=QGuiApplication([]);style=Style()
register_juxtopposed_palette()
qmlRegisterSingletonInstance(Style,'VLC.Style',1,0,'VLCStyle',style)
view=QQuickView();view.setResizeMode(QQuickView.SizeRootObjectToView)
view.setSource(QUrl.fromLocalFile(str(fixture.resolve())))
assert not view.errors(),[x.toString() for x in view.errors()]
root=view.rootObject();assert root
view.show();view.requestActivate();QTest.qWait(100)

def find(name):
    pending=[root]
    while pending:
        item=pending.pop()
        if item.objectName()==name:return item
        pending.extend(item.childItems())
    raise AssertionError(name)

def rect(item):return item.mapRectToScene(QRectF(0,0,item.width(),item.height()))
def click(item):
    point=rect(item).center()
    QTest.mouseClick(view,Qt.LeftButton,Qt.NoModifier,QPoint(round(point.x()),round(point.y())))
    QTest.qWait(40)

cases=[]
for scale in [1,1.5,2]:
    style.set_scale(scale)
    for width in [320,400]:
        root.setProperty('expanded',False);view.resize(round(width*scale),round(500*scale));QTest.qWait(60)
        row=find('firstRow');add=find('queueAdd');toolbar=find('queueToolbar')
        closed=abs(rect(row).top()-120*scale)<0.6
        root.setProperty('expanded',True);QTest.qWait(60)
        popup=root.findChild(QObject,'filterPopup')
        top=float(popup.property('y'));height=float(popup.property('height'))
        checks=dict(closedReferenceUnchanged=closed,filterBelowToolbar=top>=rect(toolbar).bottom(),
                    rowBelowFilter=rect(row).top()>=top+height+12*scale-0.6)
        previous=root.property('adds');click(add)
        checks['addCenterClickable']=root.property('adds')==previous+1
        previous=root.property('rowClicks');click(row)
        checks['firstRowClickable']=root.property('rowClicks')==previous+1
        root.setProperty('expanded',False);QTest.qWait(60)
        checks['closedGeometryRestored']=abs(rect(row).top()-120*scale)<0.6
        cases.append(dict(scale=scale,width=width,checks=checks,passed=all(checks.values())))
        print(('PASS ' if cases[-1]['passed'] else 'FAIL ')+str(cases[-1]))
view.close();app.processEvents()
result=dict(passed=all(x['passed'] for x in cases) and not messages,cases=cases,diagnostics=messages,
            sourceSha256={p.name:sha256(p.read_bytes()).hexdigest() for p in [args.pane_source,args.panel_source]},
            scope=__doc__)
(args.output_dir/'verification.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8',newline='\n')
assert result['passed'],result
