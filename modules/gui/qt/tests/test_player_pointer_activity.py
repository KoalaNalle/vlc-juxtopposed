#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Exact production audio HoverHandler; offscreen Qt input/timer regression.

The supplied MainCtx/FSM boundaries count activity and arm a real Qt timer.
This does not establish native VLC playback or Windows input acceptance.
"""
from pathlib import Path
from hashlib import sha256
import argparse,json,os,sys

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-dir',type=Path,required=True)
    parser.add_argument('--repo-root',type=Path,default=Path(__file__).resolve().parents[4])
    args=parser.parse_args();out=args.output_dir.resolve();out.mkdir(parents=True,exist_ok=False)
    source=args.repo_root/'modules/gui/qt/player/qml/Player.qml'
    text=source.read_text();start=text.index('    HoverHandler {',text.index('// Passive observation adds audio-mode reveal'))
    end=text.index('    PointHandler {',start);block=text[start:end]
    (out/'production-hover.qml.txt').write_text(block)
    qml='''import QtQuick
Item {
    width: 400; height: 300
    QtObject {
        id: playerToolbarVisibilityFSM
        property int moves: 0
        signal mouseMove()
        onMouseMove: { moves++; idle.restart() }
    }
    property alias moveCount: playerToolbarVisibilityFSM.moves
    property bool expired: false
    Timer { id: idle; interval: 200; onTriggered: parent.expired = true }
'''+block.replace('HoverHandler {','HoverHandler {\n        objectName: "ownedPointerObserver"',1)+'}\n'
    fixture=out/'fixture.qml';fixture.write_text(qml)
    os.environ.update(QT_QPA_PLATFORM='offscreen',QT_QUICK_BACKEND='software',QML_DISABLE_DISK_CACHE='1')
    from PySide6.QtCore import QObject,Property,QUrl,QPoint,Qt,QMetaObject,qInstallMessageHandler
    from PySide6.QtGui import QGuiApplication
    from PySide6.QtQuick import QQuickView
    from PySide6.QtTest import QTest
    app=QGuiApplication([]);messages=[]
    def handler(kind,context,message):messages.append(message)
    qInstallMessageHandler(handler)
    class Context(QObject):
        @Property(bool,constant=True)
        def hasEmbededVideo(self):return False
        @Property(int,constant=True)
        def controlAutoHideMode(self):return 2
    context=Context();window=QQuickView();window.rootContext().setContextProperty('MainCtx',context)
    record=dict(passed=False,nativeApplicationAccepted=False,scope=__doc__,sourceSha256=sha256(source.read_bytes()).hexdigest(),
                extractedBlockSha256=sha256(block.encode()).hexdigest(),checks=[],qtMessages=messages)
    def check(condition,name):
        record['checks'].append(dict(name=name,passed=bool(condition)))
        assert condition,name
    try:
        window.setSource(QUrl.fromLocalFile(str(fixture)));window.resize(400,300);window.show();QTest.qWait(60)
        check(window.status()==QQuickView.Ready,'Exact production hover block loads with the supplied boundary')
        root=window.rootObject();observer=root.findChild(QObject,'ownedPointerObserver')
        check(observer is not None,'Actual HoverHandler materializes')
        QTest.mouseMove(window,QPoint(150,140));QTest.qWait(40)
        check(observer.property('hovered') and root.property('moveCount')>0,'Actual Qt pointer movement reveals once')
        before=root.property('moveCount')
        check(QMetaObject.invokeMethod(observer,'pointChanged'),'Point notification is a real Qt signal')
        QTest.qWait(20)
        check(root.property('moveCount')==before,'Unchanged point notification cannot restart the timer')
        root.setProperty('expired',False)
        QTest.mouseMove(window,QPoint(175,145));QTest.qWait(30);before=root.property('moveCount')
        for _ in range(15):QMetaObject.invokeMethod(observer,'pointChanged');QTest.qWait(20)
        check(root.property('moveCount')==before and root.property('expired'),
              'Stationary notification churn cannot prevent real idle expiry')
        QTest.mouseMove(window,QPoint(185,150));QTest.qWait(20)
        check(root.property('moveCount')>before,'Changed scene position still reveals controls')
        before=root.property('moveCount');QTest.mouseMove(window,QPoint(-10,-10));QTest.qWait(20)
        QTest.mouseMove(window,QPoint(185,150));QTest.qWait(20)
        check(root.property('moveCount')>before,'Leaving and reentering at the same position reveals again')
        check(not messages,'No Qt diagnostics')
        record['passed']=True
    except Exception as error:record['error']=str(error)
    finally:
        window.close();window.setSource(QUrl());QTest.qWait(20)
        (out/'verification.json').write_text(json.dumps(record,indent=2)+'\n')
    print(json.dumps({k:record[k] for k in ['passed','checks','qtMessages']},indent=2))
    return 0 if record['passed'] else 1

if __name__=='__main__':raise SystemExit(main())
