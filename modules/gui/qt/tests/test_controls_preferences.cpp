// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "maininterface/controls_preferences_model.hpp"
#include "qt.hpp"
#include "maininterface/compositor.hpp"
#include <vlc/vlc.h>
#include <vlc_configuration.h>
#include <vlc_actions.h>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QTest>
#include <memory>
#include <limits>
#ifdef _WIN32
# include <windows.h>
#endif
extern "C" vlc_object_t *vlc_test_library_object(libvlc_instance_t *);

class TestControlsPreferences final : public QObject
{
    Q_OBJECT
    QTemporaryDir temp;
    libvlc_instance_t *instance = nullptr;
    qt_intf_t *intf = nullptr;
    std::unique_ptr<ControlsPreferencesModel> model;
    QString config, caseDir;
    unsigned editors = 0, remoteSetups = 0;
    static QByteArray bytes(const QString &name)
    { QFile file(name); return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray{}; }
    void open(bool mouse = true, bool remote = true)
    {
        const QList<QByteArray> args = {QByteArray("--config=") + config.toUtf8(), "--no-ignore-config", "--quiet",
            "--no-media-library", "--no-metadata-network-access", "--no-auto-preparse",
            "--extraintf=", "--control=", "--aout=dummy", "--vout=dummy"};
        QList<const char *> argv; for (const auto &arg : args) argv.append(arg.constData());
        instance = libvlc_new(argv.size(), argv.data()); QVERIFY(instance);
        auto *root = vlc_test_library_object(instance);
        intf = vlc_object_create<qt_intf_t>(root); QVERIFY(intf);
        // Controller seam only. Actual click/wheel consumers are independently
        // exercised through real window events by test/src/interface/controls.c.
        var_Create(root, "mouse-click-policy-version", VLC_VAR_INTEGER);
        var_SetInteger(root, "mouse-click-policy-version", 1);
        ControlsPreferencesModel::Consumers consumers;
        consumers.mousePolicy = mouse;
        consumers.remotePolicy = remote;
        consumers.openShortcutEditor = [this](ControlsPreferencesModel *) { ++editors; };
        consumers.openRemoteSetup = [this](ControlsPreferencesModel *) { ++remoteSetups; };
        model = std::make_unique<ControlsPreferencesModel>(intf, std::move(consumers));
    }
    void close()
    {
        model.reset();
        if (intf) { vlc_object_delete(intf); intf = nullptr; }
        if (instance) { libvlc_release(instance); instance = nullptr; }
    }
private slots:
    void init()
    {
        QVERIFY(temp.isValid());
        caseDir = QDir(temp.path()).filePath(QString::fromLatin1(QTest::currentTestFunction()));
        QVERIFY(QDir().mkpath(caseDir)); config = QDir(caseDir).filePath("vlcrc");
        const auto userdata = QDir(caseDir).filePath("userdata"); QVERIFY(QDir().mkpath(userdata));
        qputenv("VLC_USERDATA_PATH", userdata.toUtf8());
#ifdef _WIN32
        SetEnvironmentVariableW(L"VLC_USERDATA_PATH", reinterpret_cast<LPCWSTR>(userdata.utf16()));
#endif
        editors = remoteSetups = 0; open();
    }
    void cleanup() { close(); }
    void initialContractDoesNotWrite()
    {
        QCOMPARE(model->values().size(), 18);
        QVERIFY(model->values().value("linkWheelSettings").toBool());
        QCOMPARE(model->values().value("veryShortSkip").toInt(), 3);
        QCOMPARE(model->values().value("shortcutsPreset").toString(), QStringLiteral("Defaults"));
        QVERIFY(model->capabilities().value("singleClick").toBool());
        QVERIFY(model->capabilities().value("remoteScrollWheel").toBool());
        QVERIFY(model->capabilities().value("webRemoteEnabled").toBool());
        QVERIFY(!model->restartRequired()); QVERIFY(!QFile::exists(config));
        QVERIFY(model->resetShortcuts()); QVERIFY(!QFile::exists(config));
    }
    void unavailableConsumerDoesNotSave()
    {
        close(); open(false, false);
        QVERIFY(!model->capabilities().value("singleClick").toBool());
        QVERIFY(!model->setValue("singleClick", 1));
        QVERIFY(!model->setValue("webRemoteEnabled", true));
        QVERIFY(!QFile::exists(config));
        QVERIFY(model->capabilities().value("veryShortSkip").toBool());
    }
    void typedValidationPreservesStateAndDisk()
    {
        QVERIFY(model->setValue("shortSkip", 19));
        const auto disk = bytes(config); const auto before = model->values();
        const QList<QPair<QString, QVariant>> invalid = {
            {"veryShortSkip", QStringLiteral("3")}, {"veryShortSkip", true}, {"veryShortSkip", -1},
            {"veryShortSkip", 3.5}, {"veryShortSkip", 86401},
            {"veryShortSkip", std::numeric_limits<double>::infinity()},
            {"veryShortSkip", std::numeric_limits<double>::quiet_NaN()},
            {"singleClick", 3}, {"doubleClick", -1}, {"scrollWheel", 1},
            {"reverseScroll", 1}, {"reverseScroll", QStringLiteral("false")},
            {"remoteReverseScroll", 1}, {"linkWheelSettings", 1}, {"remotePort", 0}, {"remotePort", 65536},
            {"remotePort", 80.5}, {"remotePort", QStringLiteral("8080")},
            {"webRemoteEnabled", 1}, {"rate", 1}, {"missing", false}};
        for (const auto &edit : invalid) {
            QVERIFY(!model->setValue(edit.first, edit.second));
            QVERIFY(!model->lastError().isEmpty());
            QCOMPARE(model->values(), before); QCOMPARE(bytes(config), disk);
        }
    }
    void liveValuesPersistAndPublishAfterSave()
    {
        QVERIFY(model->setValue("veryShortSkip", 0));
        QVERIFY(model->setValue("longSkip", 86400));
        QVERIFY(model->setValue("singleClick", 2));
        QVERIFY(model->setValue("doubleClick", 0));
        auto *root = vlc_test_library_object(instance);
        QCOMPARE(var_GetInteger(root, "extrashort-jump-size"), 0);
        QCOMPARE(var_GetInteger(root, "video-singleclick-action"), 2);
        QVERIFY(!model->restartRequired());
        close(); open();
        QCOMPARE(model->values().value("veryShortSkip").toInt(), 0);
        QCOMPARE(model->values().value("longSkip").toInt(), 86400);
        QCOMPARE(model->values().value("singleClick").toInt(), 2);
        QCOMPARE(model->values().value("doubleClick").toInt(), 0);
        QVERIFY(!model->restartRequired());
    }
    void wheelEditClearsOnlyConflictingPlainLocalBindings()
    {
        config_PutPsz("key-play-pause", "Space\tMouse Wheel Up\tMouse Wheel Right\tCtrl+Mouse Wheel Down");
        config_PutPsz("global-key-play-pause", "Alt+F11");
        QVERIFY(model->setValue("horizontalScroll", 2));
        QCOMPARE(model->shortcutValues().value("key-play-pause").toString(), QStringLiteral("Space\tMouse Wheel Up\tCtrl+Mouse Wheel Down"));
        QVERIFY(model->setValue("reverseScroll", true));
        const auto keys = model->shortcutValues();
        QCOMPARE(keys.value("key-play-pause").toString(), QStringLiteral("Space\tCtrl+Mouse Wheel Down"));
        QCOMPARE(keys.value("global-key-play-pause").toString(), QStringLiteral("Alt+F11"));
        QVERIFY(model->restartRequired());
        QCOMPARE(config_GetInt("hotkeys-wheel-reverse"), 1);
        close(); open();
        QVERIFY(model->values().value("reverseScroll").toBool());
        QVERIFY(!model->restartRequired());
    }
    void legacyDisabledPauseRemainsDisabledUntilExplicitEdit()
    {
        config_PutPsz("mouse-pause-policy", "disabled");
        QCOMPARE(config_SaveConfigFile(intf), 0);
        close(); open();
        QCOMPARE(model->values().value("singleClick").toInt(), 0);
        auto *root = vlc_test_library_object(instance);
        char *before = var_GetString(root, "mouse-pause-policy");
        QCOMPARE(QByteArray(before), QByteArray("disabled")); free(before);
        QVERIFY(model->setValue("singleClick", 1));
        QCOMPARE(model->values().value("singleClick").toInt(), 1);
        char *after = var_GetString(root, "mouse-pause-policy");
        QCOMPARE(QByteArray(after), QByteArray("auto")); free(after);
        QVERIFY(!model->restartRequired());
    }
    void shortcutSnapshotValidationAndConflictScope()
    {
        auto keys = model->shortcutValues();
        auto invalid = keys; invalid.remove(invalid.firstKey());
        QVERIFY(!model->applyShortcuts(invalid));
        invalid = keys; invalid.remove(invalid.firstKey()); invalid.insert("rate", QStringLiteral("1"));
        QVERIFY(!model->applyShortcuts(invalid));
        invalid = keys; invalid["key-play-pause"] = QStringLiteral("invalid-key-name");
        QVERIFY(!model->applyShortcuts(invalid));
        invalid["key-play-pause"] = QStringLiteral("Mouse Wheel Up");
        QVERIFY(!model->applyShortcuts(invalid));
        invalid["key-play-pause"] = QStringLiteral("Ctrl+Alt+Shift+F11\tCtrl+Alt+Shift+F11");
        QVERIFY(!model->applyShortcuts(invalid));
        invalid["key-play-pause"] = QStringLiteral("Ctrl+Alt+Shift+F11");
        invalid["key-vol-up"] = QStringLiteral("Ctrl+Alt+Shift+F11");
        QVERIFY(!model->applyShortcuts(invalid));
        QVERIFY(!QFile::exists(config));
        keys["key-play-pause"] = QStringLiteral("Ctrl+Alt+Shift+F11");
        keys["global-key-play-pause"] = QStringLiteral("Ctrl+Alt+Shift+F11");
        QVERIFY(model->applyShortcuts(keys));
        QVERIFY(model->restartRequired());
        QCOMPARE(model->values().value("shortcutsPreset").toString(), QStringLiteral("Custom"));
        close(); open(); QCOMPARE(model->shortcutValues(), keys);
        QVERIFY(!model->restartRequired());
    }
    void resetIsScopedAndReversible()
    {
        const auto defaults = model->shortcutValues();
        QVERIFY(model->setValue("shortSkip", 19));
        config_PutFloat("rate", 1.25f);
        auto keys = defaults; keys["key-play-pause"] = QStringLiteral("Ctrl+Alt+Shift+F11");
        QVERIFY(model->applyShortcuts(keys));
        QVERIFY(model->resetShortcuts());
        QCOMPARE(model->shortcutValues(), defaults);
        QCOMPARE(config_GetInt("short-jump-size"), 19);
        QCOMPARE(config_GetFloat("rate"), 1.25f);
        QVERIFY(!model->restartRequired());
        close(); open(); QCOMPARE(model->shortcutValues(), defaults);
        QCOMPARE(config_GetInt("short-jump-size"), 19); QCOMPARE(config_GetFloat("rate"), 1.25f);
    }
    void editorOwnershipBlocksCompetingEdits()
    {
        QVERIFY(model->editShortcuts()); QVERIFY(model->editorActive()); QCOMPARE(editors, 1u);
        QVERIFY(!model->editShortcuts()); QCOMPARE(editors, 1u);
        QVERIFY(!model->setValue("shortSkip", 20)); QVERIFY(!model->resetShortcuts());
        QVERIFY(model->applyShortcuts(model->shortcutValues()));
        model->shortcutEditorClosed(); QVERIFY(!model->editorActive());
        QVERIFY(model->setValue("shortSkip", 20));
    }
    void failedSaveRollsBackNativeAndLiveThenRetries()
    {
#ifdef _WIN32
        QVERIFY(model->setValue("shortSkip", 19));
        const auto disk = bytes(config); const auto before = model->values();
        auto *root = vlc_test_library_object(instance);
        HANDLE handle = CreateFileW(reinterpret_cast<LPCWSTR>(config.utf16()), GENERIC_READ,
            FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        QVERIFY(handle != INVALID_HANDLE_VALUE);
        const bool accepted = model->setValue("shortSkip", 20);
        CloseHandle(handle);
        QVERIFY(!accepted); QCOMPARE(config_GetInt("short-jump-size"), 19);
        QCOMPARE(var_GetInteger(root, "short-jump-size"), 19);
        QCOMPARE(model->values(), before); QCOMPARE(bytes(config), disk);
        QVERIFY(!model->lastError().isEmpty());
        QVERIFY(model->setValue("shortSkip", 20)); QVERIFY(model->lastError().isEmpty());
        QCOMPARE(config_GetInt("short-jump-size"), 20); QCOMPARE(var_GetInteger(root, "short-jump-size"), 20);
#else
        QSKIP("Windows sharing rejection is tested on the target platform.");
#endif
    }
    void independentRemoteWheelSavesPublishWithoutRestart()
    {
        QVERIFY(model->setValue("linkWheelSettings", false));
        const auto physicalY = config_GetInt("hotkeys-y-wheel-mode");
        const auto physicalX = config_GetInt("hotkeys-x-wheel-mode");
        QVERIFY(model->setValue("remoteScrollWheel", 2));
        QVERIFY(model->setValue("remoteHorizontalScroll", -1));
        QVERIFY(model->setValue("remoteReverseScroll", true));
        auto *root = vlc_test_library_object(instance);
        QCOMPARE(var_GetInteger(root, "http-wheel-y-mode"), 2);
        QCOMPARE(var_GetInteger(root, "http-wheel-x-mode"), -1);
        QVERIFY(var_GetBool(root, "http-wheel-reverse"));
        QCOMPARE(config_GetInt("hotkeys-y-wheel-mode"), physicalY);
        QCOMPARE(config_GetInt("hotkeys-x-wheel-mode"), physicalX);
        QVERIFY(!model->restartRequired());
        close(); open();
        QCOMPARE(model->values().value("remoteScrollWheel").toInt(), 2);
        QVERIFY(model->values().value("remoteReverseScroll").toBool());
    }
    void linkedWheelsAliasEditsAndPreserveIndependentSettings()
    {
        auto *root = vlc_test_library_object(instance);
        const auto activeY = var_GetInteger(root, "hotkeys-y-wheel-mode");
        QVERIFY(var_GetBool(root, "http-wheel-linked"));
        QVERIFY(model->setValue("linkWheelSettings", false));
        QVERIFY(model->setValue("remoteScrollWheel", -1));
        QVERIFY(model->setValue("remoteHorizontalScroll", 3));
        QVERIFY(model->setValue("remoteReverseScroll", true));
        QVERIFY(model->setValue("linkWheelSettings", true));
        QCOMPARE(model->applyModes().value("remoteScrollWheel").toString(), QStringLiteral("restart"));
        QVERIFY(model->setValue("remoteScrollWheel", 2));
        QCOMPARE(model->values().value("scrollWheel").toInt(), 2);
        QCOMPARE(config_GetInt("hotkeys-y-wheel-mode"), 2);
        QCOMPARE(var_GetInteger(root, "hotkeys-y-wheel-mode"), activeY);
        QCOMPARE(config_GetInt("http-wheel-y-mode"), -1);
        QVERIFY(model->restartRequired());
        QVERIFY(model->setValue("horizontalScroll", -1));
        QCOMPARE(model->values().value("remoteHorizontalScroll").toInt(), -1);
        QVERIFY(model->setValue("remoteReverseScroll", true));
        QVERIFY(model->values().value("reverseScroll").toBool());
        QVERIFY(model->setValue("linkWheelSettings", false));
        QCOMPARE(model->values().value("remoteScrollWheel").toInt(), -1);
        QCOMPARE(model->values().value("remoteHorizontalScroll").toInt(), 3);
        QVERIFY(model->values().value("remoteReverseScroll").toBool());
        QCOMPARE(var_GetInteger(root, "http-wheel-y-mode"), -1);
        QCOMPARE(model->applyModes().value("remoteScrollWheel").toString(), QStringLiteral("live"));
        QVERIFY(model->setValue("linkWheelSettings", true));
        close(); open();
        QVERIFY(model->values().value("linkWheelSettings").toBool());
        QCOMPARE(model->values().value("remoteScrollWheel").toInt(), 2);
        QCOMPARE(var_GetInteger(vlc_test_library_object(instance), "hotkeys-y-wheel-mode"), 2);
        QVERIFY(!model->restartRequired());
        QVERIFY(model->setValue("linkWheelSettings", false));
        QCOMPARE(model->values().value("remoteScrollWheel").toInt(), -1);
    }
    void failedLinkSaveKeepsActivePolicyAndRetries()
    {
#ifdef _WIN32
        QVERIFY(model->setValue("shortSkip", 19));
        auto *root = vlc_test_library_object(instance);
        const auto before = model->values(); const auto disk = bytes(config);
        HANDLE handle = CreateFileW(reinterpret_cast<LPCWSTR>(config.utf16()), GENERIC_READ,
            FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        QVERIFY(handle != INVALID_HANDLE_VALUE);
        const bool accepted = model->setValue("linkWheelSettings", false);
        CloseHandle(handle);
        QVERIFY(!accepted); QVERIFY(var_GetBool(root, "http-wheel-linked"));
        QCOMPARE(config_GetInt("http-wheel-linked"), 1);
        QCOMPARE(model->values(), before); QCOMPARE(bytes(config), disk);
        QVERIFY(model->setValue("linkWheelSettings", false));
        QVERIFY(!var_GetBool(root, "http-wheel-linked"));
        QVERIFY(!model->values().value("linkWheelSettings").toBool());
        QVERIFY(!model->restartRequired());
#else
        QSKIP("Windows sharing rejection is tested on the target platform.");
#endif
    }
    void remoteSetupValidationCancellationAndPrivateCredentials()
    {
        QVERIFY(model->setValue("webRemoteEnabled", true));
        QVERIFY(model->remoteSetupActive()); QVERIFY(model->busy()); QCOMPARE(remoteSetups, 1u);
        QVERIFY(!model->values().value("webRemoteEnabled").toBool());
        QVERIFY(!model->editShortcuts()); QVERIFY(!model->setValue("remotePort", 18080));
        QVERIFY(!model->applyRemoteSetup(QStringLiteral("short")));
        QVERIFY(!model->applyRemoteSetup(QStringLiteral("owned\npassword")));
        QVERIFY(!QFile::exists(config)); model->remoteSetupClosed();
        QVERIFY(!model->busy()); QVERIFY(!QFile::exists(config));
        QVERIFY(model->setValue("webRemoteEnabled", true)); QCOMPARE(remoteSetups, 2u);
        const auto secret = QStringLiteral("owned-model-test-password");
        QVERIFY(model->applyRemoteSetup(secret)); QVERIFY(!model->busy());
        QVERIFY(model->values().value("webRemoteEnabled").toBool());
        QVERIFY(model->values().value("remotePasswordConfigured").toBool());
        QVERIFY(model->restartRequired());
        QVERIFY(!QJsonDocument::fromVariant(model->values()).toJson().contains(secret.toUtf8()));
        QVERIFY(!model->status().contains(secret)); QVERIFY(!model->lastError().contains(secret));
        char *host = config_GetPsz("http-host"); QCOMPARE(QByteArray(host), QByteArray("127.0.0.1")); free(host);
        close(); open(); QVERIFY(model->values().value("webRemoteEnabled").toBool());
        QVERIFY(!model->restartRequired());
    }
    void remoteActivationPreservesOtherInterfaceBlocksAndScopedReset()
    {
        const auto before = QStringLiteral("logger{path='E:/owned:logs'}:luahttp:rc{host=\"127.0.0.1:1234\"}");
        config_PutPsz("extraintf", before.toUtf8().constData());
        config_PutPsz("control", "luaintf{intf=http}:dummy{marker='owned:keep'}");
        config_PutPsz("http-password", "owned-existing-password");
        model->reload(); QVERIFY(model->values().value("webRemoteEnabled").toBool());
        QVERIFY(model->setValue("webRemoteEnabled", false));
        char *extra = config_GetPsz("extraintf");
        QCOMPARE(QByteArray(extra), QByteArray("logger{path='E:/owned:logs'}:rc{host=\"127.0.0.1:1234\"}")); free(extra);
        char *control = config_GetPsz("control"); QCOMPARE(QByteArray(control), QByteArray("dummy{marker='owned:keep'}")); free(control);
        QVERIFY(model->setValue("webRemoteEnabled", true));
        extra = config_GetPsz("extraintf"); const QByteArray enabled(extra); free(extra);
        QVERIFY(enabled.endsWith(":http"));
        QVERIFY(model->resetShortcuts());
        extra = config_GetPsz("extraintf"); QCOMPARE(QByteArray(extra), enabled); free(extra);
        char *password = config_GetPsz("http-password"); QCOMPARE(QByteArray(password), QByteArray("owned-existing-password")); free(password);
    }
    void remoteListenerRequiresHttpsOutsideLocalhost()
    {
        config_PutPsz("http-host", "0.0.0.0");
        QVERIFY(model->setValue("webRemoteEnabled", true));
        QVERIFY(!model->applyRemoteSetup(QStringLiteral("owned-loopback-password")));
        QVERIFY(model->remoteSetupActive()); QVERIFY(!QFile::exists(config));
        QVERIFY(!model->values().value("webRemoteEnabled").toBool());
        model->remoteSetupClosed();
        config_PutPsz("http-host", "::1");
        QVERIFY(model->setValue("webRemoteEnabled", true));
        QVERIFY(model->applyRemoteSetup(QStringLiteral("owned-loopback-password")));
        QVERIFY(model->setValue("remotePort", 18080));
        QCOMPARE(config_GetInt("http-port"), 18080);
    }
    void remoteActivationFailureRollsBackCredentialAndRetries()
    {
#ifdef _WIN32
        QVERIFY(model->setValue("remotePort", 18080));
        QVERIFY(model->setValue("webRemoteEnabled", true));
        const auto disk = bytes(config);
        HANDLE handle = CreateFileW(reinterpret_cast<LPCWSTR>(config.utf16()), GENERIC_READ,
            FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        QVERIFY(handle != INVALID_HANDLE_VALUE);
        const bool accepted = model->applyRemoteSetup(QStringLiteral("owned-rollback-password"));
        CloseHandle(handle);
        QVERIFY(!accepted); QVERIFY(model->remoteSetupActive()); QCOMPARE(bytes(config), disk);
        char *password = config_GetPsz("http-password"); QVERIFY(!password || !*password); free(password);
        char *extra = config_GetPsz("extraintf"); QVERIFY(!extra || !*extra); free(extra);
        QVERIFY(!model->values().value("webRemoteEnabled").toBool());
        QVERIFY(model->applyRemoteSetup(QStringLiteral("owned-rollback-password")));
        QVERIFY(!model->busy()); QVERIFY(model->values().value("webRemoteEnabled").toBool());
#else
        QSKIP("Windows sharing rejection is tested on the target platform.");
#endif
    }
};
int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    TestControlsPreferences test;
    return QTest::qExec(&test, argc, argv);
}
#include "test_controls_preferences.moc"
