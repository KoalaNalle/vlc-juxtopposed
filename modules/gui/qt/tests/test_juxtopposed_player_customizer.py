# SPDX-License-Identifier: GPL-2.0-or-later
"""Actual draft customizer and shared preview with owned service contracts.

Pointer/keyboard/component checks only. Native C++ persistence/media and visual
fidelity are separately verified with the opt-in native app journeys.
"""
from pathlib import Path
from hashlib import sha256
import os,json,argparse
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--repo-root',type=Path,default=Path(__file__).resolve().parents[4])
parser.add_argument('--output-dir',type=Path,required=True)
args=parser.parse_args();repo=args.repo_root.resolve();qt=repo/'modules/gui/qt'
os.environ.update(QT_QPA_PLATFORM='offscreen',QT_QUICK_BACKEND='software',QML_DISABLE_DISK_CACHE='1',QT_QPA_FONTDIR=str(qt/'pixmaps/juxtopposed'))
from PySide6.QtCore import QUrl,qInstallMessageHandler,Qt,QPoint,QPointF
from PySide6.QtGui import QGuiApplication,QFontDatabase,QAccessible,QAccessibleActionInterface
from PySide6.QtQml import qmlRegisterSingletonType,QQmlExpression,qmlContext
from PySide6.QtQuick import QQuickView
from PySide6.QtTest import QTest
messages=[];qInstallMessageHandler(lambda k,c,m:messages.append(m));app=QGuiApplication([])
QFontDatabase.addApplicationFont(str(qt/'pixmaps/juxtopposed/InterVariable.ttf'))
qmlRegisterSingletonType(QUrl.fromLocalFile(str(qt/'style/JuxtopposedPalette.qml')),'VLC.Style',1,0,'JuxtopposedPalette')
out=args.output_dir.resolve();out.mkdir(parents=True,exist_ok=False)
scene='''import QtQuick
import QtQuick.Controls.Basic
import "@PLAYER@" as Player
import "@DIALOGS@" as Dialogs
Item {
 id: root; width: 1280; height: 900
 property alias dialog: customizer
 property alias service: service
 property alias layout: layout
 property int actions: 0
 property bool nativeChecked: false
 Player.JuxtopposedLayoutRow {
  id: stateRow; visible:!customizer.visible; anchors.bottom:parent.bottom; width:parent.width;height:40
  columns:[[94,95],[],[]]; assetBase:"@ASSETS@"
  describe: id=>id===95?({name:"Owned Playlist Name",kind:"metadata",key:"playlist"}):({name:"Owned Favorite",kind:"button",glyph:"x",fontFamily:"Inter"})
  state: id=>({enabled:true,checked:root.nativeChecked})
  metadata: ({playlist:"Owned <mix> & Friends"})
  onActivated: root.actions++
 }
 function findName(item,name) {
  if(item.objectName===name)return item
  for(const child of item.children??[]) {const found=findName(child,name);if(found)return found}
  return null
 }
 QtObject {
  id: layout
  property bool editing: false
  property string lastError: ""
  property var draft: ({video: {columns: [[0],[],[1]], timelineAbove: false}, audio: {columns: [[2],[],[]], timelineAbove: true}})
  property var current: ({video: {columns: [[0],[],[1]], timelineAbove: false}, audio: {columns: [[2],[],[]], timelineAbove: true}})
  property var palette: Array.from({length:31}, function(_,n) {return ({id:n,name:"Owned tile " + n})})

  function cancelEdit() {editing=false}
  function tileInfo(id) {return ({name: "Owned tile " + id, kind: "button"})}
  function update(next) {draft=next}
  function insertTile(player,column,index,id) {const next=JSON.parse(JSON.stringify(draft)); next[player===0?"video":"audio"].columns[column].splice(index,0,id); update(next); return true}
  function moveTile(player,c,i,to,n) {const next=JSON.parse(JSON.stringify(draft)); const columns=next[player===0?"video":"audio"].columns;
   const id=columns[c].splice(i,1)[0];columns[to].splice(c===to&&n>i?n-1:n,0,id);update(next);return true}
  function removeTile(player,c,i) {const next=JSON.parse(JSON.stringify(draft));next[player===0?"video":"audio"].columns[c].splice(i,1);update(next);return true}
  function setTimelineAbove(player,above) {const next=JSON.parse(JSON.stringify(draft));next[player===0?"video":"audio"].timelineAbove=above;update(next);return true}
 }
 QtObject {
  id: service
  property var layout: root.layout
  property bool busy: false
  property bool available: true
  property string lastError: ""
  property bool fail: false
  property int saves: 0
  function beginLayoutEdit() {layout.draft=JSON.parse(JSON.stringify(layout.current));layout.editing=true;return true}
  function saveLayoutDraft() {saves++; if(fail){lastError="Owned save failure. Retry.";return false}layout.current=JSON.parse(JSON.stringify(layout.draft));layout.editing=false;lastError="";return true}
 }
 Button {id: opener; text: "Open"}
 Dialogs.JuxtopposedPlayerCustomizer {
  id: customizer; parent: root; controller: service; assetBase: "@ASSETS@"
  previewComponent: Component {
   Player.JuxtopposedPlaybackBar {
    previewOnly:true; editing:true; controlLayout:customizer.draftData
    describeTile:id=>layout.tileInfo(id); tileState:id=>({enabled:true,glyph:"P",fontFamily:"Inter"});assetBase:"@ASSETS@"
    onPlayPauseRequested:root.actions++
    onTileRequested:root.actions++
    onLayoutTilePressed:(column,index,tile,item,point)=>customizer.beginDrag(column,index,tile,item,point)
    onLayoutTileMoved:(item,point)=>customizer.moveDrag(item,point)
    onLayoutTileReleased:(item,point)=>customizer.endDrag(item,point)
    onLayoutSelectionRequested:(column,index)=>customizer.selection=({column:column,index:index})
    onLayoutRemovalRequested:(column,index)=>{customizer.selection=({column:column,index:index});customizer.removeSelection()}
   }
  }
 }
}'''.replace('@PLAYER@',(qt/'player/qml').as_uri()).replace('@DIALOGS@',(qt/'dialogs/dialogs/qml').as_uri()).replace('@ASSETS@',(qt/'pixmaps/juxtopposed').as_uri()+'/')
(out/'Scene.qml').write_text(scene)
view=QQuickView();view.setResizeMode(QQuickView.SizeRootObjectToView);view.setSource(QUrl.fromLocalFile(str(out/'Scene.qml')))
assert not view.errors(),[e.toString() for e in view.errors()]
view.show();view.requestActivate();QTest.qWait(60);root=view.rootObject();checks=[]
def evaluate(code):
 e=QQmlExpression(qmlContext(root),root,code);v=e.evaluate();assert not e.hasError(),e.error().toString()
 return v[0] if isinstance(v,tuple) else v
def check(code,label):
 passed=bool(evaluate(code))
 if not passed:
  (out/'failure.json').write_text(json.dumps(dict(passed=False,error=label,completedChecks=checks,diagnostics=messages),indent=2)+'\n',newline='\n')
 assert passed,label
 checks.append(label)
def act(code):evaluate(code);QTest.qWait(20)
act('dialog.showDialog(opener)')
check('dialog.visible && layout.editing && service.saves===0','opening creates a draft without saving')
check('layout.palette.length===31 && dialog.canSave','all palette entries are available to the draft editor')
act('dialog.selectedPaletteId=9; dialog.addSelected(0)')
check('layout.draft.video.columns[0][1]===9 && layout.current.video.columns[0].length===1','adding edits only the selected player draft')
act('dialog.changedPlayer(1); dialog.selectedPaletteId=7;dialog.addSelected(2)')
check('layout.draft.audio.columns[2][0]===7 && layout.draft.video.columns[0][1]===9','music and video drafts remain independent')
act('dialog.selection=({column:2,index:0});dialog.removeSelection()')
check('layout.draft.audio.columns[2].length===0','explicit removal edits the selected draft')
act('service.fail=true;dialog.save()')
check('dialog.visible && layout.editing && layout.current.video.columns[0].length===1 && service.saves===1','failed Save retains both accepted layout and editable draft')
act('service.fail=false;dialog.save()')
check('!dialog.visible && !layout.editing && layout.current.video.columns[0][1]===9 && service.saves===2','Save retry publishes the accepted draft once')
check('opener.activeFocus','closing restores the actual opener')
act('dialog.showDialog(opener);dialog.selectedPaletteId=12;dialog.addSelected(1)')
QTest.keyClick(view,Qt.Key_Escape);QTest.qWait(20)
check('!dialog.visible && layout.current.video.columns[1].length===0 && service.saves===2','Escape cancels draft changes without saving')
act('dialog.showDialog(opener);service.busy=true')
QTest.keyClick(view,Qt.Key_Escape);QTest.qWait(20)
check('dialog.visible && !dialog.canSave','a running operation prevents dismissal and competing Save')
act('service.busy=false;dialog.reject();dialog.showDialog(opener)')
def owned_item(code):
 item=evaluate(code)
 return item.toQObject() if hasattr(item,'toQObject') else item
def drag_item(source,target):
 start=source.mapToScene(QPointF(source.width()/2,source.height()/2)).toPoint()
 end=target
 QTest.mousePress(view,Qt.LeftButton,Qt.NoModifier,start)
 for n in range(1,9):QTest.mouseMove(view,start+(end-start)*n/8,10)
 QTest.mouseRelease(view,Qt.LeftButton,Qt.NoModifier,end);QTest.qWait(30)
palette_tile=owned_item('findName(dialog.contentItem,"customizerPaletteTile12")')
destination=owned_item('findName(dialog.contentItem,"customizerPreview").item.layoutRow.itemFor(0)')
drop=destination.mapToScene(QPointF(destination.width()*.8,destination.height()/2)).toPoint()
drag_item(palette_tile,drop)
check('layout.draft.video.columns[0][1]===12 && layout.current.video.columns[0].length===2 && actions===0','actual palette-to-player pointer drag inserts into draft without playback actions')
source_tile=owned_item('findName(dialog.contentItem,"customizerPreview").item.layoutRow.itemFor(12).parent')
drop=palette_tile.mapToScene(QPointF(palette_tile.width()/2,palette_tile.height()/2)).toPoint()
drag_item(source_tile,drop)
check('layout.draft.video.columns[0].length===2 && layout.draft.video.columns[0].indexOf(12)<0 && actions===0','actual player-to-palette pointer drag removes only the draft control')
original_draft=evaluate('JSON.stringify(layout.draft)')
act('const next=JSON.parse(JSON.stringify(layout.draft));next.video.columns[0]=Array(40).fill(1);layout.update(next)')
act('findName(dialog.contentItem,"juxtopposedCustomControls").contentX=200')
check('findName(dialog.contentItem,"juxtopposedCustomControls").contentX>100','overflow case has an actually scrolled player row')
viewport=owned_item('findName(dialog.contentItem,"juxtopposedCustomControls")')
drop=viewport.mapToScene(QPointF(-15,16)).toPoint()
drag_item(palette_tile,drop)
check('layout.draft.video.columns[0].length===40','clipped control-row padding is not a drop destination')
act(f'layout.update(JSON.parse({json.dumps(original_draft)}))')
act('findName(dialog.contentItem,"juxtopposedCustomControls").contentX=0')
selected=owned_item('findName(dialog.contentItem,"customizerPreview").item.layoutRow.itemFor(9).parent')
point=selected.mapToScene(QPointF(selected.width()/2,selected.height()/2)).toPoint()
QTest.mouseClick(view,Qt.LeftButton,Qt.NoModifier,point);QTest.qWait(20)
check('findName(dialog.contentItem,"customizerPreview").item.layoutRow.itemFor(9).parent.activeFocus && dialog.selection.index===1',
      'pointer selection gives the actual editable player tile keyboard focus')
assert view.grabWindow().save(str(out/'customizer.png'))
view.resize(430,310);QTest.qWait(50)
check('dialog.width===width && dialog.height===height','compact editor stays inside its owning window')
assert view.grabWindow().save(str(out/'customizer-compact.png'))
act('dialog.reject();nativeChecked=true')
check('stateRow.itemFor(94).checked && stateRow.itemFor(94).Accessible.checked',
      'authoritative checked state reaches both the actual button and accessibility properties')
act('nativeChecked=false')
check('!stateRow.itemFor(94).checked && !stateRow.itemFor(94).Accessible.checked && stateRow.itemFor(94).checkable',
      'unchecked state remains a natively checkable control')
toggle=owned_item('stateRow.itemFor(94)')
point=toggle.mapToScene(QPointF(toggle.width()/2,toggle.height()/2)).toPoint()
QTest.mouseClick(view,Qt.LeftButton,Qt.NoModifier,point);QTest.qWait(20)
check('!nativeChecked && !stateRow.itemFor(94).checked && actions===1',
      'actual unchecked click requests one action without guessing a successful state change')
act('nativeChecked=true')
QTest.mouseClick(view,Qt.LeftButton,Qt.NoModifier,point);QTest.qWait(20)
check('nativeChecked && stateRow.itemFor(94).checked && actions===2',
      'actual checked click preserves confirmed state until the owner accepts removal')
interface=QAccessible.queryAccessibleInterface(toggle)
assert interface and interface.state().checkable and interface.state().checked
checks.append('actual Qt accessible interface exposes a checked toggle button')
accessible_actions=interface.actionInterface()
assert accessible_actions and QAccessibleActionInterface.pressAction() in accessible_actions.actionNames()
accessible_actions.doAction(QAccessibleActionInterface.pressAction());QTest.qWait(20)
check('actions===3 && nativeChecked && stateRow.itemFor(94).checked',
      'native accessibility Press requests one action without optimistic toggling')
toggle.forceActiveFocus(Qt.TabFocusReason);QTest.keyClick(view,Qt.Key_Return);QTest.qWait(20)
check('actions===4 && nativeChecked && stateRow.itemFor(94).checked',
      'Return requests exactly one action and retains owner-confirmed state')
toggle.forceActiveFocus(Qt.MouseFocusReason);QTest.qWait(20);QTest.keyClick(view,Qt.Key_Space);QTest.qWait(20)
check('actions===5 && nativeChecked && stateRow.itemFor(94).checked && stateRow.itemFor(94).navigationKeyScope',
      'Space after pointer-style focus requests one action through the native key-ownership seam')
check('stateRow.itemFor(95).children[0].text==="Owned <mix> & Friends" && stateRow.itemFor(95).children[0].textFormat===Text.PlainText',
      'playlist metadata renders literal text rather than interpreting title markup')
view.close();QTest.qWait(30)
(out/'verification.json').write_text(json.dumps(dict(passed=not messages,checks=checks,diagnostics=messages,
 sourceSha256={p.relative_to(repo).as_posix():sha256(p.read_bytes()).hexdigest() for p in [Path(__file__),qt/'player/qml/JuxtopposedPlaybackBar.qml',qt/'player/qml/JuxtopposedVolumeControl.qml',qt/'player/qml/JuxtopposedLayoutRow.qml',qt/'dialogs/dialogs/qml/JuxtopposedPlayerCustomizer.qml']},
 scope='Actual production customizer and shared bar preview; owned controller contract, not native C++/media or design-fidelity acceptance'),indent=2)+'\n')
assert not messages,messages
print('Passed',len(checks),'customizer component checks with no diagnostics.')
