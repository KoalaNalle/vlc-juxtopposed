// SPDX-License-Identifier: GPL-2.0-or-later
// Private native acceptance: real sender vlc.exe and real receiving Core/WinMsg.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <vlc_common.h>
#include <vlc/vlc.h>
#include <vlc_interface.h>
#include <vlc_input_item.h>
#include <vlc_playlist.h>
#include <vlc_player.h>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>
#include <windows.h>

extern "C" vlc_object_t *vlc_test_library_object(libvlc_instance_t *);

class TestLibraryOpeningProcess final : public QObject
{
    Q_OBJECT
    QString m_root;
    QString m_media, m_sender, m_config, m_work;
    libvlc_instance_t *m_instance = nullptr;
    intf_thread_t *m_intf = nullptr;
    QList<QByteArray> m_arguments;
    QStringList m_processEvidence;

    static QString uri(const QString &path)
    { return QString::fromUtf8(QUrl::fromLocalFile(path).toEncoded()); }
    static bool write(const QString &path, const QByteArray &bytes)
    { QFile file(path); return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size(); }
    vlc_playlist_t *playlist() const { return vlc_intf_GetMainPlaylist(m_intf); }
    QStringList queue() const
    {
        QStringList result;
        auto *pl = playlist();
        vlc_playlist_Lock(pl);
        for (size_t i = 0; i < vlc_playlist_Count(pl); ++i)
        {
            char *raw = input_item_GetURI(vlc_playlist_item_GetMedia(vlc_playlist_Get(pl, i)));
            result.append(QString::fromUtf8(raw));
            free(raw);
        }
        vlc_playlist_Unlock(pl);
        return result;
    }
    int state() const
    {
        auto *pl = playlist(); vlc_playlist_Lock(pl);
        const int value = vlc_player_GetState(vlc_playlist_GetPlayer(pl));
        vlc_playlist_Unlock(pl); return value;
    }
    qint64 clock() const
    {
        auto *pl = playlist(); vlc_playlist_Lock(pl);
        const auto value = vlc_player_GetTime(vlc_playlist_GetPlayer(pl));
        vlc_playlist_Unlock(pl); return value;
    }
    ssize_t selected() const
    {
        auto *pl = playlist(); vlc_playlist_Lock(pl);
        const auto value = vlc_playlist_GetCurrentIndex(pl);
        vlc_playlist_Unlock(pl); return value;
    }
    bool configure(bool master, int filePolicy, bool append)
    {
        return write(m_config, QByteArray("one-instance=") + (master ? "1" : "0")
            + "\none-instance-when-started-from-file=1\none-instance-file-manager-policy="
            + QByteArray::number(filePolicy) + "\nplaylist-open-mode=" + (append ? "1" : "0")
            + "\nmetadata-network-access=0\nmedia-library=0\n");
    }
    bool send(const QStringList &files, bool fromFileManager = false, bool separate = false)
    {
        QProcess process;
        process.setWorkingDirectory(m_work);
        auto env = QProcessEnvironment::systemEnvironment();
        env.insert(QStringLiteral("VLC_USERDATA_PATH"), QDir(m_root).filePath(QStringLiteral("child-data")));
        process.setProcessEnvironment(env);
        QStringList arguments = {QStringLiteral("--config=") + m_config, QStringLiteral("--no-ignore-config"),
            QStringLiteral("--intf=dummy"), QStringLiteral("--extraintf="), QStringLiteral("--control="),
            QStringLiteral("--no-media-library"), QStringLiteral("--no-auto-preparse"),
            QStringLiteral("--no-metadata-network-access"), QStringLiteral("--aout=dummy"),
            QStringLiteral("--vout=dummy"), QStringLiteral("--no-video"), QStringLiteral("--quiet")};
        if (fromFileManager) arguments.append(QStringLiteral("--started-from-file"));
        if (separate) arguments.append({QStringLiteral("--run-time=0.25"), QStringLiteral("--play-and-exit")});
        arguments.append(files);
        process.start(m_sender, arguments);
        if (!process.waitForStarted(5000)) return false;
        const auto pid = process.processId();
        if (!process.waitForFinished(15000))
        {
            m_processEvidence.append(QStringLiteral("Timed out sender pid %1").arg(pid));
            process.kill(); process.waitForFinished(5000);
            return false;
        }
        m_processEvidence.append(QStringLiteral("Sender pid %1 exited normally=%2 code=%3")
            .arg(pid).arg(process.exitStatus() == QProcess::NormalExit).arg(process.exitCode()));
        return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
    }
private slots:
    void initTestCase()
    {
        const auto output = qEnvironmentVariable("VLC_LIBRARY_PROCESS_OUTPUT");
        QVERIFY(QFileInfo(output).isAbsolute());
        m_root = QDir(output).filePath(QStringLiteral("owned"));
        QVERIFY(!QFileInfo::exists(m_root));
        QVERIFY(QDir().mkpath(m_root));
        m_media = qEnvironmentVariable("VLC_LIBRARY_MEDIA");
        m_sender = qEnvironmentVariable("VLC_LIBRARY_SENDER");
        QVERIFY(QFileInfo::exists(m_sender));
        m_work = QDir(m_root).filePath(QStringLiteral("different-目录"));
        QVERIFY(QDir().mkpath(m_work));
        for (const auto *name : {"one.wav", "two.wav"})
            QVERIFY(QFile::copy(QDir(m_media).filePath(QString::fromLatin1(name)),
                                QDir(m_work).filePath(QString::fromLatin1(name))));
        QVERIFY(QFile::copy(QDir(m_media).filePath(QStringLiteral("one.wav")),
                            QDir(m_work).filePath(QStringLiteral("new-音乐.wav"))));
        m_config = QDir(m_root).filePath(QStringLiteral("sender.conf"));
        const auto caption = QStringLiteral("VLC ipc " VERSION).toStdWString();
        QVERIFY2(!FindWindowW(nullptr, caption.c_str()), "Another same-version VLC instance is running; do not send it test files");
        const auto config = QDir(m_root).filePath(QStringLiteral("receiver.conf"));
        QVERIFY(write(config, {}));
        qputenv("VLC_USERDATA_PATH", QDir(m_root).filePath(QStringLiteral("receiver-data")).toUtf8());
        m_arguments = {QByteArray("--config=") + config.toUtf8(), "--no-ignore-config", "--quiet",
            "--one-instance", "--one-instance-file-manager-policy=1", "--playlist-open-mode=0",
            "--no-media-library", "--no-auto-preparse", "--no-metadata-network-access",
            "--extraintf=", "--control=", "--aout=dummy", "--vout=dummy", "--no-video"};
        QList<const char *> argv;
        for (const auto &arg : m_arguments) argv.append(arg.constData());
        m_instance = libvlc_new(int(argv.size()), argv.data());
        QVERIFY(m_instance);
        m_intf = vlc_object_create<intf_thread_t>(vlc_test_library_object(m_instance));
        QVERIFY(m_intf);
        QVERIFY(FindWindowW(nullptr, caption.c_str()));
    }
    void cleanupTestCase()
    {
        if (m_intf) { vlc_object_delete(m_intf); m_intf = nullptr; }
        if (m_instance) { libvlc_release(m_instance); m_instance = nullptr; }
        const auto report = qEnvironmentVariable("VLC_LIBRARY_PROCESS_TRACE");
        if (!report.isEmpty()) QVERIFY(write(report, m_processEvidence.join('\n').toUtf8()));
    }
    void init()
    {
        auto *pl = playlist(); vlc_playlist_Lock(pl);
        vlc_playlist_Stop(pl); vlc_playlist_Clear(pl);
        int result = VLC_SUCCESS;
        for (const auto *name : {"one.wav", "two.wav"})
        {
            const auto raw = uri(QDir(m_media).filePath(QString::fromLatin1(name))).toUtf8();
            auto *item = input_item_New(raw.constData(), nullptr);
            if (!item) { result = VLC_ENOMEM; break; }
            result = vlc_playlist_AppendOne(pl, item);
            input_item_Release(item);
            if (result != VLC_SUCCESS) break;
        }
        if (result == VLC_SUCCESS) result = vlc_playlist_GoTo(pl, 1);
        if (result == VLC_SUCCESS) result = vlc_playlist_Start(pl);
        vlc_playlist_Unlock(pl);
        QCOMPARE(result, VLC_SUCCESS);
        QTRY_COMPARE_WITH_TIMEOUT(state(), int(VLC_PLAYER_STATE_PLAYING), 10000);
        QTRY_VERIFY_WITH_TIMEOUT(clock() > 0, 10000);
    }
    void replaceFromDifferentWorkingDirectory()
    {
        QVERIFY(configure(true, 1, false));
        QVERIFY(send({QStringLiteral("new-音乐.wav"), QStringLiteral(":start-time=4"), QStringLiteral("two.wav")}));
        QCOMPARE(queue(), (QStringList{uri(QDir(m_work).filePath(QStringLiteral("new-音乐.wav"))),
                                      uri(QDir(m_work).filePath(QStringLiteral("two.wav")))}));
        QCOMPARE(selected(), ssize_t(0));
        QTRY_COMPARE_WITH_TIMEOUT(state(), int(VLC_PLAYER_STATE_PLAYING), 10000);
        QTRY_VERIFY_WITH_TIMEOUT(clock() >= VLC_TICK_FROM_SEC(4), 10000);
    }
    void appendPreservesPlayingItemAndClock()
    {
        QVERIFY(configure(true, 1, true));
        const auto old = queue(); const auto time = clock();
        QVERIFY(send({QStringLiteral("new-音乐.wav")}));
        QCOMPARE(queue(), old + QStringList{uri(QDir(m_work).filePath(QStringLiteral("new-音乐.wav")))});
        QCOMPARE(selected(), ssize_t(1));
        QCOMPARE(state(), int(VLC_PLAYER_STATE_PLAYING));
        QVERIFY(clock() >= time);
    }
    void appendPreservesPausedItemAndClock()
    {
        auto *pl = playlist(); vlc_playlist_Lock(pl);
        vlc_playlist_Pause(pl); vlc_playlist_Unlock(pl);
        QTRY_COMPARE_WITH_TIMEOUT(state(), int(VLC_PLAYER_STATE_PAUSED), 5000);
        QVERIFY(configure(true, 1, true));
        const auto old = queue(); const auto time = clock();
        QVERIFY(send({QStringLiteral("new-音乐.wav")}));
        QCOMPARE(queue(), old + QStringList{uri(QDir(m_work).filePath(QStringLiteral("new-音乐.wav")))});
        QCOMPARE(selected(), ssize_t(1));
        QCOMPARE(state(), int(VLC_PLAYER_STATE_PAUSED));
        QCOMPARE(clock(), time);
    }
    void fileManagerPolicy_data()
    {
        QTest::addColumn<bool>("master"); QTest::addColumn<int>("policy");
        QTest::addColumn<bool>("fromFile"); QTest::addColumn<bool>("forwarded");
        QTest::newRow("included-file-manager") << true << 1 << true << true;
        QTest::newRow("excluded-file-manager") << true << 0 << true << false;
        QTest::newRow("ordinary-launch-still-forwarded") << true << 0 << false << true;
        QTest::newRow("master-off") << false << 1 << true << false;
    }
    void fileManagerPolicy()
    {
        QFETCH(bool, master); QFETCH(int, policy); QFETCH(bool, fromFile); QFETCH(bool, forwarded);
        QVERIFY(configure(master, policy, true));
        const auto old = queue();
        QVERIFY(send({QStringLiteral("new-音乐.wav")}, fromFile, !forwarded));
        QCOMPARE(queue(), forwarded ? old + QStringList{uri(QDir(m_work).filePath(QStringLiteral("new-音乐.wav")))} : old);
        QCOMPARE(selected(), ssize_t(1));
        QCOMPARE(state(), int(VLC_PLAYER_STATE_PLAYING));
    }
};
QTEST_GUILESS_MAIN(TestLibraryOpeningProcess)
#include "test_library_opening_process.moc"
