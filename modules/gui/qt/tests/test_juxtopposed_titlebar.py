# SPDX-License-Identifier: GPL-2.0-or-later
'''Original title-strip geometry with supplied controller seams, not native OS acceptance.'''
import argparse,json,os,shutil,tempfile
from pathlib import Path
from xml.etree import ElementTree
os.environ.update(QT_QPA_PLATFORM='offscreen',QT_QUICK_BACKEND='software',QML_DISABLE_DISK_CACHE='1',QT_QPA_FONTDIR=str(Path(__file__).resolve().parents[4]/'modules/gui/qt/pixmaps/juxtopposed'))
from PySide6.QtCore import QObject,Property,Signal,Slot,QUrl,Qt,QPoint,qInstallMessageHandler
from PySide6.QtQml import QmlAttached,qmlRegisterType,QQmlExpression,qmlContext
from PySide6.QtQuick import QQuickView
from PySide6.QtGui import QGuiApplication
from PySide6.QtTest import QSignalSpy,QTest
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output-dir',type=Path,required=True);parser.add_argument('--qml-file',type=Path);parser.add_argument('--shortcut-qml',type=Path);args=parser.parse_args();args.output_dir.mkdir(exist_ok=False)
r=Path(__file__).resolve().parents[4];messages=[]
def message_handler(kind,ctx,message):
 messages.append(message)
 with (args.output_dir/'messages-live.txt').open('a',encoding='utf-8') as f:f.write(message+'\n')
print('titlebar phase: Qt application',flush=True)
qInstallMessageHandler(message_handler);app=QGuiApplication([])
print('titlebar phase: attached navigation',flush=True)
class NavProperties(QObject):
    changed=Signal()
    def __init__(self,parent=None):super().__init__(parent);self.values={}
    @Slot('QVariant')
    def defaultKeyAction(self,event):pass
    @Property(QObject,notify=changed)
    def parentItem(self):return self.values.get('parentItem')
    @parentItem.setter
    def parentItem(self,value):self.values['parentItem']=value;self.changed.emit()
    @Property(QObject,notify=changed)
    def leftItem(self):return self.values.get('leftItem')
    @leftItem.setter
    def leftItem(self,value):self.values['leftItem']=value;self.changed.emit()
    @Property(QObject,notify=changed)
    def rightItem(self):return self.values.get('rightItem')
    @rightItem.setter
    def rightItem(self,value):self.values['rightItem']=value;self.changed.emit()
    @Property(QObject,notify=changed)
    def upItem(self):return self.values.get('upItem')
    @upItem.setter
    def upItem(self,value):self.values['upItem']=value;self.changed.emit()
    @Property(QObject,notify=changed)
    def downItem(self):return self.values.get('downItem')
    @downItem.setter
    def downItem(self,value):self.values['downItem']=value;self.changed.emit()
@QmlAttached(NavProperties)
class NavigationAdapter(QObject):
    @staticmethod
    def qmlAttachedProperties(owner,target):return NavProperties(target)
print('titlebar phase: register navigation',flush=True)
qmlRegisterType(NavigationAdapter,'VLC.Util',1,0,'Navigation')
print('titlebar phase: write supplied modules',flush=True)
module=tempfile.TemporaryDirectory(prefix='vlc-titlebar-component-');m=Path(module.name)
def write(group,name,text):
 p=m/'VLC'/group/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(text,encoding='utf-8');return p
write('Util','qmldir','module VLC.Util\nsingleton SVGColorImage 1.0 SVGColorImage.qml\n')
write('Util','SVGColorImage.qml','pragma Singleton\nimport QtQml\nQtObject { function colorize(path) { return { any: function(colors) { return this }, uri: function() { return path } } } }')
write('Style','qmldir','module VLC.Style\nsingleton VLCStyle 1.0 VLCStyle.qml\nsingleton JuxtopposedPalette 1.0 JuxtopposedPalette.qml\n')
write('Style','VLCStyle.qml','pragma Singleton\nimport QtQml\nQtObject { property real scale: 1; function dp(n,s) { return n*s } }')
shutil.copyfile(r/'modules/gui/qt/style/JuxtopposedPalette.qml',m/'VLC/Style/JuxtopposedPalette.qml')
write('Widgets','qmldir','module VLC.Widgets\nCSDTitlebarTapNDrapHandler 1.0 CSDTitlebarTapNDrapHandler.qml\nAcrylicBackground 1.0 AcrylicBackground.qml\n')
write('Widgets','AcrylicBackground.qml','import QtQuick\nRectangle { property color tintColor; property color alternativeColor: tintColor; color: alternativeColor }')
write('Widgets','CSDTitlebarTapNDrapHandler.qml','import QtQuick\nItem {}')
write('Menus','qmldir','module VLC.Menus\nQmlGlobalMenu 1.0 QmlGlobalMenu.qml\n')
write('Menus','QmlGlobalMenu.qml','''import QtQml
QtObject { property var ctx; property bool shown: false; property bool navigationAvailable: false; property bool navigationVisible: false; signal navigationRequested(); signal aboutToShow(); signal aboutToHide(); function popup(pos) { ctx.menuPopups++; shown=true; aboutToShow() } function close() { shown=false; aboutToHide() } }''')
write('MainInterface','qmldir','module VLC.MainInterface\nsingleton MainCtx 1.0 MainCtx.qml\nCSDButton 1.0 CSDButton.qml\n')
write('MainInterface','CSDButton.qml','import QtQml\nQtObject { enum ButtonType { Minimize, MaximizeRestore, Close } }')
write('MainInterface','MainCtx.qml','''pragma Singleton
import QtQuick
import QtQuick.Window
QtObject {
 id: ctx
 property bool clientSideDecoration: true
 property int menuPopups: 0
 property int minimizeCalls: 0
 property int maximizeCalls: 0
 property int closeCalls: 0
 property int fullscreenCalls: 0
 property bool hasEmbededVideo: false
 property bool juxtopposedControls: true
 property bool playerView: false
 property bool useGlobalShortcuts: true
 property bool mediaLibraryAvailable: false
 property bool interfaceFullscreen: false
 property bool videoFullscreen: false
 property QtObject intfMainWindow: QtObject { property int visibility: Window.Windowed; property int width: 1920; property int height: 1080 }
 property QtObject minimumButton: QtObject { property int type: 0; property rect rect; property bool showHovered: false; property bool externalPressed: false; function click() {ctx.minimizeCalls++} }
 property QtObject maximumButton: QtObject { property int type: 1; property rect rect; property bool showHovered: false; property bool externalPressed: false; function click() {ctx.maximizeCalls++; ctx.intfMainWindow.visibility=ctx.intfMainWindow.visibility===Window.Maximized?Window.Windowed:Window.Maximized} }
 property QtObject closeButton: QtObject { property int type: 2; property rect rect; property bool showHovered: false; property bool externalPressed: false; function click() {ctx.closeCalls++} }
 property QtObject csdButtonModel: QtObject { property var windowCSDButtons: [ctx.minimumButton,ctx.maximumButton,ctx.closeButton] }
 function updateFullscreen() { intfMainWindow.visibility=(interfaceFullscreen || videoFullscreen)?Window.FullScreen:Window.Windowed }
 function interfaceFullscreenHeld() { return interfaceFullscreen }
 function toggleInterfaceFullScreen() { fullscreenCalls++; interfaceFullscreen=intfMainWindow.visibility!==Window.FullScreen; updateFullscreen() }
}''')
write('Dialogs','qmldir','module VLC.Dialogs\nsingleton DialogsProvider 1.0 DialogsProvider.qml\n')
write('Dialogs','DialogsProvider.qml','pragma Singleton\nimport QtQml\nQtObject { property bool qmlDialogVisible: false; property bool qmlInputFocused: false }')
write('Player','qmldir','module VLC.Player\nsingleton Player 1.0 Player.qml\n')
write('Player','Player.qml','''pragma Singleton
import QtQml
import VLC.MainInterface
QtObject {
 property bool hasVideoOutput: false
 property bool fullscreen: false
 property int toggleCalls: 0
 function setFullscreen(value) { fullscreen=value; MainCtx.videoFullscreen=MainCtx.hasEmbededVideo && value; MainCtx.updateFullscreen() }
 function toggleFullscreen() { toggleCalls++; setFullscreen(!fullscreen) }
}''')
assets=m/'assets';assets.mkdir();qrc=ElementTree.parse(r/'modules/gui/qt/assets.qrc').getroot()
for resource in qrc.findall('qresource'):
 if resource.get('prefix')!='/juxtopposed':continue
 for file in resource.findall('file'):
  alias=file.get('alias','')
  if alias=='InterVariable.ttf':font_url=QUrl.fromLocalFile(str(r/'modules/gui/qt'/file.text)).toString()
  if alias.startswith('titlebar-'):shutil.copyfile(r/'modules/gui/qt'/file.text,assets/alias)
source=(args.qml_file or r/'modules/gui/qt/maininterface/qml/JuxtopposedTitlebar.qml').read_text().replace('qrc:/juxtopposed/InterVariable.ttf',font_url).replace('qrc:/juxtopposed/',QUrl.fromLocalFile(str(assets)+'/').toString())
# Supply the Player singleton to the negative-control baseline as well, so its
# failure is the lost fullscreen routing rather than a missing fixture import.
if 'import VLC.Player' not in source:source='import VLC.Player\n'+source
under=m/'TitlebarUnderTest.qml';under.write_text(source,encoding='utf-8')
print('titlebar phase: load actual component',flush=True)
view=QQuickView();view.engine().addImportPath(str(m));view.setResizeMode(QQuickView.SizeRootObjectToView);view.setSource(QUrl.fromLocalFile(str(under)))
assert not view.errors(),[e.toString() for e in view.errors()]
root=view.rootObject();assert root
view.show();checks=[]
def expr(code):
 e=QQmlExpression(qmlContext(root),root,code);value,_=e.evaluate();assert not e.hasError(),e.error().toString();return value

def item(name):
 pending=[root]
 while pending:
  obj=pending.pop()
  if obj.objectName()==name:return obj
  pending.extend(obj.childItems())
 raise AssertionError(name)
def click(obj):QTest.mouseClick(view,Qt.LeftButton,Qt.NoModifier,obj.mapToScene(obj.boundingRect().center()).toPoint());QTest.qWait(10)
def near(a,b):assert abs(a-b)<0.1,(a,b)
for scale in [1,1.5,2]:
 expr('VLCStyle.scale='+str(scale))
 for player in [False,True]:
  root.setProperty('playerContext',player);root.setProperty('sourceTitle','Videos')
  for width in [1920,320]:
   view.resize(int(width*scale),int(40*scale));QTest.qWait(60)
   near(root.implicitHeight(),40*scale)
   logo=item('juxtopposedStriplogo');menu=item('juxtopposedStripmenu')
   near(logo.mapToScene(logo.boundingRect().topLeft()).x(),6*scale)
   near(menu.mapToScene(menu.boundingRect().topLeft()).x(),51*scale)
   for key,size in [('logo',24),('menu',18),('minimize',24),('maximize',24),('fullscreen',24),('close',24)]:
    art=item('juxtopposedStripArt'+key);near(art.width(),size);near(art.height(),size);near(art.scale(),scale);evaluation=QQmlExpression(qmlContext(art),art,'Number(status)');status,_=evaluation.evaluate();assert not evaluation.hasError() and status==1,(key,messages)
   controls=[item('juxtopposedStrip'+key) for key in ['minimize','maximize','fullscreen','close']]
   for index,control in enumerate(controls):
    near(control.width(),36*scale);near(control.mapToScene(control.boundingRect().topLeft()).x(),(width-150+36*index)*scale)
   assert item('juxtopposedStripnavigation').isVisible()==player
   if player and width==1920:
    near(item('juxtopposedStripArtseparator').mapToScene(item('juxtopposedStripArtseparator').boundingRect().topLeft()).x(),117*scale)
    near(item('juxtopposedStripArtsourceFolder').mapToScene(item('juxtopposedStripArtsourceFolder').boundingRect().topLeft()).x(),143*scale)
   title=item('juxtopposedStripTitle');assert title.isVisible()==(width==1920)
   if title.isVisible():assert abs(title.mapToScene(title.boundingRect().center()).x()-width*scale/2)<=0.51, 'Pixel-aligned text center stays within half a logical pixel'
   view.grabWindow().save(str(args.output_dir/f'{"player" if player else "home"}-{width}-{scale}.png'))
   checks.append(dict(scale=scale,playerContext=player,width=width,geometry=True))
expr('VLCStyle.scale=1');root.setProperty('playerContext',False);view.resize(1920,40);QTest.qWait(40)
logo=item('juxtopposedStriplogo');logo.forceActiveFocus();QTest.keyClick(view,Qt.Key_Tab);assert item('juxtopposedStripmenu').hasActiveFocus()
QTest.keyClick(view,Qt.Key_Space);assert expr('MainCtx.menuPopups')==1;root.dismissRequested.emit()
for key,counter in [('minimize','minimizeCalls'),('maximize','maximizeCalls'),('close','closeCalls')]:
 click(item('juxtopposedStrip'+key));assert expr('MainCtx.'+counter)==1
click(item('juxtopposedStripmaximize'));assert expr('MainCtx.maximizeCalls')==2
click(item('juxtopposedStripfullscreen'));QTest.qWait(30);assert not item('juxtopposedStripclose').isVisible()
click(item('juxtopposedStripfullscreen'));QTest.qWait(30);assert item('juxtopposedStripclose').isVisible() and expr('MainCtx.fullscreenCalls')==2
root.setProperty('playerContext',True);spy=QSignalSpy(root.navigationRequested);click(item('juxtopposedStripnavigation'));assert spy.count()==1
fullscreen=item('juxtopposedStripfullscreen')
# For embedded video, the strip and VLC's F/Escape actions must share the video
# fullscreen holder. Simulate the native player boundary, keeping the separate
# interface holder observable so the two possible exit paths cannot mask a leak.
expr('MainCtx.hasEmbededVideo=true; Player.hasVideoOutput=true')
click(fullscreen);assert expr('Player.fullscreen && !MainCtx.interfaceFullscreen')
assert fullscreen.property('description')=='Leave fullscreen'
expr('Player.setFullscreen(false)');assert expr('MainCtx.intfMainWindow.visibility === Window.Windowed')
assert fullscreen.property('description')=='Fullscreen'
expr('Player.toggleFullscreen()');assert expr('Player.fullscreen')
click(fullscreen);assert expr('!Player.fullscreen && MainCtx.intfMainWindow.visibility === Window.Windowed')
# F11 remains an interface action. Its active hold is released by the strip,
# rather than accidentally starting a second video fullscreen hold.
expr('MainCtx.toggleInterfaceFullScreen()');assert expr('MainCtx.interfaceFullscreen && !Player.fullscreen')
click(fullscreen);assert expr('!MainCtx.interfaceFullscreen && !Player.fullscreen && MainCtx.intfMainWindow.visibility === Window.Windowed')
# If both independent owners are active, releasing one must not discard the
# other. A subsequent strip action can release the remaining interface hold.
expr('MainCtx.toggleInterfaceFullScreen(); Player.toggleFullscreen()')
click(fullscreen);assert expr('MainCtx.interfaceFullscreen && !Player.fullscreen && MainCtx.intfMainWindow.visibility === Window.FullScreen')
click(fullscreen);assert expr('!MainCtx.interfaceFullscreen && MainCtx.intfMainWindow.visibility === Window.Windowed')
# Audio and detached-video player contexts keep the existing interface action.
expr('MainCtx.hasEmbededVideo=false')
click(fullscreen);assert expr('MainCtx.interfaceFullscreen && !Player.fullscreen')
click(fullscreen);assert expr('!MainCtx.interfaceFullscreen && !Player.fullscreen')
expr('MainCtx.hasEmbededVideo=true; Player.hasVideoOutput=false')
click(fullscreen);assert expr('MainCtx.interfaceFullscreen && !Player.fullscreen')
click(fullscreen);assert expr('!MainCtx.interfaceFullscreen && !Player.fullscreen')
# The Home strip still controls the interface even if a video is present.
root.setProperty('playerContext',False)
expr('MainCtx.hasEmbededVideo=true; Player.hasVideoOutput=true')
click(fullscreen);assert expr('MainCtx.interfaceFullscreen && !Player.fullscreen')
click(fullscreen);assert expr('!MainCtx.interfaceFullscreen && !Player.fullscreen')
# When Home is shown while embedded video owns fullscreen, its visible exit
# must release that video owner. If both owners exist, Home releases its own
# interface owner first and leaves the independent video owner intact.
expr('Player.toggleFullscreen()');assert expr('Player.fullscreen && !MainCtx.interfaceFullscreen')
click(fullscreen);assert expr('!Player.fullscreen && MainCtx.intfMainWindow.visibility === Window.Windowed')
expr('MainCtx.toggleInterfaceFullScreen(); Player.toggleFullscreen()')
click(fullscreen);assert expr('!MainCtx.interfaceFullscreen && Player.fullscreen && MainCtx.intfMainWindow.visibility === Window.FullScreen')
click(fullscreen);assert expr('!MainCtx.interfaceFullscreen && !Player.fullscreen && MainCtx.intfMainWindow.visibility === Window.Windowed')
root.setProperty('playerContext',True)
root.setProperty('sourceTitle','My Songs');assert root.property('sourceDescription')=='Return to My Songs'
source_spy=QSignalSpy(root.sourceRequested);click(item('juxtopposedStripsourceFolder'));assert source_spy.count()==1
root.setProperty('sourceDescription','Show My Songs');assert item('juxtopposedStripsourceFolder').property('description')=='Show My Songs'
click(item('juxtopposedStripsourceFolder'));assert source_spy.count()==2
close=item('juxtopposedStripclose');home=item('juxtopposedStriplogo')
QTest.mouseMove(view,QPoint(view.width()//2,view.height()//2));close.forceActiveFocus(Qt.TabFocusReason);QTest.qWait(650)
def visible_help():return [obj.objectName() for obj in root.findChildren(QObject) if obj.objectName().startswith('juxtopposedStripTooltip') and obj.property('visible')]
assert visible_help()==['juxtopposedStripTooltipclose'],visible_help()
QTest.mouseMove(view,home.mapToScene(home.boundingRect().center()).toPoint());QTest.qWait(650)
assert close.hasActiveFocus(), 'Hover must not steal keyboard focus'
assert visible_help()==['juxtopposedStripTooltiplogo'],visible_help()
QTest.mouseMove(view,QPoint(view.width()//2,view.height()//2));QTest.qWait(650)
assert visible_help()==['juxtopposedStripTooltipclose'],visible_help()
view.close();app.processEvents()

# Exercise the real global shortcut component alongside the real title strip;
# only the player, dialog and native window holder boundaries are supplied.
shutil.copyfile(args.shortcut_qml or r/'modules/gui/qt/menus/qml/GlobalShortcuts.qml',m/'GlobalShortcutsUnderTest.qml')
shutil.copyfile(r/'modules/gui/qt/menus/qml/ShortcutExt.qml',m/'ShortcutExt.qml')
(m/'FullscreenShortcutFixture.qml').write_text('''import QtQuick
import QtQuick.Window
import VLC.MainInterface
import VLC.Player
import VLC.Dialogs
Item {
 width: 900; height: 60
 GlobalShortcutsUnderTest {}
 TitlebarUnderTest { objectName: "fullscreenTestTitlebar"; width: parent.width; playerContext: true }
}''',encoding='utf-8')
shortcut_view=QQuickView();shortcut_view.engine().addImportPath(str(m));shortcut_view.setSource(QUrl.fromLocalFile(str(m/'FullscreenShortcutFixture.qml')))
assert not shortcut_view.errors(),[error.toString() for error in shortcut_view.errors()]
shortcut_view.show();shortcut_view.requestActivate();QTest.qWait(100)
shortcut_root=shortcut_view.rootObject();assert shortcut_root
shortcut_strip=shortcut_root.findChild(QObject,'fullscreenTestTitlebar');assert shortcut_strip
shortcut_button=shortcut_strip.findChild(QObject,'juxtopposedStripfullscreen');assert shortcut_button
def shortcut_expr(code):
 e=QQmlExpression(qmlContext(shortcut_root),shortcut_root,code);value,_=e.evaluate();assert not e.hasError(),e.error().toString();return value
def f11():QTest.keyClick(shortcut_view,Qt.Key_F11);QTest.qWait(30)
def strip_click():QTest.mouseClick(shortcut_view,Qt.LeftButton,Qt.NoModifier,shortcut_button.mapToScene(shortcut_button.boundingRect().center()).toPoint());QTest.qWait(30)
shortcut_expr('MainCtx.playerView=true; MainCtx.hasEmbededVideo=true; Player.hasVideoOutput=true')
# Reproduced defect: F11 entry must use the same VIDEO route that native
# leave-fullscreen actions release. The latter is simulated at this boundary.
f11();assert shortcut_expr('Player.fullscreen && !MainCtx.interfaceFullscreen'), 'active embedded F11 must acquire VIDEO, not INTERFACE'
shortcut_expr('Player.setFullscreen(false)')
assert shortcut_expr('MainCtx.intfMainWindow.visibility === Window.Windowed')
# Entry via F11 and exit via strip, then the inverse route.
f11();strip_click();assert shortcut_expr('!Player.fullscreen && !MainCtx.interfaceFullscreen && MainCtx.intfMainWindow.visibility === Window.Windowed')
strip_click();assert shortcut_expr('Player.fullscreen && !MainCtx.interfaceFullscreen')
before=shortcut_expr('MainCtx.fullscreenCalls + Player.toggleCalls')
f11();assert shortcut_expr('MainCtx.fullscreenCalls + Player.toggleCalls')==before+1
assert shortcut_expr('!Player.fullscreen && !MainCtx.interfaceFullscreen && MainCtx.intfMainWindow.visibility === Window.Windowed')
# Home entered while VIDEO is held should allow F11 to release VIDEO.
f11();shortcut_expr('MainCtx.playerView=false')
f11();assert shortcut_expr('!Player.fullscreen && !MainCtx.interfaceFullscreen')
# The visible Home strip also releases F11-acquired VIDEO on return home.
shortcut_expr('MainCtx.playerView=true');f11()
shortcut_expr('MainCtx.playerView=false');shortcut_strip.setProperty('playerContext',False)
strip_click();assert shortcut_expr('!Player.fullscreen && !MainCtx.interfaceFullscreen')
shortcut_strip.setProperty('playerContext',True)
# Windowed Home with video in the background preserves INTERFACE semantics.
f11();assert shortcut_expr('MainCtx.interfaceFullscreen && !Player.fullscreen')
f11();assert shortcut_expr('!MainCtx.interfaceFullscreen')
# A pre-existing INTERFACE hold must be released first, without creating VIDEO.
f11();shortcut_expr('MainCtx.playerView=true')
f11();assert shortcut_expr('!MainCtx.interfaceFullscreen && !Player.fullscreen')
# Both holders: release the independent INTERFACE owner, then VIDEO.
shortcut_expr('MainCtx.toggleInterfaceFullScreen(); Player.toggleFullscreen()')
f11();assert shortcut_expr('!MainCtx.interfaceFullscreen && Player.fullscreen && MainCtx.intfMainWindow.visibility === Window.FullScreen')
f11();assert shortcut_expr('!Player.fullscreen && MainCtx.intfMainWindow.visibility === Window.Windowed')
# Normal Home/audio and detached-video paths keep their interface action.
shortcut_expr('MainCtx.hasEmbededVideo=false; Player.hasVideoOutput=false')
f11();assert shortcut_expr('MainCtx.interfaceFullscreen && !Player.fullscreen')
f11();assert shortcut_expr('!MainCtx.interfaceFullscreen')
shortcut_expr('Player.hasVideoOutput=true; Player.setFullscreen(true)')
f11();assert shortcut_expr('MainCtx.interfaceFullscreen && Player.fullscreen')
f11();assert shortcut_expr('!MainCtx.interfaceFullscreen && Player.fullscreen')
shortcut_expr('Player.setFullscreen(false)')
# Traditional controls retain their existing interface shortcut policy.
shortcut_expr('MainCtx.juxtopposedControls=false; MainCtx.hasEmbededVideo=true; Player.setFullscreen(true)')
f11();assert shortcut_expr('!MainCtx.interfaceFullscreen && Player.fullscreen')
shortcut_expr('Player.setFullscreen(false)')
# Shared modals, text input and the global-shortcuts switch still own F11.
shortcut_expr('MainCtx.juxtopposedControls=true; Player.setFullscreen(true); DialogsProvider.qmlDialogVisible=true')
before=shortcut_expr('MainCtx.fullscreenCalls + Player.toggleCalls')
f11();assert shortcut_expr('MainCtx.fullscreenCalls + Player.toggleCalls')==before
assert shortcut_expr('Player.fullscreen')
shortcut_expr('DialogsProvider.qmlDialogVisible=false; DialogsProvider.qmlInputFocused=true')
f11();assert shortcut_expr('MainCtx.fullscreenCalls + Player.toggleCalls')==before
assert shortcut_expr('Player.fullscreen')
shortcut_expr('DialogsProvider.qmlInputFocused=false; MainCtx.useGlobalShortcuts=false')
f11();assert shortcut_expr('MainCtx.fullscreenCalls + Player.toggleCalls')==before
assert shortcut_expr('Player.fullscreen')
shortcut_expr('MainCtx.useGlobalShortcuts=true')
f11();assert shortcut_expr('!Player.fullscreen && MainCtx.intfMainWindow.visibility === Window.Windowed')
shortcut_view.close();app.processEvents();bad=[message for message in messages if any(term in message for term in ['Error','Cannot','Unable','Binding loop','is not a type','Invalid'])];assert not bad,bad
(args.output_dir/'verification.json').write_text(json.dumps(dict(passed=True,geometryCases=checks,tooltipHoverPriority=True,keyboardTooltipResumes=True,fullscreenHolderRouting=True,globalF11Routing=True,controllerSeamsOnly=True,nativeWindowAcceptance=False,messages=messages),indent=2)+'\n')
print('Original title-strip12 geometry cases plus focus/controller-seam checks pass; native window actions remain separate.')
