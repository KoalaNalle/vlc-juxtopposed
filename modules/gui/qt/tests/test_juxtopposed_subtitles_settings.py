# SPDX-License-Identifier: GPL-2.0-or-later
"""Actual Subtitles defaults category/shared host with strict supplied QObject state.
No native settings, C++ storage, renderer, track/preset application or full-fidelity acceptance.
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
fields=json.loads((ROOT/'modules/gui/qt/tests/fixtures/juxtopposed_subtitles_fields.json').read_text(encoding='utf-8'))
seed=dict(enableSubtitles=True,preferredLanguage='',textEncoding='',autoLoad=True,filenameMatching=3,fontFamily='Arial',sizeMode=0,fontSize=6.25,bold=False,textColor='#FFFFFF',textOpacity=100.,outlineThickness=4.,outlineColor='#000000',shadow=True,shadowColor='#000000',shadowDistance=.06,background='none',backgroundColor='#000000',backgroundOpacity=0.,verticalPosition=-1,subtitleDelay=0,showFeedback=True,feedbackDuration=-1,showFilename=True,filenameDuration=5000,searchFolders='./Subtitles, ./subtitles',ignoreAssStyling=False,shadowAngle=-45.,textDirection=2,monospaceFont='Courier New',visualizationSize='native')
assert set(seed)==set(fields['values'])and len(seed)==31
style_keys={'fontFamily','sizeMode','fontSize','textColor','outlineThickness','outlineColor','background','backgroundOpacity'}
booleans={k for k,v in seed.items()if type(v)is bool}
class Presets(QObject):
 stateChanged=Signal()
 def __init__(self):super().__init__();self.refreshes=0;self.live={'font':'Live edited font','pixelSize':22.5};self.saved={'owned':'Saved preset'};self.calls=[]
 @Slot()
 def refreshDefaults(self):self.refreshes+=1
 @Slot(str,result=bool)
 def apply(self,value):self.calls.append(['apply',value]);raise AssertionError('Opening defaults must never apply to current media')
 @Slot(str,result=bool)
 def save(self,value):self.calls.append(['save',value]);raise AssertionError('Defaults must not save a current-media preset')
class SubtitlesPreferences(QObject):
 valuesChanged=Signal();capabilitiesChanged=Signal();choicesChanged=Signal();lastErrorChanged=Signal();restartRequiredChanged=Signal()
 def __init__(self,presets):
  super().__init__();self.presets=presets;self.state=copy.deepcopy(seed);self.caps={k:k not in['ignoreAssStyling','visualizationSize','verticalPosition','showFeedback','feedbackDuration']for k in seed};self.error='';self.restart=False;self.requests=[];self.reloads=0;self.fail_next='';self.units={'relativeSize':6.25,'pixelSize':18.,'relativeOutline':4.,'pixelOutline':1.};self.preserved={'scale':150,'embedded':True,'durationFactor':2.5,'filters':'owned-filter:subsdelay'}
  self.rows={'preferredLanguage':[dict(value=v,name=n)for v,n in[('','Automatic (native track selection)'),('any','Any available language'),('none','No fallback subtitle'),('eng','English'),('fin','Finnish')]],
   'textEncoding':[dict(value='',name='Automatic'),dict(value='UTF-8',name='UTF-8'),dict(value='UTF-16',name='UTF-16')],
   'filenameMatching':[dict(value=i,name=n)for i,n in enumerate(['No matching','Exact title','Title with extra characters','Contains title','All subtitle files'])],
   'sizeMode':[dict(value=0,name='Relative'),dict(value=1,name='Pixels')],
   'background':[dict(value='none',name='None'),dict(value='box',name='Box')],
   'verticalPosition':[dict(value=v,name=n)for v,n in[(-1,'Native subtitle placement'),(4,'Top'),(0,'Center'),(8,'Bottom')]],
   'textDirection':[dict(value=0,name='Left to right'),dict(value=1,name='Right to left'),dict(value=2,name='Automatic')]}
  self.modes={k:'restart'for k in seed};self.modes.update({k:'nextInput'for k in ['preferredLanguage','textEncoding','autoLoad','filenameMatching','subtitleDelay','searchFolders']});self.modes.update(showFeedback='nextFeedback',feedbackDuration='nextFeedback',backgroundColor='rendererUpdate',ignoreAssStyling='unsupported',visualizationSize='unsupported')
  self.notes=dict(preferredLanguage='Automatic uses native track selection, not operating system language. ISO language lists can preserve a saved order.',appearance='Saved defaults after restart; live appearance, saved presets, embedded style and duration remain separate.',fontSize='Relative percentage of video height, or source video pixels. Inactive unit values are retained.',outlineThickness='Relative percentage of font size or pixels; inactive unit values are retained.',background='None and opacity share the same alpha; Box retains a positive alpha or 128.',backgroundColor='May update currently rendered text and OSD; ASS bitmap styles remain separate.',opacity='Native 0–255 alpha steps; displayed percent reflects quantization.',verticalPosition='Relative primary placement only; absolute ASS, secondary tracks and OSD are unchanged.',subtitleDelay='Milliseconds at 100 ms steps for next media.',feedbackDuration='Native default -1 keeps text 1 s and widgets 1.2 s. Milliseconds for future shortcut messages.',showFeedback='Shortcut-only messages, separate from file names. Existing disabled OSD master is preserved; enable it in Advanced.',searchFolders='Comma-separated local folders; dot-relative folders resolve beside each future media.',ignoreAssStyling='Unavailable: libass bitmap styles cannot be removed by plain-text embedded-format flags.',visualizationSize='Unavailable: canvas derives from actual video/display dimensions.')
 values=Property('QVariantMap',lambda s:s.state,notify=valuesChanged)
 capabilities=Property('QVariantMap',lambda s:s.caps,notify=capabilitiesChanged)
 choices=Property('QVariantMap',lambda s:s.rows,notify=choicesChanged)
 applyModes=Property('QVariantMap',lambda s:s.modes,constant=True)
 supportNotes=Property('QVariantMap',lambda s:s.notes,notify=valuesChanged)
 lastError=Property(str,lambda s:s.error,notify=lastErrorChanged)
 restartRequired=Property(bool,lambda s:s.restart,notify=restartRequiredChanged)
 def feedback(self,text):self.error=text;self.lastErrorChanged.emit()
 def patch(self,**values):
  assert all(k in seed for k in values);self.state={**self.state,**copy.deepcopy(values)};self.valuesChanged.emit()
  restart=any(self.state[k]!=seed[k]for k in seed if self.modes[k]=='restart')
  if restart!=self.restart:self.restart=restart;self.restartRequiredChanged.emit()
 @Slot()
 def reload(self):self.reloads+=1;self.feedback('');self.valuesChanged.emit()
 @Slot(str,'QVariant',result=bool)
 def setValue(self,key,value):
  assert key in seed and self.caps[key],(key,value);self.requests.append([key,copy.deepcopy(value)]);valid=True
  numeric=type(value)in[int,float]and math.isfinite(value)
  if key in booleans:valid=type(value)is bool
  elif key in ['textColor','outlineColor','shadowColor','backgroundColor']:valid=type(value)is str and re.fullmatch(r'#[0-9a-fA-F]{6}',value)is not None
  elif key in ['textOpacity','backgroundOpacity']:valid=numeric and 0<=value<=100
  elif key=='fontSize':valid=numeric and(1 if self.state['sizeMode']==1 else .1)<=value<=(512 if self.state['sizeMode']==1 else 50)
  elif key=='outlineThickness':valid=numeric and 0<=value<=50
  elif key=='shadowDistance':valid=numeric and 0<=value<=1
  elif key=='shadowAngle':valid=numeric and -360<=value<=360
  elif key=='subtitleDelay':valid=numeric and int(value)==value and -5000<=value<=5000 and value%100==0
  elif key=='feedbackDuration':valid=numeric and int(value)==value and 500<=value<=5000
  elif key=='filenameDuration':valid=numeric and int(value)==value and 1000<=value<=10000
  elif key in self.rows:valid=any(type(value)is type(row['value'])and value==row['value']for row in self.rows[key])
  else:
   valid=type(value)is str and not any(ord(c)<32 or ord(c)==127 for c in value)
   if key in ['fontFamily','monospaceFont']:valid=valid and len(value.encode('utf-8'))<=256
   if key=='searchFolders':
    valid=valid and '://'not in value and len(value.encode('utf-8'))<=4096
    if value:
     parts=value.split(',');valid=valid and len(parts)<=16
     for i,part in enumerate(parts):
      path=part.lstrip(' ')if i else part;valid=valid and bool(path)and path==path.strip()and(Path(path).is_absolute()and Path(path).is_dir()or path.startswith('.'))
  if not valid:self.feedback('Use a supported subtitle default.');return False
  if self.fail_next:self.feedback(self.fail_next);self.fail_next='';return False
  changes={key:value}
  if key in ['textColor','outlineColor','shadowColor','backgroundColor']:changes[key]=value.upper()
  if key in ['textOpacity','backgroundOpacity']:changes[key]=math.floor(value*255/100+.5)*100/255
  if key=='background':changes['backgroundOpacity']=0. if value=='none'else self.state['backgroundOpacity']if self.state['backgroundOpacity']>0 else 128*100/255
  if key=='backgroundOpacity':changes['background']='box'if changes[key]>0 else'none'
  if key=='sizeMode':changes['fontSize']=self.units['pixelSize'if value else'relativeSize'];changes['outlineThickness']=self.units['pixelOutline'if value else'relativeOutline']
  if key=='fontSize':self.units['pixelSize'if self.state['sizeMode']else'relativeSize']=value
  if key=='outlineThickness':self.units['pixelOutline'if self.state['sizeMode']else'relativeOutline']=value
  self.patch(**changes);self.presets.refreshDefaults();self.feedback('');return True
identities={}
def adapter(relative,name,new_name):
 source=ROOT/relative;identities[relative]=hashlib.sha256(source.read_bytes()).hexdigest();node=next(n for n in ast.parse(source.read_text(encoding='utf-8')).body if isinstance(n,ast.ClassDef)and n.name==name);node.name=new_name;exec(compile(ast.Module(body=[node],type_ignores=[]),str(source),'exec'),globals());return globals()[new_name]
Network=adapter('modules/gui/qt/tests/test_juxtopposed_network_dialog.py','Controller','Network');Playlist=adapter('modules/gui/qt/tests/test_juxtopposed_new_playlist.py','Controller','Playlist');Conversion=adapter('modules/gui/qt/tests/test_juxtopposed_conversion_dialog.py','Controller','Conversion');Provider=adapter('modules/gui/qt/tests/test_juxtopposed_dialogs_host.py','Provider','Provider')
class MainContext(QObject):
 def __init__(self):super().__init__();self.general=General();self.presets=Presets();self.subtitles=SubtitlesPreferences(self.presets)
 generalPreferences=Property(QObject,lambda s:s.general,constant=True)
 subtitlesPreferences=Property(QObject,lambda s:s.subtitles,constant=True)
 subtitlePresets=Property(QObject,lambda s:s.presets,constant=True)
 useGlobalShortcuts=Property(bool,lambda s:True,constant=True)
messages=[];qInstallMessageHandler(lambda kind,context,message:messages.append(message))
app=QGuiApplication([]);ctx=MainContext();model=ctx.subtitles;provider=Provider()
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
 id:window;visible:true;width:1080;height:897;property bool omitSubtitles:false
 QtObject{id:nativePalette;property bool isDark:suppliedContext.generalPreferences.values.theme!==1}
 Binding{target:JuxtopposedPalette;property:"nativePalette";value:nativePalette}
 Binding{target:JuxtopposedPalette;property:"preferences";value:suppliedContext.generalPreferences}
 QtObject{id:generalOnly;property var generalPreferences:suppliedContext.generalPreferences}
 Rectangle{id:underlay;anchors.fill:parent;color:"#173c66"}
 C.Button{id:opener;objectName:"opener";text:"Settings";onClicked:suppliedProvider.showGlobalSettingsDialog()}
 Dialogs.JuxtopposedDialogsHost{anchors.fill:parent;mainCtx:window.omitSubtitles?generalOnly:suppliedContext;dialogsProvider:suppliedProvider;appearance:JuxtopposedPalette;assetBase:"@ASSETS@";backdropSource:underlay;fallbackFocusItem:opener}
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
QTest.qWait(70);click('opener');dialog=item('juxtopposedGlobalSettings');assert evaluate(dialog,'chooseCategory("subtitles")');page=item('juxtopposedSubtitlesSettings')
assert page.property('service')is model and dialog.property('preferences')is ctx.general and model.reloads==1 and not model.requests and not ctx.presets.refreshes and not ctx.presets.calls
assert dialog.property('title')=='Subtitle Defaults'and len(model.state)==31
for key in ['ignoreAssStyling','visualizationSize','verticalPosition','showFeedback','feedbackDuration']:assert not item('subtitles'+key).isEnabled(),key
assert not item('subtitlessearchFolders').isVisible()and item('subtitlesfontSizeInput').property('text')=='6.25'
assert window.grabWindow().save(str(results/'01-subtitles-dark-authored.png'))
done('Actual31-value separate service mounts seeds without saving, refreshing presets or applying current appearance; unsupported/native consumer gates remain honest')
for key in ['enableSubtitles','autoLoad','bold','shadow','showFilename']:
 before=model.state[key];click('subtitles'+key);assert model.requests[-1]==[key,not before]
for key in ['verticalPosition','showFeedback','feedbackDuration']:model.caps[key]=True
model.capabilitiesChanged.emit();QTest.qWait(25)
click('subtitlesshowFeedback');assert model.requests[-1]==['showFeedback',False]
assert item('subtitlesfeedbackDuration').isEnabled()and item('subtitlesfilenameDuration').isEnabled()
done('Strict bool payloads and explicitly supplied verified capability states; duration preparation remains independent of visibility switches')
select('subtitlespreferredLanguage',4);assert model.requests[-1]==['preferredLanguage','fin']
select('subtitlestextEncoding',1);assert model.requests[-1]==['textEncoding','UTF-8']
select('subtitlesfilenameMatching',4);assert model.requests[-1]==['filenameMatching',4]
select('subtitlesverticalPosition',1);assert model.requests[-1]==['verticalPosition',4]
select('subtitlesverticalPosition',2);assert model.requests[-1]==['verticalPosition',0]
assert 'next media' in evaluate(page,'helpFor("preferredLanguage")')and 'operating system' in evaluate(page,'helpFor("preferredLanguage")')
done('Actual language/encoding/matching/native sparse placement choices submit their real values with honest next-input language semantics')
preserved=copy.deepcopy(model.preserved);inactive=copy.deepcopy(model.units)
click('subtitlesSizeModePixels');assert model.state['sizeMode']==1 and model.state['fontSize']==18. and model.units['relativeSize']==inactive['relativeSize']
edit('subtitlesfontSizeInput','22.5');assert model.state['fontSize']==22.5 and model.units['relativeSize']==inactive['relativeSize']
edit('subtitlesfontSizeInput','0.1');assert model.state['fontSize']==22.5 and item('subtitlesfontSizeInput').property('text')=='22.5'
click('subtitlesSizeModeRelative');assert model.state['fontSize']==6.25
edit('subtitlesfontSizeInput','0.1');assert model.requests[-1]==['fontSize',.1]and model.state['fontSize']==.1
edit('subtitlesoutlineThicknessInput','0.125');assert model.state['outlineThickness']==.125 and model.units['pixelOutline']==inactive['pixelOutline']
import struct
expanded=struct.unpack('f',struct.pack('f',7.2))[0];model.patch(fontSize=expanded);before=len(model.requests);focus('subtitlesfontSizeInput');QTest.keyClick(window,Qt.Key_Tab);assert len(model.requests)==before and model.state['fontSize']==expanded
assert model.preserved==preserved and ctx.presets.live=={'font':'Live edited font','pixelSize':22.5}and ctx.presets.saved=={'owned':'Saved preset'}and not ctx.presets.calls
done('Editable numeric defaults accept genuine decimal bounds, preserve inactive units and legacy style state; focus alone never rounds/saves native float or applies live presets')
edit('subtitlesfontFamily','Owned Åä中 family');assert model.state['fontFamily']=='Owned Åä中 family'
for key,value in [('textColor','#12aBcD'),('outlineColor','#203040'),('shadowColor','#102030'),('backgroundColor','#405060')]:
 edit('subtitles'+key,value);assert model.state[key]==value.upper()and item('subtitles'+key+'Swatch').property('color').name()==value.lower()
before=model.state['textColor'];edit('subtitlestextColor','#11223344');assert model.state['textColor']==before and item('subtitlestextColor').property('text')==before and item('subtitlesSettingsError').isVisible()
done('UTF8 font families and exact RGB strings drive real swatches; invalid color rollback never invents alpha or touches live presets')
focus('subtitlestextOpacity');QTest.keyClick(window,Qt.Key_Left);QTest.qWait(25);assert model.requests[-1][0]=='textOpacity'and math.isclose(model.state['textOpacity']*255/100,round(model.state['textOpacity']*255/100))
select('subtitlesbackground',1);assert model.state['background']=='box'and model.state['backgroundOpacity']>0
focus('subtitlesbackgroundOpacity');QTest.keyClick(window,Qt.Key_Home);QTest.qWait(25);assert model.state['backgroundOpacity']==0 and model.state['background']=='none'
assert item('subtitlesbackgroundColor').isEnabled()and item('subtitlesshadowColor').isEnabled()and item('subtitlesshadowDistanceInput').isEnabled()
edit('subtitlesshadowDistanceInput','0.1');assert model.requests[-1]==['shadowDistance',.1]
done('Opacity reflects native alpha quantization and shared background alpha; independent shadow/background parameters remain preparable while off')
focus('subtitlessubtitleDelay');QTest.keyClick(window,Qt.Key_Right);QTest.qWait(25);assert model.requests[-1]==['subtitleDelay',100]
assert model.state['feedbackDuration']==-1 and not evaluate(item('subtitlesfeedbackDuration'),'handle.visible')
before=len(model.requests);focus('subtitlesfeedbackDuration');QTest.keyClick(window,Qt.Key_Tab);assert len(model.requests)==before and model.state['feedbackDuration']==-1
focus('subtitlesfeedbackDuration');QTest.keyClick(window,Qt.Key_End);QTest.qWait(25);assert model.requests[-1]==['feedbackDuration',5000]
focus('subtitlesfilenameDuration');QTest.keyClick(window,Qt.Key_End);QTest.qWait(25);assert model.requests[-1]==['filenameDuration',10000]
assert 'milliseconds' in evaluate(page,'helpFor("subtitleDelay")').lower()and 'next shortcut' in evaluate(page,'helpFor("feedbackDuration")')
done('Delay is integral100ms and duration fields are milliseconds; native default sentinel stays unchanged and has no fabricated thumb until an explicit choice')
before=copy.deepcopy(model.state);units=copy.deepcopy(model.units);refreshes=ctx.presets.refreshes;model.fail_next='The owned settings file is locked.';edit('subtitlesfontSizeInput','12.5')
assert model.state==before and model.units==units and ctx.presets.refreshes==refreshes and item('subtitlesSettingsError').isVisible()and item('subtitlesfontSizeInput').property('text')==str(before['fontSize'])
edit('subtitlesfontSizeInput','12.5');assert model.state['fontSize']==12.5 and not model.error and model.restart and item('subtitlesSettingsRestart').isVisible()
done('Rejected mixed style saves roll back all supplied values/units and do not refresh presets; retry clears error only after accepted save')
before=len(model.requests);focus('subtitlesfontSizeInput');QTest.keyClick(window,Qt.Key_Down);QTest.qWait(20);assert item('subtitlesfontSizePopup').property('visible')and len(model.requests)==before
QTest.keyClick(window,Qt.Key_Home);QTest.keyClick(window,Qt.Key_Down);QTest.keyClick(window,Qt.Key_Return);QTest.qWait(25);assert model.requests[-1]==['fontSize',6.25]and model.units['pixelSize']==22.5
select('subtitlesoutlineThickness',1);assert model.requests[-1]==['outlineThickness',2]
select('subtitlesshadowDistance',3);assert model.requests[-1]==['shadowDistance',.5]
before=len(model.requests);click('subtitlesfontSizeMenuButton');assert item('subtitlesfontSizePopup').property('visible')and len(model.requests)==before
click('subtitlesfontSizeOption2');assert model.requests[-1]==['fontSize',12.5]and len(model.requests)==before+1 and item('subtitlesfontSizeInput').hasActiveFocus()
before=len(model.requests);click('subtitlesfontSizeMenuButton');QTest.keyClick(window,Qt.Key_Escape);QTest.qWait(25);assert not item('subtitlesfontSizePopup').property('visible')and len(model.requests)==before and item('subtitlesfontSizeInput').hasActiveFocus()
assert not any(row['name']=='Automatic'for row in evaluate(item('subtitlesfontSize'),'options'))
done('Editable numeric dropdowns open without writes, accept genuine scalar convenience selections and preserve inactive units without inventing Automatic')
click('subtitlesAdvanced');owned=results/'Generated subtitles Åä中';owned.mkdir();folders=owned.resolve().as_posix()+', ./subtitles';edit('subtitlessearchFolders',folders);assert model.requests[-1]==['searchFolders',folders]
before=model.state['searchFolders'];edit('subtitlessearchFolders','https://example.invalid/subs');assert model.state['searchFolders']==before and item('subtitlessearchFolders').property('text')==before
edit('subtitlesshadowAngle','-90.5');assert model.requests[-1]==['shadowAngle',-90.5]
select('subtitlestextDirection',1);assert model.requests[-1]==['textDirection',1]
edit('subtitlesmonospaceFont','Owned Mono Å');assert model.state['monospaceFont']=='Owned Mono Å'
assert not item('subtitlesignoreAssStyling').isEnabled()and not item('subtitlesvisualizationSize').isEnabled()
assert 'libass' in evaluate(page,'helpFor("ignoreAssStyling")')and 'canvas' in evaluate(page,'helpFor("visualizationSize")')
done('Six genuine advanced rows preserve literal local-folder grammar/UTF8, numeric angle and native direction; ASS and canvas gaps stay disabled without fake picker/actions')
images=[];fixture=json.loads((ROOT/'modules/gui/qt/tests/fixtures/juxtopposed_subtitles_assets.json').read_text(encoding='utf-8'))
for name,row in fixture.items():assert hashlib.sha256((assets/name).read_bytes()).hexdigest()==row['sha256']
for row in fields['rows']:
 if row['infoNode']:
  obj=item('subtitles'+row['key']+'InfoOriginal');assert obj.width()==18 and obj.height()==18 and evaluate(obj,'status')==1;images.append(dict(name=obj.objectName(),width=obj.width(),height=obj.height(),source=obj.property('source').toString()))
 if row['kind']in['choice','numericChoice']:
  obj=item('subtitles'+row['key']+'OriginalChevron');assert obj.width()==24 and obj.height()==24 and obj.property('mirror')and evaluate(obj,'status')==1;images.append(dict(name=obj.objectName(),width=obj.width(),height=obj.height(),source=obj.property('source').toString()))
 for i in range(len(row['tickNodes'])):
  obj=item('subtitles'+row['key']+'OriginalTick'+str(i));edge=i in[0,len(row['tickNodes'])-1];assert[obj.width(),obj.height()]==([32,32]if edge else[2,8])and obj.parentItem().clip()==edge and evaluate(obj,'status')==1;images.append(dict(name=obj.objectName(),width=obj.width(),height=obj.height(),source=obj.property('source').toString()))
for name,mirror in [('subtitlesAdvancedOriginalChevron',True),('globalCategorysubtitlesOriginalIcon',False)]:
 obj=item(name);assert obj.width()==24 and obj.height()==24 and obj.property('mirror')==mirror and evaluate(obj,'status')==1;images.append(dict(name=obj.objectName(),width=obj.width(),height=obj.height(),source=obj.property('source').toString()))
assert len(images)==sum(bool(r['infoNode'])+(r['kind']in['choice','numericChoice'])+len(r['tickNodes'])for r in fields['rows'])+2
assert window.grabWindow().save(str(results/'02-subtitles-expanded.png'))
done('Every authored mounted info/choice/tick/chevron/selected slot loads exact original SVG bytes, dimensions, transforms and endpoint clipping')
geometry=[]
for width,height,scale in[(1080,897,1),(640,420,1),(960,600,1.5),(1280,720,2)]:
 window.resize(width,height);dialog.setProperty('uiScale',scale);QTest.qWait(25)
 for name in ['subtitlesfontFamily','subtitlesSizeModePixels','subtitlesfontSizeInput','subtitlesbackgroundColor','subtitlessubtitleDelay','subtitlessearchFolders','subtitlesshadowAngle','subtitlesmonospaceFont']:
  obj=focus(name);assert obj.hasActiveFocus();point=obj.mapToScene(QPointF(0,0));viewport=item('globalSettingsPageScroll');top=viewport.mapToScene(QPointF(0,0)).y();assert point.x()>=-.5 and point.x()+obj.width()<=width+.5 and point.y()>=top-.5 and point.y()+obj.height()<=top+viewport.height()+.5,(name,point,top,viewport.height())
 geometry.append(dict(width=width,height=height,scale=scale));assert item('subtitlestextOpacityOriginalTick5').scale()==scale;assert window.grabWindow().save(str(results/f'03-subtitles-{width}-{scale}.png'))
done('Real input/segmented/range focus reveals within actual shared host at natural/narrow150/200percent and original artwork scales')
window.resize(1080,897);dialog.setProperty('uiScale',1);page.setProperty('advancedExpanded',False);focus('globalSettingsSearch');text('Monospace');QTest.qWait(25);assert item('subtitlesmonospaceFont').isVisible()and not item('subtitlesfontFamily').isVisible()
QTest.keyClick(window,Qt.Key_A,Qt.ControlModifier);QTest.keyClick(window,Qt.Key_Backspace);QTest.qWait(25)
focus('subtitlestextEncoding');QTest.keyClick(window,Qt.Key_Space);QTest.qWait(20);assert item('subtitlestextEncodingPopup').property('visible');n=model.reloads;provider.showGlobalSettingsDialog.emit();QTest.qWait(25);assert model.reloads==n and not item('subtitlestextEncodingPopup').property('visible')and item('globalSettingsSearch').hasActiveFocus()
ctx.general.patch(theme=1,accentIndex=4);QTest.qWait(25);assert page.property('accent').name()=='#18aad3'and item('subtitlesfontFamily').property('color').name()!='#8f8f8f'and not ctx.general.requests
done('Actual search/disclosure/repeated-popup focus and shared theme/accent retain General and perform no implicit subtitle or preset writes')
n=len(model.requests);model.patch(preferredLanguage='legacy-unknown',verticalPosition=2,sizeMode=2)
for key in style_keys:model.caps[key]=False
model.caps['showFeedback']=False;model.caps['showFilename']=False;model.capabilitiesChanged.emit();QTest.qWait(25)
assert item('subtitlespreferredLanguage').property('currentIndex')==-1 and item('subtitlesverticalPosition').property('currentIndex')==-1 and not item('subtitlesSizeModeRelative').isEnabled()and not item('subtitlesSizeModePixels').isEnabled()
assert not item('subtitlesshowFeedback').isEnabled()and not item('subtitlesshowFilename').isEnabled()and len(model.requests)==n
QTest.keyClick(window,Qt.Key_Escape);QTest.qWait(25);assert not dialog.property('visible')and item('opener').hasActiveFocus()
window.setProperty('omitSubtitles',True);click('opener');assert not evaluate(dialog,'chooseCategory("subtitles")')and dialog.property('selectedCategory')=='general'and not ctx.presets.calls
done('Unknown saved combinations and invalid legacy-style/OSD master gates remain unselected or disabled; missing explicit service cannot fall back to General')
window.setProperty('omitSubtitles',False);QTest.qWait(25);assert evaluate(dialog,'chooseCategory("subtitles")')
model.patch(sizeMode=0,fontSize=6.25)
for key in style_keys:model.caps[key]=True
model.capabilitiesChanged.emit();QTest.qWait(25);click('subtitlesfontSizeMenuButton');assert item('subtitlesfontSizePopup').property('visible');before_teardown=len(model.requests)
destroyed=[];engine.destroyed.connect(lambda:destroyed.append(True));window.close();engine.deleteLater();QCoreApplication.sendPostedEvents(None,QEvent.DeferredDelete);QTest.qWait(25);assert destroyed==[True]and not messages,messages
assert len(model.requests)==before_teardown and not ctx.presets.calls
done('Actual window/engine teardown with numeric menu open retains warning storage through destruction and dispatches no settings/preset/current-media action')
(results/'verification.json').write_text(json.dumps(dict(passed=True,caseCount=len(checks),cases=checks,messages=messages,geometry=geometry,renderedImageInstances=images,workerSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),adapters=identities,nativeAccepted=False,scope=__doc__),indent=2)+'\n',encoding='utf-8')
print('PASS actual Subtitles QML with strict supplied native-shaped service only.',flush=True)
