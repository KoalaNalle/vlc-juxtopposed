# SPDX-License-Identifier: GPL-2.0-or-later
"""Maintained Playback/host QML with an explicit native-shaped supplied contract.
This executes real proposed QML, not native VLC consumers, pickers or persistence.
"""
from pathlib import Path
import argparse,ast,hashlib,json,math,os,shutil,sys
from enum import IntEnum
HERE=Path(__file__).resolve().parent
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--repo-root',type=Path,default=HERE.parents[3])
parser.add_argument('--output-dir',type=Path,required=True)
args=parser.parse_args();ROOT=args.repo_root.resolve();results=args.output_dir.resolve();results.mkdir(parents=True,exist_ok=False)
os.environ.update(QT_QPA_PLATFORM='offscreen',QT_QUICK_BACKEND='software',QML_DISABLE_DISK_CACHE='1',QT_QPA_FONTDIR=str(ROOT/'modules/gui/qt/pixmaps/juxtopposed'))
from PySide6.QtCore import QObject,Property,Signal,Slot,QEnum,Qt,QUrl,QPointF,QEvent,QCoreApplication,qInstallMessageHandler
from PySide6.QtGui import QGuiApplication,QKeyEvent
from PySide6.QtQml import QQmlApplicationEngine,QQmlExpression,qmlContext,qmlRegisterType,qmlRegisterSingletonType,qmlRegisterUncreatableType
from PySide6.QtQuick import QQuickItem
from PySide6.QtTest import QTest
sys.path.insert(0,str(ROOT/'modules/gui/qt/tests'))
from juxtopposed_general_preferences_fixture import GeneralPreferences as General
fields=json.loads((ROOT/'modules/gui/qt/tests/fixtures/juxtopposed_playback_fields.json').read_text(encoding='utf-8'))
seed=dict(continuePlayback=1,startPlayback=True,defaultPlaybackSpeed=1.25,pauseOtherAudio=False,
 preferredVideoQuality=576,preferredMenuLanguage='en',defaultAudioTrack=-1,defaultSubtitleTrack=0,
 repeatMode=0,shuffle=False,stopAfterEachItem=False,finishAction=0,preciseSeeking=True,showThumbnails=False,
 defaultOpticalDrive='',aviRepair=0,mkvPreload=True,timeShiftFolder='',timeShiftSize=-1)
assert list(seed)==[f['key']for f in fields] and len(seed)==19
booleans={k for k,v in seed.items()if type(v)is bool}
class PlaybackPreferences(QObject):
    valuesChanged=Signal();capabilitiesChanged=Signal();lastErrorChanged=Signal();restartRequiredChanged=Signal();pickerActiveChanged=Signal()
    def __init__(self):
        super().__init__();self.state=dict(seed);self.caps={k:k not in ['preciseSeeking','showThumbnails']for k in seed}
        self.options={
         'continuePlayback':[dict(value=2,name='Always'),dict(value=0,name='Never'),dict(value=1,name='Ask')],
         'preferredVideoQuality':[dict(value=v,name=n)for v,n in[(-1,'Best available'),(1080,'1080 lines'),(720,'720 lines'),(576,'576 lines'),(360,'360 lines'),(240,'240 lines')]],
         'preferredMenuLanguage':[dict(value='',name='Default language'),dict(value='any',name='Any language'),dict(value='en',name='English'),dict(value='fi',name='Finnish')],
         'repeatMode':[dict(value=0,name='Off'),dict(value=1,name='One item'),dict(value=2,name='All items')],
         'finishAction':[dict(value=0,name='Stop'),dict(value=1,name='Quit VLC')],
         'aviRepair':[dict(value=v,name=n)for v,n in[(0,'Ask'),(1,'Always fix'),(2,'Never fix'),(3,'Fix when necessary')]]}
        self.modes={k:'nextInput'for k in seed}
        self.modes.update({k:'live'for k in['startPlayback','repeatMode','shuffle','stopAfterEachItem','finishAction','preciseSeeking']})
        self.modes.update(defaultPlaybackSpeed='restart',pauseOtherAudio='restart',showThumbnails='unavailable')
        self.notes={k:'Supplied native help for '+k for k in seed}
        self.notes.update(timeShiftSize='Maximum bytes per native temporary-file chunk, not a total buffer limit. -1 uses the native 50 MiB chunk default.',
          defaultAudioTrack='Zero-based stream index; -1 means automatic.',defaultSubtitleTrack='Zero-based stream index; -1 means automatic.',
          pauseOtherAudio='Audio communication/ducking events after restart; ordinary other-app audio is not promised.')
        self.error='';self.restart=False;self.picking=False;self.requests=[];self.reloads=0;self.pickers=0;self.fail_next='';self.next_folder=None;self.during_picker=None
    values=Property('QVariantMap',lambda s:s.state,notify=valuesChanged)
    capabilities=Property('QVariantMap',lambda s:s.caps,notify=capabilitiesChanged)
    choices=Property('QVariantMap',lambda s:s.options,constant=True)
    applyModes=Property('QVariantMap',lambda s:s.modes,constant=True)
    supportNotes=Property('QVariantMap',lambda s:s.notes,notify=valuesChanged)
    lastError=Property(str,lambda s:s.error,notify=lastErrorChanged)
    restartRequired=Property(bool,lambda s:s.restart,notify=restartRequiredChanged)
    pickerActive=Property(bool,lambda s:s.picking,notify=pickerActiveChanged)
    def feedback(self,text):self.error=text;self.lastErrorChanged.emit()
    def patch(self,**values):
        assert all(k in seed for k in values);self.state={**self.state,**values};self.valuesChanged.emit()
        self.caps['stopAfterEachItem']=self.state['finishAction']==0;self.capabilitiesChanged.emit()
        restart=any(self.state[k]!=seed[k]for k in['defaultPlaybackSpeed','pauseOtherAudio'])
        if restart!=self.restart:self.restart=restart;self.restartRequiredChanged.emit()
    @Slot()
    def reload(self):
        assert not self.picking;self.reloads+=1;self.feedback('');self.valuesChanged.emit()
    @Slot(str,'QVariant',result=bool)
    def setValue(self,key,value):
        assert key in seed and self.caps[key] and not self.picking,(key,value)
        self.requests.append([key,value])
        valid=True
        if key in booleans:valid=type(value)is bool
        elif key in self.options:valid=any(type(value)is type(r['value'])and value==r['value']for r in self.options[key])
        elif key=='defaultPlaybackSpeed':valid=type(value)in[int,float]and math.isfinite(value)and .25<=value<=2 and value*4==int(value*4)
        elif key in['defaultAudioTrack','defaultSubtitleTrack','timeShiftSize']:
            valid=type(value)in[int,float]and math.isfinite(value)and value==int(value)and -1<=value<=2147483647
            if key=='timeShiftSize':valid=valid and(value==-1 or value>=1048576)
        else:
            valid=type(value)is str
            if key=='timeShiftFolder':valid=valid and(not value or Path(value).is_absolute()and Path(value).is_dir()and os.access(value,os.W_OK))
        if not valid:self.feedback('Choose a valid native Playback value.');return False
        if self.fail_next:self.feedback(self.fail_next);self.fail_next='';return False
        self.patch(**{key:value});self.feedback('');return True
    @Slot(result=bool)
    def chooseTimeShiftFolder(self):
        assert self.caps['timeShiftFolder']and not self.picking
        self.feedback('');self.pickers+=1;self.picking=True;self.pickerActiveChanged.emit()
        if self.during_picker:self.during_picker()
        self.picking=False;self.pickerActiveChanged.emit()
        if self.next_folder is None:return False
        result=self.setValue('timeShiftFolder',self.next_folder);self.next_folder=None;return result
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
    def __init__(self):super().__init__();self.general=General();self.playback=PlaybackPreferences()
    generalPreferences=Property(QObject,lambda s:s.general,constant=True)
    playbackPreferences=Property(QObject,lambda s:s.playback,constant=True)
    useGlobalShortcuts=Property(bool,lambda s:True,constant=True)
messages=[];qInstallMessageHandler(lambda kind,context,message:messages.append(message))
app=QGuiApplication([]);ctx=MainContext();model=ctx.playback;provider=Provider()
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
 property bool omitPlayback:false
 QtObject { id:nativePalette;property bool isDark:suppliedContext.generalPreferences.values.theme!==1 }
 Binding { target:JuxtopposedPalette;property:"nativePalette";value:nativePalette }
 QtObject { id:generalOnly;property var generalPreferences:suppliedContext.generalPreferences }
 Rectangle { id:underlay;anchors.fill:parent;color:"#173c66" }
 C.Button { id:opener;objectName:"opener";text:"Settings";onClicked:suppliedProvider.showGlobalSettingsDialog() }
 Binding { target:JuxtopposedPalette;property:"preferences";value:suppliedContext.generalPreferences }
 Dialogs.JuxtopposedDialogsHost { anchors.fill:parent;mainCtx:window.omitPlayback ? generalOnly : suppliedContext;dialogsProvider:suppliedProvider;appearance:JuxtopposedPalette;assetBase:"@ASSETS@";backdropSource:underlay;fallbackFocusItem:opener }
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
    obj=focus(name);assert obj.isEnabled()and obj.isVisible(),name
    QTest.mouseClick(window,Qt.LeftButton,Qt.NoModifier,obj.mapToScene(QPointF(obj.width()/2,obj.height()/2)).toPoint());QTest.qWait(25)
def text(value):
    for char in value:
        for kind in[QEvent.KeyPress,QEvent.KeyRelease]:QGuiApplication.sendEvent(window,QKeyEvent(kind,0,Qt.NoModifier,char))
def edit(name,value):
    focus(name);QTest.keyClick(window,Qt.Key_A,Qt.ControlModifier);text(value);QTest.keyClick(window,Qt.Key_Tab);QTest.qWait(25)
def select(name,index):
    focus(name);QTest.keyClick(window,Qt.Key_Space);QTest.qWait(20);assert item(name+'Popup').property('visible');QTest.keyClick(window,Qt.Key_Home)
    for _ in range(index):QTest.keyClick(window,Qt.Key_Down)
    QTest.keyClick(window,Qt.Key_Return);QTest.qWait(25)
def done(name):checks.append(name);print('PASS',name,flush=True)
QTest.qWait(70);click('opener');dialog=item('juxtopposedGlobalSettings');assert evaluate(dialog,'chooseCategory("playback")')
page=item('juxtopposedPlaybackSettings')
assert page.property('service')is model and dialog.property('preferences')is ctx.general and not model.requests and not ctx.general.requests and model.reloads==1
assert dialog.property('title')=='Playback Preferences'and item('playbackcontinuePlayback').property('currentIndex')==2
assert not item('playbackshowThumbnails').isEnabled()and not item('playbackpreciseSeeking').isEnabled()
assert item('playbackdefaultPlaybackSpeed').property('value')==1.25 and item('playbackdefaultAudioTrack').property('text')=='-1'
assert not item('playbacktimeShiftFolder').isVisible()
assert item('playbackAdvanced').height()==38 and item('playbackAdvancedOriginalChevron').property('source').toString().endswith('playback-settings-97863.svg')
done('Actual host mounts explicit19-key Playback service; native choices/seeds and unsupported gates, no load writes')
assert window.grabWindow().save(str(results/'01-playback-dark-authored.png'))
select('playbackcontinuePlayback',0);assert model.requests[-1]==['continuePlayback',2]
select('playbackpreferredVideoQuality',4);assert model.requests[-1]==['preferredVideoQuality',360]
select('playbackpreferredMenuLanguage',3);assert model.requests[-1]==['preferredMenuLanguage','fi']
select('playbackrepeatMode',2);assert model.requests[-1]==['repeatMode',2]
select('playbackaviRepair',3);assert model.requests[-1]==['aviRepair',3]
done('All native dropdowns submit their actual numeric/string values, including reordered nonordinal resume choices')
for key in['startPlayback','shuffle','stopAfterEachItem','mkvPreload']:
    before=model.state[key];click('playback'+key);assert model.requests[-1]==[key,not before]
edit('playbackdefaultAudioTrack','2');assert model.requests[-1]==['defaultAudioTrack',2]
edit('playbackdefaultSubtitleTrack','-1');assert model.requests[-1]==['defaultSubtitleTrack',-1]
edit('playbackdefaultOpticalDrive','D:/Generated Disc');assert model.requests[-1]==['defaultOpticalDrive','D:/Generated Disc']
done('Actual toggles and text edits deliver strict booleans, zero-based whole tracks and literal optical path')
before=model.state['defaultPlaybackSpeed'];focus('playbackdefaultPlaybackSpeed');QTest.keyClick(window,Qt.Key_Right);QTest.qWait(25)
assert model.requests[-1]==['defaultPlaybackSpeed',before+.25]and model.restart and item('playbackSettingsRestart').isVisible()
click('playbackpauseOtherAudio');assert model.requests[-1]==['pauseOtherAudio',True]
assert evaluate(page,'helpFor("pauseOtherAudio")').find('Restart VLC')>=0
model.patch(defaultPlaybackSpeed=seed['defaultPlaybackSpeed'],pauseOtherAudio=seed['pauseOtherAudio']);QTest.qWait(20)
assert not model.restart and not item('playbackSettingsRestart').isVisible()
done('Actual speed keyboard quarter steps and restart-only native settings show and clear restart feedback')
select('playbackfinishAction',1);assert model.requests[-1]==['finishAction',1]and not item('playbackstopAfterEachItem').isEnabled()
select('playbackfinishAction',0);assert model.requests[-1]==['finishAction',0]and item('playbackstopAfterEachItem').isEnabled()
model.caps['preciseSeeking']=True;model.capabilitiesChanged.emit();QTest.qWait(20);click('playbackpreciseSeeking');assert model.requests[-1]==['preciseSeeking',False]
done('Live native capability changes enforce stopped-action precedence and explicit precise-seek consumer gate')
before=len(model.requests);model.patch(preferredMenuLanguage='fi,en',preferredVideoQuality=432,defaultPlaybackSpeed=3.125)
assert item('playbackpreferredMenuLanguage').property('currentIndex')==-1 and item('playbackpreferredVideoQuality').property('currentIndex')==-1
assert 'matching quarter-step' in evaluate(item('playbackdefaultPlaybackSpeed'),'Accessible.description')
assert len(model.requests)==before
model.patch(defaultOpticalDrive=None);QTest.qWait(25)
assert item('playbackdefaultOpticalDrive').isEnabled()and item('playbackdefaultOpticalDrive').property('text')=='' and item('playbackdefaultOpticalDrive').property('placeholderText')=='Different saved drive defaults'
assert len(model.requests)==before
edit('playbackdefaultOpticalDrive','D:/Generated Unified Disc');assert model.requests[-1]==['defaultOpticalDrive','D:/Generated Unified Disc']
select('playbackpreferredMenuLanguage',2);assert model.requests[-1]==['preferredMenuLanguage','en']
model.patch(defaultPlaybackSpeed=1.25,preferredVideoQuality=576)
done('Unknown/multi-language/custom native states remain unselected and unchanged until explicit preset selection')
before=model.state['defaultAudioTrack'];edit('playbackdefaultAudioTrack','2.5')
assert model.state['defaultAudioTrack']==before and float(item('playbackdefaultAudioTrack').property('text'))==before and item('playbackSettingsError').isVisible(),(before,model.state['defaultAudioTrack'],item('playbackdefaultAudioTrack').property('text'),model.error,model.requests[-3:])
model.fail_next='The owned settings file is locked.';before_shuffle=model.state['shuffle'];click('playbackshuffle')
assert model.state['shuffle']==before_shuffle and 'locked' in item('playbackSettingsError').property('text')
model.fail_next='The owned settings file is locked.';before=model.state['defaultPlaybackSpeed'];focus('playbackdefaultPlaybackSpeed');QTest.keyClick(window,Qt.Key_Right);QTest.qWait(25)
assert model.state['defaultPlaybackSpeed']==before and item('playbackdefaultPlaybackSpeed').property('value')==before
click('playbackshuffle');assert model.state['shuffle']is not before_shuffle and not item('playbackSettingsError').isVisible()
done('Invalid integer/native save failures keep previous values and resync edited input, toggle and slider without false success')
model.fail_next='The owned settings file is locked.';before=model.state['preferredVideoQuality'];select('playbackpreferredVideoQuality',0)
assert model.state['preferredVideoQuality']==before and item('playbackpreferredVideoQuality').property('currentIndex')==3
model.fail_next='The owned settings file is locked.';before=model.state['defaultOpticalDrive'];edit('playbackdefaultOpticalDrive','E:/Generated Rejected Disc')
assert model.state['defaultOpticalDrive']==before and item('playbackdefaultOpticalDrive').property('text')==before
done('Failed native dropdown and path saves also restore their original presentation without replacing native state')
click('playbackAdvanced');assert item('playbacktimeShiftFolder').isVisible()
owned_folder=results/'Generated buffer Åä中';owned_folder.mkdir();folder=owned_folder.resolve().as_posix()
edit('playbacktimeShiftFolder',folder);assert model.requests[-1]==['timeShiftFolder',folder]
edit('playbacktimeShiftFolder',(results/'Not a created folder').resolve().as_posix());assert model.state['timeShiftFolder']==folder and item('playbacktimeShiftFolder').property('text')==folder
select('playbacktimeShiftSize',3);assert model.requests[-1]==['timeShiftSize',52428800]
assert 'per native temporary-file chunk' in evaluate(page,'helpFor("timeShiftSize")')
model.patch(timeShiftSize=7777777);assert item('playbacktimeShiftSize').property('currentIndex')==-1
done('Authored Advanced Options expands genuine time-shift rows; explicit byte presets never pretend Medium is a native enum')
before=dict(model.state);n=len(model.requests);click('playbacktimeShiftBrowse');assert model.pickers==1 and model.state==before and len(model.requests)==n
picked_folder=results/'Generated chosen buffer';picked_folder.mkdir();model.next_folder=picked_folder.resolve().as_posix();click('playbacktimeShiftBrowse');assert model.requests[-1]==['timeShiftFolder',model.state['timeShiftFolder']]
def while_picker():
    QTest.qWait(30);assert page.property('busy')and dialog.property('busy')
    before=model.reloads;provider.showGlobalSettingsDialog.emit();QTest.qWait(25);assert model.reloads==before and dialog.property('visible')
    assert not evaluate(dialog,'chooseCategory("general")')and not item('globalSettingsClose').isEnabled()
    assert item('globalSettingsSearch').property('readOnly')and not item('playbacktimeShiftBrowse').isEnabled()and not item('playbackshuffle').isEnabled()
    for signal,name in[(provider.showNetworkStreamDialog,'juxtopposedNetworkDialog'),(provider.showNewPlaylistDialog,'juxtopposedNewPlaylistDialog'),(provider.showConversionDialog,'juxtopposedConversionDialog')]:
        signal.emit();QTest.qWait(20);assert dialog.property('visible')and not item(name).property('visible')
    QTest.keyClick(window,Qt.Key_Escape);QTest.qWait(20);assert dialog.property('visible')
model.during_picker=while_picker;click('playbacktimeShiftBrowse');model.during_picker=None
assert not page.property('busy')and item('globalSettingsClose').isEnabled()
done('Supplied native-picker cancel/select and nested-loop busy guards reject repeated settings/reload, edits, category/other-modal routes and Escape')
n=model.reloads;focus('playbackcontinuePlayback');QTest.keyClick(window,Qt.Key_Space);QTest.qWait(20);assert item('playbackcontinuePlaybackPopup').property('visible')
provider.showGlobalSettingsDialog.emit();QTest.qWait(25)
assert model.reloads==n and not item('playbackcontinuePlaybackPopup').property('visible')and item('globalSettingsSearch').hasActiveFocus()
page.setProperty('advancedExpanded',False);focus('globalSettingsSearch');text('Time-shift');QTest.qWait(25)
assert item('playbacktimeShiftFolder').isVisible()and not item('playbackdefaultAudioTrack').isVisible()
assert item('playbackAdvancedOriginalChevron').property('source').toString().endswith('playback-settings-764c9.svg')
QTest.keyClick(window,Qt.Key_A,Qt.ControlModifier);QTest.keyClick(window,Qt.Key_Backspace);QTest.qWait(25)
assert not item('playbacktimeShiftFolder').isVisible();click('playbackAdvanced')
done('Real search reveals advanced rows; repeated request closes transient choices and preserves active service without reload')
images=[]
fixture=json.loads((ROOT/'modules/gui/qt/tests/fixtures/juxtopposed_playback_assets.json').read_text(encoding='utf-8'))
for name,entry in fixture.items():assert hashlib.sha256((assets/name).read_bytes()).hexdigest()==entry['sha256'],name
for key in seed:
    if key=='defaultPlaybackSpeed':continue
    obj=item('playback'+key+'InfoOriginal')
    assert obj.width()==18 and obj.height()==18 and evaluate(obj,'status')==1 and obj.scale()==1
    images.append(dict(name=obj.objectName(),source=obj.property('source').toString(),width=obj.width(),height=obj.height(),scale=obj.scale()))
for key in['continuePlayback','preferredVideoQuality','preferredMenuLanguage','repeatMode','finishAction','aviRepair','timeShiftSize']:
    obj=item('playback'+key+'OriginalChevron');assert obj.width()==24 and obj.height()==24 and obj.property('mirror')and evaluate(obj,'status')==1
    images.append(dict(name=obj.objectName(),source=obj.property('source').toString(),width=obj.width(),height=obj.height(),scale=obj.scale()))
for index in range(8):
    obj=item('playbackOriginalSpeedTick'+str(index));assert [obj.width(),obj.height()]==([32,32]if index in[0,7]else[2,8])and evaluate(obj,'status')==1
    assert obj.parentItem().clip()==(index in[0,7])
    images.append(dict(name=obj.objectName(),source=obj.property('source').toString(),width=obj.width(),height=obj.height(),scale=obj.scale()))
assert len(images)==33
for name,tail in [('playbackAdvancedOriginalChevron','playback-settings-764c9.svg'),('globalCategoryplaybackOriginalIcon','playback-settings-bc944.svg')]:
    obj=item(name);assert obj.width()==24 and obj.height()==24 and obj.property('source').toString().endswith(tail)and evaluate(obj,'status')==1
    images.append(dict(name=obj.objectName(),source=obj.property('source').toString(),width=obj.width(),height=obj.height(),scale=obj.scale()))
assert window.grabWindow().save(str(results/'02-playback-expanded-dark.png'))
done('All35 original body/disclosure/selected-category images resolve unchanged artwork at exact root geometry, mirrors and endpoint clipping')
geometry=[]
for width,height,scale in[(1080,897,1),(640,420,1),(960,600,1.5),(1280,720,2)]:
    window.resize(width,height);dialog.setProperty('uiScale',scale);QTest.qWait(25)
    for key in['continuePlayback','defaultPlaybackSpeed','defaultAudioTrack','defaultOpticalDrive','timeShiftFolder','timeShiftSize']:
        obj=focus('playback'+key);assert obj.hasActiveFocus(),key
        point=obj.mapToScene(QPointF(0,0));viewport=item('globalSettingsPageScroll');top=viewport.mapToScene(QPointF(0,0)).y()
        assert point.x()>=-.5 and point.x()+obj.width()<=width+.5 and point.y()>=top-.5 and point.y()+obj.height()<=top+viewport.height()+.5,(key,point,top,viewport.height())
    for i in range(8):assert item('playbackOriginalSpeedTick'+str(i)).scale()==scale
    geometry.append(dict(width=width,height=height,scale=scale));assert window.grabWindow().save(str(results/f'03-playback-{width}-{scale}.png'))
done('Actual enabled focus targets reveal within host at natural, narrow,150% and200%; authored asset scales follow provider')
window.resize(1080,897);dialog.setProperty('uiScale',1);ctx.general.patch(theme=1,accentIndex=4);QTest.qWait(30)
assert page.property('accent').name()=='#18aad3'and item('playbackdefaultAudioTrack').property('color').name()!='#8f8f8f'
assert not ctx.general.requests and window.grabWindow().save(str(results/'04-playback-light-adaptation.png'))
ctx.general.patch(theme=2,accentIndex=0)
QTest.keyClick(window,Qt.Key_Escape);QTest.qWait(25);assert not dialog.property('visible')and item('opener').hasActiveFocus()
window.setProperty('omitPlayback',True);click('opener');assert not evaluate(dialog,'chooseCategory("playback")')and dialog.property('selectedCategory')=='general'
done('Actual shared theme/accent, opener focus restore and missing-service disabled category preserve General ownership')
# Scrolled native-error visibility regression: same exact provider and original
# oracle. Return keeps the rejected field focused; the feedback must be inside
# the actual scroll viewport rather than merely setting visible=true above it.
window.setProperty('omitPlayback',False);QTest.keyClick(window,Qt.Key_Escape);QTest.qWait(25)
click('opener');assert evaluate(dialog,'chooseCategory("playback")')
page=item('juxtopposedPlaybackSettings');page.setProperty('advancedExpanded',True)
error_geometry=[]
for width,height,scale in [(1080,897,1),(640,420,1),(960,600,1.5),(1280,720,2)]:
    window.resize(width,height);dialog.setProperty('uiScale',scale);model.patch(timeShiftFolder='');model.feedback('');QTest.qWait(40)
    field=focus('playbacktimeShiftFolder');viewport=item('globalSettingsPageScroll')
    before=len(model.requests);QTest.keyClick(window,Qt.Key_A,Qt.ControlModifier);text('not-a-folder');QTest.keyClick(window,Qt.Key_Return);QTest.qWait(80)
    assert len(model.requests)==before+1 and model.requests[-1]==['timeShiftFolder','not-a-folder']
    assert model.state['timeShiftFolder']==''and field.property('text')==''and field.hasActiveFocus()
    assert model.error and item('playbackSettingsError').isVisible()and dialog.property('visible')and not dialog.property('busy')
    top=viewport.mapToScene(QPointF(0,0));banner=item('playbackSettingsError');banner_point=banner.mapToScene(QPointF(0,0))
    tip=window.findChild(QObject,'playbacktimeShiftFolderValidationError')
    assert tip is not None,('Missing in-viewport feedback after actual lower-field rejection',width,height,scale,top,banner_point)
    assert tip.property('visible')and not tip.property('focus')and tip.property('text')==model.error
    box=tip.property('background');point=box.mapToScene(QPointF(0,0));field_point=field.mapToScene(QPointF(0,0))
    assert point.y()>=top.y()-.5 and point.y()+box.height()<=top.y()+viewport.height()+.5,(point,box.height(),top,viewport.height())
    assert point.x()>=top.x()-.5 and point.x()+box.width()<=top.x()+viewport.width()+.5,(point,box.width(),top,viewport.width())
    assert field_point.y()>=top.y()-.5 and field_point.y()+field.height()<=top.y()+viewport.height()+.5,('field-and-error-must-both-fit',width,height,scale,field_point,field.height(),point,box.height(),top,viewport.height())
    error_geometry.append(dict(width=width,height=height,scale=scale,error=[point.x(),point.y(),box.width(),box.height()],viewport=[top.x(),top.y(),viewport.width(),viewport.height()],fieldFocused=field.hasActiveFocus()))
    assert window.grabWindow().save(str(results/f'05-inline-folder-error-{width}-{scale}.png'))
    n=len(model.requests);provider.showGlobalSettingsDialog.emit();QTest.qWait(25)
    assert len(model.requests)==n and dialog.property('visible')and item('globalSettingsSearch').hasActiveFocus()and not tip.property('visible')
    # Existing repeated-route behavior may focus Search; return to the same
    # field and retain the last failure until edit/success clears it.
    focus('playbacktimeShiftFolder');QTest.qWait(30);assert tip.property('visible')
    QTest.keyClick(window,Qt.Key_A,Qt.ControlModifier);text('x');QTest.keyClick(window,Qt.Key_A,Qt.ControlModifier);QTest.keyClick(window,Qt.Key_Backspace);QTest.qWait(30)
    assert not tip.property('visible')
(results/'inline-error-geometry.json').write_text(json.dumps(dict(layouts=error_geometry,nativeAccepted=False),indent=2)+'\n',encoding='utf-8')
done('Rejected lower Advanced folder retains rollback/focus and genuine native error inside actual viewport at natural/narrow150/200 percent')
destroyed=[];engine.destroyed.connect(lambda:destroyed.append(True));window.close();engine.deleteLater();QCoreApplication.sendPostedEvents(None,QEvent.DeferredDelete);QTest.qWait(25)
assert destroyed==[True]and not messages,messages
(results/'verification.json').write_text(json.dumps(dict(passed=True,caseCount=len(checks),cases=checks,messages=messages,geometry=geometry,renderedImageInstances=images,adapters=identities,nativeAcceptance=False,scope=__doc__,workerSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()),indent=2)+'\n',encoding='utf-8',newline='\n')
print('PASS Playback supplied-contract component only.',flush=True)
