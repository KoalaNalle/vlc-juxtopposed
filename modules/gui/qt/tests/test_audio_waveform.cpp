// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "player/audio_waveform.hpp"
#include <vlc/vlc.h>
#include <vlc_player.h>
#include <QDataStream>
#include <QElapsedTimer>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>
#include <cmath>

extern "C" vlc_object_t *vlc_test_media_player_object(libvlc_media_player_t *);

class TestAudioWaveform : public QObject
{
    Q_OBJECT
    libvlc_instance_t *m_instance = nullptr;
    libvlc_media_player_t *m_player = nullptr;
    std::unique_ptr<AudioWaveform> m_waveform;
    QTemporaryDir m_directory;
    QUrl m_fixture;
    std::atomic_uint m_audioBuffers{0};
    static void audio(void *opaque, const void *, unsigned, int64_t)
    {
        ++static_cast<TestAudioWaveform *>(opaque)->m_audioBuffers;
    }

    QUrl wav(const QString &name, const QList<qint16> &samples)
    {
        const auto path = m_directory.filePath(name);
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) return {};
        QDataStream stream(&file);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream.writeRawData("RIFF", 4);
        stream << quint32(36 + samples.size() * 2);
        stream.writeRawData("WAVEfmt ", 8);
        stream << quint32(16) << quint16(1) << quint16(1) << quint32(8000)
               << quint32(16000) << quint16(2) << quint16(16);
        stream.writeRawData("data", 4);
        stream << quint32(samples.size() * 2);
        for (qint16 sample : samples) stream << sample;
        return QUrl::fromLocalFile(path);
    }
private slots:
    void initTestCase()
    {
        const auto fixture = qEnvironmentVariable("VLC_WAVEFORM_FIXTURE");
        if (fixture.isEmpty()) QSKIP("Set VLC_WAVEFORM_FIXTURE to the generated 30-second audio WAV");
        QVERIFY(m_directory.isValid());
        const auto copy = m_directory.filePath("source 'quoted, audio.wav");
        QVERIFY(QFile::copy(fixture, copy));
        m_fixture = QUrl::fromLocalFile(copy);
        const char *args[] = {"--ignore-config", "--no-media-library", "--verbose=2", "--file-logging", "--logfile=audio-waveform-native.log"};
        m_instance = libvlc_new(ARRAY_SIZE(args), args);
        QVERIFY(m_instance);
        m_player = libvlc_media_player_new(m_instance, nullptr, nullptr);
        QVERIFY(m_player);
        m_waveform = std::make_unique<AudioWaveform>(vlc_test_media_player_object(m_player));
        m_waveform->setEnabled(true);
    }
    void fullNativeDecodeAndCache()
    {
        m_waveform->setSource(m_fixture);
        QVERIFY(m_waveform->loading());
        QTRY_VERIFY_WITH_TIMEOUT(!m_waveform->loading(), 15000);
        QVERIFY2(m_waveform->available(), qPrintable(m_waveform->error()));
        QCOMPARE(m_waveform->durationSeconds(), 30.0);
        const auto peaks = m_waveform->peaks();
        QCOMPARE(peaks.size(), 300);
        double error = 0;
        for (int i = 0; i < peaks.size(); ++i)
        {
            const double expected = .4 * (.6 + .4 * std::sin(2 * M_PI * .2 * ((i + .5) * .1)));
            error += std::abs(peaks[i].toDouble() - expected);
        }
        qInfo("Native waveform points=%d duration=%.3f amplitude error=%.6f",
              int(peaks.size()), m_waveform->durationSeconds(), error / peaks.size());
        QVERIFY(error / peaks.size() < .015);
        m_waveform->setSource({});
        QVERIFY(!m_waveform->available());
        m_waveform->setSource(m_fixture);
        QVERIFY(!m_waveform->loading()); // Bounded in-memory cache, no new decode.
        QCOMPARE(m_waveform->peaks(), peaks);
    }
    void silentAndVeryShortAudio()
    {
        const auto silence = wav("silence.wav", QList<qint16>(8000, 0));
        m_waveform->setSource(silence);
        QTRY_VERIFY_WITH_TIMEOUT(!m_waveform->loading(), 15000);
        QVERIFY2(m_waveform->available(), qPrintable(m_waveform->error()));
        QCOMPARE(m_waveform->durationSeconds(), 1.0);
        QCOMPARE(m_waveform->peaks().size(), 300);
        for (const auto &peak : m_waveform->peaks()) QCOMPARE(peak.toDouble(), 0.0);
        const auto shortAudio = wav("short.wav", {-32768, 0, 16384, 32767});
        m_waveform->setSource(shortAudio);
        QTRY_VERIFY_WITH_TIMEOUT(!m_waveform->loading(), 15000);
        QVERIFY2(m_waveform->available(), qPrintable(m_waveform->error()));
        QCOMPARE(m_waveform->peaks().size(), 4);
        QCOMPARE(m_waveform->durationSeconds(), 4.0 / 8000);
        QCOMPARE(m_waveform->peaks()[0].toDouble(), 1.0);
        QCOMPARE(m_waveform->peaks()[2].toDouble(), .5);
    }
    void failureAndSourceValidation()
    {
        m_waveform->setSource(QUrl("https://example.invalid/audio.wav"));
        QVERIFY(!m_waveform->loading());
        QVERIFY(!m_waveform->error().isEmpty());
        QVERIFY(!m_waveform->available());
        m_waveform->setSource(QUrl::fromLocalFile(m_directory.filePath("missing.wav")));
        QVERIFY(!m_waveform->error().isEmpty());
        QFile invalid(m_directory.filePath("broken.wav"));
        QVERIFY(invalid.open(QIODevice::WriteOnly));
        invalid.write("Not an audio file");
        invalid.close();
        m_waveform->setSource(QUrl::fromLocalFile(invalid.fileName()));
        QTRY_VERIFY_WITH_TIMEOUT(!m_waveform->loading(), 15000);
        QVERIFY(!m_waveform->error().isEmpty());
        QVERIFY(!m_waveform->available());
    }
    void cancellationMediaChangesAndInvalidatedCache()
    {
        const auto source = wav("mutable.wav", QList<qint16>(8000, 0));
        m_waveform->setSource(source);
        m_waveform->setEnabled(false);
        QVERIFY(!m_waveform->loading());
        QVERIFY(!m_waveform->available());
        QTest::qWait(100);
        QVERIFY(!m_waveform->available());
        m_waveform->setEnabled(true);
        for (int i = 0; i < 10; ++i)
            m_waveform->setSource(i % 2 ? source : m_fixture);
        m_waveform->setSource(m_fixture);
        QTest::qWait(100); // Cancelled/late work must not replace the final source.
        QCOMPARE(m_waveform->source(), m_fixture);
        QCOMPARE(m_waveform->durationSeconds(), 30.0);
        m_waveform->setSource(source);
        QTRY_VERIFY_WITH_TIMEOUT(!m_waveform->loading(), 15000);
        QCOMPARE(m_waveform->durationSeconds(), 1.0);
        m_waveform->setEnabled(false);
        wav("mutable.wav", QList<qint16>(16000, 16384));
        m_waveform->setEnabled(true);
        QVERIFY(m_waveform->loading()); // File size/mtime invalidates old peaks.
        QTRY_VERIFY_WITH_TIMEOUT(!m_waveform->loading(), 15000);
        QVERIFY2(m_waveform->available(), qPrintable(m_waveform->error()));
        QCOMPARE(m_waveform->durationSeconds(), 2.0);
        QCOMPARE(m_waveform->peaks()[0].toDouble(), .5);
    }
    void independentPrimaryPlayback()
    {
        libvlc_audio_set_callbacks(m_player, audio, nullptr, nullptr, nullptr, nullptr, this);
        libvlc_audio_set_format(m_player, "FL32", 44100, 1);
        const auto sourceUri = m_fixture.toEncoded();
        auto *media = libvlc_media_new_location(sourceUri.constData());
        QVERIFY(media);
        libvlc_media_player_set_media(m_player, media);
        libvlc_media_release(media);
        QCOMPARE(libvlc_media_player_play(m_player), 0);
        QTRY_VERIFY_WITH_TIMEOUT(m_audioBuffers > 3, 10000);
        const auto before = m_audioBuffers.load();
        AudioWaveform independent(vlc_test_media_player_object(m_player));
        independent.setSource(m_fixture);
        independent.setEnabled(true);
        QTRY_VERIFY_WITH_TIMEOUT(!independent.loading(), 15000);
        QVERIFY2(independent.available(), qPrintable(independent.error()));
        QCOMPARE(independent.durationSeconds(), 30.0);
        QTRY_VERIFY_WITH_TIMEOUT(m_audioBuffers > before + 3, 5000);
        QCOMPARE(libvlc_media_player_get_state(m_player), libvlc_Playing);
        libvlc_media_player_set_pause(m_player, 1);
        QTRY_COMPARE_WITH_TIMEOUT(libvlc_media_player_get_state(m_player), libvlc_Paused, 5000);
        const auto pausedTime = libvlc_media_player_get_time(m_player);
        AudioWaveform whilePaused(vlc_test_media_player_object(m_player));
        whilePaused.setSource(m_fixture);
        whilePaused.setEnabled(true);
        QTRY_VERIFY_WITH_TIMEOUT(!whilePaused.loading(), 15000);
        QVERIFY2(whilePaused.available(), qPrintable(whilePaused.error()));
        QCOMPARE(whilePaused.durationSeconds(), 30.0);
        QCOMPARE(libvlc_media_player_get_time(m_player), pausedTime);
        libvlc_media_player_stop_async(m_player);
    }
    void workerShutdown()
    {
        auto pending = std::make_unique<AudioWaveform>(vlc_test_media_player_object(m_player));
        pending->setSource(m_fixture);
        pending->setEnabled(true);
        QElapsedTimer timer;
        timer.start();
        pending.reset();
        QVERIFY(timer.elapsed() < 10000);
        QCoreApplication::processEvents(); // No queued result targets freed storage.
    }
    void cleanupTestCase()
    {
        m_waveform.reset();
        if (m_player) libvlc_media_player_release(m_player);
        if (m_instance) libvlc_release(m_instance);
    }
};
QTEST_GUILESS_MAIN(TestAudioWaveform)
#include "test_audio_waveform.moc"
