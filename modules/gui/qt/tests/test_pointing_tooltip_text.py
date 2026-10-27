# SPDX-License-Identifier: GPL-2.0-or-later
"""Compare production pointing-tooltip text layout with literal Qt text layout."""
import argparse
from hashlib import sha256
import json
import os
from pathlib import Path
import shutil

os.environ['QT_QPA_PLATFORM']='offscreen'
os.environ['QT_QUICK_BACKEND']='software'
os.environ['QML_DISABLE_DISK_CACHE']='1'
os.environ['QT_QPA_FONTDIR']=str(Path(__file__).resolve().parents[4]/'modules/gui/qt/pixmaps/juxtopposed')
from PySide6.QtCore import QObject,Property,QUrl,qInstallMessageHandler
from PySide6.QtGui import QFontDatabase,QGuiApplication,QWindow
from PySide6.QtQml import qmlRegisterUncreatableType
from PySide6.QtQuick import QQuickView
from PySide6.QtTest import QTest

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--repo-root',type=Path,required=True)
parser.add_argument('--output-dir',type=Path,required=True)
parser.add_argument('--mode',choices=['baseline','fixed'],required=True)
args=parser.parse_args();repo=args.repo_root.resolve();out=args.output_dir.resolve()
out.mkdir(parents=True,exist_ok=False)
messages=[];qInstallMessageHandler(lambda kind,context,text:messages.append(text))
app=QGuiApplication([])
font_path=repo/'modules/gui/qt/pixmaps/juxtopposed/InterVariable.ttf'
font_id=QFontDatabase.addApplicationFont(str(font_path));assert font_id>=0
qmlRegisterUncreatableType(QWindow,'VLC.MainInterface',1,0,'QtWindow','Owned window type')
class Context(QObject):
    @Property(bool,constant=True)
    def createWindowWithoutRedirectionSurface(self):return False
context=Context()
def write(path,text):
    path.parent.mkdir(parents=True,exist_ok=True);path.write_text(text,encoding='utf-8',newline='\n')
style=out/'imports/VLC/Style'
write(style/'qmldir','module VLC.Style\nsingleton VLCStyle 1.0 VLCStyle.qml\nColorContext 1.0 ColorContext.qml\n')
write(style/'VLCStyle.qml','''pragma Singleton
import QtQuick
QtObject {
 property int margin_xsmall: 8
 property int margin_xxsmall: 4
 property int margin_xxxsmall: 2
 property int fontSize_normal: 14
 property real scale: 1
 function dp(value,scale) { return value*scale }
}
''')
write(style/'ColorContext.qml','''import QtQuick
QtObject {
 enum Set { Tooltip }
 property int colorSet: 0
 property var bg: ({primary:"#222222"})
 property var fg: ({primary:"#eeeeee"})
 property color border: "#777777"
}
''')
sources={font_path.relative_to(repo).as_posix():sha256(font_path.read_bytes()).hexdigest()}
for name in ['ToolTipExt.qml','PointingTooltip.qml']:
    original=repo/'modules/gui/qt/widgets/qml'/name
    sources[str(original.relative_to(repo)).replace('\\','/')]=sha256(original.read_bytes()).hexdigest()
    target=out/'components'/name;target.parent.mkdir(exist_ok=True)
    shutil.copyfile(original,target)
write(out/'main.qml','''import QtQuick
import "components"
Item {
 width: 800; height: 250
 PointingTooltip { objectName: "tooltipUnderTest"; font.pixelSize: 14 }
 Text { objectName: "literalReference"; font.pixelSize: 14; textFormat: Text.PlainText }
}
''')
view=QQuickView();view.engine().addImportPath(str(out/'imports'))
view.rootContext().setContextProperty('MainCtx',context)
view.setSource(QUrl.fromLocalFile(str(out/'main.qml')))
assert not view.errors(),[e.toString() for e in view.errors()]
root=view.rootObject();assert root
tip=root.findChild(QObject,'tooltipUnderTest');reference=root.findChild(QObject,'literalReference')
assert tip and reference
content=tip.property('contentItem');assert content
font=tip.property('font');font.setFamily(QFontDatabase.applicationFontFamilies(font_id)[0]);tip.setProperty('font',font)
reference.setProperty('font',tip.property('font'))
cases=[]
for name,text in [
    ('plainChapter','00:20 - Middle chapter'),
    ('literalChapterTags','00:20 - Middle <b>äö</b>'),
    ('literalLineBreak','00:40 - Ending <br> next'),
    ('literalBookmarkEntities','Bookmark <i>Evening &amp; morning</i>'),
    ('empty','')]:
    tip.setProperty('text',text);reference.setProperty('text',text);QTest.qWait(30)
    actual_width=content.property('implicitWidth');expected_width=reference.property('implicitWidth')
    actual_height=content.property('implicitHeight');expected_height=reference.property('implicitHeight')
    passed=abs(actual_width-expected_width)<.1 and abs(actual_height-expected_height)<.1
    cases.append(dict(name=name,text=text,actualWidth=actual_width,expectedWidth=expected_width,
        actualHeight=actual_height,expectedHeight=expected_height,passed=passed))
failed={c['name'] for c in cases if not c['passed']}
expected={'literalChapterTags','literalLineBreak','literalBookmarkEntities'}
record=dict(passed=not failed and not messages,negativeBaselineReproduced=args.mode=='baseline' and failed==expected and not messages,
    cases=cases,messages=messages,sourceSha256=sources,helperSha256=sha256(Path(__file__).read_bytes()).hexdigest(),
    fullBetaAccepted=False,scope='Actual production ToolTipExt/PointingTooltip and real Qt text shaping/layout compared with an independent literal Text item. Supplied style/MainCtx dependencies; no ordinary popup placement, OS-DPI or screen-reader acceptance.')
(out/'verification.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps(record,indent=2),flush=True)
view.setSource(QUrl());app.processEvents()
raise SystemExit(0 if record['passed'] or record['negativeBaselineReproduced'] else 1)
