// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "player/audio_equalizer.hpp"
#include <vlc/vlc.h>
#include <QSignalSpy>
#include <QTest>
#include <atomic>
#include <cmath>
#include <limits>

extern "C" vlc_player_t *vlc_test_media_player_core(libvlc_media_player_t *);

class TestAudioEqualizer : public QObject
{
    Q_OBJECT
    libvlc_instance_t *m_instance = nullptr;
    libvlc_media_player_t *m_mediaPlayer = nullptr;
    std::unique_ptr<AudioEqualizer> m_equalizer;
    std::atomic<float> m_rms{0.f};
    std::atomic<unsigned> m_buffers{0};
    static void samples(void *opaque, const void *data, unsigned count, int64_t)
    {
        auto *self = static_cast<TestAudioEqualizer *>(opaque);
        const auto *values = static_cast<const float *>(data);
        double sum = 0;
        for (unsigned i = 0; i < count; ++i) sum += values[i] * values[i];
        self->m_rms.store(count ? std::sqrt(sum / count) : 0.f);
        self->m_buffers.fetch_add(1);
    }
private slots:
    void initTestCase()
    {
        const auto fixture = qEnvironmentVariable("VLC_EQUALIZER_FIXTURE");
        if (fixture.isEmpty()) QSKIP("Set VLC_EQUALIZER_FIXTURE to the generated audio WAV for native output checks");
        const char *args[] = {"--ignore-config", "--no-media-library", "--verbose=2", "--file-logging", "--logfile=audio-equalizer-native.log"};
        m_instance = libvlc_new(ARRAY_SIZE(args), args);
        QVERIFY(m_instance);
        m_mediaPlayer = libvlc_media_player_new(m_instance, nullptr, nullptr);
        QVERIFY(m_mediaPlayer);
        libvlc_audio_set_callbacks(m_mediaPlayer, samples, nullptr, nullptr, nullptr, nullptr, this);
        libvlc_audio_set_format(m_mediaPlayer, "FL32", 44100, 1);
        auto *media = libvlc_media_new_path(qtu(fixture));
        QVERIFY(media);
        libvlc_media_player_set_media(m_mediaPlayer, media);
        libvlc_media_release(media);
        QCOMPARE(libvlc_media_player_play(m_mediaPlayer), 0);
        QTRY_VERIFY_WITH_TIMEOUT(m_buffers.load() > 2, 10000);
        m_equalizer = std::make_unique<AudioEqualizer>(vlc_test_media_player_core(m_mediaPlayer));
        m_equalizer->refresh();
        QVERIFY(m_equalizer->available());
    }
    void engineSynchronizationAndConsecutiveEdits()
    {
        QSignalSpy updates(m_equalizer.get(), &AudioEqualizer::stateChanged);
        auto *player = vlc_test_media_player_core(m_mediaPlayer);
        vlc_player_Lock(player);
        auto *output = vlc_player_aout_Hold(player);
        vlc_player_Unlock(player);
        QVERIFY(output);
        QCOMPARE(var_SetFloat(output, "equalizer-preamp", 7.f), VLC_SUCCESS);
        QTRY_VERIFY(updates.count() > 0);
        QCOMPARE(m_equalizer->preamp(), 7.f);
        // Both edits must survive before queued observer callbacks are processed.
        m_equalizer->setBand(0, -5.f);
        m_equalizer->setBand(1, 6.f);
        QCOMPARE(m_equalizer->bands()[0].toFloat(), -5.f);
        QCOMPARE(m_equalizer->bands()[1].toFloat(), 6.f);
        aout_Release(output);
    }
    void validationAndDisabledPreset()
    {
        m_equalizer->setEnabled(false);
        m_equalizer->setPreset(3);
        QCOMPARE(m_equalizer->presetIndex(), 3);
        QCOMPARE(m_equalizer->preamp(), 5.f);
        QCOMPARE(m_equalizer->bands()[0].toFloat(), 9.6f);
        const auto before = m_equalizer->bands();
        m_equalizer->setBand(-1, 2.f);
        m_equalizer->setBand(10, 2.f);
        m_equalizer->setBand(0, std::numeric_limits<float>::quiet_NaN());
        m_equalizer->setPreset(1000);
        QCOMPARE(m_equalizer->bands(), before);
        m_equalizer->setPreamp(std::numeric_limits<float>::infinity());
        QCOMPARE(m_equalizer->preamp(), 5.f);
        m_equalizer->setPreamp(100.f);
        QCOMPARE(m_equalizer->preamp(), 20.f);
    }
    void actualFilteredAudio()
    {
        m_equalizer->setPreset(0);
        m_equalizer->setEnabled(true);
        QTRY_VERIFY(m_equalizer->enabled());
        auto buffers = m_buffers.load();
        QTest::qWait(400);
        QVERIFY(m_buffers.load() > buffers);
        const float before = m_rms.load();
        QVERIFY(before > 0.001f);
        m_equalizer->setPreamp(-12.f);
        QTest::qWait(400);
        const float quieter = m_rms.load();
        qInfo("Native PCM RMS before=%f after=-12dB:%f", before, quieter);
        QVERIFY2(quieter < before * 0.3f, "The equalizer did not reduce actual audio output");
        m_equalizer->setEnabled(false);
        QTest::qWait(400);
        const float restored = m_rms.load();
        qInfo("Native PCM RMS after disabling=%f", restored);
        QVERIFY(restored > quieter * 3.f);
    }
    void cleanupTestCase()
    {
        m_equalizer.reset();
        if (m_mediaPlayer) { libvlc_media_player_stop_async(m_mediaPlayer); libvlc_media_player_release(m_mediaPlayer); }
        if (m_instance) libvlc_release(m_instance);
    }
};
QTEST_GUILESS_MAIN(TestAudioEqualizer)
#include "test_audio_equalizer.moc"
