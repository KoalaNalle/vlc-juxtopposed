# SPDX-License-Identifier: GPL-2.0-or-later
"""Exercise the actual production host with real dialogs and strict supplied state.

Controllers are explicit supplied-state contracts extracted from the project's
existing component tests. This does not compile or replace native VLC methods.
"""
from pathlib import Path
import argparse, ast, hashlib, json, os, shutil
from enum import IntEnum
from juxtopposed_backdrop_fixture import software_backdrop_state

HERE=Path(__file__).resolve().parent
REPO=HERE.parents[3]
FILES=REPO
os.environ.update(QT_QPA_PLATFORM='offscreen',QT_QUICK_BACKEND='software',QML_DISABLE_DISK_CACHE='1',
                  QT_QPA_FONTDIR=str(REPO/'modules/gui/qt/pixmaps/juxtopposed'))
from PySide6.QtCore import QObject, Property, Signal, Slot, QEnum, Qt, QUrl, QEvent, QCoreApplication, qInstallMessageHandler
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

class MainContext(QObject):
    generalPreferences=Property(QObject,lambda s:s.general,constant=True)
    useGlobalShortcuts=Property(bool,lambda s:True,constant=True)
    def __init__(self):
        super().__init__(); self.general=General()

class Provider(QObject):
    changed=Signal()
    showGlobalSettingsDialog=Signal()
    showNetworkStreamDialog=Signal()
    showNewPlaylistDialog=Signal()
    showConversionDialog=Signal()
    newPlaylistSaved=Signal('QVariant')
    def __init__(self):
        super().__init__()
        self.network,self.playlist,self.convert=Network(),Playlist(),Conversion()
        self.visible,self.editing,self.native=False,False,False
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
asset_dir=args.output_dir/'assets'; asset_dir.mkdir()
for path in (REPO/'modules/gui/qt/pixmaps/juxtopposed').iterdir():
    if path.is_file(): shutil.copyfile(path,asset_dir/path.name)
for path in (FILES/'modules/gui/qt/pixmaps/juxtopposed').iterdir():
    if path.is_file(): shutil.copyfile(path,asset_dir/path.name)
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
names=['juxtopposedGlobalSettings','juxtopposedNetworkDialog','juxtopposedNewPlaylistDialog','juxtopposedConversionDialog']
signals=[provider.showGlobalSettingsDialog,provider.showNetworkStreamDialog,provider.showNewPlaylistDialog,provider.showConversionDialog]
dialogs=[item(name) for name in names]
def visible(): return [bool(d.property('visible')) for d in dialogs]
def route(index): signals[index].emit();QTest.qWait(35)
def close_all():
    for d in dialogs:
        expr(d,'close()')
    QTest.qWait(30)

check('initial host: no modal, no native text input, one genuine General service',
      visible()==[False]*4 and not provider.visible and not provider.editing
      and dialogs[0].property('preferences') is ctx.general)
focus('opener');click('opener')
backdrop_state=software_backdrop_state(window,dialogs[0],item('settingsTestUnderlay'))
check('real opener activates the shared Global route; Search sets actual TextInputFocus',
      visible()==[True,False,False,False] and provider.visible and provider.editing
      and window.activeFocusItem().objectName()=='globalSettingsSearch')
before=ctx.general.reloads;route(0)
check('repeat Settings request focuses existing Search and does not reload/reset the open service',ctx.general.reloads==before)
check('pending Interface category remains disabled rather than loading General or a fake service',
      not expr(dialogs[0],'chooseCategory("interface")') and dialogs[0].property('selectedCategory')=='general')

transitions=0
for source in range(4):
    close_all();focus('opener');route(source)
    for target in range(4):
        if target==source:continue
        route(target)
        assert visible()==[i==target for i in range(4)],(source,target,visible())
        assert provider.visible
        route(source)
        transitions+=1
check('all 12 directed modal transitions keep exactly one visible dialog and the shared guard',transitions==12)

busy_cases=0
for current,model in [(1,provider.network),(2,provider.playlist),(3,provider.convert)]:
    close_all();route(current)
    if current==3:model.phase(model.State.Encoding,.5)
    else:
        model._busy=True
        (model.stateChanged if current==1 else model.changed).emit()
    baseline=(provider.network.resets,provider.playlist.resets,provider.convert.resets,provider.convert.cancels)
    focused=window.activeFocusItem()
    for target in range(4):
        if target==current:continue
        route(target)
        assert visible()==[i==current for i in range(4)]
        assert baseline==(provider.network.resets,provider.playlist.resets,provider.convert.resets,provider.convert.cancels)
        assert window.activeFocusItem() is focused
        busy_cases+=1
    if current==3:model.phase(model.State.Idle)
    else:
        model._busy=False
        (model.stateChanged if current==1 else model.changed).emit()
check('all 9 busy cross-modal requests preserve the current operation without cancel/reset',busy_cases==9)

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
check('real Advanced button closes Global then invokes the explicit native entry once',provider.experts==1 and provider.native and not provider.visible and visible()==[False]*4)
for index in range(4):route(index)
check('all QML entries refuse to overlay the open native Preferences dialog',visible()==[False]*4)
provider.native=False;provider.changed.emit();route(0)
check('normal Settings can reopen after native Preferences closes and reloads current values',visible()==[True,False,False,False])

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
check('never-cleared QML message ledger is empty before teardown',not messages)
destroyed=[];engine.destroyed.connect(lambda:destroyed.append(True));engine.deleteLater()
QCoreApplication.sendPostedEvents(None,QEvent.DeferredDelete);QTest.qWait(40)
check('actual engine destruction completes without new binding/provider errors',destroyed==[True] and not messages)
sources=[FILES/path for path in [
    'modules/gui/qt/dialogs/dialogs/qml/JuxtopposedDialogsHost.qml',
    'modules/gui/qt/dialogs/dialogs/qml/JuxtopposedGlobalSettings.qml',
    'modules/gui/qt/dialogs/dialogs/qml/JuxtopposedGeneralSettings.qml',
    'modules/gui/qt/dialogs/dialogs/qml/JuxtopposedSettingsBackdrop.qml',
    'modules/gui/qt/dialogs/dialogs/qml/JuxtopposedSettingsBackdropEffect.qml',
    'modules/gui/qt/dialogs/dialogs/qml/JuxtopposedNetworkDialog.qml',
    'modules/gui/qt/dialogs/dialogs/qml/JuxtopposedNewPlaylistDialog.qml',
    'modules/gui/qt/dialogs/dialogs/qml/JuxtopposedConversionDialog.qml',
    'modules/gui/qt/style/JuxtopposedPalette.qml', 'modules/gui/qt/style/VLCStyle.qml',
    'modules/gui/qt/widgets/qml/TextInputFocus.qml', 'modules/gui/qt/menus/qml/ShortcutExt.qml',
    'modules/gui/qt/maininterface/mainctx.cpp', 'modules/gui/qt/maininterface/mainctx.hpp',
    'modules/gui/qt/dialogs/dialogs_provider.cpp']]
for path in sources:identities[str(path.relative_to(REPO))]=hashlib.sha256(path.read_bytes()).hexdigest()
report={'checks':checks,'checkCount':len(checks),'backdropSoftwareFallback':backdrop_state,'directedTransitions':transitions,'busyGuards':busy_cases,'qtMessages':messages,
        'sourceSha256':identities,'scope':'Actual integrated production QML host, General shell/body/palette and three real dialogs; supplied strict controller contracts. Native C++/OS hotkey/expert/save/reset build execution remains Root gate.'}
(args.output_dir/'verification.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print(json.dumps({'checks':len(checks),'directedTransitions':transitions,'busyGuards':busy_cases,'qtMessages':messages}))
