// Private owned-window test TU. Never linked into the ordinary GUI.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "qt.hpp"
#include "maininterface/mainctx.hpp"
#include "maininterface/network_preferences_model.hpp"
#include "dialogs/dialogs_provider.hpp"
#include <vlc_configuration.h>
#include <vlc_interface.h>
#include <QApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSet>
#include <QTest>
#include <QTimer>
#include <QWidget>
#include <windows.h>
#include <stdexcept>

namespace {
QJsonArray checks;
MainCtx *context = nullptr;
QQuickWindow *ownedWindow = nullptr;
QJsonObject report;
QList<QObject*> objects()
{
    QList<QObject*> pending, result;
    for (auto *window : QGuiApplication::allWindows()) {
        auto *quick=qobject_cast<QQuickWindow*>(window);
        if (quick) pending.append(quick->contentItem());
    }
    QSet<QObject*> visited;
    while (!pending.isEmpty()) {
        auto *object=pending.takeLast();
        if (!object || visited.contains(object)) continue;
        visited.insert(object); result.append(object); pending.append(object->children());
        if (auto *item=qobject_cast<QQuickItem*>(object))
            for (auto *child:item->childItems()) pending.append(child);
    }
    return result;
}
QObject *find(const char *name)
{
    for (auto *object:objects()) if (object->objectName()==QLatin1String(name)) return object;
    return nullptr;
}
void check(bool ok,const char *name)
{
    checks.append(QJsonObject{{"name",QString::fromUtf8(name)},{"passed",ok}});
    if (!ok) throw std::runtime_error(name);
}
void settle(int ms=120) { QTest::qWait(ms); }
void click(const char *name)
{
    auto *item=qobject_cast<QQuickItem*>(find(name));
    check(item && item->isVisible() && item->isEnabled(),name);
    ownedWindow=item->window();
    check(ownedWindow!=nullptr,"target belongs to an owned QQuickWindow");
    ownedWindow->requestActivate();settle(60);
    auto point=item->mapToScene(QPointF(item->width()/2,item->height()/2)).toPoint();
    QTest::mouseClick(ownedWindow,Qt::LeftButton,Qt::NoModifier,point);settle();
}
void type(const char *name,const QString &value)
{
    click(name);QTest::keyClick(ownedWindow,Qt::Key_A,Qt::ControlModifier);
    QKeyEvent press(QEvent::KeyPress,0,Qt::NoModifier,value);
    QCoreApplication::sendEvent(ownedWindow,&press);
    QTest::keyClick(ownedWindow,Qt::Key_Return);settle();
}
void choice(const char *name,int index)
{
    click(name);QTest::keyClick(ownedWindow,Qt::Key_Home);
    for (int i=0;i<index;i++) QTest::keyClick(ownedWindow,Qt::Key_Down);
    QTest::keyClick(ownedWindow,Qt::Key_Return);settle();
}
void finish(bool passed,const QString &error={})
{
    report.insert("passed",passed);report.insert("error",error);report.insert("checks",checks);
    report.insert("ownedWindowOnly",true);report.insert("ordinaryGuiContainsQa",false);
    report.insert("windowsDpiMatrixAccepted",false);report.insert("fullBetaAccepted",false);
    QFile output(qEnvironmentVariable("VLC_OWNED_UI_REPORT"));
    if (output.open(QIODevice::WriteOnly)) output.write(QJsonDocument(report).toJson());
    if (context) libvlc_Quit(vlc_object_instance(context->getIntf()));
    else QCoreApplication::quit();
}
void run()
{
    try {
        check(context && ownedWindow,"real MainCtx and native VLC window ready");
        ownedWindow->resize(1500,1000);ownedWindow->requestActivate();settle();
        auto *model=context->getNetworkPreferences();
        check(model!=nullptr,"real MainCtx owns the Network controller");
        DialogsProvider::getInstance()->showGlobalSettingsDialog();settle(250);
        auto *settings=find("juxtopposedGlobalSettings");
        check(settings && settings->property("visible").toBool(),"native shared settings popup opens");
        click("globalCategorynetwork");
        auto *page=find("juxtopposedNetworkSettings");
        check(page && qvariant_cast<QObject*>(page->property("service"))==model,"QML binds the actual native Network controller");
        check(find("networkLocalFiles")!=nullptr,"actual Network page loaded through compiled QML");
        report.insert("devicePixelRatio",ownedWindow->devicePixelRatio());
        const QString folder=qEnvironmentVariable("VLC_OWNED_UI_OUTPUT");
        check(ownedWindow->grabWindow().save(folder+"/network-native-dark.png"),"native Network window capture saved");
        if (qEnvironmentVariable("VLC_OWNED_UI_MODE")==QStringLiteral("reopen")) {
            check(model->values().value("localFiles").toLongLong()==819,"saved buffering survives a real application restart");
            check(model->values().value("proxyPasswordConfigured").toBool(),"saved password is present without exposing plaintext");
            check(find("networkProxyPassword")->property("text").toString().isEmpty(),"reopened native password field is empty");
        } else {
            choice("networkBuffering",3);
            check(config_GetInt("file-caching")==300 && config_GetInt("network-caching")==1000,"owned pointer and keys save the native Normal preset");
            choice("networkBuffering",0);
            type("networkLocalFiles",QStringLiteral("777"));
            check(config_GetInt("file-caching")==777,"owned key editing saves native Custom buffering");
            const QString path=qEnvironmentVariable("VLC_OWNED_UI_CONFIG");
            HANDLE locked=CreateFileW(reinterpret_cast<LPCWSTR>(path.utf16()),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
            check(locked!=INVALID_HANDLE_VALUE,"owned native preferences file lock acquired");
            type("networkLocalFiles",QStringLiteral("819"));
            const bool rejected=!model->lastError().isEmpty() && config_GetInt("file-caching")==777;
            CloseHandle(locked);
            check(rejected,"actual UI reports locked persistence and keeps native values unchanged");
            QTest::keyClick(ownedWindow,Qt::Key_Return);settle();
            check(model->lastError().isEmpty() && config_GetInt("file-caching")==819,"actual native field permits retry after unlocking");
            type("networkHttpProxy",QStringLiteral("http://owned-ui@127.0.0.1:18080"));
            type("networkProxyPassword",QStringLiteral("owned-ui-secret"));
            check(model->values().value("proxyPasswordConfigured").toBool(),"owned native password entry saved");
            check(find("networkProxyPassword")->property("text").toString().isEmpty(),"native password field clears after checked save");
            click("networkAdvanced");settle(250);
            check(!settings->property("visible").toBool(),"Advanced releases the actual QML modal");
            QWidget *expert=nullptr;
            for (auto *widget:QApplication::topLevelWidgets())
                if (widget->isVisible() && widget->inherits("PrefsDialog")) expert=widget;
            check(expert!=nullptr,"Advanced opens the actual native expert preferences dialog");
            QTest::keyClick(expert,Qt::Key_Escape);settle();
        }
        finish(true);
    } catch(const std::exception &error) { finish(false,QString::fromUtf8(error.what())); }
}
void waitForWindow(int attempts)
{
    for (auto *object:objects()) {
        auto *engine=qmlEngine(object);
        if (!engine) continue;
        auto *candidate=engine->singletonInstance<MainCtx*>("VLC.MainInterface","MainCtx");
        if (candidate) {
            context=candidate;
            if (auto *item=qobject_cast<QQuickItem*>(object); item && item->window()) ownedWindow=item->window();
            if (ownedWindow) { QTimer::singleShot(250,qApp,run);return; }
        }
    }
    if (attempts>0) QTimer::singleShot(250,qApp,[attempts]{waitForWindow(attempts-1);});
    else finish(false,QStringLiteral("Native Qt window did not become ready"));
}
void startOwnedJourney()
{
    if (qEnvironmentVariableIsEmpty("VLC_OWNED_UI_REPORT")) return;
    QTimer::singleShot(1000,qApp,[]{waitForWindow(40);});
}
}
Q_COREAPP_STARTUP_FUNCTION(startOwnedJourney)
