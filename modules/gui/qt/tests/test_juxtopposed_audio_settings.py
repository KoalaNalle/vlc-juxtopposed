# SPDX-License-Identifier: GPL-2.0-or-later
"""Actual Audio category/host QML with a strict supplied native-shaped contract.
No compiled audio output, OS device, DSP, profile, restart or native-picker acceptance.
"""
from pathlib import Path
import argparse,ast,hashlib,json,math,os,shutil,sys,re,copy,struct
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
fields=json.loads((ROOT/'modules/gui/qt/tests/fixtures/juxtopposed_audio_fields.json').read_text(encoding='utf-8'))
seed=dict(audioEnabled=True,outputDevice='',speakerSetup=0,maximumVolume=200,rememberVolume=True,startupVolume=100.0,volumeStep=1.0,
 normalizeLoudness=False,normalizationTarget=1.0,nightMode=None,replayGainMode='none',replayGainPreamp=0.0,
 equalizerEnabled=False,equalizerPreset='',equalizerTwoPass=False,equalizerPreamp=12.0,equalizerBands=[0.0]*10,
 spatialAudio=None,spatialAudioMode=None,stereoWidening=False,compressor=False,pitchShift=0.0,chorusFlanger=False,karaoke=False,
 preferredLanguage='',audioDelayMs=0,visualizer='none',passthrough=False,detectDolby=0,resamplingQuality=None,mediaRole='video',visualizationSize=dict(width=800,height=500))
assert set(seed)==set(fields['values'])and len(seed)==32 and len(fields['rows'])==29
unavailable={'nightMode','spatialAudio','spatialAudioMode','visualizationSize','mediaRole'}
gated={'outputDevice','startupVolume','passthrough','volumeStep'}
booleans={k for k,v in seed.items()if type(v)is bool}
native=(ROOT/'modules/audio_filter/equalizer_presets.h').read_text(encoding='utf-8')
names=re.findall(r'"([^"]+)"',re.search(r'preset_list\[NB_PRESETS\]\s*=\s*\{(.*?)\};',native,re.S).group(1))
labels=re.findall(r'N_\("([^"]+)"\)',re.search(r'preset_list_text\[NB_PRESETS\]\s*=\s*\{(.*?)\};',native,re.S).group(1))
preset_rows=re.findall(r'\{\s*"([^"]+)",\s*10,\s*([-+.\deEf]+),\s*\{([^}]+)\}',native,re.S)
presets={name:dict(preamp=float(preamp.rstrip('f')),bands=[float(v.strip().rstrip('f'))for v in values.split(',')if v.strip()])for name,preamp,values in preset_rows}
assert len(names)==len(labels)==len(presets)==18
class AudioPreferences(QObject):
    valuesChanged=Signal();capabilitiesChanged=Signal();optionsChanged=Signal();devicesChanged=Signal();equalizerFrequenciesChanged=Signal();lastErrorChanged=Signal();restartRequiredChanged=Signal()
    def __init__(self):
        super().__init__();self.state=copy.deepcopy(seed);self.caps={k:k not in unavailable|gated for k in seed};self.error='';self.restart=False;self.requests=[];self.reloads=0;self.fail_next='';self.raw_passthrough=0;self.rejected_meta_types=[]
        self.device_rows=[];self.freqs=[60,170,310,600,1000,3000,6000,12000,14000,16000]
        self.rows={
          'speakerSetup':[dict(value=v,name=n)for v,n in[(0,'Automatic'),(1,'Stereo'),(3,'4.0'),(4,'5.1'),(5,'7.1')]],
          'equalizerPreset':[dict(value=v,name=n)for v,n in zip(names,labels)],
          'replayGainMode':[dict(value=v,name=n)for v,n in[('none','Off'),('track','Track'),('album','Album')]],
          'preferredLanguage':[dict(value=v,name=n)for v,n in[('','Automatic'),('any','Any language'),('none','No audio track'),('en','English'),('fi','Finnish')]],
          'visualizer':[dict(value=v,name=n)for v,n in[('none','Off'),('spectrum','Spectrum'),('vuMeter','VU meter')]],
          'detectDolby':[dict(value=v,name=n)for v,n in[(0,'Automatic'),(1,'On'),(2,'Off')]],
          'resamplingQuality':[dict(value=i,name=n)for i,n in enumerate(['Quick cubic interpolation','Low 16-bit','Balanced (medium 16-bit)','High quality','Very high quality'])],
          'mediaRole':[dict(value='video',name='Video'),dict(value='music',name='Music')]}
        self.bounds={k:dict(minimum=a,maximum=b)for k,a,b in [('maximumVolume',60,300),('startupVolume',0,125),('volumeStep',100/256,100),('normalizationTarget',.000001,3.4028234663852886e38),('replayGainPreamp',-20,20),('equalizerPreamp',-20,20),('equalizerBands',-20,20),('pitchShift',-12,12),('audioDelayMs',-2147483648,2147483647)]}
    values=Property('QVariantMap',lambda s:s.state,notify=valuesChanged)
    capabilities=Property('QVariantMap',lambda s:s.caps,notify=capabilitiesChanged)
    options=Property('QVariantMap',lambda s:s.rows,notify=optionsChanged)
    devices=Property('QVariantList',lambda s:s.device_rows,notify=devicesChanged)
    equalizerFrequencies=Property('QVariantList',lambda s:s.freqs,notify=equalizerFrequenciesChanged)
    ranges=Property('QVariantMap',lambda s:s.bounds,constant=True)
    lastError=Property(str,lambda s:s.error,notify=lastErrorChanged)
    restartRequired=Property(bool,lambda s:s.restart,notify=restartRequiredChanged)
    def feedback(self,text):self.error=text;self.lastErrorChanged.emit()
    def patch(self,**values):
        assert all(k in seed for k in values);self.state={**self.state,**copy.deepcopy(values)};self.valuesChanged.emit()
        # The native fingerprint compares effective bands/preamp and excludes
        # the redundant preset name once effective bands are valid.
        restart=any(self.state[k]!=seed[k]for k in seed if k not in ('equalizerPreset','volumeStep','audioDelayMs'))
        if restart!=self.restart:self.restart=restart;self.restartRequiredChanged.emit()
    def enable_consumers(self):
        for k in gated:self.caps[k]=True
        self.capabilitiesChanged.emit()
        self.device_rows=[dict(value='',name='System default'),dict(value='generated-endpoint-A',name='Generated device A'),dict(value='generated-endpoint-B',name='Generated device B')];self.devicesChanged.emit()
    @Slot()
    def reload(self):self.reloads+=1;self.feedback('');self.valuesChanged.emit()
    @Slot(str,'QVariant',result=bool)
    def setValue(self,key,value):
        assert key in seed and self.caps[key],(key,value)
        if key=='equalizerBands'and type(value)is not list:
            self.rejected_meta_types.append(type(value).__name__);self.feedback('Provide exactly ten equalizer gains.');return False
        self.requests.append([key,copy.deepcopy(value)])
        valid=True
        if key in booleans:valid=type(value)is bool
        elif key=='equalizerBands':valid=type(value)is list and len(value)==10 and all(type(v)in[int,float]and math.isfinite(v)and -20<=v<=20 for v in value)
        elif key in self.bounds:
            bound=self.bounds[key];valid=type(value)in[int,float]and math.isfinite(value)and bound['minimum']<=value<=bound['maximum']
            if key in['maximumVolume','audioDelayMs']:valid=valid and value==int(value)
        elif key=='outputDevice':valid=type(value)is str and any(r['value']==value for r in self.device_rows)
        elif key=='preferredLanguage':valid=type(value)is str and len(value.encode('utf-8'))<=256 and all(v in['en','fi','eng','fin','any','none']for v in value.split(','))if value else value==''
        elif key in self.rows:valid=any(type(value)is type(r['value'])and value==r['value']for r in self.rows[key])
        if not valid:self.feedback('Choose a supported Audio value.');return False
        if self.fail_next:self.feedback(self.fail_next);self.fail_next='';return False
        changes={key:value}
        if key=='equalizerPreset':changes.update(equalizerPreamp=presets[value]['preamp'],equalizerBands=presets[value]['bands'])
        if key in['equalizerPreamp','equalizerBands']:changes['equalizerPreset']=''
        if key=='passthrough':self.raw_passthrough=(1 if self.raw_passthrough==1 else 2)if value else 0
        self.patch(**changes);self.feedback('');return True
identities={}
def adapter(relative,name,new_name):
    source=ROOT/relative;identities[relative]=hashlib.sha256(source.read_bytes()).hexdigest();node=next(n for n in ast.parse(source.read_text(encoding='utf-8')).body if isinstance(n,ast.ClassDef)and n.name==name);node.name=new_name;exec(compile(ast.Module(body=[node],type_ignores=[]),str(source),'exec'),globals());return globals()[new_name]
Network=adapter('modules/gui/qt/tests/test_juxtopposed_network_dialog.py','Controller','Network')
Playlist=adapter('modules/gui/qt/tests/test_juxtopposed_new_playlist.py','Controller','Playlist')
Conversion=adapter('modules/gui/qt/tests/test_juxtopposed_conversion_dialog.py','Controller','Conversion')
Provider=adapter('modules/gui/qt/tests/test_juxtopposed_dialogs_host.py','Provider','Provider')
class MainContext(QObject):
    def __init__(self):super().__init__();self.general=General();self.audio=AudioPreferences()
    generalPreferences=Property(QObject,lambda s:s.general,constant=True)
    audioPreferences=Property(QObject,lambda s:s.audio,constant=True)
    useGlobalShortcuts=Property(bool,lambda s:True,constant=True)
messages=[];qInstallMessageHandler(lambda kind,context,message:messages.append(message))
app=QGuiApplication([]);ctx=MainContext();model=ctx.audio;provider=Provider()
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
 property bool omitAudio:false
 QtObject{id:nativePalette;property bool isDark:suppliedContext.generalPreferences.values.theme!==1}
 Binding{target:JuxtopposedPalette;property:"nativePalette";value:nativePalette}
 Binding{target:JuxtopposedPalette;property:"preferences";value:suppliedContext.generalPreferences}
 QtObject{id:generalOnly;property var generalPreferences:suppliedContext.generalPreferences}
 Rectangle{id:underlay;anchors.fill:parent;color:"#173c66"}
 C.Button{id:opener;objectName:"opener";text:"Settings";onClicked:suppliedProvider.showGlobalSettingsDialog()}
 Dialogs.JuxtopposedDialogsHost{anchors.fill:parent;mainCtx:window.omitAudio?generalOnly:suppliedContext;dialogsProvider:suppliedProvider;appearance:JuxtopposedPalette;assetBase:"@ASSETS@";backdropSource:underlay;fallbackFocusItem:opener}
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
def focus(name):
    obj=item(name);obj.forceActiveFocus(Qt.TabFocusReason);QTest.qWait(20);return obj
def click(name):
    obj=focus(name);assert obj.isEnabled()and obj.isVisible(),name;QTest.mouseClick(window,Qt.LeftButton,Qt.NoModifier,obj.mapToScene(QPointF(obj.width()/2,obj.height()/2)).toPoint());QTest.qWait(25)
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
QTest.qWait(70);click('opener');dialog=item('juxtopposedGlobalSettings');assert evaluate(dialog,'chooseCategory("audio")');page=item('juxtopposedAudioSettings')
assert page.property('service')is model and dialog.property('preferences')is ctx.general and dialog.property('title')=='Audio Defaults'and model.reloads==1 and not model.requests
for key in unavailable|gated:assert not item('audio'+key).isEnabled(),key
assert len(evaluate(page,'settingKeys'))==32 and item('audioequalizerPreset').property('currentIndex')==-1
assert page.property('busy')is False and not hasattr(model,'pickerActive')
assert window.grabWindow().save(str(results/'01-audio-dark-authored.png'))
done('Actual explicit32-value Audio service mounts all29rows; unsupported/native-consumer gates, Custom EQ and exact seeds retained without writes')
model.enable_consumers();QTest.qWait(25)
select('audiooutputDevice',2);assert model.requests[-1]==['outputDevice','generated-endpoint-B']
select('audiospeakerSetup',2);assert model.requests[-1]==['speakerSetup',3]
select('audioreplayGainMode',2);assert model.requests[-1]==['replayGainMode','album']
select('audiopreferredLanguage',4);assert model.requests[-1]==['preferredLanguage','fi']
select('audiovisualizer',1);assert model.requests[-1]==['visualizer','spectrum']
done('Actual device/native options submit exact IDs/strings and sparse speaker enums, not option indices')
for key in['audioEnabled','normalizeLoudness','equalizerEnabled','equalizerTwoPass','stereoWidening','compressor','chorusFlanger','karaoke']:
    before=model.state[key];click('audio'+key);assert model.requests[-1]==[key,not before]
# Parameters stay editable with their effects off.
model.patch(equalizerEnabled=False,normalizeLoudness=False)
for key,value in [('normalizationTarget','1.25'),('replayGainPreamp','-3.5'),('pitchShift','2.5'),('volumeStep','1.25'),('audioDelayMs','-125')]:
    edit('audio'+key,value);assert model.requests[-1]==[key,float(value)]
assert item('audionormalizationTarget').isEnabled()and item('audioequalizerPreset').isEnabled()
done('Strict boolean and numeric fields stay independently editable while effects are off; explicit milliseconds and semitones preserved')
edit('audiostartupVolume','125%');assert model.requests[-1]==['startupVolume',125]and model.state['rememberVolume']is True
click('audioFixedVolume');assert model.requests[-1]==['rememberVolume',False]
click('audiorememberVolume');assert model.requests[-1]==['rememberVolume',True]
before=model.state['startupVolume'];edit('audiostartupVolume','200%');assert model.state['startupVolume']==before and item('audiostartupVolume').property('text')=='125%'and item('audioSettingsError').isVisible()
done('Startup remember/fixed policy is separate from prepared0–125% number; authored200% is genuinely rejected and rolled back')
before=len(model.requests);model.patch(maximumVolume=75);QTest.qWait(20)
assert not item('audioMaximumThumb').isVisible()and len(model.requests)==before
focus('audiomaximumVolume');QTest.keyClick(window,Qt.Key_Right);QTest.qWait(25)
assert model.requests[-1]==['maximumVolume',101]
model.fail_next='The owned settings file is locked.';before=model.state['maximumVolume'];focus('audiomaximumVolume');QTest.keyClick(window,Qt.Key_Right);QTest.qWait(25)
assert model.state['maximumVolume']==before and item('audiomaximumVolume').property('value')==before
done('Saved60–99% remains unchanged/visible outside authored maximum-volume scale; explicit slider edits and failed-save rollback are real')
select('audioequalizerPreset',13);assert model.requests[-1]==['equalizerPreset','rock']and model.state['equalizerBands']==presets['rock']['bands']
before=copy.deepcopy(model.state)
assert not evaluate(page,'service.setValue("equalizerBands",[0,0,0,0,0,0,0,0,0,0])')
assert model.rejected_meta_types==['QJSValue']and model.state==before
done('Negative control rejects a plain JS array at the real QObject QVariant boundary; the fixture never normalizes it to pass')
click('audioOpenEqualizer');assert page.property('equalizerExpanded')
focus('audioEqGain1');QTest.keyClick(window,Qt.Key_Up);QTest.qWait(25)
assert model.requests[-1][0]=='equalizerBands'and type(model.requests[-1][1])is list and len(model.requests[-1][1])==10 and model.state['equalizerPreset']=='',model.requests[-1]
edit('audioEqInput0','3.5 dB');assert model.requests[-1]==['equalizerPreamp',3.5]
edit('audioEqInput10','-2.5 dB');assert model.requests[-1][0]=='equalizerBands'and model.state['equalizerBands'][9]==-2.5
assert item('audioequalizerPreset').property('currentIndex')==-1
done('Real expanded11-channel EQ submits an actual ten-number QVariant-compatible list, applies preset bands atomically and clears named preset on gain edits')
before=copy.deepcopy(model.state);model.fail_next='The owned settings file is locked.';focus('audioEqGain10');QTest.keyClick(window,Qt.Key_Up);QTest.qWait(25)
assert model.state==before and item('audioEqGain10').property('value')==before['equalizerBands'][9]
model.fail_next='The owned settings file is locked.';select('audioequalizerPreset',0);assert model.state==before and item('audioequalizerPreset').property('currentIndex')==-1
edit('audioEqInput1','22 dB');assert model.state==before and '22'not in item('audioEqInput1').property('text')
model.freqs=[31.25,62.5,125,250,500,1000,2000,4000,8000,16000];model.equalizerFrequenciesChanged.emit();QTest.qWait(25)
assert evaluate(page,'frequencyLabel(frequencies[0])')=='31.25 Hz'
assert item('audioEqFrequency1').property('text')=='31.25 Hz'
done('EQ save failures/invalid gains restore every value; live native frequency notifications drive labels without replacing gains')
native_gain=struct.unpack('f',struct.pack('f',7.2))[0];bands=list(model.state['equalizerBands']);bands[0]=native_gain
model.patch(equalizerPreamp=native_gain,equalizerBands=bands);QTest.qWait(25)
assert item('audioEqInput0').property('text')=='7.2 dB'and item('audioEqInput1').property('text')=='7.2 dB'
n=len(model.requests);focus('audioEqInput0');QTest.keyClick(window,Qt.Key_Tab);focus('audioEqInput1');QTest.keyClick(window,Qt.Key_Tab);QTest.qWait(25)
assert len(model.requests)==n and model.state['equalizerPreamp']==native_gain and model.state['equalizerBands'][0]==native_gain
done('Real float32-expanded7.2 gains display authored precision; ordinary focus changes never submit rounded values')
click('audioAdvanced');assert item('audiopassthrough').isVisible()
select('audiodetectDolby',2);assert model.requests[-1]==['detectDolby',2]
select('audioresamplingQuality',3);assert model.requests[-1]==['resamplingQuality',3]
model.patch(passthrough=None);model.raw_passthrough=42;QTest.qWait(20)
focus('audiopassthrough');QTest.keyClick(window,Qt.Key_Left);QTest.qWait(25);assert model.requests[-1]==['passthrough',False]and model.raw_passthrough==0
model.patch(passthrough=True);model.raw_passthrough=1
focus('audiopassthrough');QTest.keyClick(window,Qt.Key_Right);QTest.qWait(25);assert model.raw_passthrough==1
model.patch(passthrough=None);model.raw_passthrough=42;obj=focus('audiopassthrough')
QTest.mouseClick(window,Qt.LeftButton,Qt.NoModifier,obj.mapToScene(QPointF(obj.width()/4,obj.height()/2)).toPoint());QTest.qWait(25)
assert model.requests[-1]==['passthrough',False]and model.raw_passthrough==0
done('Advanced native policy choices use exact enums; unknown MMDevice passthrough can explicitly repair Off and existing On1 remains preserved')
before=len(model.requests);model.patch(outputDevice='disconnected-id',preferredLanguage='fi,en',resamplingQuality=None,speakerSetup=2)
assert item('audiooutputDevice').property('currentIndex')==-1 and item('audiopreferredLanguage').property('currentIndex')==-1 and item('audiospeakerSetup').property('currentIndex')==-1
model.device_rows.append(dict(value='disconnected-id',name='Generated reconnected device'));model.devicesChanged.emit();QTest.qWait(25)
assert item('audiooutputDevice').property('currentIndex')==3 and len(model.requests)==before
model.device_rows.pop();model.devicesChanged.emit();QTest.qWait(25);assert item('audiooutputDevice').property('currentIndex')==-1 and len(model.requests)==before
model.caps['resamplingQuality']=False;model.caps['equalizerBands']=False;model.capabilitiesChanged.emit();QTest.qWait(20)
assert not item('audioresamplingQuality').isEnabled()and not item('audioEqGain1').isEnabled()and item('audioequalizerPreset').isEnabled()
model.caps['resamplingQuality']=True;model.caps['equalizerBands']=True;model.capabilitiesChanged.emit()
done('Disconnected devices, custom languages/unmanaged resampler and missing Binaural stay unselected; dynamic caps disable only their real owners')
model.fail_next='The owned settings file is locked.';before=model.state['normalizeLoudness'];click('audionormalizeLoudness');assert model.state['normalizeLoudness']==before and item('audioSettingsError').isVisible()
model.fail_next='The owned settings file is locked.';before=model.state['pitchShift'];edit('audiopitchShift','-1.25');assert model.state['pitchShift']==before and float(item('audiopitchShift').property('text'))==before
assert item('audioSettingsRestart').isVisible();model.patch(**seed);QTest.qWait(25);assert not item('audioSettingsRestart').isVisible()
select('audioequalizerPreset',0);assert model.state['equalizerPreset']=='flat'and not model.restart
model.patch(equalizerPreset='');QTest.qWait(20)
done('Failed boolean/number writes preserve actual display and prior native-shaped state; restoring original effective defaults clears restart feedback')
images=[];fixture=json.loads((ROOT/'modules/gui/qt/tests/fixtures/juxtopposed_audio_assets.json').read_text(encoding='utf-8'))
for name,entry in fixture.items():assert hashlib.sha256((assets/name).read_bytes()).hexdigest()==entry['sha256']
for f in fields['rows']:
 if f['key']=='maximumVolume':continue
 obj=item('audio'+f['key']+'InfoOriginal');assert obj.width()==18 and obj.height()==18 and evaluate(obj,'status')==1
 images.append(dict(name=obj.objectName(),width=obj.width(),height=obj.height(),scale=obj.scale(),source=obj.property('source').toString()))
for key in['outputDevice','speakerSetup','replayGainMode','equalizerPreset','spatialAudioMode','preferredLanguage','visualizer','detectDolby','resamplingQuality','mediaRole','visualizationSize']:
 obj=item('audio'+key+'OriginalChevron');assert obj.width()==24 and obj.height()==24 and obj.property('mirror')and evaluate(obj,'status')==1
 images.append(dict(name=obj.objectName(),width=obj.width(),height=obj.height(),source=obj.property('source').toString()))
for prefix,count in[('audioMaximumOriginalTick',9)]+[('audioEq'+str(i)+'OriginalTick',2 if i==0 else 3)for i in range(11)]:
 for i in range(count):
  obj=item(prefix+str(i));endpoint=i in[0,count-1]
  assert [obj.width(),obj.height()]==([32,32]if endpoint else[2,8])and evaluate(obj,'status')==1
  assert obj.parentItem().clip()==endpoint
  images.append(dict(name=obj.objectName(),width=obj.width(),height=obj.height(),rotation=obj.rotation(),source=obj.property('source').toString()))
for name in['audioEqualizerOriginalChevron','audioAdvancedOriginalChevron','globalCategoryaudioOriginalIcon']:
 obj=item(name);assert obj.width()==24 and obj.height()==24 and evaluate(obj,'status')==1
 images.append(dict(name=obj.objectName(),width=obj.width(),height=obj.height(),source=obj.property('source').toString()))
assert len(images)==83,len(images)
model.freqs=[60,170,310,600,1000,3000,6000,12000,14000,16000];model.equalizerFrequenciesChanged.emit();QTest.qWait(25)
panel=item('audioEqualizerPanel');viewport=item('globalSettingsPageScroll')
viewport.setProperty('contentY',panel.mapToItem(viewport.property('contentItem'),QPointF(0,0)).y()-100);QTest.qWait(25)
assert item('audioEqInput0').property('text')=='12.0 dB'and item('audioEqInput1').property('text')=='0.0 dB'
assert window.grabWindow().save(str(results/'02-audio-expanded.png'))
done('All83 authored info/chevron/maximum/EQ tick/selected-category image instances resolve exact original bytes, dimensions and clipping')
geometry=[]
for width,height,scale in[(1080,897,1),(640,420,1),(960,600,1.5),(1280,720,2)]:
 window.resize(width,height);dialog.setProperty('uiScale',scale);QTest.qWait(25)
 for name in['audiooutputDevice','audiostartupVolume','audiovolumeStep','audioEqGain1','audioEqInput10','audiodetectDolby']:
  obj=focus(name);assert obj.hasActiveFocus(),name
  point=obj.mapToScene(QPointF(0,0));viewport=item('globalSettingsPageScroll');top=viewport.mapToScene(QPointF(0,0)).y()
  assert point.x()>=-.5 and point.x()+obj.width()<=width+.5 and point.y()>=top-.5 and point.y()+obj.height()<=top+viewport.height()+.5,(name,point,top,viewport.height())
 for i in range(9):assert item('audioMaximumOriginalTick'+str(i)).scale()==scale
 for i in range(11):assert item('audioEq'+str(i)+'OriginalTick0').scale()==scale
 geometry.append(dict(width=width,height=height,scale=scale));assert window.grabWindow().save(str(results/f'03-audio-{width}-{scale}.png'))
done('Real horizontal EQ and vertical host focus reveal keep controls within natural/narrow150/200% layouts; all original artwork scales')
window.resize(1080,897);dialog.setProperty('uiScale',1);page.setProperty('equalizerExpanded',False);focus('globalSettingsSearch');text('Equalizer bands');QTest.qWait(25)
assert item('audioEqGain1').isVisible()and not item('audiooutputDevice').isVisible()
QTest.keyClick(window,Qt.Key_A,Qt.ControlModifier);text('Resampling');QTest.qWait(25)
assert item('audioresamplingQuality').isVisible()and not item('audiooutputDevice').isVisible()
QTest.keyClick(window,Qt.Key_A,Qt.ControlModifier);QTest.keyClick(window,Qt.Key_Backspace);QTest.qWait(25)
focus('audiospeakerSetup');QTest.keyClick(window,Qt.Key_Space);QTest.qWait(20);assert item('audiospeakerSetupPopup').property('visible')
n=model.reloads;provider.showGlobalSettingsDialog.emit();QTest.qWait(25);assert model.reloads==n and not item('audiospeakerSetupPopup').property('visible')and item('globalSettingsSearch').hasActiveFocus()
ctx.general.patch(theme=1,accentIndex=4);QTest.qWait(25);assert page.property('accent').name()=='#18aad3'and item('audiovolumeStep').property('color').name()!='#8f8f8f'and not ctx.general.requests
assert window.grabWindow().save(str(results/'04-audio-shared-light-adaptation.png'))
ctx.general.patch(theme=2,accentIndex=0)
QTest.keyClick(window,Qt.Key_Escape);QTest.qWait(25);assert not dialog.property('visible')and item('opener').hasActiveFocus()
window.setProperty('omitAudio',True);click('opener');assert not evaluate(dialog,'chooseCategory("audio")')and dialog.property('selectedCategory')=='general'
done('Actual search/disclosure, repeat popup focus, shared theme/accent, Escape/opener and missing separate-service category gates preserve General')
# Additive lower-field error regression; original providers/assertions above stay
# unchanged. Return retains input focus while genuine supplied native validation
# or save failure rolls back. A top offscreen banner alone cannot satisfy this.
window.setProperty('omitAudio',False);QTest.qWait(25)
assert evaluate(dialog,'chooseCategory("audio")')
page=item('juxtopposedAudioSettings');page.setProperty('advancedExpanded',True)
inline_error_geometry=[]
for field_width,field_height,field_scale in [(1080,897,1),(640,420,1),(960,600,1.5),(1280,720,2)]:
 window.resize(field_width,field_height);dialog.setProperty('uiScale',field_scale);model.patch(audioDelayMs=0);model.feedback('');QTest.qWait(40)
 field=item('audioaudioDelayMs');field.setFocus(False);field.forceActiveFocus(Qt.TabFocusReason);QTest.qWait(25)
 before=len(model.requests);QTest.keyClick(window,Qt.Key_A,Qt.ControlModifier);text('invalid-native-value');QTest.keyClick(window,Qt.Key_Return);QTest.qWait(90)
 assert len(model.requests)==before+1 and model.requests[-1]==['audioDelayMs','invalid-native-value']
 assert model.state['audioDelayMs']==0 and field.property('text')=='0 ms' and field.hasActiveFocus()
 assert model.error and item('audioSettingsError').isVisible()and dialog.property('visible')and not dialog.property('busy')
 view=item('globalSettingsPageScroll');top=view.mapToScene(QPointF(0,0));banner=item('audioSettingsError');banner_point=banner.mapToScene(QPointF(0,0))
 tip=field.findChild(QObject,'audioaudioDelayMsValidationError')
 if tip is None:
  assert window.grabWindow().save(str(results/'missing-inline-feedback.png'))
  (results/'missing-inline-feedback.json').write_text(json.dumps(dict(category='audio',field='audioaudioDelayMs',width=field_width,height=field_height,scale=field_scale,bannerY=banner_point.y(),viewportTop=top.y(),fieldFocused=field.hasActiveFocus(),rolledBack=True,nativeAcceptance=False),indent=2)+'\n',encoding='utf-8')
 assert tip is not None,('Missing in-viewport genuine field feedback',field_width,field_height,field_scale,banner_point,top)
 assert tip.property('visible')and not tip.property('focus')and tip.property('text')==model.error
 box=tip.property('background');position=box.mapToScene(QPointF(0,0));field_position=field.mapToScene(QPointF(0,0))
 assert top.x()-.5<=position.x()and position.x()+box.width()<=top.x()+view.width()+.5 and top.y()-.5<=position.y()and position.y()+box.height()<=top.y()+view.height()+.5,(position,box.width(),box.height(),top,view.width(),view.height())
 assert top.y()-.5<=field_position.y()and field_position.y()+field.height()<=top.y()+view.height()+.5
 inline_error_geometry.append(dict(width=field_width,height=field_height,scale=field_scale,error=[position.x(),position.y(),box.width(),box.height()],viewport=[top.x(),top.y(),view.width(),view.height()],bannerY=banner_point.y(),fieldFocused=field.hasActiveFocus()))
 assert window.grabWindow().save(str(results/f'05-audio-inline-error-{field_width}-{field_scale}.png'))
 requests=len(model.requests);reloads=model.reloads;provider.showGlobalSettingsDialog.emit();QTest.qWait(30)
 assert model.reloads==reloads and len(model.requests)==requests and dialog.property('visible')and item('globalSettingsSearch').hasActiveFocus()and not tip.property('visible')
 field.forceActiveFocus(Qt.TabFocusReason);QTest.qWait(30);assert tip.property('visible')and field.hasActiveFocus()
 QTest.keyClick(window,Qt.Key_A,Qt.ControlModifier);text('x');QTest.qWait(30);assert not tip.property('visible')
 QTest.keyClick(window,Qt.Key_A,Qt.ControlModifier);text('1');QTest.keyClick(window,Qt.Key_Return);QTest.qWait(70)
 assert model.state['audioDelayMs']==1 and field.hasActiveFocus()and not model.error and not tip.property('visible')
 model.fail_next='The owned native settings file is locked.';QTest.keyClick(window,Qt.Key_A,Qt.ControlModifier);text('2');QTest.keyClick(window,Qt.Key_Return);QTest.qWait(80)
 assert model.state['audioDelayMs']==1 and tip.property('visible')and tip.property('text')==model.error and 'locked' in model.error
 assert model.error in evaluate(field,'Accessible.description')
 model.feedback('');QTest.qWait(40);assert not tip.property('visible')and field.hasActiveFocus()
# The same Input component owns typed ten-band writes. Verify its far-right
# scrolled field also keeps feedback inside the actual page/visible EQ viewport.
page.setProperty('equalizerExpanded',True)
for field_width,field_height,field_scale in [(1080,897,1),(640,420,1),(960,600,1.5),(1280,720,2)]:
 window.resize(field_width,field_height);dialog.setProperty('uiScale',field_scale);model.patch(equalizerBands=[0.0]*10);model.feedback('');QTest.qWait(40)
 field=item('audioEqInput10');field.setFocus(False);field.forceActiveFocus(Qt.TabFocusReason);QTest.qWait(30)
 meta_before=list(model.rejected_meta_types);QTest.keyClick(window,Qt.Key_A,Qt.ControlModifier);text('99');QTest.keyClick(window,Qt.Key_Return);QTest.qWait(90)
 assert model.state['equalizerBands']==[0.0]*10 and model.rejected_meta_types==meta_before and field.property('text')=='0.0 dB'and field.hasActiveFocus()
 tip=field.findChild(QObject,'audioEqInput10ValidationError');assert tip is not None and tip.property('visible')and tip.property('text')==model.error
 box=tip.property('background');position=box.mapToScene(QPointF(0,0));view=item('globalSettingsPageScroll');top=view.mapToScene(QPointF(0,0));eq=item('audioEqualizerScroll');eq_left=eq.mapToScene(QPointF(0,0)).x()
 assert top.x()-.5<=position.x()and position.x()+box.width()<=top.x()+view.width()+.5 and top.y()-.5<=position.y()and position.y()+box.height()<=top.y()+view.height()+.5,(position,top,box.width(),view.width())
 assert eq_left-.5<=position.x()and position.x()+box.width()<=eq_left+eq.width()+.5
 inline_error_geometry.append(dict(field='audioEqInput10',width=field_width,height=field_height,scale=field_scale,error=[position.x(),position.y(),box.width(),box.height()],typedListPreserved=True))
 assert window.grabWindow().save(str(results/f'06-audio-eq-inline-error-{field_width}-{field_scale}.png'))
 model.feedback('');QTest.qWait(30);assert not tip.property('visible')

(results/'inline-error-geometry.json').write_text(json.dumps(inline_error_geometry,indent=2)+'\n',encoding='utf-8')
done('Rejected lower Audio field retains rollback/focus and native error inside actual viewport at natural/narrow150/200%; search/edit/success/error-clear lifecycle and save failure preserved')

# Additive desired display oracle. All original16 groups/providers/dirty-f32 cases above remain exact.
window.resize(1080,897);dialog.setProperty('uiScale',1);QTest.qWait(25)
focus('globalSettingsSearch');QTest.keyClick(window,Qt.Key_A,Qt.ControlModifier);QTest.keyClick(window,Qt.Key_Backspace);QTest.qWait(25)
native_step=struct.unpack('f',struct.pack('f',.05))[0]*100
assert native_step==5.000000074505806
model.caps['volumeStep']=False;model.capabilitiesChanged.emit();model.patch(volumeStep=native_step);QTest.qWait(25)
step=item('audiovolumeStep');assert not step.isEnabled()and step.property('current')==native_step
assert step.property('text')=='5',('NATIVE_FLOAT_DISPLAY',step.property('text'),native_step)
focus('globalSettingsSearch');text('Volume step');QTest.qWait(25);assert window.grabWindow().save(str(results/'07-audio-derived-native-step.png'))
QTest.keyClick(window,Qt.Key_A,Qt.ControlModifier);QTest.keyClick(window,Qt.Key_Backspace);QTest.qWait(25)
model.caps['volumeStep']=True;model.capabilitiesChanged.emit();QTest.qWait(25)
for key,value,expected in [('volumeStep',native_step,'5'),('normalizationTarget',1.23456789,'1.23457'),('pitchShift',2.34567891,'2.34568'),('startupVolume',100.00000000000001,'100'),('audioDelayMs',2147483647,'2147483647')]:
 model.patch(**{key:value});QTest.qWait(20);field=item('audio'+key);unit=field.property('unit');count=len(model.requests)
 assert field.property('text')==expected+unit and field.property('current')==value,(key,field.property('text'),expected+unit)
 focus('audio'+key);QTest.keyClick(window,Qt.Key_Return);QTest.keyClick(window,Qt.Key_Tab);QTest.qWait(25)
 assert len(model.requests)==count and model.state[key]==value and not field.property('userEdited')
model.patch(volumeStep='1.23456789');QTest.qWait(20);assert step.property('text')=='1.23456789'and step.property('current')=='1.23456789'
edit('audiovolumeStep','6.1250001');assert model.requests[-1]==['volumeStep',6.1250001]and model.state['volumeStep']==6.1250001 and item('audiovolumeStep').property('text')=='6.125'
model.patch(volumeStep=native_step);QTest.qWait(20);count=len(model.requests);focus('audiovolumeStep');QTest.keyClick(window,Qt.Key_Tab);assert len(model.requests)==count and model.state['volumeStep']==native_step
done('Derived native floats display six significant digits without changing exact model values; disabled native5.000000074505806 shows5, integers/text stay exact and unedited focus never resaves rounded values')

assert 'Applies to volume steps immediately.' in evaluate(page,'helpFor("volumeStep")')
assert 'Restart VLC' not in evaluate(page,'helpFor("volumeStep")')
assert 'Restart VLC' in evaluate(page,'helpFor("startupVolume")')
model.caps['volumeStep']=False;model.capabilitiesChanged.emit();QTest.qWait(25)
assert 'unavailable' in evaluate(page,'helpFor("volumeStep")')
assert 'immediately' not in evaluate(page,'helpFor("volumeStep")')
model.caps['volumeStep']=True;model.capabilitiesChanged.emit();QTest.qWait(25)
done('Volume-step help distinguishes a linked live policy from startup defaults and an unavailable consumer')

assert 'next media' not in evaluate(page,'helpFor("audioEnabled")')
assert 'Restart VLC' in evaluate(page,'helpFor("audioEnabled")')
done('Audio on/off help follows the verified startup policy without promising a same-player next-input change')

assert 'next media item' in evaluate(page,'helpFor("audioDelayMs")')
assert 'Restart VLC' not in evaluate(page,'helpFor("audioDelayMs")')
done('Audio-delay help states the verified next-input timing separately from live volume steps and restart-only defaults')

destroyed=[];engine.destroyed.connect(lambda:destroyed.append(True));window.close();engine.deleteLater();QCoreApplication.sendPostedEvents(None,QEvent.DeferredDelete);QTest.qWait(25);assert destroyed==[True]and not messages,messages
(results/'verification.json').write_text(json.dumps(dict(passed=True,caseCount=len(checks),cases=checks,messages=messages,geometry=geometry,renderedImageInstances=images,adapters=identities,workerSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),nativeAccepted=False,scope=__doc__),indent=2)+'\n',encoding='utf-8')
print('PASS actual Audio QML with supplied native-shaped provider only.',flush=True)
