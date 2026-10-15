// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef VLC_QT_FULLSCREEN_SCREEN_POLICY_HPP
#define VLC_QT_FULLSCREEN_SCREEN_POLICY_HPP
#include <QGuiApplication>
#include <QMetaType>
#include <QPointer>
#include <QScreen>
#include <QSharedPointer>
#include <QVariant>
#include <QWindow>

namespace vlc::qt {
// Owned by the QWindow's dynamic QVariant. A disconnected original screen is
// observed through QPointer rather than a stale QScreen pointer. No global
// window registry or independent QObject lifetime is introduced.
struct FullscreenScreenState {
    QPointer<QScreen> previousScreen;
    QRect previousGeometry;
    bool restoreGeometry = false;
};
using FullscreenScreenStatePtr = QSharedPointer<FullscreenScreenState>;
}
Q_DECLARE_METATYPE(vlc::qt::FullscreenScreenStatePtr)

namespace vlc::qt {
inline bool holdWindowFullscreen(QWindow *window, unsigned source, bool hold,
                                 int screenIndex = -1)
{
    if (!window) return false;
    bool valid = false;
    unsigned before = window->property("__windowFullScreen").toUInt(&valid);
    if (!valid) before = 0;
    const unsigned after = hold ? before | source : before & ~source;
    const auto oldFlags = window->windowStates();
    const auto newFlags = after ? oldFlags | Qt::WindowFullScreen : oldFlags & ~Qt::WindowFullScreen;
    const bool entering = before == 0 && after != 0;
    const bool leaving = before != 0 && after == 0;
    constexpr auto stateKey = "__vlcFullscreenScreenPolicy";

    if (entering) {
        window->setProperty(stateKey, QVariant{});
        const auto screens = QGuiApplication::screens();
        // Qt Wayland doesn't support arbitrary output/position requests. The
        // service's selectable-screen capability is separately Windows-gated.
        const bool maySelect = !QGuiApplication::platformName().startsWith(QLatin1String("wayland"));
        if (maySelect && screenIndex >= 0 && screenIndex < screens.size()) {
            auto target = screens[screenIndex];
            if (target != window->screen() || !target->geometry().contains(window->position())) {
                auto state = FullscreenScreenStatePtr::create();
                state->previousScreen = window->screen();
                state->previousGeometry = window->geometry();
                state->restoreGeometry = !(oldFlags & Qt::WindowFullScreen);
                window->setProperty(stateKey, QVariant::fromValue(state));
                window->setScreen(target);
                if (!target->geometry().contains(window->position()))
                    window->setPosition(target->geometry().topLeft());
            }
        }
    }
    window->setProperty("__windowFullScreen", QVariant::fromValue(after));
    if (newFlags != oldFlags) window->setWindowStates(newFlags);
    if (leaving) {
        const auto state = window->property(stateKey).value<FullscreenScreenStatePtr>();
        window->setProperty(stateKey, QVariant{});
        if (state && state->previousScreen && QGuiApplication::screens().contains(state->previousScreen.data())) {
            window->setScreen(state->previousScreen);
            // Retain maximized/minimized state. Normal windows recover their
            // exact previous geometry while the original output still exists.
            if (state->restoreGeometry && !(newFlags & (Qt::WindowMaximized | Qt::WindowMinimized)))
                window->setGeometry(state->previousGeometry);
        }
    }
    return after != 0;
}
}
#endif
