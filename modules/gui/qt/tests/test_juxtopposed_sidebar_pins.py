#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Actual sidebar QML with an explicit supplied pin model; native acceptance is separate."""
import argparse
import json
import os
import re
from pathlib import Path
import shutil
import tempfile

root_dir = Path(__file__).resolve().parents[4]
os.environ.update(QT_QPA_PLATFORM='offscreen', QT_QUICK_BACKEND='software', QML_DISABLE_DISK_CACHE='1', QT_QPA_FONTDIR=str(root_dir/'modules/gui/qt/pixmaps/juxtopposed'))
from PySide6.QtCore import QAbstractListModel, QCoreApplication, QModelIndex, QPoint, QPointF, Property, Signal, Slot, Qt, QUrl, qInstallMessageHandler
from PySide6.QtGui import QGuiApplication, QFontDatabase, QWheelEvent
from PySide6.QtQml import QQmlExpression, qmlContext
from PySide6.QtQuick import QQuickView
from PySide6.QtTest import QSignalSpy, QTest
from juxtopposed_palette_fixture import register_juxtopposed_palette

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output-dir',type=Path,required=True)
parser.add_argument('--qml-file',type=Path,help='Immutable negative-control component; defaults to production QML')
args=parser.parse_args()
args.output_dir.mkdir(parents=True,exist_ok=False)
app=QGuiApplication([])
messages=[]
qInstallMessageHandler(lambda kind,context,message: messages.append(message))
register_juxtopposed_palette()
font_id=QFontDatabase.addApplicationFont(str(root_dir/'modules/gui/qt/pixmaps/VLCIcons.ttf'))
assert font_id>=0
icon_family=QFontDatabase.applicationFontFamilies(font_id)[0]
icons=(root_dir/'modules/gui/qt/style/vlcicons.hpp').read_text(encoding='utf-8')
def icon(name):
    match=re.search(r"QChar " + name + r"\{ u'\\u([a-f0-9]{4})' \}",icons)
    assert match,name
    return chr(int(match[1],16))

class SuppliedPins(QAbstractListModel):
    stateChanged=Signal()
    countChanged=Signal()
    def __init__(self):
        super().__init__()
        self.rows=[dict(pinKey=f'owned-{i}',kind=['media','folder','playlist','artist','source'][i%5],
                        title=f'Owned pin Ää {i}',artwork=QUrl(),available=i!=1) for i in range(60)]
        self.error_text=''
        self.retry_pending=False
        self.retry_count=0
        self.roles={Qt.UserRole+1+i:name for i,name in enumerate(('pinKey','kind','title','artwork','available'))}
    def rowCount(self,parent=QModelIndex()): return 0 if parent.isValid() else len(self.rows)
    def roleNames(self): return {role:name.encode() for role,name in self.roles.items()}
    def data(self,index,role=Qt.DisplayRole):
        return self.rows[index.row()].get(self.roles.get(role,'')) if index.isValid() and 0<=index.row()<len(self.rows) else None
    count=Property(int,lambda self:len(self.rows),notify=countChanged)
    error=Property(str,lambda self:self.error_text,notify=stateChanged)
    canRetry=Property(bool,lambda self:self.retry_pending,notify=stateChanged)
    @Slot(int,result='QVariantMap')
    def get(self,row): return dict(self.rows[row],key=self.rows[row]['pinKey'])
    @Slot(str,int,result=bool)
    def move(self,key,destination):
        source=next((i for i,row in enumerate(self.rows) if row['pinKey']==key),-1)
        if source<0 or destination<0 or destination>=len(self.rows): return False
        if source==destination: return True
        self.beginMoveRows(QModelIndex(),source,source,QModelIndex(),destination+1 if destination>source else destination)
        self.rows.insert(destination,self.rows.pop(source)); self.endMoveRows(); return True
    @Slot()
    def retry(self): self.retry_count+=1;self.retry_pending=False;self.error_text='';self.stateChanged.emit()
    @Slot()
    def dismissError(self): self.retry_pending=False;self.error_text='';self.stateChanged.emit()

model=SuppliedPins()
module=tempfile.TemporaryDirectory(prefix='vlc-sidebar-widgets-')
widgets=Path(module.name)/'VLC/Widgets';widgets.mkdir(parents=True)
shutil.copyfile(root_dir/'modules/gui/qt/widgets/qml/ImageExt.qml',widgets/'ImageExt.qml')
(widgets/'qmldir').write_text('module VLC.Widgets\nImageExt 1.0 ImageExt.qml\n',encoding='utf-8')
util = widgets.parent / 'Util'
util.mkdir()
(util / 'qmldir').write_text('module VLC.Util\nsingleton SVGColorImage 1.0 SVGColorImage.qml\n', encoding='utf-8')
(util / 'SVGColorImage.qml').write_text('pragma Singleton\nimport QtQml\nQtObject { function colorize(path) { return { any: function(colors) { return this }, uri: function() { return path } } } }\n', encoding='utf-8')
view=QQuickView();view.engine().addImportPath(module.name)
view.setResizeMode(QQuickView.SizeRootObjectToView)
view.setInitialProperties(dict(pinModel=model,iconFontFamily=icon_family,addIcon=icon('add'),preferencesIcon=icon('settings'),pinIcons={kind:icon(name) for kind,name in dict(media='play',folder='topbar_discover',source='topbar_network',playlist='playlist',artist='topbar_music').items()},routes=[dict(key='video',text='Videos',icon=icon('topbar_video'),uri=['video','all']),dict(key='network',text='Network',icon=icon('topbar_network'),uri=['network'])],assetBase=QUrl.fromLocalFile(str(root_dir/'modules/gui/qt/pixmaps/juxtopposed')+'/')))
view.setSource(QUrl.fromLocalFile(str(args.qml_file or root_dir/'modules/gui/qt/maininterface/qml/JuxtopposedNavigation.qml')))
assert not view.errors(),[error.toString() for error in view.errors()]
root=view.rootObject();view.resize(217,360);root.setProperty('expanded',True);view.show();QTest.qWait(100)
checks=[]
def check(value,name):
    checks.append(name)
    if not value:
        active=view.activeFocusItem()
        details=dict(passed=False,failed=name,checks=checks,messages=messages,
                     activeFocus=active.objectName() if active else None)
        (args.output_dir/'verification.json').write_text(json.dumps(details,indent=2)+'\n',encoding='utf-8')
        view.grabWindow().save(str(args.output_dir/'failed.png'))
    assert value,name

def expr(code):
    expression=QQmlExpression(qmlContext(root),root,code)
    value,_=expression.evaluate()
    assert not expression.hasError(),expression.error().toString()
    return value

def item(name):
    pending=[root]
    while pending:
        current=pending.pop()
        if current.objectName()==name:return current
        pending.extend(current.childItems())
    raise AssertionError(name)

def focus(index):
    expr(f'focusPin({index}, Qt.TabFocusReason)');QTest.qWait(40)
    return item(f'juxtopposedSidebarPin{index}')

activate=QSignalSpy(root.pinRequested);context=QSignalSpy(root.pinMenuRequested);add=QSignalSpy(root.addRequested)
row=focus(0);QTest.keyClick(view,Qt.Key_Space)
check(activate.count()==1 and activate.at(0)[0]=='owned-0','actual pin button dispatches stable identity')
row=focus(1);check(row.isEnabled(),'unavailable pin remains focusable and removable')
QTest.keyClick(view,Qt.Key_F10,Qt.ShiftModifier)
check(context.count()==1 and context.at(0)[0]=='owned-1','keyboard context menu dispatches unavailable stable pin')
row=focus(59);key=model.rows[59]['pinKey'];QTest.keyClick(view,Qt.Key_Up,Qt.ControlModifier);QTest.qWait(50)
check(model.rows[58]['pinKey']==key,'actual Control-Up dispatches model reorder')
check(item('juxtopposedSidebarPin58').hasActiveFocus(),'reorder retains keyboard focus on same identity')
check(item('juxtopposedSidebarPin58').property('visualFocus'),'keyboard reorder retains the visible focus outline')
row=focus(59);QTest.keyClick(view,Qt.Key_Down);QTest.qWait(50)
check(item('juxtopposedSidebarAdd').hasActiveFocus(),'virtualized last row navigates to Add item')
QTest.keyClick(view,Qt.Key_Space)
check(add.count()==1,'actual Add item activates by keyboard')
model.error_text='Owned save failed. Restore preferences and retry.';model.retry_pending=True;model.stateChanged.emit()
focus(59);QTest.keyClick(view,Qt.Key_Down);QTest.qWait(40)
retry=item('juxtopposedSidebarRetry')
check(retry.hasActiveFocus() and not item('juxtopposedSidebarAdd').isEnabled(),'error arrows skip disabled Add and reach Retry')
QTest.keyClick(view,Qt.Key_Down);QTest.qWait(40)
check(item('juxtopposedSidebarDismiss').hasActiveFocus(),'error arrows reach Dismiss after Retry')
QTest.keyClick(view,Qt.Key_Up);QTest.qWait(40)
check(retry.hasActiveFocus(),'error reverse arrow returns to Retry')
QTest.keyClick(view,Qt.Key_Space);QTest.qWait(40)
check(model.retry_count==1 and not model.retry_pending,'visible Retry action invokes exact supplied operation')
check(item('juxtopposedSidebarAdd').hasActiveFocus(),'successful Retry restores enabled Add focus')
check(item('juxtopposedSidebarAdd').property('visualFocus'),'keyboard Retry restores a visible Add focus outline')
model.error_text='Owned save failed again.';model.retry_pending=True;model.stateChanged.emit()
focus(59);QTest.keyClick(view,Qt.Key_Down);QTest.keyClick(view,Qt.Key_Down);QTest.qWait(40)
check(item('juxtopposedSidebarDismiss').hasActiveFocus(),'second recoverable failure retains keyboard access to Dismiss')
QTest.keyClick(view,Qt.Key_Space);QTest.qWait(40)
check(not model.retry_pending and not model.error_text and item('juxtopposedSidebarAdd').hasActiveFocus(),
      'Dismiss clears the error and restores enabled Add focus')
check(item('juxtopposedSidebarAdd').property('visualFocus'),'keyboard Dismiss restores a visible Add focus outline')
check(len(context.at(0))==3 and context.at(0)[2]==Qt.ShortcutFocusReason.value,'keyboard context request preserves the keyboard focus reason')
for expanded,width in ((True,217),(False,50)):
    root.setProperty('expanded',expanded);view.resize(width,260)
    for index in (0,30,59):
        row=focus(index);point=row.mapToScene(row.boundingRect().center())
        check(view.size().width()>point.x()>=0 and view.size().height()>point.y()>=0,f'{width}px rail focuses virtual pin {index} inside viewport')
    QTest.keyClick(view,Qt.Key_Down);QTest.qWait(40)
    check(item('juxtopposedSidebarAdd').hasActiveFocus(),f'{width}px rail reaches visible Add item')
    last=item('juxtopposedSidebarPin59');add_item=item('juxtopposedSidebarAdd')
    gap=add_item.mapToScene(add_item.boundingRect().topLeft()).y()-last.mapToScene(last.boundingRect().bottomLeft()).y()
    check(abs(gap-14)<0.1,f'{width}px rail preserves original14px gap before Add item')
    check(view.grabWindow().save(str(args.output_dir/f'sidebar-{width}.png')),f'{width}px rail captured')
for expanded,width in ((True,217),(False,50)):
    root.setProperty('expanded',expanded);view.resize(width,260);QTest.qWait(40)
    listing=item('juxtopposedSidebarPinList')
    def wheel(point,angle=QPoint(0,-120),pixels=QPoint()):
        event=QWheelEvent(point,view.mapToGlobal(point.toPoint()),pixels,angle,
                          Qt.NoButton,Qt.NoModifier,Qt.NoScrollPhase,False)
        QCoreApplication.sendEvent(view,event);QTest.qWait(250)
        return event.isAccepted()
    for padding in (False,True):
        expr('pinList.positionViewAtBeginning(); pinList.forceLayout()')
        before=listing.property('contentY')
        point=root.mapToScene(QPointF(2,root.height()/2)) if padding else listing.mapToScene(listing.boundingRect().center())
        accepted=wheel(point)
        check(accepted and listing.property('contentY')>before+.5,
              f'{width}px rail scrolls actual rows or padding, padding={padding}')
    padding_point=root.mapToScene(QPointF(2,root.height()/2))
    expr('pinList.cancelFlick(); pinList.positionViewAtBeginning(); pinList.forceLayout()')
    before=listing.property('contentY')
    accepted=wheel(padding_point,QPoint(),QPoint(0,-23))
    check(accepted and abs(listing.property('contentY')-before-23)<.51,
          f'{width}px rail honors pixel wheel distance over padding')
    before=listing.property('contentY')
    check(wheel(padding_point,QPoint(120,0)) and abs(listing.property('contentY')-before)<.51,
          f'{width}px rail consumes horizontal padding wheel without vertical movement')
    expr('pinList.contentY = Math.max(pinList.originY, pinList.originY + pinList.contentHeight - pinList.height); pinList.forceLayout()')
    before=listing.property('contentY')
    check(wheel(padding_point) and abs(listing.property('contentY')-before)<.51,
          f'{width}px rail consumes padding wheel at the end without overshooting')
model.beginResetModel();model.rows=[];model.endResetModel();model.countChanged.emit()
root.setProperty('routes',[dict(key=name,text=name,icon='X',uri=[name],enabled=True)
    for name in ('home','video','music','network','playlists','recent')])
for expanded,width in ((True,217),(False,50)):
    root.setProperty('expanded',expanded);view.resize(width,226);QTest.qWait(100)
    expr('focusFooter(Qt.PopupFocusReason)');QTest.qWait(100)
    listing=item('juxtopposedSidebarPinList');add_item=item('juxtopposedSidebarAdd')
    def footer_visible():
        bounds=add_item.mapRectToItem(listing,add_item.boundingRect())
        return add_item.hasActiveFocus() and bounds.top()>=-.51 and bounds.bottom()<=listing.height()+.51
    check(footer_visible(),f'{width}px empty rail shows its focused Add fallback')
    model.error_text='Owned empty-store failure.';model.retry_pending=True;model.stateChanged.emit();QTest.qWait(100)
    expr('focusFooter(Qt.TabFocusReason)');QTest.qWait(60)
    QTest.keyClick(view,Qt.Key_Down);QTest.qWait(60)
    check(item('juxtopposedSidebarDismiss').hasActiveFocus(),f'{width}px empty rail reaches Dismiss')
    QTest.keyClick(view,Qt.Key_Space);QTest.qWait(150)
    check(footer_visible(),f'{width}px empty rail keeps Add visible after error footer collapses')
    view.resize(width,200);QTest.qWait(100)
    check(footer_visible(),f'{width}px empty rail keeps focused Add visible after resize')
    check(view.grabWindow().save(str(args.output_dir/f'sidebar-empty-{width}.png')),f'{width}px empty rail captured after Dismiss')
check(not messages,'no Qt/QML diagnostics')
(args.output_dir/'verification.json').write_text(json.dumps(dict(passed=True,checks=checks,suppliedModel=True,nativeAcceptance=False,originalFidelityAccepted=False),indent=2)+'\n',encoding='utf-8')
print(f'Passed {len(checks)} sidebar component checks; native behavior and original fidelity remain separate.')
