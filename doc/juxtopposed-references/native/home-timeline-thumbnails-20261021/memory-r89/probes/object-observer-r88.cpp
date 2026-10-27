// Private diagnostic only: three bounded QObject-tree snapshots, no input.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "qt.hpp"
#include "maininterface/mainctx.hpp"
#include <QAccessible>
#include <QApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSet>
#include <QTimer>
#include <QWidget>

namespace {
class Observer final : public QObject {
    QElapsedTimer clock;
    qint64 swapped = 0;
    QSet<QQuickWindow *> observedWindows;

    void snapshot(const char *label) {
        QList<QObject *> pending{qApp};
        QSet<QObject *> seen;
        QSet<QQmlEngine *> engines;
        QJsonObject classes;
        for (auto *w : QGuiApplication::allWindows()) {
            pending.append(w);
            if (auto *quick = qobject_cast<QQuickWindow *>(w)) {
                pending.append(quick->contentItem());
                if (!observedWindows.contains(quick)) {
                    observedWindows.insert(quick);
                    connect(quick, &QQuickWindow::frameSwapped, this,
                            [this] { ++swapped; }, Qt::QueuedConnection);
                }
            }
        }
        for (auto *widget : QApplication::topLevelWidgets()) pending.append(widget);
        while (!pending.isEmpty()) {
            auto *object = pending.takeLast();
            if (!object || seen.contains(object)) continue;
            seen.insert(object);
            const auto name = QString::fromLatin1(object->metaObject()->className());
            classes[name] = classes.value(name).toInt() + 1;
            pending.append(object->children());
            if (auto *item = qobject_cast<QQuickItem *>(object))
                for (auto *child : item->childItems()) pending.append(child);
            if (auto *engine = qmlEngine(object); engine && !engines.contains(engine)) {
                engines.insert(engine);
                pending.append(engine);
                // Home already consumes this singleton. It includes its model tree.
                pending.append(engine->singletonInstance<MainCtx *>("VLC.MainInterface", "MainCtx"));
            }
        }
        const QJsonObject row{{"label", label}, {"ms", clock.elapsed()},
            {"reachableQObjectCount", seen.size()}, {"engineCount", engines.size()},
            {"framesSwappedSinceFirstSnapshot", swapped},
            {"accessibilityActive", QAccessible::isActive()}, {"classes", classes}};
        QFile output(qEnvironmentVariable("VLC_OWNED_OBJECT_TRACE"));
        if (output.open(QIODevice::WriteOnly | QIODevice::Append))
            output.write(QJsonDocument(row).toJson(QJsonDocument::Compact) + '\n');
    }
public:
    Observer() : QObject(qApp) {
        clock.start();
        QTimer::singleShot(10000, this, [this] { snapshot("10s"); });
        QTimer::singleShot(100000, this, [this] { snapshot("100s"); });
        QTimer::singleShot(190000, this, [this] { snapshot("190s"); });
        connect(qApp, &QCoreApplication::aboutToQuit, this,
                [this] { snapshot("about-to-quit"); });
    }
};
void start() {
    if (!qEnvironmentVariableIsEmpty("VLC_OWNED_OBJECT_TRACE")) new Observer;
}
}
Q_COREAPP_STARTUP_FUNCTION(start)
