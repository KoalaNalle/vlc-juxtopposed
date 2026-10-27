// SPDX-License-Identifier: GPL-2.0-or-later
// Hidden native QWindow state/screen component proof. This does not accept the
// visibly presented app, OS placement/DPI behavior or actual screen hot-unplug.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "util/fullscreen_screen_policy.hpp"
#include <QApplication>
#include <QTest>
#include <QtPlugin>
// Root's static Windows build must link the actual production plugin imports
// and platform libraries. Do not substitute a QCoreApplication fixture here.
#include "plugins.hpp"

class TestFullscreenScreenPolicy : public QObject
{
    Q_OBJECT
    static constexpr unsigned Interface = 1, Video = 2;
    QScreen *anchor() const {
        const auto screens = QGuiApplication::screens();
        return screens.isEmpty() ? nullptr : screens[screens.size() > 1 ? 1 : 0];
    }
private slots:
    void actualScreensAndIndependentHolders() {
        QVERIFY(anchor());
        QWindow window; window.setScreen(anchor());
        const auto initial = QRect(anchor()->availableGeometry().topLeft() + QPoint(20,20), QSize(320,240));
        window.setGeometry(initial);
        QVERIFY(vlc::qt::holdWindowFullscreen(&window, Interface, true));
        QCOMPARE(window.property("__windowFullScreen").toUInt(), Interface);
        QVERIFY(window.windowStates() & Qt::WindowFullScreen);
        QVERIFY(vlc::qt::holdWindowFullscreen(&window, Video, true));
        QCOMPARE(window.property("__windowFullScreen").toUInt(), Interface | Video);
        QVERIFY(vlc::qt::holdWindowFullscreen(&window, Interface, false));
        QCOMPARE(window.property("__windowFullScreen").toUInt(), Video);
        QVERIFY(window.windowStates() & Qt::WindowFullScreen);
        QVERIFY(!vlc::qt::holdWindowFullscreen(&window, Video, false));
        QVERIFY(!(window.windowStates() & Qt::WindowFullScreen));
        QCOMPARE(window.screen(), anchor());
        QCOMPARE(window.geometry(), initial);
    }
    void releasingVideoFirstPreservesInterfaceHold() {
        QVERIFY(anchor());
        QVERIFY(!vlc::qt::isWindowFullscreenHeld(nullptr, Interface));
        QWindow window; window.setScreen(anchor());
        const auto initial = QRect(anchor()->availableGeometry().topLeft() + QPoint(25,25), QSize(320,240));
        window.setGeometry(initial);
        QVERIFY(vlc::qt::holdWindowFullscreen(&window, Interface, true));
        QVERIFY(vlc::qt::isWindowFullscreenHeld(&window, Interface));
        QVERIFY(!vlc::qt::isWindowFullscreenHeld(&window, Video));
        QVERIFY(vlc::qt::holdWindowFullscreen(&window, Video, true));
        QVERIFY(vlc::qt::isWindowFullscreenHeld(&window, Video));
        QCOMPARE(window.property("__windowFullScreen").toUInt(), Interface | Video);
        // The player title strip now releases the VIDEO owner first. That must
        // not clear an independently acquired interface/F11 fullscreen hold.
        QVERIFY(vlc::qt::holdWindowFullscreen(&window, Video, false));
        QVERIFY(vlc::qt::isWindowFullscreenHeld(&window, Interface));
        QVERIFY(!vlc::qt::isWindowFullscreenHeld(&window, Video));
        QCOMPARE(window.property("__windowFullScreen").toUInt(), Interface);
        QVERIFY(window.windowStates() & Qt::WindowFullScreen);
        QVERIFY(!vlc::qt::holdWindowFullscreen(&window, Interface, false));
        QVERIFY(!vlc::qt::isWindowFullscreenHeld(&window, Interface));
        QCOMPARE(window.property("__windowFullScreen").toUInt(), 0u);
        QVERIFY(!(window.windowStates() & Qt::WindowFullScreen));
        QCOMPARE(window.screen(), anchor());
        QCOMPARE(window.geometry(), initial);
    }
    void sharedTargetAndFinalReleaseRestore() {
        const auto screens = QGuiApplication::screens();
        if (screens.size() < 2) QSKIP("Two genuine connected QScreens are required");
        const int anchorIndex = screens.indexOf(anchor());
        const int targetIndex = anchorIndex == 0 ? 1 : 0;
        const auto initial = QRect(anchor()->availableGeometry().topLeft() + QPoint(30,30), QSize(320,240));
        QWindow window; window.setScreen(anchor()); window.setGeometry(initial);
        QVERIFY(vlc::qt::holdWindowFullscreen(&window, Interface, true, targetIndex));
        QCOMPARE(window.screen(), screens[targetIndex]);
        QVERIFY(screens[targetIndex]->geometry().contains(window.position()));
        const auto movedGeometry = window.geometry();
        // A second route cannot replace the first route's original screen or
        // silently move an already-held fullscreen window to its own target.
        QVERIFY(vlc::qt::holdWindowFullscreen(&window, Video, true, anchorIndex));
        QCOMPARE(window.screen(), screens[targetIndex]); QCOMPARE(window.geometry(), movedGeometry);
        QVERIFY(vlc::qt::holdWindowFullscreen(&window, Interface, false));
        QCOMPARE(window.screen(), screens[targetIndex]);
        QVERIFY(!vlc::qt::holdWindowFullscreen(&window, Video, false));
        QCOMPARE(window.screen(), anchor()); QCOMPARE(window.geometry(), initial);
        QVERIFY(!window.property("__vlcFullscreenScreenPolicy").isValid());
    }
    void invalidAndCurrentChoicesDoNotMove() {
        QVERIFY(anchor());
        const int invalidTarget = int(QGuiApplication::screens().size()) + 10;
        for (int target : {-1, invalidTarget}) {
            QWindow window; window.setScreen(anchor());
            const auto initial = QRect(anchor()->availableGeometry().topLeft() + QPoint(40,40), QSize(320,240));
            window.setGeometry(initial);
            QVERIFY(vlc::qt::holdWindowFullscreen(&window, Video, true, target));
            QCOMPARE(window.screen(), anchor()); QCOMPARE(window.geometry(), initial);
            QVERIFY(!vlc::qt::holdWindowFullscreen(&window, Video, false));
            QCOMPARE(window.screen(), anchor()); QCOMPARE(window.geometry(), initial);
        }
    }
    void stateBelongsToWindowLifetime() {
        const auto screens = QGuiApplication::screens();
        if (screens.size() < 2) QSKIP("Two genuine connected QScreens are required");
        QWeakPointer<vlc::qt::FullscreenScreenState> weak;
        {
            QWindow window; window.setScreen(anchor());
            window.setGeometry(QRect(anchor()->availableGeometry().topLeft() + QPoint(30,30), QSize(320,240)));
            const int target = screens.indexOf(anchor()) == 0 ? 1 : 0;
            QVERIFY(vlc::qt::holdWindowFullscreen(&window, Interface, true, target));
            const auto state = window.property("__vlcFullscreenScreenPolicy").value<vlc::qt::FullscreenScreenStatePtr>();
            QVERIFY(state); weak = state.toWeakRef();
        }
        QVERIFY(weak.isNull());
    }
};
QTEST_MAIN(TestFullscreenScreenPolicy)
#include "test_fullscreen_screen_policy.moc"
