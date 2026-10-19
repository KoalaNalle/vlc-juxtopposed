# SPDX-License-Identifier: GPL-2.0-or-later
"""Actual Privacy/host QML contract, keyboard, errors, artwork and narrow layout.
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
from juxtopposed_privacy_preferences_fixture import PrivacyPreferences,SEED
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
    def __init__(self):super().__init__();self.general=General();self.library=LibraryPreferences();self.privacy=PrivacyPreferences()
    generalPreferences=Property(QObject,lambda s:s.general,constant=True)
    libraryPreferences=Property(QObject,lambda s:s.library,constant=True)
    privacyPreferences=Property(QObject,lambda s:s.privacy,constant=True)
    useGlobalShortcuts=Property(bool,lambda s:True,constant=True)
messages=[];qInstallMessageHandler(lambda kind,context,message:messages.append(message))
app=QGuiApplication([]);ctx=MainContext();model=ctx.privacy;provider=Provider()
qmlRegisterUncreatableType(Conversion,'VLC.Dialogs',1,0,'ConversionController','Supplied state enum')
qmlRegisterType(QUrl.fromLocalFile(str(ROOT/'modules/gui/qt/widgets/qml/TextInputFocus.qml')),'VLC.Widgets',1,0,'TextInputFocus')
qmlRegisterSingletonType(QUrl.fromLocalFile(str(ROOT/'modules/gui/qt/style/JuxtopposedPalette.qml')),'VLC.Style',1,0,'JuxtopposedPalette')
qmlRegisterType(QUrl.fromLocalFile(str(ROOT/'modules/gui/qt/player/qml/JuxtopposedPlaybackBar.qml')),'VLC.Player',1,0,'JuxtopposedPlaybackBar')
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
QTest.qWait(70);click('opener');dialog=item('juxtopposedGlobalSettings');assert evaluate(dialog,'chooseCategory("privacy")')
page=item('juxtopposedPrivacySettings')
assert page.property('service') is model and dialog.property('title')=='Privacy Settings'
assert len(model.state)==8 and not model.requests and model.reloads==1
assert len(evaluate(page,'rows'))==9 and len(evaluate(page,'focusTargets'))==18
assert window.grabWindow().save(str(results/'00-privacy-default-dark.png'))
done('Actual host mounts nine-row Privacy page with separate service and no opening writes')
for key in ['saveRecentlyPlayed','rememberPlaybackPosition','historyExclusion','internetMetadata']:
    assert item('privacy'+key).isEnabled(),key
for key in ['submitPlayedTracks','lastfmUsername','lastfmPassword','lastfmSubmissionUrl']:
    assert not item('privacy'+key).isEnabled(),key
    assert item('privacy'+key+'Info').isEnabled(),key
assert evaluate(item('privacylastfmPassword'),'echoMode === TextInput.Password')
done('Genuine native-shaped gates leave legacy submission and credentials unavailable with accessible information and masked password')
click('privacysaveRecentlyPlayed');assert model.requests[-1]==['saveRecentlyPlayed',False]
click('privacyrememberPlaybackPosition');assert model.requests[-1]==['rememberPlaybackPosition',False]
edit('privacyhistoryExclusion','Private ?音*');assert model.requests[-1]==['historyExclusion','Private ?音*']
done('Toggles and Unicode wildcard text dispatch independent typed keys')
model.reject=True;before=model.state['historyExclusion'];edit('privacyhistoryExclusion','rejected')
assert model.state['historyExclusion']==before and item('privacyError').isVisible()
assert 'owned write rejection' in model.lastError
model.reject=False;edit('privacyhistoryExclusion','accepted');assert not item('privacyError').isVisible()
done('Failed save leaves the service value unchanged and exposes an error; explicit retry succeeds')
click('privacyClearHistory');assert model.busy
for key in ['saveRecentlyPlayed','rememberPlaybackPosition','historyExclusion','internetMetadata']:
    assert not item('privacy'+key).isEnabled()
assert not item('privacyClearHistory').isEnabled()
model.finish(False);QTest.qWait(40);assert item('privacyError').isVisible()
click('privacyClearHistory');model.finish(True);QTest.qWait(40)
assert item('privacyStatus').isVisible() and not item('privacyError').isVisible()
done('Clear has explicit busy, failure and retry states without optimistic success')
click('privacyinternetMetadata');assert item('privacyRestart').isVisible()
assert model.requests[-1]==['internetMetadata',True]
done('Shared metadata key displays restart timing')
focus('globalSettingsSearch');text('password');QTest.qWait(30)
assert item('privacylastfmPasswordRow').isVisible() and not item('privacylastfmUsernameRow').isVisible()
QTest.keyClick(window,Qt.Key_A,Qt.ControlModifier);QTest.keyClick(window,Qt.Key_Backspace);QTest.qWait(30)
# Every SVG is the exact archived original with its original dimensions.
manifest=json.loads((ROOT/'doc/juxtopposed-references/figma/20261017/privacy/manifest.json').read_text(encoding='utf-8'))
for entry in manifest['assets']:
    source=ROOT/'doc/juxtopposed-references/figma/20261017/privacy'/entry['file']
    asset=assets/('privacy-settings-'+source.name)
    assert asset.read_bytes()==source.read_bytes()
image=item('privacyhistoryExclusionInfoOriginal')
assert image.property('width')==18 and image.property('height')==18
assert image.property('source').toString().endswith('privacy-settings-1f9fe.svg')
assert window.grabWindow().save(str(results/'01-privacy-dark.png'))
done('Search includes adapted password label; archived SVG bytes and intrinsic information slot match')
window.setWidth(650);window.setHeight(720);QTest.qWait(60)
assert page.property('stacked')
for row in evaluate(page,'rows'):
    if row.isVisible(): assert row.height()>32
assert window.grabWindow().save(str(results/'02-privacy-narrow.png'))
ctx.general._values['theme']=1;ctx.general.valuesChanged.emit();QTest.qWait(60)
assert window.grabWindow().save(str(results/'03-privacy-light-narrow.png'))
done('Narrow layout stacks all row controls and shares the existing light appearance')
errors=[m for m in messages if any(x in m for x in ['ReferenceError','TypeError','is not a type','Unable to assign','Cannot assign','Cannot load'])]
assert not errors,errors
(results/'verification.json').write_text(json.dumps(dict(passed=True,checks=checks,checkCount=len(checks),qmlMessages=messages,desktopDpiAccepted=False,nativeConsumersAccepted=False),indent=2),encoding='utf-8',newline='\n')
print('PASS:',len(checks),'Privacy QML groups',flush=True)
window.close();engine.deleteLater();QCoreApplication.sendPostedEvents(None,QEvent.DeferredDelete);app.processEvents()
