# SPDX-License-Identifier: GPL-2.0-or-later
"""Actual production Interface/host QML with an explicit native-V2 contract fixture.
No compiled native consumer, file dialog, screen, tray or restart acceptance.
"""
from pathlib import Path
import argparse,ast,hashlib,json,math,os,shutil,sys
from enum import IntEnum
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[3]
os.environ.update(QT_QPA_PLATFORM='offscreen',QT_QUICK_BACKEND='software',QML_DISABLE_DISK_CACHE='1',QT_QPA_FONTDIR=str(ROOT/'modules/gui/qt/pixmaps/juxtopposed'))
from PySide6.QtCore import QObject,Property,Signal,Slot,QEnum,Qt,QUrl,QPointF,QEvent,QCoreApplication,qInstallMessageHandler
from PySide6.QtGui import QGuiApplication,QKeyEvent
from PySide6.QtQml import QQmlApplicationEngine,QQmlExpression,qmlContext,qmlRegisterType,qmlRegisterSingletonType,qmlRegisterUncreatableType
from PySide6.QtQuick import QQuickItem
from PySide6.QtTest import QTest
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output-dir',type=Path,required=True);parser.add_argument('--repo-root',type=Path,default=ROOT)
args=parser.parse_args();ROOT=args.repo_root.resolve();os.environ['QT_QPA_FONTDIR']=str(ROOT/'modules/gui/qt/pixmaps/juxtopposed');results=args.output_dir.resolve();results.mkdir(parents=True,exist_ok=False)
sys.path.insert(0,str(ROOT/'modules/gui/qt/tests'))
from juxtopposed_general_preferences_fixture import GeneralPreferences as General
from juxtopposed_backdrop_fixture import software_backdrop_state
seed=dict(customSkin=False,skinFile='',embeddedVideo=True,resizeVideo=False,rememberGeometry=True,
    alwaysOnTop=False,windowDecorations=True,titleFileName=True,fullscreenScreen=-1,compactMode=False,
    controlAutoHide=-1,controlHideDelay=777,fullscreenControls=True,systemTray=True,startMinimized=False,
    trackPopup=1,popupDuration=3000,playlistTree=False)
booleans={key for key,value in seed.items() if type(value)is bool}
new_gates={'rememberGeometry','titleFileName','fullscreenScreen','controlAutoHide','controlHideDelay','popupDuration','fullscreenControls'}
unsupported={'playlistTree'}
class InterfacePreferences(QObject):
    valuesChanged=Signal();capabilitiesChanged=Signal();screensChanged=Signal()
    lastErrorChanged=Signal();restartRequiredChanged=Signal();pickerActiveChanged=Signal()
    def __init__(self):
        super().__init__();self.state=dict(seed);self.caps={key:key not in new_gates|unsupported for key in seed}
        self.error='';self.restart=False;self.picking=False;self.requests=[];self.reloads=0;self.pickers=0
        self.fail_next='';self.next_file=None;self.during_picker=None;self.consumer_flags=set()
        self.screen_rows=[dict(index=0,name='Generated screen1'),dict(index=1,name='Generated screen2')]
    values=Property('QVariantMap',lambda s:s.state,notify=valuesChanged)
    capabilities=Property('QVariantMap',lambda s:s.caps,notify=capabilitiesChanged)
    screens=Property('QVariantList',lambda s:s.screen_rows,notify=screensChanged)
    lastError=Property(str,lambda s:s.error,notify=lastErrorChanged)
    restartRequired=Property(bool,lambda s:s.restart,notify=restartRequiredChanged)
    pickerActive=Property(bool,lambda s:s.picking,notify=pickerActiveChanged)
    def feedback(self,text):self.error=text;self.lastErrorChanged.emit()
    def patch(self,**values):
        assert all(key in seed for key in values);self.state={**self.state,**values};self.valuesChanged.emit()
        if 'embeddedVideo'in values:
            self.caps['fullscreenScreen']='fullscreenScreen'in self.consumer_flags and values['embeddedVideo']
            self.caps['fullscreenControls']='fullscreenControls'in self.consumer_flags and values['embeddedVideo']
            self.capabilitiesChanged.emit()
        restart=self.state!=seed
        if restart!=self.restart:self.restart=restart;self.restartRequiredChanged.emit()
    def enable_consumers(self):
        # Supplied flags exercise UI contracts only; Root must keep real native
        # flags false until the associated consumers have actually passed gates.
        self.consumer_flags=set(new_gates);self.caps={key:key not in unsupported for key in seed}
        self.caps['fullscreenScreen']=self.state['embeddedVideo'];self.capabilitiesChanged.emit()
        self.caps['fullscreenControls']=self.state['embeddedVideo'];self.capabilitiesChanged.emit()
    @Slot()
    def reload(self):
        assert not self.picking
        self.reloads+=1;self.feedback('');self.valuesChanged.emit()
    @Slot(str,'QVariant',result=bool)
    def setValue(self,key,value):
        assert key in seed and self.caps[key] and not self.picking,(key,value)
        if key in booleans:assert type(value)is bool
        elif key=='skinFile':assert type(value)is str
        else:
            assert type(value)in[int,float] and math.isfinite(value) and value==int(value)
            assert (0<=value<=10000 if key in ['controlHideDelay','popupDuration'] else -1<=value<len(self.screen_rows) if key=='fullscreenScreen' else value in [0,1,2]),(key,value)
        self.requests.append([key,value])
        if self.fail_next:self.feedback(self.fail_next);self.fail_next='';return False
        if key=='skinFile' and (not Path(value).is_absolute() or not Path(value).is_file() or Path(value).stat().st_size==0 or Path(value).suffix.lower()not in['.vlt','.wsz','.xml']):
            self.feedback('Choose an existing readable VLC skin file.');return False
        if key=='customSkin' and value and not self.state['skinFile']:
            self.feedback('Choose a skin file before enabling custom skins.');return False
        if key=='systemTray' and not value and self.state['startMinimized']:
            self.feedback('Turn off minimized startup before hiding the tray icon.');return False
        self.patch(**{key:value});self.feedback('');return True
    @Slot(result=bool)
    def chooseSkinFile(self):
        assert self.caps['skinFile'] and not self.picking
        self.feedback('')
        self.pickers+=1;self.picking=True;self.pickerActiveChanged.emit()
        if self.during_picker:self.during_picker()
        self.picking=False;self.pickerActiveChanged.emit()
        if self.next_file is None:return False
        result=self.setValue('skinFile',self.next_file);self.next_file=None;return result

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
    def __init__(self):super().__init__();self.general=General();self.interface=InterfacePreferences()
    generalPreferences=Property(QObject,lambda s:s.general,constant=True)
    interfacePreferences=Property(QObject,lambda s:s.interface,constant=True)
    useGlobalShortcuts=Property(bool,lambda s:True,constant=True)
messages=[];qInstallMessageHandler(lambda kind,context,message:messages.append(message))
app=QGuiApplication([]);ctx=MainContext();model=ctx.interface;provider=Provider()
qmlRegisterUncreatableType(Conversion,'VLC.Dialogs',1,0,'ConversionController','Supplied state enum')
qmlRegisterType(QUrl.fromLocalFile(str(ROOT/'modules/gui/qt/widgets/qml/TextInputFocus.qml')),'VLC.Widgets',1,0,'TextInputFocus')
qmlRegisterSingletonType(QUrl.fromLocalFile(str(ROOT/'modules/gui/qt/style/JuxtopposedPalette.qml')),'VLC.Style',1,0,'JuxtopposedPalette')
qmlRegisterType(QUrl.fromLocalFile(str(ROOT/'modules/gui/qt/player/qml/JuxtopposedPlaybackBar.qml')),'VLC.Player',1,0,'JuxtopposedPlaybackBar')
assets=results/'assets';assets.mkdir();qml=results/'qml';qml.mkdir()
for p in (ROOT/'modules/gui/qt/pixmaps/juxtopposed').iterdir():
    if p.is_file():shutil.copyfile(p,assets/p.name)
for p in (ROOT/'modules/gui/qt/dialogs/dialogs/qml').glob('*.qml'):shutil.copyfile(p,qml/p.name)
engine=QQmlApplicationEngine();engine.rootContext().setContextProperty('suppliedContext',ctx);engine.rootContext().setContextProperty('suppliedProvider',provider)
driver='''import QtQuick
import QtQuick.Controls.Basic as C
import VLC.Style
import "@QML@" as Dialogs
C.ApplicationWindow {
 id:window;visible:true;width:1080;height:897
 property bool omitInterface:false
 Rectangle { id:backdropUnderlay;objectName:"settingsTestUnderlay";anchors.fill:parent;color:window.color }
 QtObject { id:nativePalette;property bool isDark:suppliedContext.generalPreferences.values.theme!==1 }
 Binding { target:JuxtopposedPalette;property:"nativePalette";value:nativePalette }
 QtObject { id:generalOnly;property var generalPreferences:suppliedContext.generalPreferences }
 C.Button { id:opener;objectName:"opener";text:"Settings";onClicked:suppliedProvider.showGlobalSettingsDialog() }
 Binding { target:JuxtopposedPalette;property:"preferences";value:suppliedContext.generalPreferences }
 Dialogs.JuxtopposedDialogsHost { anchors.fill:parent;backdropSource:backdropUnderlay;mainCtx:window.omitInterface ? generalOnly : suppliedContext;dialogsProvider:suppliedProvider;appearance:JuxtopposedPalette;assetBase:"@ASSETS@";fallbackFocusItem:opener }
}'''.replace('@QML@',qml.as_uri()).replace('@ASSETS@',assets.as_uri()+'/')
(results/'driver.qml').write_text(driver,encoding='utf-8',newline='\n');engine.loadData(driver.encode('utf-8'),QUrl.fromLocalFile(str(results/'driver.qml')))
assert engine.rootObjects(),messages;window=engine.rootObjects()[0];checks=[]
def item(name):
    obj=window.findChild(QObject,name)
    if obj:return obj
    queue=[window.contentItem()]
    while queue:
        obj=queue.pop()
        if obj.objectName()==name:return obj
        queue.extend(obj.childItems())
    raise AssertionError((name,messages))
def evaluate(obj,text):
    expr=QQmlExpression(qmlContext(obj),obj,text);value,_=expr.evaluate();assert not expr.hasError(),expr.error().toString();QTest.qWait(20);return value.toVariant()if hasattr(value,'toVariant')else value
def focus(name):
    obj=item(name);obj.forceActiveFocus(Qt.TabFocusReason);QTest.qWait(25);return obj
def click(name):
    obj=focus(name);assert obj.isEnabled()and obj.isVisible(),name;QTest.mouseClick(window,Qt.LeftButton,Qt.NoModifier,obj.mapToScene(QPointF(obj.width()/2,obj.height()/2)).toPoint());QTest.qWait(25)
def text(value):
    for char in value:
        for kind in [QEvent.KeyPress,QEvent.KeyRelease]:QGuiApplication.sendEvent(window,QKeyEvent(kind,0,Qt.NoModifier,char))
def edit(name,value):focus(name);QTest.keyClick(window,Qt.Key_A,Qt.ControlModifier);text(value);QTest.keyClick(window,Qt.Key_Tab);QTest.qWait(25)
def select(name,index):
    focus(name);QTest.keyClick(window,Qt.Key_Space);QTest.qWait(20);assert item(name+'Popup').property('visible');QTest.keyClick(window,Qt.Key_Home)
    for _ in range(index):QTest.keyClick(window,Qt.Key_Down)
    QTest.keyClick(window,Qt.Key_Return);QTest.qWait(25)
def done(name):checks.append(name);print('PASS',name,flush=True)
QTest.qWait(70);click('opener');dialog=item('juxtopposedGlobalSettings');assert evaluate(dialog,'chooseCategory("interface")')
backdrop_state=software_backdrop_state(window,dialog,item('settingsTestUnderlay'))
page=item('juxtopposedInterfaceSettings');assert page.property('service')is model and model.reloads==1 and not model.requests
assert len(model.state)==18
for key in seed:
    if key in booleans-{'resizeVideo'}:assert item('interface'+key).property('selected')is seed[key],key
assert item('interfaceresizeVideo').property('currentIndex')==0
for key in new_gates|unsupported:assert not item('interface'+key).isEnabled(),key
assert item('interfaceskinFile').isEnabled() and item('interfaceskinBrowse').isEnabled() and not model.state['customSkin']
historical_record=json.loads((HERE/'fixtures/juxtopposed_interface_v1_obsolete.json').read_text(encoding='utf-8'))
try:
    exec(compile(historical_record['obsoleteLoop'],'historical-Interface-V1-obsolete-OFF-skin-assertion','exec'),globals())
except AssertionError as failure:
    assert failure.args==('skinFile',),failure.args
    historical_result=dict(desiredNegative=True,originalV1Passed=False,reason='Native V2 permits choosing a skin while OFF before enable',failedKey='skinFile')
else:
    raise AssertionError('Historical OFF-skin assertion unexpectedly passed')
assert item('interfacecontrolHideDelay').property('value')==.777
assert item('globalSettingsTitle').property('text')=='Interface Settings'
assert window.grabWindow().save(str(results/'01-interface-native-default-gates.png'))
done('Actual host mounts separate18-key service; new-consumer gates and unsupported tree row disabled;777ms/native legacy mode preserved without writes')

images={};queue=[window.contentItem()]
while queue:
    obj=queue.pop();queue.extend(obj.childItems())
    if not obj.isVisible()or'Original'not in obj.objectName():continue
    url=obj.property('source')
    if not url:continue
    assert url.isLocalFile()and Path(url.toLocalFile()).parent==assets
    assert evaluate(obj,'Number(status)')==1
    name=Path(url.toLocalFile()).name;size=[obj.width(),obj.height()]
    if obj.objectName().startswith('interface')and'OriginalTick'in obj.objectName():
        index=int(obj.objectName().split('OriginalTick')[1]);assert name==('interface-settings-8ba8e.svg'if index in[0,20]else'interface-settings-1ab84.svg')
        assert size==([32,32]if index in[0,20]else[2,8])and obj.parentItem().property('clip')is(index in[0,20])
    elif obj.objectName().startswith('interface')and obj.objectName().endswith('InfoOriginal'):
        assert name=='global-settings-1f9fe.svg'and size==[18,18]and not obj.property('mirror')
    elif obj.objectName().startswith('interface')and obj.objectName().endswith('OriginalChevron'):
        assert name=='global-settings-764c9.svg'and size==[24,24]and obj.property('mirror')
    images[obj.objectName()]=dict(file=name,naturalSize=size,scale=obj.scale(),mirror=obj.property('mirror'),sha256=hashlib.sha256((assets/name).read_bytes()).hexdigest())
assert len([name for name in images if'OriginalTick'in name])==42
assert len([name for name in images if name.startswith('interface')and name.endswith('InfoOriginal')])==15
assert len([name for name in images if name.startswith('interface')and name.endswith('OriginalChevron')])==4
assert images['globalCategoryinterfaceOriginalIcon']['file']=='global-settings-f1a4a.svg'
reference=HERE/'fixtures/juxtopposed_interface_settings_assets.json'
for original,record in json.loads(reference.read_text(encoding='utf-8'))['assets'].items():
    assert any(hashlib.sha256(p.read_bytes()).hexdigest()==record['sha256']for p in assets.glob('*.svg')),original
done('All21 contextual originals retained; actual15 info,4 mirrored choices,42 original tick slots and selected Interface icon verified')

click('interfacecustomSkin');assert not model.state['customSkin'] and item('interfaceSettingsError').isVisible()
before=dict(model.state);click('interfaceskinBrowse');assert model.pickers==1 and model.state==before
skin=results/'Generated skin.vlt';skin.write_bytes(b'Owned component fixture; no native skin-parser acceptance.')
model.next_file=skin.as_posix();click('interfaceskinBrowse');assert model.state['skinFile']==skin.as_posix() and item('interfaceskinFile').property('text')==skin.as_posix()
click('interfacecustomSkin');assert model.state['customSkin'] and model.restart
model.fail_next='The file could not be saved.';edit('interfaceskinFile',str(results/'missing.vlt'))
assert model.state['skinFile']==skin.as_posix() and item('interfaceskinFile').property('text')==skin.as_posix()
assert item('interfaceSettingsError').property('text')=='The file could not be saved.'
assert not item('globalSettingsError').isVisible() and item('interfaceSettingsRestart').isVisible()
done('First-use skin flow chooses file while OFF before enable; supplied picker cancel/select and rejected text preserve real values/feedback')

def while_picker():
    assert page.property('busy')and dialog.property('busy')and not item('interfaceskinBrowse').isEnabled()
    assert not item('generalAdvanced').isEnabled() and item('globalSettingsSearch').property('readOnly')
    before=(model.reloads,model.pickers,dict(model.state))
    for signal in [provider.showGlobalSettingsDialog,provider.showNetworkStreamDialog,provider.showNewPlaylistDialog,provider.showConversionDialog]:signal.emit();QTest.qWait(20)
    assert dialog.property('visible')and not evaluate(dialog,'chooseCategory("general")')
    QTest.keyClick(window,Qt.Key_Escape);QTest.qWait(20);assert dialog.property('visible')
    assert before==(model.reloads,model.pickers,model.state)
model.during_picker=while_picker;click('interfaceskinBrowse');model.during_picker=None
assert not page.property('busy')and not model.picking
done('Native-compatible pickerActive guards actual category/Advanced/Escape/repeated/cross-modal routes without second picker,reload or cancellation')

model.enable_consumers();QTest.qWait(25)
for key in unsupported:assert not item('interface'+key).isEnabled()
for key in ['rememberGeometry','alwaysOnTop','windowDecorations','titleFileName','compactMode','fullscreenControls','embeddedVideo','resizeVideo','systemTray','startMinimized']:
    if key=='resizeVideo':select('interfaceresizeVideo',1);assert model.requests[-1]==['resizeVideo',True];continue
    old=model.state[key];click('interface'+key);assert model.requests[-1]==[key,not old];click('interface'+key)
select('interfacecontrolAutoHide',2);assert model.requests[-1]==['controlAutoHide',2]
select('interfacefullscreenScreen',2);assert model.requests[-1]==['fullscreenScreen',1]
select('interfacetrackPopup',2);assert model.requests[-1]==['trackPopup',2]
for key in ['controlHideDelay','popupDuration']:
    slider=focus('interface'+key);before=model.state[key];QTest.keyClick(window,Qt.Key_Right);QTest.qWait(20)
    assert model.requests[-1]==[key,before+500] and abs(slider.property('value')-(before+500)/1000)<.0001
done('Supplied accepted-consumer phase drives actual bool/native-enum/screen/integral-duration controls; forced unsupported rows never receive writes')

model.fail_next='The mode could not be saved.';before=model.state['controlAutoHide'];select('interfacecontrolAutoHide',0)
assert model.state['controlAutoHide']==before and item('interfacecontrolAutoHide').property('currentIndex')==2
model.fail_next='The delay could not be saved.';before=model.state['controlHideDelay'];focus('interfacecontrolHideDelay');QTest.keyClick(window,Qt.Key_Right);QTest.qWait(25)
assert model.state['controlHideDelay']==before and abs(item('interfacecontrolHideDelay').property('value')-before/1000)<.0001
model.fail_next='The toggle could not be saved.';before=model.state['alwaysOnTop'];click('interfacealwaysOnTop')
assert model.state['alwaysOnTop']is before and item('interfacealwaysOnTop').property('selected')is before
model.feedback('');model.patch(**seed);QTest.qWait(25);assert not model.restart and not item('interfaceSettingsRestart').isVisible()
done('Choice/range/toggle failures roll back; returning effective saved values to initial state clears actual-service restart feedback')

window.resize(1280,900);dialog.setProperty('uiScale',1);QTest.qWait(40)
viewport=item('globalSettingsPageScroll');viewport.setProperty('contentY',0);QTest.qWait(25)
toggle=focus('interfacefullscreenControls')
assert not model.restart
click('interfacefullscreenControls');QTest.qWait(80)
assert model.restart and item('interfaceSettingsRestart').isVisible()
viewport=item('globalSettingsPageScroll')
point=toggle.mapToScene(QPointF(0,0));top=viewport.mapToScene(QPointF(0,0)).y()
assert point.y()>=top-.5 and point.y()+toggle.height()<=top+viewport.height()+.5,(
    'Focused fullscreen control clipped after restart notice inserted',point.y(),toggle.height(),top,viewport.height())
assert window.grabWindow().save(str(results/'03-focused-control-after-restart-feedback.png'))
click('interfacefullscreenControls');assert not model.restart
window.resize(1080,897);QTest.qWait(25)
done('Focused control stays fully revealed when restart feedback changes the actual host layout')

model.patch(fullscreenScreen=2,controlAutoHide=-1);QTest.qWait(20)
assert item('interfacefullscreenScreen').property('currentIndex')==-1 and item('interfacecontrolAutoHide').property('currentIndex')==-1
before=len(model.requests);model.screen_rows.append(dict(index=2,name='Generated reconnected screen'));model.screensChanged.emit();QTest.qWait(20)
assert item('interfacefullscreenScreen').property('currentIndex')==3
model.screen_rows.pop();model.screensChanged.emit();QTest.qWait(20);assert item('interfacefullscreenScreen').property('currentIndex')==-1 and len(model.requests)==before
model.patch(embeddedVideo=False,systemTray=False);QTest.qWait(20)
for key in ['resizeVideo','fullscreenScreen','fullscreenControls','startMinimized','trackPopup','popupDuration']:assert not item('interface'+key).isEnabled(),key
model.patch(**seed);model.enable_consumers();model.caps['skinFile']=False;model.caps['customSkin']=False;model.capabilitiesChanged.emit();QTest.qWait(20)
assert not item('interfaceskinFile').isEnabled()and not item('interfaceskinBrowse').isEnabled()and not item('interfacecustomSkin').isEnabled()
model.caps={key:key not in unsupported for key in seed};model.capabilitiesChanged.emit()
done('Disconnected screens/legacy modes stay untouched; actual module/embed/tray capability dependencies disable only their affected controls')

before=len(model.requests);ctx.general.patch(theme=1,accentIndex=4);QTest.qWait(25)
assert page.property('accent').name()=='#18aad3' and item('interfaceskinFile').property('color').name()!='#8f8f8f'
assert len(model.requests)==before and not ctx.general.requests
assert window.grabWindow().save(str(results/'02-interface-shared-light-cyan.png'))
ctx.general.patch(theme=2,accentIndex=0);QTest.qWait(20)
done('Actual shared palette provider inputs update Interface live without a second settings store or Interface writes')

geometry=[]
for width,height,scale in [(1080,897,1),(640,420,1),(960,600,1.5),(1280,720,2)]:
    window.resize(width,height);dialog.setProperty('uiScale',scale);QTest.qWait(25)
    for key in ['skinFile','resizeVideo','fullscreenScreen','controlAutoHide','controlHideDelay','trackPopup','popupDuration']:
        obj=focus('interface'+key);assert obj.hasActiveFocus(),key
        point=obj.mapToScene(QPointF(0,0));viewport=item('globalSettingsPageScroll');top=viewport.mapToScene(QPointF(0,0)).y()
        assert point.x()>=-.5 and point.x()+obj.width()<=width+.5 and point.y()>=top-.5 and point.y()+obj.height()<=top+viewport.height()+.5,(key,point)
    for key in ['controlHideDelay','popupDuration']:
        for index in range(21):
            obj=item('interface'+key+'OriginalTick'+str(index));assert [obj.width(),obj.height()]==([32,32]if index in[0,20]else[2,8])and obj.scale()==scale
    geometry.append(dict(width=width,height=height,scale=scale))
done('Real enabled controls reveal within actual host at natural/narrow/150/200%; all original range root dimensions and clipping retained')

window.resize(1080,897);dialog.setProperty('uiScale',1);focus('globalSettingsSearch');text('Hide after');QTest.qWait(25)
assert item('interfacecontrolHideDelay').isVisible()and not item('interfaceskinFile').isVisible()
QTest.keyClick(window,Qt.Key_A,Qt.ControlModifier);QTest.keyClick(window,Qt.Key_Backspace);QTest.qWait(25)
focus('interfacecontrolAutoHide');QTest.keyClick(window,Qt.Key_Space);QTest.qWait(20);assert item('interfacecontrolAutoHidePopup').property('visible')
provider.showGlobalSettingsDialog.emit();QTest.qWait(25);assert not item('interfacecontrolAutoHidePopup').property('visible')and item('globalSettingsSearch').hasActiveFocus()
click('generalAdvanced');QTest.qWait(35);assert not dialog.property('visible')and provider.experts==1 and provider.native
provider.native=False;provider.changed.emit();click('opener');assert dialog.property('selectedCategory')=='interface'
QTest.keyClick(window,Qt.Key_Escape);QTest.qWait(25);assert not dialog.property('visible')and item('opener').hasActiveFocus()
window.setProperty('omitInterface',True);click('opener');assert not evaluate(dialog,'chooseCategory("interface")')and dialog.property('selectedCategory')=='general'
done('Actual search/repeated-popup-focus/intentional native-expert handoff/opener restore work; missing service disables category rather than General fallback')

destroyed=[];engine.destroyed.connect(lambda:destroyed.append(True));window.close();engine.deleteLater();QCoreApplication.sendPostedEvents(None,QEvent.DeferredDelete);QTest.qWait(25)
assert destroyed==[True]and not messages,messages
report=dict(passed=True,cases=checks,caseCount=len(checks),messages=messages,geometry=geometry,nativeAcceptance=False,scope=__doc__,
    renderedImageInstances=images,referenceSha256=hashlib.sha256(reference.read_bytes()).hexdigest(),
    sourceSha256={p.relative_to(ROOT).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in (ROOT/'modules/gui/qt/dialogs/dialogs/qml').glob('*.qml')},
    backdropSoftwareFallback=backdrop_state,historicalNegative=historical_result,
    workerSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),adapters=identities,
    nativeBackendAccepted=False)
(results/'verification.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8',newline='\n')
print('PASS native-V2 Interface supplied-contract component only.',flush=True)
