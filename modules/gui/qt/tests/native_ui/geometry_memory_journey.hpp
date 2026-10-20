// SPDX-License-Identifier: GPL-2.0-or-later
// Exercise the real serializer on the actual native main window. Private only.
#include "widgets/native/qvlcframe.hpp"
void exerciseGeometrySnapshot(const QString &mode)
{
    auto *target=window->screen();
    window->showNormal();
    wait([&]{return window->windowStates()==Qt::WindowNoState;},"geometry snapshot starts in normal state");
    window->setGeometry(QRect(target->availableGeometry().topLeft()+QPoint(40,40),QSize(1280,900)));
    QTest::qWait(150);
    auto normal=window->geometry();auto *normalScreen=target;
    check(target && target->name()==qEnvironmentVariable("VLC_OWNED_UI_SCREEN")
          && target->availableGeometry().contains(window->frameGeometry()),
          "normal geometry fixture and its frame fit the approved monitor");
    const bool cross=mode.contains(QStringLiteral("-cross-"));
    const bool fullscreenMode=mode.endsWith(QStringLiteral("-fullscreen"));
    const bool minimized=mode.endsWith(QStringLiteral("-minimized"));
    if(cross){
        QScreen *other=nullptr;for(auto *screen:QGuiApplication::screens())if(screen!=target){other=screen;break;}
        check(other,"cross-screen geometry case has a genuine second monitor");
        window->hide();QTest::qWait(100);
        check(!window->isVisible() && !IsWindowVisible(reinterpret_cast<HWND>(window->winId())),
              "main window is hidden in Qt and Windows before moving away from monitor2");
        window->setScreen(other);window->setFramePosition(other->availableGeometry().topLeft()+QPoint(45,45));
        QTest::qWait(150);normal=window->geometry();normalScreen=other;
        check(window->screen()==other && !window->isVisible(),"original hidden normal window is on the real other monitor");
        WindowStateHolder::holdFullscreen(window,WindowStateHolder::INTERFACE,true,QGuiApplication::screens().indexOf(target));
        QTest::qWait(150);
        check(window->screen()==target && !window->isVisible() && !IsWindowVisible(reinterpret_cast<HWND>(window->winId())),
              "production fullscreen policy selects monitor2 while the window stays hidden");
    }else if(fullscreenMode) context->setInterfaceFullScreen(true);
    else if(minimized) window->showMinimized();
    else if(mode.contains(QStringLiteral("-system-")))
        SendMessageW(reinterpret_cast<HWND>(window->winId()),WM_SYSCOMMAND,SC_MAXIMIZE,0);
    else if(mode.endsWith(QStringLiteral("-maximized"))) window->showMaximized();
    QTest::qWait(350);
    const auto state=window->windowStates();
    check(mode.endsWith(QStringLiteral("-normal")) ? state==Qt::WindowNoState
          : minimized ? state.testFlag(Qt::WindowMinimized)
          : fullscreenMode ? state.testFlag(Qt::WindowFullScreen)
          : state.testFlag(Qt::WindowMaximized),"actual Windows main window reaches the requested save state");
    QSettings settings(QDir(qEnvironmentVariable("VLC_OWNED_UI_OUTPUT")).filePath("geometry-probe.ini"),QSettings::IniFormat);
    QVLCTools::saveWindowPosition(&settings,window);settings.sync();
    const auto raw=settings.value("geometry").toByteArray();QDataStream stream(raw);stream.setVersion(QDataStream::Qt_4_0);
    quint32 magic;quint8 major,minor,maximized,fullscreen;QString name;qreal dpr;QRect screenRect,client;QPoint position;
    stream>>magic>>major>>minor>>name>>dpr>>screenRect>>position>>client>>maximized>>fullscreen;
    const auto rect=[](const QRect &r){return QJsonArray{r.x(),r.y(),r.width(),r.height()};};
    report.insert("geometrySerialization",QJsonObject{{"normalClient",rect(normal)},{"currentClient",rect(window->geometry())},
        {"savedClient",rect(client)},{"savedPosition",QJsonArray{position.x(),position.y()}},
        {"savedScreen",name},{"savedMaximized",bool(maximized)},{"savedFullscreen",bool(fullscreen)}});
    if(fullscreenMode)WindowStateHolder::holdFullscreen(window,WindowStateHolder::INTERFACE,false);
    else window->showNormal();
    QTest::qWait(200);
    check(stream.status()==QDataStream::Ok && magic==0x1D94200 && major==1,
          "real geometry serializer keeps its compatible versioned format");
    check(client==normal && position==normal.topLeft(),
          "saved normal client bounds survive maximized or fullscreen shutdown");
    check(name==normalScreen->name() && dpr==normalScreen->devicePixelRatio() && screenRect==normalScreen->geometry(),
          "serialized normal rectangle retains the correct real screen identity");
    QWindow restored;restored.setScreen(target);restored.setFlag(Qt::FramelessWindowHint);restored.create();
    QVLCTools::restoreWindowPosition(&settings,&restored,QSize(600,420));
    check(restored.windowStates()==(state & (Qt::WindowMaximized | Qt::WindowFullScreen)),
          "fresh native window restores the requested maximized or fullscreen state");
    restored.setWindowStates(Qt::WindowNoState);
    QTest::qWait(150);
    check(restored.geometry()==normal,"cold normal return retains the exact pre-maximize or pre-fullscreen rectangle");
    check(!restored.isVisible() && !IsWindowVisible(reinterpret_cast<HWND>(restored.winId())),
          "fresh restore probe never becomes visible on another monitor");
    if(cross){window->setScreen(target);window->setFramePosition(target->availableGeometry().topLeft()+QPoint(40,40));window->showNormal();
        wait([&]{return window->screen()==target && window->isVisible();},"normal presentation resumes only on monitor2");}
    report.insert("nativeGeometrySnapshotAccepted",true);
}
