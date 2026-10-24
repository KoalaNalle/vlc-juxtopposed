// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
// Native source/parse/player checks; supplied-state QML checks are separate.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "player/subtitle_cue_timeline.hpp"
#include "qt.hpp"
#include <vlc/vlc.h>
#include <vlc_cxx_helpers.hpp>
using vlc_player_locker = vlc_locker<vlc_player_t, vlc_player_Lock, vlc_player_Unlock>;
#include <vlc_input_item.h>
#include <vlc_vout.h>
#include <QCryptographicHash>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>
#include <atomic>
#include <cmath>

extern "C" vlc_player_t *vlc_test_media_player_core(libvlc_media_player_t *);
extern "C" vlc_object_t *vlc_test_media_player_object(libvlc_media_player_t *);

// A distinct owned player makes accidental recursive locking a real failure.
// The native runner still needs a process timeout to bound a lock regression.
struct NormalTimelineFixture
{
    vlc_player_t *player = nullptr;
    vlc_player_timer_id *timer = nullptr;
    std::unique_ptr<SubtitleCueTimeline> timeline;
    std::atomic<vlc_tick_t> requested{VLC_TICK_INVALID};
    std::atomic_uint requests{0}, completions{0};

    ~NormalTimelineFixture()
    {
        // Listener removal and parser join happen before the player disappears.
        timeline.reset();
        if (player)
        {
            {
                vlc_player_locker lock(player);
                if (timer) vlc_player_RemoveTimer(player, timer);
                vlc_player_Stop(player);
            }
            // Delete requires an unlocked player; callback storage is alive
            // until timer removal synchronizes with any callback in progress.
            vlc_player_Delete(player);
        }
    }
    static void onUpdate(const vlc_player_timer_point *, void *) {}
    static void onSeek(const vlc_player_timer_point *point, void *opaque)
    {
        // Native timer callbacks must never call player or QObject APIs.
        auto *self = static_cast<NormalTimelineFixture *>(opaque);
        if (point)
        {
            self->requested.store(point->ts, std::memory_order_relaxed);
            self->requests.fetch_add(1, std::memory_order_release);
        }
        else
            self->completions.fetch_add(1, std::memory_order_release);
    }
    vlc_player_state state() const
    {
        vlc_player_locker lock(player);
        return vlc_player_GetState(player);
    }
    vlc_tick_t time() const
    {
        vlc_player_locker lock(player);
        return vlc_player_GetTime(player);
    }
};

class TestSubtitleCueTimeline : public QObject
{
    Q_OBJECT
    libvlc_instance_t *m_instance = nullptr;
    libvlc_media_player_t *m_mediaPlayer = nullptr;
    vlc_player_t *m_player = nullptr;
    std::unique_ptr<SubtitleCueTimeline> m_timeline;
    QString m_directory;
    QUrl m_subtitle;
    QTemporaryDir m_scratch;
    QByteArray m_primaryTrack;
    quintptr m_primaryMedia = 0;
    SubtitleCueCancellation token() { return std::make_shared<std::atomic_bool>(false); }
    QByteArray selectedId()
    {
        vlc_player_locker lock(m_player);
        const auto *track = vlc_player_GetSelectedTrack(m_player, SPU_ES);
        return track ? QByteArray(vlc_es_id_GetStrId(track->es_id)) : QByteArray{};
    }
    quintptr mediaToken()
    {
        vlc_player_locker lock(m_player);
        return reinterpret_cast<quintptr>(vlc_player_GetCurrentMedia(m_player));
    }
    bool waitForNativeSubtitleSelectionCount(unsigned expected)
    {
        QElapsedTimer deadline;
        deadline.start();
        while (deadline.elapsed() < 5000)
        {
            unsigned selected = 0;
            {
                vlc_player_locker lock(m_player);
                const auto count = vlc_player_GetTrackCount(m_player, SPU_ES);
                if (count > 512) return false;
                for (size_t i = 0; i < count; ++i)
                    selected += vlc_player_GetTrackAt(m_player, SPU_ES, i)->selected;
            }
            if (selected == expected) return true;
            // No GUI event processing: observe the real native transition
            // before testing immediate epoch rejection with the old view.
            QTest::qSleep(5);
        }
        return false;
    }
    void assertPrimaryUnchanged()
    {
        QCOMPARE(selectedId(), m_primaryTrack);
        QCOMPARE(mediaToken(), m_primaryMedia);
        QCOMPARE(libvlc_media_player_get_state(m_mediaPlayer), libvlc_Paused);
    }
private slots:
    void parserUnitsOrderingAndOverlap()
    {
        auto result = parseExternalSrtCues(QByteArray("\xef\xbb\xbf") +
            "3\r\n00:00:04,000 --> 00:00:06,000\r\nThird\r\n\r\n"
            "1\r\n00:00:01,125 --> 00:00:02,250\r\nFirst\r\nMultiline\r\n\r\n"
            "2\r\n00:00:01,500 --> 00:00:03,000\r\n<font color=\"green\">Overlap</font>\r\n", token());
        QVERIFY2(result.error.isEmpty(), qPrintable(result.error));
        QCOMPARE(result.cues.size(), 3);
        QCOMPARE(result.cues[0].start, VLC_TICK_FROM_MS(1125));
        QCOMPARE(result.cues[0].end, VLC_TICK_FROM_MS(2250));
        QCOMPARE(result.cues[0].text, QString("First\nMultiline"));
        QCOMPARE(result.cues[1].start, VLC_TICK_FROM_MS(1500));
        QCOMPARE(result.cues[2].end, VLC_TICK_FROM_MS(6000));
        result = parseExternalSrtCues("1\n2500:00:00,000 --> 2500:00:01,000\nLong\n", token());
        QVERIFY(result.error.isEmpty());
        QCOMPARE(result.cues[0].start, VLC_TICK_FROM_SEC(INT64_C(2500) * 3600));
        QVERIFY(result.cues[0].start > INT32_MAX);
    }
    void invalidEmptyUnsupportedAndCanceledReads()
    {
        for (const auto *bytes : {"1\n00:00:04,000 --> 00:00:03,000\nBackwards", "invalid", "1\n00:60:00,000 --> 01:00:01,000\nInvalid", "1\n00:00:01,000 --> 00:00:02,000"})
        {
            const auto result = parseExternalSrtCues(bytes, token());
            QVERIFY(!result.error.isEmpty() && result.cues.isEmpty());
        }
        auto result = parseExternalSrtCues("\xef\xbb\xbf \r\n", token());
        QVERIFY(result.error.isEmpty() && result.cues.isEmpty());
        result = parseExternalSrtCues(QByteArray("\xff\xfe\x31\0", 4), token());
        QVERIFY(!result.error.isEmpty());
        for (const auto &tail : {QByteArray::fromHex("c3"), QByteArray::fromHex("e282"),
                                 QByteArray::fromHex("f09f92")})
        {
            result = parseExternalSrtCues(QByteArray("1\n00:00:01,000 --> 00:00:02,000\nText") + tail, token());
            QVERIFY2(!result.error.isEmpty() && result.cues.isEmpty(),
                "A truncated final UTF-8 sequence must not silently disappear");
        }
        result = parseExternalSrtCues(QByteArray(2 * 1024 * 1024 + 1, 'x'), token());
        QVERIFY(!result.error.isEmpty());
        result = parseExternalSrtCues(QByteArray("1\n00:00:01,000 --> 00:00:02,000\nCue\n\n").repeated(20001), token());
        QVERIFY(!result.error.isEmpty() && result.cues.isEmpty());
        auto cancel = token();
        cancel->store(true);
        result = parseExternalSrtCues("1\n00:00:01,000 --> 00:00:02,000\nCue", cancel);
        QVERIFY(result.canceled && result.cues.isEmpty());
        result = readExternalSrtCues(QUrl("file://server/share/cues.srt"), token());
        QVERIFY(!result.error.isEmpty());
        for (const auto *url : {"file:%5C%5Cunavailable-server%5Cshare%5Ccues.srt",
                               "file:///%5C%5Cunavailable-server%5Cshare%5Ccues.srt",
                               "file:%5C%5C.%5Cpipe%5Ccues.srt",
                               "file:%5C%5C%3F%5CC:%5Ccues.srt",
                               "file:%2F%5Cunavailable-server%5Ccues.srt",
                               "file:%5C%2Funavailable-server%5Ccues.srt",
                               "file:relative.srt", "file:C:relative.srt", "file:C:"})
        {
            result = readExternalSrtCues(QUrl(url), token());
            QCOMPARE(result.error, QString("Subtitle timeline supports a selected local SRT file."));
            QVERIFY(result.cues.isEmpty()); // exact pre-stat rejection branch, no remote/device IO
        }
        result = readExternalSrtCues(QUrl::fromLocalFile(m_scratch.filePath("already-canceled.srt")), cancel);
        QVERIFY(result.canceled && result.error.isEmpty());
        result = readExternalSrtCues(QUrl::fromLocalFile(m_scratch.filePath("missing.srt")), token());
        QVERIFY(!result.error.isEmpty());
        result = readExternalSrtCues(QUrl::fromLocalFile(m_scratch.filePath("unsupported.ass")), token());
        QVERIFY(!result.error.isEmpty());
    }
    void actualSelectedExternalSource()
    {
        m_directory = qEnvironmentVariable("VLC_TIMELINE_FIXTURE_DIR");
        QVERIFY2(!m_directory.isEmpty() && QDir(m_directory).exists(), "Use the generated subtitle-style.mp4 + duration-cues.srt directory");
        QVERIFY(m_scratch.isValid());
        m_subtitle = QUrl::fromLocalFile(QDir(m_directory).filePath("duration-cues.srt"));
        const char *args[] = {"--ignore-config", "--no-media-library", "--no-sub-autodetect-file",
            "--no-audio", "--no-hw-dec", "--vout=dummy", "--no-video-title-show", "--no-osd",
            "--verbose=2", "--file-logging", "--logfile=subtitle-cue-native.log"};
        m_instance = libvlc_new(ARRAY_SIZE(args), args);
        QVERIFY(m_instance);
        m_mediaPlayer = libvlc_media_player_new(m_instance, nullptr, nullptr);
        QVERIFY(m_mediaPlayer);
        m_player = vlc_test_media_player_core(m_mediaPlayer);
        QVERIFY(m_player);
        m_timeline = std::make_unique<SubtitleCueTimeline>(m_player);
        m_timeline->setEnabled(true);
        const auto video = QDir::toNativeSeparators(QDir(m_directory).filePath("subtitle-style.mp4")).toUtf8();
        auto *media = libvlc_media_new_path(video.constData());
        QVERIFY(media);
        libvlc_media_player_set_media(m_mediaPlayer, media);
        libvlc_media_release(media);
        QCOMPARE(libvlc_media_player_play(m_mediaPlayer), 0);
        QTRY_COMPARE_WITH_TIMEOUT(libvlc_media_player_get_state(m_mediaPlayer), libvlc_Playing, 10000);
        const auto exact = m_subtitle.toEncoded();
        QCOMPARE(libvlc_media_player_add_slave(m_mediaPlayer, libvlc_media_slave_type_subtitle, exact.constData(), true), 0);
        QTRY_VERIFY_WITH_TIMEOUT(m_timeline->available() && m_timeline->cueCount() == 2, 10000);
        libvlc_media_player_set_pause(m_mediaPlayer, true);
        QTRY_COMPARE(libvlc_media_player_get_state(m_mediaPlayer), libvlc_Paused);
        QTRY_VERIFY(m_timeline->available() && m_timeline->cueCount() == 2);
        const auto source = selectedExternalSrtSource(m_player);
        QVERIFY2(source.available, qPrintable(source.status));
        QCOMPARE(source.exactUri, exact);
        QCOMPARE(source.uri, m_subtitle);
        const QByteArray selectedPrefix = QCryptographicHash::hash(exact, QCryptographicHash::Md5).toHex() + "/spu/";
        QVERIFY(source.trackId.startsWith(selectedPrefix));
        m_primaryTrack = source.trackId;
        m_primaryMedia = source.mediaToken;
        const auto view = m_timeline->view(0, VLC_TICK_FROM_SEC(10))["segments"].toList();
        QCOMPARE(view.size(), 2);
        QCOMPARE(view[0].toMap()["sourceStartTicks"].toLongLong(), VLC_TICK_FROM_SEC(1));
        QCOMPARE(view[1].toMap()["sourceEndTicks"].toLongLong(), VLC_TICK_FROM_SEC(6));
        m_timeline->refresh();
        assertPrimaryUnchanged();
    }
    void nativeDelayInheritanceOverrideAndFpsGuard()
    {
        {
            vlc_player_locker lock(m_player);
            const auto *track = vlc_player_GetSelectedTrack(m_player, SPU_ES);
            QCOMPARE(vlc_player_GetEsIdDelay(m_player, track->es_id), vlc_tick_t(INT64_MAX));
            vlc_player_SetSubtitleDelay(m_player, VLC_TICK_FROM_MS(1500), VLC_PLAYER_WHENCE_ABSOLUTE);
        }
        QTRY_VERIFY(m_timeline->available() && m_timeline->delayTicks() == VLC_TICK_FROM_MS(1500));
        auto view = m_timeline->view(0, VLC_TICK_FROM_SEC(10))["segments"].toList();
        QCOMPARE(view[0].toMap()["startTicks"].toLongLong(), VLC_TICK_FROM_MS(2500));
        QCOMPARE(view[0].toMap()["sourceStartTicks"].toLongLong(), VLC_TICK_FROM_SEC(1));
        {
            vlc_player_locker lock(m_player);
            const auto *track = vlc_player_GetSelectedTrack(m_player, SPU_ES);
            QCOMPARE(vlc_player_SetEsIdDelay(m_player, track->es_id, -VLC_TICK_FROM_MS(1500), VLC_PLAYER_WHENCE_ABSOLUTE), VLC_SUCCESS);
        }
        QTRY_VERIFY(m_timeline->available() && m_timeline->delayTicks() == -VLC_TICK_FROM_MS(1500));
        view = m_timeline->view(0, VLC_TICK_FROM_SEC(10))["segments"].toList();
        QCOMPARE(view[0].toMap()["startTicks"].toLongLong(), -VLC_TICK_FROM_MS(500));
        {
            vlc_player_locker lock(m_player);
            const auto *track = vlc_player_GetSelectedTrack(m_player, SPU_ES);
            QCOMPARE(vlc_player_SetEsIdDelay(m_player, track->es_id, INT64_MAX, VLC_PLAYER_WHENCE_ABSOLUTE), VLC_SUCCESS);
        }
        QTRY_VERIFY(m_timeline->available() && m_timeline->delayTicks() == VLC_TICK_FROM_MS(1500));
        {
            vlc_player_locker lock(m_player);
            vlc_player_SetAssociatedSubsFPS(m_player, 23.976f);
        }
        QTRY_VERIFY(!m_timeline->available() && m_timeline->status().contains("FPS"));
        QVERIFY(m_timeline->view(0, VLC_TICK_FROM_SEC(10))["segments"].toList().isEmpty());
        {
            vlc_player_locker lock(m_player);
            vlc_player_SetAssociatedSubsFPS(m_player, 0.f);
        }
        QTRY_VERIFY_WITH_TIMEOUT(m_timeline->available(), 10000);
        assertPrimaryUnchanged();
    }
    void actualCueSeekAndStaleRequestGuards()
    {
        QTRY_VERIFY(m_timeline->available());
        QVERIFY(m_timeline->seekCue(1, m_timeline->revision()));
        QTRY_VERIFY_WITH_TIMEOUT(std::abs(libvlc_media_player_get_time(m_mediaPlayer) -
            VLC_TICK_FROM_MS(5500)) <= VLC_TICK_FROM_MS(50), 5000);
        QVERIFY(!m_timeline->seekCue(-1, m_timeline->revision()));
        QVERIFY(!m_timeline->seekCue(999, m_timeline->revision()));
        const auto oldRevision = m_timeline->revision();
        {
            vlc_player_locker lock(m_player);
            vlc_player_SetSubtitleDelay(m_player, -VLC_TICK_FROM_MS(1500), VLC_PLAYER_WHENCE_ABSOLUTE);
        }
        // Reject before the GUI refresh processes the new native generation.
        QVERIFY(!m_timeline->seekCue(1, oldRevision));
        QTRY_VERIFY(m_timeline->available() && m_timeline->delayTicks() == -VLC_TICK_FROM_MS(1500));
        QVERIFY(!m_timeline->seekCue(0, m_timeline->revision())); // effective start -500ms
        QVERIFY(m_timeline->seekCue(1, m_timeline->revision()));
        QTRY_VERIFY_WITH_TIMEOUT(std::abs(libvlc_media_player_get_time(m_mediaPlayer) -
            VLC_TICK_FROM_MS(2500)) <= VLC_TICK_FROM_MS(50), 5000);
        {
            vlc_player_locker lock(m_player);
            vlc_player_SetSubtitleDelay(m_player, VLC_TICK_FROM_SEC(100), VLC_PLAYER_WHENCE_ABSOLUTE);
        }
        QTRY_VERIFY(m_timeline->available() && m_timeline->delayTicks() == VLC_TICK_FROM_SEC(100));
        QVERIFY(!m_timeline->seekCue(1, m_timeline->revision())); // beyond current media length
        {
            vlc_player_locker lock(m_player);
            vlc_player_SetSubtitleDelay(m_player, VLC_TICK_FROM_MS(1500), VLC_PLAYER_WHENCE_ABSOLUTE);
        }
        QTRY_VERIFY(m_timeline->available() && m_timeline->delayTicks() == VLC_TICK_FROM_MS(1500));
        const auto beforeReload = m_timeline->revision();
        m_timeline->reload();
        QVERIFY(!m_timeline->seekCue(1, beforeReload));
        QTRY_VERIFY_WITH_TIMEOUT(m_timeline->available(), 10000);
        QVERIFY(!m_timeline->seekCue(1, beforeReload));
        m_timeline->setEnabled(false);
        QVERIFY(!m_timeline->seekCue(1, m_timeline->revision()));
        m_timeline->setEnabled(true);
        QTRY_VERIFY_WITH_TIMEOUT(m_timeline->available(), 10000);
        const auto beforeFps = m_timeline->revision();
        {
            vlc_player_locker lock(m_player);
            vlc_player_SetAssociatedSubsFPS(m_player, 23.976f);
        }
        QVERIFY(!m_timeline->seekCue(1, beforeFps));
        {
            vlc_player_locker lock(m_player);
            vlc_player_SetAssociatedSubsFPS(m_player, 0.f);
        }
        QTRY_VERIFY_WITH_TIMEOUT(m_timeline->available(), 10000);
        assertPrimaryUnchanged();
    }
    void actualNormalLockAndExactMicrosecondSeekRequest()
    {
        QVERIFY(m_mediaPlayer);
        const auto primaryTime = libvlc_media_player_get_time(m_mediaPlayer);
        NormalTimelineFixture fixture;
        fixture.player = vlc_player_New(vlc_test_media_player_object(m_mediaPlayer),
                                       VLC_PLAYER_LOCK_NORMAL);
        QVERIFY(fixture.player);
        fixture.timeline = std::make_unique<SubtitleCueTimeline>(fixture.player);
        fixture.timeline->setEnabled(true);
        const auto video = QUrl::fromLocalFile(QDir(m_directory).filePath("subtitle-style.mp4")).toEncoded();
        auto *media = input_item_New(video.constData(), nullptr);
        QVERIFY(media);
        const auto release = std::unique_ptr<input_item_t, decltype(&input_item_Release)>(media, &input_item_Release);
        {
            vlc_player_locker lock(fixture.player);
            QCOMPARE(vlc_player_SetCurrentMedia(fixture.player, media), VLC_SUCCESS);
            vlc_player_SetStartPaused(fixture.player, false);
            QCOMPARE(vlc_player_Start(fixture.player), VLC_SUCCESS);
        }
        QTRY_COMPARE_WITH_TIMEOUT(fixture.state(), VLC_PLAYER_STATE_PLAYING, 10000);
        const auto exact = m_subtitle.toEncoded();
        {
            vlc_player_locker lock(fixture.player);
            QCOMPARE(vlc_player_AddAssociatedMedia(fixture.player, SPU_ES,
                         exact.constData(), true, false, true), VLC_SUCCESS);
        }
        QTRY_VERIFY_WITH_TIMEOUT(fixture.timeline->available() &&
            fixture.timeline->cueCount() == 2 && fixture.timeline->source() == m_subtitle, 10000);
        {
            vlc_player_locker lock(fixture.player);
            vlc_player_Pause(fixture.player);
        }
        QTRY_COMPARE_WITH_TIMEOUT(fixture.state(), VLC_PLAYER_STATE_PAUSED, 10000);
        QTRY_VERIFY_WITH_TIMEOUT(fixture.timeline->available(), 10000);
        const auto initial = selectedExternalSrtSource(fixture.player);
        QVERIFY2(initial.available && initial.canSeek, qPrintable(initial.status));
        QCOMPARE(initial.mediaToken, reinterpret_cast<quintptr>(media));
        QCOMPARE(initial.exactUri, exact);
        {
            vlc_player_locker lock(fixture.player);
            const auto *track = vlc_player_GetSelectedTrack(fixture.player, SPU_ES);
            QVERIFY(track);
            QCOMPARE(vlc_player_GetEsIdDelay(fixture.player, track->es_id), vlc_tick_t(INT64_MAX));
            vlc_player_SetSubtitleDelay(fixture.player, 352555, VLC_PLAYER_WHENCE_ABSOLUTE);
        }
        QTRY_VERIFY_WITH_TIMEOUT(fixture.timeline->available() &&
            fixture.timeline->delayTicks() == 352555, 10000);
        const auto pausedBeforeShift = fixture.time();
        const auto beforeShiftRevision = fixture.timeline->revision();
        // Exercise the timing write with the actual NORMAL lock too. No
        // player lock surrounds the request and the original player is idle.
        QVERIFY(fixture.timeline->setDelayTicks(352556, beforeShiftRevision));
        QVERIFY(!fixture.timeline->setDelayTicks(352557, beforeShiftRevision));
        QTRY_VERIFY_WITH_TIMEOUT(fixture.timeline->available() &&
            fixture.timeline->delayTicks() == 352556, 10000);
        QCOMPARE(fixture.time(), pausedBeforeShift);
        QCOMPARE(fixture.state(), VLC_PLAYER_STATE_PAUSED);
        QVERIFY(fixture.timeline->setDelayTicks(352555, fixture.timeline->revision()));
        QTRY_VERIFY_WITH_TIMEOUT(fixture.timeline->available() &&
            fixture.timeline->delayTicks() == 352555, 10000);
        const auto segments = fixture.timeline->view(0, VLC_TICK_FROM_SEC(10))["segments"].toList();
        QCOMPARE(segments.size(), 2);
        QCOMPARE(segments[1].toMap()["startTicks"].toLongLong(), qint64(4352555));
        static const vlc_player_timer_cbs timerCallbacks = {
            &NormalTimelineFixture::onUpdate, nullptr, &NormalTimelineFixture::onSeek
        };
        fixture.timer = vlc_player_AddTimer(fixture.player, VLC_TICK_FROM_MS(10),
                                            &timerCallbacks, &fixture);
        QVERIFY(fixture.timer);
        const auto revision = fixture.timeline->revision();
        // No outer player lock: seekCue must acquire exactly one NORMAL lock.
        QVERIFY(fixture.timeline->seekCue(1, revision));
        // timer.c reports the unrounded request synchronously. Millisecond
        // getters or frame-completion tolerance cannot prove this precision.
        QCOMPARE(fixture.requests.load(std::memory_order_acquire), 1u);
        QCOMPARE(fixture.requested.load(std::memory_order_relaxed), vlc_tick_t(4352555));
        // Observe actual asynchronous completion separately from the request.
        QTRY_VERIFY_WITH_TIMEOUT(fixture.completions.load(std::memory_order_acquire) > 0 &&
            std::abs(fixture.time() - 4352555) <= VLC_TICK_FROM_MS(50), 5000);
        QCOMPARE(fixture.state(), VLC_PLAYER_STATE_PAUSED);
        const auto after = selectedExternalSrtSource(fixture.player);
        QVERIFY2(after.available, qPrintable(after.status));
        QCOMPARE(after.mediaToken, initial.mediaToken);
        QCOMPARE(after.trackId, initial.trackId);
        QCOMPARE(after.exactUri, initial.exactUri);
        {
            vlc_player_locker lock(fixture.player);
            vlc_player_SetSubtitleDelay(fixture.player, 352556, VLC_PLAYER_WHENCE_ABSOLUTE);
        }
        QVERIFY(!fixture.timeline->seekCue(1, revision));
        QCOMPARE(fixture.requests.load(std::memory_order_acquire), 1u);
        QCOMPARE(libvlc_media_player_get_time(m_mediaPlayer), primaryTime);
        assertPrimaryUnchanged();
    }
    void actualGuardedTimingShiftAndRetainedPlayback()
    {
        QTRY_VERIFY_WITH_TIMEOUT(m_timeline->available(), 10000);
        const auto beforeTime = libvlc_media_player_get_time(m_mediaPlayer);
        const auto revision = m_timeline->revision();
        QVERIFY(m_timeline->setDelayTicks(1852555, revision));
        QVERIFY(!m_timeline->setDelayTicks(1852556, revision));
        QTRY_VERIFY_WITH_TIMEOUT(m_timeline->available() && m_timeline->delayTicks() == 1852555, 10000);
        {
            vlc_player_locker lock(m_player);
            const auto *track = vlc_player_GetSelectedTrack(m_player, SPU_ES);
            QVERIFY(track);
            QCOMPARE(vlc_player_GetEsIdDelay(m_player, track->es_id), vlc_tick_t(INT64_MAX));
            QCOMPARE(vlc_player_GetSubtitleDelay(m_player), vlc_tick_t(1852555));
            QCOMPARE(vlc_player_SetEsIdDelay(m_player, track->es_id, -1500555,
                         VLC_PLAYER_WHENCE_ABSOLUTE), VLC_SUCCESS);
        }
        QTRY_VERIFY_WITH_TIMEOUT(m_timeline->available() && m_timeline->delayTicks() == -1500555, 10000);
        QVERIFY(m_timeline->setDelayTicks(-1148000, m_timeline->revision()));
        QTRY_VERIFY_WITH_TIMEOUT(m_timeline->available() && m_timeline->delayTicks() == -1148000, 10000);
        {
            vlc_player_locker lock(m_player);
            const auto *track = vlc_player_GetSelectedTrack(m_player, SPU_ES);
            QVERIFY(track);
            QCOMPARE(vlc_player_GetEsIdDelay(m_player, track->es_id), vlc_tick_t(-1148000));
            QCOMPARE(vlc_player_GetSubtitleDelay(m_player), vlc_tick_t(1852555));
            QCOMPARE(vlc_player_SetEsIdDelay(m_player, track->es_id, INT64_MAX,
                         VLC_PLAYER_WHENCE_ABSOLUTE), VLC_SUCCESS);
            vlc_player_SetSubtitleDelay(m_player, VLC_TICK_FROM_MS(1500), VLC_PLAYER_WHENCE_ABSOLUTE);
        }
        QTRY_VERIFY_WITH_TIMEOUT(m_timeline->available() && m_timeline->delayTicks() == VLC_TICK_FROM_MS(1500), 10000);
        QVERIFY(!m_timeline->setDelayTicks(VLC_TICK_FROM_SEC(INT64_C(31) * 24 * 60 * 60), m_timeline->revision()));
        QVERIFY(!m_timeline->setDelayTicks(-VLC_TICK_FROM_SEC(INT64_C(31) * 24 * 60 * 60), m_timeline->revision()));
        const auto beforeFps = m_timeline->revision();
        {
            vlc_player_locker lock(m_player);
            vlc_player_SetAssociatedSubsFPS(m_player, 23.976f);
        }
        QVERIFY(!m_timeline->setDelayTicks(0, beforeFps));
        {
            vlc_player_locker lock(m_player);
            QCOMPARE(vlc_player_GetSubtitleDelay(m_player), VLC_TICK_FROM_MS(1500));
            vlc_player_SetAssociatedSubsFPS(m_player, 0.f);
        }
        QTRY_VERIFY_WITH_TIMEOUT(m_timeline->available(), 10000);
        const auto beforeDisable = m_timeline->revision();
        m_timeline->setEnabled(false);
        QVERIFY(!m_timeline->setDelayTicks(0, beforeDisable));
        m_timeline->setEnabled(true);
        QTRY_VERIFY_WITH_TIMEOUT(m_timeline->available(), 10000);
        QCOMPARE(libvlc_media_player_get_time(m_mediaPlayer), beforeTime);
        assertPrimaryUnchanged();
    }
    void durationEffectsDoNotPretendToBeSourceExpiry()
    {
        vout_thread_t *vout = nullptr;
        {
            vlc_player_locker lock(m_player);
            const auto *track = vlc_player_GetSelectedTrack(m_player, SPU_ES);
            vout = vlc_player_GetEsIdVout(m_player, track->es_id, nullptr);
            QVERIFY(vout);
            vout_Hold(vout);
        }
        const auto release = std::unique_ptr<vout_thread_t, decltype(&vout_Release)>(vout, &vout_Release);
        auto *filters = var_GetString(vout, "sub-filter");
        const QByteArray original = filters ? QByteArray(filters) : QByteArray{};
        free(filters);
        const auto before = libvlc_media_player_get_time(m_mediaPlayer);
        for (const auto *chain : {"subsdelay", " subsdelay ", "subsdelay {mode=0}"})
        {
            const auto revision = m_timeline->revision();
            QCOMPARE(var_SetString(vout, "sub-filter", chain), VLC_SUCCESS);
            QVERIFY(!m_timeline->seekCue(1, revision)); // live filter check before polling
            QVERIFY(!m_timeline->setDelayTicks(0, revision));
            QTRY_VERIFY(!m_timeline->available() && m_timeline->status().contains("duration effects"));
            QVERIFY(m_timeline->view(0, VLC_TICK_FROM_SEC(10))["segments"].toList().isEmpty());
            QCOMPARE(var_SetString(vout, "sub-filter", original.constData()), VLC_SUCCESS);
            QTRY_VERIFY_WITH_TIMEOUT(m_timeline->available(), 10000);
        }
        QCOMPARE(libvlc_media_player_get_time(m_mediaPlayer), before);
        assertPrimaryUnchanged();
    }
    void ambiguousSourcesFailClosed()
    {
        const auto second = QUrl::fromLocalFile(m_scratch.filePath("second source.srt"));
        QVERIFY(QFile::copy(m_subtitle.toLocalFile(), second.toLocalFile()));
        QCOMPARE(libvlc_media_player_add_slave(m_mediaPlayer, libvlc_media_slave_type_subtitle, second.toEncoded().constData(), true), 0);
        const QByteArray prefix = QCryptographicHash::hash(second.toEncoded(), QCryptographicHash::Md5).toHex() + "/spu/";
        auto secondTrackExists = [&] {
            vlc_player_locker lock(m_player);
            for (size_t i = 0; i < vlc_player_GetTrackCount(m_player, SPU_ES); ++i)
                if (QByteArray(vlc_es_id_GetStrId(vlc_player_GetTrackAt(m_player, SPU_ES, i)->es_id)).startsWith(prefix))
                    return true;
            return false;
        };
        QTRY_VERIFY_WITH_TIMEOUT(secondTrackExists(), 10000);
        {
            vlc_player_locker lock(m_player);
            const auto count = vlc_player_GetTrackCount(m_player, SPU_ES);
            QVERIFY(count > 0 && count <= 512);
            const vlc_player_track *selected = nullptr;
            for (size_t i = 0; i < count; ++i)
            {
                const auto *track = vlc_player_GetTrackAt(m_player, SPU_ES, i);
                if (QByteArray(vlc_es_id_GetStrId(track->es_id)).startsWith(prefix)) selected = track;
            }
            QVERIFY(selected);
            // Force-add or selecting an already-selected ES can retain the
            // other selection. Queue deselection before the sole-source list.
            vlc_es_id_t *list[] = {selected->es_id, nullptr};
            vlc_player_UnselectTrackCategory(m_player, SPU_ES);
            QCOMPARE(vlc_player_SelectEsIdList(m_player, SPU_ES, list), unsigned(1));
            vlc_player_SetTime(m_player, VLC_TICK_FROM_MS(3000));
        }
        QTRY_VERIFY_WITH_TIMEOUT(m_timeline->source() == second && m_timeline->available() &&
            m_timeline->delayTicks() == VLC_TICK_FROM_MS(1500), 10000);
        const auto revision = m_timeline->revision();
        {
            vlc_player_locker lock(m_player);
            const auto count = vlc_player_GetTrackCount(m_player, SPU_ES);
            QVERIFY(count > 0 && count <= 512);
            const vlc_player_track *first = nullptr;
            for (size_t i = 0; i < count; ++i)
            {
                const auto *track = vlc_player_GetTrackAt(m_player, SPU_ES, i);
                if (QByteArray(vlc_es_id_GetStrId(track->es_id)) == m_primaryTrack) first = track;
            }
            QVERIFY(first);
            QCOMPARE(vlc_player_SelectEsId(m_player, first->es_id, VLC_PLAYER_SELECT_SIMULTANEOUS), unsigned(2));
        }
        // Selection is an asynchronous input command. A still-current sole
        // source may legitimately accept a cue until the command is processed.
        QVERIFY(waitForNativeSubtitleSelectionCount(2));
        QVERIFY(!m_timeline->seekCue(0, revision));
        QVERIFY(!m_timeline->setDelayTicks(0, revision));
        QTRY_VERIFY(!m_timeline->available() && m_timeline->status().contains("one external"));
        QVERIFY(m_timeline->view(0, VLC_TICK_FROM_SEC(10))["segments"].toList().isEmpty());
    }
    void staleReadOnDisableAndMediaReplacement()
    {
        {
            vlc_player_locker lock(m_player);
            const auto count = vlc_player_GetTrackCount(m_player, SPU_ES);
            QVERIFY(count > 0 && count <= 512);
            const vlc_player_track *first = nullptr;
            for (size_t i = 0; i < count; ++i)
            {
                const auto *track = vlc_player_GetTrackAt(m_player, SPU_ES, i);
                if (QByteArray(vlc_es_id_GetStrId(track->es_id)) == m_primaryTrack) first = track;
            }
            QVERIFY(first);
            vlc_es_id_t *list[] = {first->es_id, nullptr};
            vlc_player_UnselectTrackCategory(m_player, SPU_ES);
            QCOMPARE(vlc_player_SelectEsIdList(m_player, SPU_ES, list), unsigned(1));
            vlc_player_SetTime(m_player, VLC_TICK_FROM_MS(3000));
        }
        QTRY_VERIFY_WITH_TIMEOUT(m_timeline->available() && m_timeline->source() == m_subtitle &&
            m_timeline->delayTicks() == VLC_TICK_FROM_MS(1500), 10000);
        m_timeline->reload();
        m_timeline->setEnabled(false);
        QTest::qWait(100);
        QVERIFY(!m_timeline->available() && !m_timeline->loading() && m_timeline->cueCount() == 0);
        m_timeline->setEnabled(true);
        const auto video = QDir::toNativeSeparators(QDir(m_directory).filePath("paused-seek-colors.mp4")).toUtf8();
        auto *media = libvlc_media_new_path(video.constData());
        QVERIFY(media);
        const auto oldRevision = m_timeline->revision();
        libvlc_media_player_set_media(m_mediaPlayer, media);
        QVERIFY(!m_timeline->seekCue(0, oldRevision));
        QVERIFY(!m_timeline->setDelayTicks(0, oldRevision));
        libvlc_media_release(media);
        QCOMPARE(libvlc_media_player_play(m_mediaPlayer), 0);
        QTRY_VERIFY_WITH_TIMEOUT(mediaToken() != m_primaryMedia, 10000);
        QTRY_VERIFY(!m_timeline->available() && m_timeline->cueCount() == 0 && m_timeline->source().isEmpty());
        QVERIFY(!m_timeline->seekCue(0, oldRevision));
    }
    void cleanupTestCase()
    {
        m_timeline.reset();
        if (m_mediaPlayer) libvlc_media_player_release(m_mediaPlayer);
        if (m_instance) libvlc_release(m_instance);
    }
};
QTEST_GUILESS_MAIN(TestSubtitleCueTimeline)
#include "test_subtitle_cue_timeline.moc"
