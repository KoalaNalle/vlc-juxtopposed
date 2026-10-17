# SPDX-License-Identifier: GPL-2.0-or-later
"""Actual Performance host/page with explicit owned preferences seam.
Programmatic offscreen coverage; not native history or desktop DPI acceptance.
"""
from pathlib import Path
import argparse,ast,hashlib,json,os,shutil,sys
from copy import deepcopy
from enum import IntEnum
HERE=Path(__file__).resolve().parent
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--repo-root',type=Path,default=HERE.parents[3]);parser.add_argument('--output-dir',type=Path,required=True)
args=parser.parse_args();ROOT=args.repo_root.resolve();results=args.output_dir.resolve();results.mkdir(parents=True,exist_ok=False)
os.environ.update(QT_QPA_PLATFORM='offscreen',QT_QUICK_BACKEND='software',QML_DISABLE_DISK_CACHE='1',QT_QPA_FONTDIR=str(ROOT/'modules/gui/qt/pixmaps/juxtopposed'))
from PySide6.QtCore import QObject,Property,Signal,Slot,QEnum,Qt,QUrl,QPointF,QEvent,QCoreApplication,qInstallMessageHandler
from PySide6.QtGui import QGuiApplication,QKeyEvent,QFontDatabase,QFontInfo
from PySide6.QtQml import QQmlApplicationEngine,QQmlExpression,qmlContext,qmlRegisterType,qmlRegisterSingletonType,qmlRegisterUncreatableType
from PySide6.QtQuick import QQuickItem
from PySide6.QtTest import QTest
sys.path.insert(0,str(ROOT/'modules/gui/qt/tests'))
from juxtopposed_general_preferences_fixture import GeneralPreferences as General
from juxtopposed_library_preferences_fixture import LibraryPreferences
from juxtopposed_controls_preferences_fixture import ControlsPreferences
from juxtopposed_performance_preferences_fixture import PerformancePreferences,SEED
identities={}
def adapter(relative,name,new_name):
    source=ROOT/relative;identities[relative]=hashlib.sha256(source.read_bytes()).hexdigest()
    node=next(n for n in ast.parse(source.read_text(encoding='utf-8')).body if isinstance(n,ast.ClassDef)and n.name==name)
    node.name=new_name;exec(compile(ast.Module(body=[node],type_ignores=[]),str(source),'exec'),globals());return globals()[new_name]
Network=adapter('modules/gui/qt/tests/test_juxtopposed_network_dialog.py','Controller','Network')
Playlist=adapter('modules/gui/qt/tests/test_juxtopposed_new_playlist.py','Controller','Playlist')
Conversion=adapter('modules/gui/qt/tests/test_juxtopposed_conversion_dialog.py','Controller','Conversion')
Provider=adapter('modules/gui/qt/tests/test_juxtopposed_dialogs_host.py','Provider','Provider')
class MainContext(QObject):
    def __init__(self):super().__init__();self.general=General();self.library=LibraryPreferences();self.controls=ControlsPreferences();self.network=PerformancePreferences()
    generalPreferences=Property(QObject,lambda s:s.general,constant=True)
    libraryPreferences=Property(QObject,lambda s:s.library,constant=True)
    controlsPreferences=Property(QObject,lambda s:s.controls,constant=True)
    performancePreferences=Property(QObject,lambda s:s.network,constant=True)
    useGlobalShortcuts=Property(bool,lambda s:True,constant=True)
messages=[];qInstallMessageHandler(lambda kind,context,message:messages.append(message))
app=QGuiApplication([]);ctx=MainContext();model=ctx.network;provider=Provider()
qmlRegisterUncreatableType(Conversion,'VLC.Dialogs',1,0,'ConversionController','Supplied state enum')
qmlRegisterType(QUrl.fromLocalFile(str(ROOT/'modules/gui/qt/widgets/qml/TextInputFocus.qml')),'VLC.Widgets',1,0,'TextInputFocus')
qmlRegisterSingletonType(QUrl.fromLocalFile(str(ROOT/'modules/gui/qt/style/JuxtopposedPalette.qml')),'VLC.Style',1,0,'JuxtopposedPalette')
assets=results/'assets';assets.mkdir();qml=results/'qml';qml.mkdir()
for p in(ROOT/'modules/gui/qt/pixmaps/juxtopposed').iterdir():
    if p.is_file():shutil.copyfile(p,assets/p.name)
for p in(ROOT/'modules/gui/qt/dialogs/dialogs/qml').glob('*.qml'):shutil.copyfile(p,qml/p.name)
engine=QQmlApplicationEngine();engine.rootContext().setContextProperty('suppliedContext',ctx);engine.rootContext().setContextProperty('suppliedProvider',provider)
driver='''import QtQuick
import QtQuick.Controls.Basic as C
import VLC.Style
import "@QML@" as Dialogs
C.ApplicationWindow {
 id:window;visible:true;width:1080;height:897
 property bool omitLibrary:false
 QtObject {id:nativePalette;property bool isDark:suppliedContext.generalPreferences.values.theme!==1}
 Binding {target:JuxtopposedPalette;property:"nativePalette";value:nativePalette}
 Binding {target:JuxtopposedPalette;property:"preferences";value:suppliedContext.generalPreferences}
 QtObject {id:generalOnly;property var generalPreferences:suppliedContext.generalPreferences}
 Rectangle {id:underlay;anchors.fill:parent;color:"#173c66"}
 C.Button {id:opener;objectName:"opener";text:"Settings";onClicked:suppliedProvider.showGlobalSettingsDialog()}
 Dialogs.JuxtopposedDialogsHost {anchors.fill:parent;mainCtx:window.omitLibrary?generalOnly:suppliedContext;dialogsProvider:suppliedProvider;appearance:JuxtopposedPalette;assetBase:"@ASSETS@";backdropSource:underlay;fallbackFocusItem:opener}
}'''.replace('@QML@',qml.as_uri()).replace('@ASSETS@',assets.as_uri()+'/')
(results/'driver.qml').write_text(driver,encoding='utf-8',newline='\n');engine.loadData(driver.encode('utf-8'),QUrl.fromLocalFile(str(results/'driver.qml')))
assert engine.rootObjects(),messages;window=engine.rootObjects()[0];checks=[]
retained={}
def keep(obj):
    if obj is not None:retained[id(obj)]=obj
    return obj
def item(name):
    obj=window.findChild(QObject,name)
    if obj:return keep(obj)
    queue=[window.contentItem()]
    while queue:
        obj=keep(queue.pop())
        if obj.objectName()==name:return obj
        queue.extend(obj.childItems())
    raise AssertionError((name,messages))
def evaluate(obj,text):
    expr=QQmlExpression(qmlContext(obj),obj,text);value,_=expr.evaluate();assert not expr.hasError(),expr.error().toString();QTest.qWait(20);return value.toVariant()if hasattr(value,'toVariant')else value
def focus(name):
    obj=item(name);obj.setFocus(False);obj.forceActiveFocus(Qt.TabFocusReason);QTest.qWait(25);return obj
def click(name):
    obj=focus(name);assert obj.isEnabled()and obj.isVisible(),name
    QTest.mouseClick(window,Qt.LeftButton,Qt.NoModifier,obj.mapToScene(QPointF(obj.width()/2,obj.height()/2)).toPoint());QTest.qWait(30)
def text(value):
    for char in value:
        for kind in[QEvent.KeyPress,QEvent.KeyRelease]:QGuiApplication.sendEvent(window,QKeyEvent(kind,0,Qt.NoModifier,char))
def edit(name,value):
    focus(name);QTest.keyClick(window,Qt.Key_A,Qt.ControlModifier);QTest.keyClick(window,Qt.Key_Backspace);text(value);QTest.keyClick(window,Qt.Key_Return);QTest.qWait(50)
def select(name,index):
    focus(name);QTest.keyClick(window,Qt.Key_Space);QTest.qWait(25);assert evaluate(item(name),'popup.visible');QTest.keyClick(window,Qt.Key_Home)
    for _ in range(index):QTest.keyClick(window,Qt.Key_Down)
    QTest.keyClick(window,Qt.Key_Return);QTest.qWait(25)
def done(name):checks.append(name);print('PASS',name,flush=True)

QTest.qWait(70);click('opener');dialog=item('juxtopposedGlobalSettings');assert evaluate(dialog,'chooseCategory("performance")')
page=item('juxtopposedPerformanceSettings')
assert page.property('service') is model and dialog.property('title')=='Performance Settings'
assert not model.requests and model.reloads>=1 and len(evaluate(page,'rows'))==13
assert item('performanceHardwareDecoder').property('displayText')=='Automatic'
assert item('performancePostprocessing').property('displayText')=='Off'
assert item('performanceLoopFilter').property('displayText')=='Never'
assert item('performanceDecodingThreads').property('text')=='0'
assert item('performanceClockJitter').property('text')=='5000 ms'
assert item('performanceDropLateFrames').property('selected') is True
assert item('performanceDeveloperSection').isVisible()
assert window.grabWindow().save(str(results/'00-performance-defaults-dark.png'))
done('Actual host mounts all14 authored fields/actions without opening writes; Developer starts expanded')
select('performanceHardwareDecoder',2);assert model.requests[-1]==['hardwareDecoder','off']
assert 'Restart' in item('performanceStatus').property('text')
select('performancePostprocessing',3);assert model.state['postprocessing']==3
select('performanceLoopFilter',2);assert model.state['loopFilter']==2
edit('performanceDecodingThreads','4');assert model.state['decodingThreads']==4
edit('performanceDecodingThreads','2.5');assert model.state['decodingThreads']==4 and item('performanceError').isVisible()
edit('performanceDecodingThreads','2');assert model.state['decodingThreads']==2 and not item('performanceError').isVisible()
click('performanceDropLateFrames');assert model.state['dropLateFrames'] is False
edit('performanceProcessPriority','-1');assert model.state['processPriority']==-1
assert item('performanceStatus').property('text')=='Applied and saved.'
model.reject=True;click('performanceRealTimePriority');assert model.state['realTimePriority'] is False and item('performanceError').isVisible()
model.reject=False;edit('performanceProcessPriority','0');assert model.state['processPriority']==0
model.state['realTimePriority']='requested';model.state['prioritySavedRealTime']=True;model.valuesChanged.emit();QTest.qWait(20)
click('performanceRealTimePriority');click('performanceRealTimePriorityCustomOff')
assert model.state['realTimePriority'] is False and not model.state['prioritySavedRealTime']
done('Decoding choices, strict integer correction, signed priority and refused real-time edits preserve state with visible feedback')
select('performanceEncoder',1);assert model.state['encoder']=='flac' and 'conversion' in item('performanceStatus').property('text')
select('performanceDecoder',1);assert model.state['decoder']=='avcodec'
select('performanceClockSync',2);assert model.state['clockSync']==1
edit('performanceClockJitter','1200 ms');assert model.state['clockJitter']==1200
edit('performanceClockAverage','20');assert model.state['clockAverage']==20
model.reject=True;edit('performanceClockAverage','25');assert model.state['clockAverage']==20 and item('performanceClockAverage').property('text')=='25'
model.reject=False;QTest.keyClick(window,Qt.Key_Return);QTest.qWait(30);assert model.state['clockAverage']==25
model.state['decoder']='custom-owned';model.valuesChanged.emit();QTest.qWait(20)
assert item('performanceDecoder').property('currentIndex')==-1 and 'Custom' in item('performanceDecoder').property('displayText')
model.unsupported.add('decodingThreads');model.valuesChanged.emit();QTest.qWait(20);assert not item('performanceDecodingThreads').isEnabled()
model.unsupported.clear();model.valuesChanged.emit()
done('All Developer settings dispatch typed values; rejection/retry, custom selections and unsupported controls stay explicit')
click('performanceAdvanced');assert not item('performanceDeveloperSection').isVisible()
focus('globalSettingsSearch');text('jitter');QTest.qWait(30)
assert item('performanceDeveloperSection').isVisible() and item('performanceJitterRow').isVisible() and not item('performanceEncoderRow').isVisible()
assert evaluate(item('performanceAdvanced'),'Accessible.checked') is True
QTest.keyClick(window,Qt.Key_A,Qt.ControlModifier);QTest.keyClick(window,Qt.Key_Backspace);QTest.qWait(20)
assert not item('performanceDeveloperSection').isVisible()
click('performanceAdvanced');assert item('performanceDeveloperSection').isVisible()
assert dialog.property('visible')
done('Advanced is a local disclosure; search reveals collapsed Developer fields and clearing restores the disclosure state')
click('performanceRescan');assert page.property('busy') and dialog.property('busy')
assert not item('performanceDecodingThreads').isEnabled() and not item('globalSettingsClose').isEnabled()
assert not evaluate(dialog,'chooseCategory("network")')
QTest.qWait(750);assert not page.property('busy') and 'Restart' in item('performanceStatus').property('text')
done('Rescan exposes busy state, blocks competing edits/navigation/close and announces restart after completion')
manifest=json.loads((ROOT/'doc/juxtopposed-references/figma/20261017/performance/manifest.json').read_text())
for name in manifest['assets']:
 assert (assets/('performance-settings-'+name)).read_bytes()==(ROOT/'doc/juxtopposed-references/figma/20261017/performance/assets'/name).read_bytes()
assert item('performanceDecoderInfoOriginal').isVisible()
for name in ['performanceHardwareInfoOriginal','performanceEncoderInfoOriginal','performanceDecoderInfoOriginal','performanceJitterInfoOriginal']:
 assert item(name).property('width')==18 and item(name).property('source').toString().endswith('performance-settings-1f9fe.svg')
assert item('globalCategoryperformanceOriginalIcon').property('source').toString().endswith('performance-settings-4dae7.svg')
assert 'performance-settings-' in item('globalSettingsOriginalSearch').property('source').toString()
assert window.grabWindow().save(str(results/'01-performance-edits-dark.png'))
window.setWidth(650);window.setHeight(720);QTest.qWait(50);assert page.property('stacked')
assert window.grabWindow().save(str(results/'02-performance-narrow-dark.png'))
ctx.general._values['theme']=1;ctx.general.valuesChanged.emit();QTest.qWait(40)
assert window.grabWindow().save(str(results/'03-performance-narrow-light.png'))
window.setWidth(1620);window.setHeight(1345);item('juxtopposedDialogsHost').setProperty('uiScale',1.5);QTest.qWait(50)
assert item('performanceHardwareInfoOriginal').property('scale')==1.5
assert item('performanceHardwareDecoderOriginalArrow').property('width')==24
assert window.grabWindow().save(str(results/'04-performance-light-scale150.png'))
done('Original SVG bytes/intrinsic dimensions, selected category, Dark/Light and narrow/scaled layout remain intact')
errors=[m for m in messages if any(x in m for x in ['ReferenceError','TypeError','is not a type','Unable to assign','Cannot assign','Cannot load','Required property'])]
assert not errors,errors
assert len(checks)==6
(results/'verification.json').write_text(json.dumps(dict(passed=True,checks=checks,checkCount=len(checks),qmlMessages=messages,nativeConsumersAccepted=False,desktopDpiAccepted=False),indent=2)+'\n',encoding='utf-8',newline='\n')
print('PASS:',len(checks),'Performance QML groups',flush=True)
window.close();engine.deleteLater();QCoreApplication.sendPostedEvents(None,QEvent.DeferredDelete);app.processEvents()
