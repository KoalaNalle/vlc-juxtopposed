// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "player/video_transform.hpp"
#include <vlc/vlc.h>
#include <vlc_player.h>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QSignalSpy>
#include <QTest>
#include <QMutex>
#include <QMutexLocker>
#include <array>
#include <atomic>
#include <limits>

extern "C" vlc_player_t *vlc_test_media_player_core(libvlc_media_player_t *);

class TestVideoTransform : public QObject
{
    Q_OBJECT
    libvlc_instance_t *m_instance = nullptr;
    libvlc_media_player_t *m_mediaPlayer = nullptr;
    vout_thread_t *m_output = nullptr;
    VideoTransform m_transform;
    QString m_directory;
    QMutex m_mutex;
    alignas(32) std::array<unsigned char, 128 * 128 * 4> m_buffer;
    QImage m_image;
    std::atomic<unsigned> m_frames{0};
    static void *lock(void *opaque, void **planes)
    {
        auto *self = static_cast<TestVideoTransform *>(opaque);
        self->m_mutex.lock();
        *planes = self->m_buffer.data();
        return nullptr;
    }
    static void unlock(void *opaque, void *, void *const *)
    {
        auto *self = static_cast<TestVideoTransform *>(opaque);
        self->m_image = QImage(self->m_buffer.data(), 128, 128, 512, QImage::Format_RGB32).copy();
        self->m_mutex.unlock();
        self->m_frames.fetch_add(1);
    }
    static void display(void *, void *) {}
    QString picture(const QString &name)
    {
        // The fixture's quadrant order is R,G / B,Y. Check decoded output,
        // not just filter names. Waiting allows the live chain to rebuild.
        QTest::qWait(350);
        QImage image;
        {
            QMutexLocker guard(&m_mutex);
            image = m_image;
        }
        if (image.isNull()) return "rendered frame missing";
        if (!image.save(m_directory + '/' + name + ".png")) return "capture failed";
        QString colors;
        for (const auto &point : {QPoint(32, 32), QPoint(96, 32), QPoint(32, 96), QPoint(96, 96)})
        {
            const auto color = image.pixelColor(point);
            if (color.red() > 180 && color.green() < 80 && color.blue() < 80) colors += 'R';
            else if (color.green() > 180 && color.red() < 80 && color.blue() < 80) colors += 'G';
            else if (color.blue() > 180 && color.red() < 80 && color.green() < 80) colors += 'B';
            else if (color.red() > 180 && color.green() > 180 && color.blue() < 80) colors += 'Y';
            else colors += '?';
        }
        qInfo("%s: %s", qtu(name), qtu(colors));
        return colors;
    }
private slots:
    void initTestCase()
    {
        const auto fixture = qEnvironmentVariable("VLC_TRANSFORM_FIXTURE");
        if (fixture.isEmpty()) QSKIP("Set VLC_TRANSFORM_FIXTURE to the four-quadrant video for native pixel checks");
        m_directory = qEnvironmentVariable("VLC_TRANSFORM_OUTPUT", "video-transform-output");
        QVERIFY(QDir().mkpath(m_directory));
        const char *args[] = {"--ignore-config", "--no-media-library", "--no-audio", "--no-spu", "--no-osd",
                             "--no-video-title-show", "--no-hw-dec", "--verbose=2",
                             "--file-logging", "--logfile=video-transform-native.log"};
        m_instance = libvlc_new(ARRAY_SIZE(args), args);
        QVERIFY(m_instance);
        m_mediaPlayer = libvlc_media_player_new(m_instance, nullptr, nullptr);
        QVERIFY(m_mediaPlayer);
        libvlc_video_set_callbacks(m_mediaPlayer, lock, unlock, display, this);
        libvlc_video_set_format(m_mediaPlayer, "BGRA", 128, 128, 512);
        auto *media = libvlc_media_new_path(qtu(fixture));
        QVERIFY(media);
        libvlc_media_player_set_media(m_mediaPlayer, media);
        libvlc_media_release(media);
        QCOMPARE(libvlc_media_player_play(m_mediaPlayer), 0);
        // HoldAll (and therefore has_vout) can return a dummy before startup.
        // Wait for actual rendered frames before obtaining the live output.
        QTRY_VERIFY_WITH_TIMEOUT(m_frames.load() > 3, 10000);
        auto *player = vlc_test_media_player_core(m_mediaPlayer);
        vlc_player_Lock(player);
        m_output = vlc_player_vout_Hold(player);
        vlc_player_Unlock(player);
        QVERIFY(m_output);
        m_transform.resetOutput(m_output);
        QVERIFY(m_transform.available());
        QCOMPARE(picture("original"), "RGBY");
    }
    void actualFlipAndRotation()
    {
        m_transform.setFlip(1);
        QCOMPARE(picture("horizontal"), "GRYB");
        m_transform.setFlip(2);
        QCOMPARE(picture("vertical"), "BYRG");
        m_transform.setFlip(0);
        m_transform.setRotation(90.f);
        QCOMPARE(picture("rotate-90"), "GYRB");
        m_transform.setFlip(1);
        QCOMPARE(picture("horizontal-rotate-90"), "YGBR");
        // Arbitrary angles must reach the actual rotating filter too.
        m_transform.setFlip(0);
        m_transform.setRotation(37.f);
        QCOMPARE(m_transform.rotation(), 37.f);
        QCOMPARE(picture("rotate-37"), "RGBY");
        const auto corner = QImage(m_directory + "/rotate-37.png").pixelColor(0, 0);
        QVERIFY(corner.red() < 30 && corner.green() < 30 && corner.blue() < 30);
        m_transform.setRotation(360.f);
        QCOMPARE(picture("restored"), "RGBY");
    }
    void externalTransformNormalization_data()
    {
        QTest::addColumn<QString>("type");
        for (const auto &type : {"90", "180", "270", "hflip", "vflip", "transpose", "antitranspose"})
            QTest::newRow(type) << QString(type);
    }
    void externalTransformNormalization()
    {
        QFETCH(QString, type);
        QCOMPARE(var_SetString(m_output, "transform-type", qtu(type)), VLC_SUCCESS);
        QCOMPARE(var_SetString(m_output, "video-filter", "adjust{hue=0}:transform"), VLC_SUCCESS);
        const auto before = picture("external-" + type);
        QVERIFY(!before.contains('?'));
        QVERIFY(before != "RGBY");
        m_transform.setFlip(m_transform.flip());
        QCOMPARE(picture("normalized-" + type), before);
        const auto chain = vlc::wrap_cptr(var_GetString(m_output, "video-filter"));
        QVERIFY(QByteArray(chain.get()).startsWith("adjust{hue=0}"));
    }
    void engineSynchronizationValidationAndLifetime()
    {
        var_SetString(m_output, "video-filter", "rotate");
        QSignalSpy updates(&m_transform, &VideoTransform::stateChanged);
        var_SetFloat(m_output, "rotate-angle", 23.f);
        QTRY_VERIFY(updates.count() > 0);
        QCOMPARE(m_transform.rotation(), 23.f);
        m_transform.setRotation(std::numeric_limits<float>::quiet_NaN());
        m_transform.setRotation(-1.f);
        m_transform.setRotation(361.f);
        m_transform.setFlip(-1);
        m_transform.setFlip(3);
        QCOMPARE(m_transform.rotation(), 23.f);
        const char *other = "adjust{hue=0}:marq{marquee='a:b'}";
        var_SetString(m_output, "video-filter", other);
        m_transform.setFlip(1);
        const auto chain = vlc::wrap_cptr(var_GetString(m_output, "video-filter"));
        QCOMPARE(QByteArray(chain.get()), QByteArray(other) + ":transform");
        m_transform.resetOutput(nullptr);
        QVERIFY(!m_transform.available());
        m_transform.setFlip(2);
        m_transform.setRotation(45.f);
        QCOMPARE(m_transform.flip(), 0);
        QCOMPARE(m_transform.rotation(), 0.f);
        m_transform.resetOutput(m_output);
        QCOMPARE(m_transform.flip(), 1);
        m_transform.setFlip(0);
    }
    void cleanupTestCase()
    {
        m_transform.resetOutput(nullptr);
        if (m_output) { vout_Release(m_output); m_output = nullptr; }
        if (m_mediaPlayer) { libvlc_media_player_stop_async(m_mediaPlayer); libvlc_media_player_release(m_mediaPlayer); }
        if (m_instance) libvlc_release(m_instance);
    }
};
QTEST_GUILESS_MAIN(TestVideoTransform)
#include "test_video_transform.moc"
