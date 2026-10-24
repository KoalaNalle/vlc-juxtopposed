# SPDX-License-Identifier: GPL-2.0-or-later
"""Exercise the exact bookmark/chapter pointer subtrees in real Qt Controls.

Selection and activation recipients are supplied. This checks event delivery
through the production Control/child-item hierarchy, not native media seeking.
"""
import argparse,hashlib,json,os,re,shutil
from pathlib import Path
os.environ['QT_QPA_PLATFORM']='offscreen'
os.environ['QT_QUICK_BACKEND']='software'
os.environ['QML_DISABLE_DISK_CACHE']='1'
from PySide6.QtCore import QPoint,Qt,QUrl,qInstallMessageHandler
from PySide6.QtGui import QGuiApplication
from PySide6.QtQuick import QQuickView
from PySide6.QtTest import QTest

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--repo-root',type=Path,default=Path(__file__).resolve().parents[4])
parser.add_argument('--output-dir',type=Path,required=True)
parser.add_argument('--expect-original-failures',action='store_true')
args=parser.parse_args();assert not args.output_dir.exists();args.output_dir.mkdir(parents=True)
qt=args.repo_root/'modules/gui/qt';messages=[]
qInstallMessageHandler(lambda kind,context,message:messages.append(message))
app=QGuiApplication([])
util=args.output_dir/'imports/VLC/Util';util.mkdir(parents=True)
touch=qt/'util/qml/DelegateTouchTapHandler.qml';shutil.copyfile(touch,util/touch.name)
(util/'qmldir').write_text('module VLC.Util\nDelegateTouchTapHandler 1.0 DelegateTouchTapHandler.qml\n',encoding='utf-8')

def block(text,start):
    opening=text.index('{',start);depth=1;end=opening+1
    while depth:
        depth+=(text[end]=='{')-(text[end]=='}');end+=1
    return text[start:end]

cases=[];hashes={touch.relative_to(qt).as_posix():hashlib.sha256(touch.read_bytes()).hexdigest()}
for kind,filename,row_id,area_id in [
    ('chapter','PlaylistPane.qml','chapterRow','chapterPointerArea'),
    ('bookmark','JuxtopposedBookmarkList.qml','row','bookmarkPointerArea')]:
    source=qt/'playlist/qml'/filename;text=source.read_text(encoding='utf-8')
    hashes[source.relative_to(qt).as_posix()]=hashlib.sha256(source.read_bytes()).hexdigest()
    delegate_match=re.search(r'delegate:\s*T\.Control\s*\{\s*id:\s*'+row_id+r'\b',text)
    assert delegate_match;delegate=block(text,delegate_match.start())
    area=re.search(r'Item\s*\{\s*id:\s*'+area_id+r'\b',delegate)
    if area:subtree=block(delegate,area.start())
    else:
        mouse=re.search(r'\bTapHandler\s*\{',delegate)
        finger=re.search(r'\bDelegateTouchTapHandler\s*\{',delegate)
        assert mouse and finger
        subtree=block(delegate,mouse.start())+'\n'+block(delegate,finger.start())
    for action in ['single-left','double-left','single-right']:
        scene=args.output_dir/(kind+'-'+action+'.qml')
        scene.write_text('''import QtQuick
import QtQuick.Templates as T
import VLC.Util
Item {
    id: root
    width: 360; height: 32
    property int currentIndex: -1
    property var calls: []
    property QtObject selectionModel: QtObject {
        property var selectedIndexesFlat: []
        function updateSelection(modifiers, oldIndex, newIndex) { selectedIndexesFlat = [newIndex] }
    }
    function activate(index) { calls = calls.concat([index]) }
    QtObject {
        id: chaptersView
        property int currentIndex: -1
        property QtObject selectionModel: root.selectionModel
        function activate(index) { root.activate(index) }
    }
    T.Control {
        id: '''+row_id+'''
        anchors.fill: parent
        property int index: 1
        '''+subtree+'''
    }
    readonly property int selectedIndex: '''+('chaptersView' if kind=='chapter' else 'root')+'''.currentIndex
    readonly property var selectedRows: selectionModel.selectedIndexesFlat
}''',encoding='utf-8')
        view=QQuickView();view.engine().addImportPath(str((args.output_dir/'imports').resolve()))
        messages.clear();view.setSource(QUrl.fromLocalFile(str(scene.resolve())))
        assert not view.errors(),[e.toString() for e in view.errors()]
        view.show();QTest.qWait(80);root=view.rootObject()
        if action=='double-left':QTest.mouseDClick(view,Qt.LeftButton,Qt.NoModifier,QPoint(100,16),50)
        else:QTest.mouseClick(view,Qt.RightButton if action=='single-right' else Qt.LeftButton,Qt.NoModifier,QPoint(100,16),50)
        QTest.qWait(app.styleHints().mouseDoubleClickInterval()+100)
        actual=dict(index=root.property('selectedIndex'),selected=root.property('selectedRows').toVariant(),calls=root.property('calls').toVariant())
        # Qt's full double-click gesture also delivers the selection tap.
        expected=dict(index=1,selected=[1],calls=[1] if action=='double-left' else [])
        cases.append(dict(name=kind+'-'+action,actual=actual,expected=expected,diagnostics=list(messages),passed=actual==expected and not messages))
        view.close();view.deleteLater();app.processEvents()
negative=args.expect_original_failures and len(cases)==6 and all(not c['passed'] and not c['diagnostics'] for c in cases)
report=dict(passed=all(c['passed'] for c in cases),expectedNegativeVerified=negative,cases=cases,sourceSha256=hashes,
    scope='Exact production mouse/touch handler subtrees and Control-versus-child-item ownership in real Qt; supplied row index and selection/activation recipients. No native model, desktop, physical touch or seeking acceptance.')
(args.output_dir/'verification.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps(dict(passed=report['passed'],expectedNegativeVerified=negative,cases=len(cases),failures=sum(not c['passed'] for c in cases))))
raise SystemExit(0 if report['passed'] or negative else 1)
