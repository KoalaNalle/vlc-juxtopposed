# SPDX-License-Identifier: GPL-2.0-or-later
"""Exercise production floating notice geometry with supplied theme/notifier state.

The actual notice, SubtitleLabel and LabelExt are loaded unchanged. Only native
style values and the decorative shadow are supplied. Ordinary delivery and
Windows DPI require separate application checks.
"""
import argparse,json,os,shutil
from pathlib import Path
os.environ.update(QT_QPA_PLATFORM='offscreen',QT_QUICK_BACKEND='software',QML_DISABLE_DISK_CACHE='1')
from PySide6.QtCore import QObject,Signal,QUrl,qInstallMessageHandler
from PySide6.QtGui import QGuiApplication,QAccessible
from PySide6.QtQml import QQmlApplicationEngine
from PySide6.QtQuick import QQuickItem
from PySide6.QtTest import QTest
parser=argparse.ArgumentParser();parser.add_argument('--output-dir',type=Path,required=True);args=parser.parse_args()
out=args.output_dir.resolve();out.mkdir(parents=True,exist_ok=False)
repo=Path(__file__).resolve().parents[4]
def write(name,text):
    p=out/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(text,encoding='utf-8',newline='\n')
for name in ['FloatingNotification.qml','SubtitleLabel.qml','LabelExt.qml']:
    shutil.copyfile(repo/'modules/gui/qt/widgets/qml'/name,out/name)
write('DefaultShadow.qml','import QtQuick\nItem {}\n')
write('imports/VLC/MainInterface/qmldir','module VLC.MainInterface\nFixture 1.0 Fixture.qml\n')
write('imports/VLC/MainInterface/Fixture.qml','import QtQml\nQtObject {}\n')
write('imports/VLC/Style/qmldir','module VLC.Style\nsingleton VLCStyle 1.0 VLCStyle.qml\nColorContext 1.0 ColorContext.qml\n')
write('imports/VLC/Style/VLCStyle.qml','''pragma Singleton
import QtQml
QtObject {
 property real scale: 1
 property int margin_normal: 12
 property int margin_small: 8
 property int duration_short: 0
 property int duration_humanMoment: 1000000
 property int delayToolTipAppear: 1000
 property int fontSize_xxxlarge: 32
 function dp(value, scale) { return value * scale }
}
''')
write('imports/VLC/Style/ColorContext.qml','''import QtQuick
QtObject {
 enum ColorSet { Window }
 property int colorSet: ColorContext.Window
 property var bg: ({primary: "#202020"})
 property var fg: ({primary: "white"})
}
''')
write('scene.qml','''import QtQuick
import QtQuick.Controls.Basic as C
C.ApplicationWindow {
 visible: true; width: 400; height: 600; color: "#444444"
 FloatingNotification { objectName: "notices"; anchors { bottom: parent.bottom; left: parent.left; right: parent.right; margins: 16 } }
}
''')
class Notifier(QObject):showNotification=Signal(int,str)
messages=[];qInstallMessageHandler(lambda kind,context,message:messages.append(message))
app=QGuiApplication([]);notifier=Notifier();engine=QQmlApplicationEngine()
engine.addImportPath(str(out/'imports'));engine.rootContext().setContextProperty('UINotifier',notifier)
engine.load(QUrl.fromLocalFile(str(out/'scene.qml')));assert engine.rootObjects(),messages
window=engine.rootObjects()[0];root=window.findChild(QObject,'notices');assert root
rows=[]
def visual(node):
    yield node
    for child in node.childItems():yield from visual(child)
try:
    for width,text in [(400,'Subtitles did not load. Open Add Subtitles to review them.'),(800,'Subtitles did not load. Open Add Subtitles to review them.'),(400,'Volume 100%')]:
        window.setWidth(width);QTest.qWait(40);notifier.showNotification.emit(0,text);QTest.qWait(80)
        rectangles=[p for p in visual(root) if p.metaObject().className().startswith('QQuickRectangle') and p.property('text')==text]
        assert len(rectangles)==1,(len(rectangles),messages)
        rect=rectangles[0];labels=[p for p in visual(rect) if p.property('text')==text and p is not rect];assert len(labels)==1
        label=labels[0];point=rect.mapToScene(rect.boundingRect().topLeft())
        row=dict(windowWidth=width,text=text,width=rect.width(),height=rect.height(),x=point.x(),y=point.y(),labelTruncated=bool(label.property('truncated')))
        rows.append(row);(out/'geometry.json').write_text(json.dumps(rows,indent=2)+'\n',encoding='utf-8')
        assert rect.width()<=root.width()+1,row
        assert point.x()>=-1 and point.x()+rect.width()<=width+1,row
        assert rect.height()>0 and point.y()>=0 and point.y()+rect.height()<=window.height()+1,row
        assert not label.property('truncated'),row
        interface=QAccessible.queryAccessibleInterface(label)
        assert interface and interface.text(QAccessible.Text.Name)==text
        assert interface.role()==QAccessible.Role.AlertMessage
        assert window.grabWindow().save(str(out/f'notice-{width}-{len(rows)}.png'))
    errors=[m for m in messages if any(x in m for x in ['Error:','Cannot assign','Cannot read','is not defined','Binding loop','Unable to assign'])]
    assert not errors,errors
    (out/'verification.json').write_text(json.dumps(dict(status='pass',rows=rows,messages=messages,suppliedStateComponent=True,ordinaryGuiAccepted=False,fullBetaAccepted=False),indent=2)+'\n',encoding='utf-8')
    print('Floating notice geometry checks passed',flush=True)
finally:
    (out/'qml-messages.json').write_text(json.dumps(messages,indent=2)+'\n',encoding='utf-8');window.close();QTest.qWait(20)
