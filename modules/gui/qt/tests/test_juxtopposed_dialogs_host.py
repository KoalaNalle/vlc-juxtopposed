# SPDX-License-Identifier: GPL-2.0-or-later
"""Exercise the actual production host with real dialogs and strict supplied state.

Controllers are explicit supplied-state contracts extracted from the project's
existing component tests. This does not compile or replace native VLC methods.
"""
from pathlib import Path
import argparse, ast, hashlib, json, os, shutil, xml.etree.ElementTree as ET
from enum import IntEnum
from juxtopposed_backdrop_fixture import software_backdrop_state

HERE=Path(__file__).resolve().parent
REPO=HERE.parents[3]
FILES=REPO
os.environ.update(QT_QPA_PLATFORM='offscreen',QT_QUICK_BACKEND='software',QML_DISABLE_DISK_CACHE='1',
                  QT_QPA_FONTDIR=str(REPO/'modules/gui/qt/pixmaps/juxtopposed'))
from PySide6.QtCore import QObject, Property, Signal, Slot, QEnum, Qt, QUrl, QPoint, QEvent, QCoreApplication, qInstallMessageHandler
from PySide6.QtGui import QGuiApplication
from PySide6.QtQml import QQmlApplicationEngine, QQmlExpression, qmlContext, qmlRegisterType, qmlRegisterSingletonType, qmlRegisterSingletonInstance, qmlRegisterUncreatableType
from PySide6.QtQuick import QQuickItem
from PySide6.QtTest import QTest

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output-dir',type=Path,required=True)
parser.add_argument('--repo-root',type=Path,default=REPO)
args=parser.parse_args()
REPO=args.repo_root.resolve(); FILES=REPO
os.environ['QT_QPA_FONTDIR']=str(REPO/'modules/gui/qt/pixmaps/juxtopposed')
args.output_dir.mkdir(parents=True,exist_ok=False)
messages=[]
qInstallMessageHandler(lambda kind,context,message: messages.append(message))
identities={}
def adapter(relative,name,new_name):
    source=REPO/relative
    identities[relative]=hashlib.sha256(source.read_bytes()).hexdigest()
    tree=ast.parse(source.read_text(encoding='utf-8'))
    node=next(n for n in tree.body if isinstance(n,ast.ClassDef) and n.name==name)
    node.name=new_name
    exec(compile(ast.Module(body=[node],type_ignores=[]),str(source),'exec'),globals())
    return globals()[new_name]

from juxtopposed_general_preferences_fixture import GeneralPreferences as General
Network=adapter('modules/gui/qt/tests/test_juxtopposed_network_dialog.py','Controller','NetworkAdapter')
Playlist=adapter('modules/gui/qt/tests/test_juxtopposed_new_playlist.py','Controller','PlaylistAdapter')
Conversion=adapter('modules/gui/qt/tests/test_juxtopposed_conversion_dialog.py','Controller','ConversionAdapter')

class SetupGeneral(General):
    setupStateChanged=Signal()
    def __init__(self):
        super().__init__(); self.required=True; self.ready=False; self.finishes=[]
    setupRequired=Property(bool,lambda s:s.required,notify=setupStateChanged)
    setupReady=Property(bool,lambda s:s.ready,notify=setupStateChanged)
    @Slot('QVariant','QVariant',result=bool)
    def finishSetup(self,theme,accent):
        self.finishes.append([theme,accent])
        if not self.setValue('theme',theme):return False
        if not self.setValue('accentIndex',accent):return False
        self.required=self.ready=False; self.setupStateChanged.emit(); return True

class ProfileLayout(QObject):
    changed=Signal()
    draftChanged=Signal()
    editing=Property(bool,lambda s:s._editing,notify=changed)
    lastError=Property(str,lambda s:'',constant=True)
    palette=Property('QVariantList',lambda s:[],constant=True)
    current=Property('QVariantMap',lambda s:s.data,constant=True)
    draft=Property('QVariantMap',lambda s:s.data,notify=draftChanged)
    def __init__(self,parent):
        super().__init__(parent);self._editing=False
        self.data={'video':{'columns':[[0],[],[]],'timelineAbove':False},'audio':{'columns':[[0],[],[]],'timelineAbove':True}}
    @Slot()
    def cancelEdit(self):self._editing=False;self.changed.emit()
    @Slot(int,result='QVariantMap')
    def tileInfo(self,id):return dict(id=id,name='Owned play',kind='play',glyph='P',fontFamily='Inter')

class Profiles(QObject):
    changed=Signal()
    available=Property(bool,lambda s:True,constant=True)
    busy=Property(bool,lambda s:s._busy,notify=changed)
    needsRecovery=Property(bool,lambda s:False,constant=True)
    selectedName=Property(str,lambda s:'Movies',constant=True)
    lastError=Property(str,lambda s:'',constant=True)
    layout=Property(QObject,lambda s:s._layout,constant=True)
    def __init__(self):super().__init__();self._busy=False;self._layout=ProfileLayout(self)
    @Slot(result=bool)
    def beginLayoutEdit(self):
        if self._busy:return False
        self._layout._editing=True;self._layout.changed.emit();return True
    @Slot(result=bool)
    def saveLayoutDraft(self):
        if self._busy:return False
        self._layout.cancelEdit();return True
    @Slot(str,result=bool)
    def createProfile(self,name):return bool(name.strip())
    @Slot(result=bool)
    def retrySave(self):return not self._busy
    @Slot(result=bool)
    def retrySwitch(self):return not self._busy
    @Slot(result=bool)
    def restorePreviousProfile(self):return not self._busy

class MainContext(QObject):
    juxtopposedControlsChanged=Signal()
    juxtopposedControls=Property(bool,lambda s:s.redesigned,notify=juxtopposedControlsChanged)
    generalPreferences=Property(QObject,lambda s:s.general,constant=True)
    useGlobalShortcuts=Property(bool,lambda s:True,constant=True)
    playerProfiles=Property(QObject,lambda s:s.profiles,constant=True)
    def __init__(self):
        super().__init__(); self.general=SetupGeneral(); self.profiles=Profiles(); self.redesigned=False

class Provider(QObject):
    changed=Signal()
    firstRunDialogVisibleChanged=Signal()
    firstRunDialogVisible=Property(bool,lambda s:s.wizard,notify=firstRunDialogVisibleChanged)
    showGlobalSettingsDialog=Signal()
    showNetworkStreamDialog=Signal()
    showNewPlaylistDialog=Signal()
    showConversionDialog=Signal()
    showPlayerProfilesDialog=Signal(bool)
    showToolbarEditorDialog=Signal()
    newPlaylistSaved=Signal('QVariant')
    def __init__(self):
        super().__init__()
        self.network,self.playlist,self.convert=Network(),Playlist(),Conversion()
        self.visible,self.editing,self.native=False,False,False
        self.wizard=False
        self.experts=0; self.calls=[]; self.visibility_changes=[]
    networkStream=Property(QObject,lambda s:s.network,constant=True)
    newPlaylist=Property(QObject,lambda s:s.playlist,constant=True)
    conversion=Property(QObject,lambda s:s.convert,constant=True)
    def set_visible(self,v):
        if self.visible==v:return
        self.visible=v; self.visibility_changes.append(v); self.changed.emit()
    def set_editing(self,v):
        if self.editing==v:return
        self.editing=v; self.changed.emit()
    qmlDialogVisible=Property(bool,lambda s:s.visible,set_visible,notify=changed)
    qmlInputFocused=Property(bool,lambda s:s.editing,set_editing,notify=changed)
    prefsDialogVisible=Property(bool,lambda s:s.native,notify=changed)
    @Slot()
    def expertPrefsDialog(self):
        # Assert the host fulfilled the real native entry precondition; do not
        # implement routing inside an adapter and mistakenly test that policy.
        assert not self.visible
        self.experts+=1; self.native=True; self.changed.emit()
    @Slot()
    def chooseNewPlaylistFiles(self): self.calls.append('playlistFiles')
    @Slot()
    def chooseNewPlaylistThumbnail(self): self.calls.append('playlistThumbnail')
    @Slot()
    def chooseConversionSource(self): self.calls.append('conversionSource')
    @Slot()
    def chooseConversionDestination(self): self.calls.append('conversionDestination')
    @Slot()
    def openLegacyTranscodingDialog(self): self.calls.append('allProfiles')

app=QGuiApplication([])
provider,ctx=Provider(),MainContext()
qmlRegisterSingletonInstance(MainContext,'VLC.MainInterface',1,0,'MainCtx',ctx)
qmlRegisterSingletonInstance(Provider,'VLC.Dialogs',1,0,'DialogsProvider',provider)
qmlRegisterUncreatableType(Conversion,'VLC.Dialogs',1,0,'ConversionController','Supplied-state enum')
qmlRegisterType(QUrl.fromLocalFile(str(REPO/'modules/gui/qt/widgets/qml/TextInputFocus.qml')),
                'VLC.Widgets',1,0,'TextInputFocus')
qmlRegisterSingletonType(QUrl.fromLocalFile(str(FILES/'modules/gui/qt/style/JuxtopposedPalette.qml')),
                         'VLC.Style',1,0,'JuxtopposedPalette')
qmlRegisterType(QUrl.fromLocalFile(str(FILES/'modules/gui/qt/player/qml/JuxtopposedPlaybackBar.qml')),
               'VLC.Player',1,0,'JuxtopposedPlaybackBar')
asset_dir=args.output_dir/'assets'; asset_dir.mkdir()
for path in (REPO/'modules/gui/qt/pixmaps/juxtopposed').iterdir():
    if path.is_file(): shutil.copyfile(path,asset_dir/path.name)
for path in (FILES/'modules/gui/qt/pixmaps/juxtopposed').iterdir():
    if path.is_file(): shutil.copyfile(path,asset_dir/path.name)
for entry in ET.parse(FILES/'modules/gui/qt/assets.qrc').findall(".//qresource[@prefix='/juxtopposed']/file"):
    destination=asset_dir/entry.attrib.get('alias',Path(entry.text).name)
    destination.parent.mkdir(parents=True,exist_ok=True)
    shutil.copyfile(FILES/'modules/gui/qt'/entry.text,destination)
# Supplied SVG dependency; native recoloring remains an actual-app gate.
util=args.output_dir/'imports/VLC/Util'; util.mkdir(parents=True)
(util/'qmldir').write_text('module VLC.Util\nsingleton SVGColorImage 1.0 SVGColorImage.qml\n')
(util/'SVGColorImage.qml').write_text('pragma Singleton\nimport QtQml\nQtObject { function colorize(path) { return { any: function(colors) { return this }, uri: function() { return path } } } }\n')
qml_dir=QUrl.fromLocalFile(str(FILES/'modules/gui/qt/dialogs/dialogs/qml')).toString()
menus=QUrl.fromLocalFile(str(REPO/'modules/gui/qt/menus/qml')).toString()
assets=QUrl.fromLocalFile(str(asset_dir.resolve())+'/').toString()
driver=f'''import QtQuick
import QtQuick.Controls.Basic as C
import VLC.MainInterface
import VLC.Dialogs
import VLC.Style
import "{qml_dir}" as Dialogs
import "{menus}" as Menus
C.ApplicationWindow {{
    id: window
    visible:true; width:1300; height:1000; color:"#181818"
    property int transports:0
    property int savedNotifications:0
    property var savedSelection:null
    property Item ephemeral:null
    Component {{ id: ephemeralButton; C.Button {{ objectName:"ephemeralOpener"; text:"Temporary"; x:700; y:10 }} }}
    Rectangle {{ id: backdropUnderlay; objectName:"settingsTestUnderlay"; anchors.fill:parent; color:window.color }}
    C.Button {{ id: opener; objectName:"opener"; x:10; y:10; text:"Settings"; onClicked:DialogsProvider.showGlobalSettingsDialog() }}
    C.Button {{ id: fallback; objectName:"fallback"; x:160; y:10; text:"View" }}
    C.TextField {{ objectName:"outsideText"; x:300; y:10; width:160 }}
    C.TextField {{ objectName:"readonlyText"; x:480; y:10; width:160; readOnly:true; text:"readonly" }}
    Menus.ShortcutExt {{ objectName:"transportShortcut"; sequence:"Space"; onActivated:window.transports++ }}
    Binding {{ target:JuxtopposedPalette; property:"preferences"; value:MainCtx.generalPreferences }}
    Dialogs.JuxtopposedDialogsHost {{
        id:host; anchors.fill:parent; mainCtx:MainCtx; dialogsProvider:DialogsProvider
        backdropSource:backdropUnderlay
        appearance:JuxtopposedPalette; assetBase:"{assets}"; fallbackFocusItem:fallback
        onPlaylistSaved:window.savedNotifications++
        onPlaylistRequested:playlist => window.savedSelection=playlist
    }}
}}'''
(args.output_dir/'driver.qml').write_text(driver,encoding='utf-8')
engine=QQmlApplicationEngine()
engine.addImportPath(str((args.output_dir/'imports').resolve()))
engine.loadData(driver.encode(),QUrl.fromLocalFile(str(args.output_dir.resolve()/'driver.qml')))
assert engine.rootObjects(),messages
window=engine.rootObjects()[0]
QTest.qWait(150)
checks=[]
def check(name,condition):
    assert condition,(name,messages)
    checks.append(name)
def item(name):
    obj=window.findChild(QObject,name)
    if obj:return obj
    pending=[window.contentItem()]
    while pending:
        obj=pending.pop()
        if obj.objectName()==name:return obj
        pending.extend(obj.childItems())
    raise AssertionError(name)
def expr(obj,code):
    evaluation=QQmlExpression(qmlContext(obj),obj,code)
    value,_=evaluation.evaluate()
    assert not evaluation.hasError(),evaluation.error().toString()
    return value.toVariant() if hasattr(value,'toVariant') else value
def click(name):
    control=item(name)
    assert control.isVisible() and control.isEnabled(),name
    QTest.mouseClick(window,Qt.LeftButton,Qt.NoModifier,control.mapToScene(control.boundingRect().center()).toPoint())
    QTest.qWait(35)
def focus(name): item(name).forceActiveFocus(); QTest.qWait(20)
names=['juxtopposedGlobalSettings','juxtopposedNetworkDialog','juxtopposedNewPlaylistDialog','juxtopposedConversionDialog','juxtopposedPlayerProfilesDialog','juxtopposedPlayerCustomizer']
signals=[provider.showGlobalSettingsDialog.emit,provider.showNetworkStreamDialog.emit,provider.showNewPlaylistDialog.emit,provider.showConversionDialog.emit,lambda:provider.showPlayerProfilesDialog.emit(False),provider.showToolbarEditorDialog.emit]
dialogs=[item(name) for name in names]
def visible(): return [bool(d.property('visible')) for d in dialogs]
def route(index): signals[index]();QTest.qWait(35)
def close_all():
    for d in dialogs:
        expr(d,'close()')
    QTest.qWait(30)

check('initial host: no modal, no native text input, one genuine General service',
      visible()==[False]*len(names) and not provider.visible and not provider.editing
      and dialogs[0].property('preferences') is ctx.general)
focus('opener');click('opener')
backdrop_state=software_backdrop_state(window,dialogs[0],item('settingsTestUnderlay'))
check('real opener activates the shared Global route; Search sets actual TextInputFocus',
      visible()==[True,False,False,False,False,False] and provider.visible and provider.editing
      and window.activeFocusItem().objectName()=='globalSettingsSearch')
before=ctx.general.reloads;route(0)
check('repeat Settings request focuses existing Search and does not reload/reset the open service',ctx.general.reloads==before)
check('pending Interface category remains disabled rather than loading General or a fake service',
      not expr(dialogs[0],'chooseCategory("interface")') and dialogs[0].property('selectedCategory')=='general')

close_all();provider.showToolbarEditorDialog.emit();QTest.qWait(20)
check('traditional toolbar request does not open the redesigned customizer',not dialogs[5].property('visible'))
ctx.redesigned=True;ctx.juxtopposedControlsChanged.emit()
transitions=0
for source in range(len(names)):
    close_all();focus('opener');route(source)
    for target in range(len(names)):
        if target==source:continue
        route(target)
        assert visible()==[i==target for i in range(len(names))],(source,target,visible())
        assert provider.visible
        route(source)
        transitions+=1
check('all 30 directed modal transitions keep exactly one visible dialog and the shared guard',transitions==30)

busy_cases=0
for current,model in [(1,provider.network),(2,provider.playlist),(3,provider.convert),(4,ctx.profiles),(5,ctx.profiles)]:
    close_all();route(current)
    if current==3:model.phase(model.State.Encoding,.5)
    else:
        model._busy=True
        (model.stateChanged if current==1 else model.changed).emit()
    baseline=(provider.network.resets,provider.playlist.resets,provider.convert.resets,provider.convert.cancels)
    focused=window.activeFocusItem()
    for target in range(len(names)):
        if target==current:continue
        route(target)
        assert visible()==[i==current for i in range(len(names))]
        assert baseline==(provider.network.resets,provider.playlist.resets,provider.convert.resets,provider.convert.cancels)
        assert window.activeFocusItem() is focused
        busy_cases+=1
    if current==3:model.phase(model.State.Idle)
    else:
        model._busy=False
        (model.stateChanged if current==1 else model.changed).emit()
check('all 25 busy cross-modal requests preserve the current operation without cancel/reset',busy_cases==25)

close_all();focus('opener');route(0)
before=window.property('transports');QTest.keyClick(window,Qt.Key_Space);QTest.qWait(20)
check('actual ShortcutExt transport suppressed while Global Search owns the window focus',window.property('transports')==before and provider.editing)
focus('globalSettingsClose')
check('nontext focus inside Global retains modal suppression without a false editing flag',not provider.editing and provider.visible)
QTest.keyClick(window,Qt.Key_Escape);QTest.qWait(40)
check('Escape closes Global, clears one shared modal guard, and restores the original opener',
      not provider.visible and not provider.editing and window.activeFocusItem().objectName()=='opener')
focus('outsideText');QTest.keyClick(window,Qt.Key_Space);QTest.qWait(20)
check('outside editable text still uses the real window focus guard after the modal closes',provider.editing and window.property('transports')==before)
focus('readonlyText');QTest.keyClick(window,Qt.Key_Space);QTest.qWait(20)
check('read-only text is not treated as an editor by the original helper',not provider.editing and window.property('transports')==before+1)
focus('fallback');QTest.keyClick(window,Qt.Key_Space);QTest.qWait(20)
check('transport resumes through the unchanged ShortcutExt when modal and editing guards clear',window.property('transports')==before+2)

close_all();route(0)
log=item('generalLogFile')
expr(log,'forceActiveFocus()');QTest.qWait(20)
check('actual Log-file TextInput also updates the shared native input-focus flag',provider.editing and log.hasActiveFocus())
focus('globalSettingsClose');QTest.keyClick(window,Qt.Key_Escape);QTest.qWait(30)
focus('opener');route(0);item('opener').setVisible(False)
QTest.keyClick(window,Qt.Key_Escape);QTest.qWait(30)
check('hidden opener returns focus to the current view fallback',window.activeFocusItem().objectName()=='fallback')
item('opener').setVisible(True)
expr(window,'ephemeral=ephemeralButton.createObject(window.contentItem);ephemeral.forceActiveFocus()')
QTest.qWait(20);route(0)
expr(window,'ephemeral.destroy();ephemeral=null');QTest.qWait(20)
QTest.keyClick(window,Qt.Key_Escape);QTest.qWait(30)
check('destroyed opener also returns focus safely to the current view fallback',window.activeFocusItem().objectName()=='fallback')

focus('opener');route(0)
advanced=item('generalAdvanced')
expr(advanced,'forceActiveFocus()');QTest.qWait(70)
QTest.keyClick(window,Qt.Key_Space);QTest.qWait(70)
check('real Advanced button closes Global then invokes the explicit native entry once',provider.experts==1 and provider.native and not provider.visible and visible()==[False]*len(names))
for index in range(len(names)):route(index)
check('all QML entries refuse to overlay the open native Preferences dialog',visible()==[False]*len(names))
provider.native=False;provider.changed.emit();route(0)
check('normal Settings can reopen after native Preferences closes and reloads current values',visible()==[True,False,False,False,False,False])

close_all();route(3)
click('conversionSourceBrowse');click('conversionDestinationBrowse');click('conversionAllProfiles')
close_all();route(2)
click('newPlaylistSelectFiles');click('newPlaylistThumbnail')
check('existing file/profile picker routes still call their actual provider API names',
      provider.calls==['conversionSource','conversionDestination','allProfiles','playlistFiles','playlistThumbnail'])
provider.playlist.complete(True);QTest.qWait(25)
selection=window.property('savedSelection')
if hasattr(selection,'toVariant'):selection=selection.toVariant()
check('actual playlist completion forwards its selected playlist and closes its modal',selection=={'id':73,'type':'playlist'} and not provider.visible)
provider.newPlaylistSaved.emit({'id':73});QTest.qWait(20)
check('saved-playlist refresh notification survives host extraction',window.property('savedNotifications')==1)

close_all();route(0)
ctx.general.patch(accentIndex=6);QTest.qWait(30)
check('same General provider drives a live shared accent on the actual mounted shell',expr(dialogs[0],'String(accent)')=='#8e42ff')
window.grabWindow().save(str(args.output_dir/'global-mounted.png'))
# Pointer events target the actual production header rather than an adapter.
# Run Network first so the original immovable dialog is a direct regression.
movement=[]
def drag_header(dialog, delta=None, end=None):
    header=dialog.property('header')
    start=header.mapToScene(header.boundingRect().center()).toPoint()
    if end is not None:delta=end-start
    QTest.mouseMove(window,start);QTest.mousePress(window,Qt.LeftButton,Qt.NoModifier,start)
    for step in range(1,6):
        point=start+QPoint(round(delta.x()*step/5),round(delta.y()*step/5))
        QTest.mouseMove(window,point,10)
    QTest.mouseRelease(window,Qt.LeftButton,Qt.NoModifier,start+delta)
    QTest.qWait(25)

def position(dialog):return [dialog.property('x'),dialog.property('y')]

for scale in (1,1.5,2):
    window.resize(1300,1000)
    for index in (1,0,2,3):
        close_all();dialogs[index].setProperty('uiScale',scale)
        focus('opener');route(index);dialog=dialogs[index]
        origin=position(dialog);focused=window.activeFocusItem()
        dx=min(60,round((window.width()-dialog.property('width'))/3))
        dy=min(40,round((window.height()-dialog.property('height'))/3))
        drag_header(dialog,QPoint(dx,dy))
        after=position(dialog)
        movement.append(dict(dialog=names[index],scale=scale,before=origin,after=after,requestedDelta=[dx,dy]))
        (args.output_dir/'movement-observation.json').write_text(json.dumps(movement,indent=2)+'\n',encoding='utf-8')
        check(f'{names[index]} header drag moves both axes at scale {scale}',
              abs(after[0]-origin[0]-dx)<1.5 and abs(after[1]-origin[1]-dy)<1.5)
        check(f'{names[index]} drag preserves focus and modal guard at scale {scale}',
              window.activeFocusItem() is focused and provider.visible and visible()==[i==index for i in range(len(names))])
        # Both far corners must keep the complete dialog inside its owner.
        drag_header(dialog,end=QPoint(1,1));low=position(dialog)
        drag_header(dialog,end=QPoint(window.width()-1,window.height()-1));high=position(dialog)
        check(f'{names[index]} header stays reachable at both bounds at scale {scale}',
              low[0]>=0 and low[1]>=0 and abs(low[0])<1.5 and abs(low[1])<1.5
              and high[0]+dialog.property('width')<=window.width()+1.5
              and high[1]+dialog.property('height')<=window.height()+1.5)
        window.resize(640,420);QTest.qWait(40)
        check(f'{names[index]} parent shrink keeps header and close reachable at scale {scale}',
              dialog.property('x')>=0 and dialog.property('y')>=0
              and dialog.property('x')+dialog.property('width')<=640+1.5
              and dialog.property('y')+dialog.property('height')<=420+1.5)
        # The actual close button still receives a click, followed by opener focus.
        close_names=['globalSettingsClose','networkClose','newPlaylistClose','conversionClose']
        click(close_names[index]);QTest.qWait(30)
        check(f'{names[index]} close remains clickable and restores focus at scale {scale}',
              not provider.visible and window.activeFocusItem().objectName()=='opener')
        window.resize(1300,1000);route(index);QTest.qWait(25)
        check(f'{names[index]} reopen centers in owner at scale {scale}',
              abs(dialog.property('x')-(window.width()-dialog.property('width'))/2)<1.5
              and abs(dialog.property('y')-(window.height()-dialog.property('height'))/2)<1.5)
close_all()

host=item('juxtopposedDialogsHost')
window.resize(1300,1000)
check('completed old modal journeys have not instantiated setup',not host.property('setupVisible'))
ctx.redesigned=False;ctx.juxtopposedControlsChanged.emit();QTest.qWait(20)
ctx.general.ready=True; ctx.general.setupStateChanged.emit(); QTest.qWait(40)
check('traditional mode leaves setup inactive',not host.property('setupVisible'))
provider.wizard=True;provider.firstRunDialogVisibleChanged.emit()
ctx.redesigned=True;ctx.juxtopposedControlsChanged.emit();QTest.qWait(50)
check('setup waits for actual native privacy window to close',not host.property('setupVisible') and not provider.visible)
provider.wizard=False;provider.firstRunDialogVisibleChanged.emit();QTest.qWait(100)
setup=item('juxtopposedSetupDialog')
check('native privacy completion opens setup through sole modal owner',host.property('setupVisible') and provider.visible and visible()==[False]*len(names))
for index in range(len(names)):
    route(index)
    check(f'route {index} cannot replace active setup',host.property('setupVisible') and visible()==[False]*len(names) and provider.visible)
before=window.property('transports')
focus('setupTheme2');QTest.keyClick(window,Qt.Key_Space);QTest.qWait(30)
check('setup keeps existing global transport hotkey suppressed',window.property('transports')==before)
QTest.keyClick(window,Qt.Key_Escape);QTest.qWait(80)
check('setup Escape neither completes nor immediately reopens',not provider.visible and not host.property('setupVisible') and ctx.general.required and not ctx.general.finishes)
ctx.general.setupStateChanged.emit();QTest.qWait(80)
check('postponed setup stays closed on repeated readiness notification',not host.property('setupVisible'))
focus('opener');host.setProperty('setupPostponed',False);expr(host,'tryOpenSetup()');QTest.qWait(80)
setup=item('juxtopposedSetupDialog')
click('setupTheme1');click('setupNext')
ctx.general.fail_next='Owned host fixture cannot save yet.'
click('setupFinish')
check('setup save failure retains exclusive modal ownership',provider.visible and host.property('setupVisible') and ctx.general.required and len(ctx.general.finishes)==1)
route(1)
check('save failure cannot be replaced by network dialog',not dialogs[1].property('visible') and host.property('setupVisible'))
click('setupFinish');QTest.qWait(80)
check('successful setup releases sole modal guard and restores opener',not provider.visible and not host.property('setupVisible') and not ctx.general.required and window.activeFocusItem().objectName()=='opener')
ctx.general.setupStateChanged.emit();QTest.qWait(60)
check('completed setup never reopens on readiness notification',not host.property('setupVisible'))
route(1)
check('ordinary modal routing works after setup',visible()==[False,True,False,False,False,False] and provider.visible)
close_all()

check('never-cleared QML message ledger is empty before teardown',not messages)
destroyed=[];engine.destroyed.connect(lambda:destroyed.append(True));engine.deleteLater()
QCoreApplication.sendPostedEvents(None,QEvent.DeferredDelete);QTest.qWait(40)
check('actual engine destruction completes without new binding/provider errors',destroyed==[True] and not messages)
sources=[FILES/path for path in [
    'modules/gui/qt/dialogs/dialogs/qml/JuxtopposedDialogsHost.qml',
    'modules/gui/qt/dialogs/dialogs/qml/JuxtopposedSetupDialog.qml',
    'modules/gui/qt/dialogs/dialogs/qml/JuxtopposedSetupPreview.qml',
    'modules/gui/qt/dialogs/dialogs/qml/JuxtopposedPlayerProfilesDialog.qml',
    'modules/gui/qt/dialogs/dialogs/qml/JuxtopposedMovableDialog.qml',
    'modules/gui/qt/dialogs/dialogs/qml/JuxtopposedDialogDragArea.qml',
    'modules/gui/qt/dialogs/dialogs/qml/JuxtopposedGlobalSettings.qml',
    'modules/gui/qt/dialogs/dialogs/qml/JuxtopposedGeneralSettings.qml',
    'modules/gui/qt/dialogs/dialogs/qml/JuxtopposedSettingsBackdrop.qml',
    'modules/gui/qt/dialogs/dialogs/qml/JuxtopposedSettingsBackdropEffect.qml',
    'modules/gui/qt/dialogs/dialogs/qml/JuxtopposedNetworkDialog.qml',
    'modules/gui/qt/dialogs/dialogs/qml/JuxtopposedNewPlaylistDialog.qml',
    'modules/gui/qt/dialogs/dialogs/qml/JuxtopposedConversionDialog.qml',
    'modules/gui/qt/dialogs/dialogs/qml/JuxtopposedPlayerCustomizer.qml',
    'modules/gui/qt/player/qml/JuxtopposedPlaybackBar.qml',
    'modules/gui/qt/player/qml/JuxtopposedVolumeControl.qml',
    'modules/gui/qt/player/qml/JuxtopposedLayoutRow.qml',
    'modules/gui/qt/style/JuxtopposedPalette.qml', 'modules/gui/qt/style/VLCStyle.qml',
    'modules/gui/qt/widgets/qml/TextInputFocus.qml', 'modules/gui/qt/menus/qml/ShortcutExt.qml',
    'modules/gui/qt/maininterface/mainctx.cpp', 'modules/gui/qt/maininterface/mainctx.hpp',
    'modules/gui/qt/dialogs/dialogs_provider.cpp']]
for path in sources:identities[str(path.relative_to(REPO))]=hashlib.sha256(path.read_bytes()).hexdigest()
report={'checks':checks,'checkCount':len(checks),'backdropSoftwareFallback':backdrop_state,'directedTransitions':transitions,'busyGuards':busy_cases,'popupMovement':movement,'qtMessages':messages,
        'sourceSha256':identities,'scope':'Actual integrated production QML host, General shell/body/palette and five other real dialogs; supplied strict controller contracts. Native C++/OS hotkey/expert/save/reset build execution remains Root gate.'}
(args.output_dir/'verification.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print(json.dumps({'checks':len(checks),'directedTransitions':transitions,'busyGuards':busy_cases,'qtMessages':messages}))
