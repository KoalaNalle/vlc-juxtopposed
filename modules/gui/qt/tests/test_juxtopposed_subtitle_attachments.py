# SPDX-License-Identifier: GPL-2.0-or-later
"""Exercise production attachment QML with supplied state; native scope is separate."""
from pathlib import Path
import argparse,json,os,sys,hashlib
repo=Path(__file__).resolve().parents[4]
os.environ.update(QT_QPA_PLATFORM='offscreen',QT_QUICK_BACKEND='software',QML_DISABLE_DISK_CACHE='1',
                  QT_QPA_FONTDIR=str(repo/'modules/gui/qt/pixmaps/juxtopposed'))
sys.path.insert(0,str(repo/'modules/gui/qt/tests'))
from PySide6.QtCore import QObject,Property,Signal,Slot,Qt,QUrl,QPoint,qInstallMessageHandler
from PySide6.QtGui import QGuiApplication,QAccessible
from PySide6.QtQml import QQmlApplicationEngine,QQmlExpression,qmlContext
from PySide6.QtQuick import QQuickItem
from PySide6.QtTest import QTest
parser=argparse.ArgumentParser();parser.add_argument('--output-dir',type=Path,required=True);args=parser.parse_args()
args.output_dir.mkdir(parents=True,exist_ok=False)
messages=[];qInstallMessageHandler(lambda kind,ctx,msg:messages.append(msg))
class Controller(QObject):
    changed=Signal()
    def __init__(self):
        super().__init__();self._active=False;self._scope=0;self.pairs=[];self._error='';self.actions=[];self.next_error=''
    active=Property(bool,lambda s:s._active,notify=changed)
    editable=Property(bool,lambda s:s._active,notify=changed)
    scope=Property(int,lambda s:s._scope,notify=changed)
    canSave=Property(bool,lambda s:s._active and bool(s.pairs),notify=changed)
    folderAvailable=Property(bool,lambda s:True,constant=True)
    playlistAvailable=Property(bool,lambda s:True,constant=True)
    sourceLabel=Property(str,lambda s:'Videos / Shows / The Wand / S01',constant=True)
    error=Property(str,lambda s:s._error,notify=changed)
    @Property('QVariantList',notify=changed)
    def mediaRows(self):
        return [dict(title=f'The_Wand_S01E{i+1:02}.mp4',subtitles=[f'The_Wand_S01E{j+1:02}.srt' for row,j in self.pairs if row==i],count=sum(row==i for row,j in self.pairs)) for i in range(1 if self._scope==0 else 3)]
    @Property('QVariantList',notify=changed)
    def subtitleRows(self):
        return [dict(title=f'The_Wand_S01E{j+1:02}.srt',available=True,selectedRows=[row for row,sub in self.pairs if sub==j]) for j in range(3)]
    @Slot(result=bool)
    def begin(self):self._active=True;self._scope=0;self.pairs=[];self._error='';self.actions.append(['begin']);self.changed.emit();return True
    @Slot()
    def cancel(self):self._active=False;self.actions.append(['cancel']);self.changed.emit()
    @Slot(int)
    def setScope(self,value):self._scope=value;self.pairs=[];self.actions.append(['scope',value]);self.changed.emit()
    @Slot(int,int)
    def toggle(self,row,sub):
        pair=(row,sub)
        if pair in self.pairs:self.pairs.remove(pair)
        else:self.pairs.append(pair)
        self.actions.append(['toggle',row,sub]);self.changed.emit()
    @Slot()
    def matchNames(self):self.pairs=[(0,0),(1,1),(2,2)];self.actions.append(['match']);self.changed.emit()
    @Slot(result=bool)
    def save(self):
        self.actions.append(['save'])
        if self.next_error:self._error=self.next_error;self.next_error='';self.changed.emit();return False
        self._active=False;self.changed.emit();return True
app=QGuiApplication([])
from juxtopposed_palette_fixture import register_juxtopposed_palette
register_juxtopposed_palette()
controller=Controller();engine=QQmlApplicationEngine();engine.rootContext().setContextProperty('attachmentController',controller)
qml_dir=QUrl.fromLocalFile(str(repo/'modules/gui/qt/dialogs/dialogs/qml')).toString()
asset=QUrl.fromLocalFile(str(repo/'modules/gui/qt/pixmaps/juxtopposed')+'/').toString()
driver=f'''import QtQuick
import QtQuick.Controls.Basic as C
import "{qml_dir}" as Dialogs
C.ApplicationWindow {{
 id:window; visible:true; width:1120; height:880; color:"#202020"
 property int fileRequests:0; property int folderRequests:0; property int onlineRequests:0
 C.Button {{id:opener; objectName:"opener";text:"Add subtitles";x:10;y:10;onClicked:dialog.showDialog(opener)}}
 Dialogs.JuxtopposedSubtitleAttachments {{id:dialog;parent:C.Overlay.overlay;controller:attachmentController;assetBase:"{asset}"
 onFilesRequested:window.fileRequests++;onFolderRequested:window.folderRequests++;onOnlineRequested:window.onlineRequests++}}
}}'''
engine.loadData(driver.encode(),QUrl.fromLocalFile(str(args.output_dir.resolve()/'driver.qml')))
(args.output_dir/'qml-messages.json').write_text(json.dumps(messages,indent=2)+'\n',encoding='utf-8')
assert engine.rootObjects(),messages
window=engine.rootObjects()[0];dialog=window.findChild(QObject,'juxtopposedSubtitleAttachments');assert dialog,messages
def evaluate(target,code):
    expr=QQmlExpression(qmlContext(target),target,code);value,_=expr.evaluate()
    assert not expr.hasError(),expr.error().toString()
    return value
def item(name):
    obj=window.findChild(QObject,name)
    if obj:return obj
    def visual(node):
        if node.objectName()==name:return node
        for child in node.childItems():
            found=visual(child)
            if found:return found
    obj=visual(window.contentItem());assert obj,name;return obj
def click_obj(obj):
    QTest.mouseClick(window,Qt.LeftButton,Qt.NoModifier,obj.mapToScene(obj.boundingRect().center()).toPoint());QTest.qWait(40)
def click(name):click_obj(item(name))
def sub(index):return item(f'subtitleAttachmentSubtitle{index}')
def media(index):return item(f'subtitleAttachmentMedia{index}')
def screenshot(name):assert window.grabWindow().save(str(args.output_dir/name))
def guard_messages():
    errors=[m for m in messages if any(x in m for x in ['Error:','Unable to assign','Cannot assign','Cannot read','is not defined','Binding loop','Failed to load'])]
    assert not errors,errors
try:
    QTest.qWait(100);click('opener');assert dialog.property('visible') and item('subtitleAttachmentScope').hasActiveFocus()
    assert not item('subtitleAttachmentSave').property('enabled')
    click_obj(sub(0));click_obj(sub(1));assert controller.pairs==[(0,0),(0,1)]
    assert item('subtitleAttachmentSave').property('enabled')
    screenshot('file-local.png')
    controller.next_error='Settings cannot be saved. Check the settings folder and try again.'
    click('subtitleAttachmentSave');assert dialog.property('visible') and item('subtitleAttachmentError').property('visible')
    assert controller.pairs==[(0,0),(0,1)]
    screenshot('save-error.png');click('subtitleAttachmentSave');assert not dialog.property('visible')
    QTest.qWait(30);assert item('opener').hasActiveFocus()
    click('opener');click('subtitleAttachmentChooseFiles');click('subtitleAttachmentFolder')
    assert window.property('fileRequests')==1 and window.property('folderRequests')==1
    QTest.keyClick(window,Qt.Key_Escape);QTest.qWait(40);assert not dialog.property('visible') and not controller._active
    click('opener');choice=item('subtitleAttachmentScope');choice.forceActiveFocus(Qt.TabFocusReason)
    QTest.keyClick(window,Qt.Key_Space);QTest.qWait(30)
    for index,title in enumerate(['To This File','To This Folder','To This Playlist']):
        interface=QAccessible.queryAccessibleInterface(item(f'subtitleAttachmentScopeOption{index}'))
        assert interface and interface.text(QAccessible.Text.Name)==title,(index,title)
    QTest.keyClick(window,Qt.Key_Down);QTest.keyClick(window,Qt.Key_Return);QTest.qWait(50)
    assert controller._scope==1,controller.actions
    click('subtitleAttachmentMatchNames');assert controller.pairs==[(0,0),(1,1),(2,2)]
    screenshot('folder-aligned.png')
    click_obj(media(1));click_obj(sub(1));click_obj(sub(2))
    assert (1,1) not in controller.pairs and (1,2) in controller.pairs
    screenshot('folder-reassigned.png')
    # A real pointer drag must update the assigned media, not just paint a line.
    source=sub(0);target=media(2)
    start=source.mapToScene(source.boundingRect().center()).toPoint();end=target.mapToScene(target.boundingRect().center()).toPoint()
    QTest.mousePress(window,Qt.LeftButton,Qt.NoModifier,start)
    for step in range(1,13):QTest.mouseMove(window,QPoint(start.x()+(end.x()-start.x())*step//12,start.y()+(end.y()-start.y())*step//12),25);QTest.qWait(20)
    QTest.mouseRelease(window,Qt.LeftButton,Qt.NoModifier,end);QTest.qWait(80)
    assert (2,0) in controller.pairs,controller.actions
    # Escape from an open choice closes only that choice before the dialog.
    choice.forceActiveFocus(Qt.TabFocusReason);QTest.keyClick(window,Qt.Key_Space);QTest.qWait(30)
    QTest.keyClick(window,Qt.Key_Escape);QTest.qWait(30);assert dialog.property('visible')
    QTest.keyClick(window,Qt.Key_Escape);QTest.qWait(30);assert not dialog.property('visible')
    window.resize(602,421);QTest.qWait(50);click('opener');click_obj(sub(0))
    controller.next_error='A selected subtitle file is missing. Restore it or choose another file, then try again.'
    click('subtitleAttachmentSave');QTest.qWait(60)
    for name in ['subtitleAttachmentSave','subtitleAttachmentCancel','subtitleAttachmentError']:
        obj=item(name);p=obj.mapToScene(obj.boundingRect().topLeft());assert p.y()>=0 and p.y()+obj.height()<=window.height()+1,(name,p,obj.height())
    error_interface=QAccessible.queryAccessibleInterface(item('subtitleAttachmentError'))
    assert error_interface and error_interface.text(QAccessible.Text.Name)==controller._error
    screenshot('short-window-error.png')
    QTest.keyClick(window,Qt.Key_Escape);QTest.qWait(30)
    guard_messages()
    (args.output_dir/'verification.json').write_text(json.dumps(dict(status='pass',suppliedStateComponent=True,ordinaryGuiAccepted=False,fullBetaAccepted=False,actions=controller.actions,messages=messages),indent=2)+'\n',encoding='utf-8')
    print('Attachment component checks passed',flush=True)
finally:
    (args.output_dir/'qml-messages.json').write_text(json.dumps(messages,indent=2)+'\n',encoding='utf-8')
    window.close();QTest.qWait(30)
