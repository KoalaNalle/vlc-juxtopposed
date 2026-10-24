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
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output-dir',type=Path,required=True);parser.add_argument('--qml-file',type=Path);args=parser.parse_args();args.output_dir.mkdir(exist_ok=False)
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
 property QtObject intfMainWindow: QtObject { property int visibility: Window.Windowed; property int width: 1920; property int height: 1080 }
 property QtObject minimumButton: QtObject { property int type: 0; property rect rect; property bool showHovered: false; property bool externalPressed: false; function click() {ctx.minimizeCalls++} }
 property QtObject maximumButton: QtObject { property int type: 1; property rect rect; property bool showHovered: false; property bool externalPressed: false; function click() {ctx.maximizeCalls++; ctx.intfMainWindow.visibility=ctx.intfMainWindow.visibility===Window.Maximized?Window.Windowed:Window.Maximized} }
 property QtObject closeButton: QtObject { property int type: 2; property rect rect; property bool showHovered: false; property bool externalPressed: false; function click() {ctx.closeCalls++} }
 property QtObject csdButtonModel: QtObject { property var windowCSDButtons: [ctx.minimumButton,ctx.maximumButton,ctx.closeButton] }
 function toggleInterfaceFullScreen() { fullscreenCalls++; intfMainWindow.visibility=intfMainWindow.visibility===Window.FullScreen?Window.Windowed:Window.FullScreen }
}''')
assets=m/'assets';assets.mkdir();qrc=ElementTree.parse(r/'modules/gui/qt/assets.qrc').getroot()
for resource in qrc.findall('qresource'):
 if resource.get('prefix')!='/juxtopposed':continue
 for file in resource.findall('file'):
  alias=file.get('alias','')
  if alias=='InterVariable.ttf':font_url=QUrl.fromLocalFile(str(r/'modules/gui/qt'/file.text)).toString()
  if alias.startswith('titlebar-'):shutil.copyfile(r/'modules/gui/qt'/file.text,assets/alias)
source=(args.qml_file or r/'modules/gui/qt/maininterface/qml/JuxtopposedTitlebar.qml').read_text().replace('qrc:/juxtopposed/InterVariable.ttf',font_url).replace('qrc:/juxtopposed/',QUrl.fromLocalFile(str(assets)+'/').toString())
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
close=item('juxtopposedStripclose');home=item('juxtopposedStriplogo')
QTest.mouseMove(view,QPoint(view.width()//2,view.height()//2));close.forceActiveFocus(Qt.TabFocusReason);QTest.qWait(650)
def visible_help():return [obj.objectName() for obj in root.findChildren(QObject) if obj.objectName().startswith('juxtopposedStripTooltip') and obj.property('visible')]
assert visible_help()==['juxtopposedStripTooltipclose'],visible_help()
QTest.mouseMove(view,home.mapToScene(home.boundingRect().center()).toPoint());QTest.qWait(650)
assert close.hasActiveFocus(), 'Hover must not steal keyboard focus'
assert visible_help()==['juxtopposedStripTooltiplogo'],visible_help()
QTest.mouseMove(view,QPoint(view.width()//2,view.height()//2));QTest.qWait(650)
assert visible_help()==['juxtopposedStripTooltipclose'],visible_help()
view.close();app.processEvents();bad=[message for message in messages if any(term in message for term in ['Error','Cannot','Unable','Binding loop','is not a type','Invalid'])];assert not bad,bad
(args.output_dir/'verification.json').write_text(json.dumps(dict(passed=True,geometryCases=checks,tooltipHoverPriority=True,keyboardTooltipResumes=True,controllerSeamsOnly=True,nativeWindowAcceptance=False,messages=messages),indent=2)+'\n')
print('Original title-strip12 geometry cases plus focus/controller-seam checks pass; native window actions remain separate.')
