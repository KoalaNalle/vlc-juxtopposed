# SPDX-License-Identifier: GPL-2.0-or-later
"""Actual production Light/Dark volume artwork and native binding contract.

Supplied QObject state only: not a compiled native VLC or OS theme test.
"""
from pathlib import Path
import argparse,hashlib,json,math,os,shutil,subprocess,xml.etree.ElementTree as ET
HERE = Path(__file__).resolve().parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--repo-root', type=Path, default=Path(__file__).resolve().parents[4])
parser.add_argument('--output-dir', type=Path, default=Path('ui-previews/light-volume'))
args = parser.parse_args()
ROOT = args.repo_root.resolve()
results = args.output_dir.resolve()
results.mkdir(parents=True, exist_ok=False)
fixture = HERE / 'fixtures/juxtopposed_light_volume_assets.json'
expected = json.loads(fixture.read_text(encoding='utf-8'))
os.environ.update(QT_QPA_PLATFORM='offscreen',QT_QUICK_BACKEND='software',QML_DISABLE_DISK_CACHE='1',
    QT_QPA_FONTDIR=str(ROOT/'modules/gui/qt/pixmaps/juxtopposed'))
import PySide6
from PySide6.QtCore import QObject,Property,Signal,Qt,QUrl,QPoint,QPointF,QRectF,QEvent,QCoreApplication,qInstallMessageHandler
from PySide6.QtGui import QGuiApplication,QWheelEvent,QImage
from PySide6.QtQml import QQmlApplicationEngine,QQmlExpression,qmlContext,qmlRegisterSingletonType
from PySide6.QtQuick import QQuickItem
from PySide6.QtTest import QTest,QSignalSpy
class PlayerFixture(QObject):
    volumeChanged=Signal();mutedChanged=Signal()
    def __init__(self):super().__init__();self.level=.5;self.silent=False;self.requests=[]
    def set_volume(self,value):
        assert math.isfinite(value) and 0<=value<=2,value
        self.requests.append(['volume',value]);self.level=value;self.volumeChanged.emit()
    def set_muted(self,value):
        assert type(value)is bool;self.requests.append(['muted',value]);self.silent=value;self.mutedChanged.emit()
    volume=Property(float,lambda s:s.level,set_volume,notify=volumeChanged)
    muted=Property(bool,lambda s:s.silent,set_muted,notify=mutedChanged)
    def seed(self,level,muted=False):self.level=level;self.silent=muted;self.volumeChanged.emit();self.mutedChanged.emit()
class ContextFixture(QObject):
    maxVolume=Property(int,lambda s:200,constant=True)
messages=[];qInstallMessageHandler(lambda kind,context,message:messages.append(message))
app=QGuiApplication([]);player=PlayerFixture();context=ContextFixture()
qmlRegisterSingletonType(QUrl.fromLocalFile(str(ROOT/'modules/gui/qt/style/JuxtopposedPalette.qml')),'VLC.Style',1,0,'JuxtopposedPalette')
# The current side imports production QML directly. The comparison control
# differs ONLY in the two historical hardcoded Dark volume asset expressions.
# All other current playback behavior/dependencies remain shared. This tests
# the narrow historical asset choice; it is not a frozen full-bar geometry.
assets = ROOT / 'modules/gui/qt/pixmaps/juxtopposed'
qml = results / 'historical-control'
qml.mkdir()
player_qml = ROOT / 'modules/gui/qt/player/qml'
current_source = (player_qml / 'JuxtopposedVolumeControl.qml').read_text(encoding='utf-8')
historical_source = current_source
historical_reversions = {}
for filename in ['volume-background.svg', 'volume-fill.svg']:
    current_expression = 'source: root.assetBase + root.appearance.icon("' + filename + '")'
    historical_expression = 'source: root.assetBase + "' + filename + '"'
    assert historical_source.count(current_expression) == 1, filename
    historical_source = historical_source.replace(current_expression, historical_expression)
    historical_reversions[current_expression] = historical_expression
historical_path = qml / 'HistoricalDarkVolumeControl.qml'
historical_path.write_text('import "' + player_qml.as_uri() + '"\n' + historical_source,
                           encoding='utf-8', newline='\n')
historical_bar = (player_qml / 'JuxtopposedPlaybackBar.qml').read_text(encoding='utf-8')
assert historical_bar.count('JuxtopposedVolumeControl {') == 2
(qml/'HistoricalDarkVolumeBar.qml').write_text('import "' + player_qml.as_uri() + '"\n'
    + historical_bar.replace('JuxtopposedVolumeControl {','HistoricalDarkVolumeControl {'),encoding='utf-8',newline='\n')
source=(ROOT/'modules/gui/qt/player/qml/ControlBar.qml').read_text(encoding='utf-8')
handler=source[source.index('            onVolumeRequested: (value) => {'):source.index('            onAudioRequested:',source.index('            onVolumeRequested: (value) => {'))]
assert 'Player.muted = false' in handler and 'Player.volume = value' in handler
bindings='\n'.join(line for line in source.splitlines() if line.strip() in ['volume: Player.volume','maximumVolume: MainCtx.maxVolume / 100','muted: Player.muted'])
assert len(bindings.splitlines())==3
engine=QQmlApplicationEngine();engine.rootContext().setContextProperty('Player',player);engine.rootContext().setContextProperty('MainCtx',context)
driver='''import QtQuick
import QtQuick.Controls.Basic as C
import VLC.Style
import "@BASELINE@" as Historical
import "@PENDING@" as Current
C.ApplicationWindow {
    id:window;visible:true;width:1920;height:2*(width<860*testScale ? 94:54)*testScale
    property bool dark:true
    property real testScale:1
    QtObject { id:nativePalette;property bool isDark:window.dark }
    Binding { target:JuxtopposedPalette;property:"nativePalette";value:nativePalette }
    Historical.HistoricalDarkVolumeBar {
        id:baseline;objectName:"baseline";width:parent.width;height:implicitHeight;uiScale:window.testScale
        assetBase:"@ASSETS@"
@BINDINGS@
@HANDLER@
    }
    Current.JuxtopposedPlaybackBar {
        id:pending;objectName:"pending";y:baseline.height;width:parent.width;height:implicitHeight;uiScale:window.testScale
        assetBase:"@ASSETS@"
@BINDINGS@
@HANDLER@
    }
}'''.replace('@BASELINE@',qml.as_uri()).replace('@PENDING@',player_qml.as_uri()).replace('@ASSETS@',assets.as_uri()+'/').replace('@BINDINGS@',bindings).replace('@HANDLER@',handler)
(results/'driver.qml').write_text(driver,encoding='utf-8',newline='\n')
engine.loadData(driver.encode(),QUrl.fromLocalFile(str(results/'driver.qml')));assert engine.rootObjects(),messages
window=engine.rootObjects()[0];QTest.qWait(80)
baseline=window.findChild(QObject,'baseline');pending=window.findChild(QObject,'pending')
def item(name):
    obj=pending.findChild(QObject,name);assert obj,name;return obj
def evaluate(obj,code):
    expr=QQmlExpression(qmlContext(obj),obj,code);value,_=expr.evaluate();assert not expr.hasError(),expr.error().toString();return value
def done(name):checks.append(name);print('PASS',name,flush=True)
checks=[];geometry=[]
volume=item('juxtopposedVolume');clip=item('juxtopposedVolumeFillClip');fill=item('juxtopposedVolumeOriginalFill');background=item('juxtopposedVolumeOriginalBackground')
for name,record in expected['assets'].items():
    assert hashlib.sha256((assets/name).read_bytes()).hexdigest()==record['sha256']
    root=ET.parse(assets/name).getroot();assert [root.attrib['width'],root.attrib['height']]==[record['rootWidth'],record['rootHeight']]
assert expected['assets']['volume-fill-light.svg']['rootWidth']=='86'
assert hashlib.sha256((assets/'volume-fill-light.svg').read_bytes()).hexdigest()!=expected['rejectedClippedFragment']['sha256']
done('Original Light background89px and complete fill86px identities/root dimensions verified; clipped53px fragment is not shipped')

def bars_identical():
    pending.forceActiveFocus(Qt.OtherFocusReason);QTest.qWait(20)
    image=window.grabWindow();height=round(baseline.height())
    return image.copy(0,0,window.width(),height)==image.copy(0,height,window.width(),height)
for value in [0,.5,1,1.5,2]:
    before=len(player.requests);player.seed(value);QTest.qWait(25)
    assert len(player.requests)==before
    assert background.property('source').fileName()=='volume-background.svg' and fill.property('source').fileName()=='volume-fill.svg'
    assert bars_identical(),value
    assert abs(clip.width()-value/2*86)<.0001 and fill.width()==86
assert window.grabWindow().save(str(results/'01-dark-baseline-and-proposed-identical.png'))
done('Current production and narrowly historical Dark asset control render pixel-identically at0/50/100/150/200%; engine state emits no user actions')

before=len(player.requests);window.setProperty('dark',False);player.seed(2);QTest.qWait(30)
assert len(player.requests)==before
assert background.property('source').fileName()=='volume-background-light.svg' and fill.property('source').fileName()=='volume-fill-light.svg'
for image in [background,fill]:assert evaluate(image,'Number(status)')==1
assert fill.width()==86 and fill.height()==24.56 and clip.width()==86 and clip.property('clip')
assert window.grabWindow().save(str(results/'02-light-baseline-dark-volume-versus-original-fill.png'))
done('Actual production Light bar selects full original assets live; full-range clipping shows86px, not stretched53px artwork')

# The actual cached Figma PNG is the visual oracle. Its opaque export backdrop
# differs from the VLC pane, so compare only fill/background interior pixels,
# not blank/antialiased pixels surrounding the artwork. No SVG path is read.
composite=HERE/'fixtures/juxtopposed_light_volume_reference.png'
assert hashlib.sha256(composite.read_bytes()).hexdigest()==expected['composite']['sha256']
player.seed(2*51.5/86);pending.forceActiveFocus();QTest.qWait(25)
actual=window.grabWindow();origin=volume.mapToScene(QPointF(0,0));samples=[]
oracle=QImage(str(composite));assert not oracle.isNull() and oracle.width()==89 and oracle.height()==28
for x,y in [(30,20),(45,20),(65,20)]:
    actual_color=actual.pixelColor(round(origin.x()+x),round(origin.y()+y));reference_color=oracle.pixelColor(x,y)
    distance=max(abs(actual_color.red()-reference_color.red()),abs(actual_color.green()-reference_color.green()),abs(actual_color.blue()-reference_color.blue()))
    assert distance<=4,(x,y,actual_color.name(),reference_color.name(),distance)
    samples.append(dict(point=[x,y],actual=actual_color.name(),reference=reference_color.name(),maxChannelDifference=distance))
done('Light interior fill/background pixels agree with unchanged tracked designer composite at the same clipping boundary')

for width,scale in [(1920,1),(1280,2),(960,1.5)]:
    window.setProperty('testScale',scale);window.resize(width,round(2*(94 if width<860*scale else 54)*scale));QTest.qWait(30)
    for dark in [True,False]:
        window.setProperty('dark',dark)
        for value in [0,.5,1,1.5,2]:
            player.seed(value);QTest.qWait(20)
            assert abs(clip.width()-value/2*86*scale)<.0001
            assert fill.width()==86*scale and abs(fill.height()-24.56*scale)<.0001
            assert abs(clip.x()-1.5*scale)<.0001 and abs(clip.y()-1.72*scale)<.0001
            assert background.width()==89*scale and abs(background.height()-27.36*scale)<.0001
            assert volume.property('from')==0 and volume.property('to')==2
            assert fill.property('source').fileName()==('volume-fill.svg' if dark else 'volume-fill-light.svg')
            assert background.property('source').fileName()==('volume-background.svg' if dark else 'volume-background-light.svg')
    geometry.append(dict(width=width,scale=scale,fullFillSize=[fill.width(),fill.height()]))
done('Actual0–200% clip geometry and fixed full artwork scale correctly in both themes at100/150/200%; native max binding preserved')

window.setProperty('testScale',1);window.resize(1920,108);window.setProperty('dark',False);player.seed(.5,True);QTest.qWait(25)
assert volume.property('opacity')==.5
volume.forceActiveFocus(Qt.TabFocusReason);moved=QSignalSpy(pending.volumeRequested)
QTest.keyClick(window,Qt.Key_Right);QTest.qWait(25)
assert moved.count()==1 and abs(player.level-.55)<.0001 and not player.silent
assert player.requests[-2:]==[['muted',False],['volume',.55]]
point=volume.mapToScene(QPointF(volume.width()/2,volume.height()/2));before=player.level
wheel=QWheelEvent(point,QPointF(window.mapToGlobal(point.toPoint())),QPoint(0,0),QPoint(0,120),Qt.NoButton,Qt.NoModifier,Qt.NoScrollPhase,False)
QGuiApplication.sendEvent(window,wheel);QTest.qWait(25)
assert abs(player.level-before-.05)<.0001
lock=QSignalSpy(pending.requestLockUnlockAutoHide)
target=volume.mapToScene(QPointF(volume.width()-1,volume.height()/2)).toPoint()
QTest.mousePress(window,Qt.LeftButton,Qt.NoModifier,target);QTest.qWait(15);QTest.mouseRelease(window,Qt.LeftButton,Qt.NoModifier,target);QTest.qWait(25)
assert player.level>=1.95 and player.level<=2 and lock.count()==2
assert lock.at(0)==[True] and lock.at(1)==[False]
done('Real keyboard,wheel,pointer controls retain source-exact native Player write/unmute contract and autohide lock/unlock signals')

before=len(player.requests);player.seed(.75,True);QTest.qWait(20);assert len(player.requests)==before and volume.property('opacity')==.5
window.setProperty('dark',True);QTest.qWait(20);assert fill.property('source').fileName()=='volume-fill.svg' and len(player.requests)==before
done('Mute/engine updates and live theme changes preserve volume without emitting input')
destroyed=[];engine.destroyed.connect(lambda:destroyed.append(True));window.close();engine.deleteLater();QCoreApplication.sendPostedEvents(None,QEvent.DeferredDelete);QTest.qWait(30)
assert destroyed==[True] and not messages,messages
for source in [player_qml/'JuxtopposedPlaybackBar.qml',player_qml/'JuxtopposedVolumeControl.qml',ROOT/'modules/gui/qt/style/JuxtopposedPalette.qml']:
    parsed=subprocess.run([str(Path(PySide6.__file__).parent/('qmlformat.exe' if os.name=='nt' else 'qmlformat')),'--ignore-settings',str(source)],stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True,timeout=20)
    assert parsed.returncode==0 and not parsed.stderr,(source,parsed.stderr)
record=dict(passed=True,cases=checks,caseCount=len(checks),messages=messages,geometry=geometry,designerCompositeSamples=samples,nativeAcceptance=False,
    sourceSha256={p.relative_to(ROOT).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in [
        player_qml/'JuxtopposedPlaybackBar.qml',player_qml/'JuxtopposedVolumeControl.qml',player_qml/'JuxtopposedLayoutRow.qml',player_qml/'ControlBar.qml',player_qml/'JuxtopposedButton.qml',
        player_qml/'JuxtopposedSeekBackground.qml',ROOT/'modules/gui/qt/style/JuxtopposedPalette.qml']},
    historicalControlReversions=historical_reversions,artworkFixtureSha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),
    designerCompositeSha256=hashlib.sha256(composite.read_bytes()).hexdigest(),repoRoot=str(ROOT),outputDirectory=str(results),
    workerSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),nativeBindingSourceSha256=hashlib.sha256((ROOT/'modules/gui/qt/player/qml/ControlBar.qml').read_bytes()).hexdigest(),
    actualBindingLines=bindings,actualVolumeHandler=handler,scope=__doc__)
(results/'verification.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8',newline='\n');print('PASS production Light volume component contract only.',flush=True)
