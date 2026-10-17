# SPDX-License-Identifier: GPL-2.0-or-later
"""Actual Network host/page with explicit owned preferences seam.
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
from juxtopposed_network_preferences_fixture import NetworkPreferences,SEED
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
    def __init__(self):super().__init__();self.general=General();self.library=LibraryPreferences();self.controls=ControlsPreferences();self.network=NetworkPreferences()
    generalPreferences=Property(QObject,lambda s:s.general,constant=True)
    libraryPreferences=Property(QObject,lambda s:s.library,constant=True)
    controlsPreferences=Property(QObject,lambda s:s.controls,constant=True)
    networkPreferences=Property(QObject,lambda s:s.network,constant=True)
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

QTest.qWait(70);click('opener');dialog=item('juxtopposedGlobalSettings');assert evaluate(dialog,'chooseCategory("network")')
page=item('juxtopposedNetworkSettings')
assert page.property('service') is model and dialog.property('title')=='Network Connections & Security'
assert not model.requests and model.reloads>=1
assert len(evaluate(page,'rows'))==9
assert item('networkBuffering').property('displayText')=='Custom'
assert item('networkLocalFiles').property('text')=='1000 ms'
assert item('networkCipherPreference').property('displayText')=='Balanced'
assert item('networkSystemTrust').property('selected') is True
assert not item('networkProxyPassword').isEnabled()
assert item('networkHttpProxy').property('placeholderText')=='System proxy'
assert window.grabWindow().save(str(results/'00-network-native-defaults-dark.png'))
done('Actual host mounts nine native-shaped Network rows, preserves Custom native defaults and performs no opening writes')

select('networkBuffering',3);assert model.requests[-1]==['buffering',300]
assert model.state['networkStreams']==1000 and model.state['localFiles']==300
for name in ['networkLocalFiles','networkDiscs','networkStreams','networkLiveCapture']:
    assert not item(name).isEnabled() and item(name).property('text')==''
assert window.grabWindow().save(str(results/'01-network-reference-normal-dark.png'))
select('networkBuffering',0);assert item('networkLocalFiles').isEnabled()
edit('networkLocalFiles','450 ms');assert model.state['localFiles']==450
edit('networkStreams','1.5 ms');assert model.state['networkStreams']==1000 and item('networkError').isVisible()
edit('networkStreams','2500 ms');assert model.state['networkStreams']==2500 and not item('networkError').isVisible()
done('Buffering dropdown disables authored preset fields; Custom validates milliseconds and allows correction')

edit('networkHttpProxy','http://owned-user@proxy.owned.invalid:8080')
assert item('networkProxyPassword').isEnabled()
edit('networkProxyPassword','owned-fixture-password')
assert model._password=='owned-fixture-password' and item('networkProxyPassword').property('text')==''
assert evaluate(item('networkProxyPassword'),'echoMode === TextInput.Password')
assert 'owned-fixture-password' not in str(model.state)
assert item('networkClearPassword').isVisible()
model.reject=True;edit('networkProxyPassword','owned-retry-password')
assert model._password=='owned-fixture-password' and item('networkProxyPassword').property('text')=='owned-retry-password'
assert item('networkError').isVisible()
model.reject=False;QTest.keyClick(window,Qt.Key_Return);QTest.qWait(30)
assert model._password=='owned-retry-password' and item('networkProxyPassword').property('text')==''
click('networkClearPassword');assert not model.state['proxyPasswordConfigured'] and model._password==''
done('Proxy password is masked, absent from values, cleared after save and retained only as an owned draft on rejection')

model.reject=True;edit('networkHttpProxy','direct://')
assert model.state['httpProxy']=='http://owned-user@proxy.owned.invalid:8080' and item('networkError').isVisible()
model.reject=False;edit('networkHttpProxy','direct://')
assert model.state['httpProxy']=='direct://' and not item('networkProxyPassword').isEnabled()
edit('networkHttpProxy','');assert model.state['httpProxy']==''
click('networkSystemTrust');assert model.requests[-1]==['systemTrust',False]
select('networkCipherPreference',2);assert model.state['cipherPreference']=='SECURE128'
model.state['cipherPreference']='CUSTOM-NATIVE';model.valuesChanged.emit();QTest.qWait(20)
assert item('networkCipherPreference').property('currentIndex')==-1 and 'Custom' in item('networkCipherPreference').property('displayText')
model.unsupported.add('systemTrust');model.valuesChanged.emit();QTest.qWait(20);assert not item('networkSystemTrust').isEnabled()
model.unsupported.clear();model.valuesChanged.emit()
done('Proxy rejection/retry, System/direct transitions, TLS choices and unavailable/custom settings remain explicit')

focus('globalSettingsSearch');text('cipher');QTest.qWait(30)
assert item('networkCipherRow').isVisible() and not item('networkProxyRow').isVisible()
QTest.keyClick(window,Qt.Key_A,Qt.ControlModifier);QTest.keyClick(window,Qt.Key_Backspace);QTest.qWait(20)
manifest=json.loads((ROOT/'doc/juxtopposed-references/figma/20261017/network/manifest.json').read_text(encoding='utf-8'))
for name,entry in manifest['assets'].items():
    original=ROOT/'doc/juxtopposed-references/figma/20261017/network/assets'/name
    assert (assets/('network-settings-'+name)).read_bytes()==original.read_bytes()
for name in ['networkBufferingInfoOriginal','networkProxyInfoOriginal','networkPasswordInfoOriginal','networkTrustInfoOriginal','networkCipherInfoOriginal']:
    image=item(name);assert image.property('width')==18 and image.property('height')==18
    assert image.property('source').toString().endswith('network-settings-1f9fe.svg')
for name in ['networkBufferingOriginalArrow','networkCipherPreferenceOriginalArrow','networkAdvancedOriginalChevron','globalSettingsOriginalSearch','globalSettingsOriginalClose']:
    image=item(name);assert image.property('width')==24 and image.property('height')==24
    assert 'network-settings-' in image.property('source').toString()
assert item('globalCategorynetworkOriginalIcon').property('source').toString().endswith('network-settings-7f8c3.svg')
assert window.grabWindow().save(str(results/'02-network-interactions-dark.png'))
done('Search reaches actual Network controls and every visible original help/header/dropdown/category asset keeps intrinsic geometry')

window.setWidth(650);window.setHeight(720);QTest.qWait(50);assert page.property('stacked')
assert window.grabWindow().save(str(results/'03-network-narrow-dark.png'))
ctx.general._values['theme']=1;ctx.general.valuesChanged.emit();QTest.qWait(50)
assert window.grabWindow().save(str(results/'04-network-narrow-light.png'))
window.setWidth(1620);window.setHeight(1345);item('juxtopposedDialogsHost').setProperty('uiScale',1.5);QTest.qWait(50)
assert item('networkBufferingInfoOriginal').property('scale')==1.5
assert item('networkBufferingOriginalArrow').property('width')==24
assert window.grabWindow().save(str(results/'05-network-light-scale150.png'))
done('Dark/Light, narrow and scaled components retain original artwork dimensions and shared settings layout')

click('networkAdvanced');QTest.qWait(40);assert model.requests[-1]==['openAdvanced'] and not dialog.property('visible')
done('Advanced Options dispatches the real-shaped service action and releases the shared QML modal')
errors=[m for m in messages if any(x in m for x in ['ReferenceError','TypeError','is not a type','Unable to assign','Cannot assign','Cannot load','Required property'])]
assert not errors,errors
assert len(checks)==7
(results/'verification.json').write_text(json.dumps(dict(passed=True,checks=checks,checkCount=len(checks),qmlMessages=messages,nativeConsumersAccepted=False,desktopDpiAccepted=False),indent=2)+'\n',encoding='utf-8',newline='\n')
print('PASS:',len(checks),'Network QML groups',flush=True)
window.close();engine.deleteLater();QCoreApplication.sendPostedEvents(None,QEvent.DeferredDelete);app.processEvents()
