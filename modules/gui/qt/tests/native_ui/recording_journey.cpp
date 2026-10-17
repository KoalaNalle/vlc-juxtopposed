// Private owned-window test TU. Never linked into the ordinary GUI.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "qt.hpp"
#include "maininterface/mainctx.hpp"
#include "maininterface/recording_preferences_model.hpp"
#include "player/player_controller.hpp"
#include "util/renderer_manager.hpp"
#include "dialogs/sout/sout.hpp"
#include <vlc_renderer_discovery.h>
#include <vlc_player.h>
#include <QCheckBox>
#include <QDialog>
#include <QDir>
#include <QFileDialog>
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
#include <QStyle>
#include <QStyleOptionButton>
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
QString nativeText(const char *key)
{
    char *raw=config_GetPsz(key);const auto value=QString::fromUtf8(raw ? raw : "");free(raw);return value;
}
void finish(bool passed,const QString &error={})
{
    if (context) {
        auto *player=context->getIntf()->p_player;
        vlc_player_Lock(player);vlc_player_SetRenderer(player,nullptr);vlc_player_Unlock(player);
    }
    report.insert("passed",passed);report.insert("error",error);report.insert("checks",checks);
    report.insert("ownedWindowOnly",true);report.insert("ordinaryGuiContainsQa",false);
    report.insert("folderChooserImplementation",QStringLiteral("Real QFileDialog, Qt widget mode in private test only"));
    report.insert("liveCastEndpointAccepted",false);report.insert("windowsDpiMatrixAccepted",false);report.insert("fullBetaAccepted",false);
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
        auto *model=context->getRecordingPreferences();check(model!=nullptr,"real MainCtx owns Recording controller");
        DialogsProvider::getInstance()->showGlobalSettingsDialog();settle(250);
        auto *settings=find("juxtopposedGlobalSettings");check(settings && settings->property("visible").toBool(),"native settings popup opens");
        click("globalCategoryrecording");auto *page=find("juxtopposedRecordingSettings");
        check(page && qvariant_cast<QObject*>(page->property("service"))==model,"compiled QML binds actual Recording controller");
        const auto base=qEnvironmentVariable("VLC_OWNED_UI_BASE");
        const auto chosen=QDir(base).filePath(QString::fromUtf8("Chosen Records Ää"));
        check(QDir().mkpath(chosen),"owned Unicode recording directory exists");
        const auto output=qEnvironmentVariable("VLC_OWNED_UI_OUTPUT");
        check(ownedWindow->grabWindow().save(output+"/recording-native-dark.png"),"actual Recording capture saved");
        report.insert("devicePixelRatio",ownedWindow->devicePixelRatio());
        if (qEnvironmentVariable("VLC_OWNED_UI_MODE")==QStringLiteral("reopen")) {
            check(model->values().value("recordingDirectory").toString()==chosen,"chosen recording directory survives app restart");
            check(!model->values().value("nativeRecording").toBool(),"native recording preference survives restart");
            check(!model->values().value("discoverDevices").toBool(),"discovery Off survives restart");
            auto *renderer=context->getIntf()->p_mainPlayerController->getRendererManager();
            renderer->StartScan();
            check(renderer->getStatus()==RendererManager::IDLE && renderer->getScanRemain()==0,"stored discovery Off prevents a real restarted scan");
            check(model->values().value("playLocally").toBool(),"local streaming default survives restart");
            check(!model->values().value("includeAudio").toBool() && !model->values().value("includeVideo").toBool() && model->values().value("includeSubtitles").toBool(),"three independent output flags survive restart");
            check(model->values().value("encodingPreset").toString()==QStringLiteral("fast") && model->values().value("tuning").toString().isEmpty(),"encoder preset and cleared tuning survive restart");
        } else {
            const auto typed=QDir(base).filePath(QString::fromUtf8("Typed Records Ää"));
            check(QDir().mkpath(typed),"owned typed recording directory exists");
            type("recordingDirectory",typed);check(nativeText("input-record-path")==typed,"actual keyboard folder edit saves native Unicode path");
            const auto beforePicker=nativeText("input-record-path");
            auto picker=[&](bool accept) {
                bool observed=false,guarded=false;
                QTimer timer;timer.setSingleShot(true);
                QObject::connect(&timer,&QTimer::timeout,qApp,[&] {
                    for (auto *widget:QApplication::topLevelWidgets()) {
                        if (auto *dialog=qobject_cast<QFileDialog*>(widget);dialog && dialog->isVisible()) {
                            observed=true;guarded=model->busy() && settings->property("busy").toBool() && !find("globalSettingsClose")->property("enabled").toBool();
                            if (accept) { dialog->setDirectory(chosen);QMetaObject::invokeMethod(dialog,"accept",Qt::QueuedConnection); }
                            else dialog->reject();
                            break;
                        }
                    }
                });
                timer.start(250);
                click("recordingBrowse");
                check(observed && guarded && !model->busy(),"real Qt folder chooser has guarded lifetime and closes normally");
            };
            picker(false);check(nativeText("input-record-path")==beforePicker,"cancelling real chooser preserves native recording path");
            picker(true);check(nativeText("input-record-path")==chosen,"accepting real chooser saves existing Unicode directory");
            const auto config=qEnvironmentVariable("VLC_OWNED_UI_CONFIG");
            HANDLE locked=CreateFileW(reinterpret_cast<LPCWSTR>(config.utf16()),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
            check(locked!=INVALID_HANDLE_VALUE,"owned config lock acquired");
            const bool nativeBefore=model->values().value("nativeRecording").toBool();
            click("recordingNative");
            const bool rejected=!model->lastError().isEmpty() && model->values().value("nativeRecording").toBool()==nativeBefore;
            CloseHandle(locked);check(rejected,"actual recording toggle rejects an unsaved edit without optimistic state");
            click("recordingNative");check(model->lastError().isEmpty() && !config_GetInt("input-record-native"),"actual recording toggle retries native preference after unlocking");
            check(model->supportNotes().value("nativeRecording").toString().contains(QStringLiteral("does not force re-encoding")),"native recording help states approved fallback semantics");
            auto chooseValue=[&](const char *name,const char *key,const QVariant &value) {
                const auto entries=model->choices().value(QString::fromLatin1(key)).toList();
                for (qsizetype index=0;index<entries.size();++index) if (entries[index].toMap().value("value")==value) { choice(name,int(index));return; }
                check(false,"required native choice is present");
            };
            for (const auto &entry:model->choices().value("castQuality").toList()) {
                const auto value=entry.toMap().value("value");
                chooseValue("recordingCastQuality","castQuality",value);
                check(config_GetInt("sout-chromecast-conversion-quality")==value.toLongLong(),"actual cast-quality choice saves native future-cast policy");
            }
            chooseValue("recordingEncodingPreset","encodingPreset",QStringLiteral("fast"));
            chooseValue("recordingTuning","tuning",QStringLiteral("zerolatency"));
            chooseValue("recordingTuning","tuning",QString{});
            check(nativeText("sout-x264-preset")==QStringLiteral("fast") && nativeText("sout-x264-tune").isEmpty(),"actual encoder choices save Fast and clear tuning to None");
            click("recordingIncludeAudio");click("recordingIncludeVideo");click("recordingIncludeSubtitles");
            check(!config_GetInt("sout-spu"),"actual subtitle toggle disables VLC's native On default");
            click("recordingIncludeSubtitles");
            check(!config_GetInt("sout-audio") && !config_GetInt("sout-video") && config_GetInt("sout-spu"),"actual output toggles independently save subtitle-only defaults");
            click("recordingPlayLocally");check(config_GetInt("sout-display"),"actual local-playback toggle saves next-wizard default");
            auto *renderer=context->getIntf()->p_mainPlayerController->getRendererManager();
            click("recordingDiscoverDevices");renderer->StartScan();
            check(renderer->getStatus()==RendererManager::IDLE && renderer->getScanRemain()==0,"actual discovery Off prevents Cast-menu scan");
            click("recordingDiscoverDevices");renderer->StartScan();settle(250);
            check(renderer->getStatus()==RendererManager::RUNNING && renderer->getScanRemain()>0,"actual discovery On permits bounded native renderer scan");
            auto *player=context->getIntf()->p_player;
            auto *item=vlc_renderer_item_new("owned-test","Owned selected renderer","dummy://127.0.0.1:9",nullptr,nullptr,nullptr,VLC_RENDERER_CAN_VIDEO);
            check(item!=nullptr,"owned renderer selection item created without receiver playback");
            vlc_player_Lock(player);vlc_player_SetRenderer(player,item);vlc_player_Unlock(player);settle();
            vlc_renderer_item_release(item);
            check(renderer->useRenderer(),"actual player renderer selection reaches Qt manager");
            click("recordingDiscoverDevices");
            vlc_player_Lock(player);const bool preserved=vlc_player_GetRenderer(player)==item;vlc_player_Unlock(player);
            check(renderer->getStatus()==RendererManager::IDLE && renderer->getScanRemain()==0 && renderer->useRenderer() && preserved,"saving discovery Off stops native scan and preserves selected renderer");
            renderer->disableRenderer();settle();check(!renderer->useRenderer(),"owned renderer selection cleaned up normally");
            QMetaObject::invokeMethod(settings,"close");settle();
            {
                SoutDialog wizard(ownedWindow,context->getIntf(),QStringLiteral("owned-source"));wizard.show();wizard.next();settle();
                auto *local=wizard.findChild<QCheckBox*>(QStringLiteral("localOutput"));
                if (local) report.insert("wizardInitialState",QJsonObject{{"visible",local->isVisible()},{"enabled",local->isEnabled()},{"page",wizard.currentId()},{"checked",local->isChecked()},{"width",local->width()}});
                check(local && local->isVisible() && local->isEnabled() && local->isChecked() && wizard.getChain().contains(QStringLiteral("display")),"real Stream wizard initializes local output from saved On");
                auto clickLocal=[&] {
                    QStyleOptionButton option;option.initFrom(local);option.text=local->text();
                    const auto indicator=local->style()->subElementRect(QStyle::SE_CheckBoxIndicator,&option,local);
                    const bool before=local->isChecked();
                    QTest::mouseClick(local,Qt::LeftButton,Qt::NoModifier,indicator.center());settle();
                    auto clicks=report.value("wizardClicks").toArray();
                    clicks.append(QJsonObject{{"beforeChecked",before},{"afterChecked",local->isChecked()},{"visible",local->isVisible()},{"enabled",local->isEnabled()},{"indicatorX",indicator.center().x()},{"indicatorY",indicator.center().y()},{"chain",wizard.getChain()}});
                    report.insert("wizardClicks",clicks);
                };
                check(model->setValue(QStringLiteral("playLocally"),false),"future Stream default changed while wizard remains open");
                check(local->isChecked(),"open wizard keeps its local-output snapshot");
                clickLocal();
                check(!local->isChecked() && !wizard.getChain().contains(QStringLiteral("display")),"explicit wizard Off overrides saved initialization in actual chain");
                clickLocal();
                check(local->isChecked() && wizard.getChain().contains(QStringLiteral("display")) && !config_GetInt("sout-display"),"explicit wizard On overrides changed default without resaving it");
                wizard.close();settle();
            }
            {
                SoutDialog wizard(ownedWindow,context->getIntf(),QStringLiteral("owned-source"));
                auto *local=wizard.findChild<QCheckBox*>(QStringLiteral("localOutput"));
                check(local && !local->isChecked() && !wizard.getChain().contains(QStringLiteral("display")),"next Stream wizard initializes from saved Off");
            }
            DialogsProvider::getInstance()->showGlobalSettingsDialog();settle();click("globalCategoryrecording");click("recordingPlayLocally");
            check(config_GetInt("sout-display"),"native page restores saved On for restart verification");
            click("recordingAdvanced");settle();
            check(!settings->property("visible").toBool() && DialogsProvider::getInstance()->prefsDialogVisible(),"original Advanced action opens actual native expert preferences after modal closes");
            for (auto *widget:QApplication::topLevelWidgets()) if (auto *dialog=qobject_cast<QDialog*>(widget);dialog && dialog->isVisible()) dialog->reject();
            settle();check(!DialogsProvider::getInstance()->prefsDialogVisible(),"native expert preferences close without closing VLC");
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
    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    QTimer::singleShot(1000,qApp,[]{waitForWindow(40);});
}
}
Q_COREAPP_STARTUP_FUNCTION(startOwnedJourney)
