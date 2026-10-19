# SPDX-License-Identifier: GPL-2.0-or-later
"""Owned profile form component journeys; not native profile/settings acceptance."""
import argparse, json, os, shutil, tempfile
from pathlib import Path
os.environ['QT_QPA_PLATFORM']='offscreen'
os.environ['QT_QUICK_BACKEND']='software'
os.environ['QML_DISABLE_DISK_CACHE']='1'
from PySide6.QtCore import QUrl,Qt,QPoint,QPointF,qInstallMessageHandler
from PySide6.QtGui import QGuiApplication,QFontDatabase
from PySide6.QtQml import QQmlExpression,qmlContext,qmlRegisterSingletonType
from PySide6.QtQuick import QQuickView
from PySide6.QtTest import QTest
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--repo-root',type=Path,required=True);parser.add_argument('--output-dir',type=Path,required=True)
args=parser.parse_args();repo=args.repo_root.resolve();qt=repo/'modules/gui/qt';out=args.output_dir.resolve();out.mkdir(parents=True,exist_ok=False)
os.environ['QT_QPA_FONTDIR']=str(qt/'pixmaps/juxtopposed')
messages=[];qInstallMessageHandler(lambda kind,context,message:messages.append(message));app=QGuiApplication([])
assert QFontDatabase.addApplicationFont(str(qt/'pixmaps/juxtopposed/InterVariable.ttf'))>=0
qmlRegisterSingletonType(QUrl.fromLocalFile(str(qt/'style/JuxtopposedPalette.qml')),'VLC.Style',1,0,'JuxtopposedPalette')
temporary=tempfile.TemporaryDirectory(prefix='vlc-profile-form-');folder=Path(temporary.name)
scene='''import QtQuick
import QtQuick.Controls.Basic
import VLC.Style
import "@DIALOGS@" as Authored
Item {
 id: root; width: 900; height: 650
 property alias dialog: form
 property alias service: service
 property alias opener: opener
 property alias testScale: form.uiScale
 QtObject { id: appearance; property var values: ({theme: 2, accentIndex: 0}) }
 Component.onCompleted: JuxtopposedPalette.preferences = appearance
 QtObject {
  id: service
  property bool available: true
  property bool busy: false
  property bool needsRecovery: false
  property bool unsaved: false
  property string selectedName: "Movies"
  property string lastError: ""
  property bool fail: false
  property int creates: 0
  property int finishes: 0
  property int restores: 0
  property int saves: 0
  property string submitted: ""
  function accepted() { lastError = fail ? "Owned save failure. Retry is available." : ""; return !fail }
  function createProfile(name) { creates++; submitted = name; return accepted() }
  function retrySwitch() { finishes++; if (!accepted()) return false; needsRecovery = false; return true }
  function restorePreviousProfile() { restores++; if (!accepted()) return false; needsRecovery = false; return true }
  function retrySave() { saves++; return accepted() }
 }
 Rectangle { anchors.fill: parent; color: "#383838" }
 Button { id: opener; objectName: "profileFormOpener"; text: "Open" }
 Authored.JuxtopposedPlayerProfilesDialog { id: form; parent: root; controller: service; assetBase: "@ASSETS@" }
}'''.replace('@DIALOGS@',(qt/'dialogs/dialogs/qml').as_uri()).replace('@ASSETS@',(qt/'pixmaps/juxtopposed').as_uri()+'/')
(folder/'Scene.qml').write_text(scene,encoding='utf-8')
view=QQuickView();view.setResizeMode(QQuickView.SizeRootObjectToView);view.setSource(QUrl.fromLocalFile(str(folder/'Scene.qml')))
assert not view.errors(),[error.toString() for error in view.errors()]
root=view.rootObject();view.show();view.requestActivate();QTest.qWait(80);checks=[]
def evaluate(code,obj=None):
 expression=QQmlExpression(qmlContext(obj or root),obj or root,code);value=expression.evaluate()
 assert not expression.hasError(),expression.error().toString()
 return value[0] if isinstance(value,tuple) else value
def check(code,label):
 assert evaluate(code),label
 checks.append(label)
def act(code):evaluate(code);QTest.qWait(20)
def capture(name):
 assert view.grabWindow().save(str(out/name))
act('dialog.showDialog(opener, true)')
check('dialog.visible && dialog.createMode && dialog.title === "New Profile"','opens the profile-name form')
check('!dialog.canSave && dialog.contentItem !== null','empty profile name cannot save')
check('service.creates === 0 && service.finishes === 0','opening makes no service write')
check('dialog.contentItem.children.length > 0','form content is instantiated')
name=root.findChild(type(root),'playerProfileName')
assert name is not None
assert name.property('activeFocus')
checks.append('name editor receives opening focus')
name.setProperty('text','   ');QTest.qWait(10)
check('!dialog.canSave','whitespace name cannot save')
name.setProperty('text','Travel Ää 日本語');act('service.fail = true; dialog.save()')
check('dialog.visible && service.creates === 1 && service.submitted === "Travel Ää 日本語"','failed create preserves entered Unicode name')
check('service.lastError.length > 0 && !dialog.busy','failed create remains retryable')
capture('new-profile-save-failure.png')
act('service.fail = false; dialog.save()')
check('!dialog.visible && service.creates === 2','retry succeeds and closes')
check('opener.activeFocus','closing restores the opener')
act('dialog.showDialog(opener, true)')
check('!dialog.canSave','reopening starts with a fresh name')
QTest.keyClick(view,Qt.Key_Escape);QTest.qWait(20)
check('!dialog.visible && service.creates === 2','Escape cancels without saving')
act('service.needsRecovery = true; service.lastError = "An interrupted profile switch needs recovery."; dialog.showDialog(opener, false)')
check('dialog.visible && !dialog.createMode && dialog.recovering && dialog.canSave','pending switch exposes recovery controls')
capture('profile-switch-recovery.png')
act('service.fail = true; dialog.save()')
check('dialog.visible && service.finishes === 1 && service.needsRecovery','failed recovery retains the pending switch')
act('service.fail = false; dialog.save()')
check('!dialog.visible && service.finishes === 2 && !service.needsRecovery','successful finish closes recovery')
act('service.needsRecovery = true; dialog.showDialog(opener, false)')
restore=root.findChild(type(root),'playerProfileRestore');assert restore is not None
evaluate('clicked()',restore);QTest.qWait(20)
check('!dialog.visible && service.restores === 1 && !service.needsRecovery','Restore previous calls the dedicated restore operation')
act('dialog.showDialog(opener, false); service.busy = true')
check('dialog.busy && !dialog.canSave','busy service disables saving')
QTest.keyClick(view,Qt.Key_Escape);QTest.qWait(20)
check('dialog.visible','busy Escape cannot dismiss an operation')
act('service.busy = false; dialog.save()')
check('!dialog.visible && service.saves === 1','retry-save uses the catalog operation outside recovery')
act('service.available = false; dialog.showDialog(opener, true)')
check('!dialog.canSave','unavailable service cannot create')
act('dialog.reject(); service.available = true; dialog.showDialog(opener, true)')
header=root.findChild(type(root),'playerProfileHeader');assert header is not None
start=header.mapToScene(QPointF(header.width()/2,header.height()/2)).toPoint()
origin=evaluate('[dialog.x,dialog.y]');origin=origin.toVariant() if hasattr(origin,'toVariant') else origin
QTest.mousePress(view,Qt.LeftButton,Qt.NoModifier,start)
QTest.mouseMove(view,start+QPoint(30,25),20);QTest.mouseRelease(view,Qt.LeftButton,Qt.NoModifier,start+QPoint(30,25));QTest.qWait(20)
check(f'dialog.x > {origin[0]} + 20 && dialog.y > {origin[1]} + 15','actual pointer header moves the profile popup')
# Same movable base as accepted native dialogs; exercise its real bounded state.
act('dialog.moveFromHeader(100000,100000)')
check('dialog.x + dialog.width <= width + 0.1 && dialog.y + dialog.height <= height + 0.1','popup stays inside owner after movement')
view.resize(430,310);act('testScale = 1.25')
check('dialog.width <= width && dialog.height <= height && dialog.x >= 0 && dialog.y >= 0','compact scaled popup remains bounded')
scroll=root.findChild(type(root),'playerProfileScroll');assert scroll is not None
flick=scroll.property('contentItem');point=name.mapToItem(flick,QPointF(0,0))
assert point.y()>=-0.5 and point.y()+name.height()<=flick.height()+0.5,(point.y(),name.height(),flick.height())
checks.append('focused name editor scrolls fully into the compact viewport')
capture('profile-form-compact-125.png')
act('dialog.reject()');view.close();QTest.qWait(50)
diagnostics=[m for m in messages if not m.startswith('qt.qpa.')]
result={'passed':not diagnostics,'checks':len(checks),'journeys':checks,'diagnostics':diagnostics,
 'scope':'Owned QML form with supplied profile service. New/Cancel/failure/retry/recovery/restore, modal focus and compact scale; not actual native settings, OS DPI or full design fidelity.',
 'nativeAppAccepted':False,'fullBetaAccepted':False}
(out/'verification.json').write_text(json.dumps(result,indent=2)+'\n')
print('Profile form component',len(checks),'checks; diagnostics',len(diagnostics))
assert result['passed'],diagnostics
