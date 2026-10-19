// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "qt.hpp"
#include "maininterface/compositor.hpp"
#include "maininterface/playback_preferences_model.hpp"
#include "maininterface/audio_preferences_model.hpp"
#include <vlc/vlc.h>
#include <vlc_configuration.h>
#include <vlc_input.h>
#include <vlc_player.h>
#include <vlc_playlist.h>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QTest>
#include <QUrl>
#include <array>
#include <atomic>
#include <memory>

extern "C" const char vlc_module_name[] = "playback_track_consumers";
extern "C" vlc_object_t *vlc_test_general_instance_object(libvlc_instance_t *);
extern "C" vlc_playlist_t *vlc_test_playback_main_playlist(libvlc_instance_t *);
extern "C" void vlc_test_playback_exit_handler(libvlc_instance_t *, void (*)(void *), void *);
using TrackConsumerLocker = vlc_locker<vlc_player_t, vlc_player_Lock, vlc_player_Unlock>;

// Real native input/decoder track selection, with no Qt window, display window
// or audible output. The preference model is the ordinary production model.
class TestPlaybackTrackConsumers final : public QObject
{
    Q_OBJECT
    QString m_output, m_config, m_log;
    unsigned m_openSerial = 0;
    libvlc_instance_t *m_instance = nullptr;
    qt_intf_t *m_intf = nullptr;
    std::unique_ptr<PlaybackPreferencesModel> m_model;
    std::unique_ptr<AudioPreferencesModel> m_audio;
    std::array<input_item_t *, 3> m_observedMedia{};
    std::array<int, 16> m_startedIndices{};
    std::atomic<unsigned> m_startedCount{0};
    std::array<int, 16> m_transitionIndices{};
    std::atomic<unsigned> m_transitionCount{0};
    std::atomic<unsigned> m_exitCount{0};
    vlc_player_cbs m_callbacks{};
    vlc_player_listener_id *m_listener = nullptr;
    static void recordTransition(TestPlaybackTrackConsumers *self, input_item_t *media)
    {
        if (!media) return;
        int index = -1;
        for (size_t i = 0; i < self->m_observedMedia.size(); ++i)
            if (self->m_observedMedia[i] && self->m_observedMedia[i] == media) index = int(i);
        const unsigned count = self->m_transitionCount.load(std::memory_order_relaxed);
        if (count < self->m_transitionIndices.size()) self->m_transitionIndices[count] = index;
        self->m_transitionCount.store(count + 1, std::memory_order_release);
    }
    static void onCurrentMediaChanged(vlc_player_t *player, input_item_t *media, void *opaque)
    {
        // Gapless transitions intentionally keep the player in PLAYING and do
        // not send a fresh playing-state event (vlc_player.h playback contract).
        if (vlc_player_GetState(player) == VLC_PLAYER_STATE_PLAYING)
            recordTransition(static_cast<TestPlaybackTrackConsumers *>(opaque), media);
    }
    static void onStateChanged(vlc_player_t *player, enum vlc_player_state state, void *opaque)
    {
        if (state != VLC_PLAYER_STATE_PLAYING) return;
        auto *self = static_cast<TestPlaybackTrackConsumers *>(opaque);
        auto *media = vlc_player_GetCurrentMedia(player); // Callback holds the native lock.
        int index = -1;
        for (size_t i = 0; i < self->m_observedMedia.size(); ++i)
            if (self->m_observedMedia[i] && self->m_observedMedia[i] == media) index = int(i);
        const unsigned count = self->m_startedCount.load(std::memory_order_relaxed);
        if (count < self->m_startedIndices.size()) self->m_startedIndices[count] = index;
        self->m_startedCount.store(count + 1, std::memory_order_release);
        if (self->m_transitionCount.load(std::memory_order_relaxed) == 0)
            recordTransition(self, media);
    }
    bool observeStarts()
    {
        m_startedCount.store(0);
        m_startedIndices.fill(-1);
        m_transitionCount.store(0);
        m_transitionIndices.fill(-1);
        m_callbacks = {};
        m_callbacks.on_state_changed = onStateChanged;
        m_callbacks.on_current_media_changed = onCurrentMediaChanged;
        TrackConsumerLocker lock(m_intf->p_player);
        m_listener = vlc_player_AddListener(m_intf->p_player, &m_callbacks, this);
        return m_listener != nullptr;
    }
    bool openNative()
    {
        m_log = QDir(QFileInfo(m_config).absolutePath()).filePath(
            QStringLiteral("vlc-open-%1.log").arg(++m_openSerial));
        const QByteArray config = QByteArray("--config=") + m_config.toUtf8();
        const QByteArray log = QByteArray("--logfile=") + m_log.toUtf8();
        const char *args[] = {"--no-ignore-config", "--no-media-library",
            "--no-metadata-network-access", "--no-ml-network-discovery",
            "--extraintf=", "--control=", "--aout=adummy", "--vout=vdummy",
            "--no-hw-dec", "--dec-dev=none", "--adaptive-logic=highest",
            "--width=640", "--height=360",
            "--quiet", "--file-logging", "--log-verbose=2", config.constData(), log.constData()};
        m_instance = libvlc_new(ARRAY_SIZE(args), args);
        if (!m_instance) return false;
        auto *instanceObject = vlc_test_general_instance_object(m_instance);
        // A dummy display alone still lets Core create an ordinary video
        // window. The window selector is an inherited object variable.
        if (var_Create(instanceObject, "window", VLC_VAR_STRING) != VLC_SUCCESS
            || var_SetString(instanceObject, "window", "wdummy,none") != VLC_SUCCESS)
            return false;
        m_exitCount.store(0);
        vlc_test_playback_exit_handler(m_instance, [](void *opaque) {
            static_cast<TestPlaybackTrackConsumers *>(opaque)->m_exitCount.fetch_add(1);
        }, this);
        m_intf = vlc_object_create<qt_intf_t>(vlc_test_general_instance_object(m_instance));
        if (!m_intf) return false;
        m_model = std::make_unique<PlaybackPreferencesModel>(m_intf);
        m_audio = std::make_unique<AudioPreferencesModel>(m_intf);
        return true;
    }
    vlc_player_state state() const
    {
        TrackConsumerLocker lock(m_intf->p_player);
        return vlc_player_GetState(m_intf->p_player);
    }
    bool closeNative()
    {
        // Teardown's own Stop/release is not a tested finish-action event.
        // Unregistration also waits for an already-running exit callback.
        if (m_instance) vlc_test_playback_exit_handler(m_instance, nullptr, nullptr);
        bool stopped = true;
        if (m_intf && m_intf->p_playlist) {
            vlc_playlist_Lock(m_intf->p_playlist);
            vlc_playlist_Stop(m_intf->p_playlist);
            vlc_playlist_Unlock(m_intf->p_playlist);
            stopped = QTest::qWaitFor([this] { return state() == VLC_PLAYER_STATE_STOPPED; }, 10000);
            if (m_listener) {
                TrackConsumerLocker lock(m_intf->p_player);
                vlc_player_RemoveListener(m_intf->p_player, m_listener);
                m_listener = nullptr;
            }
            // The genuine main playlist belongs to the isolated instance and
            // is deleted by libvlc_release, after its input has stopped.
            m_intf->p_playlist = nullptr;
            m_intf->p_player = nullptr;
        }
        m_audio.reset();
        m_model.reset();
        if (m_intf) { vlc_object_delete(m_intf); m_intf = nullptr; }
        if (m_instance) { libvlc_release(m_instance); m_instance = nullptr; }
        return stopped;
    }
    bool ensureOwner()
    {
        if (!m_intf->p_playlist) {
            m_intf->p_playlist = vlc_test_playback_main_playlist(m_instance);
            if (!m_intf->p_playlist) return false;
            m_intf->p_player = vlc_playlist_GetPlayer(m_intf->p_playlist);
        }
        return true;
    }
    QString nativeLog() const
    {
        QFile file(m_log);
        return file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll()) : QString{};
    }
    bool startOwned(const QString &explicitFixture = {}, bool play = true)
    {
        const auto fixture = explicitFixture.isEmpty()
            ? qEnvironmentVariable("VLC_PLAYBACK_CONSUMER_FIXTURE") : explicitFixture;
        if (!QFileInfo(fixture).isAbsolute() || !QFileInfo::exists(fixture) || !ensureOwner()) return false;
        const auto uri = QUrl::fromLocalFile(fixture).toEncoded();
        auto *media = input_item_New(uri.constData(), nullptr);
        if (!media) return false;
        vlc_playlist_Lock(m_intf->p_playlist);
        const size_t index = vlc_playlist_Count(m_intf->p_playlist);
        int result = vlc_playlist_InsertOne(m_intf->p_playlist, index, media);
        if (result == VLC_SUCCESS && index < m_observedMedia.size()) m_observedMedia[index] = media;
        if (result == VLC_SUCCESS && play) result = vlc_playlist_PlayAt(m_intf->p_playlist, index);
        vlc_playlist_Unlock(m_intf->p_playlist);
        input_item_Release(media);
        return result == VLC_SUCCESS;
    }
    QString shortFixture(int index) const
    {
        return QDir(qEnvironmentVariable("VLC_PLAYBACK_SHORT_FIXTURES")).filePath(
            QStringLiteral("clip-%1.wav").arg(index));
    }
    bool playFirst()
    {
        vlc_playlist_Lock(m_intf->p_playlist);
        const int result = vlc_playlist_PlayAt(m_intf->p_playlist, 0);
        vlc_playlist_Unlock(m_intf->p_playlist);
        return result == VLC_SUCCESS;
    }
    bool decodedBoth() const
    {
        TrackConsumerLocker lock(m_intf->p_player);
        const auto *stats = vlc_player_GetStatistics(m_intf->p_player);
        return stats && stats->i_decoded_video > 0 && stats->i_decoded_audio > 0;
    }
    bool decodedVideo() const
    {
        TrackConsumerLocker lock(m_intf->p_player);
        const auto *stats = vlc_player_GetStatistics(m_intf->p_player);
        return stats && stats->i_decoded_video > 0;
    }
    int videoHeight() const
    {
        TrackConsumerLocker lock(m_intf->p_player);
        if (vlc_player_GetTrackCount(m_intf->p_player, VIDEO_ES) != 1) return 0;
        const auto &format = vlc_player_GetTrackAt(m_intf->p_player, VIDEO_ES, 0)->fmt.video;
        return int(format.i_visible_height ? format.i_visible_height : format.i_height);
    }
    int selectedIndex(enum es_format_category_e category) const
    {
        TrackConsumerLocker lock(m_intf->p_player);
        const size_t count = vlc_player_GetTrackCount(m_intf->p_player, category);
        int selected = -1;
        for (size_t index = 0; index < count; ++index) {
            if (!vlc_player_GetTrackAt(m_intf->p_player, category, index)->selected) continue;
            if (selected != -1) return -2; // The fixture expects one selected track.
            selected = int(index);
        }
        return selected;
    }
    int trackCount(enum es_format_category_e category) const
    {
        TrackConsumerLocker lock(m_intf->p_player);
        return int(vlc_player_GetTrackCount(m_intf->p_player, category));
    }
    input_item_t *currentMedia() const
    {
        TrackConsumerLocker lock(m_intf->p_player);
        return vlc_player_GetCurrentMedia(m_intf->p_player);
    }
    float currentRate() const
    {
        TrackConsumerLocker lock(m_intf->p_player);
        return vlc_player_GetRate(m_intf->p_player);
    }
    vlc_tick_t currentAudioDelay() const
    {
        TrackConsumerLocker lock(m_intf->p_player);
        return vlc_player_GetAudioDelay(m_intf->p_player);
    }
    vlc_tick_t currentTime() const
    {
        TrackConsumerLocker lock(m_intf->p_player);
        return vlc_player_GetTime(m_intf->p_player);
    }
    bool waitForStablePausedClock()
    {
        QElapsedTimer deadline, stable;
        deadline.start(); stable.start();
        vlc_tick_t previous = currentTime();
        while (deadline.elapsed() < 5000) {
            if (state() != VLC_PLAYER_STATE_PAUSED) return false;
            const vlc_tick_t value = currentTime();
            if (value != previous) { previous = value; stable.restart(); }
            if (stable.elapsed() >= 1200) return true;
            QTest::qWait(20);
        }
        return false;
    }
    QString trackId(enum es_format_category_e category, size_t index) const
    {
        TrackConsumerLocker lock(m_intf->p_player);
        if (index >= vlc_player_GetTrackCount(m_intf->p_player, category)) return {};
        const auto *track = vlc_player_GetTrackAt(m_intf->p_player, category, index);
        return QString::fromUtf8(vlc_es_id_GetStrId(track->es_id));
    }
    static QByteArray bytes(const QString &path)
    {
        QFile file(path);
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray{};
    }
    static QString nativeText(const char *name)
    {
        char *raw = config_GetPsz(name);
        const auto text = QString::fromUtf8(raw ? raw : "");
        free(raw);
        return text;
    }
private slots:
    void initTestCase()
    {
        m_output = qEnvironmentVariable("VLC_PLAYBACK_CONSUMER_OUTPUT");
        QVERIFY(QFileInfo(m_output).isAbsolute());
        QVERIFY(!QFileInfo::exists(m_output));
        QVERIFY(QDir().mkpath(m_output));
        const auto userdata = QDir(m_output).filePath(QStringLiteral("userdata"));
        QVERIFY(QDir().mkpath(userdata));
        qputenv("VLC_USERDATA_PATH", userdata.toUtf8());
    }
    void init()
    {
        m_observedMedia = {};
        m_startedCount.store(0);
        m_transitionCount.store(0);
        const auto folder = QDir(m_output).filePath(QString::fromLatin1(QTest::currentTestFunction()));
        QVERIFY(QDir().mkpath(folder));
        m_config = QDir(folder).filePath(QStringLiteral("vlcrc"));
        QFile initialConfig(m_config);
        QVERIFY(initialConfig.open(QIODevice::WriteOnly));
        initialConfig.close(); // Explicit fresh defaults; no missing-file error.
        QVERIFY(openNative());
    }
    void cleanup() { QVERIFY2(closeNative(), "The owned native input did not acknowledge stop"); }

    void actualSelectedTracksConsumeSavedDefaultsOnReopen()
    {
        QVERIFY(m_model->setValue(QStringLiteral("defaultAudioTrack"), 1));
        QVERIFY(m_model->setValue(QStringLiteral("defaultSubtitleTrack"), 1));
        QVERIFY(closeNative());
        QVERIFY(openNative());
        QCOMPARE(m_model->values().value(QStringLiteral("defaultAudioTrack")).toInt(), 1);
        QCOMPARE(m_model->values().value(QStringLiteral("defaultSubtitleTrack")).toInt(), 1);
        QVERIFY(startOwned());
        QTRY_VERIFY_WITH_TIMEOUT(decodedBoth(), 10000);
        QTRY_COMPARE_WITH_TIMEOUT(trackCount(AUDIO_ES), 2, 10000);
        QTRY_COMPARE_WITH_TIMEOUT(trackCount(SPU_ES), 2, 10000);
        QTRY_COMPARE_WITH_TIMEOUT(selectedIndex(AUDIO_ES), 1, 10000);
        QTRY_COMPARE_WITH_TIMEOUT(selectedIndex(SPU_ES), 1, 10000);
        const auto audioSecond = trackId(AUDIO_ES, 1), subtitleSecond = trackId(SPU_ES, 1);
        QVERIFY(!audioSecond.isEmpty() && !subtitleSecond.isEmpty());
        {
            TrackConsumerLocker lock(m_intf->p_player);
            vlc_player_Pause(m_intf->p_player);
        }
        QTRY_COMPARE_WITH_TIMEOUT(state(), VLC_PLAYER_STATE_PAUSED, 10000);
        QVERIFY2(waitForStablePausedClock(), "The native paused clock did not settle before any preference write");
        const auto untouchedTime = currentTime();
        QTest::qWait(200); // No-write control before taking the mutation baseline.
        QCOMPARE(currentTime(), untouchedTime);
        input_item_t *input;
        vlc_tick_t clock;
        {
            TrackConsumerLocker lock(m_intf->p_player);
            input = vlc_player_GetCurrentMedia(m_intf->p_player);
            clock = vlc_player_GetTime(m_intf->p_player);
        }
        // Existing per-ID preferences must be cleared by an explicit index
        // choice. Do not set any media/player override to make selection pass.
        const auto audioId = audioSecond.toUtf8(), subtitleId = subtitleSecond.toUtf8();
        config_PutPsz("audio-track-id", audioId.constData());
        config_PutPsz("sub-track-id", subtitleId.constData());
        QVERIFY(m_model->setValue(QStringLiteral("defaultAudioTrack"), 0));
        QVERIFY(m_model->setValue(QStringLiteral("defaultSubtitleTrack"), 0));
        QVERIFY(nativeText("audio-track-id").isEmpty());
        QVERIFY(nativeText("sub-track-id").isEmpty());
        QTest::qWait(150);
        QCOMPARE(selectedIndex(AUDIO_ES), 1);
        QCOMPARE(selectedIndex(SPU_ES), 1);
        QCOMPARE(trackId(AUDIO_ES, 1), audioSecond);
        QCOMPARE(trackId(SPU_ES, 1), subtitleSecond);
        {
            TrackConsumerLocker lock(m_intf->p_player);
            QCOMPARE(vlc_player_GetState(m_intf->p_player), VLC_PLAYER_STATE_PAUSED);
            QCOMPARE(vlc_player_GetCurrentMedia(m_intf->p_player), input);
            QCOMPARE(vlc_player_GetTime(m_intf->p_player), clock);
        }
        const auto saved = bytes(m_config);
        QVERIFY(!saved.isEmpty());
        auto *const owner = m_intf->p_player;
        QVERIFY(startOwned()); // Same native owner, a different input item.
        QTRY_VERIFY_WITH_TIMEOUT(currentMedia() != input, 10000);
        QTRY_VERIFY_WITH_TIMEOUT(decodedBoth(), 10000);
        QTRY_COMPARE_WITH_TIMEOUT(selectedIndex(AUDIO_ES), 0, 10000);
        QTRY_COMPARE_WITH_TIMEOUT(selectedIndex(SPU_ES), 0, 10000);
        QCOMPARE(m_intf->p_player, owner);
        QCOMPARE(trackCount(AUDIO_ES), 2);
        QCOMPARE(trackCount(SPU_ES), 2);
        QCOMPARE(bytes(m_config), saved);
        QVERIFY(closeNative());
        QVERIFY(openNative());
        QVERIFY(startOwned());
        QTRY_VERIFY_WITH_TIMEOUT(decodedBoth(), 10000);
        QTRY_COMPARE_WITH_TIMEOUT(selectedIndex(AUDIO_ES), 0, 10000);
        QTRY_COMPARE_WITH_TIMEOUT(selectedIndex(SPU_ES), 0, 10000);
        QCOMPARE(trackCount(AUDIO_ES), 2);
        QCOMPARE(trackCount(SPU_ES), 2);
        QCOMPARE(bytes(m_config), saved);
    }

    void actualAdaptiveResolutionConsumesSavedMaximumOnNextInput()
    {
        const auto fixture = qEnvironmentVariable("VLC_PLAYBACK_HLS_FIXTURE");
        QVERIFY2(QFileInfo(fixture).isAbsolute() && QFileInfo::exists(fixture),
                 "Provide the generated local two-rendition HLS master");
        QVERIFY(m_model->setValue(QStringLiteral("preferredVideoQuality"), 240));
        QVERIFY(startOwned(fixture));
        QTRY_VERIFY_WITH_TIMEOUT(decodedVideo(), 10000);
        QTRY_COMPARE_WITH_TIMEOUT(videoHeight(), 240, 10000);
        {
            TrackConsumerLocker lock(m_intf->p_player);
            vlc_player_Pause(m_intf->p_player);
        }
        QTRY_COMPARE_WITH_TIMEOUT(state(), VLC_PLAYER_STATE_PAUSED, 10000);
        QVERIFY2(waitForStablePausedClock(), "The native paused clock did not settle before any preference write");
        const auto untouchedTime = currentTime();
        QTest::qWait(200); // No-write control before taking the mutation baseline.
        QCOMPARE(currentTime(), untouchedTime);
        input_item_t *input;
        vlc_tick_t clock;
        {
            TrackConsumerLocker lock(m_intf->p_player);
            input = vlc_player_GetCurrentMedia(m_intf->p_player);
            clock = vlc_player_GetTime(m_intf->p_player);
        }
        QVERIFY(m_model->setValue(QStringLiteral("preferredVideoQuality"), 720));
        QTest::qWait(150);
        QCOMPARE(videoHeight(), 240);
        {
            TrackConsumerLocker lock(m_intf->p_player);
            QCOMPARE(vlc_player_GetState(m_intf->p_player), VLC_PLAYER_STATE_PAUSED);
            QCOMPARE(vlc_player_GetCurrentMedia(m_intf->p_player), input);
            QCOMPARE(vlc_player_GetTime(m_intf->p_player), clock);
        }
        const auto saved = bytes(m_config);
        auto *const owner = m_intf->p_player;
        QVERIFY(startOwned(fixture));
        QTRY_VERIFY_WITH_TIMEOUT(currentMedia() != input, 10000);
        QTRY_VERIFY_WITH_TIMEOUT(decodedVideo(), 10000);
        QTRY_COMPARE_WITH_TIMEOUT(videoHeight(), 360, 10000);
        QCOMPARE(m_intf->p_player, owner);
        QCOMPARE(bytes(m_config), saved);
        QVERIFY(closeNative());
        QVERIFY(openNative());
        QVERIFY(startOwned(fixture));
        QTRY_VERIFY_WITH_TIMEOUT(decodedVideo(), 10000);
        QTRY_COMPARE_WITH_TIMEOUT(videoHeight(), 360, 10000);
        QCOMPARE(bytes(m_config), saved);
    }

    void actualStartPlaybackPolicyAffectsNextInputWithoutResumingCurrent()
    {
        // Like MainCtx, this transport setting needs the real native owner
        // before it can advertise or apply a live next-start policy.
        QVERIFY(ensureOwner());
        m_model->reload();
        QVERIFY2(m_model->setValue(QStringLiteral("startPlayback"), false), qPrintable(m_model->lastError()));
        QVERIFY(closeNative());
        QVERIFY(openNative());
        QVERIFY(startOwned());
        QTRY_COMPARE_WITH_TIMEOUT(state(), VLC_PLAYER_STATE_PAUSED, 10000);
        QTRY_VERIFY_WITH_TIMEOUT(currentMedia() != nullptr, 10000);
        QVERIFY2(waitForStablePausedClock(), "The native paused clock did not settle before any preference write");
        const auto untouchedTime = currentTime();
        QTest::qWait(200); // No-write control before taking the mutation baseline.
        QCOMPARE(currentTime(), untouchedTime);
        input_item_t *input;
        vlc_tick_t clock;
        {
            TrackConsumerLocker lock(m_intf->p_player);
            input = vlc_player_GetCurrentMedia(m_intf->p_player);
            clock = vlc_player_GetTime(m_intf->p_player);
        }
        QVERIFY(m_model->setValue(QStringLiteral("startPlayback"), true));
        QVERIFY(!m_model->restartRequired());
        QTest::qWait(150);
        {
            TrackConsumerLocker lock(m_intf->p_player);
            QCOMPARE(vlc_player_GetState(m_intf->p_player), VLC_PLAYER_STATE_PAUSED);
            QCOMPARE(vlc_player_GetCurrentMedia(m_intf->p_player), input);
            QCOMPARE(vlc_player_GetTime(m_intf->p_player), clock);
        }
        const auto saved = bytes(m_config);
        auto *const owner = m_intf->p_player;
        QVERIFY(startOwned());
        QTRY_VERIFY_WITH_TIMEOUT(currentMedia() != input, 10000);
        QTRY_COMPARE_WITH_TIMEOUT(state(), VLC_PLAYER_STATE_PLAYING, 10000);
        QTRY_VERIFY_WITH_TIMEOUT(decodedBoth(), 10000);
        QCOMPARE(m_intf->p_player, owner);
        QCOMPARE(bytes(m_config), saved);
        QVERIFY(closeNative());
        QVERIFY(openNative());
        QVERIFY(startOwned());
        QTRY_COMPARE_WITH_TIMEOUT(state(), VLC_PLAYER_STATE_PLAYING, 10000);
        QTRY_VERIFY_WITH_TIMEOUT(decodedBoth(), 10000);
        QCOMPARE(bytes(m_config), saved);
    }

    void actualSavedRateRequiresNewPlayerAndPreservesCurrentRate()
    {
        QVERIFY(startOwned());
        QTRY_VERIFY_WITH_TIMEOUT(decodedBoth(), 10000);
        QCOMPARE(currentRate(), 1.f);
        {
            TrackConsumerLocker lock(m_intf->p_player);
            vlc_player_Pause(m_intf->p_player);
        }
        QTRY_COMPARE_WITH_TIMEOUT(state(), VLC_PLAYER_STATE_PAUSED, 10000);
        QVERIFY2(waitForStablePausedClock(), "The native paused clock did not settle before any preference write");
        const auto untouchedTime = currentTime();
        QTest::qWait(200); // No-write control before taking the mutation baseline.
        QCOMPARE(currentTime(), untouchedTime);
        input_item_t *input;
        vlc_tick_t clock;
        {
            TrackConsumerLocker lock(m_intf->p_player);
            input = vlc_player_GetCurrentMedia(m_intf->p_player);
            clock = vlc_player_GetTime(m_intf->p_player);
        }
        QVERIFY(m_model->setValue(QStringLiteral("defaultPlaybackSpeed"), 1.5));
        QVERIFY(m_model->restartRequired());
        QCOMPARE(currentRate(), 1.f);
        {
            TrackConsumerLocker lock(m_intf->p_player);
            QCOMPARE(vlc_player_GetState(m_intf->p_player), VLC_PLAYER_STATE_PAUSED);
            QCOMPARE(vlc_player_GetCurrentMedia(m_intf->p_player), input);
            QCOMPARE(vlc_player_GetTime(m_intf->p_player), clock);
        }
        const auto saved = bytes(m_config);
        auto *const owner = m_intf->p_player;
        QVERIFY(startOwned());
        QTRY_VERIFY_WITH_TIMEOUT(currentMedia() != input, 10000);
        QTRY_VERIFY_WITH_TIMEOUT(decodedBoth(), 10000);
        QCOMPARE(m_intf->p_player, owner);
        QCOMPARE(currentRate(), 1.f);
        QVERIFY(closeNative());
        QVERIFY(openNative());
        QVERIFY(!m_model->restartRequired());
        QVERIFY(startOwned());
        QTRY_VERIFY_WITH_TIMEOUT(decodedBoth(), 10000);
        QCOMPARE(currentRate(), 1.5f);
        QCOMPARE(bytes(m_config), saved);
    }

    void actualRepeatCurrentRestartsOnlyTheSameFiniteInput()
    {
        QVERIFY(ensureOwner());
        QVERIFY(m_model->setValue(QStringLiteral("repeatMode"), int(VLC_PLAYLIST_PLAYBACK_REPEAT_CURRENT)));
        QVERIFY(m_model->setValue(QStringLiteral("shuffle"), false));
        QVERIFY(startOwned(shortFixture(0), false));
        QVERIFY(observeStarts());
        QVERIFY(playFirst());
        QTRY_VERIFY_WITH_TIMEOUT(m_transitionCount.load(std::memory_order_acquire) >= 3, 10000);
        for (unsigned index = 0; index < 3; ++index) QCOMPARE(m_transitionIndices[index], 0);
    }

    void actualRepeatAllTraversesFiniteInputsThenWraps()
    {
        QVERIFY(ensureOwner());
        QVERIFY(m_model->setValue(QStringLiteral("repeatMode"), int(VLC_PLAYLIST_PLAYBACK_REPEAT_ALL)));
        QVERIFY(m_model->setValue(QStringLiteral("shuffle"), false));
        for (int index = 0; index < 3; ++index) QVERIFY(startOwned(shortFixture(index), false));
        QVERIFY(observeStarts());
        QVERIFY(playFirst());
        QTRY_VERIFY_WITH_TIMEOUT(m_transitionCount.load(std::memory_order_acquire) >= 4, 10000);
        const std::array<int, 4> expected{0, 1, 2, 0};
        for (unsigned index = 0; index < expected.size(); ++index) QCOMPARE(m_transitionIndices[index], expected[index]);
    }

    void actualStopAfterEachItemDoesNotStartTheNextQueuedInput()
    {
        QVERIFY(ensureOwner());
        QVERIFY(m_model->setValue(QStringLiteral("repeatMode"), int(VLC_PLAYLIST_PLAYBACK_REPEAT_NONE)));
        QVERIFY(m_model->setValue(QStringLiteral("stopAfterEachItem"), true));
        for (int index = 0; index < 3; ++index) QVERIFY(startOwned(shortFixture(index), false));
        QVERIFY(observeStarts());
        QVERIFY(playFirst());
        QTRY_VERIFY_WITH_TIMEOUT(m_startedCount.load(std::memory_order_acquire) >= 1, 10000);
        QTRY_COMPARE_WITH_TIMEOUT(state(), VLC_PLAYER_STATE_STOPPED, 10000);
        QTest::qWait(200);
        QCOMPARE(m_startedCount.load(std::memory_order_acquire), 1u);
        QCOMPARE(m_startedIndices[0], 0);
        QCOMPARE(m_transitionCount.load(std::memory_order_acquire), 1u);
        QCOMPARE(m_transitionIndices[0], 0);
    }

    void actualFinishActionRequestsExitAfterFiniteQueueEnd()
    {
        QVERIFY(ensureOwner());
        QVERIFY(m_model->setValue(QStringLiteral("repeatMode"), int(VLC_PLAYLIST_PLAYBACK_REPEAT_NONE)));
        QVERIFY(m_model->setValue(QStringLiteral("stopAfterEachItem"), false));
        QVERIFY(m_model->setValue(QStringLiteral("finishAction"), 1));
        QVERIFY(!m_model->restartRequired());
        for (int index = 0; index < 3; ++index) QVERIFY(startOwned(shortFixture(index), false));
        QVERIFY(observeStarts());
        QVERIFY(playFirst());
        QTRY_VERIFY_WITH_TIMEOUT(m_transitionCount.load(std::memory_order_acquire) >= 3, 10000);
        QTRY_COMPARE_WITH_TIMEOUT(state(), VLC_PLAYER_STATE_STOPPED, 10000);
        QTRY_COMPARE_WITH_TIMEOUT(m_exitCount.load(), 1u, 5000);
        QCOMPARE(m_transitionCount.load(std::memory_order_acquire), 3u);
        for (unsigned index = 0; index < 3; ++index) QCOMPARE(m_transitionIndices[index], int(index));
    }

    void actualFinishActionChangesLiveAndManualStopRequestsExit()
    {
        QVERIFY(ensureOwner());
        QVERIFY(m_model->setValue(QStringLiteral("finishAction"), 0));
        QVERIFY(m_model->setValue(QStringLiteral("stopAfterEachItem"), true));
        QVERIFY(startOwned());
        QTRY_VERIFY_WITH_TIMEOUT(decodedBoth(), 10000);
        {
            TrackConsumerLocker lock(m_intf->p_player);
            vlc_player_Pause(m_intf->p_player);
        }
        QTRY_COMPARE_WITH_TIMEOUT(state(), VLC_PLAYER_STATE_PAUSED, 10000);
        QVERIFY(waitForStablePausedClock());
        const auto input = currentMedia(); const auto clock = currentTime();
        QTest::qWait(200); QCOMPARE(currentTime(), clock);
        QVERIFY(m_model->setValue(QStringLiteral("finishAction"), 1));
        QVERIFY(!m_model->capabilities().value(QStringLiteral("stopAfterEachItem")).toBool());
        QVERIFY(config_GetInt("play-and-stop"));
        QVERIFY(config_GetInt("play-and-exit"));
        QVERIFY(!m_model->restartRequired());
        QTest::qWait(150);
        QCOMPARE(m_exitCount.load(), 0u);
        QCOMPARE(state(), VLC_PLAYER_STATE_PAUSED);
        QCOMPARE(currentMedia(), input); QCOMPARE(currentTime(), clock);
        {
            TrackConsumerLocker lock(m_intf->p_player);
            vlc_playlist_Stop(m_intf->p_playlist);
        }
        QTRY_COMPARE_WITH_TIMEOUT(state(), VLC_PLAYER_STATE_STOPPED, 10000);
        QTRY_COMPARE_WITH_TIMEOUT(m_exitCount.load(), 1u, 5000);
    }

    void actualStopFinishActionRestoresSavedStopAfterWithoutExit()
    {
        QVERIFY(ensureOwner());
        QVERIFY(m_model->setValue(QStringLiteral("stopAfterEachItem"), true));
        QVERIFY(m_model->setValue(QStringLiteral("finishAction"), 1));
        QVERIFY(m_model->setValue(QStringLiteral("finishAction"), 0));
        QVERIFY(m_model->capabilities().value(QStringLiteral("stopAfterEachItem")).toBool());
        QVERIFY(config_GetInt("play-and-stop"));
        QVERIFY(!config_GetInt("play-and-exit"));
        const auto saved = bytes(m_config);
        QVERIFY(closeNative()); QVERIFY(openNative()); QVERIFY(ensureOwner());
        QCOMPARE(bytes(m_config), saved);
        for (int index = 0; index < 3; ++index) QVERIFY(startOwned(shortFixture(index), false));
        QVERIFY(observeStarts()); QVERIFY(playFirst());
        QTRY_VERIFY_WITH_TIMEOUT(m_startedCount.load(std::memory_order_acquire) >= 1, 10000);
        QTRY_COMPARE_WITH_TIMEOUT(state(), VLC_PLAYER_STATE_STOPPED, 10000);
        QTest::qWait(200);
        QCOMPARE(m_transitionCount.load(std::memory_order_acquire), 1u);
        QCOMPARE(m_transitionIndices[0], 0);
        QCOMPARE(m_exitCount.load(), 0u);
    }

    void actualShuffleUsesRandomOrderWithoutRepeatingOrLosingInputs()
    {
        QVERIFY(ensureOwner());
        QVERIFY(m_model->setValue(QStringLiteral("repeatMode"), int(VLC_PLAYLIST_PLAYBACK_REPEAT_NONE)));
        QVERIFY(m_model->setValue(QStringLiteral("stopAfterEachItem"), false));
        QVERIFY(m_model->setValue(QStringLiteral("shuffle"), true));
        {
            TrackConsumerLocker lock(m_intf->p_player);
            QCOMPARE(vlc_playlist_GetPlaybackOrder(m_intf->p_playlist), VLC_PLAYLIST_PLAYBACK_ORDER_RANDOM);
        }
        for (int index = 0; index < 3; ++index) QVERIFY(startOwned(shortFixture(index), false));
        QVERIFY(observeStarts());
        QVERIFY(playFirst());
        QTRY_VERIFY_WITH_TIMEOUT(m_transitionCount.load(std::memory_order_acquire) >= 3, 10000);
        QTRY_COMPARE_WITH_TIMEOUT(state(), VLC_PLAYER_STATE_STOPPED, 10000);
        QCOMPARE(m_transitionCount.load(std::memory_order_acquire), 3u);
        std::array<int, 3> counts{};
        for (unsigned index = 0; index < 3; ++index) {
            QVERIFY(m_transitionIndices[index] >= 0 && m_transitionIndices[index] < 3);
            ++counts[size_t(m_transitionIndices[index])];
        }
        for (int count : counts) QCOMPARE(count, 1);
    }

    void actualTimeShiftConstructorConsumesNextInputDefaults()
    {
        const auto directory = QFileInfo(m_config).absolutePath();
        const auto firstFolder = QDir(directory).filePath(QStringLiteral("buffer-a"));
        const auto nextFolder = QDir(directory).filePath(QStringLiteral("buffer-b"));
        QVERIFY(QDir().mkpath(firstFolder)); QVERIFY(QDir().mkpath(nextFolder));
        QVERIFY(m_model->setValue(QStringLiteral("timeShiftFolder"), firstFolder));
        QVERIFY(m_model->setValue(QStringLiteral("timeShiftSize"), 1024*1024));
        QVERIFY(startOwned());
        QTRY_VERIFY_WITH_TIMEOUT(decodedBoth(), 10000);
        const QString firstPathMessage = QStringLiteral("using timeshift path: ") + firstFolder;
        const QString nextPathMessage = QStringLiteral("using timeshift path: ") + nextFolder;
        QTRY_VERIFY_WITH_TIMEOUT(QDir::fromNativeSeparators(nativeLog()).contains(firstPathMessage), 10000);
        QVERIFY(nativeLog().contains(QStringLiteral("using timeshift granularity of 1 MiB")));
        {
            TrackConsumerLocker lock(m_intf->p_player);
            vlc_player_Pause(m_intf->p_player);
        }
        QTRY_COMPARE_WITH_TIMEOUT(state(), VLC_PLAYER_STATE_PAUSED, 10000);
        QVERIFY(waitForStablePausedClock());
        const auto input = currentMedia(); const auto clock = currentTime();
        QTest::qWait(200); QCOMPARE(currentTime(), clock);
        const auto constructorCount = nativeLog().count(QStringLiteral("using timeshift granularity of "));
        QVERIFY(m_model->setValue(QStringLiteral("timeShiftFolder"), nextFolder));
        QVERIFY(m_model->setValue(QStringLiteral("timeShiftSize"), 2*1024*1024));
        QVERIFY(!m_model->restartRequired());
        const auto saved = bytes(m_config);
        QTest::qWait(150);
        QCOMPARE(state(), VLC_PLAYER_STATE_PAUSED);
        QCOMPARE(currentMedia(), input); QCOMPARE(currentTime(), clock);
        QCOMPARE(nativeLog().count(QStringLiteral("using timeshift granularity of ")), constructorCount);
        QVERIFY(!QDir::fromNativeSeparators(nativeLog()).contains(nextPathMessage));
        auto *owner = m_intf->p_player;
        QVERIFY(startOwned());
        QTRY_VERIFY_WITH_TIMEOUT(currentMedia() != input, 10000);
        QTRY_VERIFY_WITH_TIMEOUT(decodedBoth(), 10000);
        QCOMPARE(m_intf->p_player, owner);
        QTRY_VERIFY_WITH_TIMEOUT(QDir::fromNativeSeparators(nativeLog()).contains(nextPathMessage), 10000);
        QVERIFY(nativeLog().contains(QStringLiteral("using timeshift granularity of 2 MiB")));
        QVERIFY(closeNative()); QVERIFY(openNative()); QVERIFY(startOwned());
        QTRY_VERIFY_WITH_TIMEOUT(decodedBoth(), 10000);
        QTRY_VERIFY_WITH_TIMEOUT(QDir::fromNativeSeparators(nativeLog()).contains(nextPathMessage), 10000);
        QVERIFY(nativeLog().contains(QStringLiteral("using timeshift granularity of 2 MiB")));
        QCOMPARE(bytes(m_config), saved);
        // These are the actual Core constructor's messages. Local MKV does not
        // establish live-source pause buffering, disk writes or chunk rollover.
    }

    void actualTimeShiftConstructorUsesOwnedSystemTempAndNativeChunkDefault()
    {
        QVERIFY(m_model->setValue(QStringLiteral("timeShiftFolder"), QString{}));
        QVERIFY(m_model->setValue(QStringLiteral("timeShiftSize"), -1));
        QVERIFY(startOwned());
        QTRY_VERIFY_WITH_TIMEOUT(decodedBoth(), 10000);
        const QString expected = QStringLiteral("using timeshift path: ")
            + QDir::fromNativeSeparators(qEnvironmentVariable("TEMP"));
        QVERIFY(!qEnvironmentVariable("TEMP").isEmpty());
        QTRY_VERIFY_WITH_TIMEOUT(QDir::fromNativeSeparators(nativeLog()).contains(expected), 10000);
        QVERIFY(nativeLog().contains(QStringLiteral("using timeshift granularity of 50 MiB")));
        QVERIFY(!m_model->restartRequired());
    }

    void actualPreferredAudioLanguageUsesSavedStartupDefault()
    {
        QVERIFY(m_model->setValue(QStringLiteral("defaultAudioTrack"), -1));
        QVERIFY(m_audio->setValue(QStringLiteral("preferredLanguage"), QStringLiteral("eng")));
        QVERIFY(closeNative());
        QVERIFY(openNative());
        QVERIFY(startOwned());
        QTRY_VERIFY_WITH_TIMEOUT(decodedBoth(), 10000);
        QTRY_COMPARE_WITH_TIMEOUT(selectedIndex(AUDIO_ES), 0, 10000);
        {
            TrackConsumerLocker lock(m_intf->p_player);
            vlc_player_Pause(m_intf->p_player);
        }
        QTRY_COMPARE_WITH_TIMEOUT(state(), VLC_PLAYER_STATE_PAUSED, 10000);
        QVERIFY(waitForStablePausedClock());
        const auto input = currentMedia();
        const auto clock = currentTime();
        QTest::qWait(200);
        QCOMPARE(currentTime(), clock);
        QVERIFY(m_audio->setValue(QStringLiteral("preferredLanguage"), QStringLiteral("fin")));
        QVERIFY(m_audio->restartRequired());
        QTest::qWait(150);
        QCOMPARE(state(), VLC_PLAYER_STATE_PAUSED);
        QCOMPARE(currentMedia(), input);
        QCOMPARE(currentTime(), clock);
        QCOMPARE(selectedIndex(AUDIO_ES), 0);
        auto *const owner = m_intf->p_player;
        QVERIFY(startOwned());
        QTRY_VERIFY_WITH_TIMEOUT(currentMedia() != input, 10000);
        QTRY_VERIFY_WITH_TIMEOUT(decodedBoth(), 10000);
        QCOMPARE(m_intf->p_player, owner);
        QCOMPARE(selectedIndex(AUDIO_ES), 0);
        const auto saved = bytes(m_config);
        QVERIFY(closeNative());
        QVERIFY(openNative());
        QVERIFY(startOwned());
        QTRY_VERIFY_WITH_TIMEOUT(decodedBoth(), 10000);
        QTRY_COMPARE_WITH_TIMEOUT(selectedIndex(AUDIO_ES), 1, 10000);
        QCOMPARE(bytes(m_config), saved);
    }

    void actualAudioEnabledIsAStartupPolicyAndPreservesCurrentTrack()
    {
        QVERIFY(m_model->setValue(QStringLiteral("defaultAudioTrack"), -1));
        QVERIFY(m_audio->setValue(QStringLiteral("audioEnabled"), true));
        QVERIFY(startOwned());
        QTRY_VERIFY_WITH_TIMEOUT(decodedBoth(), 10000);
        QTRY_VERIFY_WITH_TIMEOUT(selectedIndex(AUDIO_ES) >= 0, 10000);
        const int selected = selectedIndex(AUDIO_ES);
        {
            TrackConsumerLocker lock(m_intf->p_player);
            vlc_player_Pause(m_intf->p_player);
        }
        QTRY_COMPARE_WITH_TIMEOUT(state(), VLC_PLAYER_STATE_PAUSED, 10000);
        QVERIFY(waitForStablePausedClock());
        const auto input = currentMedia();
        const auto clock = currentTime();
        QTest::qWait(200);
        QCOMPARE(currentTime(), clock);
        QVERIFY(m_audio->setValue(QStringLiteral("audioEnabled"), false));
        QVERIFY(m_audio->restartRequired());
        QTest::qWait(150);
        QCOMPARE(state(), VLC_PLAYER_STATE_PAUSED);
        QCOMPARE(currentMedia(), input);
        QCOMPARE(currentTime(), clock);
        QCOMPARE(selectedIndex(AUDIO_ES), selected);
        auto *const owner = m_intf->p_player;
        QVERIFY(startOwned());
        QTRY_VERIFY_WITH_TIMEOUT(currentMedia() != input, 10000);
        QTRY_VERIFY_WITH_TIMEOUT(decodedBoth(), 10000);
        QCOMPARE(m_intf->p_player, owner);
        QCOMPARE(selectedIndex(AUDIO_ES), selected);
        const auto saved = bytes(m_config);
        QVERIFY(closeNative());
        QVERIFY(openNative());
        QVERIFY(startOwned());
        QTRY_VERIFY_WITH_TIMEOUT(decodedVideo(), 10000);
        QCOMPARE(trackCount(AUDIO_ES), 2);
        QCOMPARE(selectedIndex(AUDIO_ES), -1);
        QCOMPARE(bytes(m_config), saved);
    }

    void actualSavedAudioDelayIsConsumedByNextInput()
    {
        QVERIFY(startOwned());
        QTRY_VERIFY_WITH_TIMEOUT(decodedBoth(), 10000);
        QCOMPARE(currentAudioDelay(), vlc_tick_t(0));
        {
            TrackConsumerLocker lock(m_intf->p_player);
            vlc_player_Pause(m_intf->p_player);
        }
        QTRY_COMPARE_WITH_TIMEOUT(state(), VLC_PLAYER_STATE_PAUSED, 10000);
        QVERIFY(waitForStablePausedClock());
        const auto input = currentMedia();
        const auto clock = currentTime();
        QTest::qWait(200);
        QCOMPARE(currentTime(), clock);
        QVERIFY(m_audio->setValue(QStringLiteral("audioDelayMs"), 125));
        QVERIFY2(!m_audio->restartRequired(), "The actual next-input audio-delay consumer does not require a new player");
        QTest::qWait(150);
        QCOMPARE(state(), VLC_PLAYER_STATE_PAUSED);
        QCOMPARE(currentMedia(), input);
        QCOMPARE(currentTime(), clock);
        QCOMPARE(currentAudioDelay(), vlc_tick_t(0));
        auto *const owner = m_intf->p_player;
        QVERIFY(startOwned());
        QTRY_VERIFY_WITH_TIMEOUT(currentMedia() != input, 10000);
        QTRY_VERIFY_WITH_TIMEOUT(decodedBoth(), 10000);
        QCOMPARE(m_intf->p_player, owner);
        QCOMPARE(currentAudioDelay(), VLC_TICK_FROM_MS(125));
        const auto saved = bytes(m_config);
        QVERIFY(closeNative());
        QVERIFY(openNative());
        QVERIFY(startOwned());
        QTRY_VERIFY_WITH_TIMEOUT(decodedBoth(), 10000);
        QCOMPARE(currentAudioDelay(), VLC_TICK_FROM_MS(125));
        QCOMPARE(bytes(m_config), saved);
        QVERIFY(!m_audio->restartRequired());
    }
};
QTEST_GUILESS_MAIN(TestPlaybackTrackConsumers)
#include "test_playback_track_consumers.moc"
