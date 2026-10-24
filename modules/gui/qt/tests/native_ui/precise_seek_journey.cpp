// Opt-in real Qt seek policy/MP4 consumer regression. Never in ordinary VLC.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "qt.hpp"
#include "maininterface/mainctx.hpp"
#include "maininterface/playback_preferences_model.hpp"
#include "dialogs/dialogs_provider.hpp"
#include "player/player_controller.hpp"
#include <vlc_configuration.h>
#include <vlc_input_item.h>
#include <vlc_player.h>
#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
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
#include <QSignalSpy>
#include <QTest>
#include <QTimer>
#include <QUrl>
#include <cmath>
#include <functional>
#include <stdexcept>
#include <windows.h>

namespace {
MainCtx *context=nullptr;QQuickWindow *window=nullptr;
vlc_player_t *native=nullptr;QJsonObject report;QJsonArray checks,seeks,timerEvents;
void save(){report.insert("checks",checks);report.insert("seeks",seeks);report.insert("timerEvents",timerEvents);report.insert("privateFixture",true);report.insert("fullBetaAccepted",false);QFile f(qEnvironmentVariable("VLC_OWNED_UI_REPORT"));if(f.open(QIODevice::WriteOnly))f.write(QJsonDocument(report).toJson());}
void check(bool good,const char *name){checks.append(QJsonObject{{"name",name},{"passed",good}});save();if(!good)throw std::runtime_error(name);}
void wait(const std::function<bool()> &ready,const char *name,int limit=10000){QElapsedTimer t;t.start();while(!ready()&&t.elapsed()<limit)QTest::qWait(25);check(ready(),name);}
QByteArray bytes(const QString &path){QFile f(path);return f.open(QIODevice::ReadOnly)?f.readAll():QByteArray{};}
QList<QObject *> objects(){QList<QObject *> todo,result;QSet<QObject *> seen;for(auto *w:QGuiApplication::allWindows())if(auto *q=qobject_cast<QQuickWindow *>(w))todo.append(q->contentItem());while(!todo.isEmpty()){auto *o=todo.takeLast();if(!o||seen.contains(o))continue;seen.insert(o);result.append(o);todo.append(o->children());if(auto *q=qobject_cast<QQuickItem *>(o))for(auto *c:q->childItems())todo.append(c);}return result;}
QQuickItem *item(const char *name){for(auto *o:objects())if(o->objectName()==QLatin1String(name))if(auto *q=qobject_cast<QQuickItem *>(o))return q;return nullptr;}
PlayerController *controller(){return context->getIntf()->p_mainPlayerController;}
vlc_tick_t time(){vlc_player_Lock(native);const auto v=vlc_player_GetTime(native);vlc_player_Unlock(native);return v;}
vlc_player_state state(){vlc_player_Lock(native);const auto v=vlc_player_GetState(native);vlc_player_Unlock(native);return v;}
int displayed(){vlc_player_Lock(native);const auto *s=vlc_player_GetStatistics(native);const int v=s?s->i_displayed_pictures:0;vlc_player_Unlock(native);return v;}
int decoded(){vlc_player_Lock(native);const auto *s=vlc_player_GetStatistics(native);const int v=s?s->i_decoded_video:0;vlc_player_Unlock(native);return v;}
// The native timer lock serializes these callbacks. Copy data only: the API
// forbids calling any player function here. Read both clocks on the Qt owner.
unsigned timerSequence=0,seekEpoch=0;bool timerSeeking=false;
void traceTimer(const char *kind,const vlc_player_timer_point *point,vlc_tick_t date){
    QJsonObject row{{"kind",kind},{"sequence",int(++timerSequence)},{"seekEpoch",int(seekEpoch)},{"seeking",timerSeeking},{"callbackTick",double(vlc_tick_now())},{"systemDate",double(date)}};
    if(point){row.insert("ts",double(point->ts));row.insert("position",point->position);row.insert("length",double(point->length));row.insert("rate",point->rate);row.insert("live",point->live);}
    QMetaObject::invokeMethod(context,[row]()mutable{
        row.insert("deliveryTick",double(vlc_tick_now()));row.insert("label",report.value("activeSeek"));
        row.insert("qtTime",double(controller()->getTime().toVLCTick()));row.insert("qtDisplayTime",double(controller()->getDisplayTime().toVLCTick()));row.insert("qtPosition",controller()->getPosition());row.insert("coreTime",double(time()));row.insert("media",QString::number(reinterpret_cast<quintptr>(controller()->getInput()),16));timerEvents.append(row);
    },Qt::QueuedConnection);
}
const vlc_player_timer_cbs traceCallbacks={
    [](const vlc_player_timer_point *point,void *){traceTimer("update",point,point->system_date);},
    [](vlc_tick_t date,void *){traceTimer("paused",nullptr,date);},
    [](const vlc_player_timer_point *point,void *){if(point){++seekEpoch;timerSeeking=true;}else timerSeeking=false;traceTimer(point?"seek-begin":"seek-end",point,point?point->system_date:VLC_TICK_INVALID);}
};
void click(QQuickItem *q){
    check(q&&q->isVisible()&&q->isEnabled(),"actual seek settings control is enabled");q->forceActiveFocus(Qt::TabFocusReason);QTest::qWait(150);
    if(q->objectName().startsWith(QStringLiteral("playback"))){
        auto *viewport=item("globalSettingsPageScroll");check(viewport,"actual clipped settings viewport exists");const auto before=q->mapRectToItem(viewport,q->boundingRect());
        const auto rect=q->mapRectToItem(viewport,q->boundingRect());auto rows=report.value("controlGeometry").toArray();rows.append(QJsonObject{{"control",q->objectName()},{"beforeTop",before.top()},{"beforeBottom",before.bottom()},{"top",rect.top()},{"bottom",rect.bottom()},{"viewportHeight",viewport->height()}});report.insert("controlGeometry",rows);save();check(rect.top()>=0&&rect.bottom()<=viewport->height(),"entire seek toggle is inside the clipped settings viewport before pointer input");
        const auto capture=report.value("pointerCapture").toString();if(!capture.isEmpty()){check(window->grabWindow().save(QDir(qEnvironmentVariable("VLC_OWNED_UI_BASE")).filePath(capture+QStringLiteral(".png"))),"actual bounded pointer state capture saved");report.remove("pointerCapture");}
    }
    const auto point=q->mapToScene(QPointF(q->width()/2,q->height()/2));check(window->contentItem()->contains(point),"actual seek control center is inside its native window");QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,point.toPoint());QTest::qWait(100);
}
void openSettings(){auto *p=DialogsProvider::getInstance();check(QMetaObject::invokeMethod(p,"showGlobalSettingsDialog"),"actual Global settings route opens");wait([]{return item("globalCategoryplayback");},"actual Playback category exists");click(item("globalCategoryplayback"));wait([]{return item("playbackpreciseSeeking");},"actual precise seeking toggle mounts");}
void closeSettings(){QTest::keyClick(window,Qt::Key_Escape);wait([]{return !DialogsProvider::getInstance()->qmlDialogVisible();},"actual Playback settings close normally");}
void toggle(bool precise){auto *p=context->getPlaybackPreferences();if(p->values().value("preciseSeeking").toBool()!=precise){openSettings();click(item("playbackpreciseSeeking"));check(p->values().value("preciseSeeking").toBool()==precise&&p->lastError().isEmpty(),"actual precise seek toggle commits the requested policy");closeSettings();}check(bool(config_GetInt("input-fast-seek"))==!precise,"native saved flag matches the accepted precision choice");}
void seek(const char *label,bool precise,double expected,const std::function<void()> &action){
    toggle(precise);const auto input=controller()->getInput();const auto old=displayed();
    report.insert("activeSeek",label);
    msg_Dbg(context->getIntf(),"owned-seek-begin %s %s",label,precise?"precise":"fast");action();
    // The file logger can buffer a completed paused seek. The parent verifies
    // its exact branch/request between these markers after normal process exit.
    wait([&]{return std::abs(double(time())/CLOCK_FREQ-expected)<.35;},"native paused clock reaches the expected precise or keyframe position");
    wait([&]{return displayed()>old;},"actual Windows video output displays a new seeked picture");
    check(controller()->getInput()==input&&state()==VLC_PLAYER_STATE_PAUSED,"seek preserves the same native input and paused state");
    QTest::qWait(250);
    check(std::abs(double(time())/CLOCK_FREQ-expected)<.35,"settled native paused clock still matches the displayed seek landing");
    check(std::abs(double(controller()->getTime().toVLCTick())/CLOCK_FREQ-expected)<.35&&std::abs(double(controller()->getDisplayTime().toVLCTick())/CLOCK_FREQ-expected)<.35&&std::abs(controller()->getPosition()-expected/60.)<.006,"settled Qt elapsed display and slider match the native seek landing");
    seeks.append(QJsonObject{{"label",label},{"precise",precise},{"expectedSeconds",expected},{"actualSeconds",double(time())/CLOCK_FREQ},{"qtSeconds",double(controller()->getTime().toVLCTick())/CLOCK_FREQ},{"qtDisplaySeconds",double(controller()->getDisplayTime().toVLCTick())/CLOCK_FREQ},{"qtPosition",controller()->getPosition()},{"displayedPictures",displayed()},{"decodedPictures",decoded()}});save();
    msg_Dbg(context->getIntf(),"owned-seek-end %s",label);
}
void pointerSeek(const char *label,double requested,double expected){
    QQuickItem *slider=nullptr;int count=0;for(auto *o:objects())if(QByteArray(o->metaObject()->className()).startsWith("SliderBar_QMLTYPE_"))if(auto *q=qobject_cast<QQuickItem *>(o))if(q->isVisible()&&q->window()==window){slider=q;++count;}
    check(slider&&count==1,"one actual production timeline is visible for held-pointer regression");
    slider->forceActiveFocus(Qt::TabFocusReason);const auto point=slider->mapToScene(QPointF(slider->width()*requested/60.,slider->height()/2)).toPoint();const auto pictures=displayed();const auto input=controller()->getInput();
    report.insert("activeSeek",label);msg_Dbg(context->getIntf(),"owned-pointer-seek-begin %s",label);QTest::mousePress(window,Qt::LeftButton,Qt::NoModifier,point);
    try{
        wait([&]{return displayed()>pictures&&std::abs(double(time())/CLOCK_FREQ-expected)<.35&&std::abs(controller()->getPosition()-expected/60.)<.006;},"actual timeline seek completes while the pointer remains held");QTest::qWait(150);
    }catch(...){QTest::mouseRelease(window,Qt::LeftButton,Qt::NoModifier,point);throw;}
    const double held=slider->property("value").toDouble();QTest::mouseRelease(window,Qt::LeftButton,Qt::NoModifier,point);QTest::qWait(200);
    auto rows=report.value("pointerSeeks").toArray();rows.append(QJsonObject{{"label",label},{"requested",requested},{"expected",expected},{"heldSlider",held},{"releasedSlider",slider->property("value").toDouble()},{"qtPosition",controller()->getPosition()},{"coreSeconds",double(time())/CLOCK_FREQ},{"qtSeconds",double(controller()->getTime().toVLCTick())/CLOCK_FREQ}});report.insert("pointerSeeks",rows);save();
    check(std::abs(slider->property("value").toDouble()-expected/60.)<.006&&std::abs(double(time())/CLOCK_FREQ-expected)<.35&&std::abs(double(controller()->getTime().toVLCTick())/CLOCK_FREQ-expected)<.35,"released production timeline reconciles completed frame, elapsed time and slider without Resume");
    check(state()==VLC_PLAYER_STATE_PAUSED&&controller()->getInput()==input,"held-pointer seek preserves paused native input");msg_Dbg(context->getIntf(),"owned-pointer-seek-end %s",label);
}
void run(){try{
    window->resize(1280,900);native=context->getIntf()->p_player;
    auto *trace=vlc_player_AddTimer(native,VLC_TICK_INVALID,&traceCallbacks,nullptr);check(trace,"private copied-data timer observer attaches");QObject::connect(context,&QObject::destroyed,[player=native,trace]{vlc_player_RemoveTimer(player,trace);});
    const auto base=QDir(qEnvironmentVariable("VLC_OWNED_UI_BASE")).absolutePath()+QLatin1Char('/');const auto config=qEnvironmentVariable("VLC_OWNED_SETTINGS_CONFIG");
    check(QFileInfo(config).absoluteFilePath().startsWith(base)&&QFileInfo(context->getIntf()->mainSettings->fileName()).absoluteFilePath().startsWith(base),"seek fixture settings are fenced to owned paths");
    auto *p=context->getPlaybackPreferences();check(p&&p->capabilities().value("preciseSeeking").toBool(),"actual MainCtx enables its paired native precise seek consumer");
    const auto mode=qEnvironmentVariable("VLC_OWNED_SEEK_MODE");const bool cold=mode.startsWith(QStringLiteral("cold"));const bool coldPrecise=mode==QStringLiteral("cold-precise");
    check(p->values().value("preciseSeeking").toBool()==(!cold||coldPrecise),"native startup restores the saved precise seek default");
    const auto uri=QUrl::fromLocalFile(qEnvironmentVariable("VLC_OWNED_SEEK_MEDIA")).toEncoded();auto *media=input_item_New(uri.constData(),"Owned seek regression");check(media,"owned generated MP4 opens");
    vlc_player_Lock(native);const auto selected=vlc_player_SetCurrentMedia(native,media);const auto started=vlc_player_Start(native);vlc_player_Unlock(native);input_item_Release(media);check(selected==VLC_SUCCESS&&started==VLC_SUCCESS,"actual primary starts the owned MP4");
    wait([]{return state()==VLC_PLAYER_STATE_PLAYING&&decoded()>10&&displayed()>5&&time()>VLC_TICK_FROM_MS(300);},"actual primary decodes and displays generated video",15000);
    context->setPlayerView(true);vlc_player_Lock(native);vlc_player_Pause(native);vlc_player_Unlock(native);wait([]{return state()==VLC_PLAYER_STATE_PAUSED;},"native primary pauses before seek checks");QTest::qWait(400);
    if(cold){openSettings();check(item("playbackpreciseSeeking")->property("selected").toBool()==coldPrecise,"cold actual toggle restores the saved precision choice");closeSettings();seek(coldPrecise?"cold-precise-time":"cold-fast-time",coldPrecise,coldPrecise?24.2:20.,[]{controller()->setTime(VLCTime(VLC_TICK_FROM_MS(24200)));});if(!coldPrecise)toggle(true);}
    else {
        seek("precise-time",true,24.2,[]{controller()->setTime(VLCTime(VLC_TICK_FROM_MS(24200)));});
        seek("fast-time",false,30.,[]{controller()->setTime(VLCTime(VLC_TICK_FROM_MS(34200)));});
        seek("fast-same-keyframe-time",false,30.,[]{controller()->setTime(VLCTime(VLC_TICK_FROM_MS(38200)));});
        seek("precise-position",true,24.2,[]{controller()->setPosition(24.2/60.);});
        seek("fast-position",false,30.,[]{controller()->setPosition(34.2/60.);});
        seek("precise-relative-time",true,34.2,[]{controller()->jumpTime(VLCTime(VLC_TICK_FROM_MS(4200)));});
        seek("fast-relative-time",false,40.,[]{controller()->jumpTime(VLCTime(VLC_TICK_FROM_MS(14200)));});
        seek("precise-timeline",true,24.2,[]{controller()->jumpToPos(24.2/60.);});
        seek("fast-timeline",false,30.,[]{controller()->jumpToPos(34.2/60.);});
        seek("precise-forward",true,33.,[]{controller()->jumpFwd();});
        seek("fast-forward",false,30.,[]{controller()->jumpFwd();});
        seek("precise-backward",true,27.,[]{controller()->jumpBwd();});
        seek("fast-backward",false,20.,[]{controller()->jumpBwd();});
        const auto accepted=bytes(config);const auto input=controller()->getInput();const auto clock=time();
        openSettings();HANDLE lock=CreateFileW(config.toStdWString().c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);check(lock!=INVALID_HANDLE_VALUE,"owned native config replacement is blocked");
        report.insert("pointerCapture","before-blocked-toggle");click(item("playbackpreciseSeeking"));const bool rejected=!p->lastError().isEmpty()&&!p->values().value("preciseSeeking").toBool()&&config_GetInt("input-fast-seek")==1&&bytes(config)==accepted;CloseHandle(lock);
        window->grabWindow().save(QDir(qEnvironmentVariable("VLC_OWNED_UI_BASE")).filePath(QStringLiteral("after-blocked-toggle.png")));
        check(rejected,"failed actual precision save preserves config and accepted policy with error feedback");
        auto *control=item("playbackpreciseSeeking"),*viewport=item("globalSettingsPageScroll"),*error=item("playbackpreciseSeekingValidationError");
        // Popup is a QObject, not an Item: inspect its rendered background.
        QObject *tip=nullptr;for(auto *o:objects())if(o->objectName()==QStringLiteral("playbackpreciseSeekingValidationError"))tip=o;
        error=tip?qvariant_cast<QQuickItem *>(tip->property("background")):nullptr;
        check(control&&viewport&&error&&tip->property("visible").toBool()&&!tip->property("focus").toBool()&&control->hasActiveFocus(),"native rejected Toggle retains focus and readable nonfocusable local error");
        for(auto *q:{control,error}){const auto r=q->mapRectToItem(viewport,q->boundingRect());check(r.left()>=0&&r.top()>=0&&r.right()<=viewport->width()&&r.bottom()<=viewport->height(),"native rejected Toggle and full error both fit actual viewport without fixture reveal");}
        check(state()==VLC_PLAYER_STATE_PAUSED&&controller()->getInput()==input&&std::abs(time()-clock)<VLC_TICK_FROM_MS(50),"failed save preserves paused media and clock");
        seek("fast-after-rejected-save",false,30.,[]{controller()->setTime(VLCTime(VLC_TICK_FROM_MS(34200)));});
        QSignalSpy valueSignals(p,&PlaybackPreferencesModel::valuesChanged),errorSignals(p,&PlaybackPreferencesModel::lastErrorChanged);report.insert("pointerCapture","before-retry-toggle");click(item("playbackpreciseSeeking"));report.insert("retry",QJsonObject{{"valueSignals",valueSignals.count()},{"errorSignals",errorSignals.count()},{"precise",p->values().value("preciseSeeking").toBool()},{"lastError",p->lastError()},{"nativeFast",int(config_GetInt("input-fast-seek"))}});save();check(p->lastError().isEmpty()&&p->values().value("preciseSeeking").toBool(),"same actual toggle retries successfully after storage recovers");closeSettings();
        seek("precise-after-retry",true,24.2,[]{controller()->setTime(VLCTime(VLC_TICK_FROM_MS(24200)));});toggle(false);
        pointerSeek("held-fast-time",34.2,30.);pointerSeek("held-same-keyframe-time",38.2,30.);
    }
    vlc_player_Lock(native);vlc_player_Resume(native);vlc_player_Unlock(native);const auto paused=time();wait([&]{return state()==VLC_PLAYER_STATE_PLAYING&&time()>paused+VLC_TICK_FROM_MS(300);},"native decoded playback resumes after seeks and settings edits");
    wait([&]{return controller()->getTime().toVLCTick()>paused+VLC_TICK_FROM_MS(200);},"Qt elapsed clock advances with resumed playback");
    if(!cold){const auto oldPictures=displayed();controller()->jumpToPos(34.2/60.);
    wait([&]{const auto now=double(time())/CLOCK_FREQ;return displayed()>oldPictures&&now>=30.&&now<32.;},"playing Fast seek displays the keyframe and continues its clock");
    vlc_player_Lock(native);vlc_player_Pause(native);vlc_player_Unlock(native);wait([]{return state()==VLC_PLAYER_STATE_PAUSED;},"playing seek pauses normally for clock reconciliation");QTest::qWait(300);
    check(std::abs(controller()->getTime().toVLCTick()-time())<VLC_TICK_FROM_MS(100),"playing seek and subsequent pause retain aligned native and Qt clocks");
    vlc_player_Lock(native);vlc_player_Resume(native);vlc_player_Unlock(native);}
    report.insert("passed",true);report.insert("status","checks_complete");save();DialogsProvider::getInstance()->quit();
}catch(const std::exception &e){if(window)window->grabWindow().save(QDir(qEnvironmentVariable("VLC_OWNED_UI_BASE")).filePath(QStringLiteral("failure.png")));report.insert("passed",false);report.insert("error",e.what());save();if(context)DialogsProvider::getInstance()->quit();}}
void findWindow(int n){for(auto *o:objects())if(o->objectName()==QStringLiteral("juxtopposedDialogsHost"))if(auto *q=qobject_cast<QQuickItem *>(o)){auto *engine=qmlEngine(o);context=engine?engine->singletonInstance<MainCtx *>("VLC.MainInterface","MainCtx"):nullptr;window=q->window();}if(context&&window){const auto gate=qEnvironmentVariable("VLC_OWNED_UI_START_GATE");if(gate.isEmpty()||QFile::exists(gate)){QTimer::singleShot(150,context,run);return;}report.insert("status","awaiting-owned-window-inspection");save();}if(n>0)QTimer::singleShot(250,qApp,[n]{findWindow(n-1);});else{report.insert("passed",false);report.insert("error","owned window/gate unavailable");save();}}
void start(){if(!qEnvironmentVariableIsEmpty("VLC_OWNED_UI_REPORT"))QTimer::singleShot(800,qApp,[]{findWindow(1200);});}
}
Q_COREAPP_STARTUP_FUNCTION(start)
