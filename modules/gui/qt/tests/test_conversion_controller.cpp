// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "dialogs/conversion_controller.hpp"
#include <vlc/vlc.h>
#include <vlc_input_item.h>
#include <vlc_player.h>
#include <vlc_playlist.h>
#include <QCryptographicHash>
#include <QDataStream>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>
#include <algorithm>
#include <atomic>
#include <cmath>

extern "C" vlc_object_t *vlc_test_media_player_object(libvlc_media_player_t *);
extern "C" vlc_playlist_t *vlc_test_stream_playlist(libvlc_instance_t *);
extern "C" void vlc_test_stream_audio(vlc_playlist_t *,
    void (*)(void *, const void *, unsigned, int64_t), void *);

class TestConversionController : public QObject
{
    Q_OBJECT
    libvlc_instance_t *m_instance = nullptr;
    libvlc_media_player_t *m_player = nullptr;
    vlc_playlist_t *m_playlist = nullptr;
    QTemporaryDir m_directory;
    QUrl m_source, m_primary, m_large, m_result;
    std::atomic_uint m_primaryBuffers{0}, m_outputBuffers{0};
    std::atomic<quint64> m_outputSamples{0};
    std::atomic<double> m_peak{0};
    static void primaryAudio(void *opaque, const void *, unsigned, int64_t)
    { ++static_cast<TestConversionController *>(opaque)->m_primaryBuffers; }
    static void outputAudio(void *opaque, const void *bytes, unsigned count, int64_t)
    {
        auto *self = static_cast<TestConversionController *>(opaque);
        const auto *samples = static_cast<const float *>(bytes);
        double peak = 0;
        for (unsigned i = 0; i < count; ++i) peak = std::max(peak, std::abs(double(samples[i])));
        if (peak > self->m_peak) self->m_peak = peak;
        self->m_outputSamples += count; ++self->m_outputBuffers;
    }
    QUrl output(const QString &name) const
    { return QUrl::fromLocalFile(m_directory.filePath(name)); }
    QByteArray hash(const QUrl &url) const
    {
        QFile file(url.toLocalFile());
        if (!file.open(QIODevice::ReadOnly)) return {};
        QCryptographicHash digest(QCryptographicHash::Sha256);
        if (!digest.addData(&file)) return {};
        return digest.result();
    }
    QUrl wav(const QString &name, int seconds, bool noise = false)
    {
        const auto path = m_directory.filePath(name);
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) return {};
        QDataStream stream(&file); stream.setByteOrder(QDataStream::LittleEndian);
        stream.writeRawData("RIFF", 4); stream << quint32(0);
        stream.writeRawData("WAVEfmt ", 8);
        stream << quint32(16) << quint16(1) << quint16(1) << quint32(44100)
               << quint32(88200) << quint16(2) << quint16(16);
        stream.writeRawData("data", 4); stream << quint32(seconds * 44100 * 2);
        QByteArray block;
        QDataStream samples(&block, QIODevice::WriteOnly); samples.setByteOrder(QDataStream::LittleEndian);
        quint32 random = 0x42a24f3bu;
        for (int i = 0; i < 44100; ++i) {
            random ^= random << 13; random ^= random >> 17; random ^= random << 5;
            samples << (noise ? qint16(random) : qint16(12000 * std::sin(2 * M_PI * 220 * i / 44100)));
        }
        for (int i = 0; i < seconds; ++i)
            if (file.write(block) != block.size()) return {};
        QByteArray info("INFO", 4);
        QDataStream tags(&info, QIODevice::Append); tags.setByteOrder(QDataStream::LittleEndian);
        const QByteArray title("Conversion fixture\0", 19);
        tags.writeRawData("INAM", 4); tags << quint32(title.size());
        tags.writeRawData(title.constData(), title.size());
        if (title.size() % 2) tags << quint8(0);
        stream.writeRawData("LIST", 4); stream << quint32(info.size());
        stream.writeRawData(info.constData(), info.size());
        const auto size = file.size(); file.seek(4); stream << quint32(size - 8);
        return QUrl::fromLocalFile(path);
    }
    void configure(ConversionController &controller, const QUrl &source, const QUrl &destination)
    { controller.setSourceUrl(source); controller.setDestinationUrl(destination); }
    bool awaitNative(ConversionController &controller)
    {
        QElapsedTimer timer; timer.start();
        while (controller.busy() && !controller.nativeInputActive() && timer.elapsed() < 10000)
            QTest::qWait(1);
        return controller.busy() && controller.nativeInputActive();
    }
    QStringList queueUris() const
    {
        QStringList result;
        vlc_playlist_Lock(m_playlist);
        for (size_t i = 0; i < vlc_playlist_Count(m_playlist); ++i) {
            char *uri = input_item_GetURI(vlc_playlist_item_GetMedia(vlc_playlist_Get(m_playlist, i)));
            result.append(QString::fromUtf8(uri)); free(uri);
        }
        vlc_playlist_Unlock(m_playlist); return result;
    }
    void noTemporaryDirectories() const
    {
        QCOMPARE(QDir(m_directory.path()).entryList({".vlc-convert-*"}, QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot).size(), 0);
    }
private slots:
    void initTestCase()
    {
        QVERIFY(m_directory.isValid());
        QVERIFY(QDir(m_directory.path()).mkdir(QString::fromUtf8("dest 'quoted, \xC3\xA4")));
        m_source = wav("source 'quoted, audio.wav", 2);
        m_primary = wav("primary audio.wav", 20);
        m_large = wav("large audio.wav", 600, true); // actual 52 MiB native encode input
        QVERIFY(!m_source.isEmpty() && !m_primary.isEmpty() && !m_large.isEmpty());
        const char *args[] = {"--ignore-config", "--no-media-library", "--no-auto-preparse",
            "--aout=amem,none", "--no-video", "--verbose=2", "--file-logging", "--logfile=conversion-native.log"};
        m_instance = libvlc_new(ARRAY_SIZE(args), args); QVERIFY(m_instance);
        m_player = libvlc_media_player_new(m_instance, nullptr, nullptr); QVERIFY(m_player);
        auto *parent = vlc_test_media_player_object(m_player);
        // A real secondary player inherits these variables unless production
        // deliberately overrides them. Conversion must remain full length and
        // unfiltered despite unrelated playback/transcode preferences.
        QCOMPARE(var_Create(parent, "rate", VLC_VAR_FLOAT), VLC_SUCCESS);
        QCOMPARE(var_SetFloat(parent, "rate", .5f), VLC_SUCCESS);
        QCOMPARE(var_Create(parent, "sout-transcode-afilter", VLC_VAR_STRING), VLC_SUCCESS);
        QCOMPARE(var_SetString(parent, "sout-transcode-afilter", "normvol{max-level=0.1}"), VLC_SUCCESS);
        m_playlist = vlc_test_stream_playlist(m_instance); QVERIFY(m_playlist);
        vlc_test_stream_audio(m_playlist, primaryAudio, this);
    }
    void localValidationAndNoOverwrite()
    {
        ConversionController controller(vlc_test_media_player_object(m_player));
        configure(controller, QUrl("https://example.invalid/source.wav"), output("remote.flac"));
        QVERIFY(!controller.canStart()); QVERIFY(!controller.start()); QVERIFY(!controller.error().isEmpty());
        for (const auto &url : {QUrl("file://unavailable-server/share/source.wav"),
                               QUrl("file:////unavailable-server/share/source.wav"),
                               QUrl("file:%5C%5Cunavailable-server%5Cshare%5Csource.wav"),
                               QUrl("file:%5C%5C%3F%5CC:%5Csource.wav"),
                               QUrl("file:%5C%5C.%5Cpipe%5Csource.wav"),
                               QUrl("file:%5C%2Funavailable-server%5Csource.wav"),
                               QUrl("file:%2F%5Cunavailable-server%5Csource.wav"),
                               QUrl("file:relative.wav"), QUrl("file:C:relative.wav"), QUrl("file:C:")}) {
            configure(controller, url, output("unc.flac"));
            QVERIFY(!controller.start());
            QCOMPARE(controller.error(), QString("Choose a local audio source and a local FLAC destination."));
            configure(controller, m_source, url);
            QVERIFY(!controller.start());
            QCOMPARE(controller.error(), QString("Choose a local audio source and a local FLAC destination."));
        }
        configure(controller, m_source, QUrl("file://unavailable-server/share/output.flac"));
        QVERIFY(!controller.start());
        configure(controller, m_source, m_source); QVERIFY(!controller.start());
        configure(controller, m_source, output("wrong.wav")); QVERIFY(!controller.start());
        configure(controller, m_source, output("missing-folder/output.flac")); QVERIFY(!controller.start());
        QVERIFY(QDir(m_directory.path()).mkdir("removed-folder"));
        const auto removed = output("removed-folder/deleted-parent.flac");
        configure(controller, m_source, removed); QVERIFY(controller.canStart());
        QVERIFY(QDir(m_directory.path()).rmdir("removed-folder"));
        QVERIFY(!controller.start());
        QVERIFY(!QFile::exists(removed.toLocalFile()));
        QVERIFY(!QFile::exists(QDir::current().filePath("deleted-parent.flac")));
        const auto existing = output("existing.flac");
        QFile file(existing.toLocalFile()); QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("preserve this file"), qint64(18)); file.close();
        const auto before = hash(existing);
        configure(controller, m_source, existing); QVERIFY(!controller.start()); QCOMPARE(hash(existing), before);
        noTemporaryDirectories();
    }
    void realFlacAndActualPlayback()
    {
        ConversionController controller(vlc_test_media_player_object(m_player));
        m_result = output(QString::fromUtf8("dest 'quoted, \xC3\xA4/result 'quoted, \xC3\xA4.flac"));
        const auto sourceHash = hash(m_source);
        configure(controller, m_source, m_result); QVERIFY(controller.canStart()); QVERIFY(controller.start());
        QVERIFY(!controller.start()); // one worker/job only
        controller.setSourceUrl(m_large); controller.setDestinationUrl(output("ignored.flac"));
        QCOMPARE(controller.sourceUrl(), m_source); QCOMPARE(controller.destinationUrl(), m_result);
        controller.reset(); QVERIFY(controller.busy()); // reset cannot detach a running job
        QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), 20000);
        QVERIFY2(controller.state() == ConversionController::Succeeded, qPrintable(controller.error()));
        QCOMPARE(controller.outputUrl(), m_result); QCOMPARE(controller.progress(), 1.);
        QVERIFY(std::abs(controller.sourceDurationMs() - 2000) <= 25);
        QCOMPARE(hash(m_source), sourceHash);
        QFile flac(m_result.toLocalFile()); QVERIFY(flac.open(QIODevice::ReadOnly)); QCOMPARE(flac.read(4), QByteArray("fLaC"));
        noTemporaryDirectories();
        // Independently play the published file through ordinary LibVLC/amem,
        // rather than accepting the converter's own validation report.
        libvlc_audio_set_callbacks(m_player, outputAudio, nullptr, nullptr, nullptr, nullptr, this);
        libvlc_audio_set_format(m_player, "FL32", 44100, 1);
        const auto uri = m_result.toEncoded(); auto *media = libvlc_media_new_location(uri.constData()); QVERIFY(media);
        libvlc_media_player_set_media(m_player, media); libvlc_media_release(media);
        QCOMPARE(libvlc_media_player_play(m_player), 0);
        QTRY_VERIFY_WITH_TIMEOUT(m_outputBuffers > 2, 10000);
        QTRY_COMPARE_WITH_TIMEOUT(libvlc_media_player_get_state(m_player), libvlc_Stopped, 10000);
        QVERIFY(std::abs(qint64(m_outputSamples.load()) - 88200) <= 100);
        QVERIFY(m_peak > .3 && m_peak < .5);
        const auto evidence = qEnvironmentVariable("VLC_CONVERSION_EVIDENCE_DIR");
        if (!evidence.isEmpty()) {
            QVERIFY(QFile::copy(m_source.toLocalFile(), QDir(evidence).filePath("source.wav")));
            QVERIFY(QFile::copy(m_result.toLocalFile(), QDir(evidence).filePath("converted.flac")));
        }
    }
    void inheritedRunTimeAndInputChoicesDoNotTrim()
    {
        auto *parent = vlc_test_media_player_object(m_player);
        QCOMPARE(var_Create(parent, "run-time", VLC_VAR_FLOAT), VLC_SUCCESS);
        QCOMPARE(var_SetFloat(parent, "run-time", .25f), VLC_SUCCESS);
        QCOMPARE(var_Create(parent, "demux", VLC_VAR_STRING), VLC_SUCCESS);
        QCOMPARE(var_SetString(parent, "demux", "subtitle"), VLC_SUCCESS);
        QCOMPARE(var_Create(parent, "audio-track-id", VLC_VAR_STRING), VLC_SUCCESS);
        QCOMPARE(var_SetString(parent, "audio-track-id", "not-a-real-audio-track"), VLC_SUCCESS);
        // Both encoding and validation create fresh inputs. A shared inherited
        // run-time must not quietly stop both at the same quarter-second.
        ConversionController controller(parent);
        const auto destination = output("inherited-options.flac");
        configure(controller, m_source, destination); QVERIFY(controller.start());
        QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), 15000);
        QCOMPARE(var_SetFloat(parent, "run-time", 0.f), VLC_SUCCESS);
        QCOMPARE(var_SetString(parent, "demux", ""), VLC_SUCCESS);
        QCOMPARE(var_SetString(parent, "audio-track-id", ""), VLC_SUCCESS);
        QVERIFY2(controller.state() == ConversionController::Succeeded, qPrintable(controller.error()));
        m_outputBuffers = 0; m_outputSamples = 0; m_peak = 0;
        const auto uri = destination.toEncoded();
        auto *media = libvlc_media_new_location(uri.constData()); QVERIFY(media);
        libvlc_media_player_set_media(m_player, media); libvlc_media_release(media);
        QCOMPARE(libvlc_media_player_play(m_player), 0);
        QTRY_VERIFY_WITH_TIMEOUT(m_outputBuffers > 2, 10000);
        QTRY_COMPARE_WITH_TIMEOUT(libvlc_media_player_get_state(m_player), libvlc_Stopped, 10000);
        QVERIFY2(std::abs(qint64(m_outputSamples.load()) - 88200) <= 100,
            "Inherited run-time and input choices must not trim the published audio");
        QVERIFY(m_peak > .3 && m_peak < .5);
        noTemporaryDirectories();
    }
    void inheritedFileFormatKeepsLiteralPercentDestination()
    {
        // Own a preference parent instead of changing the shared test player's
        // options. The controller is declared later, so it joins before this
        // real VLC configuration object is deleted on every return path.
        const auto deletePreferences = [](vlc_object_t *object) { vlc_object_delete(object); };
        std::unique_ptr<vlc_object_t, decltype(deletePreferences)> preferences(
            static_cast<vlc_object_t *>(vlc_object_create(
                vlc_test_media_player_object(m_player), sizeof(vlc_object_t))), deletePreferences);
        QVERIFY(preferences);
        QCOMPARE(var_Create(preferences.get(), "sout-file-format", VLC_VAR_BOOL), VLC_SUCCESS);
        QCOMPARE(var_SetBool(preferences.get(), "sout-file-format", true), VLC_SUCCESS);
        const auto literalFolder = m_directory.filePath("literal 100%% %Y");
        const auto formattedFolder = m_directory.filePath("literal 100% "
            + QString::number(QDateTime::currentDateTime().date().year()));
        QVERIFY(QDir().mkpath(literalFolder));
        QVERIFY(QDir().mkpath(formattedFolder));
        const auto sentinel = QUrl::fromLocalFile(QDir(formattedFolder).filePath("sentinel.keep"));
        QFile other(sentinel.toLocalFile());
        QVERIFY(other.open(QIODevice::WriteOnly | QIODevice::NewOnly));
        const QByteArray sentinelBytes("Preserve the unrelated formatted-path sibling");
        QCOMPARE(other.write(sentinelBytes), qint64(sentinelBytes.size())); other.close();
        const auto sentinelHash = hash(sentinel), sourceHash = hash(m_source);
        QVERIFY(!sentinelHash.isEmpty() && !sourceHash.isEmpty());
        const auto destination = QUrl::fromLocalFile(QDir(literalFolder).filePath("literal result.flac"));
        ConversionController controller(preferences.get());
        configure(controller, m_source, destination);
        QVERIFY(controller.canStart()); QVERIFY(controller.start());
        QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), 15000);
        QVERIFY2(controller.state() == ConversionController::Succeeded, qPrintable(controller.error()));
        QCOMPARE(controller.outputUrl(), destination);
        QVERIFY(var_GetBool(preferences.get(), "sout-file-format"));
        QCOMPARE(hash(m_source), sourceHash); QCOMPARE(hash(sentinel), sentinelHash);
        QFile flac(destination.toLocalFile());
        QVERIFY(flac.open(QIODevice::ReadOnly)); QCOMPARE(flac.read(4), QByteArray("fLaC")); flac.close();
        QCOMPARE(QDir(literalFolder).entryList({".vlc-convert-*"},
            QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot).size(), 0);
        QCOMPARE(QDir(formattedFolder).entryList(QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot),
            QStringList{"sentinel.keep"});
        // Published-file playback is independent of conversion's own second
        // stage. Inherited formatting must not break either native file sink.
        m_outputBuffers = 0; m_outputSamples = 0; m_peak = 0;
        libvlc_audio_set_callbacks(m_player, outputAudio, nullptr, nullptr, nullptr, nullptr, this);
        libvlc_audio_set_format(m_player, "FL32", 44100, 1);
        const auto uri = destination.toEncoded();
        auto *media = libvlc_media_new_location(uri.constData()); QVERIFY(media);
        libvlc_media_player_set_media(m_player, media); libvlc_media_release(media);
        QCOMPARE(libvlc_media_player_play(m_player), 0);
        QTRY_VERIFY_WITH_TIMEOUT(m_outputBuffers > 2, 10000);
        QTRY_COMPARE_WITH_TIMEOUT(libvlc_media_player_get_state(m_player), libvlc_Stopped, 10000);
        QVERIFY2(std::abs(qint64(m_outputSamples.load()) - 88200) <= 100,
            "A literal percent-folder conversion must preserve complete native audio");
        QVERIFY(m_peak > .3 && m_peak < .5);
        noTemporaryDirectories();
    }
    void corruptSourceDoesNotPublish()
    {
        QFile file(m_directory.filePath("broken.wav")); QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("Not an audio file"); file.close();
        ConversionController controller(vlc_test_media_player_object(m_player));
        configure(controller, QUrl::fromLocalFile(file.fileName()), output("broken.flac"));
        QVERIFY(controller.start()); QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), 15000);
        QCOMPARE(controller.state(), ConversionController::Failed); QVERIFY(!controller.error().isEmpty());
        QVERIFY(controller.outputUrl().isEmpty()); QVERIFY(!QFile::exists(output("broken.flac").toLocalFile()));
        noTemporaryDirectories();
    }
    void destinationRaceIsPreserved()
    {
        ConversionController controller(vlc_test_media_player_object(m_player));
        const auto destination = output("racing.flac"); configure(controller, m_source, destination);
        QVERIFY(controller.start());
        QFile file(destination.toLocalFile()); QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::NewOnly));
        file.write("A racing independent file"); file.close(); const auto expected = hash(destination);
        QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), 15000);
        QCOMPARE(controller.state(), ConversionController::Failed); QCOMPARE(hash(destination), expected);
        QVERIFY(controller.outputUrl().isEmpty()); noTemporaryDirectories();
    }
    void runningCancellationAndReuse()
    {
        ConversionController controller(vlc_test_media_player_object(m_player));
        configure(controller, m_large, output("cancelled.flac")); QVERIFY(controller.start());
        QVERIFY2(awaitNative(controller), "Cancellation must target a real native input, not queued work");
        QElapsedTimer timer; timer.start(); controller.cancel();
        QCOMPARE(controller.state(), ConversionController::Cancelling);
        QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), 10000);
        QCOMPARE(controller.state(), ConversionController::Cancelled); QVERIFY(timer.elapsed() < 10000);
        QVERIFY(!QFile::exists(output("cancelled.flac").toLocalFile())); noTemporaryDirectories();
        configure(controller, m_source, output("after-cancel.flac")); QVERIFY(controller.start());
        QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), 15000);
        QVERIFY2(controller.state() == ConversionController::Succeeded, qPrintable(controller.error()));
        controller.reset(); QTest::qWait(100);
        QCOMPARE(controller.state(), ConversionController::Idle); QVERIFY(controller.outputUrl().isEmpty());
    }
    void independentPrimaryQueueAndPausedClock()
    {
        input_item_t *items[2];
        const auto first = m_primary.toEncoded(), second = m_source.toEncoded();
        items[0] = input_item_New(first.constData(), nullptr); items[1] = input_item_New(second.constData(), nullptr);
        QVERIFY(items[0] && items[1]);
        vlc_playlist_Lock(m_playlist);
        const auto inserted = vlc_playlist_Insert(m_playlist, 0, items, 2);
        const auto started = vlc_playlist_GoTo(m_playlist, 0) == VLC_SUCCESS && vlc_playlist_Start(m_playlist) == VLC_SUCCESS;
        vlc_playlist_Unlock(m_playlist); input_item_Release(items[0]); input_item_Release(items[1]);
        QCOMPARE(inserted, VLC_SUCCESS); QVERIFY(started);
        QTRY_VERIFY_WITH_TIMEOUT(m_primaryBuffers > 3, 10000);
        const auto beforeBuffers = m_primaryBuffers.load();
        const auto expectedQueue = queueUris();
        ConversionController controller(vlc_test_media_player_object(m_player));
        configure(controller, m_source, output("independent-playing.flac")); QVERIFY(controller.start());
        QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), 15000);
        QVERIFY2(controller.state() == ConversionController::Succeeded, qPrintable(controller.error()));
        QTRY_VERIFY_WITH_TIMEOUT(m_primaryBuffers > beforeBuffers + 3, 5000); QCOMPARE(queueUris(), expectedQueue);
        auto *primary = vlc_playlist_GetPlayer(m_playlist);
        vlc_player_Lock(primary); const auto playingState = vlc_player_GetState(primary);
        vlc_player_Pause(primary); vlc_player_Unlock(primary);
        QCOMPARE(playingState, VLC_PLAYER_STATE_PLAYING);
        QTRY_VERIFY_WITH_TIMEOUT(([&] { vlc_player_Lock(primary); const bool paused = vlc_player_GetState(primary) == VLC_PLAYER_STATE_PAUSED; vlc_player_Unlock(primary); return paused; })(), 5000);
        vlc_player_Lock(primary); const auto pausedTime = vlc_player_GetTime(primary); vlc_player_Unlock(primary);
        configure(controller, m_source, output("independent-paused.flac")); QVERIFY(controller.start());
        QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), 15000);
        QVERIFY2(controller.state() == ConversionController::Succeeded, qPrintable(controller.error()));
        vlc_player_Lock(primary); const auto finalTime = vlc_player_GetTime(primary); vlc_player_Unlock(primary);
        QCOMPARE(finalTime, pausedTime); QCOMPARE(queueUris(), expectedQueue);
        vlc_playlist_Lock(m_playlist); const auto selected = vlc_playlist_GetCurrentIndex(m_playlist);
        vlc_playlist_Stop(m_playlist); vlc_playlist_Unlock(m_playlist);
        QCOMPARE(selected, ssize_t(0));
    }
    void runningWorkerShutdown()
    {
        auto controller = std::make_unique<ConversionController>(vlc_test_media_player_object(m_player));
        configure(*controller, m_large, output("shutdown.flac")); QVERIFY(controller->start());
        QVERIFY2(awaitNative(*controller), "Shutdown must join a real native input");
        QElapsedTimer timer; timer.start(); controller.reset();
        QVERIFY(timer.elapsed() < 10000); QCoreApplication::processEvents();
        QVERIFY(!QFile::exists(output("shutdown.flac").toLocalFile())); noTemporaryDirectories();
    }
    void cleanupTestCase()
    {
        if (m_playlist) vlc_playlist_Delete(m_playlist);
        if (m_player) libvlc_media_player_release(m_player);
        if (m_instance) libvlc_release(m_instance);
    }
};
QTEST_GUILESS_MAIN(TestConversionController)
#include "test_conversion_controller.moc"
