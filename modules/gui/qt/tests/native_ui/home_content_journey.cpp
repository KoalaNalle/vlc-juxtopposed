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
#include "dialogs/new_playlist_model.hpp"
#include "menus/menus.hpp"
#include "util/navigation_history.hpp"

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
        report.insert("finalHome",QJsonObject{{"mediaCount",home->mediaCount()},{"folders",home->folders().size()},{"loading",home->loading()},{"error",home->error()},{"playlists",home->playlists().size()},{"artists",home->artists().size()}});
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
    QTest::mouseMove(targetWindow,center.toPoint());settle(350);
    until([&]{for(auto *a=item;a;a=a->parentItem())if(a->opacity()<.95)return false;return item->isVisible();},"actual pointer target is rendered before click",2500);
    auto actual=report.value("actualTargets").toArray();actual.append(QJsonObject{{"name",item->objectName()},{"x",center.x()},{"y",center.y()},{"width",targetWindow->width()},{"height",targetWindow->height()},{"opacity",item->opacity()}});report.insert("actualTargets",actual);
    QTest::mouseClick(targetWindow,Qt::LeftButton,Qt::NoModifier,center.toPoint());settle();
}

MediaLib *library(){return context->getMediaLibrary();}
MLHomeActivity *activity(){return library()->homeActivity();}
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
NavigationHistory *history(){return qmlEngine(find("juxtopposedDialogsHost"))->singletonInstance<NavigationHistory*>("VLC.MainInterface","History");}
void recordRoute(const QString &step){auto rows=report.value("routes").toArray();rows.append(QJsonObject{{"step",step},{"path",QJsonArray::fromStringList(history()->viewPath())}});report.insert("routes",rows);}
void toHome(){activate(homeButton());homeReady();recordRoute(QStringLiteral("Home"));}
QQuickItem *contentCard(const QString &title){for(int i=0;i<3;++i)if(auto *item=named(QStringLiteral("homeCollectionItem")+QString::number(i));item&&item->property("title").toString()==title)return item;return nullptr;}
void contentFolder(){activate(named(QStringLiteral("homefolder0")));until([]{return contentCard(QStringLiteral("First owned track"))&&contentCard(QStringLiteral("Second owned track"))&&contentCard(QStringLiteral("owned-clip.mp4"));},"real mixed folder shows both tagged tracks and owned video",15000);}
void playContent(const QString &title,bool video){
    activate(contentCard(title));auto *player=context->getIntf()->p_mainPlayerController;
    until([&]{const auto s=primary();return context->isPlayerView()&&player->getPlayingState()==PlayerController::PLAYING_STATE_PLAYING&&s.value("time").toDouble()>200000&&(s.value("videoTracks").toInt()>0)==video;},"real owned card starts expected native media kind",15000);
    check(player->getUrl().toLocalFile().startsWith(qEnvironmentVariable("VLC_OWNED_AUDIO_FOLDER")+QLatin1Char('/')),"primary media remains inside owned Unicode E folder");
}
void seekQuarter(){
    // Prepare the actual player, never database progress or supplied Home rows.
    auto *native=context->getIntf()->p_player;vlc_player_Lock(native);vlc_player_SetPosition(native,.25);vlc_player_Unlock(native);
    until([]{auto s=primary();return s.value("time").toDouble()>14000000&&s.value("time").toDouble()<19000000;},"actual native seek reaches quarter position");settle(300);
}
void movePreviewAway(){
    until([]{return named(QStringLiteral("pip window"));},"native floating preview is ready for real drag");
    auto *pip=named(QStringLiteral("pip window"));auto *player=context->getIntf()->p_mainPlayerController;auto *input=player->getInput();check(input!=nullptr,"preview drag begins with actual primary input");input_item_Hold(input);std::unique_ptr<input_item_t,decltype(&input_item_Release)> hold(input,&input_item_Release);
    const auto before=primary();const auto beforePos=pip->mapToScene(QPointF{});const auto local=QPointF(pip->width()/2,pip->height()/4);const auto start=pip->mapToScene(local);
    const auto end=pip->parentItem()->mapToScene(QPointF(pip->property("dragXMax").toDouble()-16+local.x(),pip->property("dragYMin").toDouble()+16+local.y()));
    check(QRectF(QPointF{},QSizeF(ownedWindow->size())).contains(start)&&QRectF(QPointF{},QSizeF(ownedWindow->size())).contains(end),"real preview drag endpoints stay inside owned window");
    QTest::mouseMove(ownedWindow,start.toPoint());settle(150);QTest::mousePress(ownedWindow,Qt::LeftButton,Qt::NoModifier,start.toPoint());
    for(int i=1;i<=16;++i){QTest::mouseMove(ownedWindow,(start+(end-start)*(i/16.)).toPoint());settle(25);}
    QTest::mouseRelease(ownedWindow,Qt::LeftButton,Qt::NoModifier,end.toPoint());settle(250);
    const auto afterPos=pip->mapToScene(QPointF{});report.insert("previewDrag",QJsonObject{{"fromX",beforePos.x()},{"fromY",beforePos.y()},{"toX",afterPos.x()},{"toY",afterPos.y()},{"before",before},{"after",primary()}});
    check(afterPos.x()>beforePos.x()+200,"actual pointer drag moves floating video away from left-side library controls");
    const auto after=primary();check(player->getInput()==input&&after.value("state")==before.value("state")&&after.value("url")==before.value("url")&&after.value("rate")==before.value("rate")&&after.value("time").toDouble()>=before.value("time").toDouble(),"real preview drag preserves actual input state URL rate and clock");
}
void leavePlayer(bool video){
    QTest::mouseMove(ownedWindow,QPoint(ownedWindow->width()/2,qMin(80,ownedWindow->height()/2)));settle(350);
    if(video){QQuickItem *back=nullptr;for(auto *object:objects())if(auto *item=qobject_cast<QQuickItem*>(object);item&&item->isVisible()&&item->objectName()==QStringLiteral("IconToolButton")&&item->property("description").toString()==QStringLiteral("Back")){back=item;break;}activate(back);}
    else activate(named(QStringLiteral("musicBack")));
    until([]{return !context->isPlayerView();},"real player Back returns to library while input continues");toHome();if(video)movePreviewAway();
}
void enterText(QQuickItem *item,const QString &text){activate(item);QTest::keyClick(ownedWindow,Qt::Key_A,Qt::ControlModifier);if(text.isEmpty())QTest::keyClick(ownedWindow,Qt::Key_Backspace);else{QKeyEvent key(QEvent::KeyPress,0,Qt::NoModifier,text);QCoreApplication::sendEvent(ownedWindow,&key);}settle(120);}
void playlistPicker(){
    bool seen=false,prepared=false,timedOut=false;QPointer<QFileDialog> dialog;QDeadlineTimer deadline(12000);QTimer timer;timer.setInterval(150);
    const auto path=QDir(qEnvironmentVariable("VLC_OWNED_AUDIO_FOLDER")).filePath(QStringLiteral("first.flac"));
    QObject::connect(&timer,&QTimer::timeout,qApp,[&]{
        if(!dialog)for(auto *widget:QApplication::topLevelWidgets())if(auto *d=qobject_cast<QFileDialog*>(widget);d&&d->isVisible()){dialog=d;break;}
        if(deadline.hasExpired()){timedOut=true;if(dialog)dialog->reject();return;}
        if(!dialog||!dialog->isVisible())return;
        seen=true;
        if(!prepared){dialog->setDirectory(QFileInfo(path).absolutePath());if(auto *focus=QApplication::focusWidget();focus&&dialog->isAncestorOf(focus))focus->clearFocus();dialog->selectFile(path);prepared=true;return;}
        auto files=dialog->selectedFiles();if(files.size()==1&&QFileInfo(files.first()).absoluteFilePath()==QFileInfo(path).absoluteFilePath())QMetaObject::invokeMethod(dialog,"accept",Qt::QueuedConnection);
        else{if(auto *focus=QApplication::focusWidget();focus&&dialog->isAncestorOf(focus))focus->clearFocus();dialog->selectFile(path);}
    });timer.start();activate(named(QStringLiteral("newPlaylistSelectFiles")));timer.stop();check(seen&&!timedOut,"actual New Playlist chooser accepts owned indexed audio");
}
bool nativeModel(const char *type,MLItemId parent,int count){for(auto *o:objects())if(o->inherits(type)&&o->property("parentId").value<MLItemId>()==parent&&o->property("count").toInt()==count)return true;return false;}
MLItemId createPlaylist(){
    QMenu menu;VLCMenuBar::FileMenu(context->getIntf(),&menu);QAction *action=nullptr;for(auto *a:menu.actions())if(a->text().remove(QLatin1Char('&'))==QStringLiteral("New Playlist...")){action=a;break;}
    check(action&&action->isEnabled(),"production File menu exposes New Playlist");action->trigger();until([]{return named(QStringLiteral("newPlaylistName"));},"real native File action opens designed New Playlist");
    enterText(named(QStringLiteral("newPlaylistName")),QStringLiteral("Travel Mix Ää"));playlistPicker();auto *model=DialogsProvider::getInstance()->newPlaylist();
    until([&]{return model->canSave()&&model->files().size()==1;},"real playlist model accepts native file selection");activate(named(QStringLiteral("newPlaylistSave")));
    until([&]{return !model->busy()&&model->savedPlaylist().isValid()&&!named(QStringLiteral("newPlaylistName"));},"real Save creates native playlist and closes dialog",15000);check(model->error().isEmpty(),"native playlist save reports no failure");
    const auto id=model->savedPlaylist();recordRoute(QStringLiteral("Created playlist"));check(history()->viewPath()==QStringList{QStringLiteral("playlists"),QStringLiteral("list")},"saved playlist opens actual content route");check(history()->viewProp()->value(QStringLiteral("parentId")).value<MLItemId>()==id,"saved playlist route carries actual native ID");
    until([]{return activity()->playlists().size()==1;},"actual playlist route records genuine Home visit");return id;
}
void allCards(){
    until([]{return !activity()->loading()&&activity()->folders().size()==1&&activity()->playlists().size()==1&&activity()->artists().size()==1&&named(QStringLiteral("homevideo0"))&&named(QStringLiteral("homeaudio0"));},"all five Home sections contain actual native data",20000);
    check(activity()->error().isEmpty()&&activity()->mediaCount()==3,"Home retains only three owned media without error");const auto artist=activity()->artists().first().toMap();check(artist.value("title").toString()==QStringLiteral("Travel Artist Ää")&&artist.value("count").toInt()==2,"native frequent artist resolves tagged Unicode metadata and two tracks");
    auto *a=QAccessible::queryAccessibleInterface(named(QStringLiteral("homeaudio0")));check(a&&a->text(QAccessible::Name)==QStringLiteral("First owned track — Travel Artist Ää"),"real audio Home card exposes valid Unicode title artist accessible name");
    check(named(QStringLiteral("homevideo0"))->property("progress").toDouble()>.15,"Continue Watching reflects actual saved native progress");report.insert("artistId",artist.value("id").value<MLItemId>().toString());report.insert("playlistId",activity()->playlists().first().toMap().value("id").value<MLItemId>().toString());
}
void exerciseSections(){
    for(const auto *c:{"continueWatching","listenAgain","frequentFolders","frequentPlaylists","frequentArtists"}){
        const auto category=QString::fromLatin1(c);activate(named(QStringLiteral("homeSection")+category));until([]{return named(QStringLiteral("homeCollectionItem0"));},"actual See All opens nonempty native collection");recordRoute(category);check(history()->viewPath()==QStringList{QStringLiteral("home"),category},"See All enters exactly the designed section route");
        const auto kind=category==QStringLiteral("continueWatching")?QStringLiteral("video"):category==QStringLiteral("listenAgain")?QStringLiteral("audio"):category==QStringLiteral("frequentFolders")?QStringLiteral("folder"):category==QStringLiteral("frequentPlaylists")?QStringLiteral("playlist"):QStringLiteral("artist");check(named(QStringLiteral("homeCollectionItem0"))->property("kind").toString()==kind,"native collection retains correct media or parent kind");activate(named(QStringLiteral("libraryBack")));homeReady();
    }
}
void run(){
    try{
        ownedWindow->resize(1280,900);ownedWindow->requestActivate();QAccessible::setActive(true);settle(200);check(library()&&activity()&&history(),"actual Home navigation services available");homeReady();const auto mode=qEnvironmentVariable("VLC_OWNED_UI_MODE");
        if(mode==QStringLiteral("populate")){
            check(roots().isEmpty()&&activity()->mediaCount()==0,"fresh owned content library has no media or roots");picker(false);until([]{return library()->idle()&&!activity()->loading()&&activity()->mediaCount()==3&&activity()->folders().size()==1;},"native index contains exactly three generated tagged media",30000);ownedRoot();
            contentFolder();playContent(QStringLiteral("First owned track"),false);seekQuarter();leavePlayer(false);contentFolder();playContent(QStringLiteral("owned-clip.mp4"),true);seekQuarter();leavePlayer(true);contentFolder();playContent(QStringLiteral("First owned track"),false);leavePlayer(false);
            const auto playlist=createPlaylist();until([&]{return nativeModel("MLPlaylistModel",playlist,1);},"created playlist uses real native one-track content model");toHome();
        }else{ownedRoot();until([]{return library()->idle()&&activity()->mediaCount()==3;},"restart retains all actual indexed content");}
        allCards();capture(QStringLiteral("home-all-top.png"));auto *last=named(QStringLiteral("homeartist0"));check(last!=nullptr,"actual Home artist card exists");last->forceActiveFocus(Qt::TabFocusReason);settle(150);capture(QStringLiteral("home-all-bottom.png"));auto *player=context->getIntf()->p_mainPlayerController;
        if(mode==QStringLiteral("populate")){
            auto *input=player->getInput();check(input!=nullptr,"playing primary exists before Home navigation");input_item_Hold(input);std::unique_ptr<input_item_t,decltype(&input_item_Release)> hold(input,&input_item_Release);const auto url=player->getUrl();const auto rate=player->getRate();const auto before=primary();exerciseSections();
            activate(named(QStringLiteral("libraryView")));settle(150);check(!context->property("gridView").toBool()&&named(QStringLiteral("homeaudio0"))->height()==44,"actual view button switches native Home to list");capture(QStringLiteral("home-list-playing.png"));activate(named(QStringLiteral("libraryView")));settle(150);check(context->property("gridView").toBool(),"actual view button restores Home grid");
            activate(named(QStringLiteral("homeSectionfrequentArtists")));until([]{return named(QStringLiteral("homeCollectionItem0"));},"actual artist collection ready for search");enterText(named(QStringLiteral("librarySearch")),QStringLiteral("missing-owned-artist"));until([]{return !named(QStringLiteral("homeCollectionItem0"));},"real library search filters native artist collection");activate(named(QStringLiteral("librarySearchClear")));until([]{return named(QStringLiteral("homeCollectionItem0"));},"real clear search restores native artist");activate(named(QStringLiteral("libraryBack")));homeReady();
            check(player->getInput()==input&&player->getUrl()==url&&player->getPlayingState()==PlayerController::PLAYING_STATE_PLAYING&&qFuzzyCompare(player->getRate(),rate)&&primary().value("time").toDouble()>before.value("time").toDouble()+200000,"five sections search and view changes preserve playing input rate and advancing clock");
        }else exerciseSections();
        activate(named(QStringLiteral("homevideo0")));until([&]{auto s=primary();return player->getUrl().fileName()==QStringLiteral("owned-clip.mp4")&&s.value("videoTracks").toInt()>0&&player->getPlayingState()==PlayerController::PLAYING_STATE_PLAYING&&s.value("time").toDouble()>10000000;},"actual Continue Watching overrides global Never and resumes native video",15000);check(primary().value("time").toDouble()<25000000,"native Home resume stays near saved quarter position");
        auto *native=context->getIntf()->p_player;vlc_player_Lock(native);vlc_player_Pause(native);vlc_player_Unlock(native);until([&]{return player->getPlayingState()==PlayerController::PLAYING_STATE_PAUSED;},"resumed video pauses before parent route checks");int stable=0;auto paused=primary();QDeadlineTimer deadline(4000);while(stable<4&&!deadline.hasExpired()){settle(100);auto next=primary();stable=next==paused?stable+1:0;paused=next;}check(stable==4,"resumed native and UI paused clocks settle");
        auto *input=player->getInput();input_item_Hold(input);std::unique_ptr<input_item_t,decltype(&input_item_Release)> hold(input,&input_item_Release);leavePlayer(true);allCards();
        const auto playlist=activity()->playlists().first().toMap().value("id").value<MLItemId>();activate(named(QStringLiteral("homeplaylist0")));until([&]{return history()->viewPath()==QStringList{QStringLiteral("playlists"),QStringLiteral("list")}&&nativeModel("MLPlaylistModel",playlist,1);},"real Home playlist card opens native content model");recordRoute(QStringLiteral("Home playlist"));toHome();
        const auto artist=activity()->artists().first().toMap().value("id").value<MLItemId>();activate(named(QStringLiteral("homeartist0")));until([&]{return history()->viewPath()==QStringList{QStringLiteral("music"),QStringLiteral("artists"),QStringLiteral("albums")}&&nativeModel("MLAlbumModel",artist,1);},"real Home artist card opens native album model",15000);recordRoute(QStringLiteral("Home artist"));capture(QStringLiteral("native-artist-album.png"));toHome();check(player->getInput()==input&&primary()==paused,"playlist and artist routes preserve exact resumed paused input and both clocks");ownedRoot();finish(true);
    }catch(const std::exception &e){report.insert("failedPrimary",primary());if(history())recordRoute(QStringLiteral("failure"));finish(false,QString::fromUtf8(e.what()));}
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
