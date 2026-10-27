// Opt-in real profile-conflict error visibility. Never in the ordinary plugin.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "qt.hpp"
#include "maininterface/mainctx.hpp"
#include "maininterface/general_preferences_model.hpp"
#include "maininterface/player_profiles_model.hpp"
#include "maininterface/player_profiles_store.hpp"
#include "dialogs/dialogs_provider.hpp"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSet>
#include <QSettings>
#include <QTest>
#include <QTimer>
#include <stdexcept>

namespace {
MainCtx *context=nullptr;QQuickWindow *window=nullptr;
QJsonObject report;QJsonArray checks;
void save()
{
    report.insert("checks",checks);report.insert("privateFixture",true);report.insert("fullBetaAccepted",false);
    QFile f(qEnvironmentVariable("VLC_OWNED_UI_REPORT"));if(f.open(QIODevice::WriteOnly))f.write(QJsonDocument(report).toJson());
}
void check(bool value,const char *name)
{
    checks.append(QJsonObject{{"name",name},{"passed",value}});save();if(!value)throw std::runtime_error(name);
}
QList<QObject *> objects()
{
    QList<QObject *> pending,found;QSet<QObject *> seen;
    for(auto *candidate:QGuiApplication::allWindows())if(auto *quick=qobject_cast<QQuickWindow *>(candidate))pending.append(quick->contentItem());
    while(!pending.isEmpty()){
        auto *object=pending.takeLast();if(!object||seen.contains(object))continue;seen.insert(object);found.append(object);pending.append(object->children());
        if(auto *item=qobject_cast<QQuickItem *>(object))for(auto *child:item->childItems())pending.append(child);
    }
    return found;
}
QQuickItem *item(const char *name)
{for(auto *object:objects())if(object->objectName()==QLatin1String(name))if(auto *quick=qobject_cast<QQuickItem *>(object))return quick;return nullptr;}
void click(QQuickItem *target)
{check(target&&target->isVisible()&&target->isEnabled(),"actual profile control is visible and enabled");QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,target->mapToScene(QPointF(target->width()/2,target->height()/2)).toPoint());QTest::qWait(100);}
QByteArray bytes(const QString &path)
{QFile f(path);return f.open(QIODevice::ReadOnly)?f.readAll():QByteArray{};}
void bounds(const QString &label)
{
    QTest::qWait(150);auto *error=item("playerProfileError");auto *scroll=item("playerProfileScroll");
    check(error&&error->isVisible()&&scroll,"actual native conflict label and viewport exist");
    auto *flick=scroll->property("contentItem").value<QQuickItem *>();check(flick,"actual profile ScrollView has a native Quick viewport");
    const auto rect=error->mapRectToItem(flick,error->boundingRect());
    report.insert(label,QJsonObject{{"top",rect.top()},{"bottom",rect.bottom()},{"viewportHeight",flick->height()},{"error",error->property("text").toString()}});save();
    check(rect.top()>=-.5&&rect.bottom()<=flick->height()+.5,"entire native profile recovery error fits the visible viewport");
    check(window->grabWindow().save(QDir(qEnvironmentVariable("VLC_OWNED_UI_BASE")).filePath(label+QStringLiteral(".png"))),"native conflict error capture saved");
}
void run()
{
    try {
        window->resize(1280,900);context->setIntfUserScaleFactor(1.0);QTest::qWait(150);
        auto *profiles=context->getPlayerProfiles();auto *settings=context->getIntf()->mainSettings;const auto path=settings->fileName();
        check(QFileInfo(path).absoluteFilePath().startsWith(QDir(qEnvironmentVariable("VLC_OWNED_UI_BASE")).absolutePath()+QLatin1Char('/')),"native conflict fixture is fenced to owned settings");
        check(context->getGeneralPreferences()->setValue(QStringLiteral("theme"),2),"actual General owner establishes native Dark for initial checks");
        check(profiles&&profiles->available()&&profiles->createProfile(QStringLiteral("Owned seed")),"actual MainCtx saves a valid native profile seed");
        QSettings peerSettings(path,QSettings::IniFormat);PlayerProfilesStore peer(&peerSettings);QString error,id;
        check(peer.reload({},error),"independent production store reloads the valid saved native snapshots");
        const auto current=peer.profiles()[peer.indexOf(peer.selectedId())].snapshot;
        check(peer.create(QStringLiteral("Owned competing commit"),current,id,error),"competing catalog commit makes the actual MainCtx profile model stale");
        const auto accepted=bytes(path);
        DialogsProvider::getInstance()->showPlayerProfilesDialog(true);QTest::qWait(150);
        auto *name=item("playerProfileName");check(name&&name->isVisible(),"actual New profile dialog opens");name->setProperty("text",QStringLiteral("Rejected stale draft"));QTest::qWait(50);
        click(item("playerProfileSave"));
        check(profiles->lastError()==QStringLiteral("Player profiles changed in another window. Restart to reload them before editing."),"actual failed native save supplies the complete conflict recovery message");
        check(bytes(path)==accepted&&profiles->selectedName()==QStringLiteral("Owned seed"),"rejected draft preserves the accepted catalog and local active profile");
        bounds(QStringLiteral("native-standard-dark"));
        context->setIntfUserScaleFactor(1.25);window->resize(650,480);bounds(QStringLiteral("native-compact-dark-125"));
        check(context->getGeneralPreferences()->setValue(QStringLiteral("theme"),1),"actual General owner commits native Light");QTest::qWait(250);
        bounds(QStringLiteral("native-compact-light-125"));
        context->setIntfUserScaleFactor(1.0);window->resize(1280,900);bounds(QStringLiteral("native-standard-light"));
        check(profiles->lastError().contains(QStringLiteral("Restart"))&&name->property("text").toString()==QStringLiteral("Rejected stale draft"),"revealing errors preserves the recovery instruction and entered name");
        click(item("playerProfileCancel"));check(!item("playerProfileError")->isVisible(),"Cancel dismisses the native error dialog");
        peerSettings.sync();PlayerProfilesStore restored(&peerSettings);check(restored.reload({},error)&&restored.profiles().size()==6&&restored.selectedId()==id,"Cancel and unrelated appearance saves retain the competing catalog");
        report.insert("passed",true);report.insert("status","checks_complete");save();DialogsProvider::getInstance()->quit();
    }catch(const std::exception &error){report.insert("passed",false);report.insert("error",error.what());save();DialogsProvider::getInstance()->quit();}
}
void findWindow(int attempts)
{
    for(auto *object:objects())if(object->objectName()==QStringLiteral("juxtopposedDialogsHost"))if(auto *quick=qobject_cast<QQuickItem *>(object)){
        auto *engine=qmlEngine(object);context=engine?engine->singletonInstance<MainCtx *>("VLC.MainInterface","MainCtx"):nullptr;window=quick->window();
    }
    if(context&&window){
        const auto gate=qEnvironmentVariable("VLC_OWNED_UI_START_GATE");if(gate.isEmpty()||QFile::exists(gate)){QTimer::singleShot(150,context,run);return;}
        report.insert("status","awaiting-owned-window-inspection");save();
    }
    if(attempts>0)QTimer::singleShot(250,qApp,[attempts]{findWindow(attempts-1);});
    else {report.insert("passed",false);report.insert("error","owned window/gate unavailable");save();}
}
void start(){if(!qEnvironmentVariableIsEmpty("VLC_OWNED_UI_REPORT"))QTimer::singleShot(800,qApp,[]{findWindow(1200);});}
}
Q_COREAPP_STARTUP_FUNCTION(start)
