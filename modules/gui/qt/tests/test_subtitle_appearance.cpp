// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "player/subtitle_appearance.hpp"
#include <vlc/vlc.h>
#include <vlc_vout_osd.h>
#include <vlc_subpicture.h>
#include <QDir>
#include <QImage>
#include <QMutex>
#include <QMutexLocker>
#include <QSignalSpy>
#include <QTest>
#include <QUrl>
#include <array>
#include <atomic>
#include <limits>
extern "C" vlc_player_t *vlc_test_media_player_core(libvlc_media_player_t *);

class TestSubtitleAppearance : public QObject
{
    Q_OBJECT
    libvlc_instance_t *m_instance = nullptr;
    libvlc_media_player_t *m_mediaPlayer = nullptr;
    vout_thread_t *m_output = nullptr;
    std::unique_ptr<SubtitleAppearance> m_style;
    QString m_directory;
    QMutex m_mutex;
    alignas(32) std::array<unsigned char, 320 * 180 * 4> m_buffer;
    QImage m_image;
    std::atomic<unsigned> m_frames{0};
    static void *lock(void *opaque, void **planes)
    {
        auto *self = static_cast<TestSubtitleAppearance *>(opaque);
        self->m_mutex.lock(); *planes = self->m_buffer.data(); return nullptr;
    }
    static void unlock(void *opaque, void *, void *const *)
    {
        auto *self = static_cast<TestSubtitleAppearance *>(opaque);
        self->m_image = QImage(self->m_buffer.data(), 320, 180, 1280, QImage::Format_RGB32).copy();
        self->m_mutex.unlock(); self->m_frames.fetch_add(1);
    }
    static void display(void *, void *) {}
    QImage frame(const QString &name = {})
    {
        if (!name.isEmpty()) QTest::qWait(400);
        QMutexLocker guard(&m_mutex);
        auto image = m_image;
        if (!name.isEmpty()) image.save(m_directory + '/' + name + ".png");
        return image;
    }
    static QRect pixels(const QImage &image, int kind, int *count = nullptr)
    {
        QRect bounds; int total = 0;
        for (int y = 0; y < image.height(); ++y)
            for (int x = 0; x < image.width(); ++x)
            {
                const auto c = image.pixelColor(x, y);
                const bool match = kind == 0 ? c.red() > 200 && c.green() > 200 && c.blue() > 200
                    : kind == 1 ? c.red() > 180 && c.green() < 80 && c.blue() < 80
                    : kind == 2 ? c.green() > 180 && c.red() < 80 && c.blue() < 80
                    : kind == 3 ? c.blue() > 180 && c.red() < 80 && c.green() < 80
                    : c.red() < 20 && c.green() < 20 && c.blue() < 20;
                if (match) { bounds |= QRect(x, y, 1, 1); ++total; }
            }
        if (count) *count = total;
        return bounds;
    }
private slots:
    void initTestCase()
    {
        const auto fixture = qEnvironmentVariable("VLC_SUBTITLE_FIXTURE");
        if (fixture.isEmpty()) QSKIP("Set VLC_SUBTITLE_FIXTURE to the generated subtitle-style.mp4");
        m_directory = qEnvironmentVariable("VLC_SUBTITLE_OUTPUT", "subtitle-appearance-output");
        QVERIFY(QDir().mkpath(m_directory));
        const char *args[] = {"--ignore-config", "--no-media-library", "--no-audio", "--no-osd",
            "--no-video-title-show", "--no-sub-autodetect-file", "--no-hw-dec", "--verbose=2",
            "--file-logging", "--logfile=subtitle-appearance-native.log"};
        m_instance = libvlc_new(ARRAY_SIZE(args), args); QVERIFY(m_instance);
        m_mediaPlayer = libvlc_media_player_new(m_instance, nullptr, nullptr); QVERIFY(m_mediaPlayer);
        libvlc_video_set_callbacks(m_mediaPlayer, lock, unlock, display, this);
        libvlc_video_set_format(m_mediaPlayer, "BGRA", 320, 180, 1280);
        auto *media = libvlc_media_new_path(qtu(fixture)); QVERIFY(media);
        libvlc_media_player_set_media(m_mediaPlayer, media); libvlc_media_release(media);
        QCOMPARE(libvlc_media_player_play(m_mediaPlayer), 0);
        QTRY_VERIFY_WITH_TIMEOUT(m_frames.load() > 3, 10000);
        const auto subtitle = QUrl::fromLocalFile(QFileInfo(fixture).absolutePath() + "/subtitle-style.srt").toEncoded();
        QCOMPARE(libvlc_media_player_add_slave(m_mediaPlayer, libvlc_media_slave_type_subtitle, subtitle.constData(), true), 0);
        QTRY_VERIFY_WITH_TIMEOUT(!pixels(frame(), 2).isEmpty(), 10000);
        auto *player = vlc_test_media_player_core(m_mediaPlayer);
        vlc_player_Lock(player); m_output = vlc_player_vout_Hold(player); vlc_player_Unlock(player);
        QVERIFY(m_output);
        m_style = std::make_unique<SubtitleAppearance>(player);
        m_style->resetOutput(m_output); QVERIFY(m_style->available());
        QVERIFY(!var_GetBool(m_output, "sub-text-style-enabled"));
        QVERIFY(!pixels(frame("embedded-original"), 2).isEmpty());
        // Pause inside the one long cue. Every subsequent change must update it.
        libvlc_media_player_set_pause(m_mediaPlayer, true);
        QTRY_COMPARE(libvlc_media_player_get_state(m_mediaPlayer), libvlc_Paused);
    }
    void embeddedFormattingAndLiveColor()
    {
        m_style->setEmbeddedFormat(false);
        QVERIFY(!pixels(frame("embedded-off"), 0).isEmpty());
        m_style->setTextColor("#ff0000");
        QCOMPARE(m_style->textColor(), "#FF0000");
        QVERIFY(!pixels(frame("red-text"), 1).isEmpty());
        m_style->setEmbeddedFormat(true);
        QVERIFY(!pixels(frame("embedded-restored"), 2).isEmpty());
        m_style->setEmbeddedFormat(false);
    }
    void fontUnitsFamilyAndScale()
    {
        m_style->setTextColor("#FFFFFF"); m_style->setOutlineSize(0);
        m_style->setSizeMode(1); m_style->setOutlineSize(0);
        m_style->setFontFamily("Arial"); m_style->setFontSize(18);
        const auto small = pixels(frame("font-18"), 0); QVERIFY(!small.isEmpty());
        m_style->setFontSize(28);
        const auto large = pixels(frame("font-28"), 0);
        QVERIFY2(large.height() > small.height() * 1.3, "Pixel font size did not change rendered glyph height");
        m_style->setFontFamily("Courier New");
        const auto mono = pixels(frame("font-monospace"), 0);
        qInfo("Glyph bounds Arial=%dx%d Courier=%dx%d", large.width(), large.height(), mono.width(), mono.height());
        QVERIFY2(mono.width() > large.width() * 1.4, "Font family did not change the repeated i glyph widths");
        m_style->setSizeMode(0); m_style->setFontSize(10);
        QVERIFY(!pixels(frame("font-relative"), 0).isEmpty());
        m_style->setSizeMode(1); m_style->setFontSize(18);
        auto *player = vlc_test_media_player_core(m_mediaPlayer);
        QSignalSpy updates(m_style.get(), &SubtitleAppearance::stateChanged);
        vlc_player_Lock(player); vlc_player_SetSubtitleTextScale(player, 200); vlc_player_Unlock(player);
        QTRY_VERIFY(updates.count() > 0);
        QCOMPARE(m_style->fontSize(), 36.f);
        const auto scaled = pixels(frame("font-scale-200"), 0);
        QVERIFY(scaled.height() > small.height() * 1.5);
        vlc_player_Lock(player); vlc_player_SetSubtitleTextScale(player, 100); vlc_player_Unlock(player);
    }
    void outlineBackgroundAndOffset()
    {
        m_style->setFontSize(28); m_style->setOutlineColor("#0000FF"); m_style->setOutlineSize(3);
        QVERIFY(!pixels(frame("blue-outline"), 3).isEmpty());
        m_style->setOutlineSize(0);
        int before = 0, after = 0;
        pixels(frame("background-off"), 4, &before);
        m_style->setBackgroundOpacity(100);
        pixels(frame("background-on"), 4, &after);
        QVERIFY2(after > before + 200, "Opaque subtitle background did not change output pixels");
        const auto original = pixels(frame(), 0);
        m_style->setOffset(20);
        const auto shifted = pixels(frame("offset-20"), 0);
        qInfo("Subtitle baseline before=%d after=%d; black pixels before=%d after=%d", original.bottom(), shifted.bottom(), before, after);
        QVERIFY(shifted.bottom() <= original.bottom() - 15);
    }
    void validationEngineChangesAndUnavailable()
    {
        QSignalSpy updates(m_style.get(), &SubtitleAppearance::stateChanged);
        var_SetInteger(m_output, "sub-text-style-color", 0xFF0000);
        QTRY_VERIFY(updates.count() > 0); QCOMPARE(m_style->textColor(), "#FF0000");
        QVERIFY(!pixels(frame("engine-red"), 1).isEmpty());
        var_SetBool(m_output, "sub-text-style-supported", false);
        QVERIFY(!m_style->available()); m_style->setTextColor("#00FF00");
        QCOMPARE(m_style->textColor(), "#FF0000");
        var_SetBool(m_output, "sub-text-style-supported", true);
        m_style->setTextColor("invalid"); m_style->setFontFamily("");
        m_style->setFontSize(std::numeric_limits<float>::quiet_NaN());
        m_style->setBackgroundOpacity(101); m_style->setSizeMode(2); m_style->setOffset(2001);
        QCOMPARE(m_style->textColor(), "#FF0000"); QCOMPARE(m_style->offset(), 20);
        m_style->resetOutput(nullptr); QVERIFY(!m_style->available());
        m_style->setTextColor("#00FF00");
        m_style->resetOutput(m_output); QCOMPARE(m_style->textColor(), "#FF0000");
        auto *player = vlc_test_media_player_core(m_mediaPlayer);
        vlc_player_Lock(player); vlc_player_UnselectTrackCategory(player, SPU_ES); vlc_player_Unlock(player);
        QTRY_VERIFY(!m_style->available()); m_style->setTextColor("#00FF00");
        QCOMPARE(m_style->textColor(), "#FF0000");
    }
    void playbackMessagesKeepTheirAppearance()
    {
        var_Create(m_output, "osd", VLC_VAR_BOOL | VLC_VAR_DOINHERIT);
        var_SetBool(m_output, "osd", true);
        vout_OSDText(m_output, 0, SUBPICTURE_ALIGN_TOP | SUBPICTURE_ALIGN_LEFT,
                     VLC_TICK_FROM_SEC(2), "OSD");
        const auto message = pixels(frame("osd-unaffected"), 0);
        QVERIFY2(!message.isEmpty() && message.bottom() < 90,
                 "Subtitle color/font overrides affected the playback message");
        var_Destroy(m_output, "osd");
    }
    void cleanupTestCase()
    {
        m_style.reset();
        if (m_output) { vout_Release(m_output); m_output = nullptr; }
        if (m_mediaPlayer) { libvlc_media_player_stop_async(m_mediaPlayer); libvlc_media_player_release(m_mediaPlayer); }
        if (m_instance) libvlc_release(m_instance);
    }
};
QTEST_GUILESS_MAIN(TestSubtitleAppearance)
#include "test_subtitle_appearance.moc"
