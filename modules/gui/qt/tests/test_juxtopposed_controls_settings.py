# SPDX-License-Identifier: GPL-2.0-or-later
"""Actual Controls/host QML contract, keyboard, errors, artwork and narrow layout.
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
from juxtopposed_controls_preferences_fixture import ControlsPreferences,SEED
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
    def __init__(self):super().__init__();self.general=General();self.library=LibraryPreferences();self.controls=ControlsPreferences()
    generalPreferences=Property(QObject,lambda s:s.general,constant=True)
    libraryPreferences=Property(QObject,lambda s:s.library,constant=True)
    controlsPreferences=Property(QObject,lambda s:s.controls,constant=True)
    useGlobalShortcuts=Property(bool,lambda s:True,constant=True)
messages=[];qInstallMessageHandler(lambda kind,context,message:messages.append(message))
app=QGuiApplication([]);ctx=MainContext();model=ctx.controls;provider=Provider()
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
    focus(name);QTest.keyClick(window,Qt.Key_A,Qt.ControlModifier);text(value);QTest.keyClick(window,Qt.Key_Return);QTest.qWait(50)
def select(name,index):
    focus(name);QTest.keyClick(window,Qt.Key_Space);QTest.qWait(25);assert evaluate(item(name),'popup.visible');QTest.keyClick(window,Qt.Key_Home)
    for _ in range(index):QTest.keyClick(window,Qt.Key_Down)
    QTest.keyClick(window,Qt.Key_Return);QTest.qWait(25)
def done(name):checks.append(name);print('PASS',name,flush=True)

QTest.qWait(70);click('opener');dialog=item('juxtopposedGlobalSettings');assert evaluate(dialog,'chooseCategory("controls")')
page=item('juxtopposedControlsSettings')
assert page.property('service') is model and dialog.property('title')=='Mouse, Keyboard & Remote Controls'
assert not model.requests and model.reloads >= 1, (model.requests,model.reloads,messages)
assert len(evaluate(page,'rows'))==17 and len(evaluate(page,'focusTargets'))==17
assert model.state['linkWheelSettings'] is True
assert item('controlsLinkWheelSettings').property('selected') is True
assert item('controlsVeryShortSkip').property('text')=='3 s'
assert item('controlsSingleClick').property('displayText')=='Play / Pause'
assert item('controlsDoubleClick').property('displayText')=='Toggle fullscreen'
assert window.grabWindow().save(str(results/'00-controls-default-dark.png'))
focus('controlsLinkWheelSettings')
assert window.grabWindow().save(str(results/'00b-controls-linked-remotes-dark.png'))
done('Actual host mounts sixteen original rows and the requested default-ON wheel link without opening writes')
edit('controlsVeryShortSkip','7 s');assert model.requests[-1]==['veryShortSkip',7]
edit('controlsShortSkip','5.5 s');assert model.state['shortSkip']==10 and item('controlsError').isVisible()
edit('controlsShortSkip','19 s');assert model.state['shortSkip']==19 and not item('controlsError').isVisible()
model.reject=True;edit('controlsLongSkip','12 s');assert model.state['longSkip']==300 and item('controlsError').isVisible()
model.reject=False;edit('controlsLongSkip','12 s');assert model.state['longSkip']==12 and not item('controlsError').isVisible()
done('Seconds fields preserve typed values and retain genuine rejection feedback until an explicit successful retry')
select('controlsSingleClick',0);assert model.requests[-1]==['singleClick',0]
select('controlsDoubleClick',1);assert model.requests[-1]==['doubleClick',1]
select('controlsScrollWheel',2);assert model.requests[-1]==['scrollWheel',2]
assert model.state['remoteScrollWheel']==2
select('controlsRemoteScrollWheel',3);assert model.requests[-1]==['remoteScrollWheel',3]
assert model.state['scrollWheel']==3
click('controlsReverseScroll');assert model.requests[-1]==['reverseScroll',True]
assert model.state['remoteReverseScroll'] is True
click('controlsLinkWheelSettings');assert model.requests[-1]==['linkWheelSettings',False]
assert model.state['remoteScrollWheel']==0 and model.state['remoteReverseScroll'] is False
select('controlsRemoteScrollWheel',2);assert model.state['scrollWheel']==3 and model.state['remoteScrollWheel']==2
click('controlsRemoteReverseScroll');assert model.requests[-1]==['remoteReverseScroll',True]
click('controlsLinkWheelSettings');assert model.state['remoteScrollWheel']==3
click('controlsLinkWheelSettings');assert model.state['remoteScrollWheel']==2
assert item('controlsRestart').isVisible()
done('Wheel link defaults ON, mirrors edits in both directions, and OFF restores independent settings through repeated toggles')
click('controlsEditShortcuts');assert model.busy and model.editor=='shortcuts'
QTest.keyClick(window,Qt.Key_Escape);QTest.qWait(30);assert dialog.property('visible')
assert not item('controlsResetShortcuts').isEnabled()
model.finish(True);QTest.qWait(30);assert model.state['shortcutsPreset']=='Custom'
before=model.state['remoteReverseScroll'];click('controlsResetShortcuts')
assert model.requests[-1]==['resetShortcuts'] and model.state['shortcutsPreset']=='Defaults'
assert model.state['remoteReverseScroll']==before
done('Shortcut editor owns the shared modal and scoped reset leaves unrelated settings intact')
click('controlsWebRemote');assert model.busy and model.editor=='remote' and not model.state['webRemoteEnabled']
QTest.keyClick(window,Qt.Key_Escape);QTest.qWait(20);assert dialog.property('visible')
model.finish(False);QTest.qWait(20);assert not model.state['webRemoteEnabled']
click('controlsWebRemote');model.finish(True);QTest.qWait(30)
assert model.state['webRemoteEnabled'] and model.state['remotePasswordConfigured']
edit('controlsRemotePort','18080');assert model.requests[-1]==['remotePort',18080]
assert item('controlsRemoteServiceState').isVisible()
assert 'password' not in model.state
model.state['remoteState']='failed';model.valuesChanged.emit();QTest.qWait(20)
assert 'could not start' in item('controlsRemoteServiceState').property('text')
done('Remote setup preserves OFF while pending/cancelled and exposes service failure without exposing credentials')
model.unsupported.add('singleClick');model.valuesChanged.emit();QTest.qWait(20)
assert not item('controlsSingleClick').isEnabled()
model.unsupported.clear();model.state['scrollWheel']=1;model.valuesChanged.emit();QTest.qWait(20)
assert item('controlsScrollWheel').property('currentIndex')==-1
assert 'Custom' in item('controlsScrollWheel').property('displayText')
select('controlsScrollWheel',1);assert model.state['scrollWheel']==0
done('Unavailable consumers and unknown saved enum values remain explicit without selecting an optimistic default')
focus('globalSettingsSearch');text('reverse');QTest.qWait(30)
assert item('controlsReverseRow').isVisible() and item('controlsRemoteReverseRow').isVisible()
assert not item('controlsSingleRow').isVisible()
QTest.keyClick(window,Qt.Key_A,Qt.ControlModifier);QTest.keyClick(window,Qt.Key_Backspace);QTest.qWait(30)
manifest=json.loads((ROOT/'doc/juxtopposed-references/figma/20261017/controls/manifest.json').read_text(encoding='utf-8'))
for entry in manifest['assets']:
    source=ROOT/'doc/juxtopposed-references/figma/20261017/controls'/entry['file']
    assert (assets/('controls-settings-'+source.name)).read_bytes()==source.read_bytes()
image=item('controlsSingleClickOriginalArrow')
assert image.property('width')==24 and image.property('height')==24
assert image.property('source').toString().endswith('controls-settings-764c9.svg')
assert item('globalCategorycontrolsOriginalIcon').property('source').toString().endswith('controls-settings-46129.svg')
assert window.grabWindow().save(str(results/'01-controls-interaction-dark.png'))
done('Search preserves both authored Remotes/Mouse duplicates and all eighteen original SVG assets remain unchanged')
window.setWidth(650);window.setHeight(720);QTest.qWait(60);assert page.property('stacked')
for row in evaluate(page,'rows'):
    if row.isVisible():assert row.height()>32
assert window.grabWindow().save(str(results/'02-controls-narrow.png'))
ctx.general._values['theme']=1;ctx.general.valuesChanged.emit();QTest.qWait(60)
assert window.grabWindow().save(str(results/'03-controls-light-narrow.png'))
window.setWidth(1620);window.setHeight(1345);item('juxtopposedDialogsHost').setProperty('uiScale',1.5);QTest.qWait(60)
assert item('controlsSingleClickOriginalArrow').property('width')==24
assert item('controlsSingleClickOriginalArrow').property('scale')==1.5
assert window.grabWindow().save(str(results/'04-controls-light-scale150.png'))
done('Normal, narrow, Light and scaled components retain shared settings layout and intrinsic artwork geometry')
errors=[m for m in messages if any(x in m for x in ['ReferenceError','TypeError','is not a type','Unable to assign','Cannot assign','Cannot load','Required property'])]
assert not errors,errors
assert len(checks)==8
(results/'verification.json').write_text(json.dumps(dict(passed=True,checks=checks,checkCount=len(checks),qmlMessages=messages,desktopDpiAccepted=False,nativeConsumersAccepted=False),indent=2)+'\n',encoding='utf-8',newline='\n')
print('PASS:',len(checks),'Controls QML groups',flush=True)
window.close();engine.deleteLater();QCoreApplication.sendPostedEvents(None,QEvent.DeferredDelete);app.processEvents()
