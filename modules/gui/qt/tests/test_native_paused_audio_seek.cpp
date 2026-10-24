// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
// Actual production controller, NORMAL-lock player and Windows WASAPI.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "maininterface/compositor.hpp"
#include "player/player_controller.hpp"
#include <vlc/vlc.h>
#include <vlc_input_item.h>
#include <vlc_playlist.h>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QSignalSpy>
#include <QTest>
#include <QUrl>
#include <cmath>
#include <memory>

extern "C" const char vlc_module_name[] = "native_paused_audio_seek";
extern "C" vlc_object_t *vlc_test_native_instance_object(libvlc_instance_t *);
extern "C" vlc_playlist_t *vlc_test_stream_playlist(libvlc_instance_t *);

class TestNativePausedAudioSeek : public QObject
{
    Q_OBJECT
    libvlc_instance_t *m_instance = nullptr;
    vlc_playlist_t *m_playlist = nullptr;
    vlc_player_t *m_player = nullptr;
    qt_intf_t *m_context = nullptr;
    std::unique_ptr<QSettings> m_settings;
    std::unique_ptr<PlayerController> m_controller;
    QString m_directory, m_longFile, m_shortFile;
    QJsonArray m_samples;
    int m_serial = 0;

    vlc_player_state state() const
    { vlc_player_locker lock{m_player}; return vlc_player_GetState(m_player); }
    vlc_tick_t nativeTime() const
    { vlc_player_locker lock{m_player}; return vlc_player_GetTime(m_player); }
    bool start(const QString &file, bool initialPaused)
    {
        const auto uri = QUrl::fromLocalFile(file).toEncoded();
        auto *item = input_item_New(uri.constData(), nullptr);
        if (!item) return false;
        if (initialPaused && input_item_AddOption(item, ":start-paused", VLC_INPUT_OPTION_TRUSTED) != VLC_SUCCESS)
        { input_item_Release(item); return false; }
        int configured, started;
        {
            vlc_player_locker lock{m_player};
            vlc_player_SetStartPaused(m_player, initialPaused);
            configured = vlc_player_SetCurrentMedia(m_player, item);
            started = configured == VLC_SUCCESS ? vlc_player_Start(m_player) : VLC_EGENERIC;
        }
        input_item_Release(item);
        return configured == VLC_SUCCESS && started == VLC_SUCCESS;
    }
    void sample(const QString &phase)
    {
        m_samples.append(QJsonObject{{"phase", phase},
            {"time", QString::number(m_controller->getTime().toVLCTick())},
            {"position", m_controller->getPosition()},
            {"length", QString::number(m_controller->getLength().toVLCTick())},
            {"nativeOutputTime", QString::number(nativeTime())}, {"state", int(state())}});
    }
    bool at(double seconds, double position) const
    {
        return std::abs(m_controller->getTime().toVLCTick() - VLC_TICK_0 - seconds * CLOCK_FREQ) < 1000
            && std::abs(m_controller->getPosition() - position) < .001;
    }
private slots:
    void initTestCase()
    {
        m_directory = qEnvironmentVariable("VLC_PAUSED_AUDIO_OUTPUT");
        m_longFile = qEnvironmentVariable("VLC_PAUSED_AUDIO_LONG_WAV");
        m_shortFile = qEnvironmentVariable("VLC_PAUSED_AUDIO_SHORT_WAV");
        QVERIFY(!m_directory.isEmpty() && QDir().mkpath(m_directory));
        QVERIFY(QFile::exists(m_longFile) && QFile::exists(m_shortFile));
        qRegisterMetaType<VLCTime>(); qRegisterMetaType<VLCDuration>();
    }
    void init()
    {
        m_samples = QJsonArray{};
        const QByteArray log = "--logfile=" + QDir(m_directory).filePath(QString("native-%1.log").arg(++m_serial)).toUtf8();
        const char *args[] = {"--ignore-config", "--no-media-library", "--no-metadata-network-access", "--no-ml-network-discovery",
            "--no-video", "--no-spu", "--aout=mmdevice", "--mmdevice-backend=wasapi,none", "--file-logging", "--verbose=2", log.constData()};
        m_instance = libvlc_new(ARRAY_SIZE(args), args); QVERIFY(m_instance);
        m_playlist = vlc_test_stream_playlist(m_instance); QVERIFY(m_playlist);
        m_player = vlc_playlist_GetPlayer(m_playlist);
        m_context = vlc_object_create<qt_intf_t>(vlc_test_native_instance_object(m_instance)); QVERIFY(m_context);
        m_settings = std::make_unique<QSettings>(QDir(m_directory).filePath(QString("settings-%1.ini").arg(m_serial)), QSettings::IniFormat);
        m_context->intf = nullptr; m_context->p_mi = nullptr; m_context->mainSettings = m_settings.get();
        m_context->p_playlist = m_playlist; m_context->p_player = m_player;
        m_context->p_mainPlaylistController = nullptr; m_context->p_mainPlayerController = nullptr;
        m_context->isShuttingDown = false; m_context->b_isDialogProvider = false;
        m_controller = std::make_unique<PlayerController>(m_context);
        m_context->p_mainPlayerController = m_controller.get();
    }
    void actualInitialAndPlayedPausedSeeks_data()
    {
        QTest::addColumn<bool>("initialPaused"); QTest::addColumn<bool>("byPosition");
        QTest::newRow("initial-position") << true << true;
        QTest::newRow("initial-time") << true << false;
        QTest::newRow("played-position") << false << true;
        QTest::newRow("played-time") << false << false;
    }
    void actualInitialAndPlayedPausedSeeks()
    {
        QFETCH(bool, initialPaused); QFETCH(bool, byPosition);
        QVERIFY(start(m_longFile, initialPaused));
        if (!initialPaused)
        {
            QTRY_VERIFY_WITH_TIMEOUT(state() == VLC_PLAYER_STATE_PLAYING && nativeTime() > VLC_TICK_FROM_MS(300), 10000);
            vlc_player_locker lock{m_player}; vlc_player_Pause(m_player);
        }
        QTRY_COMPARE_WITH_TIMEOUT(state(), VLC_PLAYER_STATE_PAUSED, 10000);
        QTRY_COMPARE_WITH_TIMEOUT(m_controller->getLength().toVLCTick(), VLC_TICK_FROM_SEC(30), 10000);
        QTest::qWait(200); sample("baseline");
        QSignalSpy times(m_controller.get(), &PlayerController::timeChanged);
        QSignalSpy positions(m_controller.get(), &PlayerController::positionChanged);
        if (byPosition) m_controller->setPosition(.3);
        else m_controller->setTime(VLCTime(VLCDuration(VLC_TICK_FROM_SEC(9))));
        QTest::qWait(600); sample("paused-nine");
        QVERIFY(times.count() > 0 && positions.count() > 0);
        QVERIFY2(at(9., .3), "Paused seek must update both actual controller time and position");
        QCOMPARE(state(), VLC_PLAYER_STATE_PAUSED);
        QTest::qWait(300); QVERIFY(at(9., .3)); // Neither value advances while paused.
        m_controller->setPosition(0.);
        QTRY_VERIFY_WITH_TIMEOUT(at(0., 0.), 3000); sample("paused-zero");
        m_controller->setTime(VLCTime(VLCDuration(VLC_TICK_FROM_SEC(18))));
        QTRY_VERIFY_WITH_TIMEOUT(at(18., .6), 3000); sample("paused-eighteen");
        QCOMPARE(state(), VLC_PLAYER_STATE_PAUSED);
        { vlc_player_locker lock{m_player}; vlc_player_Resume(m_player); }
        QTRY_VERIFY_WITH_TIMEOUT(state() == VLC_PLAYER_STATE_PLAYING && nativeTime() > VLC_TICK_FROM_MS(18200), 10000);
        QVERIFY(nativeTime() < VLC_TICK_FROM_SEC(20));
        QTRY_VERIFY_WITH_TIMEOUT(m_controller->getTime().toVLCTick() > VLC_TICK_FROM_MS(18200), 3000);
        sample("resumed-from-eighteen");
    }
    void actualPausedMediaSwitchUsesTheNewDuration()
    {
        QVERIFY(start(m_longFile, true));
        QTRY_COMPARE_WITH_TIMEOUT(m_controller->getLength().toVLCTick(), VLC_TICK_FROM_SEC(30), 10000);
        QVERIFY(start(m_shortFile, true));
        QTRY_COMPARE_WITH_TIMEOUT(state(), VLC_PLAYER_STATE_PAUSED, 10000);
        QTRY_COMPARE_WITH_TIMEOUT(m_controller->getLength().toVLCTick(), VLC_TICK_FROM_SEC(10), 10000);
        QTest::qWait(200);
        m_controller->setPosition(.6); QTest::qWait(600); sample("new-ten-second-media");
        QVERIFY2(at(6., .6), "Initial paused replacement must use its own duration");
        QCOMPARE(state(), VLC_PLAYER_STATE_PAUSED);
    }
    void cleanup()
    {
        bool stopped = true;
        if (m_player)
        {
            { vlc_player_locker lock{m_player}; vlc_player_Stop(m_player); }
            stopped = QTest::qWaitFor([this] { return state() == VLC_PLAYER_STATE_STOPPED; }, 10000);
        }
        m_controller.reset();
        if (m_context) vlc_object_delete(m_context);
        m_context = nullptr; m_settings.reset();
        if (m_playlist) vlc_playlist_Delete(m_playlist);
        m_playlist = nullptr; m_player = nullptr;
        if (m_instance) libvlc_release(m_instance);
        m_instance = nullptr;
        QFile output(QDir(m_directory).filePath(QString("samples-%1.json").arg(m_serial)));
        QVERIFY(output.open(QIODevice::WriteOnly)); output.write(QJsonDocument(m_samples).toJson());
        QVERIFY2(stopped, "Owned audio player did not stop normally");
    }
};
QTEST_GUILESS_MAIN(TestNativePausedAudioSeek)
#include "test_native_paused_audio_seek.moc"
