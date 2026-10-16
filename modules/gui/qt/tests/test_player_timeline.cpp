// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
// Real core/player/controller callbacks; owned local HLS, dummy audio output.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "vlc_stub_modules.hpp"
#include "player/player_controller.hpp"
#include <vlc_input_item.h>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QSignalSpy>
#include <QProcess>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QTest>
#include <QThread>
#include <cmath>
#include <atomic>
#include <cstdio>

class TestPlayerTimeline : public QObject
{
    Q_OBJECT
    QTemporaryDir m_directory;
    QDir m_fixtures;
    QList<QByteArray> m_records;
    std::unique_ptr<QProcess> m_server;
    quint16 m_serverPort = 0;
    unsigned m_playlistRequests = 0;
    std::unique_ptr<VLCTestingEnv> m_environment;
    std::unique_ptr<QSettings> m_settings;
    std::unique_ptr<PlayerController> m_controller;
    vlc_player_t *m_player = nullptr;
    unsigned m_notifications = 0;
    vlc_player_timer_id *m_timer = nullptr;
    std::atomic_uint m_seekRequests{0};
    std::atomic_bool m_seeking{false};
    FILE *m_log = nullptr;

    static void onUpdate(const vlc_player_timer_point *, void *) {}
    static void onSeek(const vlc_player_timer_point *point, void *opaque)
    {
        auto *self = static_cast<TestPlayerTimeline *>(opaque);
        self->m_seeking.store(point != nullptr);
        if (point) ++self->m_seekRequests;
    }
    void snapshot(const char *phase)
    {
        vlc_player_locker lock{m_player};
        qInfo() << phase << "core state/time/position; Qt state/time/position; seeking"
            << vlc_player_GetState(m_player) << vlc_player_GetTime(m_player) << vlc_player_GetPosition(m_player)
            << m_controller->getPlayingState() << m_controller->getTime().toVLCTick()
            << m_controller->getPosition() << m_seeking.load();
    }

    void setWindow(int first, int count)
    {
        QByteArray playlist("#EXTM3U\n#EXT-X-VERSION:3\n#EXT-X-TARGETDURATION:2\n#EXT-X-MEDIA-SEQUENCE:");
        playlist += QByteArray::number(first) + '\n';
        for (int i = first; i < first + count; ++i) playlist += m_records.at(i);
        // Deliberately no ENDLIST: production adaptive demux reports a live range.
        QSaveFile file(m_directory.filePath("live.m3u8"));
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QCOMPARE(file.write(playlist), playlist.size());
        QVERIFY(file.commit());
    }
    unsigned playlistRequests()
    {
        while (m_server->canReadLine())
        {
            const auto line = m_server->readLine().trimmed();
            if (line.startsWith("PLAYLIST ")) m_playlistRequests = line.mid(9).toUInt();
        }
        return m_playlistRequests;
    }
    bool start(const QUrl &url)
    {
        const auto uri = url.toEncoded();
        auto *item = input_item_New(uri.constData(), "Owned native timeline fixture");
        if (!item) return false;
        int configured, started;
        {
            vlc_player_locker lock{m_player};
            configured = vlc_player_SetCurrentMedia(m_player, item);
            started = configured == VLC_SUCCESS ? vlc_player_Start(m_player) : VLC_EGENERIC;
        }
        input_item_Release(item);
        return configured == VLC_SUCCESS && started == VLC_SUCCESS;
    }
    void verifyLiveClock()
    {
        QVERIFY(m_controller->isLive());
        QVERIFY(m_controller->isSeekable());
        QVERIFY(m_controller->getLength().valid());
        const auto length = m_controller->getLength().toVLCTick();
        const auto display = m_controller->getDisplayTime();
        QVERIFY(display.valid());
        // Independent presentation oracle, with only integer rounding tolerance.
        const auto expected = VLC_TICK_0 + std::llround(length * m_controller->getPosition());
        QVERIFY2(std::abs(display.toVLCTick() - expected) <= 2, "Clock is not relative to the advertised live range");
        QVERIFY(display.toVLCTick() >= VLC_TICK_0 && display.toVLCTick() <= VLC_TICK_0 + length);
        qInfo() << "live engine/window/display/position" << m_controller->getTime().toVLCTick()
                << length << display.toVLCTick() << m_controller->getPosition();
    }
private slots:
    void initTestCase()
    {
        const auto directory = qEnvironmentVariable("VLC_TIMELINE_FIXTURE_DIR");
        if (directory.isEmpty()) QSKIP("Set VLC_TIMELINE_FIXTURE_DIR to generated finite.wav and HLS fixtures");
        m_fixtures = QDir(directory);
        QVERIFY(m_fixtures.exists("finite.wav"));
        QFile file(m_fixtures.filePath("generated.m3u8"));
        QVERIFY(file.open(QIODevice::ReadOnly));
        QByteArray duration;
        for (const auto &line : file.readAll().split('\n'))
        {
            if (line.startsWith("#EXTINF:")) duration = line + '\n';
            else if (!line.isEmpty() && !line.startsWith('#'))
            {
                QVERIFY(!duration.isEmpty());
                m_records.append(duration + line + '\n');
                duration.clear();
            }
        }
        QVERIFY(m_records.size() >= 18);
        QVERIFY(m_directory.isValid());
        qputenv("VLC_TEST_TIMEOUT", "30");
        qRegisterMetaType<VLCTime>();
    }
    void init()
    {
        m_server = std::make_unique<QProcess>();
        m_playlistRequests = 0;
        setWindow(0, 12);
        const auto python = qEnvironmentVariable("VLC_TIMELINE_PYTHON", "python3");
        m_server->start(python, {m_fixtures.filePath("native_timeline_server.py"),
            m_fixtures.absolutePath(), m_directory.filePath("live.m3u8")});
        QVERIFY(m_server->waitForStarted(5000));
        QVERIFY(m_server->waitForReadyRead(5000));
        const auto greeting = m_server->readLine().trimmed();
        QVERIFY2(greeting.startsWith("PORT "), greeting.constData());
        m_serverPort = greeting.mid(5).toUShort();
        QVERIFY(m_serverPort != 0);
        m_environment = std::make_unique<VLCTestingEnv>();
        QVERIFY(m_environment->init(true));
        const auto logDirectory = qEnvironmentVariable("VLC_USERDATA_PATH");
        QVERIFY(!logDirectory.isEmpty() && QDir().mkpath(logDirectory));
        const auto logfile = QFile::encodeName(QDir(logDirectory).filePath(QString(QTest::currentTestFunction()) + ".log"));
        m_log = std::fopen(logfile.constData(), "w");
        QVERIFY(m_log);
        libvlc_log_set_file(m_environment->libvlc, m_log);
        m_player = m_environment->intf->p_player;
        QVERIFY(m_player);
        m_settings = std::make_unique<QSettings>(m_directory.filePath("settings.ini"), QSettings::IniFormat);
        m_environment->intf->mainSettings = m_settings.get();
        m_environment->intf->p_mi = nullptr;
        m_environment->intf->p_mainPlaylistController = nullptr;
        m_controller = std::make_unique<PlayerController>(m_environment->intf);
        m_environment->intf->p_mainPlayerController = m_controller.get();
        QVERIFY(m_controller->metaObject()->indexOfProperty("displayTime") >= 0);
        QVERIFY(m_controller->metaObject()->indexOfProperty("live") >= 0);
        m_seekRequests = 0;
        m_seeking = false;
        static const vlc_player_timer_cbs callbacks = {&onUpdate, nullptr, &onSeek};
        m_timer = vlc_player_AddTimer(m_player, VLC_TICK_INVALID, &callbacks, this);
        QVERIFY(m_timer);
        connect(m_controller.get(), &PlayerController::displayTimeChanged, this, [this] {
            QCOMPARE(QThread::currentThread(), m_controller->thread());
            // Re-lock the actual NORMAL player: notification under its lock deadlocks.
            (void)m_controller->getInput();
            ++m_notifications;
        }, Qt::DirectConnection);
    }
    void finiteMediaAndPausedSeekKeepEngineTime()
    {
        QSignalSpy changes(m_controller.get(), &PlayerController::displayTimeChanged);
        QVERIFY(start(QUrl::fromLocalFile(m_fixtures.filePath("finite.wav"))));
        QTRY_VERIFY_WITH_TIMEOUT(m_controller->getTime().toVLCTick() > VLC_TICK_FROM_MS(250), 10000);
        QVERIFY(!m_controller->isLive());
        QCOMPARE(m_controller->getDisplayTime().toVLCTick(), m_controller->getTime().toVLCTick());
        { vlc_player_locker lock{m_player}; vlc_player_Pause(m_player); }
        QTRY_COMPARE_WITH_TIMEOUT(m_controller->getPlayingState(), PlayerController::PLAYING_STATE_PAUSED, 5000);
        m_controller->setPosition(.4f);
        QTRY_VERIFY_WITH_TIMEOUT(m_seekRequests.load() > 0 && !m_seeking.load(), 5000);
        QTRY_VERIFY_WITH_TIMEOUT(m_controller->getPosition() > .35 && m_controller->getPosition() < .45, 5000);
        QCOMPARE(m_controller->getDisplayTime().toVLCTick(), m_controller->getTime().toVLCTick());
        m_controller->setPosition(0.f);
        QTRY_VERIFY_WITH_TIMEOUT(m_seekRequests.load() > 1 && !m_seeking.load(), 5000);
        snapshot("finite paused seek to zero");
        QTRY_VERIFY_WITH_TIMEOUT(m_controller->getDisplayTime().valid()
            && m_controller->getDisplayTime().toVLCTick() >= VLC_TICK_0
            && m_controller->getDisplayTime().toVLCTick() < VLC_TICK_FROM_MS(250), 5000);
        QVERIFY(changes.count() > 0);
    }
    void livePauseResumeWindowUpdateAndMediaReset()
    {
        QSignalSpy liveChanges(m_controller.get(), &PlayerController::liveChanged);
        QSignalSpy engineTimes(m_controller.get(), &PlayerController::timeChanged);
        QSignalSpy positions(m_controller.get(), &PlayerController::positionChanged);
        QVERIFY(start(QUrl(QString("http://127.0.0.1:%1/live.m3u8").arg(m_serverPort))));
        QTRY_VERIFY_WITH_TIMEOUT(m_controller->isLive() && m_controller->isSeekable()
            && m_controller->getDisplayTime().valid(), 10000);
        verifyLiveClock();
        QVERIFY(liveChanges.count() > 0);
        engineTimes.clear();
        positions.clear();
        {
            // Queue both commands before allowing callbacks to re-lock the
            // NORMAL player. Pause must not stop data needed by seek buffering.
            vlc_player_locker lock{m_player};
            vlc_player_SeekByPos(m_player, .3, VLC_PLAYER_SEEK_PRECISE, VLC_PLAYER_WHENCE_ABSOLUTE);
            vlc_player_Pause(m_player);
        }
        QTRY_VERIFY_WITH_TIMEOUT(m_seekRequests.load() > 0 && !m_seeking.load(), 5000);
        bool unknownPendingClock = false;
        for (const auto &change : engineTimes)
            unknownPendingClock |= !qvariant_cast<VLCTime>(change.first()).valid();
        QVERIFY2(unknownPendingClock, "A live position seek published an invented absolute timestamp");
        bool deliveredPosition = false;
        for (const auto &change : positions)
            deliveredPosition |= std::abs(change.first().toDouble() - .3) < .001;
        QVERIFY2(deliveredPosition, "The actual seek callback did not reach the controller");
        QTest::qWait(250);
        snapshot("live pause");
        QTRY_COMPARE_WITH_TIMEOUT(m_controller->getPlayingState(), PlayerController::PLAYING_STATE_PAUSED, 5000);
        QTest::qWait(100);
        verifyLiveClock();
        const auto paused = m_controller->getTime().toVLCTick();
        const auto pausedDisplay = m_controller->getDisplayTime().toVLCTick();
        QTest::qWait(250);
        QCOMPARE(m_controller->getTime().toVLCTick(), paused);
        QCOMPARE(m_controller->getDisplayTime().toVLCTick(), pausedDisplay);
        { vlc_player_locker lock{m_player}; vlc_player_Resume(m_player); }
        QTRY_VERIFY_WITH_TIMEOUT(m_controller->getTime().toVLCTick() > paused + VLC_TICK_FROM_MS(250), 5000);
        verifyLiveClock();
        const auto oldLength = m_controller->getLength().toVLCTick();
        const auto requests = playlistRequests();
        setWindow(2, 14);
        QTRY_VERIFY_WITH_TIMEOUT(playlistRequests() > requests, 6000);
        QTRY_VERIFY_WITH_TIMEOUT(m_controller->getLength().toVLCTick() > oldLength + VLC_TICK_FROM_MS(500), 6000);
        verifyLiveClock();
        { vlc_player_locker lock{m_player}; vlc_player_Stop(m_player); }
        QTRY_COMPARE_WITH_TIMEOUT(m_controller->getPlayingState(), PlayerController::PLAYING_STATE_STOPPED, 5000);
        {
            vlc_player_locker lock{m_player};
            QCOMPARE(vlc_player_SetCurrentMedia(m_player, nullptr), VLC_SUCCESS);
        }
        QTRY_VERIFY(!m_controller->hasInput());
        QVERIFY(!m_controller->isLive());
        QVERIFY(!m_controller->getDisplayTime().valid());
        QVERIFY(liveChanges.count() >= 2);
        QCOMPARE(liveChanges.last().first().toBool(), false);
    }
    void cleanup()
    {
        if (m_player)
        {
            { vlc_player_locker lock{m_player}; vlc_player_Stop(m_player); }
            QTRY_COMPARE_WITH_TIMEOUT(m_controller->getPlayingState(), PlayerController::PLAYING_STATE_STOPPED, 5000);
            if (m_timer) vlc_player_RemoveTimer(m_player, m_timer);
        }
        m_timer = nullptr;
        m_controller.reset();
        if (m_environment && m_environment->intf) m_environment->intf->p_mainPlayerController = nullptr;
        m_player = nullptr;
        if (m_environment && m_environment->libvlc) libvlc_log_unset(m_environment->libvlc);
        m_environment.reset();
        if (m_log) std::fclose(m_log);
        m_log = nullptr;
        m_settings.reset();
        if (m_server && m_server->state() != QProcess::NotRunning)
        {
            m_server->write("stop\n");
            m_server->closeWriteChannel();
            QVERIFY(m_server->waitForFinished(5000));
            QCOMPARE(m_server->exitStatus(), QProcess::NormalExit);
            QCOMPARE(m_server->exitCode(), 0);
        }
        m_server.reset();
    }
    void cleanupTestCase()
    {
        QVERIFY(m_notifications > 0);
    }
};
QTEST_GUILESS_MAIN(TestPlayerTimeline)
#include "test_player_timeline.moc"
