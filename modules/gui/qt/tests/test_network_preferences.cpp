// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "maininterface/network_preferences_model.hpp"
#include "qt.hpp"
#include "maininterface/compositor.hpp"
#include <vlc/vlc.h>
#include <vlc_configuration.h>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <memory>
#include <limits>
#ifdef _WIN32
# include <windows.h>
#endif
extern "C" vlc_object_t *vlc_test_library_object(libvlc_instance_t *);

class TestNetworkPreferences final : public QObject
{
    Q_OBJECT
    QTemporaryDir temp;
    libvlc_instance_t *instance = nullptr;
    qt_intf_t *intf = nullptr;
    std::unique_ptr<NetworkPreferencesModel> model;
    QString config, directory;
    static QByteArray bytes(const QString &name)
    { QFile file(name); return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray{}; }
    void open(bool available = true)
    {
        const QList<QByteArray> args = {QByteArray("--config=") + config.toUtf8(), "--no-ignore-config", "--quiet",
            "--no-media-library", "--no-metadata-network-access", "--no-auto-preparse", "--extraintf=", "--control="};
        QList<const char *> argv; for (const auto &arg : args) argv.append(arg.constData());
        instance = libvlc_new(argv.size(), argv.data()); QVERIFY(instance);
        intf = vlc_object_create<qt_intf_t>(vlc_test_library_object(instance)); QVERIFY(intf);
        // These flags enable the real controller mapping, not simulated native
        // TLS/caching consumers. The separate native suites verify those paths.
        model = std::make_unique<NetworkPreferencesModel>(intf,
            NetworkPreferencesModel::Consumers{available, available, available, available});
    }
    void close()
    {
        model.reset(); if (intf) { vlc_object_delete(intf); intf = nullptr; }
        if (instance) { libvlc_release(instance); instance = nullptr; }
    }
    void legacyProxy(const char *value, const char *password)
    {
        config_PutPsz("http-proxy", value); config_PutPsz("http-proxy-pwd", password);
        QCOMPARE(config_SaveConfigFile(intf), 0); close(); open();
    }
    static QString native(const char *key)
    { char *raw = config_GetPsz(key); const auto value = QString::fromUtf8(raw ? raw : ""); free(raw); return value; }
private slots:
    void init()
    {
        QVERIFY(temp.isValid()); directory = QDir(temp.path()).filePath(QString::fromLatin1(QTest::currentTestFunction()));
        QVERIFY(QDir().mkpath(directory)); config = QDir(directory).filePath("vlcrc");
        const auto userdata = QDir(directory).filePath("userdata"); QVERIFY(QDir().mkpath(userdata));
        qputenv("VLC_USERDATA_PATH", userdata.toUtf8());
#ifdef _WIN32
        SetEnvironmentVariableW(L"VLC_USERDATA_PATH", reinterpret_cast<LPCWSTR>(userdata.utf16()));
#endif
        open();
    }
    void cleanup() { close(); }
    void openingKeepsActualNativeDefaultsWithoutWrites()
    {
        QCOMPARE(model->values().size(), 10);
        QCOMPARE(model->values().value("localFiles").toInt(), 1000);
        QCOMPARE(model->values().value("buffering").toInt(), 0);
        QCOMPARE(model->values().value("cipherPreference").toString(), QStringLiteral("NORMAL"));
        QVERIFY(model->values().value("systemTrust").toBool());
        QVERIFY(model->capabilities().value("localFiles").toBool());
        QVERIFY(!model->capabilities().value("httpProxyPassword").toBool());
        QVERIFY(!QFile::exists(config)); model->reload(); QVERIFY(!QFile::exists(config));
    }
    void unavailableConsumersCannotSaveOrEmitAdvanced()
    {
        close(); open(false); QSignalSpy advanced(model.get(), &NetworkPreferencesModel::advancedOptionsRequested);
        for (const auto &key : model->capabilities().keys()) QVERIFY(!model->capabilities().value(key).toBool());
        QVERIFY(!model->setValue("buffering", 300)); QVERIFY(!model->setValue("systemTrust", false));
        QVERIFY(!model->setValue("httpProxy", QStringLiteral("direct://"))); QVERIFY(!model->openAdvanced());
        QCOMPARE(advanced.count(), 0); QVERIFY(!QFile::exists(config));
    }
    void nativePresetUsesAllFourRatiosAndCustomOnlyEnablesEditing()
    {
        QVERIFY(model->setValue("buffering", 300));
        QCOMPARE(config_GetInt("file-caching"), 300); QCOMPARE(config_GetInt("disc-caching"), 300);
        QCOMPARE(config_GetInt("live-caching"), 300); QCOMPARE(config_GetInt("network-caching"), 1000);
        QVERIFY(!model->capabilities().value("localFiles").toBool());
        QVERIFY(!model->setValue("localFiles", 450));
        const auto disk = bytes(config);
        QVERIFY(model->setValue("buffering", 0)); QCOMPARE(bytes(config), disk);
        QVERIFY(model->capabilities().value("localFiles").toBool());
        QVERIFY(model->setValue("localFiles", 450)); QCOMPARE(config_GetInt("file-caching"), 450);
        QCOMPARE(config_GetInt("disc-caching"), 300); QCOMPARE(config_GetInt("network-caching"), 1000);
        close(); open(); QCOMPARE(model->values().value("buffering").toInt(), 0);
        QCOMPARE(model->values().value("localFiles").toInt(), 450);
    }
    void presetAndMillisecondBoundsPersist()
    {
        for (const auto preset : {100, 200, 300, 500, 1000}) {
            QVERIFY(model->setValue("buffering", preset));
            QCOMPARE(model->values().value("buffering").toInt(), preset);
            QCOMPARE(config_GetInt("network-caching"), preset * 10 / 3);
        }
        QVERIFY(model->setValue("buffering", 0));
        QVERIFY(model->setValue("discs", 0)); QVERIFY(model->setValue("liveCapture", 60000));
        close(); open(); QCOMPARE(model->values().value("discs").toInt(), 0);
        QCOMPARE(model->values().value("liveCapture").toInt(), 60000);
    }
    void typedValidationRetainsStateAndDisk()
    {
        QVERIFY(model->setValue("localFiles", 400)); const auto before = model->values(); const auto disk = bytes(config);
        const QList<QPair<QString,QVariant>> edits = {{"localFiles", true}, {"localFiles", QStringLiteral("400")},
            {"localFiles", -1}, {"localFiles", 60001}, {"localFiles", 0.5},
            {"localFiles", std::numeric_limits<double>::quiet_NaN()},
            {"localFiles", std::numeric_limits<double>::infinity()}, {"buffering", 450},
            {"systemTrust", 0}, {"cipherPreference", 0}, {"cipherPreference", QStringLiteral("CUSTOM")},
            {"httpProxy", true}, {"missing", 1}};
        for (const auto &edit : edits) {
            QVERIFY(!model->setValue(edit.first, edit.second)); QVERIFY(!model->lastError().isEmpty());
            QCOMPARE(model->values(), before); QCOMPARE(bytes(config), disk);
        }
    }
    void failedPresetSaveRollsBackAllFourThenRetries()
    {
#ifdef _WIN32
        QVERIFY(model->setValue("buffering", 300)); QVERIFY(model->setValue("buffering", 0));
        const auto before = model->values(); const auto disk = bytes(config);
        HANDLE lock = CreateFileW(reinterpret_cast<LPCWSTR>(config.utf16()), GENERIC_READ, FILE_SHARE_READ,
            nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr); QVERIFY(lock != INVALID_HANDLE_VALUE);
        const bool accepted = model->setValue("buffering", 500); CloseHandle(lock);
        QVERIFY(!accepted); QCOMPARE(model->values(), before); QCOMPARE(bytes(config), disk);
        QCOMPARE(config_GetInt("file-caching"), 300); QCOMPARE(config_GetInt("network-caching"), 1000);
        QVERIFY(model->setValue("buffering", 500)); QVERIFY(model->lastError().isEmpty());
        QCOMPARE(model->values().value("buffering").toInt(), 500);
#else
        QSKIP("Windows sharing rejection is tested on the target platform.");
#endif
    }
    void embeddedSavedPasswordNeverEntersPublicState()
    {
        legacyProxy("http://owned-user:owned-embedded-secret@proxy.owned.invalid:8080", "owned-separate-secret");
        const auto disk = bytes(config); const auto state = QJsonDocument::fromVariant(model->values()).toJson();
        QVERIFY(!state.contains("owned-embedded-secret")); QVERIFY(!state.contains("owned-separate-secret"));
        QVERIFY(model->values().value("proxyPasswordConfigured").toBool());
        QCOMPARE(model->values().value("httpProxy").toString(), QStringLiteral("http://owned-user@proxy.owned.invalid:8080"));
        model->reload(); QCOMPARE(bytes(config), disk); QVERIFY(model->lastError().isEmpty());
    }
    void malformedLegacyProxyStaysExplicitAndOpaque()
    {
        legacyProxy("broken:owned-secret@proxy.invalid:invalid", "");
        const auto disk = bytes(config);
        QVERIFY(model->values().value("proxyAddressCustom").toBool());
        QVERIFY(model->values().value("httpProxy").toString().isEmpty());
        QVERIFY(!QJsonDocument::fromVariant(model->values()).toJson().contains("owned-secret"));
        model->reload(); QCOMPARE(bytes(config), disk);
    }
    void sameAuthorityMigratesEmbeddedSecretAndDifferentAuthorityClearsIt()
    {
        legacyProxy("http://owned-user:owned-secret@proxy.owned.invalid:80", "");
        QVERIFY(model->setValue("httpProxy", QStringLiteral("http://owned-user@proxy.owned.invalid")));
        QCOMPARE(native("http-proxy-pwd"), QStringLiteral("owned-secret"));
        QVERIFY(!native("http-proxy").contains(QStringLiteral("owned-secret")));
        QVERIFY(model->setValue("httpProxy", QStringLiteral("http://owned-user@other.owned.invalid")));
        QVERIFY(native("http-proxy-pwd").isEmpty());
        QVERIFY(!model->values().value("proxyPasswordConfigured").toBool());
    }
    void explicitPasswordSaveClearAndReopen()
    {
        QVERIFY(model->setValue("httpProxy", QStringLiteral("https://owned-user@proxy.owned.invalid:8443")));
        QVERIFY(model->capabilities().value("httpProxyPassword").toBool());
        QVERIFY(model->setValue("httpProxyPassword", QStringLiteral("owned-password")));
        QVERIFY(!QJsonDocument::fromVariant(model->values()).toJson().contains("owned-password"));
        close(); open(); QVERIFY(model->values().value("proxyPasswordConfigured").toBool());
        QVERIFY(model->setValue("httpProxyPassword", QString{}));
        QVERIFY(!model->values().value("proxyPasswordConfigured").toBool());
        QCOMPARE(native("http-proxy"), QStringLiteral("https://owned-user@proxy.owned.invalid:8443"));
    }
    void invalidProxyAndPasswordRetainState()
    {
        QVERIFY(model->setValue("httpProxy", QStringLiteral("http://owned-user@proxy.owned.invalid")));
        QVERIFY(model->setValue("httpProxyPassword", QStringLiteral("owned-password")));
        const auto before = model->values(); const auto disk = bytes(config);
        for (const auto &value : {QStringLiteral("ftp://proxy.invalid"), QStringLiteral("http://user:secret@proxy.invalid"),
            QStringLiteral("http://user:@proxy.invalid"), QStringLiteral("http://proxy.invalid:70000"), QStringLiteral("http://proxy.invalid/path"),
            QStringLiteral("http://proxy.invalid/?x=1"), QStringLiteral("http://proxy.invalid/#x"), QStringLiteral("http://proxy.invalid\n")}) {
            QVERIFY(!model->setValue("httpProxy", value)); QCOMPARE(model->values(), before); QCOMPARE(bytes(config), disk);
        }
        QVERIFY(!model->setValue("httpProxyPassword", true));
        QVERIFY(!model->setValue("httpProxyPassword", QStringLiteral("line\nbreak")));
        QVERIFY(!model->setValue("httpProxyPassword", QString(1025, QLatin1Char('x'))));
        QCOMPARE(model->values(), before); QCOMPARE(bytes(config), disk);
    }
    void failedProxySaveKeepsAddressAndCredentialThenRetries()
    {
#ifdef _WIN32
        legacyProxy("http://owned-user:owned-secret@proxy.owned.invalid", "");
        const auto before = model->values(); const auto disk = bytes(config);
        HANDLE lock = CreateFileW(reinterpret_cast<LPCWSTR>(config.utf16()), GENERIC_READ, FILE_SHARE_READ,
            nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr); QVERIFY(lock != INVALID_HANDLE_VALUE);
        const bool accepted = model->setValue("httpProxy", QStringLiteral("http://other-user@other.owned.invalid")); CloseHandle(lock);
        QVERIFY(!accepted); QCOMPARE(model->values(), before); QCOMPARE(bytes(config), disk);
        QCOMPARE(native("http-proxy"), QStringLiteral("http://owned-user:owned-secret@proxy.owned.invalid"));
        QVERIFY(model->setValue("httpProxy", QStringLiteral("http://other-user@other.owned.invalid")));
        QVERIFY(!model->values().value("proxyPasswordConfigured").toBool());
#else
        QSKIP("Windows sharing rejection is tested on the target platform.");
#endif
    }
    void directAndSystemKeepPasswordUnavailable()
    {
        QVERIFY(model->setValue("httpProxy", QStringLiteral("DIRECT://")));
        QCOMPARE(native("http-proxy"), QStringLiteral("direct://"));
        QVERIFY(!model->capabilities().value("httpProxyPassword").toBool());
        QVERIFY(!model->setValue("httpProxyPassword", QStringLiteral("owned-password")));
        QVERIFY(model->setValue("httpProxy", QString{})); QVERIFY(native("http-proxy").isEmpty());
        QVERIFY(!model->values().value("proxyAddressCustom").toBool());
        QVERIFY(!model->capabilities().value("httpProxyPassword").toBool());
    }
    void tlsNativeChoicesPersistAndCustomValueIsPreserved()
    {
        const auto choices = model->choices().value("cipherPreference").toList(); QCOMPARE(choices.size(), 4);
        for (const auto &entry : choices) QVERIFY(model->setValue("cipherPreference", entry.toMap().value("value")));
        QVERIFY(model->setValue("systemTrust", false)); close(); open();
        QVERIFY(!model->values().value("systemTrust").toBool());
        config_PutPsz("gnutls-priorities", "NORMAL:+VERS-TLS1.2"); QCOMPARE(config_SaveConfigFile(intf), 0);
        close(); open(); const auto disk = bytes(config);
        QCOMPARE(model->values().value("cipherPreference").toString(), QStringLiteral("NORMAL:+VERS-TLS1.2"));
        model->reload(); QCOMPARE(bytes(config), disk);
    }
    void advancedActionDoesNotSave()
    {
        QSignalSpy advanced(model.get(), &NetworkPreferencesModel::advancedOptionsRequested);
        QVERIFY(model->openAdvanced()); QCOMPARE(advanced.count(), 1); QVERIFY(!QFile::exists(config));
    }
};
int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv); TestNetworkPreferences test; return QTest::qExec(&test, argc, argv);
}
#include "test_network_preferences.moc"
