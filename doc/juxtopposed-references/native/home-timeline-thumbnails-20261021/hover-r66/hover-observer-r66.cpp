// Diagnostic-only observer. No pointer injection, backend changes or release hooks.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "qt.hpp"
#include "maininterface/mainctx.hpp"
#include "player/player_controller.hpp"
#include "player/timeline_thumbnails.hpp"
#include <QApplication>
#include <QCursor>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMouseEvent>
#include <QPointer>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSet>
#include <QTimer>
#include <windows.h>

namespace {
QList<QObject *> objects() {
    QList<QObject *> pending,result; QSet<QObject *> seen;
    for(auto *w:QGuiApplication::allWindows())
        if(auto *q=qobject_cast<QQuickWindow *>(w))pending.append(q->contentItem());
    while(!pending.isEmpty()) {
        auto *o=pending.takeLast();if(!o||seen.contains(o))continue;
        seen.insert(o);result.append(o);pending.append(o->children());
        if(auto *q=qobject_cast<QQuickItem *>(o))for(auto *c:q->childItems())pending.append(c);
    }
    return result;
}
class Observer final:public QObject {
    QPointer<MainCtx> context;
    QPointer<QQuickWindow> window;
    QPointer<QQuickItem> bar;
    QPointer<QObject> handler,card;
    QTimer timer;
    QElapsedTimer clock;
    QJsonObject previous;
    qint64 lastHeartbeat=0;
    void write(QJsonObject row) {
        row.insert("ms",clock.elapsed());
        QFile file(qEnvironmentVariable("VLC_OWNED_HOVER_TRACE"));
        if(file.open(QIODevice::WriteOnly|QIODevice::Append))file.write(QJsonDocument(row).toJson(QJsonDocument::Compact)+'\n');
    }
    void find() {
        for(auto *o:objects()) {
            if(o->objectName()==QStringLiteral("juxtopposedDialogsHost"))if(auto *q=qobject_cast<QQuickItem *>(o)) {
                auto *engine=qmlEngine(o);context=engine?engine->singletonInstance<MainCtx *>("VLC.MainInterface","MainCtx"):nullptr;
                window=q->window();
            }
            if(QByteArray(o->metaObject()->className()).startsWith("SliderBar_QMLTYPE_"))
                if(auto *q=qobject_cast<QQuickItem *>(o))if(q->isVisible())bar=q;
            if(o->objectName()==QStringLiteral("timelineThumbnail"))card=o;
        }
        if(bar)for(auto *o:bar->children())
            if(QByteArray(o->metaObject()->className()).contains("HoverHandler"))handler=o;
    }
    QJsonObject state() {
        if(!window||!bar||!context)return {{"ready",false}};
        POINT native{};const bool nativeValid=GetCursorPos(&native);
        const auto local=window->mapFromGlobal(QCursor::pos());
        const auto barPoint=bar->mapFromScene(local);
        auto *service=context->getTimelineThumbnails();
        return {{"ready",true},{"windowActive",window->isActive()},
            {"focusIsTarget",QGuiApplication::focusWindow()==window},
            {"foregroundIsTarget",GetForegroundWindow()==reinterpret_cast<HWND>(window->winId())},
            {"windowExposed",window->isExposed()},
            {"qtCursorWindowX",local.x()},{"qtCursorWindowY",local.y()},
            {"nativeCursorValid",nativeValid},
            {"qtMatchesNativeCursor",nativeValid&&QCursor::pos()==QPoint(native.x,native.y)},
            {"cursorInsideBar",bar->contains(barPoint)},
            {"barX",bar->mapToScene(QPointF()).x()},{"barY",bar->mapToScene(QPointF()).y()},
            {"barWidth",bar->width()},{"barHeight",bar->height()},
            {"barVisible",bar->isVisible()},{"barEnabled",bar->isEnabled()},
            {"hovered",handler?handler->property("hovered").toBool():false},
            {"wanted",bar->property("thumbnailWanted").toBool()},
            {"visible",card?card->property("visible").toBool():false},
            {"second",service->requestedSeconds()},{"source",service->imageSource()},
            {"loading",service->loading()},
            {"primaryPlayingState",context->getIntf()->p_mainPlayerController->property("playingState").toInt()}};
    }
    void tick() {
        if(!bar||!window||!context)find();
        const auto row=state();
        if(row!=previous||clock.elapsed()-lastHeartbeat>=5000) {
            auto saved=row;saved.insert("kind",row!=previous?"state-change":"heartbeat");write(saved);
            previous=row;lastHeartbeat=clock.elapsed();
        }
    }
    bool eventFilter(QObject *receiver,QEvent *event) override {
        if(receiver!=window)return false;
        const char *name=nullptr;
        switch(event->type()) {
            case QEvent::MouseMove:name="MouseMove";break;
            case QEvent::MouseButtonPress:name="MouseButtonPress";break;
            case QEvent::MouseButtonRelease:name="MouseButtonRelease";break;
            case QEvent::Enter:name="Enter";break;
            case QEvent::Leave:name="Leave";break;
            case QEvent::WindowActivate:name="WindowActivate";break;
            case QEvent::WindowDeactivate:name="WindowDeactivate";break;
            case QEvent::FocusIn:name="FocusIn";break;
            case QEvent::FocusOut:name="FocusOut";break;
            default:break;
        }
        if(name) {
            auto row=state();row.insert("kind","window-event");row.insert("event",name);
            row.insert("spontaneous",event->spontaneous());
            if(event->isPointerEvent()) {
                const auto *mouse=static_cast<QPointerEvent *>(event);
                if(!mouse->points().isEmpty()) {
                    row.insert("eventX",mouse->points().first().position().x());
                    row.insert("eventY",mouse->points().first().position().y());
                }
            }
            write(row);
        }
        return false;
    }
public:
    Observer():QObject(qApp) {
        clock.start();qApp->installEventFilter(this);
        QObject::connect(&timer,&QTimer::timeout,this,[this]{tick();});timer.start(200);
        QObject::connect(qApp,&QCoreApplication::aboutToQuit,this,[this]{write({{"kind","about-to-quit"}});});
    }
};
void start() {if(!qEnvironmentVariableIsEmpty("VLC_OWNED_HOVER_TRACE"))new Observer;}
}
Q_COREAPP_STARTUP_FUNCTION(start)
