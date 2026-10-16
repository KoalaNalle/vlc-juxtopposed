// SPDX-License-Identifier: GPL-2.0-or-later
// Exact production key-routing bodies, real Qt event delivery and LibVLC
// key-pressed callbacks. The small classes below adapt unrelated dependencies;
// they do not duplicate routing guards or claim full MainCtx/widget lifetime.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "qt.hpp"
#include "util/singleton.hpp"
#include "util/vlchotkeyconverter.hpp"
#include <vlc/vlc.h>
#include <vlc_actions.h>
#include <QCoreApplication>
#include <QKeyEvent>
#include <QQmlProperty>
#include <QQmlEngine>
#include <QQuickItem>
#include <QTest>
#include <QVector>
#include <cstdio>

extern "C" vlc_object_t *vlc_test_media_player_object(libvlc_media_player_t *);

class DialogsProvider {
public:
    bool visible = false;
    bool qmlDialogVisible() const { return visible; }
};
struct DiagnosticCompositor {
    QQuickItem *focus = nullptr;
    QQuickItem *activeFocusItem() const { return focus; }
};
struct DiagnosticInterface {
    vlc_object_t obj;
    DiagnosticCompositor *p_compositor;
};
class MainCtx {
public:
    explicit MainCtx(DiagnosticInterface *interface) : p_intf(interface) {}
    DiagnosticInterface *p_intf;
    bool preferred = true;
    bool preferHotkeys() const { return preferred; }
    void sendHotkey(Qt::Key, Qt::KeyboardModifiers);
    void sendVLCHotkey(int);
};
class InterfaceWindowHandler : public QObject {
public:
    InterfaceWindowHandler(DiagnosticInterface *interface, MainCtx *context)
        : p_intf(interface), m_mainCtx(context) {}
    DiagnosticInterface *p_intf;
    MainCtx *m_mainCtx;
    unsigned i_kc_offset = 0;
    const Qt::Key kc[10] {Qt::Key_Up, Qt::Key_Up, Qt::Key_Down, Qt::Key_Down,
        Qt::Key_Left, Qt::Key_Right, Qt::Key_Left, Qt::Key_Right, Qt::Key_B, Qt::Key_A};
    void kc_pressed() {} // Unrelated Easter-egg signal; production branch retained.
    bool applyKeyEvent(QKeyEvent *) const;
    bool eventFilter(QObject *, QEvent *) override;
};

#ifndef VLC_SHARED_MODAL_PRODUCTION_INCLUDE
# define VLC_SHARED_MODAL_PRODUCTION_INCLUDE "shared_modal_hotkey_production.inc"
#endif
#include VLC_SHARED_MODAL_PRODUCTION_INCLUDE

class KeyReceiver : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(bool visualFocus READ visualFocus CONSTANT)
public:
    bool visualFocus() const { return false; } // Actual problematic mouse-focus policy.
    QVector<int> presses, releases;
protected:
    void keyPressEvent(QKeyEvent *event) override { presses.append(event->key()); event->accept(); }
    void keyReleaseEvent(QKeyEvent *event) override { releases.append(event->key()); event->accept(); }
};

class TestSharedModalHotkeys : public QObject {
    Q_OBJECT
public:
    static void initMain() {
        std::fprintf(stderr, "ROUTING_STARTUP before-core-application\n");
        std::fflush(stderr);
    }
    TestSharedModalHotkeys() {
        std::fprintf(stderr, "ROUTING_STARTUP core-fixture-constructed\n");
        std::fflush(stderr);
    }
private:
    libvlc_instance_t *instance = nullptr;
    libvlc_media_player_t *player = nullptr;
    DiagnosticInterface *interface = nullptr;
    DiagnosticCompositor compositor;
    QVector<int64_t> dispatched;
    static int nativeKey(vlc_object_t *, const char *, vlc_value_t, vlc_value_t value, void *opaque)
    { static_cast<TestSharedModalHotkeys *>(opaque)->dispatched.append(value.i_int); return VLC_SUCCESS; }
    void key(QObject *receiver, QEvent::Type type, Qt::Key code) {
        QKeyEvent event(type, code, Qt::NoModifier);
        QCoreApplication::sendEvent(receiver, &event);
        QVERIFY(event.isAccepted()); // Either native route or the Qt receiver handled it.
    }
    void keys(QObject *receiver) {
        for (Qt::Key code : {Qt::Key_Space, Qt::Key_Up, Qt::Key_Down}) {
            key(receiver, QEvent::KeyPress, code);
            key(receiver, QEvent::KeyRelease, code);
        }
    }
private slots:
    void initTestCase() {
        std::fprintf(stderr, "ROUTING_STARTUP native-init-entered\n");
        std::fflush(stderr);
        qInfo("ROUTING_PRODUCTION_SHA256 %s", routingProductionIdentity);
        const char *args[] {"--ignore-config", "--no-media-library", "--no-auto-preparse", "--quiet"};
        instance = libvlc_new(ARRAY_SIZE(args), args); QVERIFY(instance);
        std::fprintf(stderr, "ROUTING_STARTUP native-instance-created\n");
        std::fflush(stderr);
        player = libvlc_media_player_new(instance, nullptr, nullptr); QVERIFY(player);
        interface = static_cast<DiagnosticInterface *>(vlc_object_create(vlc_test_media_player_object(player), sizeof(*interface)));
        QVERIFY(interface); interface->p_compositor = &compositor;
        QCOMPARE(var_Create(vlc_object_instance(interface), "key-pressed", VLC_VAR_INTEGER), VLC_SUCCESS);
        var_AddCallback(vlc_object_instance(interface), "key-pressed", nativeKey, this);
        // The actual VLC callback registration API returns void. Prove it is
        // registered by triggering a real callback before any routing checks.
        var_TriggerCallback(vlc_object_instance(interface), "key-pressed");
        QCOMPARE(dispatched.size(), 1);
        std::fprintf(stderr, "ROUTING_STARTUP native-callback-proved\n");
        std::fflush(stderr);
    }
    void absentProviderPreservesDirectDispatch() {
        QVERIFY(!Singleton<DialogsProvider>::getInstance<false>());
        dispatched.clear(); MainCtx context(interface);
        context.sendHotkey(Qt::Key_Space, Qt::NoModifier);
        context.sendHotkey(Qt::Key_Up, Qt::NoModifier);
        context.sendHotkey(Qt::Key_Down, Qt::NoModifier);
        context.sendVLCHotkey(KEY_LEFT);
        QCOMPARE(dispatched, (QVector<int64_t>{' ', KEY_UP, KEY_DOWN, KEY_LEFT}));
    }
    void mouseFocusOutsideModalDispatchesNativeKeys() {
        auto *provider = Singleton<DialogsProvider>::getInstance(); provider->visible = false;
        dispatched.clear(); KeyReceiver receiver; compositor.focus = &receiver;
        MainCtx context(interface); InterfaceWindowHandler handler(interface, &context);
        receiver.installEventFilter(&handler); keys(&receiver);
        QCOMPARE(dispatched, (QVector<int64_t>{' ', KEY_UP, KEY_DOWN}));
        QVERIFY(receiver.presses.isEmpty() && receiver.releases.isEmpty());
        compositor.focus = nullptr;
    }
    void modalKeepsPressAndReleaseInQtAndBlocksDirectDispatch() {
        auto *provider = Singleton<DialogsProvider>::getInstance(); provider->visible = true;
        dispatched.clear(); KeyReceiver receiver; compositor.focus = &receiver;
        MainCtx context(interface); InterfaceWindowHandler handler(interface, &context);
        receiver.installEventFilter(&handler); keys(&receiver);
        QCOMPARE(receiver.presses, (QVector<int>{Qt::Key_Space, Qt::Key_Up, Qt::Key_Down}));
        QCOMPARE(receiver.releases, receiver.presses); QVERIFY(dispatched.isEmpty());
        for (Qt::Key code : {Qt::Key_Space, Qt::Key_Up, Qt::Key_Down}) context.sendHotkey(code, Qt::NoModifier);
        for (int code : QVector<int>{' ', KEY_UP, KEY_DOWN}) context.sendVLCHotkey(code);
        QVERIFY(dispatched.isEmpty());
        // Removal restores the unchanged native dispatch policy immediately.
        provider->visible = false; keys(&receiver);
        context.sendVLCHotkey(KEY_LEFT);
        QCOMPARE(dispatched, (QVector<int64_t>{' ', KEY_UP, KEY_DOWN, KEY_LEFT}));
        QCOMPARE(receiver.presses.size(), 3); QCOMPARE(receiver.releases.size(), 3);
        compositor.focus = nullptr;
    }
    void outsideModalQtNavigationPolicyRemains() {
        auto *provider = Singleton<DialogsProvider>::getInstance(); provider->visible = false;
        dispatched.clear(); KeyReceiver receiver; compositor.focus = &receiver;
        MainCtx context(interface); InterfaceWindowHandler handler(interface, &context);
        receiver.installEventFilter(&handler);
        for (Qt::Key code : {Qt::Key_Tab, Qt::Key_Backtab}) {
            key(&receiver, QEvent::KeyPress, code); key(&receiver, QEvent::KeyRelease, code);
        }
        QVERIFY(dispatched.isEmpty()); QCOMPARE(receiver.presses.size(), 2); QCOMPARE(receiver.releases.size(), 2);
        context.preferred = false; keys(&receiver);
        QVERIFY(dispatched.isEmpty()); QCOMPARE(receiver.presses.size(), 5); QCOMPARE(receiver.releases.size(), 5);
        compositor.focus = nullptr;
    }
    void cleanupTestCase() {
        if (interface) {
            var_DelCallback(vlc_object_instance(interface), "key-pressed", nativeKey, this);
            var_Destroy(vlc_object_instance(interface), "key-pressed");
            vlc_object_delete(VLC_OBJECT(interface));
        }
        Singleton<DialogsProvider>::killInstance();
        if (player) libvlc_media_player_release(player);
        if (instance) libvlc_release(instance);
    }
};

// Detached metadata/key receivers use only QObject and synchronous key-event
// delivery. This fixture does not construct a window, controls, focus scene,
// render graph or QGuiApplication; packaged UI acceptance remains separate.
QTEST_GUILESS_MAIN(TestSharedModalHotkeys)
#include "test_shared_modal_hotkeys.moc"
