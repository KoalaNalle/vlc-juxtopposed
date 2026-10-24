# SPDX-License-Identifier: GPL-2.0-or-later
"""Run the production visibility FSM and its signal connections in real Qt.

Only MainCtx state is supplied. Native windows/media and ordinary docking are
separate validation scopes.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
os.environ['QT_QPA_PLATFORM']='offscreen'
os.environ['QML_DISABLE_DISK_CACHE']='1'
from PySide6.QtCore import QEvent, QMetaObject, Qt, QUrl, qInstallMessageHandler
from PySide6.QtGui import QGuiApplication
from PySide6.QtQml import QQmlComponent, QQmlEngine

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--repo-root',type=Path,default=Path(__file__).resolve().parents[4])
parser.add_argument('--output-dir',type=Path,required=True)
parser.add_argument('--expect-original-failures',action='store_true')
args=parser.parse_args();assert not args.output_dir.exists();args.output_dir.mkdir(parents=True)
qt=args.repo_root/'modules/gui/qt';modules=args.output_dir/'imports'
util=modules/'VLC/Util';main=modules/'VLC/MainInterface';util.mkdir(parents=True);main.mkdir(parents=True)
sources={}
for name in ['FSM.qml','FSMState.qml','Helpers.qml']:
    p=qt/'util/qml'/name;shutil.copyfile(p,util/name);sources['util/qml/'+name]=hashlib.sha256(p.read_bytes()).hexdigest()
(util/'qmldir').write_text('module VLC.Util\nFSM 1.0 FSM.qml\nFSMState 1.0 FSMState.qml\nsingleton Helpers 1.0 Helpers.qml\n',encoding='utf-8')
(main/'qmldir').write_text('module VLC.MainInterface\nsingleton MainCtx 1.0 MainCtx.qml\n',encoding='utf-8')
(main/'MainCtx.qml').write_text('''pragma Singleton
import QtQuick
QtObject {
    property bool hasEmbededVideo: false
    enum InterfaceMode { MAININTERFACE_MODE_MAINDISPLAY }
    property int initialEffectiveMainInterfaceMode: 0
    property QtObject playqueuePanel: QtObject {
        property bool visible: false
        property bool docked: true
    }
}
''',encoding='utf-8')
production=qt/'player/qml/PlayerPlaylistVisibilityFSM.qml'
shutil.copyfile(production,args.output_dir/'PlayerPlaylistVisibilityFSM.qml')
sources['player/qml/PlayerPlaylistVisibilityFSM.qml']=hashlib.sha256(production.read_bytes()).hexdigest()
player=(qt/'player/qml/Player.qml').read_text(encoding='utf-8')
pattern=r'Connections\s*\{\s*target:\s*MainCtx\.playqueuePanel\b'
match=re.search(pattern,player);assert match
start=player.index('{',match.start());depth=1;end=start+1
while depth:
    depth+=(player[end]=='{')-(player[end]=='}');end+=1
connections=player[match.start():end]
sources['player/qml/Player.qml']=hashlib.sha256((qt/'player/qml/Player.qml').read_bytes()).hexdigest()
scene='''import QtQuick
import VLC.MainInterface
Item {
    id: root
    property bool initialDocked: true
    property bool initialVisible: true
    property bool initialVideo: false
    property int initialMode: 0
    property var machine: null
    function setup() {
        MainCtx.playqueuePanel.docked = initialDocked
        MainCtx.playqueuePanel.visible = initialVisible
        MainCtx.hasEmbededVideo = initialVideo
        MainCtx.initialEffectiveMainInterfaceMode = initialMode
        machine = factory.createObject(root)
    }
    function dock() { MainCtx.playqueuePanel.docked = true }
    function undock() { MainCtx.playqueuePanel.docked = false }
    function toggle() { machine.toggle() }
    function showQueue() { MainCtx.playqueuePanel.visible = true }
    function hideQueue() { MainCtx.playqueuePanel.visible = false }
    Component {
        id: factory
        Item {
            property alias dockedVisible: playlistVisibility.isPlaylistVisible
            readonly property bool requestedVisible: MainCtx.playqueuePanel.visible
            function toggle() { playlistVisibility.togglePlaylistVisibility() }
            PlayerPlaylistVisibilityFSM { id: playlistVisibility }
''' + connections + '''
        }
    }
}
'''
scene_path=args.output_dir/'VisibilityScene.qml';scene_path.write_text(scene,encoding='utf-8')
messages=[];qInstallMessageHandler(lambda kind,context,message:messages.append(message))
app=QGuiApplication([]);cases=[]
scenarios=[
    ('startup-library-audio',True,True,False,0,[(None,True,True)]),
    ('startup-player-audio',True,True,False,1,[(None,False,False)]),
    ('startup-video',True,True,True,0,[(None,False,True)]),
    ('startup-hidden',True,False,False,0,[(None,False,False)]),
    ('redock-player-audio',False,True,False,1,[('dock',True,True)]),
    ('redock-library-audio',False,True,False,0,[('dock',True,True)]),
    ('redock-player-video',False,True,True,1,[('dock',True,True)]),
    ('redock-library-video',False,True,True,0,[('dock',True,True)]),
    ('redock-hidden-audio',False,False,False,1,[('dock',False,False)]),
    ('redock-hidden-video',False,False,True,1,[('dock',False,False)]),
    ('floating-toggle',False,True,False,1,[('toggle',False,False),('toggle',False,True)]),
    ('docked-toggle',True,True,False,0,[('toggle',False,False),('toggle',True,True)]),
    ('external-visibility',True,True,False,0,[('hideQueue',False,False),('showQueue',True,True)]),
    ('roundtrip',True,True,False,0,[('undock',False,True),('dock',True,True),('undock',False,True),('dock',True,True)]),
]
for name,docked,visible,video,mode,steps in scenarios:
    engine=QQmlEngine();engine.addImportPath(str(modules.resolve()))
    component=QQmlComponent(engine,QUrl.fromLocalFile(str(scene_path.resolve())))
    assert not component.isError(),[e.toString() for e in component.errors()]
    root=component.create();assert root,[e.toString() for e in component.errors()]
    for key,value in [('initialDocked',docked),('initialVisible',visible),('initialVideo',video),('initialMode',mode)]:root.setProperty(key,value)
    assert QMetaObject.invokeMethod(root,'setup',Qt.DirectConnection)
    machine=root.property('machine');assert machine
    for index,(action,shown,requested) in enumerate(steps):
        if action:assert QMetaObject.invokeMethod(root,action,Qt.DirectConnection)
        app.processEvents()
        actual=[machine.property('dockedVisible'),machine.property('requestedVisible')]
        cases.append(dict(name=name,step=index,action=action,expected=[shown,requested],actual=actual,diagnostics=list(messages),passed=actual==[shown,requested] and not messages))
        messages.clear()
    root.deleteLater();engine.deleteLater();app.sendPostedEvents(None,QEvent.DeferredDelete);app.processEvents()
failed=[(c['name'],c['step']) for c in cases if not c['passed']]
expected_negative=[('redock-player-audio',0),('redock-player-video',0),('redock-library-video',0)]
result=dict(passed=not failed,expectedNegativeVerified=args.expect_original_failures and failed==expected_negative,
    cases=cases,sourceSha256=sources,connectionsSha256=hashlib.sha256(connections.encode('utf-8')).hexdigest(),
    scope='Complete production FSM/FSMState/Helpers and exact Player signal connections in real Qt, supplied MainCtx state. No native media/window or desktop acceptance.')
(args.output_dir/'verification.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(passed=result['passed'],expectedNegativeVerified=result['expectedNegativeVerified'],cases=len(cases),failed=failed),indent=2))
assert result['expectedNegativeVerified'] if args.expect_original_failures else result['passed']
