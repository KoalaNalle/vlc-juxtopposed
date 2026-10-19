// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "qt.hpp"
#include "maininterface/compositor.hpp"
#include "maininterface/player_profile_preferences.hpp"
#include <vlc/vlc.h>
#include <vlc_configuration.h>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QProcess>
#include <QSet>
#include <QTemporaryDir>
#include <QTest>
#include <memory>
#include <limits>
#ifdef _WIN32
# include <windows.h>
#endif
extern "C" const char vlc_module_name[] = "player_profile_preferences";
extern "C" vlc_object_t *vlc_test_general_instance_object(libvlc_instance_t *);
namespace {
QByteArray bytes(const QString &path) { QFile file(path); return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray{}; }
bool write(const QString &path, const QByteArray &data)
{ QFile file(path); return file.open(QIODevice::WriteOnly) && file.write(data) == data.size(); }
void put(QVariantMap &snapshot, const char *group, const char *key, const QVariant &value)
{ auto fields = snapshot.value(QLatin1String(group)).toMap(); fields[QLatin1String(key)] = value; snapshot[QLatin1String(group)] = fields; }
struct NativeSession {
    libvlc_instance_t *instance = nullptr;
    qt_intf_t *intf = nullptr;
    std::unique_ptr<PlayerProfilePreferences> preferences;
    bool open(const QString &path) {
        const QByteArray config = QByteArray("--config=") + path.toUtf8();
        const char *args[] = {"--no-ignore-config", "--no-media-library", "--extraintf=", "--control=",
            "--no-metadata-network-access", "--quiet", config.constData()};
        instance = libvlc_new(ARRAY_SIZE(args), args); if (!instance) return false;
        intf = vlc_object_create<qt_intf_t>(vlc_test_general_instance_object(instance)); if (!intf) return false;
        preferences = std::make_unique<PlayerProfilePreferences>(intf); return true;
    }
    void close() {
        preferences.reset(); if (intf) {vlc_object_delete(intf); intf = nullptr;}
        if (instance) {libvlc_release(instance); instance = nullptr;}
    }
    ~NativeSession() { close(); }
};
}

class TestPlayerProfilePreferences final : public QObject
{
    Q_OBJECT
    QTemporaryDir temporary;
    QString root, path;
    NativeSession native;
    QVariantMap capture() {
        QVariantMap values; QString error;
        if (!native.preferences->snapshot(values, error)) QTest::qFail(qPrintable(error), __FILE__, __LINE__);
        return values;
    }
private slots:
    void initTestCase() {
        QVERIFY(temporary.isValid()); root = qEnvironmentVariable("VLC_PROFILE_DEFAULTS_OUTPUT");
        if (root.isEmpty()) root = temporary.path();
        else {QVERIFY(QFileInfo(root).isAbsolute()); QVERIFY(!QFileInfo::exists(root)); QVERIFY(QDir().mkpath(root));}
        const auto userdata = QDir(root).filePath(QStringLiteral("userdata")); QVERIFY(QDir().mkpath(userdata));
        qputenv("VLC_USERDATA_PATH", userdata.toUtf8());
    }
    void init() {
        const auto folder = QDir(root).filePath(QString::fromLatin1(QTest::currentTestFunction()));
        QVERIFY(QDir().mkpath(folder)); path = QDir(folder).filePath(QStringLiteral("vlcrc"));
        QVERIFY(native.open(path));
    }
    void cleanup() { native.close(); }
    void registryIsDisjointAndExcludesGlobalOptions() {
        QSet<QString> keys;
        for (const auto &group : PlayerProfilePreferences::groups()) {
            const auto fields = PlayerProfilePreferences::nativeKeys(group); QVERIFY(!fields.isEmpty());
            for (const auto &key : fields) {QVERIFY2(!keys.contains(key), qPrintable(key)); keys.insert(key);}
        }
        for (const auto *key : {"http-proxy", "http-proxy-pwd", "metadata-network-access", "key-play-pause",
             "qt-interface-scale", "qt-privacy-ask", "sout", "extraintf", "intf", "lua-intf", "control"})
            QVERIFY2(!keys.contains(QLatin1String(key)), key);
        QCOMPARE(PlayerProfilePreferences::nativeKeys(QStringLiteral("privacy")).size(), 0);
        const auto snapshot = capture(); auto groups = PlayerProfilePreferences::groups(); groups.sort();
        QCOMPARE(snapshot.keys(), groups);
        for (const auto &group : snapshot.keys()) for (const auto &key : snapshot.value(group).toMap().keys())
            QVERIFY(PlayerProfilePreferences::nativeKeys(group).contains(key));
        QVERIFY(snapshot.value(QStringLiteral("playback")).toMap().contains(QStringLiteral("input-timeshift-granularity")));
        QVERIFY(!snapshot.value(QStringLiteral("video")).toMap().contains(QStringLiteral("input-timeshift-granularity")));
    }
    void captureAndNoOpDoNotWrite() {
        const auto snapshot = capture(); QVERIFY(!QFile::exists(path)); QVariantMap changes; QString error;
        QVERIFY2(native.preferences->prepare(snapshot, changes, error), qPrintable(error)); QVERIFY(changes.isEmpty());
        QVERIFY(native.preferences->apply(snapshot, error)); QVERIFY(!QFile::exists(path)); QCOMPARE(capture(), snapshot);
    }
    void completeGroupsSaveAndRestoreExactly() {
        const auto before = capture(); auto target = before; QString error;
        put(target, "playback", "repeat", true); put(target, "playback", "loop", true);
        put(target, "playback", "audio-track-id", QStringLiteral("native/custom-track-Ää"));
        put(target, "audio", "equalizer-preset", QStringLiteral("rock"));
        put(target, "audio", "equalizer-bands", QStringLiteral("1 2 3 4 5 4 3 2 1 0"));
        put(target, "audio", "equalizer-preamp", double(float(3.25)));
        put(target, "audio", "audio-filter", QStringLiteral("equalizer:compressor"));
        put(target, "video", "brightness", double(float(1.25)));
        put(target, "video", "video-filter", QStringLiteral("adjust{brightness=1.25}:sharpen"));
        put(target, "subtitles", "sub-text-style-relative-size", double(float(7.25)));
        put(target, "subtitles", "sub-text-style-pixel-size", double(float(26.5)));
        put(target, "subtitles", "sub-text-style-relative-outline", double(float(1.5)));
        put(target, "subtitles", "sub-text-style-pixel-outline", double(float(2.75)));
        put(target, "subtitles", "sub-text-style-font", QStringLiteral("Inter Ää 日本語"));
        const auto proxyBefore = config_GetInt("metadata-network-access");
        QVERIFY2(native.preferences->apply(target, error), qPrintable(error)); QCOMPARE(capture(), target);
        QVERIFY(bytes(path).contains("equalizer-bands=1 2 3 4 5 4 3 2 1 0"));
        QVERIFY2(native.preferences->apply(before, error), qPrintable(error)); QCOMPARE(capture(), before);
        QCOMPARE(config_GetInt("metadata-network-access"), proxyBefore);
    }
    void helperOverridesAndSentinelsSurvive() {
        const auto before = capture(); auto target = before; QString error;
        put(target, "playback", "dvd", QStringLiteral("D:\\")); put(target, "playback", "vcd", QStringLiteral("F:\\"));
        put(target, "playback", "cd-audio", QStringLiteral("G:\\"));
        put(target, "playback", "audio-track", -1); put(target, "playback", "audio-track-id", QStringLiteral("saved-id"));
        put(target, "subtitles", "osd-feedback-duration", -1);
        put(target, "subtitles", "freetype-shadow-opacity", 77);
        put(target, "video", "disable-screensaver", 2);
        QVERIFY2(native.preferences->apply(target, error), qPrintable(error)); QCOMPARE(capture(), target);
        QVERIFY(native.preferences->apply(before, error)); QCOMPARE(capture(), before);
    }
    void injectedGlobalGroupsAndKeysAreRejectedBeforeWriting() {
        const auto before = capture(); QString error; QVariantMap changes; auto target = before;
        target[QStringLiteral("privacy")] = QVariantMap{};
        QVERIFY(!native.preferences->prepare(target, changes, error)); QVERIFY(changes.isEmpty());
        target = before; put(target, "audio", "http-proxy-pwd", QStringLiteral("not-a-profile-field"));
        put(target, "playback", "repeat", true);
        QVERIFY(!native.preferences->apply(target, error)); QCOMPARE(capture(), before); QVERIFY(!QFile::exists(path));
    }
    void missingFieldsAndChangedTypesAreRejected() {
        const auto before = capture(); QString error; auto target = before;
        auto fields = target.value(QStringLiteral("audio")).toMap(); fields.remove(QStringLiteral("audio")); target[QStringLiteral("audio")] = fields;
        QVERIFY(!native.preferences->apply(target, error)); target = before;
        put(target, "audio", "audio", 1); QVERIFY(!native.preferences->apply(target, error)); target = before;
        put(target, "playback", "audio-track", QStringLiteral("1")); QVERIFY(!native.preferences->apply(target, error));
        target = before; put(target, "audio", "audio-language", QStringLiteral("fi\nsv"));
        QVERIFY(!native.preferences->apply(target, error)); QCOMPARE(capture(), before); QVERIFY(!QFile::exists(path));
    }
    void rangeAndPrecisionFailuresCannotPartlyApply() {
        const auto before = capture(); QString error; auto target = before;
        put(target, "playback", "repeat", true); put(target, "video", "brightness", double(float(5.)));
        QVERIFY(!native.preferences->apply(target, error)); QCOMPARE(capture(), before); QVERIFY(!QFile::exists(path));
        target = before; put(target, "playback", "repeat", true);
        put(target, "video", "brightness", double(float(1.1234567)));
        QVERIFY(!native.preferences->apply(target, error)); QCOMPARE(capture(), before); QVERIFY(!QFile::exists(path));
        target = before; put(target, "video", "brightness", std::numeric_limits<double>::infinity());
        QVERIFY(!native.preferences->apply(target, error)); target = before;
        put(target, "video", "brightness", 1.1); QVERIFY(!native.preferences->apply(target, error));
        QCOMPARE(capture(), before);
    }
    void nonRestorableSourceIsRejectedBeforeSwitch() {
        config_PutFloat("brightness", float(1.1234567)); const auto before = capture();
        auto target = before; put(target, "video", "brightness", 1.0); QString error; QVariantMap changes;
        QVERIFY(!native.preferences->prepare(target, changes, error)); QVERIFY(changes.isEmpty()); QCOMPARE(capture(), before);
        // Unchanged advanced precision is retained without a needless rewrite.
        QVERIFY(native.preferences->apply(before, error)); QVERIFY(!QFile::exists(path));
    }
    void jsonRoundTripRetainsNativeFloatValues() {
        const auto before = capture(); const auto encoded = QJsonDocument::fromVariant(before).toJson(QJsonDocument::Compact);
        const auto decoded = QJsonDocument::fromJson(encoded).toVariant().toMap(); QCOMPARE(decoded, before);
        QString error; QVariantMap changes; QVERIFY2(native.preferences->prepare(decoded, changes, error), qPrintable(error));
        QVERIFY(changes.isEmpty()); QVERIFY(native.preferences->apply(decoded, error)); QVERIFY(!QFile::exists(path));
    }
    void physicalSaveFailureRestoresAllMemoryAndRetries() {
        const auto before = capture(); auto target = before; QString error;
        put(target, "playback", "repeat", true); QVERIFY(native.preferences->apply(target, error));
        const auto accepted = capture(); const auto acceptedBytes = bytes(path);
#ifdef _WIN32
        const auto handle = CreateFileW(reinterpret_cast<LPCWSTR>(path.utf16()), GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        QVERIFY(handle != INVALID_HANDLE_VALUE);
        target = before; put(target, "video", "brightness", 1.25);
        const bool saved = native.preferences->apply(target, error); CloseHandle(handle);
        QVERIFY(!saved); QVERIFY(!error.isEmpty()); QCOMPARE(capture(), accepted); QCOMPARE(bytes(path), acceptedBytes);
        QVERIFY2(native.preferences->apply(target, error), qPrintable(error)); QCOMPARE(capture(), target);
#else
        QFAIL("This Windows-native fixture requires the real file sharing/rename failure.");
#endif
    }
    void savedDefaultsSurviveColdProcess() {
        auto target = capture(); put(target, "playback", "repeat", true);
        put(target, "audio", "equalizer-bands", QStringLiteral("0 1 2 3 4 5 4 3 2 1"));
        put(target, "subtitles", "sub-text-style-font", QStringLiteral("Inter Ää 日本語"));
        QString error; QVERIFY2(native.preferences->apply(target, error), qPrintable(error));
        const auto expected = path + QStringLiteral(".expected.json"); QVERIFY(write(expected, QJsonDocument::fromVariant(target).toJson()));
        QProcess child;
#ifdef _WIN32
        child.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *args) {args->flags |= CREATE_NO_WINDOW;});
#endif
        child.start(QCoreApplication::applicationFilePath(), {QStringLiteral("--owned-profile-defaults-restart"), path, expected});
        QVERIFY(child.waitForStarted(10000)); QVERIFY(child.waitForFinished(60000)); QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QVERIFY2(child.exitCode() == 0, child.readAllStandardError().constData()); QCOMPARE(capture(), target);
    }
};

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    if (app.arguments().value(1) == QStringLiteral("--owned-profile-defaults-restart")) {
        NativeSession session; if (!session.open(app.arguments().value(2))) return 61;
        QVariantMap actual; QString error;
        if (!session.preferences->snapshot(actual, error)) return 62;
        const auto expected = QJsonDocument::fromJson(bytes(app.arguments().value(3))).toVariant().toMap();
        return actual == expected ? 0 : 63;
    }
    TestPlayerProfilePreferences test; return QTest::qExec(&test, argc, argv);
}
#include "test_player_profile_preferences.moc"
