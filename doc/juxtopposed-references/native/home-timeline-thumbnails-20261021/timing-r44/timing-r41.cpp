// SPDX-License-Identifier: GPL-2.0-or-later
// Opt-in actual Qt timeline preview journey. Never linked into ordinary VLC.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "qt.hpp"
#include "maininterface/mainctx.hpp"
#include "maininterface/playback_preferences_model.hpp"
#include "maininterface/general_preferences_model.hpp"
#include "dialogs/dialogs_provider.hpp"
#include "player/player_controller.hpp"
#include "player/timeline_thumbnails.hpp"
#include <vlc_configuration.h>
#include <vlc_player.h>
#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QMouseEvent>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQuickImageProvider>
#include <QSet>
#include <QSettings>
#include <QTest>
#include <QTimer>
#include <QUrl>
#include <cmath>
#include <functional>
#include <stdexcept>
#include <windows.h>

namespace {
MainCtx *context = nullptr;
QQuickWindow *window = nullptr;
QJsonObject report;
QJsonArray checks, previews;
struct FileLock {
    HANDLE value = INVALID_HANDLE_VALUE;
    ~FileLock() {close();}
    void close() {if(value!=INVALID_HANDLE_VALUE){CloseHandle(value);value=INVALID_HANDLE_VALUE;}}
};
void save() {
    report.insert("checks",checks); report.insert("previews",previews);
    report.insert("privateFixture",true); report.insert("fullBetaAccepted",false);
    QFile file(qEnvironmentVariable("VLC_OWNED_UI_REPORT"));
    if(file.open(QIODevice::WriteOnly))file.write(QJsonDocument(report).toJson());
}
void check(bool value,const char *name) {
    checks.append(QJsonObject{{"name",name},{"passed",value}});save();
    if(!value)throw std::runtime_error(name);
}
void wait(const std::function<bool()> &ready,const char *name,int limit=7000) {
    QElapsedTimer timer;timer.start();
    while(!ready()&&timer.elapsed()<limit)QTest::qWait(25);
    check(ready(),name);
}
QByteArray bytes(const QString &path) {QFile file(path);return file.open(QIODevice::ReadOnly)?file.readAll():QByteArray{};}
QList<QObject *> objects() {
    QList<QObject *> pending,result;QSet<QObject *> seen;
    for(auto *w:QGuiApplication::allWindows())if(auto *q=qobject_cast<QQuickWindow *>(w))pending.append(q->contentItem());
    while(!pending.isEmpty()) {
        auto *o=pending.takeLast();if(!o||seen.contains(o))continue;
        seen.insert(o);result.append(o);pending.append(o->children());
        if(auto *q=qobject_cast<QQuickItem *>(o))for(auto *child:q->childItems())pending.append(child);
    }
    return result;
}
QQuickItem *item(const char *name) {
    for(auto *o:objects())if(o->objectName()==QLatin1String(name))if(auto *q=qobject_cast<QQuickItem *>(o))return q;
    return nullptr;
}
QObject *popup() {
    for(auto *o:objects())if(o->objectName()==QStringLiteral("timelineThumbnail")&&o->property("visible").toBool())return o;
    return nullptr;
}
PlayerController *controller() {return context->getIntf()->p_mainPlayerController;}
vlc_tick_t time() {auto *p=context->getIntf()->p_player;vlc_player_Lock(p);const auto v=vlc_player_GetTime(p);vlc_player_Unlock(p);return v;}
vlc_player_state state() {auto *p=context->getIntf()->p_player;vlc_player_Lock(p);const auto v=vlc_player_GetState(p);vlc_player_Unlock(p);return v;}
QQuickItem *slider() {
    QQuickItem *found=nullptr;int count=0;
    for(auto *o:objects())if(QByteArray(o->metaObject()->className()).startsWith("SliderBar_QMLTYPE_"))
        if(auto *q=qobject_cast<QQuickItem *>(o))if(q->isVisible()&&q->window()==window){found=q;++count;}
    check(count==1,"one actual production timeline is visible");return found;
}
void click(QQuickItem *q) {
    check(q&&q->isVisible()&&q->isEnabled(),"actual thumbnail settings control is enabled");
    q->forceActiveFocus(Qt::TabFocusReason);QTest::qWait(150);
    if(q->objectName()==QStringLiteral("playbackshowThumbnails")) {
        auto *viewport=item("globalSettingsPageScroll");check(viewport,"actual clipped settings viewport exists");
        const auto r=q->mapRectToItem(viewport,q->boundingRect());
        check(r.top()>=0&&r.bottom()<=viewport->height(),"thumbnail toggle is fully visible before pointer input");
    }
    const auto point=q->mapToScene(QPointF(q->width()/2,q->height()/2));
    check(window->contentItem()->contains(point),"thumbnail control center is inside the window");
    QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,point.toPoint());QTest::qWait(100);
}
void openSettings() {
    check(QMetaObject::invokeMethod(DialogsProvider::getInstance(),"showGlobalSettingsDialog"),"actual Global settings opens");
    wait([]{return item("globalCategoryplayback");},"actual Playback category exists");
    click(item("globalCategoryplayback"));
    wait([]{return item("playbackshowThumbnails");},"actual thumbnail toggle mounts");
}
void closeSettings() {
    QTest::keyClick(window,Qt::Key_Escape);
    wait([]{return !DialogsProvider::getInstance()->qmlDialogVisible();},"settings closes normally");
}
void toggle(bool enabled) {
    auto *preferences=context->getPlaybackPreferences();
    if(preferences->values().value("showThumbnails").toBool()!=enabled) {
        openSettings();click(item("playbackshowThumbnails"));
        check(preferences->lastError().isEmpty()&&preferences->values().value("showThumbnails").toBool()==enabled,"actual toggle commits thumbnail choice");
        closeSettings();
    }
    check(context->getTimelineThumbnails()->enabled()==enabled&&bool(config_GetInt("qt-timeline-thumbnails"))==enabled,"native saved preference and live provider agree");
}
void openMedia(const QString &path,bool video) {
    const auto uri=QUrl::fromLocalFile(path).toEncoded();
    auto *media=input_item_New(uri.constData(),video?"Owned thumbnail video":"Owned silent thumbnail fallback");
    check(media,"owned synthetic input is created");
    auto *p=context->getIntf()->p_player;
    vlc_player_Lock(p);const auto selected=vlc_player_SetCurrentMedia(p,media);const auto started=vlc_player_Start(p);vlc_player_Unlock(p);input_item_Release(media);
    check(selected==VLC_SUCCESS&&started==VLC_SUCCESS,"actual primary starts synthetic media");
    wait([=]{return state()==VLC_PLAYER_STATE_PLAYING&&time()>VLC_TICK_FROM_MS(200)&&controller()->isSeekable();},"primary plays seekable synthetic media",15000);
    context->setPlayerView(true);vlc_player_Lock(p);vlc_player_Pause(p);vlc_player_Unlock(p);
    wait([]{return state()==VLC_PLAYER_STATE_PAUSED;},"primary pauses for preview independence checks");
    QTest::qWait(250);
}
struct PointerTrace final : QObject {
    QElapsedTimer *clock;
    QJsonArray events;
    explicit PointerTrace(QElapsedTimer *value) : clock(value) {}
    bool eventFilter(QObject *, QEvent *event) override {
        if(event->type()==QEvent::MouseMove) {
            const auto *mouse=static_cast<QMouseEvent *>(event);
            events.append(QJsonObject{{"ms",clock->elapsed()},{"x",mouse->position().x()},{"y",mouse->position().y()}});
        }
        return false;
    }
};
QPoint hover(double seconds,QJsonObject *timing=nullptr,QElapsedTimer *clock=nullptr) {
    const auto mark=[&](const char *name){if(timing&&clock)timing->insert(QLatin1String(name),clock->elapsed());};
    mark("activateBeginMs");
    window->requestActivate();
    mark("activateEndMs");
    wait([]{return window->isActive();},"owned VLC is active before timeline pointer input");
    mark("activeWaitAndReportEndMs");
    auto *q=slider();const double length=controller()->getLength().toSeconds();
    mark("sliderLookupAndReportEndMs");
    const auto point=q->mapToScene(QPointF(q->width()*seconds/length,q->height()/2));
    const auto sent=point.toPoint();mark("pointerSendBeginMs");QTest::mouseMove(window,sent);mark("pointerSendEndMs");return sent;
}
void preview(double seconds,const char *name,int channel,const QImage &reference={}) {
    QElapsedTimer hoverElapsed;hoverElapsed.start();
    QJsonObject timing;QJsonArray stateEvents;
    PointerTrace pointerTrace(&hoverElapsed);window->installEventFilter(&pointerTrace);
    QObject traceLifetime;
    auto *service=context->getTimelineThumbnails();
    timing.insert("sourceBeforeInput",service->imageSource());
    timing.insert("secondBeforeInput",service->requestedSeconds());
    QObject::connect(service,&TimelineThumbnails::stateChanged,&traceLifetime,[&]{
        stateEvents.append(QJsonObject{{"ms",hoverElapsed.elapsed()},{"loading",service->loading()},{"second",service->requestedSeconds()},{"source",service->imageSource()}});
    });
    const auto oldTime=time();const auto input=controller()->getInput();const auto pointer=hover(seconds,&timing,&hoverElapsed);
    auto *bar=slider();timing.insert("secondSliderLookupEndMs",hoverElapsed.elapsed());
    wait([=]{
        report.insert("hoverState",QJsonObject{{"requested",seconds},{"sampled",service->requestedSeconds()},{"loading",service->loading()},{"source",service->imageSource()},{"enabled",service->enabled()},{"wanted",bar->property("thumbnailWanted").toBool()},{"second",bar->property("thumbnailSecond").toInt()},{"windowActive",window->isActive()},{"visiblePopup",popup()!=nullptr}});
        return !service->loading()&&!service->imageSource().isEmpty()&&service->requestedSeconds()==std::floor(seconds)&&popup();
    },"hover opens the actual preview card at the requested second");
    const auto readyMs=hoverElapsed.elapsed();
    timing.insert("readyObservedMs",readyMs);
    QTest::qWait(150);timing.insert("deliberateSettleEndMs",hoverElapsed.elapsed());
    auto *card=popup();check(card,"actual preview stays visible after its image settles");
    auto *content=qvariant_cast<QQuickItem *>(card->property("contentItem"));
    auto *image=content?content->findChild<QQuickItem *>(QStringLiteral("timelineThumbnailImage")):nullptr;
    auto *label=content?content->findChild<QQuickItem *>(QStringLiteral("timelineThumbnailTime")):nullptr;
    check(image&&label&&image->property("status").toInt()==1,"actual preview image is Ready and timestamp is rendered");
    const int sampled=int(std::floor(seconds));
    const auto expected=QStringLiteral("%1:%2:%3").arg(sampled/3600,2,10,QChar('0')).arg((sampled/60)%60,2,10,QChar('0')).arg(sampled%60,2,10,QChar('0'));
    check(label->property("text").toString()==expected,"preview timestamp matches the sampled scene in Figma hours-minutes-seconds format");
    const auto scale=card->property("uiScale").toDouble();
    check(std::abs(card->property("width").toDouble()-272*scale)<.5&&std::abs(card->property("height").toDouble()-189*scale)<.5,"card dimensions match Figma at the current scale");
    check(std::abs(image->width()-252*scale)<.5&&std::abs(image->height()-142*scale)<.5,"image dimensions match Figma at the current scale");
    auto *surface=image->window();timing.insert("firstGrabBeginMs",hoverElapsed.elapsed());
    auto capture=surface->grabWindow();timing.insert("firstGrabEndMs",hoverElapsed.elapsed());
    check(!capture.isNull(),"actual native preview surface capture is available");
    auto *background=qvariant_cast<QQuickItem *>(card->property("background"));
    check(background,"rendered preview background exists");
    const auto backgroundColor=background->property("color").value<QColor>();
    const auto textColor=label->property("color").value<QColor>();
    const auto region=background->mapRectToScene(background->boundingRect());const auto dpr=surface->devicePixelRatio();
    const QRect pixels(qRound(region.x()*dpr),qRound(region.y()*dpr),qRound(region.width()*dpr),qRound(region.height()*dpr));
    const double expectedX=std::max(0.,std::min(double(pointer.x())-region.width()/2,surface->width()-region.width()));
    const double expectedY=std::max(0.,bar->mapToScene(QPointF(0,0)).y()-region.height()-8*scale);
    report.insert("previewPlacement",QJsonObject{{"name",name},{"pointerX",pointer.x()},{"actualX",region.x()},{"expectedX",expectedX},{"actualY",region.y()},{"expectedY",expectedY}});save();
    check(capture.rect().contains(pixels),"whole preview card fits its rendering surface");
    const auto point=image->mapToScene(QPointF(image->width()/2,image->height()/2))*surface->devicePixelRatio();
    const auto matchesScene=[channel](const QColor &color) {
        const int channels[]={color.red(),color.green(),color.blue()};
        return channels[channel]>180&&channels[(channel+1)%3]<70&&channels[(channel+2)%3]<70;
    };
    const auto matchesCapture=[&](const QImage &candidate) {
        if(reference.isNull())return matchesScene(candidate.pixelColor(point.toPoint()));
        for(const auto &sample:{QPoint(63,35),QPoint(189,35),QPoint(63,106),QPoint(189,106)}) {
            const auto position=image->mapToScene(QPointF(image->width()*sample.x()/252.,image->height()*sample.y()/142.))*dpr;
            const auto a=candidate.pixelColor(position.toPoint()),b=reference.pixelColor(sample);
            if(std::abs(a.red()-b.red())>=35||std::abs(a.green()-b.green())>=35||std::abs(a.blue()-b.blue())>=35)return false;
        }
        return true;
    };
    // Image.Ready precedes the shader-backed item reaching the render surface.
    // Await those actual pixels with a fixed bound; a provider-only check would
    // miss a persistent blank image or a stale scene after a cached request.
    QElapsedTimer presentation;presentation.start();
    if(!matchesCapture(capture))
        check(capture.copy(pixels).save(QDir(qEnvironmentVariable("VLC_OWNED_UI_BASE")).filePath(QString::fromLatin1(name)+QStringLiteral("-first-frame.png"))),"initial unsettled preview capture saved");
    while(!matchesCapture(capture)&&presentation.elapsed()<3000) {
        QTest::qWait(25);capture=surface->grabWindow();
        check(!capture.isNull()&&capture.rect().contains(pixels),"preview surface remains available while awaiting presentation");
    }
    const auto color=capture.pixelColor(point.toPoint());
    const bool rendered=matchesCapture(capture);
    const auto pixelsMs=hoverElapsed.elapsed();
    timing.insert("pixelsObservedMs",pixelsMs);timing.insert("pointerEvents",pointerTrace.events);timing.insert("providerEvents",stateEvents);
    check(capture.copy(pixels).save(QDir(qEnvironmentVariable("VLC_OWNED_UI_BASE")).filePath(QString::fromLatin1(name)+QStringLiteral(".png"))),"bounded preview capture saved");
    if(!rendered) {
        std::unique_ptr<QQuickImageProvider> provider(service->createImageProvider());QSize size;
        const auto raw=provider->requestImage(QUrl(service->imageSource()).path().mid(1),&size,{});
        QJsonObject probe{{"name",name},{"renderedColor",color.name()},{"rawColor",raw.isNull()?QStringLiteral("null"):raw.pixelColor(126,71).name()},{"imageVisible",image->isVisible()},{"imageOpacity",image->opacity()},{"status",image->property("status").toInt()},{"shaderStatus",image->property("shaderStatus").toInt()},{"imageWidth",image->width()},{"imageHeight",image->height()},{"source",image->property("source").toUrl().toString()}};
        QJsonArray effects;
        for(auto *child:image->children())if(QByteArray(child->metaObject()->className()).contains("ShaderEffect"))effects.append(QJsonObject{{"class",child->metaObject()->className()},{"visible",child->property("visible").toBool()},{"width",child->property("width").toDouble()},{"height",child->property("height").toDouble()}});
        probe.insert("effects",effects);QTest::qWait(750);const auto late=surface->grabWindow();
        probe.insert("lateColor",late.isNull()?QStringLiteral("null"):late.pixelColor(point.toPoint()).name());
        report.insert("renderProbe",probe);report.insert("status","awaiting-render-inspection");save();
        const auto gate=qEnvironmentVariable("VLC_OWNED_RENDER_INSPECT_GATE");
        if(!gate.isEmpty()){QElapsedTimer hold;hold.start();while(!QFile::exists(gate)&&hold.elapsed()<240000)QTest::qWait(100);}
    }
    check(rendered,"rendered preview pixels match the requested synthetic scene");
    check(std::abs(region.x()-expectedX)<2.,"preview stays centered on the actual pointer with edge clamping");
    check(std::abs(region.y()-expectedY)<2.,"preview stays above the timeline with its scaled gap");
    check(!card->property("focus").toBool()&&state()==VLC_PLAYER_STATE_PAUSED&&controller()->getInput()==input&&std::abs(time()-oldTime)<VLC_TICK_FROM_MS(50),"preview preserves focus policy, input and paused primary clock");
    if(qEnvironmentVariable("VLC_OWNED_THUMBNAIL_MODE")==QStringLiteral("extended")) {
        const bool light=context->getGeneralPreferences()->values().value("theme").toInt()==1;
        check(light ? backgroundColor.lightness()>160&&textColor.lightness()<150
                    : backgroundColor.lightness()<80&&textColor.lightness()>100,
              "actual preview background and caption follow the selected appearance");
    }
    QJsonArray corners;
    if(!reference.isNull())for(const auto &sample:{QPoint(63,35),QPoint(189,35),QPoint(63,106),QPoint(189,106)}) {
        const auto position=image->mapToScene(QPointF(image->width()*sample.x()/252.,image->height()*sample.y()/142.))*dpr;
        corners.append(capture.pixelColor(position.toPoint()).name());
    }
    previews.append(QJsonObject{{"timingTrace",timing},{"name",name},{"requested",seconds},{"sampled",service->requestedSeconds()},{"label",label->property("text").toString()},{"scale",scale},{"dpr",surface->devicePixelRatio()},{"width",card->property("width").toDouble()},{"height",card->property("height").toDouble()},{"color",color.name()},{"background",backgroundColor.name(QColor::HexArgb)},{"caption",textColor.name()},{"windowWidth",window->width()},{"windowHeight",window->height()},{"presentationWaitMs",presentation.elapsed()},{"hoverToReadyMs",readyMs},{"hoverToPixelsMs",pixelsMs},{"geometryReference",!reference.isNull()},{"renderedCorners",corners},{"placement",report.value("previewPlacement")}});save();
}
void geometryChecks() {
    const QDir fixtures(qEnvironmentVariable("VLC_OWNED_THUMBNAIL_GEOMETRY_DIR"));
    check(fixtures.exists()&&!qEnvironmentVariableIsEmpty("VLC_OWNED_THUMBNAIL_GEOMETRY_DIR"),"owned geometry fixture directory exists");
    toggle(true);
    for(const auto *name:{"landscape","portrait","sar-2-to-1","rotate-90","rotate-180","rotate-270"}) {
        const auto stem=QString::fromLatin1(name);
        QTest::mouseMove(window,QPoint(20,100));
        openMedia(fixtures.filePath(stem+QStringLiteral(".mp4")),true);
        const QImage reference(fixtures.filePath(stem+QStringLiteral("-reference.png")));
        check(reference.size()==QSize(252,142),"independent geometry reference image loads");
        preview(.75,name,0,reference);
    }
    auto *p=context->getIntf()->p_player;const auto before=time();
    vlc_player_Lock(p);vlc_player_Resume(p);vlc_player_Unlock(p);
    wait([=]{return state()==VLC_PLAYER_STATE_PLAYING&&time()>before+VLC_TICK_FROM_MS(250);},"decoded playback resumes after geometry previews");
}
void heldSeek(const char *name,bool precise,double requested,double expected) {
    auto *preferences=context->getPlaybackPreferences();
    check(preferences->setValue(QStringLiteral("preciseSeeking"),precise),"held drag selects the real precise or fast native policy");
    auto *q=slider();q->forceActiveFocus(Qt::TabFocusReason);
    const double length=controller()->getLength().toSeconds();
    const auto start=q->mapToScene(QPointF(q->width()*3.25/length,q->height()/2)).toPoint();
    const auto target=q->mapToScene(QPointF(q->width()*requested/length,q->height()/2)).toPoint();
    const auto input=controller()->getInput();
    QTest::mousePress(window,Qt::LeftButton,Qt::NoModifier,start);
    double held=0;
    try {
        QTest::mouseMove(window,target);
        wait([=]{return std::abs(double(time())/CLOCK_FREQ-expected)<.2;},"native paused seek lands while the pointer is held");
        auto *service=context->getTimelineThumbnails();
        wait([=]{return !service->loading()&&!service->imageSource().isEmpty()&&service->requestedSeconds()==std::floor(requested)&&popup();},"preview remains available during an actual held drag");
        QTest::qWait(250);held=q->property("value").toDouble();
        check(popup()&&std::abs(held-requested/length)<.01,"held slider retains the pointer target while preview remains visible");
        check(controller()->getInput()==input&&state()==VLC_PLAYER_STATE_PAUSED,"held preview preserves the actual paused input");
    }catch(...) {QTest::mouseRelease(window,Qt::LeftButton,Qt::NoModifier,target);throw;}
    QTest::mouseRelease(window,Qt::LeftButton,Qt::NoModifier,target);
    wait([=]{return std::abs(q->property("value").toDouble()-expected/length)<.01
        &&std::abs(double(controller()->getTime().toVLCTick())/CLOCK_FREQ-expected)<.2;},"released drag reconciles native landing and displayed clock without Resume");
    auto rows=report.value("heldDrags").toArray();rows.append(QJsonObject{{"name",name},{"precise",precise},{"requested",requested},{"landed",double(time())/CLOCK_FREQ},{"heldValue",held},{"releasedValue",q->property("value").toDouble()}});report.insert("heldDrags",rows);save();
}
void extendedChecks() {
    heldSeek("held-precise",true,9.25,9.25);
    heldSeek("held-fast",false,15.25,12.);
    check(context->getPlaybackPreferences()->setValue(QStringLiteral("preciseSeeking"),true),"precise policy restores after the held fast regression");
    const struct {const char *name;int width,height;double scale;bool light;} variants[]={
        {"dark-standard",1280,900,1.,false},{"dark-compact",600,420,1.,false},
        {"light-compact-125",640,520,1.25,true},{"light-standard-125",1280,900,1.25,true},
        {"light-minimum-125",600,450,1.25,true},{"dark-minimum-125",600,450,1.25,false}};
    for(const auto &variant:variants) {
        QTest::mouseMove(window,QPoint(20,100));
        check(context->getGeneralPreferences()->setValue(QStringLiteral("theme"),variant.light?1:2),"actual General service commits the selected appearance");
        context->setIntfUserScaleFactor(variant.scale);window->resize(variant.width,variant.height);QTest::qWait(350);
        check(std::abs(context->getIntfUserScaleFactor()-variant.scale)<.001,"actual interface scale policy reaches the requested variant");
        const QByteArray left=QByteArray(variant.name)+"-left",right=QByteArray(variant.name)+"-right";
        preview(.25,left.constData(),0);
        QTest::mouseMove(window,QPoint(20,100));
        wait([]{return !popup();},"leaving timeline closes preview before opening at opposite edge");
        preview(17.25,right.constData(),2);
    }
    QTest::mouseMove(window,QPoint(20,100));context->setIntfUserScaleFactor(1.);
    check(context->getGeneralPreferences()->setValue(QStringLiteral("theme"),2),"Dark appearance restores after Light variants");
    window->resize(1280,900);QTest::qWait(350);
    controller()->setFullscreen(true);
    wait([]{return window->windowStates().testFlag(Qt::WindowFullScreen)&&controller()->isFullscreen();},"actual player enters video fullscreen");
    QTest::qWait(300);preview(8.25,"fullscreen-preview",1);
    QTest::keyClick(window,Qt::Key_Escape);
    QElapsedTimer escape;escape.start();while(window->windowStates().testFlag(Qt::WindowFullScreen)&&escape.elapsed()<1500)QTest::qWait(25);
    const bool escaped=!window->windowStates().testFlag(Qt::WindowFullScreen)&&!controller()->isFullscreen();
    report.insert("singleEscapeLeftFullscreen",escaped);save();
    if(!escaped){controller()->setFullscreen(false);QTest::qWait(300);}
    check(escaped,"one Escape leaves video fullscreen while a preview is visible");
    preview(8.25,"after-fullscreen",1);
    auto *p=context->getIntf()->p_player;const auto before=time();vlc_player_Lock(p);vlc_player_Resume(p);vlc_player_Unlock(p);
    wait([=]{return state()==VLC_PLAYER_STATE_PLAYING&&time()>before+VLC_TICK_FROM_MS(250);},"decoded playback resumes after held dragging and geometry changes");
}
void run() {try {
    const auto mode=qEnvironmentVariable("VLC_OWNED_THUMBNAIL_MODE");
    window->resize(mode==QStringLiteral("cold-edge")?600:1280,mode==QStringLiteral("cold-edge")?450:900);window->requestActivate();QTest::qWait(250);
    const bool cold=mode.startsWith(QStringLiteral("cold")), initiallyOn=mode==QStringLiteral("cold-on")||mode==QStringLiteral("cold-edge");
    if(mode==QStringLiteral("extended")||mode==QStringLiteral("geometry"))check(context->getGeneralPreferences()->setValue(QStringLiteral("theme"),2),"extended journey starts from an explicit Dark policy");
    const auto base=QDir(qEnvironmentVariable("VLC_OWNED_UI_BASE")).absolutePath()+QLatin1Char('/');
    const auto config=qEnvironmentVariable("VLC_OWNED_SETTINGS_CONFIG");
    check(QFileInfo(config).absoluteFilePath().startsWith(base)&&QFileInfo(context->getIntf()->mainSettings->fileName()).absoluteFilePath().startsWith(base),"native and Qt settings are fenced to the owned runtime");
    auto *preferences=context->getPlaybackPreferences();auto *service=context->getTimelineThumbnails();
    check(preferences->capabilities().value("showThumbnails").toBool(),"actual MainCtx enables the implemented preview consumer");
    check(service->enabled()==initiallyOn&&preferences->values().value("showThumbnails").toBool()==initiallyOn,"fresh or cold startup restores the expected thumbnail choice");
    if(mode==QStringLiteral("cold-edge")) {
        check(context->getGeneralPreferences()->values().value("theme").toInt()==1
            &&std::abs(context->getIntfUserScaleFactor()-1.25)<.001,"cold ordinary-style startup restores Light and125% scale");
        wait([]{return state()==VLC_PLAYER_STATE_PAUSED&&controller()->isSeekable()&&controller()->getLength().toSeconds()>17.;},"cold command-line media is paused and seekable",15000);
        auto *bar=slider();const auto point=bar->mapToScene(QPointF(bar->width()*16.8/controller()->getLength().toSeconds(),bar->height()/2)).toPoint();
        QTest::mouseMove(window,point);QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,point);
        wait([]{return state()==VLC_PLAYER_STATE_PAUSED&&time()>VLC_TICK_FROM_SEC(16);},"first cold pointer click seeks the actual paused media");
        preview(16.8,"cold-right",2);
        report.insert("passed",true);report.insert("status","checks_complete");save();DialogsProvider::getInstance()->quit();return;
    }
    if(mode==QStringLiteral("geometry")) {
        geometryChecks();report.insert("passed",true);report.insert("status","checks_complete");save();DialogsProvider::getInstance()->quit();return;
    }
    openMedia(qEnvironmentVariable("VLC_OWNED_THUMBNAIL_VIDEO"),true);
    if(!initiallyOn) {
        hover(2.25);QTest::qWait(500);
        check(!popup()&&!service->loading()&&service->imageSource().isEmpty(),"disabled previews preserve the existing time tooltip path");
    }
    toggle(true);preview(2.25,"preview-red",0);preview(8.25,"preview-green",1);preview(14.25,"preview-blue",2);
    if(mode==QStringLiteral("extended")) {
        extendedChecks();report.insert("passed",true);report.insert("status","checks_complete");save();DialogsProvider::getInstance()->quit();return;
    }
    QTest::mouseMove(window,QPoint(20,100));
    wait([=]{return !popup()&&!service->loading()&&service->imageSource().isEmpty();},"leaving the timeline closes the preview and cancels work");
    toggle(false);hover(8.25);QTest::qWait(400);
    check(!popup()&&!service->loading()&&service->imageSource().isEmpty(),"switching the setting off restores time-only behavior");
    if(!cold) {
        openSettings();const auto accepted=bytes(config);
        FileLock lock;lock.value=CreateFileW(config.toStdWString().c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        check(lock.value!=INVALID_HANDLE_VALUE,"owned thumbnail config replacement is blocked");
        click(item("playbackshowThumbnails"));
        const bool rejected=!preferences->lastError().isEmpty()&&!service->enabled()&&!preferences->values().value("showThumbnails").toBool()&&bytes(config)==accepted;
        lock.close();check(rejected,"failed actual toggle preserves saved policy and provider state");
        auto *control=item("playbackshowThumbnails");
        check(control->hasActiveFocus()&&!control->property("validationError").toString().isEmpty(),"rejected thumbnail toggle retains focus and local feedback");
        click(control);check(preferences->lastError().isEmpty()&&service->enabled(),"same actual thumbnail toggle retries after storage recovers");closeSettings();
        preview(8.25,"preview-after-retry",1);
        openMedia(qEnvironmentVariable("VLC_OWNED_THUMBNAIL_AUDIO"),false);hover(8.25);QTest::qWait(500);
        check(!popup()&&!service->loading()&&service->imageSource().isEmpty(),"audio-only source clears video previews and keeps the time label path");
        openMedia(qEnvironmentVariable("VLC_OWNED_THUMBNAIL_VIDEO"),true);preview(14.25,"preview-reopened",2);
    }
    if(mode==QStringLiteral("cold-on"))toggle(false);
    if(mode==QStringLiteral("cold-off"))toggle(false);
    auto *p=context->getIntf()->p_player;const auto before=time();vlc_player_Lock(p);vlc_player_Resume(p);vlc_player_Unlock(p);
    wait([=]{return state()==VLC_PLAYER_STATE_PLAYING&&time()>before+VLC_TICK_FROM_MS(250);},"decoded playback resumes after preview/settings checks");
    report.insert("passed",true);report.insert("status","checks_complete");save();DialogsProvider::getInstance()->quit();
}catch(const std::exception &error) {
    if(window)window->grabWindow().save(QDir(qEnvironmentVariable("VLC_OWNED_UI_BASE")).filePath(QStringLiteral("failure.png")));
    report.insert("passed",false);report.insert("error",error.what());save();if(context)DialogsProvider::getInstance()->quit();
}}
void findWindow(int retries) {
    for(auto *o:objects())if(o->objectName()==QStringLiteral("juxtopposedDialogsHost"))if(auto *q=qobject_cast<QQuickItem *>(o)) {
        auto *engine=qmlEngine(o);context=engine?engine->singletonInstance<MainCtx *>("VLC.MainInterface","MainCtx"):nullptr;window=q->window();
    }
    if(context&&window) {
        const auto gate=qEnvironmentVariable("VLC_OWNED_UI_START_GATE");
        if(!gate.isEmpty()&&QFile::exists(gate)){QTimer::singleShot(150,context,run);return;}
        report.insert("status","awaiting-owned-window-inspection");save();
    }
    if(retries>0)QTimer::singleShot(250,qApp,[retries]{findWindow(retries-1);});
    else {report.insert("passed",false);report.insert("error","owned window/gate unavailable");save();}
}
void start() {if(!qEnvironmentVariableIsEmpty("VLC_OWNED_UI_REPORT"))QTimer::singleShot(800,qApp,[]{findWindow(1200);});}
}
Q_COREAPP_STARTUP_FUNCTION(start)
