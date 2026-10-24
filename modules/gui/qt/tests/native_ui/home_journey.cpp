// Private owned-window test TU. Never linked into the ordinary GUI.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "qt.hpp"
#include "maininterface/mainctx.hpp"
#include "maininterface/privacy_preferences_model.hpp"
#include "medialibrary/medialib.hpp"
#include "medialibrary/mlhomeactivity.hpp"
#include "medialibrary/mlhelper.hpp"

#include <QSettings>
#include "player/player_controller.hpp"
#include "util/csdbuttonmodel.hpp"
#include <QMenu>
#include <QListView>
#include <QPointer>
#include <QAction>
#include <QDeadlineTimer>
#include <QFileInfo>
#include <memory>
#include <functional>
#include <QDialog>
#include <QDir>
#include <QFileDialog>
#include "dialogs/dialogs_provider.hpp"
#include <vlc_configuration.h>
#include <vlc_player.h>
#include <vlc_input_item.h>
#include <vlc_interface.h>
#include <QApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QQmlContext>
#include <QQmlExpression>
#include <QAccessible>
#include <QColor>
#include <cmath>
#include <algorithm>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSet>
#include <QTest>
#include <QSignalSpy>
#include <QTimer>
#include <QWidget>
#include <windows.h>
#include <dbghelp.h>
#include <stdexcept>
#include <stdlib.h>

namespace {
using OwnedDumpWriter=BOOL(WINAPI*)(HANDLE,DWORD,HANDLE,MINIDUMP_TYPE,const PMINIDUMP_EXCEPTION_INFORMATION,const PMINIDUMP_USER_STREAM_INFORMATION,const PMINIDUMP_CALLBACK_INFORMATION);
OwnedDumpWriter ownedDumpWriter=nullptr;
std::wstring ownedDumpPath;
volatile LONG ownedDumpWritten=0,ownedShutdownStarted=0;
PVOID ownedExceptionHandler=nullptr;
LPTOP_LEVEL_EXCEPTION_FILTER previousExceptionFilter=nullptr;
LONG CALLBACK captureOwnedFault(PEXCEPTION_POINTERS info)
{
    const auto code=info->ExceptionRecord->ExceptionCode;
    const bool critical=code==EXCEPTION_ACCESS_VIOLATION||code==EXCEPTION_ILLEGAL_INSTRUCTION||code==EXCEPTION_STACK_OVERFLOW||code==0xc000041d||code==0xc0000409;
    const bool shutdownThrow=InterlockedCompareExchange(&ownedShutdownStarted,0,0)&&(code==0xe06d7363||code==0x20474343);
    if((critical||shutdownThrow)&&ownedDumpWriter&&InterlockedCompareExchange(&ownedDumpWritten,1,0)==0){
        const auto file=CreateFileW(ownedDumpPath.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(file!=INVALID_HANDLE_VALUE){
            MINIDUMP_EXCEPTION_INFORMATION exception{GetCurrentThreadId(),info,FALSE};
            ownedDumpWriter(GetCurrentProcess(),GetCurrentProcessId(),file,MINIDUMP_TYPE(MiniDumpNormal|MiniDumpWithThreadInfo),&exception,nullptr,nullptr);
            CloseHandle(file);
        }
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
void installOwnedFaultCapture()
{
    const auto library=LoadLibraryW(L"dbghelp.dll");
    if(!library)return;
    ownedDumpWriter=reinterpret_cast<OwnedDumpWriter>(GetProcAddress(library,"MiniDumpWriteDump"));
    ownedDumpPath=QDir(qEnvironmentVariable("VLC_OWNED_UI_OUTPUT")).filePath("native-fault.dmp").toStdWString();
    ownedExceptionHandler=AddVectoredExceptionHandler(1,captureOwnedFault);
    previousExceptionFilter=SetUnhandledExceptionFilter(captureOwnedFault);
    QObject::connect(qApp,&QCoreApplication::aboutToQuit,qApp,[]{
        if(ownedExceptionHandler)RemoveVectoredExceptionHandler(ownedExceptionHandler);
        SetUnhandledExceptionFilter(previousExceptionFilter);
    });
}
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
    QFile progress(QDir(qEnvironmentVariable("VLC_OWNED_UI_OUTPUT")).filePath("progress.json"));
    if(progress.open(QIODevice::WriteOnly))progress.write(QJsonDocument(QJsonObject{{"terminal",false},{"checks",checks}}).toJson());
    if (!ok) throw std::runtime_error(name);
}
void settle(int ms=120) { QTest::qWait(ms); }
void finish(bool passed,const QString &error={})
{
    if(context&&context->getMediaLibrary()){
        auto *home=context->getMediaLibrary()->homeActivity();
        report.insert("finalHome",QJsonObject{{"mediaCount",home->mediaCount()},{"folders",home->folders().size()},{"loading",home->loading()},{"error",home->error()}});
        report.insert("settingsFile",context->getIntf()->mainSettings->fileName());
    }
    QJsonArray inventory;
    for(auto *object:objects())if(!object->objectName().isEmpty()){
        auto *item=qobject_cast<QQuickItem*>(object);
        inventory.append(QJsonObject{{"name",object->objectName()},{"class",QString::fromLatin1(object->metaObject()->className())},{"visible",item&&item->isVisible()}});
    }
    report.insert("finalObjects",inventory);
    if(!passed&&ownedWindow)ownedWindow->grabWindow().save(QDir(qEnvironmentVariable("VLC_OWNED_UI_OUTPUT")).filePath("failed-window.png"));
    report.insert("passed",passed);report.insert("error",error);report.insert("checks",checks);
    report.insert("ownedWindowOnly",true);report.insert("ordinaryGuiContainsQa",false);
    report.insert("fullBetaAccepted",false);report.insert("windowsDpiMatrixAccepted",false);
    QFile output(qEnvironmentVariable("VLC_OWNED_UI_REPORT"));
    if(output.open(QIODevice::WriteOnly))output.write(QJsonDocument(report).toJson());
    _flushall();
    QTimer::singleShot(200,qApp,[]{
        InterlockedExchange(&ownedShutdownStarted,1);
        if(context)libvlc_Quit(vlc_object_instance(context->getIntf()));else QCoreApplication::quit();
    });
}
void until(const std::function<bool()> &ready,const char *name,int timeout=8000)
{
    QDeadlineTimer end(timeout);
    while(!ready()&&!end.hasExpired())QTest::qWait(20);
    check(ready(),name);settle(80);
}

QQuickItem *named(const QString &name)
{
    for(auto *object:objects())if(object->objectName()==name){
        auto *item=qobject_cast<QQuickItem*>(object);if(item&&item->isVisible())return item;
    }
    return nullptr;
}
void activate(QQuickItem *item)
{
    check(item&&item->isVisible()&&item->isEnabled(),"real Home action is visible and enabled");
    auto *targetWindow=item->window();check(targetWindow!=nullptr,"action belongs to owned window");
    targetWindow->requestActivate();item->forceActiveFocus(Qt::TabFocusReason);settle(120);
    const auto center=item->mapToScene(item->boundingRect().center());
    check(QRectF(QPointF{},QSizeF(targetWindow->size())).contains(center),"focused Home action is in owned window bounds");
    for(auto *p=item->parentItem();p;p=p->parentItem())if(p->clip())
        check(p->boundingRect().contains(p->mapFromScene(center)),"focused Home action scrolls into clipped viewport");
    auto targets=report.value("ownedPointerWindows").toArray();
    targets.append(QJsonObject{{"class",QString::fromLatin1(targetWindow->metaObject()->className())},{"isMainWindow",targetWindow==ownedWindow},{"width",targetWindow->width()},{"height",targetWindow->height()}});
    report.insert("ownedPointerWindows",targets);
    QTest::mouseClick(targetWindow,Qt::LeftButton,Qt::NoModifier,center.toPoint());settle();
}

MediaLib *library(){return context->getMediaLibrary();}
MLHomeActivity *activity(){return library()->homeActivity();}
QVariantList visits(){return context->getIntf()->mainSettings->value(QStringLiteral("Juxtopposed/home-visits-v1")).toList();}
quint64 visitCount(){const auto rows=visits();return rows.size()==1?rows.first().toMap().value("count").toULongLong():0;}
QVariantList roots()
{
    struct Probe{bool ready=false,ok=false;QVariantList rows;};
    auto result=std::make_shared<Probe>();
    library()->runOnMLThread<Probe>(library(),[](vlc_medialibrary_t *ml,Probe &probe){
        ml_unique_ptr<vlc_ml_folder_list_t> list(vlc_ml_list_entry_points(ml,nullptr));
        probe.ok=bool(list);
        if(list)for(const auto &folder:ml_range_iterate<vlc_ml_folder_t>(list))
            probe.rows.append(QVariantMap{{"id",qlonglong(folder.i_id)},{"url",QUrl(QString::fromUtf8(folder.psz_mrl))}});
    },[result](quint64,Probe &probe){result->ok=probe.ok;result->rows=probe.rows;result->ready=true;});
    until([&]{return result->ready;},"native root query completes on media-library worker");
    check(result->ok,"native root list is available");return result->rows;
}
void ownedRoot()
{
    const auto rows=roots();report.insert("nativeRoots",QJsonArray::fromVariantList(rows));
    check(rows.size()==1,"native library has exactly one owned scan root");
    check(mlHomeFolderKey(rows.first().toMap().value("url").toUrl())==mlHomeFolderKey(QUrl::fromLocalFile(qEnvironmentVariable("VLC_OWNED_AUDIO_FOLDER"))),"only the generated E folder is indexed");
}
void capture(const QString &name){check(ownedWindow->grabWindow().save(QDir(qEnvironmentVariable("VLC_OWNED_UI_OUTPUT")).filePath(name)),"owned Home window capture saved");}
void picker(bool cancel)
{
    bool seen=false,prepared=false,timedOut=false;QPointer<QFileDialog> current;
    const auto path=qEnvironmentVariable("VLC_OWNED_AUDIO_FOLDER");
    QDeadlineTimer deadline(12000);QTimer timer;timer.setInterval(150);
    QObject::connect(&timer,&QTimer::timeout,qApp,[&]{
        if(!current)for(auto *widget:QApplication::topLevelWidgets())
            if(auto *dialog=qobject_cast<QFileDialog*>(widget);dialog&&dialog->isVisible()){current=dialog;break;}
        if(deadline.hasExpired()){timedOut=true;if(current)current->reject();return;}
        if(!current||!current->isVisible())return;
        if(!seen){seen=true;current->grab().save(QDir(qEnvironmentVariable("VLC_OWNED_UI_OUTPUT")).filePath(cancel?"chooser-cancel.png":"chooser-accept.png"));}
        if(cancel){current->reject();return;}
        if(!prepared){
            current->setDirectory(QFileInfo(path).absolutePath());
            if(auto *focus=QApplication::focusWidget();focus&&current->isAncestorOf(focus))focus->clearFocus();
            current->selectFile(path);prepared=true;return;
        }
        const auto selected=current->selectedFiles();report.insert("chooserSelection",QJsonArray::fromStringList(selected));
        if(selected.size()==1&&QFileInfo(selected.first()).absoluteFilePath()==QFileInfo(path).absoluteFilePath())QMetaObject::invokeMethod(current,"accept",Qt::QueuedConnection);
        else{if(auto *focus=QApplication::focusWidget();focus&&current->isAncestorOf(focus))focus->clearFocus();current->selectFile(path);}
    });
    timer.start();activate(named(QStringLiteral("homeAddFolder")));timer.stop();
    check(seen,"Home Add Folder opens the actual Qt directory chooser");check(!timedOut,"directory chooser accepts or cancels within bounded wait");
}
void homeReady(){until([]{return named(QStringLiteral("homeSectioncontinueWatching"))&&!activity()->loading();},"real Home dashboard is ready");}
void folderReady(){
    until([]{return named(QStringLiteral("homeCollectionItem0"))&&named(QStringLiteral("homeCollectionItem1"));},"native folder route displays both indexed audio files");
    for(const auto *name:{"homeCollectionItem0","homeCollectionItem1"}){
        auto *card=named(QString::fromLatin1(name));
        QQmlExpression icon(qmlContext(card),card,QStringLiteral("VLCIcons.topbar_music"));
        const auto expected=icon.evaluate().toString();
        check(!icon.hasError()&&card->property("kind").toString()==QStringLiteral("audio")&&!expected.isEmpty()&&card->property("fallbackIcon").toString()==expected,"actual native audio folder card uses music fallback");
    }
}
void backHome(){activate(named(QStringLiteral("libraryBack")));homeReady();}
void forwardFolder(){activate(named(QStringLiteral("libraryForward")));folderReady();}
void openFolder(){activate(named(QStringLiteral("homefolder0")));folderReady();}
QQuickItem *homeButton()
{
    for(auto *object:objects())if(auto *item=qobject_cast<QQuickItem*>(object);item&&item->isVisible()&&item->isEnabled()&&item->window()==ownedWindow){
        auto *accessible=QAccessible::queryAccessibleInterface(item);
        if(accessible&&accessible->text(QAccessible::Name)==QStringLiteral("Home"))return item;
    }
    return nullptr;
}
QJsonObject primary()
{
    auto *player=context->getIntf()->p_mainPlayerController;auto *native=context->getIntf()->p_player;
    vlc_player_Lock(native);const auto time=vlc_player_GetTime(native);const auto state=vlc_player_GetState(native);const auto audio=vlc_player_GetTrackCount(native,AUDIO_ES);const auto video=vlc_player_GetTrackCount(native,VIDEO_ES);vlc_player_Unlock(native);
    return {{"time",qint64(time)},{"state",int(state)},{"position",player->getPosition()},{"rate",player->getRate()},{"url",player->getUrl().toString()},{"audioTracks",int(audio)},{"videoTracks",int(video)}};
}
QJsonObject playAndPause()
{
    activate(named(QStringLiteral("homeCollectionItem0")));
    auto *player=context->getIntf()->p_mainPlayerController;
    until([&]{return context->isPlayerView()&&player->getPlayingState()==PlayerController::PLAYING_STATE_PLAYING&&primary().value("time").toDouble()>200000;},"real folder card starts advancing native audio playback",15000);
    const auto playing=primary();report.insert("playingPrimary",playing);
    check(playing.value("audioTracks").toInt()>0&&playing.value("videoTracks").toInt()==0,"owned media is genuinely audio only");
    check(QFileInfo(player->getUrl().toLocalFile()).absolutePath()==qEnvironmentVariable("VLC_OWNED_AUDIO_FOLDER"),"primary input belongs to generated owned folder");
    auto *native=context->getIntf()->p_player;vlc_player_Lock(native);vlc_player_Pause(native);vlc_player_Unlock(native);
    until([&]{return player->getPlayingState()==PlayerController::PLAYING_STATE_PAUSED;},"native audio pauses before navigation invariance checks");
    int stable=0;auto previous=primary();QDeadlineTimer deadline(4000);QJsonArray samples;
    while(stable<4&&!deadline.hasExpired()){settle(100);const auto current=primary();samples.append(current);stable=current==previous?stable+1:0;previous=current;}
    report.insert("pauseSamples",samples);check(stable==4,"native and UI paused clocks settle");
    auto *input=player->getInput();check(input!=nullptr,"native audio input identity is available");input_item_Hold(input);
    std::unique_ptr<input_item_t,decltype(&input_item_Release)> hold(input,&input_item_Release);
    capture(QStringLiteral("audio-player.png"));
    activate(named(QStringLiteral("musicBack")));until([]{return !context->isPlayerView();},"actual music Back returns to library");
    activate(homeButton());homeReady();check(player->getInput()==input,"actual audio Back and Home preserve input identity");return previous;
}
void privacyOpen()
{
    activate(named(QStringLiteral("juxtopposedLibraryPreferences")));
    until([]{return named(QStringLiteral("globalCategoryprivacy"));},"library Preferences opens actual redesigned settings");
    activate(named(QStringLiteral("globalCategoryprivacy")));
    until([]{return named(QStringLiteral("privacysaveRecentlyPlayed"));},"real Privacy category mounts native service");
}
void settingsClose(){activate(named(QStringLiteral("globalSettingsClose")));settle(150);}
void historyPolicy(bool enabled)
{
    privacyOpen();auto *service=context->getPrivacyPreferences();
    check(service->values().value("saveRecentlyPlayed").toBool()!=enabled,"history toggle starts in opposite state for paired test");
    activate(named(QStringLiteral("privacysaveRecentlyPlayed")));
    until([&]{return service->values().value("saveRecentlyPlayed").toBool()==enabled;},"actual history toggle updates native policy");
    check(service->lastError().isEmpty(),"native history policy saves successfully");settingsClose();
}
void run()
{
    try{
        ownedWindow->resize(1280,900);ownedWindow->requestActivate();QAccessible::setActive(true);settle(200);
        check(library()&&activity(),"actual application owns native Home service");homeReady();
        check(activity()->error().isEmpty(),"native Home has no startup error");
        const auto mode=qEnvironmentVariable("VLC_OWNED_UI_MODE");
        const auto oraclePath=QDir(qEnvironmentVariable("VLC_OWNED_UI_BASE")).filePath("expectations.json");
        if(mode==QStringLiteral("bootstrap")){
            check(roots().isEmpty()&&activity()->mediaCount()==0&&visits().isEmpty(),"fresh isolated library has no roots media or visits");
            for(const auto *category:{"continueWatching","listenAgain","frequentFolders","frequentPlaylists","frequentArtists"})check(named(QStringLiteral("homeSection")+QString::fromLatin1(category)),"real empty Home section exists");
            capture(QStringLiteral("home-empty.png"));picker(true);settle(350);
            check(roots().isEmpty()&&activity()->mediaCount()==0&&visits().isEmpty(),"cancelled directory chooser adds no root media or visit");
            picker(false);
            until([]{return library()->idle()&&!activity()->loading()&&activity()->mediaCount()==2&&activity()->folders().size()==1;},"real audio-only Add Folder indexes two files and bootstraps Home",30000);
            check(activity()->error().isEmpty(),"indexing and initial Home visit report no error");ownedRoot();
            until([]{return visitCount()==1;},"accepted Add Folder records exactly one native folder visit");
            const auto row=activity()->folders().first().toMap();const auto id=row.value("id").value<MLItemId>();
            check(id.isValid()&&id.type==VLC_ML_PARENT_FOLDER&&row.value("count").toInt()==2,"Home folder is actual native folder ID with two media");
            check(mlHomeFolderKey(row.value("mrl").toUrl())==mlHomeFolderKey(QUrl::fromLocalFile(qEnvironmentVariable("VLC_OWNED_AUDIO_FOLDER"))),"Home card uses exact Unicode native folder URL");
            capture(QStringLiteral("home-indexed.png"));openFolder();until([]{return visitCount()==2;},"genuine folder route entry increments native visit once");
            capture(QStringLiteral("folder-audio.png"));backHome();forwardFolder();until([]{return visitCount()==3;},"Back Forward reentry records exactly one additional visit");
            auto paused=playAndPause();check(primary()==paused,"audio Back and Home preserve paused native input and clocks");
            until([]{return !activity()->loading()&&activity()->folders().size()==1;},"Home restores indexed folder after audio playback");
            const auto finalCount=visitCount();check(finalCount>=3,"accepted visit count survives audio library navigation");
            QFile oracle(oraclePath);check(oracle.open(QIODevice::WriteOnly),"restart expectations file opens");oracle.write(QJsonDocument(QJsonObject{{"id",id.toString()},{"count",qint64(finalCount)}}).toJson());oracle.close();
            report.insert("folderId",id.toString());report.insert("finalVisitCount",qint64(finalCount));capture(QStringLiteral("home-after-audio.png"));
        }else{
            ownedRoot();until([]{return library()->idle()&&activity()->mediaCount()==2&&!activity()->loading();},"restart retains both indexed media without new scan root");
            if(mode==QStringLiteral("clear-reopen")){
                check(visits().isEmpty()&&activity()->folders().isEmpty(),"history clear survives process restart without bootstrap resurrection");settle(700);
                check(visits().isEmpty()&&activity()->folders().isEmpty(),"cleared native Home stays empty after queued startup work");
                check(context->getPrivacyPreferences()->values().value("saveRecentlyPlayed").toBool(),"re-enabled history policy persists across restart");capture(QStringLiteral("home-cleared-restart.png"));
            }else{
                QFile oracle(oraclePath);check(oracle.open(QIODevice::ReadOnly),"restart reads only prior observed expectations");const auto expected=QJsonDocument::fromJson(oracle.readAll()).object();
                until([]{return activity()->folders().size()==1;},"native Home restores saved folder activity after actual restart");
                const auto row=activity()->folders().first().toMap();check(row.value("id").value<MLItemId>().toString()==expected.value("id").toString()&&visitCount()==quint64(expected.value("count").toDouble()),"restart preserves exact real folder identity and observed visit count");capture(QStringLiteral("home-restart.png"));
                openFolder();auto paused=playAndPause();auto *input=context->getIntf()->p_mainPlayerController->getInput();input_item_Hold(input);std::unique_ptr<input_item_t,decltype(&input_item_Release)> hold(input,&input_item_Release);
                historyPolicy(false);const auto before=visits();openFolder();settle(600);backHome();forwardFolder();settle(600);
                check(visits()==before,"native folder reentry with history OFF does not record visits");backHome();historyPolicy(true);
                const auto count=visitCount();openFolder();until([&]{return visitCount()==count+1;},"paired history ON folder entry records one real visit");backHome();
                privacyOpen();activate(named(QStringLiteral("privacyClearHistory")));auto *privacy=context->getPrivacyPreferences();
                until([&]{return !privacy->busy()&&privacy->lastError().isEmpty()&&visits().isEmpty();},"real Clear History completes native and local stores",15000);settingsClose();
                until([]{return !activity()->loading()&&activity()->folders().isEmpty();},"clear removes Home visit cards");settle(700);
                check(visits().isEmpty()&&activity()->folders().isEmpty(),"clear does not resurrect queued folder visits");check(activity()->mediaCount()==2,"history clear retains indexed media");ownedRoot();
                check(primary()==paused&&context->getIntf()->p_mainPlayerController->getInput()==input,"privacy and folder navigation preserve exact paused input and both clocks");capture(QStringLiteral("home-cleared.png"));
            }
        }
        finish(true);
    }catch(const std::exception &e){finish(false,QString::fromUtf8(e.what()));}
}
void waitForWindow(int attempts)
{
    auto *host=qobject_cast<QQuickItem*>(find("juxtopposedDialogsHost"));
    if (host && host->window() && host->isVisible()) {
        auto *engine=qmlEngine(host);
        auto *candidate=engine ? engine->singletonInstance<MainCtx*>("VLC.MainInterface","MainCtx") : nullptr;
        if(candidate && find("juxtopposedGlobalSettings")) {
            context=candidate;ownedWindow=host->window();
            QTimer::singleShot(250,qApp,run);return;
        }
    }
    if (attempts>0) QTimer::singleShot(250,qApp,[attempts]{waitForWindow(attempts-1);});
    else {
        QJsonArray inventory;
        for(auto *object:objects())if(!object->objectName().isEmpty())inventory.append(QJsonObject{{"name",object->objectName()},{"class",QString::fromLatin1(object->metaObject()->className())}});
        report.insert("startupObjects",inventory);
        finish(false,QStringLiteral("Actual settings host did not become ready"));
    }
}
void startOwnedJourney()
{
    if (qEnvironmentVariableIsEmpty("VLC_OWNED_UI_REPORT")) return;
    _set_error_mode(_OUT_TO_STDERR);
    installOwnedFaultCapture();
    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    QTimer::singleShot(1000,qApp,[]{waitForWindow(120);});
}
}
Q_COREAPP_STARTUP_FUNCTION(startOwnedJourney)
