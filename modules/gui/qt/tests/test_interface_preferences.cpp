// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "maininterface/interface_preferences_model.hpp"
#include "qt.hpp"
#include "maininterface/compositor.hpp"
#include <vlc/vlc.h>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <limits>
#include <memory>
#ifdef _WIN32
# include <windows.h>
#endif
extern "C" vlc_object_t *vlc_test_general_instance_object(libvlc_instance_t *);

class TestInterfacePreferences : public QObject
{
    Q_OBJECT
    QTemporaryDir m_temp;
    QString m_output, m_case, m_config;
    libvlc_instance_t *m_instance = nullptr;
    qt_intf_t *m_intf = nullptr;
    std::unique_ptr<QSettings> m_settings;
    std::unique_ptr<InterfacePreferencesModel> m_model;
    static QByteArray bytes(const QString &path) {
        QFile file(path); return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray{};
    }
    static bool write(const QString &path, const QByteArray &data) {
        QFile file(path); return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
    }
    void closeNative() {
        m_model.reset();
        if (m_intf) { vlc_object_delete(m_intf); m_intf = nullptr; }
        if (m_instance) { libvlc_release(m_instance); m_instance = nullptr; }
    }
    bool openNative() {
        const QByteArray option = QByteArray("--config=") + m_config.toUtf8();
        const char *args[] = {"--no-ignore-config", "--no-media-library", "--quiet", option.constData()};
        m_instance = libvlc_new(ARRAY_SIZE(args), args);
        if (!m_instance) return false;
        m_intf = vlc_object_create<qt_intf_t>(vlc_test_general_instance_object(m_instance));
        return m_intf;
    }
    void makeModel(InterfacePreferencesModel::Consumers consumers = {}) {
        m_model = std::make_unique<InterfacePreferencesModel>(m_intf, m_settings.get(), consumers);
    }
private slots:
    void initTestCase() {
        QVERIFY(m_temp.isValid());
        m_output = qEnvironmentVariable("VLC_INTERFACE_OUTPUT");
        if (m_output.isEmpty()) m_output = m_temp.path();
        else {
            QVERIFY2(QFileInfo(m_output).isAbsolute(), "Explicit output must be an absolute fresh directory");
            QVERIFY2(!QFileInfo::exists(m_output), "Explicit output already exists; choose a fresh owned directory");
            QVERIFY(QDir().mkpath(m_output));
        }
        const auto userdata = QDir(m_output).filePath(QStringLiteral("userdata"));
        QVERIFY(QDir().mkpath(userdata)); qputenv("VLC_USERDATA_PATH", userdata.toUtf8());
    }
    void init() {
        m_case = QDir(m_output).filePath(QString::fromLatin1(QTest::currentTestFunction()));
        if (const auto tag = QTest::currentDataTag(); tag && *tag) m_case += '/' + QString::fromLatin1(tag);
        QVERIFY(QDir().mkpath(m_case));
        m_config = QDir(m_case).filePath(QStringLiteral("vlcrc"));
        m_settings = std::make_unique<QSettings>(QDir(m_case).filePath(QStringLiteral("qt.ini")), QSettings::IniFormat);
        QVERIFY(openNative()); makeModel();
        QVERIFY2(m_model->lastError().isEmpty(), qPrintable(m_model->lastError()));
    }
    void cleanup() { closeNative(); m_settings.reset(); }
    void exactApiAndHonestCapabilities() {
        QCOMPARE(m_model->values().size(), 18);
        QCOMPARE(m_model->capabilities().size(), 18);
        QCOMPARE(m_model->values().keys(), m_model->capabilities().keys());
        QVERIFY(!m_model->restartRequired());
        QVERIFY(!m_model->capabilities().value(QStringLiteral("playlistTree")).toBool());
        QVERIFY(!m_model->capabilities().value(QStringLiteral("fullscreenControls")).toBool());
        QVERIFY(!m_model->capabilities().value(QStringLiteral("fullscreenScreen")).toBool());
        for (const auto &key : {"rememberGeometry", "titleFileName", "controlAutoHide", "controlHideDelay", "popupDuration"})
            QVERIFY(!m_model->capabilities().value(QString::fromLatin1(key)).toBool());
        // Backend fixture is QCoreApplication: there is no fabricated screen
        // or tray availability and no Windows platform GUI startup required.
        QVERIFY(m_model->screens().isEmpty());
        QVERIFY(!m_model->capabilities().value(QStringLiteral("startMinimized")).toBool());
        QVERIFY(!m_model->chooseSkinFile()); QVERIFY(!m_model->pickerActive());
    }
    void existingNativeFlags_data() {
        QTest::addColumn<QString>("key"); QTest::addColumn<QByteArray>("native");
        QTest::newRow("embed") << QStringLiteral("embeddedVideo") << QByteArray("embedded-video");
        QTest::newRow("resize") << QStringLiteral("resizeVideo") << QByteArray("qt-video-autoresize");
        QTest::newRow("on-top") << QStringLiteral("alwaysOnTop") << QByteArray("video-on-top");
        QTest::newRow("decorations") << QStringLiteral("windowDecorations") << QByteArray("qt-titlebar");
        QTest::newRow("compact") << QStringLiteral("compactMode") << QByteArray("qt-minimal-view");
    }
    void existingNativeFlags() {
        QFETCH(QString, key); QFETCH(QByteArray, native);
        QCOMPARE(config_GetType(native.constData()), VLC_VAR_BOOL);
        QVERIFY2(m_model->capabilities().value(key).toBool(), qPrintable(key));
        const bool previous = config_GetInt(native.constData());
        const auto unrelated = config_GetInt("qt-max-volume");
        QSignalSpy values(m_model.get(), &InterfacePreferencesModel::valuesChanged);
        QVERIFY2(m_model->setValue(key, !previous), qPrintable(m_model->lastError()));
        QCOMPARE(bool(config_GetInt(native.constData())), !previous);
        QCOMPARE(m_model->values().value(key).toBool(), !previous);
        const QByteArray expected = native + '=' + (!previous ? '1' : '0');
        QVERIFY(bytes(m_config).contains(expected));
        QCOMPARE(config_GetInt("qt-max-volume"), unrelated);
        QCOMPARE(values.size(), 1); QVERIFY(m_model->restartRequired());
        QVERIFY(m_model->setValue(key, previous)); QVERIFY(!m_model->restartRequired());
        QVERIFY(m_model->lastError().isEmpty());
        // Save once more and create a genuine fresh native instance using the
        // same owned file, rather than proving only a shared config cache.
        QVERIFY(m_model->setValue(key, !previous));
        closeNative(); QVERIFY(openNative()); makeModel();
        QCOMPARE(bool(config_GetInt(native.constData())), !previous);
        QCOMPARE(m_model->values().value(key).toBool(), !previous);
        QVERIFY(!m_model->restartRequired());
    }
    void invalidAndUnsupportedWritesDoNotMutate() {
        const QList<QPair<QString, QVariant>> invalid = {
            {QStringLiteral("embeddedVideo"), 1}, {QStringLiteral("embeddedVideo"), QStringLiteral("false")},
            {QStringLiteral("resizeVideo"), 0.0}, {QStringLiteral("alwaysOnTop"), QStringLiteral("true")},
            {QStringLiteral("windowDecorations"), 1.0}, {QStringLiteral("compactMode"), 0},
            {QStringLiteral("playlistTree"), true}, {QStringLiteral("fullscreenControls"), false},
            {QStringLiteral("fullscreenScreen"), -1}, {QStringLiteral("controlAutoHide"), 2},
            {QStringLiteral("controlHideDelay"), 777}, {QStringLiteral("popupDuration"), 3000},
            {QStringLiteral("startMinimized"), true}, {QStringLiteral("unknown"), true}};
        const auto before = m_model->values(); const auto disk = bytes(m_config);
        QSignalSpy changed(m_model.get(), &InterfacePreferencesModel::valuesChanged);
        for (const auto &entry : invalid) {
            QVERIFY2(!m_model->setValue(entry.first, entry.second), qPrintable(entry.first));
            QVERIFY(!m_model->lastError().isEmpty());
            QCOMPARE(m_model->values(), before); QCOMPARE(bytes(m_config), disk);
            QVERIFY(!m_model->restartRequired());
        }
        QCOMPARE(changed.size(), 0);
        QVERIFY(m_model->setValue(QStringLiteral("compactMode"), before.value(QStringLiteral("compactMode"))));
        QVERIFY(m_model->lastError().isEmpty());
    }
    void consumerGatesAndIntegralMilliseconds() {
        // Requires Root's proposed native Qt options. This proves model/storage
        // only; separate actual QML/window/tray gates remain mandatory.
        QCOMPARE(config_GetType("qt-controls-auto-hide"), VLC_VAR_INTEGER);
        QCOMPARE(config_GetType("qt-controls-hide-timeout"), VLC_VAR_INTEGER);
        QCOMPARE(config_GetType("qt-remember-window-geometry"), VLC_VAR_BOOL);
        InterfacePreferencesModel::Consumers linked;
        linked.rememberGeometry = linked.titleFileName = linked.controlAutoHide = linked.controlHideDelay = true;
        makeModel(linked);
        for (const auto &key : {"rememberGeometry", "titleFileName", "controlAutoHide", "controlHideDelay"})
            QVERIFY(m_model->capabilities().value(QString::fromLatin1(key)).toBool());
        QVERIFY(m_model->setValue(QStringLiteral("controlHideDelay"), 777.0));
        QCOMPARE(config_GetInt("qt-controls-hide-timeout"), int64_t(777));
        QCOMPARE(m_model->values().value(QStringLiteral("controlHideDelay")).toInt(), 777);
        QVERIFY(m_model->restartRequired());
        const auto before = m_model->values(); const auto disk = bytes(m_config);
        for (const QVariant &bad : {QVariant(true), QVariant(QStringLiteral("777")), QVariant(-1), QVariant(10001),
            QVariant(777.5), QVariant(std::numeric_limits<double>::quiet_NaN()), QVariant(std::numeric_limits<double>::infinity())}) {
            QVERIFY(!m_model->setValue(QStringLiteral("controlHideDelay"), bad));
            QCOMPARE(m_model->values(), before); QCOMPARE(bytes(m_config), disk);
        }
        for (int mode = 0; mode != 3; ++mode) {
            QVERIFY(m_model->setValue(QStringLiteral("controlAutoHide"), double(mode)));
            QCOMPARE(config_GetInt("qt-controls-auto-hide"), int64_t(mode));
        }
        QVERIFY(!m_model->setValue(QStringLiteral("controlAutoHide"), -1));
        QVERIFY(!m_model->setValue(QStringLiteral("controlAutoHide"), 3));
    }
    void failedNativeDestinationIsNotAccepted() {
        QVERIFY(QDir().mkpath(m_config)); // Real existing directory at destination.
        const auto before = m_model->values();
        const bool saved = config_GetInt("qt-minimal-view");
        QSignalSpy changed(m_model.get(), &InterfacePreferencesModel::valuesChanged);
        QVERIFY(!m_model->setValue(QStringLiteral("compactMode"), !saved));
        QVERIFY(!m_model->lastError().isEmpty());
        QCOMPARE(bool(config_GetInt("qt-minimal-view")), saved);
        QCOMPARE(m_model->values(), before); QCOMPARE(changed.size(), 0);
        QVERIFY(QFileInfo(m_config).isDir()); QVERIFY(!m_model->restartRequired());
    }
    void skinPathAndForeignInterfacePreservation() {
        const auto candidate = QDir(m_case).filePath(QString::fromUtf8("Skin-\xE6\xB5\x8B\xE8\xAF\x95.xml"));
        const QByteArray selectedBytes("<Theme version=\"2.0\"></Theme>");
        QFile selected(candidate); QVERIFY(selected.open(QIODevice::WriteOnly));
        QCOMPARE(selected.write(selectedBytes), qint64(selectedBytes.size())); selected.close();
        const bool available = m_model->capabilities().value(QStringLiteral("skinFile")).toBool();
        const auto previous = m_model->values(); const auto disk = bytes(m_config);
        if (available) {
            QVERIFY2(m_model->setValue(QStringLiteral("skinFile"), candidate), qPrintable(m_model->lastError()));
            QCOMPARE(m_model->values().value(QStringLiteral("skinFile")).toString(), QDir::cleanPath(candidate));
            QVERIFY(m_model->restartRequired());
            char *saved = config_GetPsz("skins2-last");
            const auto savedText = QString::fromUtf8(saved ? saved : ""); free(saved);
            QCOMPARE(savedText, QDir::cleanPath(candidate));
            QCOMPARE(bytes(candidate), selectedBytes); // Selection never modifies the skin.
        } else {
            QVERIFY(!m_model->setValue(QStringLiteral("skinFile"), candidate));
            QCOMPARE(m_model->values(), previous); QCOMPARE(bytes(m_config), disk);
        }
        const auto beforeInvalid = m_model->values(); const auto diskBeforeInvalid = bytes(m_config);
        const QString loneSurrogate(QChar(0xd800));
        const QStringList invalid = {QStringLiteral("relative.vlt"), QStringLiteral("https://example.invalid/skin.vlt"),
            QDir(m_case).filePath(QStringLiteral("missing.vlt")), m_case, loneSurrogate,
            candidate + QChar(0), candidate + QChar('\n')};
        for (const auto &path : invalid) {
            QVERIFY(!m_model->setValue(QStringLiteral("skinFile"), path));
            QCOMPARE(m_model->values(), beforeInvalid); QCOMPARE(bytes(m_config), diskBeforeInvalid);
        }
        // Preserve an explicit third-party/custom interface list. The service
        // must not replace an unsupported preference merely to fit its switch.
        config_PutPsz("intf", "dummy,qt"); m_model->reload();
        QVERIFY(!m_model->capabilities().value(QStringLiteral("customSkin")).toBool());
        QVERIFY(!m_model->setValue(QStringLiteral("customSkin"), false));
        char *foreign = config_GetPsz("intf");
        const QByteArray foreignText(foreign ? foreign : ""); free(foreign);
        QCOMPARE(foreignText, QByteArray("dummy,qt"));
    }
    void lockedNativeDestinationPreservesAndRecovers() {
#ifdef _WIN32
        const bool initial = config_GetInt("qt-minimal-view");
        QVERIFY(m_model->setValue(QStringLiteral("compactMode"), initial));
        const auto original = bytes(m_config); QVERIFY(!original.isEmpty());
        QVERIFY(write(QDir(m_case).filePath(QStringLiteral("before-locked-save.bin")), original));
        const HANDLE handle = CreateFileW(reinterpret_cast<LPCWSTR>(m_config.utf16()), GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        QVERIFY(handle != INVALID_HANDLE_VALUE);
        const auto before = m_model->values();
        const bool accepted = m_model->setValue(QStringLiteral("compactMode"), !initial);
        const auto error = m_model->lastError(); const auto diskAfter = bytes(m_config);
        const bool nativeAfter = config_GetInt("qt-minimal-view");
        const auto valuesAfterFailure = m_model->values();
        CloseHandle(handle);
        QVERIFY(write(QDir(m_case).filePath(QStringLiteral("after-locked-save.bin")), diskAfter));
        // Owned handle is closed and the real retry is performed before final
        // desired assertions, even when a future regression violates rollback.
        m_model->reload(); const auto reloadError = m_model->lastError();
        const bool retry = m_model->setValue(QStringLiteral("compactMode"), !initial);
        const auto retryError = m_model->lastError();
        const QJsonObject trace{{"initialRejected", !accepted}, {"initialError", error},
            {"priorBytesPreserved", diskAfter == original}, {"nativeValuePreserved", nativeAfter == initial},
            {"modelValuesPreserved", valuesAfterFailure == before}, {"reloadError", reloadError},
            {"retryAttempted", true}, {"retryAccepted", retry}, {"retryError", retryError},
            {"postRetryDiskChanged", bytes(m_config) != original}};
        QVERIFY(write(QDir(m_case).filePath(QStringLiteral("locked-save-receipt.json")), QJsonDocument(trace).toJson()));
        qInfo().noquote() << "INTERFACE_NATIVE_SAVE" << QJsonDocument(trace).toJson(QJsonDocument::Compact);
        QVERIFY(!accepted); QVERIFY(!error.isEmpty()); QCOMPARE(diskAfter, original);
        QCOMPARE(nativeAfter, initial); QCOMPARE(valuesAfterFailure, before);
        QVERIFY2(reloadError.isEmpty(), qPrintable(reloadError));
        QVERIFY2(retry, qPrintable(retryError)); QVERIFY(retryError.isEmpty());
        QCOMPARE(bool(config_GetInt("qt-minimal-view")), !initial); QVERIFY(bytes(m_config) != original);
#else
        QSKIP("Uses actual Windows file sharing against an owned config");
#endif
    }
    void borrowedStorageLifetimeIsSafe() {
        m_settings.reset(); // QPointer must reject later writes safely.
        m_model->reload(); QVERIFY(!m_model->lastError().isEmpty());
        QVERIFY(!m_model->setValue(QStringLiteral("compactMode"), true));
        for (const auto &value : m_model->capabilities()) QVERIFY(!value.toBool());
    }
};
QTEST_GUILESS_MAIN(TestInterfacePreferences)
#include "test_interface_preferences.moc"
