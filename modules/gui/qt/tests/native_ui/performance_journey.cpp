// Private owned-window test TU. Never linked into the ordinary GUI.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "qt.hpp"
#include "maininterface/mainctx.hpp"
#include "maininterface/performance_preferences_model.hpp"
#include <vlc_process_priority.h>
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
vlc_process_priority_state initialPriority{};
bool haveInitialPriority=false;
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
    ownedWindow->requestActivate();item->forceActiveFocus(Qt::TabFocusReason);settle(60);
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
    if (haveInitialPriority) {
        vlc_process_priority_state restored;
        const bool ok=vlc_process_priority_apply(initialPriority.level,initialPriority.realtime,&restored)==VLC_SUCCESS;
        report.insert("initialPriorityRestored",ok);passed=passed&&ok;
    }
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
        haveInitialPriority=vlc_process_priority_get(&initialPriority)==VLC_SUCCESS;
        check(haveInitialPriority,"owned GUI initial scheduling class captured");
        ownedWindow->resize(1500,1000);ownedWindow->requestActivate();settle();
        auto *model=context->getPerformancePreferences();check(model!=nullptr,"real MainCtx owns Performance controller");
        DialogsProvider::getInstance()->showGlobalSettingsDialog();settle(250);
        auto *settings=find("juxtopposedGlobalSettings");check(settings && settings->property("visible").toBool(),"native settings popup opens");
        click("globalCategoryperformance");auto *page=find("juxtopposedPerformanceSettings");
        check(page && qvariant_cast<QObject*>(page->property("service"))==model,"compiled QML binds actual Performance controller");
        report.insert("devicePixelRatio",ownedWindow->devicePixelRatio());
        const QString folder=qEnvironmentVariable("VLC_OWNED_UI_OUTPUT");
        check(ownedWindow->grabWindow().save(folder+"/performance-native-dark.png"),"actual Performance capture saved");
        if (qEnvironmentVariable("VLC_OWNED_UI_MODE")==QStringLiteral("reopen")) {
            check(model->values().value("decodingThreads").toInt()==3,"decoding threads survive app restart");
            check(model->values().value("postprocessing").toInt()==3,"composed postprocessing survives restart");
            check(model->values().value("hardwareDecoder").toString()==QStringLiteral("off"),"hardware Off survives restart");
            check(!model->values().value("dropLateFrames").toBool(),"frame dropping survives restart");
            check(model->values().value("clockJitter").toInt()==1234 && model->values().value("clockAverage").toInt()==17,"clock defaults survive restart");
            check(model->values().value("encoder").toString()==QStringLiteral("flac") && model->values().value("decoder").toString()==QStringLiteral("avcodec"),"native module choices survive restart");
        } else {
            type("performanceDecodingThreads",QStringLiteral("3"));check(config_GetInt("avcodec-threads")==3,"native keyboard edit saves threads");
            auto chooseValue=[&](const char *name,const char *key,const QVariant &value) {
                const auto list=model->choices().value(QString::fromLatin1(key)).toList();
                for (qsizetype i=0;i<list.size();++i) if (list[i].toMap().value("value")==value) { choice(name,int(i));return; }
                check(false,"required native module choice is present");
            };
            chooseValue("performanceHardwareDecoder","hardwareDecoder",QStringLiteral("off"));
            check(!config_GetInt("hw-dec"),"actual dropdown disables hardware decoding");
            chooseValue("performancePostprocessing","postprocessing",3);
            check(config_GetInt("postproc-q")==3 && model->values().value("postprocessing").toInt()==3,"actual dropdown composes postproc filter");
            chooseValue("performanceLoopFilter","loopFilter",2);check(config_GetInt("avcodec-skiploopfilter")==2,"loop-filter native choice saved");
            click("performanceDropLateFrames");check(!config_GetInt("drop-late-frames"),"native toggle saves late-frame policy");
            const QString path=qEnvironmentVariable("VLC_OWNED_UI_CONFIG");
            HANDLE locked=CreateFileW(reinterpret_cast<LPCWSTR>(path.utf16()),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
            check(locked!=INVALID_HANDLE_VALUE,"owned config lock acquired");
            type("performanceProcessPriority",QStringLiteral("-1"));
            vlc_process_priority_state observed;vlc_process_priority_get(&observed);
            const bool rejected=!model->lastError().isEmpty() && observed.level==initialPriority.level && observed.realtime==initialPriority.realtime;
            CloseHandle(locked);check(rejected,"actual priority UI rejects unsaved edit and restores Windows class");
            QTest::keyClick(ownedWindow,Qt::Key_Return);settle();vlc_process_priority_get(&observed);
            check(model->lastError().isEmpty() && observed.level==-1,"actual priority field retries and reads exact Below-normal class");
            type("performanceProcessPriority",QString::number(initialPriority.level));
            check(model->lastError().isEmpty(),"actual priority field restores initial class before remaining GUI work");
            chooseValue("performanceEncoder","encoder",QStringLiteral("flac"));
            chooseValue("performanceDecoder","decoder",QStringLiteral("avcodec"));
            type("performanceClockJitter",QStringLiteral("1234 ms"));type("performanceClockAverage",QStringLiteral("17"));
            check(config_GetInt("clock-jitter")==1234 && config_GetInt("cr-average")==17,"actual Developer fields save native clocks");
            click("performanceAdvanced");check(!find("performanceDeveloperSection")->property("visible").toBool(),"Advanced collapses Developer locally");
            click("performanceAdvanced");check(find("performanceDeveloperSection")->property("visible").toBool(),"Advanced expands without leaving redesigned popup");
            bool sawBusy=false;
            auto connection=QObject::connect(model,&PerformancePreferencesModel::busyChanged,qApp,[&]{sawBusy=sawBusy||model->busy();});
            click("performanceRescan");
            for (int i=0;i<2000 && model->busy();++i) settle(20);
            QObject::disconnect(connection);
            check(sawBusy && !model->busy() && model->lastError().isEmpty(),"actual Rescan runs owned generator and completes normally");
            check(model->status().contains(QStringLiteral("Restart")),"actual Rescan explains next-start application");
            check(settings->property("visible").toBool(),"rescan and Advanced keep the actual redesigned popup open");
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
