# SPDX-License-Identifier: GPL-2.0-or-later
"""Actual Video category/shared host with strict supplied QObject state.
No native settings reset, C++ storage, video/window/media, picker or full-fidelity acceptance.
"""
from pathlib import Path
import argparse,ast,hashlib,json,math,os,shutil,sys,re,copy
from enum import IntEnum
HERE=Path(__file__).resolve().parent
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--repo-root',type=Path,default=HERE.parents[3]);parser.add_argument('--output-dir',type=Path,required=True)
args=parser.parse_args();ROOT=args.repo_root.resolve();results=args.output_dir.resolve();results.mkdir(parents=True,exist_ok=False)
os.environ.update(QT_QPA_PLATFORM='offscreen',QT_QUICK_BACKEND='software',QML_DISABLE_DISK_CACHE='1',QT_QPA_FONTDIR=str(ROOT/'modules/gui/qt/pixmaps/juxtopposed'))
from PySide6.QtCore import QObject,Property,Signal,Slot,QEnum,Qt,QUrl,QPointF,QEvent,QCoreApplication,qInstallMessageHandler
from PySide6.QtGui import QGuiApplication,QKeyEvent
from PySide6.QtQml import QQmlApplicationEngine,QQmlExpression,qmlContext,qmlRegisterType,qmlRegisterSingletonType,qmlRegisterUncreatableType
from PySide6.QtQuick import QQuickItem
from PySide6.QtTest import QTest
sys.path.insert(0,str(ROOT/'modules/gui/qt/tests'))
from juxtopposed_general_preferences_fixture import GeneralPreferences as General
fields=json.loads((ROOT/'modules/gui/qt/tests/fixtures/juxtopposed_video_fields.json').read_text(encoding='utf-8'))
seed=dict(enableVideo=True,startFullscreen=False,disableScreensaver=True,wallpaper=False,hue=0.0,brightness=1.0,contrast=1.0,saturation=1.0,gamma=1.0,
 deinterlace=-1,deinterlaceMode='',aspectRatio='',crop='',scaleToWindow=True,zoom='fit',position=0,
 snapshotDirectory='',snapshotPrefix='vlcsnap-',snapshotFormat='png',snapshotSize='source',snapshotSequential=False,snapshotPreview=True,
 customAspectRatios='',customCropRatios='',monitorPixelAspectRatio='1:1',timeShiftBufferSize=-1)
assert set(seed)==set(fields['values'])and len(seed)==26 and len(fields['rows'])==27
picture={'hue':(-180,180,0),'brightness':(0,2,1),'contrast':(0,2,1),'saturation':(0,3,1),'gamma':(.01,10,1)}
booleans={k for k,v in seed.items()if type(v)is bool}
class VideoPreferences(QObject):
 valuesChanged=Signal();capabilitiesChanged=Signal();choicesChanged=Signal();lastErrorChanged=Signal();restartRequiredChanged=Signal();pickerActiveChanged=Signal()
 def __init__(self):
  super().__init__();self.state=copy.deepcopy(seed);self.caps={k:k!='wallpaper'for k in seed};self.caps['resetPicture']=True;self.error='';self.restart=False;self.picking=False;self.requests=[];self.resets=0;self.reloads=0;self.pickers=0;self.fail_next='';self.rawScreensaver=2;self.next_folder=None;self.during_picker=None
  self.rows={
   'deinterlace':[dict(value=v,name=n)for v,n in[(-1,'Automatic'),(0,'Off'),(1,'On')]],
   'deinterlaceMode':[dict(value=v,name=n)for v,n in[('','Automatic'),('blend','Blend'),('yadif','Yadif')]],
   'aspectRatio':[dict(value=v,name=n)for v,n in[('','Automatic'),('16:9','16:9'),('4:3','4:3'),('fill','Fill')]],
   'crop':[dict(value=v,name=n)for v,n in[('','None'),('16:9','16:9'),('4:3','4:3')]],
   'zoom':[dict(value=v,name=n)for v,n in[('fit','Fit'),('0.25','25%'),('0.5','50%'),('1','100%'),('2','200%')]],
   'position':[dict(value=v,name=n)for v,n in[(0,'Center'),(1,'Left'),(2,'Right'),(4,'Top'),(8,'Bottom')]],
   'snapshotFormat':[dict(value='png',name='PNG'),dict(value='jpg',name='JPG')],
   'snapshotSize':[dict(value=v,name=n)for v,n in[('source','Same as video'),('640x360','640x360'),('1280x720','1280x720'),('1920x1080','1920x1080')]],
   'timeShiftBufferSize':[dict(value=v,name=n)for v,n in[(-1,'Native default (50 MiB per file)'),(16777216,'16 MiB per file'),(52428800,'50 MiB per file'),(104857600,'100 MiB per file')]]}
  self.modes={k:'restart'for k in seed};self.modes.update({k:'nextSnapshot'for k in seed if k.startswith('snapshot')});self.modes.update(enableVideo='nextInput',timeShiftBufferSize='nextInput',resetPicture='restart')
  self.notes=dict(picture='Saved picture defaults use adjust after restart; unrelated filters are preserved.',framing='Fit enables inside-window scaling; numeric zoom turns automatic window scaling off.',timeShiftBufferSize='Maximum bytes per temporary time-shift file. Same native field as Playback; not total buffered duration or disk use.')
 values=Property('QVariantMap',lambda s:s.state,notify=valuesChanged)
 capabilities=Property('QVariantMap',lambda s:s.caps,notify=capabilitiesChanged)
 choices=Property('QVariantMap',lambda s:s.rows,notify=choicesChanged)
 applyModes=Property('QVariantMap',lambda s:s.modes,constant=True)
 supportNotes=Property('QVariantMap',lambda s:{**s.notes,'disableScreensaver':'Existing fullscreen-only policy preserved.'if s.rawScreensaver==2 else'Playback inhibition policy.'},notify=valuesChanged)
 nativeScreensaverMode=Property(int,lambda s:s.rawScreensaver,notify=valuesChanged)
 lastError=Property(str,lambda s:s.error,notify=lastErrorChanged)
 restartRequired=Property(bool,lambda s:s.restart,notify=restartRequiredChanged)
 pickerActive=Property(bool,lambda s:s.picking,notify=pickerActiveChanged)
 def feedback(self,text):self.error=text;self.lastErrorChanged.emit()
 def patch(self,**values):
  assert all(k in seed for k in values);self.state={**self.state,**copy.deepcopy(values)};self.valuesChanged.emit()
  restart=any(self.state[k]!=seed[k]for k in seed if self.modes[k]=='restart')
  if restart!=self.restart:self.restart=restart;self.restartRequiredChanged.emit()
 @Slot()
 def reload(self):assert not self.picking;self.reloads+=1;self.feedback('');self.valuesChanged.emit()
 @Slot(str,'QVariant',result=bool)
 def setValue(self,key,value):
  assert key in seed and self.caps[key]and not self.picking,(key,value);self.requests.append([key,copy.deepcopy(value)])
  valid=True
  if key in booleans:valid=type(value)is bool
  elif key in picture:valid=type(value)in[int,float]and math.isfinite(value)and picture[key][0]<=value<=picture[key][1]
  elif key in self.rows:valid=any(type(value)is type(row['value'])and value==row['value']for row in self.rows[key])
  else:
   valid=type(value)is str
   if key=='snapshotDirectory':valid=valid and(not value or Path(value).is_absolute()and Path(value).is_dir()and os.access(value,os.W_OK))
   if key=='snapshotPrefix':valid=valid and len(value.encode('utf-8'))<=256 and not re.search(r'[\\/:*?"<>|]',value)
   if key in['customAspectRatios','customCropRatios','monitorPixelAspectRatio']:valid=valid and(not value or len(value.split(','))<=16 and all(re.fullmatch(r'[1-9]\d*:[1-9]\d*',v)for v in value.split(',')))
  if not valid:self.feedback('Choose a supported Video value.');return False
  if self.fail_next:self.feedback(self.fail_next);self.fail_next='';return False
  changes={key:value}
  if key=='disableScreensaver':self.rawScreensaver=1 if value else 0
  if key=='zoom':changes['scaleToWindow']=value=='fit'
  if key=='scaleToWindow':changes['zoom']='fit'if value and self.state['zoom']in['fit','1']else'custom'if value else'1'if self.state['zoom']=='fit'else self.state['zoom']
  self.patch(**changes);self.feedback('');return True
 @Slot(result=bool)
 def resetPicture(self):
  assert self.caps['resetPicture']and not self.picking;self.resets+=1
  if self.fail_next:self.feedback(self.fail_next);self.fail_next='';return False
  self.patch(**{k:row[2]for k,row in picture.items()});self.feedback('');return True
 @Slot(result=bool)
 def chooseSnapshotDirectory(self):
  assert self.caps['snapshotDirectory']and not self.picking;self.pickers+=1;self.picking=True;self.pickerActiveChanged.emit()
  if self.during_picker:self.during_picker()
  self.picking=False;self.pickerActiveChanged.emit()
  if self.next_folder is None:return True
  value=self.next_folder;self.next_folder=None;return self.setValue('snapshotDirectory',value)
identities={}
def adapter(relative,name,new_name):
 source=ROOT/relative;identities[relative]=hashlib.sha256(source.read_bytes()).hexdigest();node=next(n for n in ast.parse(source.read_text(encoding='utf-8')).body if isinstance(n,ast.ClassDef)and n.name==name);node.name=new_name;exec(compile(ast.Module(body=[node],type_ignores=[]),str(source),'exec'),globals());return globals()[new_name]
Network=adapter('modules/gui/qt/tests/test_juxtopposed_network_dialog.py','Controller','Network');Playlist=adapter('modules/gui/qt/tests/test_juxtopposed_new_playlist.py','Controller','Playlist');Conversion=adapter('modules/gui/qt/tests/test_juxtopposed_conversion_dialog.py','Controller','Conversion');Provider=adapter('modules/gui/qt/tests/test_juxtopposed_dialogs_host.py','Provider','Provider')
class MainContext(QObject):
 def __init__(self):super().__init__();self.general=General();self.video=VideoPreferences()
 generalPreferences=Property(QObject,lambda s:s.general,constant=True)
 videoPreferences=Property(QObject,lambda s:s.video,constant=True)
 useGlobalShortcuts=Property(bool,lambda s:True,constant=True)
messages=[];qInstallMessageHandler(lambda kind,context,message:messages.append(message))
app=QGuiApplication([]);ctx=MainContext();model=ctx.video;provider=Provider()
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
C.ApplicationWindow{
 id:window;visible:true;width:1080;height:897;property bool omitVideo:false
 QtObject{id:nativePalette;property bool isDark:suppliedContext.generalPreferences.values.theme!==1}
 Binding{target:JuxtopposedPalette;property:"nativePalette";value:nativePalette}
 Binding{target:JuxtopposedPalette;property:"preferences";value:suppliedContext.generalPreferences}
 QtObject{id:generalOnly;property var generalPreferences:suppliedContext.generalPreferences}
 Rectangle{id:underlay;anchors.fill:parent;color:"#173c66"}
 C.Button{id:opener;objectName:"opener";text:"Settings";onClicked:suppliedProvider.showGlobalSettingsDialog()}
 Dialogs.JuxtopposedDialogsHost{anchors.fill:parent;mainCtx:window.omitVideo?generalOnly:suppliedContext;dialogsProvider:suppliedProvider;appearance:JuxtopposedPalette;assetBase:"@ASSETS@";backdropSource:underlay;fallbackFocusItem:opener}
}'''.replace('@QML@',qml.as_uri()).replace('@ASSETS@',assets.as_uri()+'/')
(results/'driver.qml').write_text(driver,encoding='utf-8');engine.loadData(driver.encode('utf-8'),QUrl.fromLocalFile(str(results/'driver.qml')))
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
 expr=QQmlExpression(qmlContext(obj),obj,text);value,_=expr.evaluate();assert not expr.hasError(),expr.error().toString();QTest.qWait(15);return value.toVariant()if hasattr(value,'toVariant')else value
def focus(name):obj=item(name);obj.forceActiveFocus(Qt.TabFocusReason);QTest.qWait(20);return obj
def click(name):
 obj=focus(name);assert obj.isEnabled()and obj.isVisible(),name;QTest.mouseClick(window,Qt.LeftButton,Qt.NoModifier,obj.mapToScene(QPointF(obj.width()/2,obj.height()/2)).toPoint());QTest.qWait(25)
def text(value):
 for char in value:
  for kind in[QEvent.KeyPress,QEvent.KeyRelease]:QGuiApplication.sendEvent(window,QKeyEvent(kind,0,Qt.NoModifier,char))
def edit(name,value):focus(name);QTest.keyClick(window,Qt.Key_A,Qt.ControlModifier);text(value);QTest.keyClick(window,Qt.Key_Tab);QTest.qWait(25)
def select(name,index):
 focus(name);QTest.keyClick(window,Qt.Key_Space);QTest.qWait(20);assert item(name+'Popup').property('visible');QTest.keyClick(window,Qt.Key_Home)
 for _ in range(index):QTest.keyClick(window,Qt.Key_Down)
 QTest.keyClick(window,Qt.Key_Return);QTest.qWait(25)
def done(name):checks.append(name);print('PASS',name,flush=True)
QTest.qWait(70);click('opener');dialog=item('juxtopposedGlobalSettings');assert evaluate(dialog,'chooseCategory("video")');page=item('juxtopposedVideoSettings')
assert page.property('service')is model and dialog.property('preferences')is ctx.general and model.reloads==1 and not model.requests and dialog.property('title')=='Video Defaults'
assert not item('videowallpaper').isEnabled()and page.property('nativeScreensaverMode')==2 and item('videodisableScreensaver').property('selected')
assert window.grabWindow().save(str(results/'01-video-dark-authored.png'))
done('Actual26-value separate Video service and Reset capability mount exact seeds; raw fullscreen-only inhibition and wallpaper false gate preserved')
for key in['enableVideo','startFullscreen','snapshotSequential','snapshotPreview']:
 before=model.state[key];click('video'+key);assert model.requests[-1]==[key,not before]
assert model.rawScreensaver==2
click('videodisableScreensaver');assert model.requests[-1]==['disableScreensaver',False]and model.rawScreensaver==0
click('videodisableScreensaver');assert model.rawScreensaver==1
assert 'next media' in evaluate(page,'helpFor("enableVideo")')and 'next native snapshot' in evaluate(page,'helpFor("snapshotFormat")')
done('Strict bool payloads and exact application timing; unrelated edits never rewrite native inhibition2')
for key in picture:
 before=model.state[key];focus('video'+key);QTest.keyClick(window,Qt.Key_Right);QTest.qWait(25)
 assert model.requests[-1]==[key,round((before+.01)*100)/100]and model.restart
before=dict(model.state);model.fail_next='The owned settings file is locked.';focus('videogamma');QTest.keyClick(window,Qt.Key_Right);QTest.qWait(25)
assert model.state==before and item('videogamma').property('value')==before['gamma']and item('videoSettingsError').isVisible()
done('All five actual sliders submit native hundredth-step scalar values; rejected mixed picture saves resync without false changes')
before=copy.deepcopy(model.state);click('videoResetPicture')
assert item('videoResetConfirmation').property('visible')and item('videoCancelReset').hasActiveFocus()and page.property('busy')and model.resets==0
provider.showGlobalSettingsDialog.emit();QTest.qWait(25);assert item('videoCancelReset').hasActiveFocus()and not item('globalSettingsSearch').hasActiveFocus()
assert not evaluate(dialog,'chooseCategory("general")')
QTest.keyClick(window,Qt.Key_Tab);assert item('videoConfirmReset').hasActiveFocus();QTest.keyClick(window,Qt.Key_Tab);assert item('videoCancelReset').hasActiveFocus()
QTest.keyClick(window,Qt.Key_Escape);QTest.qWait(25);assert not item('videoResetConfirmation').property('visible')and model.state==before and model.resets==0
click('videoResetPicture');click('videoCancelReset');assert model.state==before and model.resets==0
done('Reset confirmation defaults Cancel, traps focus/repeated settings/category changes; Escape and Cancel write nothing')
click('videoResetPicture');click('videoConfirmReset')
assert model.resets==1 and all(model.state[k]==v[2]for k,v in picture.items())
assert all(model.state[k]==before[k]for k in seed if k not in picture)
model.patch(hue=12.5);before=copy.deepcopy(model.state);model.fail_next='The owned settings file is locked.';click('videoResetPicture');click('videoConfirmReset')
assert model.resets==2 and model.state==before and item('videoSettingsError').isVisible()
for key in [*picture,'resetPicture']:model.caps[key]=False
model.capabilitiesChanged.emit();QTest.qWait(20)
assert all(not item('video'+key).isEnabled()for key in picture)and not item('videoResetPicture').isEnabled()
for key in [*picture,'resetPicture']:model.caps[key]=True
model.capabilitiesChanged.emit()
done('Explicit confirmation calls genuine resetPicture once and touches only five values; failure and complex-filter caps keep reset/effects honest')
select('videodeinterlace',1);assert model.requests[-1]==['deinterlace',0]
select('videodeinterlaceMode',2);assert model.requests[-1]==['deinterlaceMode','yadif']and item('videodeinterlaceMode').isEnabled()
select('videoaspectRatio',1);assert model.requests[-1]==['aspectRatio','16:9']
select('videocrop',2);assert model.requests[-1]==['crop','4:3']
select('videoposition',4);assert model.requests[-1]==['position',8]
select('videozoom',4);assert model.requests[-1]==['zoom','2']and not model.state['scaleToWindow']
click('videoscaleToWindow');assert model.state['zoom']=='custom'and item('videozoom').property('currentIndex')==-1
select('videozoom',0);assert model.state['scaleToWindow']and model.state['zoom']=='fit'
done('Deinterlace/method/ratio/alignment native choices submit exact sparse values; Fit/numeric zoom and autoscale coupling comes from provider')
owned=results/'Generated snapshots Åä中';owned.mkdir();folder=owned.resolve().as_posix();edit('videosnapshotDirectory',folder);assert model.requests[-1]==['snapshotDirectory',folder]
prefix='  Generated Å_  ';edit('videosnapshotPrefix',prefix);assert model.requests[-1]==['snapshotPrefix',prefix]and item('videosnapshotPrefix').property('text')==prefix
before=model.state['snapshotPrefix'];edit('videosnapshotPrefix','../unsafe/');assert model.state['snapshotPrefix']==before and item('videosnapshotPrefix').property('text')==before
select('videosnapshotFormat',1);assert model.requests[-1]==['snapshotFormat','jpg']
select('videosnapshotSize',2);assert model.requests[-1]==['snapshotSize','1280x720']
before=dict(model.state);n=len(model.requests);click('videoSnapshotBrowse');assert model.state==before and len(model.requests)==n and model.pickers==1
chosen=results/'Generated selected snapshots';chosen.mkdir();model.next_folder=chosen.resolve().as_posix();click('videoSnapshotBrowse');assert model.state['snapshotDirectory']==chosen.resolve().as_posix()
done('Snapshot folder/prefix literal Unicode and spaces preserved, invalid prefix rolls back, paired size/format and actual picker cancel/select contracts work')
def while_picker():
 QTest.qWait(25);assert page.property('busy')and dialog.property('busy')and not item('videoResetPicture').isEnabled()and not item('videoSnapshotBrowse').isEnabled()
 n=model.reloads;provider.showGlobalSettingsDialog.emit();QTest.qWait(25);assert model.reloads==n and dialog.property('visible')and not evaluate(dialog,'chooseCategory("general")')
 for signal,name in[(provider.showNetworkStreamDialog,'juxtopposedNetworkDialog'),(provider.showNewPlaylistDialog,'juxtopposedNewPlaylistDialog'),(provider.showConversionDialog,'juxtopposedConversionDialog')]:
  signal.emit();QTest.qWait(20);assert dialog.property('visible')and not item(name).property('visible')
 QTest.keyClick(window,Qt.Key_Escape);QTest.qWait(20);assert dialog.property('visible')and item('globalSettingsSearch').property('readOnly')
model.during_picker=while_picker;click('videoSnapshotBrowse');model.during_picker=None
assert not page.property('busy')and item('globalSettingsClose').isEnabled()
done('Native-shaped folder picker nested-loop guards prevent repeat reload, competing dialogs, reset, category edits and Escape')
click('videoAdvanced')
for key,value in [('customAspectRatios','16:9,21:9'),('customCropRatios','4:3,16:9'),('monitorPixelAspectRatio','1:1')]:edit('video'+key,value);assert model.state[key]==value
before=model.state['customCropRatios'];edit('videocustomCropRatios','0:1');assert model.state['customCropRatios']==before and item('videocustomCropRatios').property('text')==before
select('videotimeShiftBufferSize',2);assert model.requests[-1]==['timeShiftBufferSize',52428800]
assert 'not total' in evaluate(page,'helpFor("timeShiftBufferSize")')
done('Expanded genuine ratio fields validate without coercion; shared time-shift choice uses explicit native per-file bytes')
n=len(model.requests);model.patch(aspectRatio='1.85:1',snapshotSize='800x-1',zoom='custom',deinterlaceMode='custom-method')
for key in['aspectRatio','snapshotSize','zoom','deinterlaceMode']:assert item('video'+key).property('currentIndex')==-1,key
model.rows['snapshotFormat']=[dict(value='png',name='PNG')];model.choicesChanged.emit();QTest.qWait(25);assert item('videosnapshotFormat').property('currentIndex')==-1 and len(model.requests)==n
model.caps['snapshotFormat']=False;model.capabilitiesChanged.emit();QTest.qWait(20);assert not item('videosnapshotFormat').isEnabled()
model.caps['snapshotFormat']=True;model.capabilitiesChanged.emit();model.patch(**seed);QTest.qWait(25)
assert not model.restart and not item('videoSettingsRestart').isVisible()
done('Unsupported saved combos and removed encoder choices stay unselected without writes; restoring native-shaped startup fingerprint clears restart')
images=[];fixture=json.loads((ROOT/'modules/gui/qt/tests/fixtures/juxtopposed_video_assets.json').read_text(encoding='utf-8'))
for name,row in fixture.items():assert hashlib.sha256((assets/name).read_bytes()).hexdigest()==row['sha256']
for row in fields['rows']:
 if row['kind']in['picture','reset']:continue
 obj=item('video'+row['key']+'InfoOriginal');assert obj.width()==18 and obj.height()==18 and evaluate(obj,'status')==1;images.append(dict(name=obj.objectName(),width=obj.width(),height=obj.height(),source=obj.property('source').toString()))
for key in['deinterlace','deinterlaceMode','aspectRatio','crop','zoom','position','snapshotFormat','snapshotSize','timeShiftBufferSize']:
 obj=item('video'+key+'OriginalChevron');assert obj.width()==24 and obj.height()==24 and obj.property('mirror')and evaluate(obj,'status')==1;images.append(dict(name=obj.objectName(),width=obj.width(),height=obj.height(),source=obj.property('source').toString()))
for key,count in [('hue',3),('brightness',3),('contrast',3),('saturation',4),('gamma',11)]:
 for i in range(count):
  obj=item('video'+key+'OriginalTick'+str(i));edge=i in[0,count-1];assert [obj.width(),obj.height()]==([32,32]if edge else[2,8])and obj.parentItem().clip()==edge and evaluate(obj,'status')==1;images.append(dict(name=obj.objectName(),width=obj.width(),height=obj.height(),source=obj.property('source').toString()))
for name in ['videoAdvancedOriginalChevron','globalCategoryvideoOriginalIcon']:
 obj=item(name);assert obj.width()==24 and obj.height()==24 and obj.property('mirror')and evaluate(obj,'status')==1;images.append(dict(name=obj.objectName(),width=obj.width(),height=obj.height(),source=obj.property('source').toString()))
assert len(images)==56,len(images)
assert window.grabWindow().save(str(results/'02-video-expanded.png'))
done('All56 authored image instances resolve exact original assets, root dimensions/mirrors and every range endpoint clipping slot')
geometry=[]
for width,height,scale in[(1080,897,1),(640,420,1),(960,600,1.5),(1280,720,2)]:
 window.resize(width,height);dialog.setProperty('uiScale',scale);QTest.qWait(25)
 for name in ['videohue','videogamma','videodeinterlaceMode','videosnapshotDirectory','videosnapshotSize','videocustomAspectRatios','videotimeShiftBufferSize']:
  obj=focus(name);assert obj.hasActiveFocus();point=obj.mapToScene(QPointF(0,0));viewport=item('globalSettingsPageScroll');top=viewport.mapToScene(QPointF(0,0)).y()
  assert point.x()>=-.5 and point.x()+obj.width()<=width+.5 and point.y()>=top-.5 and point.y()+obj.height()<=top+viewport.height()+.5,(name,point,top,viewport.height())
 geometry.append(dict(width=width,height=height,scale=scale));assert item('videogammaOriginalTick5').scale()==scale
 assert window.grabWindow().save(str(results/f'03-video-{width}-{scale}.png'))
done('Real controls reveal within actual host at natural/narrow150/200% and all original artwork scales')
window.resize(1080,897);dialog.setProperty('uiScale',1);page.setProperty('advancedExpanded',False);focus('globalSettingsSearch');text('Custom crop');QTest.qWait(25)
assert item('videocustomCropRatios').isVisible()and not item('videohue').isVisible()
QTest.keyClick(window,Qt.Key_A,Qt.ControlModifier);QTest.keyClick(window,Qt.Key_Backspace);QTest.qWait(25)
focus('videocrop');QTest.keyClick(window,Qt.Key_Space);QTest.qWait(20);assert item('videocropPopup').property('visible')
n=model.reloads;provider.showGlobalSettingsDialog.emit();QTest.qWait(25);assert model.reloads==n and not item('videocropPopup').property('visible')and item('globalSettingsSearch').hasActiveFocus()
ctx.general.patch(theme=1,accentIndex=4);QTest.qWait(25);assert page.property('accent').name()=='#18aad3'and item('videosnapshotPrefix').property('color').name()!='#8f8f8f'and not ctx.general.requests
QTest.keyClick(window,Qt.Key_Escape);QTest.qWait(25);assert not dialog.property('visible')and item('opener').hasActiveFocus()
window.setProperty('omitVideo',True);click('opener');assert not evaluate(dialog,'chooseCategory("video")')and dialog.property('selectedCategory')=='general'
done('Actual search/disclosure, repeat popup focus, shared theme/accent, close/opener and missing separate-service gate preserve General')
# Additive lower-field error regression; original providers/assertions above stay
# unchanged. Return retains input focus while genuine supplied native validation
# or save failure rolls back. A top offscreen banner alone cannot satisfy this.
window.setProperty('omitVideo',False);QTest.qWait(25)
assert evaluate(dialog,'chooseCategory("video")')
page=item('juxtopposedVideoSettings');page.setProperty('advancedExpanded',True)
inline_error_geometry=[]
for field_width,field_height,field_scale in [(1080,897,1),(640,420,1),(960,600,1.5),(1280,720,2)]:
 window.resize(field_width,field_height);dialog.setProperty('uiScale',field_scale);model.patch(monitorPixelAspectRatio='1:1');model.feedback('');QTest.qWait(40)
 field=item('videomonitorPixelAspectRatio');field.setFocus(False);field.forceActiveFocus(Qt.TabFocusReason);QTest.qWait(25)
 before=len(model.requests);QTest.keyClick(window,Qt.Key_A,Qt.ControlModifier);text('invalid-native-value');QTest.keyClick(window,Qt.Key_Return);QTest.qWait(90)
 assert len(model.requests)==before+1 and model.requests[-1]==['monitorPixelAspectRatio','invalid-native-value']
 assert model.state['monitorPixelAspectRatio']=='1:1' and field.property('text')=='1:1' and field.hasActiveFocus()
 assert model.error and item('videoSettingsError').isVisible()and dialog.property('visible')and not dialog.property('busy')
 view=item('globalSettingsPageScroll');top=view.mapToScene(QPointF(0,0));banner=item('videoSettingsError');banner_point=banner.mapToScene(QPointF(0,0))
 tip=field.findChild(QObject,'videomonitorPixelAspectRatioValidationError')
 if tip is None:
  assert window.grabWindow().save(str(results/'missing-inline-feedback.png'))
  (results/'missing-inline-feedback.json').write_text(json.dumps(dict(category='video',field='videomonitorPixelAspectRatio',width=field_width,height=field_height,scale=field_scale,bannerY=banner_point.y(),viewportTop=top.y(),fieldFocused=field.hasActiveFocus(),rolledBack=True,nativeAcceptance=False),indent=2)+'\n',encoding='utf-8')
 assert tip is not None,('Missing in-viewport genuine field feedback',field_width,field_height,field_scale,banner_point,top)
 assert tip.property('visible')and not tip.property('focus')and tip.property('text')==model.error
 box=tip.property('background');position=box.mapToScene(QPointF(0,0));field_position=field.mapToScene(QPointF(0,0))
 assert top.x()-.5<=position.x()and position.x()+box.width()<=top.x()+view.width()+.5 and top.y()-.5<=position.y()and position.y()+box.height()<=top.y()+view.height()+.5,(position,box.width(),box.height(),top,view.width(),view.height())
 assert top.y()-.5<=field_position.y()and field_position.y()+field.height()<=top.y()+view.height()+.5
 inline_error_geometry.append(dict(width=field_width,height=field_height,scale=field_scale,error=[position.x(),position.y(),box.width(),box.height()],viewport=[top.x(),top.y(),view.width(),view.height()],bannerY=banner_point.y(),fieldFocused=field.hasActiveFocus()))
 assert window.grabWindow().save(str(results/f'05-video-inline-error-{field_width}-{field_scale}.png'))
 requests=len(model.requests);reloads=model.reloads;provider.showGlobalSettingsDialog.emit();QTest.qWait(30)
 assert model.reloads==reloads and len(model.requests)==requests and dialog.property('visible')and item('globalSettingsSearch').hasActiveFocus()and not tip.property('visible')
 field.forceActiveFocus(Qt.TabFocusReason);QTest.qWait(30);assert tip.property('visible')and field.hasActiveFocus()
 QTest.keyClick(window,Qt.Key_A,Qt.ControlModifier);text('x');QTest.qWait(30);assert not tip.property('visible')
 QTest.keyClick(window,Qt.Key_A,Qt.ControlModifier);text('4:3');QTest.keyClick(window,Qt.Key_Return);QTest.qWait(70)
 assert model.state['monitorPixelAspectRatio']=='4:3' and field.hasActiveFocus()and not model.error and not tip.property('visible')
 model.fail_next='The owned native settings file is locked.';QTest.keyClick(window,Qt.Key_A,Qt.ControlModifier);text('3:2');QTest.keyClick(window,Qt.Key_Return);QTest.qWait(80)
 assert model.state['monitorPixelAspectRatio']=='4:3' and tip.property('visible')and tip.property('text')==model.error and 'locked' in model.error
 assert model.error in evaluate(field,'Accessible.description')
 model.feedback('');QTest.qWait(40);assert not tip.property('visible')and field.hasActiveFocus()
(results/'inline-error-geometry.json').write_text(json.dumps(inline_error_geometry,indent=2)+'\n',encoding='utf-8')
done('Rejected lower Video field retains rollback/focus and native error inside actual viewport at natural/narrow150/200%; search/edit/success/error-clear lifecycle and save failure preserved')

window.setProperty('omitVideo',False);QTest.qWait(25);assert evaluate(dialog,'chooseCategory("video")');click('videoResetPicture')
before_teardown_resets=model.resets;assert item('videoResetConfirmation').property('visible')and item('videoCancelReset').hasActiveFocus()
destroyed=[];engine.destroyed.connect(lambda:destroyed.append(True));window.close();engine.deleteLater();QCoreApplication.sendPostedEvents(None,QEvent.DeferredDelete);QTest.qWait(25);assert destroyed==[True]and not messages,messages
assert model.resets==before_teardown_resets
done('Actual engine/window teardown with Reset confirmation open dispatches no reset and emits no Qt warning')
(results/'verification.json').write_text(json.dumps(dict(passed=True,caseCount=len(checks),cases=checks,messages=messages,geometry=geometry,renderedImageInstances=images,workerSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),adapters=identities,nativeAccepted=False,scope=__doc__),indent=2)+'\n',encoding='utf-8')
print('PASS actual Video QML with supplied native-shaped provider only.',flush=True)
