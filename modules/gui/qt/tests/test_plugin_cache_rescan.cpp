// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "util/plugin_cache_rescan.hpp"
#include <vlc_common.h>
#include <vlc_modules.h>
#include <vlc/vlc.h>
#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QDataStream>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QThread>
#include <QUrl>
#include <atomic>
#ifdef _WIN32
# include <qt_windows.h>
#endif

extern "C" vlc_object_t *vlc_test_library_object(libvlc_instance_t *);

class TestPluginCacheRescan : public QObject
{
    Q_OBJECT
    QTemporaryDir m_directory;
    libvlc_instance_t *m_instance = nullptr;
    vlc_object_t *m_object = nullptr;
    QString m_runtime, m_template, m_trace, m_root;
    std::atomic_uint m_buffers{0};
    static void audio(void *opaque, const void *, unsigned, int64_t)
    { ++static_cast<TestPluginCacheRescan *>(opaque)->m_buffers; }
    QByteArray bytes(const QString &path) const
    { QFile f(path); if (!f.open(QIODevice::ReadOnly)) return {}; return f.readAll(); }
    bool write(const QString &path, const QByteArray &data) const
    { QFile f(path); return f.open(QIODevice::WriteOnly) && f.write(data) == data.size() && f.flush(); }
    QString cache(const QString &root) const { return QDir(root).filePath("plugins.dat"); }
    QString fakeRoot(const QString &name)
    {
        const auto root = m_directory.filePath(name);
        if (!QDir().mkpath(root) || !write(QDir(root).filePath("owned.dll"), "owned synthetic fixture")
            || !write(cache(root), "preserve this cache")) return {};
        return root;
    }
    void mode(const char *value, const QString &root)
    {
        m_trace = m_directory.filePath("child-trace.txt");
        QFile::remove(m_trace);
        qputenv("JUXTO_CACHE_FIXTURE_MODE", value);
        qputenv("JUXTO_CACHE_FIXTURE_TRACE", m_trace.toUtf8());
        qputenv("JUXTO_CACHE_FIXTURE_ROOT", root.toUtf8());
        qputenv("JUXTO_CACHE_FIXTURE_TEMPLATE", m_template.toUtf8());
    }
    PluginCacheRescan::Layout fake(const QString &root) const
    { return {QCoreApplication::applicationFilePath(), root}; }
    void failure(const char *behavior, const QString &name)
    {
        const auto root = fakeRoot(name); QVERIFY(!root.isEmpty());
        const auto old = bytes(cache(root)); mode(behavior, root);
        PluginCacheRescan scan(m_object, fake(root), {});
        QSignalSpy done(&scan, &PluginCacheRescan::finished);
        QVERIFY(scan.start()); QVERIFY(!scan.start());
        QTRY_COMPARE_WITH_TIMEOUT(done.count(), 1, 10000);
        QVERIFY(!done[0][0].toBool()); QVERIFY(!scan.busy());
        QVERIFY(!scan.lastError().isEmpty()); QCOMPARE(bytes(cache(root)), old);
        QVERIFY(!bytes(m_trace).isEmpty());
        if (QByteArray(behavior) == "change-plugin")
            QCOMPARE(bytes(QDir(root).filePath("owned.dll")), QByteArray("changed independent plugin image"));
    }
private slots:
    void initTestCase()
    {
        QVERIFY(m_directory.isValid());
        m_runtime = qEnvironmentVariable("JUXTO_CACHE_RUNTIME");
        QVERIFY(QFileInfo(QDir(m_runtime).filePath("vlc-cache-gen.exe")).isFile());
        const char *args[] = {"--ignore-config", "--no-media-library", "--no-metadata-network-access",
                             "--extraintf=", "--control=", "--quiet"};
        m_instance = libvlc_new(ARRAY_SIZE(args), args); QVERIFY(m_instance);
        m_object = vlc_test_library_object(m_instance); QVERIFY(m_object);
    }
    void unavailableAndFailedStart()
    {
        PluginCacheRescan missing(m_object, {"", ""}, {});
        QVERIFY(!missing.available()); QVERIFY(!missing.start()); QVERIFY(!missing.busy());
        const auto root = fakeRoot("bad executable"); QVERIFY(!root.isEmpty());
        const auto executable = m_directory.filePath("not-a-program.exe");
        QVERIFY(write(executable, "invalid executable"));
        PluginCacheRescan bad(m_object, {executable, root}, {});
        QSignalSpy done(&bad, &PluginCacheRescan::finished);
        QVERIFY(bad.start()); QTRY_COMPARE_WITH_TIMEOUT(done.count(), 1, 5000);
        QVERIFY(!done[0][0].toBool()); QCOMPARE(bytes(cache(root)), QByteArray("preserve this cache"));
    }
    void realGeneratorAndNativeParser()
    {
        m_root = m_directory.filePath(QString::fromUtf8("real 'quoted, \xc3\xa4/plugins"));
        QVERIFY(QDir().mkpath(m_root));
        const auto source = QDir(m_runtime).filePath("plugins");
        QDirIterator it(source, {"*.dll"}, QDir::Files, QDirIterator::Subdirectories);
        unsigned copied = 0;
        while (it.hasNext()) {
            const auto from = it.next();
            const auto to = QDir(m_root).filePath(QDir(source).relativeFilePath(from));
            QVERIFY(QDir().mkpath(QFileInfo(to).absolutePath()));
#ifdef _WIN32
            QVERIFY(CreateHardLinkW(reinterpret_cast<LPCWSTR>(to.utf16()),
                                   reinterpret_cast<LPCWSTR>(from.utf16()), nullptr));
#else
            QVERIFY(QFile::copy(from, to));
#endif
            ++copied;
        }
        QVERIFY(copied > 100);
        QVERIFY(write(cache(m_root), "old cache sentinel"));
        const auto wavePath = m_directory.filePath("primary.wav");
        QFile wave(wavePath); QVERIFY(wave.open(QIODevice::WriteOnly));
        QDataStream wav(&wave); wav.setByteOrder(QDataStream::LittleEndian);
        constexpr quint32 dataBytes = 20 * 44100 * 2;
        wav.writeRawData("RIFF", 4); wav << quint32(36 + dataBytes);
        wav.writeRawData("WAVEfmt ", 8); wav << quint32(16) << quint16(1) << quint16(1)
            << quint32(44100) << quint32(88200) << quint16(2) << quint16(16);
        wav.writeRawData("data", 4); wav << dataBytes;
        QCOMPARE(wave.write(QByteArray(dataBytes, '\0')), qint64(dataBytes)); wave.close();
        std::unique_ptr<libvlc_media_player_t, decltype(&libvlc_media_player_release)> player(
            libvlc_media_player_new(m_instance, nullptr, nullptr), libvlc_media_player_release);
        QVERIFY(player);
        libvlc_audio_set_callbacks(player.get(), audio, nullptr, nullptr, nullptr, nullptr, this);
        libvlc_audio_set_format(player.get(), "FL32", 44100, 1);
        const auto uri = QUrl::fromLocalFile(wavePath).toEncoded();
        auto *media = libvlc_media_new_location(uri.constData()); QVERIFY(media);
        libvlc_media_player_set_media(player.get(), media); libvlc_media_release(media);
        QCOMPARE(libvlc_media_player_play(player.get()), 0);
        QTRY_VERIFY_WITH_TIMEOUT(m_buffers > 3, 5000);
        const auto playingBuffers = m_buffers.load();
        size_t beforeCount = 0; module_t **before = module_list_get(&beforeCount);
        QVERIFY(before); module_list_free(before);
        PluginCacheRescan scan(m_object, {QDir(m_runtime).filePath("vlc-cache-gen.exe"), m_root}, {});
        QSignalSpy done(&scan, &PluginCacheRescan::finished);
        QVERIFY(scan.start()); QTRY_COMPARE_WITH_TIMEOUT(done.count(), 1, 30000);
        QVERIFY2(done[0][0].toBool(), qPrintable(scan.lastError()));
        QTRY_VERIFY_WITH_TIMEOUT(m_buffers > playingBuffers + 3, 5000);
        QVERIFY(bytes(cache(m_root)).size() > 1000);
        QVERIFY(vlc_module_cache_validate(m_object, m_root.toUtf8().constData()));
        libvlc_media_player_set_pause(player.get(), 1);
        QTRY_COMPARE_WITH_TIMEOUT(libvlc_media_player_get_state(player.get()), libvlc_Paused, 5000);
        const auto pausedTime = libvlc_media_player_get_time(player.get());
        QVERIFY(scan.start()); QTRY_COMPARE_WITH_TIMEOUT(done.count(), 2, 30000);
        QVERIFY2(done[1][0].toBool(), qPrintable(scan.lastError()));
        QCOMPARE(libvlc_media_player_get_time(player.get()), pausedTime);
        libvlc_media_player_stop_async(player.get());
        QTRY_COMPARE_WITH_TIMEOUT(libvlc_media_player_get_state(player.get()), libvlc_Stopped, 5000);
        // A cache-only child must still see Qt's metadata while its owned DLL
        // link is absent. Scanning the filesystem would discard that entry.
        const auto gui = QDir(m_root).filePath("gui/libqt_plugin.dll");
        QVERIFY(QFile::remove(gui));
        QProcess reload;
        reload.start(QCoreApplication::applicationFilePath(), {"--check-cache", m_root});
        const bool stopped = reload.waitForFinished(10000);
        if (!stopped) { reload.kill(); reload.waitForFinished(3000); }
        QVERIFY(stopped); QCOMPARE(reload.exitStatus(), QProcess::NormalExit); QCOMPARE(reload.exitCode(), 0);
#ifdef _WIN32
        const auto originalGui = QDir(m_runtime).filePath("plugins/gui/libqt_plugin.dll");
        QVERIFY(CreateHardLinkW(reinterpret_cast<LPCWSTR>(gui.utf16()),
                               reinterpret_cast<LPCWSTR>(originalGui.utf16()), nullptr));
#endif
        size_t afterCount = 0; module_t **after = module_list_get(&afterCount);
        QVERIFY(after); module_list_free(after); QCOMPARE(afterCount, beforeCount);
        m_template = m_directory.filePath("valid-template.dat");
        QVERIFY(QFile::copy(cache(m_root), m_template));
        const auto good = bytes(m_template);
        const auto corrupt = m_directory.filePath("corrupt"); QVERIFY(QDir().mkpath(corrupt));
        for (auto length : {qsizetype(0), qsizetype(8), good.size() - 7, good.size() - 1}) {
            QVERIFY(write(cache(corrupt), good.first(length)));
            QVERIFY(!vlc_module_cache_validate(m_object, corrupt.toUtf8().constData()));
        }
        auto bad = good; bad[0] = 'X'; QVERIFY(write(cache(corrupt), bad));
        QVERIFY(!vlc_module_cache_validate(m_object, corrupt.toUtf8().constData()));
        QVERIFY(vlc_module_cache_validate(m_object, m_root.toUtf8().constData()));
    }
    void generatorReportsOutputFailure()
    {
        const auto invalid = m_directory.filePath("not a directory");
        QVERIFY(write(invalid, "preserve this output sibling"));
        const auto original = bytes(cache(m_root));
        QProcess generator;
        generator.start(QDir(m_runtime).filePath("vlc-cache-gen.exe"), {"--output", invalid, m_root});
        const bool stopped = generator.waitForFinished(10000);
        if (!stopped) { generator.kill(); generator.waitForFinished(3000); }
        QVERIFY(stopped); QCOMPARE(generator.exitStatus(), QProcess::NormalExit);
        QVERIFY(generator.exitCode() != 0);
        QCOMPARE(bytes(cache(m_root)), original);
        QCOMPARE(bytes(invalid), QByteArray("preserve this output sibling"));
    }
    void noOutputIsFailure() { failure("empty", "empty"); }
    void corruptOutputIsFailure() { failure("invalid", "invalid"); }
    void abnormalExitIsFailure() { failure("fail", "failed"); }
    void lockedPublicationAndRetry()
    {
        const auto root = fakeRoot("locked"); QVERIFY(!root.isEmpty()); mode("copy", root);
        const auto old = bytes(cache(root));
#ifdef _WIN32
        HANDLE lock = CreateFileW(reinterpret_cast<LPCWSTR>(cache(root).utf16()), GENERIC_READ,
                                  FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        QVERIFY(lock != INVALID_HANDLE_VALUE);
        PluginCacheRescan scan(m_object, fake(root), {}); QSignalSpy done(&scan, &PluginCacheRescan::finished);
        const bool started = scan.start();
        QElapsedTimer timer; timer.start();
        while (done.isEmpty() && timer.elapsed() < 10000) QTest::qWait(10);
        CloseHandle(lock);
        QVERIFY(started); QCOMPARE(done.count(), 1); QVERIFY(!done[0][0].toBool());
        QCOMPARE(bytes(cache(root)), old);
        QVERIFY(scan.start()); QTRY_COMPARE_WITH_TIMEOUT(done.count(), 2, 10000);
        QVERIFY2(done[1][0].toBool(), qPrintable(scan.lastError()));
        QCOMPARE(bytes(cache(root)), bytes(m_template));
#else
        QFAIL("This is a Windows file-sharing integration test");
#endif
    }
    void timeoutReapsOwnedChild()
    {
        const auto root = fakeRoot("timeout"); QVERIFY(!root.isEmpty()); mode("hang", root);
        PluginCacheRescan scan(m_object, fake(root), {30000, 1000});
        QSignalSpy done(&scan, &PluginCacheRescan::finished);
        QVERIFY(scan.start());
        QTRY_VERIFY_WITH_TIMEOUT(!bytes(m_trace).isEmpty(), 5000);
#ifdef _WIN32
        HANDLE child = OpenProcess(SYNCHRONIZE, FALSE, bytes(m_trace).split('\n')[0].toUInt());
        QVERIFY(child);
        QElapsedTimer timer; timer.start();
        while (done.isEmpty() && timer.elapsed() < 5000) QTest::qWait(10);
        const auto terminated = WaitForSingleObject(child, 0); CloseHandle(child);
        QCOMPARE(done.count(), 1); QVERIFY(!done[0][0].toBool()); QCOMPARE(terminated, DWORD(WAIT_OBJECT_0));
#endif
        QVERIFY(scan.lastError().contains("timed out")); QCOMPARE(bytes(cache(root)), QByteArray("preserve this cache"));
    }
    void cancellationAndReuse()
    {
        const auto root = fakeRoot("cancel"); QVERIFY(!root.isEmpty()); mode("hang", root);
        PluginCacheRescan scan(m_object, fake(root), {}); QSignalSpy done(&scan, &PluginCacheRescan::finished);
        QVERIFY(scan.start()); QTRY_VERIFY_WITH_TIMEOUT(!bytes(m_trace).isEmpty(), 5000);
        scan.cancel(); QTRY_COMPARE_WITH_TIMEOUT(done.count(), 1, 5000);
        QVERIFY(!done[0][0].toBool()); QCOMPARE(bytes(cache(root)), QByteArray("preserve this cache"));
        mode("copy", root); QVERIFY(scan.start()); QTRY_COMPARE_WITH_TIMEOUT(done.count(), 2, 10000);
        QVERIFY2(done[1][0].toBool(), qPrintable(scan.lastError()));
    }
    void stalePluginIsRejected() { failure("change-plugin", "stale"); }
    void concurrentCacheIsPreserved()
    {
        const auto root = fakeRoot("concurrent"); QVERIFY(!root.isEmpty()); mode("change-cache", root);
        PluginCacheRescan scan(m_object, fake(root), {}); QSignalSpy done(&scan, &PluginCacheRescan::finished);
        QVERIFY(scan.start()); QTRY_COMPARE_WITH_TIMEOUT(done.count(), 1, 10000);
        QVERIFY(!done[0][0].toBool()); QCOMPARE(bytes(cache(root)), QByteArray("independent cache writer"));
    }
    void simultaneousRescanIsRejected()
    {
        const auto root = fakeRoot("simultaneous"); QVERIFY(!root.isEmpty()); mode("hang", root);
        PluginCacheRescan first(m_object, fake(root), {}), second(m_object, fake(root), {});
        QSignalSpy done(&first, &PluginCacheRescan::finished);
        QVERIFY(first.start()); QTRY_VERIFY_WITH_TIMEOUT(!bytes(m_trace).isEmpty(), 5000);
        QVERIFY(!second.start()); QVERIFY(!second.busy()); QVERIFY(!second.lastError().isEmpty());
        first.cancel(); QTRY_COMPARE_WITH_TIMEOUT(done.count(), 1, 5000);
        QVERIFY(!done[0][0].toBool()); QCOMPARE(bytes(cache(root)), QByteArray("preserve this cache"));
    }
    void cancelBeforeScannerStarts()
    {
        const auto root = fakeRoot("cancel preparing"); QVERIFY(!root.isEmpty()); mode("copy", root);
        PluginCacheRescan scan(m_object, fake(root), {}); QSignalSpy done(&scan, &PluginCacheRescan::finished);
        QVERIFY(scan.start()); scan.cancel();
        QTRY_COMPARE_WITH_TIMEOUT(done.count(), 1, 5000);
        QVERIFY(!done[0][0].toBool()); QVERIFY(!QFileInfo(m_trace).exists());
        QVERIFY(scan.start()); QTRY_COMPARE_WITH_TIMEOUT(done.count(), 2, 10000);
        QVERIFY2(done[1][0].toBool(), qPrintable(scan.lastError()));
    }
    void destructionReapsAndCleans()
    {
        const auto root = fakeRoot("shutdown"); QVERIFY(!root.isEmpty()); mode("hang", root);
        auto scan = std::make_unique<PluginCacheRescan>(m_object, fake(root), PluginCacheRescan::Limits{});
        QVERIFY(scan->start()); QTRY_VERIFY_WITH_TIMEOUT(!bytes(m_trace).isEmpty(), 5000);
        const auto trace = bytes(m_trace).split('\n'); QVERIFY(trace.size() >= 2);
#ifdef _WIN32
        HANDLE child = OpenProcess(SYNCHRONIZE, FALSE, trace[0].toUInt()); QVERIFY(child);
        scan.reset();
        const auto terminated = WaitForSingleObject(child, 0); CloseHandle(child);
        QCOMPARE(terminated, DWORD(WAIT_OBJECT_0));
#else
        scan.reset();
#endif
        QVERIFY(!QFileInfo(QString::fromUtf8(trace[1])).exists());
        QCOMPARE(bytes(cache(root)), QByteArray("preserve this cache"));
    }
    void cleanupTestCase()
    {
        qunsetenv("JUXTO_CACHE_FIXTURE_MODE");
        if (m_instance) libvlc_release(m_instance);
    }
};

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    if (app.arguments().size() == 3 && app.arguments()[1] == "--check-cache") {
        qputenv("VLC_LIB_PATH", app.arguments()[2].toUtf8());
        qunsetenv("VLC_PLUGIN_PATH");
        const char *args[] = {"--ignore-config", "--no-plugins-scan", "--no-media-library",
                             "--no-metadata-network-access", "--quiet"};
        auto *instance = libvlc_new(ARRAY_SIZE(args), args);
        if (!instance) return 10;
        const bool found = module_exists("qt") && module_exists("filesystem");
        libvlc_release(instance);
        return found ? 0 : 11;
    }
    const auto mode = qgetenv("JUXTO_CACHE_FIXTURE_MODE");
    if (!mode.isEmpty() && app.arguments().size() == 4 && app.arguments()[1] == "--output") {
        const auto stage = app.arguments()[2];
        QFile trace(qEnvironmentVariable("JUXTO_CACHE_FIXTURE_TRACE"));
        if (!trace.open(QIODevice::WriteOnly)) return 9;
        trace.write(QByteArray::number(QCoreApplication::applicationPid()) + '\n'
                    + QFileInfo(stage).absolutePath().toUtf8() + '\n'); trace.close();
        if (mode == "hang") { QThread::sleep(60); return 0; }
        if (mode == "fail") return 3;
        if (mode == "empty") return 0;
        QFile target(QDir(stage).filePath("plugins.dat"));
        if (mode == "invalid") {
            if (!target.open(QIODevice::WriteOnly)) return 5;
            target.write("invalid generated cache"); return 0;
        }
        if (!QFile::copy(qEnvironmentVariable("JUXTO_CACHE_FIXTURE_TEMPLATE"), target.fileName())) return 6;
        const auto root = qEnvironmentVariable("JUXTO_CACHE_FIXTURE_ROOT");
        if (mode == "change-plugin" || mode == "change-cache") {
            QFile changed(QDir(root).filePath(mode == "change-plugin" ? "owned.dll" : "plugins.dat"));
            if (!changed.open(QIODevice::WriteOnly)) return 7;
            changed.write(mode == "change-plugin" ? "changed independent plugin image" : "independent cache writer");
        }
        return 0;
    }
    TestPluginCacheRescan test;
    return QTest::qExec(&test, argc, argv);
}
#include <test_plugin_cache_rescan.moc>
