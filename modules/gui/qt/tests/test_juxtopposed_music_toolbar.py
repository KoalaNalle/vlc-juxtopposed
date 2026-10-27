"""Production toolbar geometry and skip controls with supplied native descriptors.
Native defaults/persistence and actual seeking are separate app/native checks.
"""
import argparse
import json
import os
from pathlib import Path
os.environ.update(QT_QPA_PLATFORM='offscreen', QT_QUICK_BACKEND='software', QML_DISABLE_DISK_CACHE='1')
from PySide6.QtCore import QPointF, Qt, QUrl, qInstallMessageHandler
from PySide6.QtGui import QGuiApplication, QFontDatabase
from PySide6.QtQml import QQmlExpression, qmlContext
from PySide6.QtQuick import QQuickView
from PySide6.QtTest import QTest

repo=Path(__file__).resolve().parents[4]
os.environ['QT_QPA_FONTDIR']=str(repo/'modules/gui/qt/pixmaps/juxtopposed')
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output-dir',type=Path,required=True)
out=parser.parse_args().output_dir;out.mkdir(parents=True,exist_ok=False)
messages=[];qInstallMessageHandler(lambda k,c,m:messages.append(m))
app=QGuiApplication([])
from juxtopposed_palette_fixture import register_juxtopposed_palette
register_juxtopposed_palette()
assets=repo/'modules/gui/qt/pixmaps/juxtopposed'
QFontDatabase.addApplicationFont(str(assets/'InterVariable.ttf'))
scene='''import QtQuick
import "@PLAYER@" as Player
Player.JuxtopposedPlaybackBar {
 id: root; property var actions: []; property bool above: false
 assetBase: "@ASSETS@"
 controlLayout: ({columns:[[3,81,0,80,4],[],[33,97,9]],timelineAbove:above})
 describeTile: id=>({name:String(id),kind:({3:"previous",81:"skip15",0:"play",80:"skip15",4:"next",33:"volume",97:"audio",9:"queue"})[id],backward:id===81})
 tileState: id=>({enabled:true,glyph:"",playing:false,source:id===3?assetBase+"previous.svg":id===4?assetBase+"next.svg":id===0?assetBase+"play.svg":""})
 onTileRequested: id=>{actions=actions.concat([id])}
}'''.replace('@PLAYER@',(repo/'modules/gui/qt/player/qml').as_uri()).replace('@ASSETS@',assets.as_uri()+'/')
(out/'Scene.qml').write_text(scene,encoding='utf-8')
view=QQuickView();view.setResizeMode(QQuickView.SizeRootObjectToView)
view.setSource(QUrl.fromLocalFile(str(out/'Scene.qml')))
assert not view.errors(),[e.toString() for e in view.errors()]
root=view.rootObject();view.resize(1920,54);view.show();view.requestActivate();QTest.qWait(60)
def evaluate(code):
    expression=QQmlExpression(qmlContext(root),root,code)
    value=expression.evaluate()
    assert not expression.hasError(),expression.error().toString()
    result=value[0] if isinstance(value,tuple) else value
    return result.toQObject() if hasattr(result,'toQObject') else result
def descendants(item):
    for child in item.childItems():
        yield child
        yield from descendants(child)
assert not root.property('stacked') and root.implicitHeight()==54
assert root.property('customized')
checks=['wide inline canonical layout is54px']
for tile in [81,80]:
    button=evaluate(f'layoutRow.itemFor({tile})')
    arrow=next(c for c in descendants(button) if c.objectName()=='juxtopposedSkip15Arrow')
    status=QQmlExpression(qmlContext(arrow),arrow,'Number(status)').evaluate()[0]
    assert status==1 and arrow.width()==19.5002 and arrow.height()==7.50015
    point=button.mapToScene(QPointF(button.width()/2,button.height()/2)).toPoint()
    QTest.mouseClick(view,Qt.LeftButton,Qt.NoModifier,point);QTest.qWait(20)
    button.forceActiveFocus(Qt.TabFocusReason)
    QTest.keyClick(view,Qt.Key_Return);QTest.qWait(20)
assert evaluate('JSON.stringify(actions)')=='[81,81,80,80]'
checks.append('both authored arrows load and pointer/Return each emit one correct tile action')
assert view.grabWindow().save(str(out/'music-toolbar-wide.png'))
root.setProperty('above',True);QTest.qWait(20)
assert root.property('stacked') and root.implicitHeight()==94
checks.append('saved alternative layout retains its explicit two-row choice')
root.setProperty('above',False);view.resize(600,94);QTest.qWait(20)
assert root.property('stacked') and root.implicitHeight()==94
checks.append('narrow width retains the existing stacked responsive fallback')
assert view.grabWindow().save(str(out/'music-toolbar-narrow.png'))
view.close();QTest.qWait(20)
(out/'verification.json').write_text(json.dumps(dict(checks=checks,diagnostics=messages,fullBetaAccepted=False),indent=2)+'\n')
assert not messages,messages
print('Passed',len(checks),'music toolbar groups; no Qt diagnostics.')
