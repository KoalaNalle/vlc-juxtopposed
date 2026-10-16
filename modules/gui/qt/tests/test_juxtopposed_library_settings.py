# SPDX-License-Identifier: GPL-2.0-or-later
"""Actual Library/host QML with an explicit seven-value native-shaped contract.
No native IPC, consumers, roots, history, persistence, first-run or restart acceptance.
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
from juxtopposed_library_preferences_fixture import LibraryPreferences,SEED
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
    def __init__(self):super().__init__();self.general=General();self.library=LibraryPreferences()
    generalPreferences=Property(QObject,lambda s:s.general,constant=True)
    libraryPreferences=Property(QObject,lambda s:s.library,constant=True)
    useGlobalShortcuts=Property(bool,lambda s:True,constant=True)
messages=[];qInstallMessageHandler(lambda kind,context,message:messages.append(message))
app=QGuiApplication([]);ctx=MainContext();model=ctx.library;provider=Provider()
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
fields=json.loads((ROOT/'modules/gui/qt/tests/fixtures/juxtopposed_library_fields.json').read_text(encoding='utf-8'))
assert [f['key']for f in fields]==list(SEED)and len(SEED)==7
QTest.qWait(70);click('opener');dialog=item('juxtopposedGlobalSettings');assert evaluate(dialog,'chooseCategory("library")')
page=item('juxtopposedLibrarySettings');assert page.property('service')is model and dialog.property('preferences')is ctx.general
assert dialog.property('title')=='Playlists, Folders & Files'and model.reloads==1 and not model.requests and not ctx.general.requests
assert not item('librarysingleInstance').isEnabled()and not item('libraryfileManagerSingleInstance').isEnabled()and not item('libraryshowHiddenFiles').isEnabled()and not item('librarymetadataSource').isEnabled()
assert item('libraryfolderRecursion').isEnabled()and item('libraryuseMediaLibrary').isEnabled()
assert item('libraryfolderRecursion').property('currentIndex')==2 and item('librarymetadataSource').property('currentText')=='From file and local cache'
assert item('libraryenqueueInSingleInstance').property('custom')and evaluate(item('libraryenqueueInSingleInstance'),'Accessible.role')!=evaluate(item('librarysingleInstance'),'Accessible.role')
assert window.grabWindow().save(str(results/'01-library-real-native-gates.png'))
done('Separate seven-value Library service, four default-false consumer gates, actual recursion/local-cache choices and legacy Custom queue state without load writes')
select('libraryfolderRecursion',0);assert model.requests[-1]==['folderRecursion','none']and model.restartRequired
assert item('libraryRestart').isVisible();select('libraryfolderRecursion',1);assert model.requests[-1]==['folderRecursion','collapse']
select('libraryfolderRecursion',2);assert model.requests[-1]==['folderRecursion','expand']and not model.restartRequired
model.patch(folderRecursion='opaque-preserved');count=len(model.requests)
assert item('libraryfolderRecursion').property('currentIndex')==-1 and len(model.requests)==count
select('libraryfolderRecursion',1);assert model.requests[-1]==['folderRecursion','collapse']
model.options['folderRecursion']=model.options['folderRecursion'][:1];model.choicesChanged.emit();QTest.qWait(30)
assert item('libraryfolderRecursion').property('currentIndex')==-1
model.options['folderRecursion']=[dict(value=v,name=n)for v,n in[('none','Do not expand folders automatically'),('collapse','Expand opened folders only'),('expand','Include subfolders')]];model.choicesChanged.emit();QTest.qWait(30)
done('Actual native recursion strings and live choice notifications preserve unknown/removed presets until an explicit supported selection')
before=model.state['useMediaLibrary'];click('libraryuseMediaLibrary');assert model.requests[-1]==['useMediaLibrary',not before]and model.active is True
assert 'currently active' in evaluate(page,'helpFor("useMediaLibrary")')and item('libraryRestart').isVisible()
assert 'Never clears' in evaluate(page,'helpFor("useMediaLibrary")')and 'first-run' in evaluate(page,'helpFor("useMediaLibrary")')
model.active=False;model.valuesChanged.emit();assert 'currently inactive' in evaluate(page,'helpFor("useMediaLibrary")')
model.module=False;model.refresh_caps();QTest.qWait(25);assert not item('libraryuseMediaLibrary').isEnabled()
model.module=True;model.active=True;model.refresh_caps();model.patch(useMediaLibrary=True,folderRecursion='expand')
done('Saved media-library startup default stays separate from current availability; no roots/history/database/first-run action or immediate library construction')
model.accepted.update({key:True for key in model.accepted});model.refresh_caps();QTest.qWait(25)
assert item('librarysingleInstance').isEnabled()and not item('libraryfileManagerSingleInstance').isEnabled()
model.legacy=True;model.patch(fileManagerSingleInstance=True);assert item('libraryfileManagerSingleInstance').property('selected')and not item('libraryfileManagerSingleInstance').isEnabled()
assert 'file-manager launches' in evaluate(page,'helpFor("singleInstance")')
click('librarysingleInstance');assert model.requests[-1]==['singleInstance',True]and model.file_policy==1 and not model.legacy
assert item('libraryfileManagerSingleInstance').isEnabled()and item('libraryenqueueInSingleInstance').isEnabled()
click('libraryfileManagerSingleInstance');assert model.requests[-1]==['fileManagerSingleInstance',False]and model.file_policy==0
click('libraryfileManagerSingleInstance');assert model.requests[-1]==['fileManagerSingleInstance',True]
click('librarysingleInstance');assert model.state['fileManagerSingleInstance']is True and not item('libraryfileManagerSingleInstance').isEnabled()
click('librarysingleInstance');assert model.state['fileManagerSingleInstance']is True and item('libraryfileManagerSingleInstance').isEnabled()
done('Explicit supplied consumer seam enforces the saved master/child hierarchy while retaining checked-but-disabled child and untouched legacy policy until edit')
assert model.state['enqueueInSingleInstance']is None and item('libraryenqueueInSingleInstance').property('custom')
count=len(model.requests);model.valuesChanged.emit();QTest.qWait(25);assert len(model.requests)==count and model.open_mode==-1
focus('libraryenqueueInSingleInstance');QTest.keyClick(window,Qt.Key_Left);QTest.qWait(25)
assert model.requests[-1]==['enqueueInSingleInstance',False]and model.open_mode==0
model.open_mode=-1;model.patch(enqueueInSingleInstance=None);count=len(model.requests);click('libraryenqueueInSingleInstance')
assert len(model.requests)==count and evaluate(item('libraryenqueueInSingleInstance'),'customPopup.visible')
QTest.keyClick(window,Qt.Key_Escape);QTest.qWait(25);assert dialog.property('visible')and not evaluate(item('libraryenqueueInSingleInstance'),'customPopup.visible')and len(model.requests)==count
click('libraryenqueueInSingleInstance');reloads=model.reloads;provider.showGlobalSettingsDialog.emit();QTest.qWait(25)
assert len(model.requests)==count and model.reloads==reloads and not evaluate(item('libraryenqueueInSingleInstance'),'customPopup.visible')and item('globalSettingsSearch').hasActiveFocus()
click('libraryenqueueInSingleInstance');QTest.keyClick(window,Qt.Key_Down);QTest.keyClick(window,Qt.Key_Down);QTest.keyClick(window,Qt.Key_Return);QTest.qWait(30)
assert model.requests[-1]==['enqueueInSingleInstance',False]and model.open_mode==0
model.open_mode=-1;model.patch(enqueueInSingleInstance=None);click('libraryenqueueInSingleInstance')
QTest.keyClick(window,Qt.Key_Down);QTest.keyClick(window,Qt.Key_Return);QTest.qWait(30)
assert model.requests[-1]==['enqueueInSingleInstance',True]and model.open_mode==1 and not item('libraryenqueueInSingleInstance').property('custom'),(model.requests[-3:],model.open_mode,evaluate(item('libraryenqueueInSingleInstance'),'customPopup.currentIndex'),evaluate(item('libraryenqueueInSingleInstance'),'customPopup.visible'))
click('libraryenqueueInSingleInstance');assert model.requests[-1]==['enqueueInSingleInstance',False]and model.open_mode==0
assert 'named playlists are unchanged' in evaluate(page,'helpFor("enqueueInSingleInstance")')
click('libraryshowHiddenFiles');assert model.requests[-1]==['showHiddenFiles',True]
assert 'Explorer' in evaluate(page,'helpFor("showHiddenFiles")')and 'existing explicit roots' in evaluate(page,'helpFor("folderRecursion")')
done('Invalid legacy queue value is visibly Custom and never coerced; explicit On/Off send strict booleans and hidden/folder scope stays truthful')
select('librarymetadataSource',1);assert model.requests[-1]==['metadataSource','network']and model.restartRequired
assert 'listening-history' in evaluate(page,'helpFor("metadataSource")')and 'Privacy' in evaluate(page,'helpFor("metadataSource")')
select('librarymetadataSource',0);assert model.requests[-1]==['metadataSource','local']
model.fail_next='Owned native config is locked.';before=model.state['metadataSource'];select('librarymetadataSource',1)
assert model.state['metadataSource']==before and item('librarymetadataSource').property('currentIndex')==0 and 'locked' in item('libraryError').property('text')
model.fail_next='Owned native config is locked.';before=model.state['showHiddenFiles'];click('libraryshowHiddenFiles')
assert model.state['showHiddenFiles']==before and item('libraryshowHiddenFiles').property('selected')is before
click('libraryshowHiddenFiles');assert not model.error and not item('libraryError').isVisible()
done('Strict metadata strings share the actual Privacy flag and failed dropdown/toggle saves retain native state with genuine feedback')
# Every authored info icon is also an accessible keyboard help action.
focus('libraryfolderRecursionInfo');QTest.keyClick(window,Qt.Key_Space);QTest.qWait(30)
assert item('libraryfolderRecursionInfo').property('tipOpen')and item('libraryfolderRecursionInfo').hasActiveFocus()
before=model.reloads;provider.showGlobalSettingsDialog.emit();QTest.qWait(30)
assert model.reloads==before and not item('libraryfolderRecursionInfo').property('tipOpen')and item('globalSettingsSearch').hasActiveFocus()
focus('libraryfolderRecursion');QTest.keyClick(window,Qt.Key_Space);QTest.qWait(25);assert evaluate(item('libraryfolderRecursion'),'popup.visible')
provider.showGlobalSettingsDialog.emit();QTest.qWait(25);assert not evaluate(item('libraryfolderRecursion'),'popup.visible')and model.reloads==before
focus('globalSettingsSearch');text('Metadata');QTest.qWait(25)
assert item('librarymetadataSource').isVisible()and item('libraryuseMediaLibrary').isVisible()and not item('librarysingleInstance').isVisible()
QTest.keyClick(window,Qt.Key_A,Qt.ControlModifier);QTest.keyClick(window,Qt.Key_Backspace);QTest.qWait(25)
done('Actual keyboard info help, filtered available categories and repeated Settings requests close transient controls without reload or a second store')
images=[];fixture=json.loads((ROOT/'modules/gui/qt/tests/fixtures/juxtopposed_library_assets.json').read_text(encoding='utf-8'))
for name,entry in fixture.items():assert hashlib.sha256((assets/name).read_bytes()).hexdigest()==entry['sha256'],name
for key in SEED:
    obj=item('library'+key+'InfoOriginal');assert obj.width()==18 and obj.height()==18 and obj.scale()==1 and evaluate(obj,'status')==1
    images.append(dict(name=obj.objectName(),source=obj.property('source').toString(),width=obj.width(),height=obj.height()))
    if key not in ['folderRecursion','metadataSource']:assert item('library'+key).width()==68 and item('library'+key).height()==32
for name in ['libraryfolderRecursionOriginalChevron','librarymetadataSourceOriginalChevron','libraryAdvancedOriginalChevron','globalCategorylibraryOriginalIcon']:
    obj=item(name);assert obj.width()==24 and obj.height()==24 and obj.property('mirror')and evaluate(obj,'status')==1
    images.append(dict(name=name,source=obj.property('source').toString(),width=obj.width(),height=obj.height()))
assert len(images)==11 and item('libraryfolderRecursion').width()==320 and item('libraryfolderRecursion').height()==36
for key in ['general','interface','privacy','controls','playback','video','audio','subtitles','network','performance','recording','extensions']:
    obj=item('globalCategory'+key+'OriginalIcon');assert obj.width()==24 and obj.height()==24 and evaluate(obj,'status')==1
    filename=Path(obj.property('source').toLocalFile()).name;assert filename in fixture
    assert obj.property('mirror')==(key in ['video','network'])
    images.append(dict(name=obj.objectName(),source=obj.property('source').toString(),width=obj.width(),height=obj.height()))
for name in ['globalSettingsOriginalSearch','globalSettingsOriginalClose']:
    obj=item(name);assert obj.width()==24 and obj.height()==24 and evaluate(obj,'status')==1
    assert Path(obj.property('source').toLocalFile()).name in fixture
    images.append(dict(name=obj.objectName(),source=obj.property('source').toString(),width=obj.width(),height=obj.height()))
assert len(images)==25 and len({entry['source']for entry in images})==17
advanced=item('libraryAdvanced');icon=item('libraryAdvancedOriginalChevron');label=icon.parentItem().parentItem().childItems()[0]
left=label.mapToItem(advanced,QPointF(0,0)).x();right=icon.mapToItem(advanced,QPointF(icon.width(),0)).x()
assert abs((left+right)/2-advanced.width()/2)<.01 and abs(icon.mapToItem(advanced,QPointF(0,icon.height()/2)).y()-advanced.height()/2)<.01
assert window.grabWindow().save(str(results/'02-library-enabled-supplied-seam.png'))
done('All19 contextual originals unchanged;25 actual frame slots/17visible originals have exact18/24px geometry, mirrors,68/320px controls and centered Advanced label/icon')
geometry=[]
error_geometry=[]
for width,height,scale in[(1080,897,1),(640,420,1),(960,600,1.5),(1280,720,2)]:
    window.resize(width,height);dialog.setProperty('uiScale',scale);QTest.qWait(30)
    for key in['singleInstance','folderRecursion','metadataSource','useMediaLibrary']:
        for suffix in['Info','']:
            obj=focus('library'+key+suffix);point=obj.mapToScene(QPointF(0,0));view=item('globalSettingsPageScroll');top=view.mapToScene(QPointF(0,0))
            assert obj.hasActiveFocus()and top.x()-.5<=point.x()and point.x()+obj.width()<=top.x()+view.width()+.5 and top.y()-.5<=point.y()and point.y()+obj.height()<=top.y()+view.height()+.5,(key,suffix,point,top,view.height())
    model.feedback('');before=model.state['useMediaLibrary'];model.fail_next='Owned native library settings could not be saved.';click('libraryuseMediaLibrary');QTest.qWait(60)
    field=item('libraryuseMediaLibrary');tip=field.findChild(QObject,'libraryuseMediaLibraryValidationError');assert tip is not None and tip.property('visible')and tip.property('text')==model.error and field.hasActiveFocus()and model.state['useMediaLibrary']==before
    box=tip.property('background');point=box.mapToScene(QPointF(0,0));view=item('globalSettingsPageScroll');top=view.mapToScene(QPointF(0,0));field_point=field.mapToScene(QPointF(0,0))
    assert top.x()-.5<=point.x()and point.x()+box.width()<=top.x()+view.width()+.5 and top.y()-.5<=point.y()and point.y()+box.height()<=top.y()+view.height()+.5,(point,top,box.width(),box.height(),view.width(),view.height())
    assert top.y()-.5<=field_point.y()and field_point.y()+field.height()<=top.y()+view.height()+.5
    error_geometry.append(dict(width=width,height=height,scale=scale,error=[point.x(),point.y(),box.width(),box.height()],viewport=[top.x(),top.y(),view.width(),view.height()]))
    assert window.grabWindow().save(str(results/f'05-library-native-error-{width}-{scale}.png'));model.feedback('');QTest.qWait(25);assert not tip.property('visible')
    assert window.grabWindow().save(str(results/f'03-library-{width}-{scale}.png'));geometry.append(dict(width=width,height=height,scale=scale))
(results/'inline-error-geometry.json').write_text(json.dumps(error_geometry,indent=2)+'\n',encoding='utf-8')
done('Real controls, keyboard help and genuine lower-field save error all reveal inside actual host viewport at natural/narrow150/200 percent')
window.resize(1080,897);dialog.setProperty('uiScale',1);ctx.general.patch(theme=1,accentIndex=4);QTest.qWait(30)
assert page.property('accent').name()=='#18aad3'and not ctx.general.requests;assert window.grabWindow().save(str(results/'04-library-light-adaptation.png'))
QTest.keyClick(window,Qt.Key_Escape);QTest.qWait(25);assert not dialog.property('visible')and item('opener').hasActiveFocus()
model.external={'metadataSource':'network'};before=model.reloads;click('opener');assert model.reloads==before+1 and item('librarymetadataSource').property('currentIndex')==1
done('Live shared appearance, opener restoration and category-entry reload observe external/shared metadata changes without startup or first-run actions')
click('libraryAdvanced');QTest.qWait(35);assert not dialog.property('visible')and provider.experts==1
provider.native=False;provider.changed.emit()
click('opener');assert evaluate(dialog,'chooseCategory("library")');QTest.keyClick(window,Qt.Key_Escape);QTest.qWait(25)
window.setProperty('omitLibrary',True);click('opener');assert not evaluate(dialog,'chooseCategory("library")')and dialog.property('selectedCategory')=='general'
done('Advanced is one intentional native expert handoff; missing separate Library service disables category without General fallback')
destroyed=[];engine.destroyed.connect(lambda:destroyed.append(True));window.close();engine.deleteLater();QCoreApplication.sendPostedEvents(None,QEvent.DeferredDelete);QTest.qWait(25)
assert destroyed==[True]and not messages,messages
(results/'verification.json').write_text(json.dumps(dict(passed=True,caseCount=len(checks),cases=checks,messages=messages,geometry=geometry,renderedImageInstances=images,adapters=identities,nativeAcceptance=False,scope=__doc__,workerSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()),indent=2)+'\n',encoding='utf-8')
print('PASS Library supplied-contract component only.',flush=True)
