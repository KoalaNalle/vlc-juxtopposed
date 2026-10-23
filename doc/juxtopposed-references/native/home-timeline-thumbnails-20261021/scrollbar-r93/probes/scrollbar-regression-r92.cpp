// Private native regression. Creates only an isolated, hidden test scrollbar.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "qt.hpp"
#include <QAccessible>
#include <QApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QScopedPointer>
#include <QSet>
#include <QTimer>
#include <private/qquickaccessibleattached_p.h>
#include <cmath>

namespace {
QQmlEngine *findEngine() {
    QList<QObject *> pending;
    QSet<QObject *> seen;
    for (auto *window : QGuiApplication::allWindows())
        if (auto *quick = qobject_cast<QQuickWindow *>(window)) pending.append(quick->contentItem());
    while (!pending.isEmpty()) {
        auto *object = pending.takeLast();
        if (!object || seen.contains(object)) continue;
        seen.insert(object);
        if (auto *engine = qmlEngine(object)) return engine;
        pending.append(object->children());
        if (auto *item = qobject_cast<QQuickItem *>(object))
            for (auto *child : item->childItems()) pending.append(child);
    }
    return nullptr;
}
void run() {
    QJsonArray checks;
    QJsonObject report{{"privateFixture", true}, {"allPassed", false}};
    auto save = [&] {
        report.insert("checks", checks);
        QFile output(qEnvironmentVariable("VLC_OWNED_SCROLLBAR_REGRESSION"));
        if (output.open(QIODevice::WriteOnly)) output.write(QJsonDocument(report).toJson());
    };
    auto *engine = findEngine();
    if (!engine) { report.insert("setupError", "No existing Home engine"); save(); return; }
    QQmlComponent component(engine);
    component.setData("import QtQuick\nimport QtQuick.Templates as T\nT.ScrollBar { size: 0.25; stepSize: 0.01; position: 0.4; visible: false }", QUrl("qrc:/owned-scrollbar-regression.qml"));
    QScopedPointer<QObject> bar(component.create());
    if (!bar) { report.insert("setupError", component.errorString()); save(); return; }
    auto *attached = qobject_cast<QQuickAccessibleAttached *>(qmlAttachedPropertiesObject<QQuickAccessibleAttached>(bar.data(), true));
    if (!attached) { report.insert("setupError", "No attached accessible object"); save(); return; }
    auto action = [&](const char *name, const char *signal, double expected) {
        bar->setProperty("position", 0.4);
        const bool invoked = QMetaObject::invokeMethod(attached, signal, Qt::DirectConnection);
        const double position = bar->property("position").toDouble();
        checks.append(QJsonObject{{"name", name}, {"signalInvoked", invoked},
            {"expectedPosition", expected}, {"actualPosition", position},
            {"passed", invoked && std::abs(position - expected) < 0.000001}});
    };
    QAccessible::setActive(false);
    QAccessible::setActive(true);
    action("one activation increases once", "increaseAction", 0.41);
    action("one activation decreases once", "decreaseAction", 0.39);
    for (int i = 0; i < 1000; ++i) QAccessible::setActive(true);
    action("1000 repeated activations still increase once", "increaseAction", 0.41);
    action("1000 repeated activations still decrease once", "decreaseAction", 0.39);
    QAccessible::setActive(false);
    action("deactivation disconnects the action", "increaseAction", 0.4);
    QAccessible::setActive(true);
    action("reactivation reconnects exactly once", "increaseAction", 0.41);
    for (int i = 0; i < 50; ++i) QAccessible::setActive(true);
    QAccessible::setActive(false);
    QAccessible::setActive(false);
    QAccessible::setActive(true);
    action("repeat and deactivate cycle reconnects exactly once", "decreaseAction", 0.39);
    bool allPassed = true;
    for (const auto &check : checks) allPassed &= check.toObject().value("passed").toBool();
    report.insert("allPassed", allPassed);
    report.insert("scope", "Real Qt Quick scrollbar accessibility signals after repeated activation, deactivation and reactivation. Hidden generated test object; no user media/settings/input.");
    save();
}
void start() {
    if (!qEnvironmentVariableIsEmpty("VLC_OWNED_SCROLLBAR_REGRESSION"))
        QTimer::singleShot(10000, qApp, run);
}
}
Q_COREAPP_STARTUP_FUNCTION(start)
