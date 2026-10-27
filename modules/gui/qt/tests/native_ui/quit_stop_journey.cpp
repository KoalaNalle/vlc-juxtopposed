// Opt-in owned-window application-Quit regression. Never in the ordinary GUI.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "qt.hpp"
#include "maininterface/mainctx.hpp"
#include "dialogs/dialogs_provider.hpp"
#include "player/player_controller.hpp"
#include <vlc_input_item.h>
#include <vlc_interface.h>
#include <vlc_player.h>
#include <QApplication>
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
#include <stdexcept>

namespace {
MainCtx *context = nullptr;
vlc_player_t *player = nullptr;
vlc_player_listener_id *listener = nullptr;
QQuickWindow *window = nullptr;
QJsonObject report;
QJsonArray checks, events;
QString mode;
bool restarted = false;

void save()
{
    report.insert("checks", checks);
    report.insert("nativeEvents", events);
    report.insert("restartBeforeFinalizer", restarted);
    QFile output(qEnvironmentVariable("VLC_OWNED_UI_REPORT"));
    if (output.open(QIODevice::WriteOnly)) output.write(QJsonDocument(report).toJson());
}
void check(bool passed, const char *name)
{
    checks.append(QJsonObject{{"name", name}, {"passed", passed}});
    save();
    if (!passed) throw std::runtime_error(name);
}
int start()
{
    vlc_player_Lock(player);
    const auto result = vlc_player_Start(player);
    vlc_player_Unlock(player);
    return result;
}
vlc_player_state state()
{
    vlc_player_Lock(player);
    const auto result = vlc_player_GetState(player);
    vlc_player_Unlock(player);
    return result;
}
void run()
{
    try
    {
        check(context && window, "real MainCtx and owned native window ready");
        player = context->getIntf()->p_player;
        check(player != nullptr, "actual primary native player available");
        mode = qEnvironmentVariable("VLC_OWNED_QUIT_MODE");
        report.insert("mode", mode);
        report.insert("privateFixture", true);
        report.insert("fullBetaAccepted", false);
        static const vlc_player_cbs callbacks = [] {
            vlc_player_cbs cbs{};
            cbs.on_state_changed = [](vlc_player_t *, vlc_player_state value, void *) {
                QMetaObject::invokeMethod(context, [value] {
                    events.append(int(value));
                    // This listener predates the provider's. Its queued restart
                    // deliberately runs before the provider's queued finalizer.
                    if (value == VLC_PLAYER_STATE_STOPPED && mode == "restart" && !restarted)
                    {
                        restarted = true;
                        report.insert("restartAfterQuitRequested", report.value("quitRequested").toBool());
                        report.insert("restartResult", start());
                    }
                    save();
                }, Qt::QueuedConnection);
            };
            return cbs;
        }();
        vlc_player_Lock(player);
        listener = vlc_player_AddListener(player, &callbacks, nullptr);
        vlc_player_Unlock(player);
        check(listener != nullptr, "native observer installed before Quit observer");
        // The video window can retain Qt after interface teardown and even until
        // Core deletes the main player. Detach at the interface owner's lifetime.
        QObject::connect(context, &QObject::destroyed, qApp, [] {
            vlc_player_Lock(player);
            const auto finalState = vlc_player_GetState(player);
            vlc_player_RemoveListener(player, listener);
            listener = nullptr;
            vlc_player_Unlock(player);
            report.insert("stateAtInterfaceTeardown", int(finalState));
            report.insert("interfaceTeardownObserved", true);
            report.insert("passed", report.value("quitRequested").toBool()
                && !report.contains("error") && finalState == VLC_PLAYER_STATE_STOPPED
                && (mode != "restart" || (restarted && report.value("restartAfterQuitRequested").toBool()
                    && report.value("restartResult").toInt(-1) == VLC_SUCCESS)));
            save();
        });
        QObject::connect(qApp, &QCoreApplication::aboutToQuit, qApp, [] {
            report.insert("guiExitObserved", true);
            save();
        });

        if (mode != "idle")
        {
            const auto uri = QUrl::fromLocalFile(qEnvironmentVariable("VLC_OWNED_QUIT_MEDIA")).toEncoded();
            auto *media = input_item_New(uri.constData(), "Owned Quit regression");
            check(media != nullptr, "owned generated media created");
            vlc_player_Lock(player);
            const auto selected = vlc_player_SetCurrentMedia(player, media);
            vlc_player_Unlock(player);
            input_item_Release(media);
            check(selected == VLC_SUCCESS, "owned media selected on actual primary player");
            check(start() == VLC_SUCCESS, "primary Start succeeds");
            if (mode != "rapid")
            {
                for (int i = 0; i < 200 && state() != VLC_PLAYER_STATE_PLAYING; ++i) QTest::qWait(25);
                check(state() == VLC_PLAYER_STATE_PLAYING, "actual primary player reaches Playing");
                QTest::qWait(1200);
                check(state() == VLC_PLAYER_STATE_PLAYING, "primary remains Playing immediately before quit preparation");
                if (mode == "paused")
                {
                    vlc_player_Lock(player);
                    vlc_player_Pause(player);
                    vlc_player_Unlock(player);
                    for (int i = 0; i < 200 && state() != VLC_PLAYER_STATE_PAUSED; ++i) QTest::qWait(25);
                    check(state() == VLC_PLAYER_STATE_PAUSED, "actual primary player reaches Paused");
                }
            }
        }
        if (qEnvironmentVariableIntValue("VLC_OWNED_QUIT_FULLSCREEN"))
        {
            context->getIntf()->p_mainPlayerController->setFullscreen(true);
            for (int i = 0; i < 100 && window->visibility() != QWindow::FullScreen; ++i) QTest::qWait(25);
            check(window->visibility() == QWindow::FullScreen, "actual player entered fullscreen immediately before Quit");
        }
        report.insert("nativeStateBeforeQuit", int(state()));
        report.insert("cachedStateBeforeQuit", int(context->getIntf()->p_mainPlayerController->getPlayingState()));
        if (mode == "rapid")
            check(report.value("cachedStateBeforeQuit").toInt(-1) == int(PlayerController::PLAYING_STATE_STOPPED),
                "rapid Start has not delivered the Qt cached-state update");
        if (mode == "restart")
            check(!restarted, "restart has not occurred before the Quit request");
        report.insert("quitRequested", true);
        save();
        DialogsProvider::getInstance()->quit();
        if (mode == "repeat")
            for (int i = 0; i < 5; ++i) DialogsProvider::getInstance()->quit();
    }
    catch (const std::exception &error)
    {
        report.insert("passed", false);
        report.insert("error", error.what());
        save();
        // A failed precondition is rejected; this fallback is never acceptance.
        if (context) libvlc_Quit(vlc_object_instance(context->getIntf()));
    }
}
void waitForWindow(int attempts)
{
    QList<QObject *> pending;
    QSet<QObject *> visited;
    for (auto *candidate : QGuiApplication::allWindows())
        if (auto *quick = qobject_cast<QQuickWindow *>(candidate)) pending.append(quick->contentItem());
    while (!pending.isEmpty())
    {
        auto *object = pending.takeLast();
        if (!object || visited.contains(object)) continue;
        visited.insert(object);
        pending.append(object->children());
        if (auto *item = qobject_cast<QQuickItem *>(object))
        {
            for (auto *child : item->childItems()) pending.append(child);
            if (object->objectName() == QStringLiteral("juxtopposedDialogsHost"))
            {
                auto *engine = qmlEngine(object);
                context = engine ? engine->singletonInstance<MainCtx *>("VLC.MainInterface", "MainCtx") : nullptr;
                window = item->window();
            }
        }
    }
    if (context && window)
    {
        const auto gate = qEnvironmentVariable("VLC_OWNED_UI_START_GATE");
        if (gate.isEmpty() || QFile::exists(gate)) { QTimer::singleShot(150, qApp, run); return; }
        report.insert("status", "awaiting-owned-window-inspection");
        save();
    }
    if (attempts > 0) QTimer::singleShot(250, qApp, [attempts] { waitForWindow(attempts - 1); });
    else { report.insert("passed", false); report.insert("error", "window or inspection gate unavailable"); save(); }
}
void startOwnedJourney()
{
    if (qEnvironmentVariableIsEmpty("VLC_OWNED_UI_REPORT")) return;
    QTimer::singleShot(800, qApp, [] { waitForWindow(1200); });
}
}
Q_COREAPP_STARTUP_FUNCTION(startOwnedJourney)
