// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "qt.hpp"
#include "maininterface/compositor.hpp"
#include "maininterface/audio_preferences_model.hpp"
#include <vlc/vlc.h>
#include <vlc_aout.h>
#include <vlc_configuration.h>
#include <vlc_input.h>
#include <vlc_player.h>
#include <vlc_playlist.h>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>
#include <cmath>
#include <memory>

extern "C" const char vlc_module_name[] = "audio_startup_consumer";
extern "C" vlc_object_t *vlc_test_general_instance_object(libvlc_instance_t *);
using StartupPlayerLocker = vlc_locker<vlc_player_t, vlc_player_Lock, vlc_player_Unlock>;

// A console test of the real preference writer and ordinary Core/MMDevice/
// WASAPI consumer. It does not create a window or substitute an audio output.
class TestAudioStartupConsumer final : public QObject
{
    Q_OBJECT
    QTemporaryDir m_temporary;
    QString m_output, m_config, m_log;
    unsigned m_openSerial = 0;
    libvlc_instance_t *m_instance = nullptr;
    qt_intf_t *m_intf = nullptr;
    std::unique_ptr<AudioPreferencesModel> m_model;

    bool openNative()
    {
        m_log = QDir(QFileInfo(m_config).absolutePath()).filePath(
            QStringLiteral("vlc-open-%1.log").arg(++m_openSerial));
        const QByteArray config = "--config=" + m_config.toUtf8();
        const QByteArray log = "--logfile=" + m_log.toUtf8();
        const char *args[] = {"--no-ignore-config", "--no-media-library",
            "--no-metadata-network-access", "--no-ml-network-discovery",
            "--extraintf=", "--control=", "--no-video", "--no-spu",
            "--mmdevice-backend=wasapi,none", "--file-logging", "--log-verbose=2",
            config.constData(), log.constData()};
        m_instance = libvlc_new(ARRAY_SIZE(args), args);
        if (!m_instance) return false;
        m_intf = vlc_object_create<qt_intf_t>(vlc_test_general_instance_object(m_instance));
        if (!m_intf) return false;
        AudioPreferencesModel::Consumers consumers;
        consumers.windowsDevices = true;
        consumers.authoredVolumeStep = true;
        m_model = std::make_unique<AudioPreferencesModel>(m_intf, consumers);
        return true;
    }
    vlc_player_state state() const
    {
        StartupPlayerLocker lock(m_intf->p_player);
        return vlc_player_GetState(m_intf->p_player);
    }
    bool audioPlayed() const
    {
        StartupPlayerLocker lock(m_intf->p_player);
        const auto *stats = vlc_player_GetStatistics(m_intf->p_player);
        return stats && stats->i_decoded_audio > 0 && stats->i_played_abuffers > 0;
    }
    void stopOwned()
    {
        vlc_playlist_Lock(m_intf->p_playlist);
        vlc_playlist_Stop(m_intf->p_playlist);
        vlc_playlist_Unlock(m_intf->p_playlist);
    }
    bool closeNative()
    {
        bool stopped = true;
        if (m_intf && m_intf->p_playlist) {
            stopOwned();
            stopped = QTest::qWaitFor([this] { return state() == VLC_PLAYER_STATE_STOPPED; }, 10000);
            // Delete joins the owned input, aout and its callbacks before their
            // object tree is released, including on a failed test assertion.
            vlc_playlist_Delete(m_intf->p_playlist);
            m_intf->p_playlist = nullptr;
            m_intf->p_player = nullptr;
        }
        m_model.reset();
        if (m_intf) { vlc_object_delete(m_intf); m_intf = nullptr; }
        if (m_instance) { libvlc_release(m_instance); m_instance = nullptr; }
        return stopped;
    }
    bool startOwned()
    {
        const auto fixture = qEnvironmentVariable("VLC_AUDIO_STARTUP_WAV");
        if (!QFileInfo(fixture).isAbsolute() || !QFileInfo::exists(fixture)) return false;
        m_intf->p_playlist = vlc_playlist_New(vlc_test_general_instance_object(m_instance),
                                            VLC_PLAYLIST_PREPARSING_DISABLED, 1, 0);
        if (!m_intf->p_playlist) return false;
        m_intf->p_player = vlc_playlist_GetPlayer(m_intf->p_playlist);
        const auto uri = QUrl::fromLocalFile(fixture).toEncoded();
        auto *media = input_item_New(uri.constData(), nullptr);
        if (!media) return false;
        vlc_playlist_Lock(m_intf->p_playlist);
        int result = vlc_playlist_InsertOne(m_intf->p_playlist, 0, media);
        if (result == VLC_SUCCESS) result = vlc_playlist_PlayAt(m_intf->p_playlist, 0);
        vlc_playlist_Unlock(m_intf->p_playlist);
        input_item_Release(media);
        return result == VLC_SUCCESS;
    }
    QString device() const
    {
        std::unique_ptr<audio_output_t, decltype(&aout_Release)> aout(
            vlc_player_aout_Hold(m_intf->p_player), aout_Release);
        if (!aout) return {};
        char *raw = aout_DeviceGet(aout.get());
        const auto result = QString::fromUtf8(raw ? raw : "");
        free(raw);
        return result;
    }
    static QByteArray bytes(const QString &path)
    {
        QFile file(path);
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray{};
    }
private slots:
    void initTestCase()
    {
        QVERIFY(m_temporary.isValid());
        m_output = qEnvironmentVariable("VLC_AUDIO_STARTUP_OUTPUT");
        QVERIFY(QFileInfo(m_output).isAbsolute());
        QVERIFY(!QFileInfo::exists(m_output));
        QVERIFY(QDir().mkpath(m_output));
        const auto userdata = QDir(m_output).filePath(QStringLiteral("userdata"));
        QVERIFY(QDir().mkpath(userdata));
        qputenv("VLC_USERDATA_PATH", userdata.toUtf8());
    }
    void init()
    {
        const auto folder = QDir(m_output).filePath(QString::fromLatin1(QTest::currentTestFunction()));
        QVERIFY(QDir().mkpath(folder));
        m_config = QDir(folder).filePath(QStringLiteral("vlcrc"));
        m_log = QDir(folder).filePath(QStringLiteral("vlc.log"));
        QVERIFY(openNative());
    }
    void cleanup() { QVERIFY2(closeNative(), "The native audio owner did not acknowledge normal stop"); }

    void liveStepDoesNotCreateAFalseRestartRequirement()
    {
        // Establish the managed MMDevice startup policy before taking the
        // baseline. Changing startupVolume also selects aout=mmdevice; that
        // separate output-policy change legitimately requires a restart.
        QVERIFY(m_model->setValue(QStringLiteral("startupVolume"), 40.));
        QVERIFY(closeNative());
        QVERIFY(openNative());
        const auto originalVolume = m_model->values().value(QStringLiteral("startupVolume"));
        QVERIFY(!m_model->restartRequired());
        QVERIFY(m_model->setValue(QStringLiteral("volumeStep"), 2.5));
        QVERIFY2(!m_model->restartRequired(), "The linked slider policy and native step action are live consumers");
        m_model->reload();
        QVERIFY(!m_model->restartRequired());
        QVERIFY(m_model->setValue(QStringLiteral("startupVolume"), 55.));
        QVERIFY(m_model->restartRequired());
        QVERIFY(m_model->setValue(QStringLiteral("startupVolume"), originalVolume));
        QVERIFY2(!m_model->restartRequired(), "A remaining live step change must not keep the restart banner visible");
    }

    void savedDeviceVolumeAndStepReachRealWasapi()
    {
        QVERIFY(m_model->capabilities().value(QStringLiteral("outputDevice")).toBool());
        QString chosen;
        for (const auto &entry : m_model->devices()) {
            const auto id = entry.toMap().value(QStringLiteral("value")).toString();
            if (!id.isEmpty()) { chosen = id; break; }
        }
        QVERIFY2(!chosen.isEmpty(), "This real-consumer case needs an active render endpoint; a skip is not acceptance");
        QVERIFY(m_model->setValue(QStringLiteral("outputDevice"), chosen));
        QVERIFY(m_model->setValue(QStringLiteral("rememberVolume"), false));
        QVERIFY(m_model->setValue(QStringLiteral("startupVolume"), 40.));
        QVERIFY(m_model->setValue(QStringLiteral("volumeStep"), 2.5));
        const auto firstSaved = bytes(m_config);
        QVERIFY(!firstSaved.isEmpty());
        QVERIFY(closeNative());
        QVERIFY(openNative());
        QVERIFY(startOwned());
        QTRY_VERIFY_WITH_TIMEOUT(audioPlayed(), 10000);
        QTRY_COMPARE_WITH_TIMEOUT(device(), chosen, 10000);
        QTRY_VERIFY_WITH_TIMEOUT(std::abs(vlc_player_aout_GetVolume(m_intf->p_player) - .4f) < .002f, 10000);

        // Stored startup changes must preserve the already-open output and its
        // paused input. Native step actions deliberately read the current policy.
        {
            StartupPlayerLocker lock(m_intf->p_player);
            vlc_player_Pause(m_intf->p_player);
        }
        QTRY_COMPARE_WITH_TIMEOUT(state(), VLC_PLAYER_STATE_PAUSED, 10000);
        QTest::qWait(200); // Allow the final pause clock notification to settle.
        vlc_tick_t clock;
        input_item_t *input;
        {
            StartupPlayerLocker lock(m_intf->p_player);
            clock = vlc_player_GetTime(m_intf->p_player);
            input = vlc_player_GetCurrentMedia(m_intf->p_player);
        }
        QVERIFY(m_model->setValue(QStringLiteral("startupVolume"), 65.));
        QCOMPARE(device(), chosen);
        QVERIFY(std::abs(vlc_player_aout_GetVolume(m_intf->p_player) - .4f) < .002f);
        float incremented = -1;
        QCOMPARE(vlc_player_aout_IncrementVolume(m_intf->p_player, 1, &incremented), VLC_SUCCESS);
        QVERIFY(std::abs(incremented - .425f) < .0001f);
        QTRY_VERIFY_WITH_TIMEOUT(std::abs(vlc_player_aout_GetVolume(m_intf->p_player) - .425f) < .002f, 10000);
        QCOMPARE(vlc_player_aout_SetVolume(m_intf->p_player, .4f), VLC_SUCCESS);
        QTRY_VERIFY_WITH_TIMEOUT(std::abs(vlc_player_aout_GetVolume(m_intf->p_player) - .4f) < .002f, 10000);
        QVERIFY(m_model->setValue(QStringLiteral("volumeStep"), 5.));
        QCOMPARE(vlc_player_aout_IncrementVolume(m_intf->p_player, 1, &incremented), VLC_SUCCESS);
        QVERIFY(std::abs(incremented - .45f) < .0001f);
        QTRY_VERIFY_WITH_TIMEOUT(std::abs(vlc_player_aout_GetVolume(m_intf->p_player) - .45f) < .002f, 10000);
        {
            StartupPlayerLocker lock(m_intf->p_player);
            QCOMPARE(vlc_player_GetState(m_intf->p_player), VLC_PLAYER_STATE_PAUSED);
            QCOMPARE(vlc_player_GetCurrentMedia(m_intf->p_player), input);
            QCOMPARE(vlc_player_GetTime(m_intf->p_player), clock);
        }
        const auto secondSaved = bytes(m_config);
        QVERIFY(secondSaved != firstSaved);
        QVERIFY(closeNative());
        QVERIFY(openNative());
        QVERIFY(startOwned());
        QTRY_VERIFY_WITH_TIMEOUT(audioPlayed(), 10000);
        QTRY_COMPARE_WITH_TIMEOUT(device(), chosen, 10000);
        QTRY_VERIFY_WITH_TIMEOUT(std::abs(vlc_player_aout_GetVolume(m_intf->p_player) - .65f) < .002f, 10000);
        QCOMPARE(bytes(m_config), secondSaved);
    }
};
QTEST_GUILESS_MAIN(TestAudioStartupConsumer)
#include "test_audio_startup_consumer.moc"
