// Private owned-window test TU. Never linked into the ordinary GUI.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "qt.hpp"
#include "maininterface/mainctx.hpp"
#include "maininterface/subtitle_preset_model.hpp"
#include "player/subtitle_appearance.hpp"
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
    check(item&&item->isVisible()&&item->isEnabled(),"real popup action is visible and enabled");
    auto *targetWindow=item->window();check(targetWindow!=nullptr,"action belongs to owned window");
    targetWindow->requestActivate();item->forceActiveFocus(Qt::TabFocusReason);settle(120);
    const auto center=item->mapToScene(item->boundingRect().center());
    check(QRectF(QPointF{},QSizeF(targetWindow->size())).contains(center),"focused popup action is in owned window bounds");
    for(auto *p=item->parentItem();p;p=p->parentItem())if(p->clip())
        check(p->boundingRect().contains(p->mapFromScene(center)),"focused popup action scrolls into clipped viewport");
    auto targets=report.value("ownedPointerWindows").toArray();
    targets.append(QJsonObject{{"class",QString::fromLatin1(targetWindow->metaObject()->className())},{"isMainWindow",targetWindow==ownedWindow},{"width",targetWindow->width()},{"height",targetWindow->height()}});
    report.insert("ownedPointerWindows",targets);
    QTest::mouseClick(targetWindow,Qt::LeftButton,Qt::NoModifier,center.toPoint());settle();
}
void enter(QQuickItem *item,const QString &text,bool commit=true)
{
    activate(item);QTest::keyClick(ownedWindow,Qt::Key_A,Qt::ControlModifier);
    QKeyEvent press(QEvent::KeyPress,0,Qt::NoModifier,text);QCoreApplication::sendEvent(ownedWindow,&press);
    if(commit)
        QTest::keyClick(ownedWindow,Qt::Key_Return);
    settle();
}
QQuickItem *editor(const char *title)
{
    for(auto *object:objects())if(object->property("title").toString()==QString::fromUtf8(title))
        if(auto *item=qobject_cast<QQuickItem*>(qvariant_cast<QObject*>(object->property("editor")));item&&item->isVisible())return item;
    return nullptr;
}
void menuEntry(const QString &id)
{
    auto *selector=named(QStringLiteral("subtitlePresetSelector"));
    activate(selector);
    activate(named(QStringLiteral("subtitlePresetEntry_")+id));
    QQmlExpression menuVisible(qmlContext(selector),selector,QStringLiteral("presets.visible"));
    const auto visible=menuVisible.evaluate();
    check(!menuVisible.hasError()&&!visible.toBool(),"preset selection closes actual submenu before model refresh");
}
void newPreset()
{
    activate(named(QStringLiteral("subtitlePresetSelector")));activate(named(QStringLiteral("newSubtitlePreset")));
    until([]{auto *field=named(QStringLiteral("subtitlePresetName"));return field&&field->hasActiveFocus();},"New Custom Preset focuses real naming editor");
}
void capture(const QString &name)
{
    check(ownedWindow->grabWindow().save(QDir(qEnvironmentVariable("VLC_OWNED_UI_OUTPUT")).filePath(name)),"owned native popup capture saved");
}
void run()
{
    try{
        auto *player=context->getIntf()->p_mainPlayerController;
        auto *presets=context->subtitlePresets();auto *style=player->getSubtitleAppearance();
        check(player&&presets&&style,"actual application owns player, preset service and appearance");
        until([&]{return player->hasInput()&&presets->canSave()&&style->available();},"generated primary video has native text subtitle and output",15000);
        auto *native=context->getIntf()->p_player;vlc_player_Lock(native);vlc_player_Pause(native);vlc_player_Unlock(native);
        until([&]{return player->getPlayingState()==PlayerController::PLAYING_STATE_PAUSED;},"actual primary player pauses for deterministic popup journey");
        auto primarySnapshot=[&]{
            vlc_player_Lock(native);const auto time=vlc_player_GetTime(native);const auto state=vlc_player_GetState(native);const auto nativePosition=vlc_player_GetPosition(native);vlc_player_Unlock(native);
            return QJsonObject{{"nativeTime",qint64(time)},{"nativeState",int(state)},{"nativePosition",nativePosition},{"uiPosition",player->getPosition()},{"uiState",int(player->getPlayingState())},{"rate",player->getRate()}};
        };
        // Paused-state notification can precede the final queued UI clock update.
        QJsonArray pauseSamples;auto previous=primarySnapshot();pauseSamples.append(previous);int stable=0;
        QDeadlineTimer pauseDeadline(4000);
        while(stable<4&&!pauseDeadline.hasExpired()){
            settle(100);const auto current=primarySnapshot();pauseSamples.append(current);
            stable=current==previous?stable+1:0;previous=current;
        }
        report.insert("pauseSettleSamples",pauseSamples);
        check(stable>=4&&previous.value("nativeTime").toDouble()>=0,"native and UI pause clocks settle before interaction baseline");
        const auto nativeBaseline=primarySnapshot();report.insert("primaryBefore",nativeBaseline);
        auto *input=player->getInput();check(input!=nullptr,"primary input identity exists");input_item_Hold(input);
        std::unique_ptr<input_item_t,decltype(&input_item_Release)> hold(input,&input_item_Release);
        const auto url=player->getUrl();const auto position=player->getPosition();const auto rate=player->getRate();
        auto preserved=[&]{
            const auto current=primarySnapshot();report.insert("primaryAfter",current);
            report.insert("primaryIdentity",QJsonObject{{"sameInput",player->getInput()==input},{"sameUrl",player->getUrl()==url}});
            return player->getInput()==input&&player->getUrl()==url&&player->getPlayingState()==PlayerController::PLAYING_STATE_PAUSED&&qFuzzyCompare(player->getRate(),rate)&&std::abs(player->getPosition()-position)<.0001&&current.value("nativeTime")==nativeBaseline.value("nativeTime");
        };
        ownedWindow->resize(1280,900);ownedWindow->requestActivate();QTest::mouseMove(ownedWindow,QPoint(1100,850));settle(200);
        QPointer<QQuickItem> opener=named(QStringLiteral("juxtopposedSettings"));
        activate(opener);activate(named(QStringLiteral("playerSettingsTab3")));
        QPointer<QObject> popup;
        int popupCount=0;
        for(auto *object:objects())if(object->property("maximumHeight").isValid()&&object->property("subtitleTimelineVisible").isValid()&&object->property("closePolicy").isValid()&&object->property("visible").toBool()){
            popup=object;++popupCount;
        }
        check(popupCount==1&&popup,"actual single player-settings popup object is identified");
        until([]{return named(QStringLiteral("subtitlePresetSelector"))!=nullptr;},"actual Subtitles tab mounts native preset selector");
        const auto mode=qEnvironmentVariable("VLC_OWNED_UI_MODE");
        if(mode==QStringLiteral("reopen")){
            until([&]{return presets->selectedName()==QStringLiteral("Travel Ää")&&!presets->restoring()&&!presets->edited();},"actual process restart restores selected preset for same media");
            check(QColor(style->textColor())==QColor(QStringLiteral("#ff0000"))&&player->getSubtitleDelayMS()==375,"restarted native preset restores appearance and signed timing");capture(QStringLiteral("restarted-preset.png"));
            menuEntry(QStringLiteral("default"));until([&]{return presets->selectedId()==QStringLiteral("default")&&!presets->restoring();},"actual Default menu action confirms native restore");
            check(player->getSubtitleDelayMS()==0&&!presets->edited(),"Default clears native timing and edited state");capture(QStringLiteral("default-preset.png"));
        }else if(mode==QStringLiteral("default-reopen")){
            until([&]{return presets->selectedId()==QStringLiteral("default")&&!presets->restoring();},"Default association survives actual process restart");
            check(player->getSubtitleDelayMS()==0&&presets->entries().size()==3,"default restart preserves saved records without reapplying named timing");
        }else{
            check(presets->entries().size()==1,"fresh owned store contains only Default");
            enter(editor("Subtitle track delay"),QStringLiteral("0.375"));
            until([&]{return player->getSubtitleDelayMS()==375;},"real timing editor sets exact native 375ms");
            enter(editor("Text color"),QStringLiteral("#ff0000"));
            until([&]{return QColor(style->textColor())==QColor(QStringLiteral("#ff0000"));},"real appearance editor updates native subtitle color");
            newPreset();check(!named(QStringLiteral("subtitlePresetSave"))->isEnabled(),"empty name disables actual Save action");
            QTest::keyClick(ownedWindow,Qt::Key_Escape);settle();
            check(named(QStringLiteral("subtitlePresetName"))==nullptr&&named(QStringLiteral("subtitlePresetSelector"))->hasActiveFocus(),"Escape cancels naming and restores selector focus");
            newPreset();enter(named(QStringLiteral("subtitlePresetName")),QStringLiteral("Travel Ää"),false);activate(named(QStringLiteral("subtitlePresetSave")));
            until([&]{return presets->entries().size()==2&&presets->selectedName()==QStringLiteral("Travel Ää")&&!presets->edited();},"actual Save creates one Unicode native preset");
            const auto saved=presets->selectedId();check(saved!=QStringLiteral("default"),"saved native preset has stable nondefault identity");capture(QStringLiteral("saved-preset.png"));
            enter(editor("Text color"),QStringLiteral("#0000ff"));until([&]{return presets->edited();},"native edit marks saved preset as Custom settings");
            check(named(QStringLiteral("subtitlePresetSelector"))->property("text").toString()==QStringLiteral("Custom settings"),"actual selector reflects native edited state");
            menuEntry(saved);until([&]{return !presets->restoring()&&!presets->edited()&&presets->selectedId()==saved;},"actual saved menu action reapplies native preset");
            check(QColor(style->textColor())==QColor(QStringLiteral("#ff0000"))&&player->getSubtitleDelayMS()==375,"saved menu restores actual style and timing");
            newPreset();enter(named(QStringLiteral("subtitlePresetName")),QStringLiteral("Travel Ää"),false);activate(named(QStringLiteral("subtitlePresetSave")));
            check(!presets->error().isEmpty()&&presets->entries().size()==2&&named(QStringLiteral("subtitlePresetName")),"duplicate Save exposes native error and retains editor without duplicate record");
            named(QStringLiteral("subtitlePresetName"))->forceActiveFocus();QTest::keyClick(ownedWindow,Qt::Key_Escape);settle();
            newPreset();enter(named(QStringLiteral("subtitlePresetName")),QStringLiteral("Retry owned preset"),false);
            const auto path=context->getIntf()->mainSettings->fileName().toStdWString();
            HANDLE file=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
            check(file!=INVALID_HANDLE_VALUE,"owned Qt preferences file is locked against replacement");
            activate(named(QStringLiteral("subtitlePresetSave")));CloseHandle(file);
            check(!presets->error().isEmpty()&&presets->entries().size()==2&&named(QStringLiteral("subtitlePresetName")),"failed native save retains editor and previous records");{
                auto *status=named(QStringLiteral("subtitlePresetStatus"));check(status&&status->property("text").toString()==presets->error(),"actual native save error is presented in popup");
                const auto center=status->mapToScene(status->boundingRect().center());
                for(auto *p=status->parentItem();p;p=p->parentItem())if(p->clip())check(p->boundingRect().contains(p->mapFromScene(center)),"native error feedback scrolls into actual clipped viewport");
            }
            capture(QStringLiteral("locked-save-error.png"));
            activate(named(QStringLiteral("subtitlePresetSave")));
            until([&]{return presets->error().isEmpty()&&presets->entries().size()==3&&!named(QStringLiteral("subtitlePresetName"));},"actual Save retries successfully after owned file unlock");
            menuEntry(saved);until([&]{return presets->selectedId()==saved&&!presets->restoring()&&!presets->edited();},"restore original saved selection for restart");
        }
        check(preserved(),"popup save/edit/apply/default work preserves primary input, pause, rate and clock");
        auto windowState=[&]{
            auto *focus=ownedWindow->activeFocusItem();
            return QJsonObject{{"width",ownedWindow->width()},{"height",ownedWindow->height()},{"popupExists",bool(popup)},{"popupVisible",popup&&popup->property("visible").toBool()},{"popupOpened",popup&&popup->property("opened").toBool()},{"focusName",focus?focus->objectName():QString{}},{"focusClass",focus?QString::fromLatin1(focus->metaObject()->className()):QString{}}};
        };
        auto *selector=named(QStringLiteral("subtitlePresetSelector"));check(selector!=nullptr,"actual preset selector remains available for mouse-focus key checks");
        selector->forceActiveFocus(Qt::MouseFocusReason);settle(100);
        QJsonArray ancestors;bool popupOwner=false;
        for(auto *item=selector;item;item=item->parentItem()){
            ancestors.append(QJsonObject{{"class",QString::fromLatin1(item->metaObject()->className())},{"visible",item->isVisible()}});
            popupOwner|=item->isVisible()&&item->inherits("QQuickPopupItem");
        }
        report.insert("focusedAncestors",ancestors);
        check(popupOwner&&!selector->property("visualFocus").toBool(),"real mouse-focused preset control belongs to visible native popup item");
        QTest::keyClick(ownedWindow,Qt::Key_Space);settle(150);
        QQmlExpression menuOpen(qmlContext(selector),selector,QStringLiteral("presets.visible"));
        check(menuOpen.evaluate().toBool()&&!menuOpen.hasError()&&preserved(),"Space opens real preset menu without changing paused primary");
        QTest::keyClick(ownedWindow,Qt::Key_Escape);settle(150);
        check(!menuOpen.evaluate().toBool()&&popup&&popup->property("visible").toBool()&&preserved(),"first Escape closes preset submenu and preserves parent popup and primary");
        const auto sizeBeforeEscape=ownedWindow->size();
        report.insert("beforeEscape",windowState());
        QTest::keyClick(ownedWindow,Qt::Key_Escape);settle(250);
        report.insert("afterEscape",windowState());
        until([&]{return !popup||!popup->property("visible").toBool();},"Escape closes actual player-settings popup object",2000);
        check(ownedWindow->size()==sizeBeforeEscape,"closing player popup does not resize the main window");
        check(opener&&opener->hasActiveFocus(),"closing popup returns keyboard focus to original Settings button");
        check(preserved(),"closing popup preserves actual primary input and both paused clocks");
        report.insert("entries",QJsonArray::fromVariantList(presets->entries()));report.insert("selectedId",presets->selectedId());report.insert("selectedName",presets->selectedName());report.insert("subtitleDelayMS",player->getSubtitleDelayMS());report.insert("textColor",style->textColor());
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
