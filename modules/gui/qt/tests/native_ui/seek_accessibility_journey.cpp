// SPDX-License-Identifier: GPL-2.0-or-later
// Opt-in actual Qt accessibility-to-player seek regression; never ordinary VLC.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "qt.hpp"
#include "maininterface/mainctx.hpp"
#include "dialogs/dialogs_provider.hpp"
#include "player/player_controller.hpp"
#include <vlc_input_item.h>
#include <vlc_player.h>
#include <QAccessible>
#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSet>
#include <QTest>
#include <QTimer>
#include <QUrl>
#include <atomic>
#include <cmath>
#include <functional>
#include <stdexcept>

namespace {
MainCtx *context=nullptr;
QQuickWindow *window=nullptr;
vlc_player_t *native=nullptr;
vlc_player_timer_id *timer=nullptr;
QJsonObject report;
QJsonArray checks,requests;
std::atomic_uint seekCount{0};
void save() {
    report.insert("checks",checks);report.insert("requests",requests);
    report.insert("privateFixture",true);report.insert("fullBetaAccepted",false);
    QFile file(qEnvironmentVariable("VLC_OWNED_UI_REPORT"));
    if(file.open(QIODevice::WriteOnly))file.write(QJsonDocument(report).toJson());
}
void check(bool good,const char *name) {
    checks.append(QJsonObject{{"name",name},{"passed",good}});save();
    if(!good)throw std::runtime_error(name);
}
void until(const std::function<bool()> &ready,const char *name,int limit=6000) {
    QElapsedTimer elapsed;elapsed.start();
    while(!ready()&&elapsed.elapsed()<limit)QTest::qWait(20);
    check(ready(),name);
}
QList<QObject *> objects() {
    QList<QObject *> pending,result;QSet<QObject *> seen;
    for(auto *w:QGuiApplication::allWindows())
        if(auto *q=qobject_cast<QQuickWindow *>(w))pending.append(q->contentItem());
    while(!pending.isEmpty()) {
        auto *o=pending.takeLast();if(!o||seen.contains(o))continue;
        seen.insert(o);result.append(o);pending.append(o->children());
        if(auto *q=qobject_cast<QQuickItem *>(o))
            for(auto *child:q->childItems())pending.append(child);
    }
    return result;
}
PlayerController *player(){return context->getIntf()->p_mainPlayerController;}
vlc_player_state state(){vlc_player_locker lock{native};return vlc_player_GetState(native);}
vlc_tick_t nativeTime(){vlc_player_locker lock{native};return vlc_player_GetTime(native);}
unsigned pictures(){vlc_player_locker lock{native};const auto *s=vlc_player_GetStatistics(native);return s?s->i_displayed_pictures:0;}
QQuickItem *slider() {
    QQuickItem *result=nullptr;unsigned count=0;
    for(auto *o:objects())if(QByteArray(o->metaObject()->className()).startsWith("SliderBar_QMLTYPE_"))
        if(auto *q=qobject_cast<QQuickItem *>(o))if(q->isVisible()&&q->window()==window){result=q;++count;}
    check(count==1,"one actual production timeline is visible");return result;
}
const vlc_player_timer_cbs callbacks={
    [](const vlc_player_timer_point *,void *){},
    [](vlc_tick_t,void *){},
    [](const vlc_player_timer_point *point,void *){if(point)++seekCount;}
};
void seek(const char *label,double fraction,const std::function<void(QAccessibleInterface *,QQuickItem *)> &action) {
    auto *item=slider();auto *iface=QAccessible::queryAccessibleInterface(item);
    check(iface&&iface->valueInterface(),"actual accessible timeline value interface exists");
    const auto input=player()->getInput();const auto before=seekCount.load();const auto oldPictures=pictures();
    action(iface,item);
    until([&]{return seekCount.load()>before;},"accessibility or pointer action reaches native seek",1500);
    until([&]{return pictures()>oldPictures&&std::abs(player()->getPosition()-fraction)<.008;},"native picture and Qt position settle after seek");
    QTest::qWait(200);
    requests.append(QJsonObject{{"label",label},{"requested",fraction},{"nativeSeekCount",int(seekCount.load()-before)},
        {"slider",item->property("value").toDouble()},{"qtPosition",player()->getPosition()},
        {"nativeTime",double(nativeTime())},{"displayedPictures",int(pictures())}});save();
    check(seekCount.load()==before+1,"one value or pointer action issues exactly one native seek");
    check(state()==VLC_PLAYER_STATE_PAUSED&&player()->getInput()==input,"seek preserves the paused media");
    check(std::abs(item->property("value").toDouble()-player()->getPosition())<.003,"display value reconciles with the native landing");
}
void finish() {
    if(timer){vlc_player_RemoveTimer(native,timer);timer=nullptr;}
    if(context)DialogsProvider::getInstance()->quit();
}
void run() {try {
    native=context->getIntf()->p_player;window->resize(1100,760);
    timer=vlc_player_AddTimer(native,VLC_TICK_INVALID,&callbacks,nullptr);check(timer,"native seek observer attaches");
    const auto uri=QUrl::fromLocalFile(qEnvironmentVariable("VLC_OWNED_SEEK_MEDIA")).toEncoded();
    auto *media=input_item_New(uri.constData(),"Owned accessibility seek video");check(media,"synthetic media exists");
    vlc_player_Lock(native);const int selected=vlc_player_SetCurrentMedia(native,media);const int started=selected==VLC_SUCCESS?vlc_player_Start(native):VLC_EGENERIC;vlc_player_Unlock(native);input_item_Release(media);
    check(selected==VLC_SUCCESS&&started==VLC_SUCCESS,"native player opens synthetic media");
    context->setPlayerView(true);
    until([]{return state()==VLC_PLAYER_STATE_PLAYING&&pictures()>0&&player()->isSeekable()&&nativeTime()>VLC_TICK_FROM_MS(300);},"normal video picture and published player statistics are ready",15000);
    vlc_player_Lock(native);vlc_player_Pause(native);vlc_player_Unlock(native);
    until([]{return state()==VLC_PLAYER_STATE_PAUSED;},"native playback pauses before accessibility checks");
    QTest::qWait(350);
    seek("accessible-forward",.42,[](auto *iface,auto *){iface->valueInterface()->setCurrentValue(.42);});
    seek("accessible-backward",.09,[](auto *iface,auto *){iface->valueInterface()->setCurrentValue(.09);});
    auto *item=slider();auto *iface=QAccessible::queryAccessibleInterface(item);
    check(iface->valueInterface()->minimumValue().toDouble()==0.&&iface->valueInterface()->maximumValue().toDouble()==1.,"accessible range reports normalized bounds");
    const auto before=seekCount.load();const auto held=item->property("value");
    iface->valueInterface()->setCurrentValue(held);QTest::qWait(250);
    check(seekCount.load()==before&&item->property("value")==held,"identical accessible value does not seek");
    seek("pointer-after-accessibility",.65,[](auto *,auto *item){const auto p=item->mapToScene(QPointF(item->width()*.65,item->height()/2));QTest::mouseClick(window,Qt::LeftButton,Qt::ShiftModifier,p.toPoint());});
    const auto playingBefore=seekCount.load();const auto timeBefore=nativeTime();
    vlc_player_Lock(native);vlc_player_Resume(native);vlc_player_Unlock(native);
    until([&]{return state()==VLC_PLAYER_STATE_PLAYING&&nativeTime()>timeBefore+VLC_TICK_FROM_MS(500);},"normal playback advances after accessibility and pointer seeks");
    QTest::qWait(500);
    check(seekCount.load()==playingBefore,"ordinary position updates never feed back as seeks");
    vlc_player_Lock(native);vlc_player_Pause(native);vlc_player_Unlock(native);
    until([]{return state()==VLC_PLAYER_STATE_PAUSED;},"playback pauses normally");QTest::qWait(250);
    seek("accessible-after-playback",.26,[](auto *iface,auto *){iface->valueInterface()->setCurrentValue(.26);});
    report.insert("passed",true);report.insert("status","checks_complete");save();finish();
}catch(const std::exception &error){report.insert("passed",false);report.insert("error",error.what());save();finish();}}
void findWindow(int remaining) {
    for(auto *o:objects())if(o->objectName()==QStringLiteral("juxtopposedDialogsHost"))if(auto *q=qobject_cast<QQuickItem *>(o)){
        auto *engine=qmlEngine(o);context=engine?engine->singletonInstance<MainCtx *>("VLC.MainInterface","MainCtx"):nullptr;window=q->window();
    }
    if(context&&window){const auto gate=qEnvironmentVariable("VLC_OWNED_UI_START_GATE");if(gate.isEmpty()||QFile::exists(gate)){QTimer::singleShot(150,context,run);return;}report.insert("status","awaiting-owned-window-inspection");save();}
    if(remaining>0)QTimer::singleShot(250,qApp,[remaining]{findWindow(remaining-1);});
    else{report.insert("passed",false);report.insert("error","owned test window or inspection gate unavailable");save();finish();}
}
void start(){if(!qEnvironmentVariableIsEmpty("VLC_OWNED_UI_REPORT"))QTimer::singleShot(800,qApp,[]{findWindow(1200);});}
}
Q_COREAPP_STARTUP_FUNCTION(start)
