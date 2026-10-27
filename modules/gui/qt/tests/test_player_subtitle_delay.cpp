// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
// Exercise the actual PlayerController and its queued production callbacks.
// VLCTestingEnv supplies the native main-playlist NORMAL-lock player; only
// unrelated interface/discovery modules use the existing test infrastructure.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "vlc_stub_modules.hpp"
#include "player/player_controller.hpp"
#include "player/delay_estimator.hpp"
#include "player/subtitle_appearance.hpp"
#include "maininterface/subtitle_preset_model.hpp"
#include "maininterface/subtitle_attachment_model.hpp"
#include <vlc_input_item.h>
#include <vlc_vout.h>
#include <QDir>
#include <QCryptographicHash>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QFile>
#include <QImage>
#include <QRegularExpression>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlExtensionPlugin>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QThread>
#include <limits>

#ifdef QT_STATIC
Q_IMPORT_QML_PLUGIN(QtQmlPlugin)
Q_IMPORT_QML_PLUGIN(QtQmlModelsPlugin)
Q_IMPORT_QML_PLUGIN(QtQmlWorkerScriptPlugin)
#endif

using HeldEs = vlc::vlc_shared_data_ptr<vlc_es_id_t, &vlc_es_id_Hold, &vlc_es_id_Release>;

class TestPlayerSubtitleDelay : public QObject
{
    Q_OBJECT
    std::unique_ptr<VLCTestingEnv> m_environment;
    std::unique_ptr<PlayerController> m_controller;
    std::unique_ptr<SubtitlePresetModel> m_presets;
    std::unique_ptr<QSettings> m_settings;
    QTemporaryDir m_directory;
    vlc_player_t *m_player = nullptr;
    HeldEs m_first, m_second;
    QString m_fixture;
    int m_observations = 0;
    int m_fpsObservations = 0;

    static unsigned frameMarker(const QImage &image)
    {
        unsigned value = 0;
        for (unsigned bit = 0; bit < 11; ++bit)
            if (image.pixelColor(32 + 64 * bit, 20).red() > 128)
                value |= 1u << bit;
        return value;
    }
    QImage captureNativeFrame(const QString &name)
    {
        const auto directory = qEnvironmentVariable("VLC_FPS_SNAPSHOTS");
        if (directory.isEmpty()) return {};
        const auto path = QDir(directory).filePath(name + ".png");
        auto *output = vlc_player_vout_Hold(m_player);
        if (!output) return {};
        var_Create(output, "snapshot-width", VLC_VAR_INTEGER); var_SetInteger(output, "snapshot-width", 960);
        var_Create(output, "snapshot-height", VLC_VAR_INTEGER); var_SetInteger(output, "snapshot-height", 540);
        var_Create(output, "snapshot-path", VLC_VAR_STRING); var_SetString(output, "snapshot-path", qtu(path));
        var_Create(output, "snapshot-format", VLC_VAR_STRING); var_SetString(output, "snapshot-format", "png");
        var_TriggerCallback(output, "video-snapshot"); vout_Release(output);
        QImage image;
        for (int i=0; i<100 && image.isNull(); ++i) { QTest::qWait(20); image.load(path); }
        return image;
    }

    QPair<vlc_es_id_t *, vlc_es_id_t *> roles() const
    {
        QPair<vlc_es_id_t *, vlc_es_id_t *> result;
        vlc_player_locker lock{m_player};
        for (size_t i = 0; i < vlc_player_GetTrackCount(m_player, SPU_ES); ++i)
        {
            const auto *track = vlc_player_GetTrackAt(m_player, SPU_ES, i);
            if (!track->selected) continue;
            vlc_vout_order order = VLC_VOUT_ORDER_NONE;
            vlc_player_GetEsIdVout(m_player, track->es_id, &order);
            if (order == VLC_VOUT_ORDER_PRIMARY) result.first = track->es_id;
            if (order == VLC_VOUT_ORDER_SECONDARY) result.second = track->es_id;
        }
        return result;
    }
    unsigned select(vlc_es_id_t *primary, vlc_es_id_t *secondary = nullptr)
    {
        // Native EsOutSelectList skips already selected IDs; merely reversing
        // them cannot change their output roles. Clear real selection first.
        {
            vlc_player_locker lock{m_player};
            vlc_player_UnselectTrackCategory(m_player, SPU_ES);
        }
        if (!waitNativeRoles(nullptr, nullptr)) return unsigned(-1);
        // Renderer channels are allocated when the first cue is decoded, not
        // in request-list order. Establish the first real output before adding
        // the other one so this fixture has deliberate primary/secondary roles.
        vlc_es_id_t *ids[] = {primary, nullptr, nullptr};
        {
            vlc_player_locker lock{m_player};
            if (vlc_player_SelectEsIdList(m_player, SPU_ES, ids) != 1)
                return unsigned(-1);
            vlc_player_SetTime(m_player, VLC_TICK_FROM_MS(1500));
        }
        QElapsedTimer deadline; deadline.start();
        bool outputReady = false;
        while (deadline.elapsed() < 5000)
        {
            {
                vlc_player_locker lock{m_player};
                outputReady = vlc_player_GetEsIdVout(m_player, primary, nullptr);
            }
            if (outputReady) break;
            QThread::msleep(10);
        }
        if (!outputReady) return unsigned(-1);
        if (!secondary) return 1;
        ids[1] = secondary;
        vlc_player_locker lock{m_player};
        const auto selected = vlc_player_SelectEsIdList(m_player, SPU_ES, ids);
        // The short second SRT must have a real cue while the video is paused.
        // These native waits/seeks do not dispatch queued QObject callbacks.
        vlc_player_SetTime(m_player, VLC_TICK_FROM_MS(1500));
        return selected;
    }
    // Deliberately do not dispatch GUI events: the setter must read the current
    // native role while the controller's queued role cache is still old.
    bool waitNativeRoles(vlc_es_id_t *primary, vlc_es_id_t *secondary)
    {
        QElapsedTimer deadline; deadline.start();
        while (deadline.elapsed() < 5000)
        {
            if (roles() == qMakePair(primary, secondary)) return true;
            QThread::msleep(10);
        }
        return false;
    }
    vlc_tick_t own(vlc_es_id_t *id) const
    {
        vlc_player_locker lock{m_player};
        return vlc_player_GetEsIdDelay(m_player, id);
    }
    vlc_tick_t category() const
    {
        vlc_player_locker lock{m_player};
        return vlc_player_GetSubtitleDelay(m_player);
    }
    int setOwn(vlc_es_id_t *id, vlc_tick_t delay)
    {
        vlc_player_locker lock{m_player};
        return vlc_player_SetEsIdDelay(m_player, id, delay, VLC_PLAYER_WHENCE_ABSOLUTE);
    }
    void setCategory(vlc_tick_t delay)
    {
        vlc_player_locker lock{m_player};
        vlc_player_SetSubtitleDelay(m_player, delay, VLC_PLAYER_WHENCE_ABSOLUTE);
    }
    void observeAfterUnlock(VLCDuration)
    {
        QCOMPARE(QThread::currentThread(), m_controller->thread());
        // This direct QObject observer re-locks the actual NORMAL player.
        // Notification under the native lock deadlocks and the external test
        // runner's finite deadline reports failure, rather than a false pass.
        (void)m_controller->getInput();
        ++m_observations;
    }
private slots:
    void initTestCase()
    {
        m_fixture = qEnvironmentVariable("VLC_SUBTITLE_FIXTURE");
        if (m_fixture.isEmpty())
            QSKIP("Set VLC_SUBTITLE_FIXTURE to run the actual native controller diagnostic");
        QVERIFY2(QFileInfo::exists(m_fixture), "Use the real generated subtitle-style.mp4");
        const QDir fixtures(QFileInfo(m_fixture).absolutePath());
        QVERIFY(fixtures.exists("subtitle-style.srt"));
        QVERIFY(fixtures.exists("duration-cues.srt"));
        QVERIFY(m_directory.isValid());
        qputenv("VLC_TEST_TIMEOUT", "30");
        qRegisterMetaType<VLCDuration>();
        m_environment = std::make_unique<VLCTestingEnv>();
        QVERIFY(m_environment->init(true));
        m_player = m_environment->intf->p_player;
        QVERIFY(m_player);
        m_settings = std::make_unique<QSettings>(m_directory.filePath("settings.ini"), QSettings::IniFormat);
        m_environment->intf->mainSettings = m_settings.get();
        m_environment->intf->p_mi = nullptr;
        m_environment->intf->p_mainPlaylistController = nullptr;
        // The existing stub defaults to a dummy text renderer. Use production
        // Freetype so Default applies through the actual SubtitlePresetModel.
        {
            vlc_player_locker lock{m_player};
            // PlayerController's production variable proxies use this native
            // object view too; the public player type is opaque in C++.
            auto *playerObject = reinterpret_cast<vlc_object_t *>(m_player);
            var_Create(playerObject, "text-renderer", VLC_VAR_STRING);
            QCOMPARE(var_SetString(playerObject, "text-renderer", "freetype"), VLC_SUCCESS);
            var_Create(playerObject, "sub-autodetect-file", VLC_VAR_BOOL);
            QCOMPARE(var_SetBool(playerObject, "sub-autodetect-file", false), VLC_SUCCESS);
        }
        var_Create(m_environment->intf, "sub-delay", VLC_VAR_INTEGER);
        var_SetInteger(m_environment->intf, "sub-delay", 0);
        var_Create(m_environment->intf, "sub-fps", VLC_VAR_FLOAT);
        var_SetFloat(m_environment->intf, "sub-fps", 0.f);
        // Exercise a value present before input creation. This emulates the
        // player's inherited FPS value; it is not a persistence/restart test.
        if (qEnvironmentVariable("VLC_TEST_INITIAL_SUBTITLE_FPS") == "50")
        {
            vlc_player_locker lock{m_player};
            vlc_player_SetAssociatedSubsFPS(m_player, 50.f);
        }
        m_controller = std::make_unique<PlayerController>(m_environment->intf);
        m_environment->intf->p_mainPlayerController = m_controller.get();
        connect(m_controller.get(), &PlayerController::subtitleDelayChanged, this,
                &TestPlayerSubtitleDelay::observeAfterUnlock, Qt::DirectConnection);
        connect(m_controller.get(), &PlayerController::secondarySubtitleDelayChanged, this,
                &TestPlayerSubtitleDelay::observeAfterUnlock, Qt::DirectConnection);
        connect(m_controller.get(), &PlayerController::secondarySubtitleSelectionChanged, this,
                [this] { observeAfterUnlock(VLCDuration()); }, Qt::DirectConnection);
        connect(m_controller.get(), &PlayerController::subtitleSelectionChanged, this,
                [this] { observeAfterUnlock(VLCDuration()); }, Qt::DirectConnection);
        m_presets = std::make_unique<SubtitlePresetModel>(m_player, m_controller->getSubtitleAppearance(),
                m_settings.get(), VLC_OBJECT(m_environment->intf));
        const auto uri = QUrl::fromLocalFile(m_fixture).toEncoded();
        auto *item = input_item_New(uri.constData(), "Native controller subtitle fixture");
        QVERIFY(item);
        int configured, started;
        {
            vlc_player_locker lock{m_player};
            configured = vlc_player_SetCurrentMedia(m_player, item);
            started = configured == VLC_SUCCESS ? vlc_player_Start(m_player) : VLC_EGENERIC;
        }
        input_item_Release(item);
        QCOMPARE(configured, VLC_SUCCESS);
        QCOMPARE(started, VLC_SUCCESS);
        QTRY_VERIFY_WITH_TIMEOUT(m_controller->hasVideoOutput(), 10000);
        for (const auto &name : {"subtitle-style.srt", "duration-cues.srt"})
        {
            const auto subtitle = QUrl::fromLocalFile(fixtures.filePath(name)).toEncoded();
            vlc_player_locker lock{m_player};
            QCOMPARE(vlc_player_AddAssociatedMedia(m_player, SPU_ES, subtitle.constData(), true, false, true), VLC_SUCCESS);
        }
        // A media may already contain two embedded tracks. Wait for the actual
        // associated sources instead of mistaking those initial rows for SRTs.
        auto findAssociated = [this, &fixtures](const QString &name, HeldEs &id) {
            if (id) return true;
            const auto uri = QUrl::fromLocalFile(fixtures.filePath(name)).toEncoded();
            const QByteArray prefix = QCryptographicHash::hash(uri, QCryptographicHash::Md5).toHex() + '/';
            vlc_player_locker lock{m_player};
            for (size_t i = 0; i < vlc_player_GetTrackCount(m_player, SPU_ES); ++i) {
                auto *es = vlc_player_GetTrackAt(m_player, SPU_ES, i)->es_id;
                if (QByteArray(vlc_es_id_GetStrId(es)).startsWith(prefix)) {
                    id.reset(es);
                    return true;
                }
            }
            return false;
        };
        QTRY_VERIFY_WITH_TIMEOUT(findAssociated("subtitle-style.srt", m_first), 10000);
        QTRY_VERIFY_WITH_TIMEOUT(findAssociated("duration-cues.srt", m_second), 10000);
        QVERIFY(m_first != m_second);
        QCOMPARE(select(m_first.get(), m_second.get()), 2u);
        QTRY_VERIFY_WITH_TIMEOUT(roles() == qMakePair(m_first.get(), m_second.get()), 5000);
        QTRY_VERIFY_WITH_TIMEOUT(m_controller->getSubtitleAppearance()->available(), 10000);
        QTRY_VERIFY_WITH_TIMEOUT(m_presets->canApply(), 5000);
        {
            vlc_player_locker lock{m_player};
            vlc_player_Pause(m_player);
        }
        QTRY_COMPARE_WITH_TIMEOUT(m_controller->getPlayingState(), PlayerController::PLAYING_STATE_PAUSED, 5000);
    }
    void initialAssociatedFpsMatchesNative()
    {
        vlc_player_locker lock{m_player};
        const auto fps = vlc_player_GetAssociatedSubsFPS(m_player);
        QCOMPARE(fps, 0.f); // the generated fixture inherits the native default
        QCOMPARE(m_controller->getSubtitleFPS(), fps);
    }
    void associatedFpsRetimesHeldCaption()
    {
        const auto directory = qEnvironmentVariable("VLC_FPS_SNAPSHOTS");
        if (directory.isEmpty())
            QSKIP("Set VLC_FPS_SNAPSHOTS and use the 60-second frame-marker fixture");
        QVERIFY(QDir(directory).exists());
        QCOMPARE(select(m_first.get()), 1u);
        QTRY_VERIFY(roles() == qMakePair(m_first.get(), static_cast<vlc_es_id_t *>(nullptr)));
        m_controller->setSubtitleFPS(0.f);
        QTRY_COMPARE(m_controller->getSubtitleFPS(), 0.f);

        auto seek = [this](vlc_tick_t time) {
            vlc_player_locker lock{m_player};
            vlc_player_SetTime(m_player, time);
        };
        auto time = [this] {
            vlc_player_locker lock{m_player};
            return vlc_player_GetTime(m_player);
        };
        auto snapshot = [this, &directory](const QString &name) {
            const auto path = QDir(directory).filePath(name + ".png");
            vout_thread_t *output;
            {
                vlc_player_locker lock{m_player};
                output = vlc_player_vout_Hold(m_player);
            }
            if (!output) return QImage();
            var_Create(output, "snapshot-width", VLC_VAR_INTEGER);
            var_SetInteger(output, "snapshot-width", 960);
            var_Create(output, "snapshot-height", VLC_VAR_INTEGER);
            var_SetInteger(output, "snapshot-height", 540);
            var_Create(output, "snapshot-path", VLC_VAR_STRING);
            var_SetString(output, "snapshot-path", qtu(path));
            var_Create(output, "snapshot-format", VLC_VAR_STRING);
            var_SetString(output, "snapshot-format", "png");
            var_TriggerCallback(output, "video-snapshot");
            vout_Release(output);
            QImage image;
            for (int i = 0; i < 100 && image.isNull(); ++i) {
                QTest::qWait(20);
                image.load(path);
            }
            return image.convertToFormat(QImage::Format_RGBA8888);
        };
        auto captionMask = [](const QImage &image) {
            QByteArray mask;
            for (int y = image.height() * 3 / 4; y < image.height(); ++y)
                for (int x = 0; x < image.width(); ++x) {
                    const auto c = image.pixelColor(x, y);
                    mask.append(char(c.red() > 160 && c.green() > 160 && c.blue() > 160));
                }
            return mask;
        };
        auto captionMatches = [](const QByteArray &actual, const QByteArray &reference) {
            if (actual.size() != reference.size()) return false;
            qsizetype intersection = 0, united = 0;
            for (qsizetype i = 0; i < actual.size(); ++i) {
                intersection += actual[i] && reference[i];
                united += actual[i] || reference[i];
            }
            // Reference cues sit over different marker colors. Their five
            // threshold-edge antialias pixels differ; the wrong cue differs
            // by thousands. Keep 99.5% overlap and exact held-video matching.
            return united > 100 && intersection * 1000 >= united * 995;
        };
        seek(VLC_TICK_FROM_SEC(24));
        QTest::qWait(700);
        const auto middle = snapshot("normal-middle-reference");
        QVERIFY(!middle.isNull());
        seek(VLC_TICK_FROM_SEC(12));
        QTest::qWait(700);
        const auto first = snapshot("normal-first-reference");
        QVERIFY(!first.isNull());
        const auto firstMask = captionMask(first), middleMask = captionMask(middle);
        QVERIFY(firstMask.count(char(1)) > 100);
        QVERIFY(middleMask.count(char(1)) > 100);
        QVERIFY(firstMask != middleMask);
        QCOMPARE(frameMarker(first), 300u);
        QCOMPARE(frameMarker(middle), 600u);
        const auto heldTime = time();
        // A paused timer may retain its previous playing point after a seek.
        // The decoded marker proves the landed frame; edits must preserve both
        // that exact picture and the timer's held value independently.
        const auto picture = first.copy(0, 0, first.width(), first.height() * 3 / 4);
        bool captionsMatch = true, picturesMatch = true, positionsMatch = true;
        for (const auto fps : {50.f, 29.97f, 23.976f, 0.f}) {
            m_controller->setSubtitleFPS(fps);
            QTRY_COMPARE(m_controller->getSubtitleFPS(), fps);
            QTest::qWait(700);
            const auto image = snapshot(QString("live-%1").arg(fps, 0, 'f', 3));
            QVERIFY(!image.isNull());
            const bool caption = captionMatches(captionMask(image), fps == 50.f ? middleMask : firstMask);
            const bool frame = image.copy(0, 0, image.width(), image.height() * 3 / 4) == picture;
            const bool position = qAbs(time() - heldTime) < VLC_TICK_FROM_MS(50);
            qInfo("FPS %.3f caption=%d heldFrame=%d heldTime=%d time=%lld", double(fps),
                  caption, frame, position, static_cast<long long>(time()));
            captionsMatch &= caption; picturesMatch &= frame; positionsMatch &= position;
        }
        m_controller->setSubtitleFPS(50.f);
        QTRY_COMPARE(m_controller->getSubtitleFPS(), 50.f);
        QTest::qWait(300);
        seek(VLC_TICK_FROM_SEC(12));
        QTest::qWait(700);
        const auto reseek = snapshot("seek-with-fps-50");
        QVERIFY(!reseek.isNull());
        captionsMatch &= captionMatches(captionMask(reseek), middleMask);
        picturesMatch &= reseek.copy(0, 0, reseek.width(), reseek.height() * 3 / 4) == picture;
        positionsMatch &= qAbs(time() - heldTime) < VLC_TICK_FROM_MS(50);
        m_controller->setSubtitleFPS(0.f);
        QTRY_COMPARE(m_controller->getSubtitleFPS(), 0.f);
        QTest::qWait(700);
        const auto reset = snapshot("reset-after-fps-seek");
        QVERIFY(!reset.isNull());
        captionsMatch &= captionMatches(captionMask(reset), firstMask);
        picturesMatch &= reset.copy(0, 0, reset.width(), reset.height() * 3 / 4) == picture;
        positionsMatch &= qAbs(time() - heldTime) < VLC_TICK_FROM_MS(50);
        QCOMPARE(select(m_first.get(), m_second.get()), 2u);
        QTRY_VERIFY(roles() == qMakePair(m_first.get(), m_second.get()));
        QVERIFY2(captionsMatch, "Live FPS/reset must update the held external cue without a user seek");
        QVERIFY2(picturesMatch, "FPS must preserve the held video picture");
        QVERIFY2(positionsMatch, "FPS must preserve the held video time");
    }
    // Run this function alone in a fresh process: earlier FPS edits initialize
    // the subtitle source and would mask the first-change regression.
    void firstDefaultFpsChangeRetimesPlayingCaption()
    {
        if (qEnvironmentVariable("VLC_TEST_FIRST_FPS_FRESH") != "1")
            QSKIP("Run alone in a fresh process with VLC_TEST_FIRST_FPS_FRESH=1");
        const auto directory = qEnvironmentVariable("VLC_FPS_SNAPSHOTS");
        QVERIFY2(!directory.isEmpty() && QDir(directory).exists(), "Set snapshot output directory");
        { vlc_player_locker lock{m_player};
          QCOMPARE(vlc_player_GetAssociatedSubsFPS(m_player), 0.f);
          QCOMPARE(vlc_player_GetState(m_player), VLC_PLAYER_STATE_PAUSED); }
        QCOMPARE(select(m_first.get()), 1u);
        QTRY_VERIFY(roles() == qMakePair(m_first.get(), static_cast<vlc_es_id_t *>(nullptr)));
        auto seek = [this](vlc_tick_t t) { vlc_player_locker lock{m_player}; vlc_player_SetTime(m_player, t); };
        auto time = [this] { vlc_player_locker lock{m_player}; return vlc_player_GetTime(m_player); };
        auto snapshot = [this, &directory](const QString &name) {
            const auto path = QDir(directory).filePath(name + ".png");
            vout_thread_t *output;
            { vlc_player_locker lock{m_player}; output = vlc_player_vout_Hold(m_player); }
            if (!output) return QImage();
            var_Create(output, "snapshot-width", VLC_VAR_INTEGER); var_SetInteger(output, "snapshot-width", 960);
            var_Create(output, "snapshot-height", VLC_VAR_INTEGER); var_SetInteger(output, "snapshot-height", 540);
            var_Create(output, "snapshot-path", VLC_VAR_STRING); var_SetString(output, "snapshot-path", qtu(path));
            var_Create(output, "snapshot-format", VLC_VAR_STRING); var_SetString(output, "snapshot-format", "png");
            var_TriggerCallback(output, "video-snapshot"); vout_Release(output);
            QImage image;
            for (int i = 0; i < 100 && image.isNull(); ++i) { QTest::qWait(20); image.load(path); }
            return image.convertToFormat(QImage::Format_RGBA8888);
        };
        auto mask = [](const QImage &image) {
            QByteArray pixels;
            for (int y = image.height() * 3 / 4; y < image.height(); ++y)
                for (int x = 0; x < image.width(); ++x) {
                    const auto c = image.pixelColor(x, y);
                    pixels.append(char(c.red() > 160 && c.green() > 160 && c.blue() > 160));
                }
            return pixels;
        };
        auto matches = [](const QByteArray &a, const QByteArray &b) {
            if (a.size() != b.size()) return false;
            qsizetype intersection = 0, united = 0;
            for (qsizetype i = 0; i < a.size(); ++i) { intersection += a[i] && b[i]; united += a[i] || b[i]; }
            return united > 100 && intersection * 1000 >= united * 995;
        };
        // Both reference captures use default 0 FPS and take place before any FPS setter.
        seek(VLC_TICK_FROM_SEC(24)); QTest::qWait(700);
        const auto middle = snapshot("fresh-fps-middle-reference");
        QVERIFY(!middle.isNull()); QCOMPARE(frameMarker(middle), 600u);
        seek(VLC_TICK_FROM_SEC(12)); QTest::qWait(700);
        const auto first = snapshot("fresh-fps-first-reference");
        QVERIFY(!first.isNull()); QCOMPARE(frameMarker(first), 300u);
        const auto firstMask = mask(first), middleMask = mask(middle);
        QVERIFY(firstMask.count(char(1)) > 100 && middleMask.count(char(1)) > 100);
        QVERIFY(!matches(firstMask, middleMask));
        { vlc_player_locker lock{m_player}; vlc_player_Resume(m_player); }
        QTRY_COMPARE(m_controller->getPlayingState(), PlayerController::PLAYING_STATE_PLAYING);
        QTRY_VERIFY(time() > VLC_TICK_FROM_SEC(12) + VLC_TICK_FROM_MS(100));
        QVERIFY(time() < VLC_TICK_FROM_SEC(14));
        const auto before = time();
        m_controller->setSubtitleFPS(50.f); // first ever FPS edit for this input
        QTRY_COMPARE(m_controller->getSubtitleFPS(), 50.f);
        QTest::qWait(700);
        const auto actual = snapshot("fresh-first-playing-fps50");
        const auto after = time();
        QVERIFY(!actual.isNull());
        QVERIFY2(after > before && after - before < VLC_TICK_FROM_SEC(3) && after < VLC_TICK_FROM_SEC(16),
                 "The main playback clock must advance without a primary seek");
        QVERIFY2(frameMarker(actual) >= 300u && frameMarker(actual) < 400u,
                 "The captured decoded frame must remain in the expected window");
        const auto actualMask = mask(actual);
        qInfo("Fresh 0->50 playing caption matches first=%d middle=%d clock %lld->%lld",
              matches(actualMask, firstMask), matches(actualMask, middleMask),
              static_cast<long long>(before), static_cast<long long>(after));
        QVERIFY2(matches(actualMask, middleMask) && !matches(actualMask, firstMask),
                 "The first default->50 FPS edit must replace the old cue without a player seek");
    }
    void firstFpsChangeRetimesDelayedPlayingCaption_data()
    {
        QTest::addColumn<int>("delayMs");
        QTest::addColumn<int>("videoSeconds");
        QTest::addColumn<int>("referenceSeconds");
        QTest::newRow("positive") << 5000 << 12 << 14;
        QTest::newRow("negative") << -5000 << 12 << 34;
        QTest::newRow("positive-change-cue") << 10000 << 24 << 28;
    }
    // Run each data row alone in its own fresh process.
    void firstFpsChangeRetimesDelayedPlayingCaption()
    {
        if (qEnvironmentVariable("VLC_TEST_FIRST_FPS_FRESH") != "1")
            QSKIP("Run each row alone in a fresh process with VLC_TEST_FIRST_FPS_FRESH=1");
        QFETCH(int, delayMs);
        QFETCH(int, videoSeconds);
        QFETCH(int, referenceSeconds);
        const auto directory = qEnvironmentVariable("VLC_FPS_SNAPSHOTS");
        QVERIFY2(!directory.isEmpty() && QDir(directory).exists(), "Set snapshot output directory");
        { vlc_player_locker lock{m_player};
          QCOMPARE(vlc_player_GetAssociatedSubsFPS(m_player), 0.f);
          QCOMPARE(vlc_player_GetState(m_player), VLC_PLAYER_STATE_PAUSED); }
        QCOMPARE(select(m_first.get()), 1u);
        QTRY_VERIFY(roles() == qMakePair(m_first.get(), static_cast<vlc_es_id_t *>(nullptr)));
        const auto seek = [this](vlc_tick_t value) {
            vlc_player_locker lock{m_player}; vlc_player_SetTime(m_player, value);
        };
        const auto time = [this] {
            vlc_player_locker lock{m_player}; return vlc_player_GetTime(m_player);
        };
        // At FPS 50 the effective source time is twice (video time minus
        // the subtitle delay). Capture the expected cue before touching FPS.
        seek(VLC_TICK_FROM_SEC(referenceSeconds));
        QTest::qWait(800);
        const auto expected = captureNativeFrame("delayed-fps-reference");
        QVERIFY(!expected.isNull());
        QCOMPARE(frameMarker(expected), unsigned(referenceSeconds * 25));
        m_controller->setSubtitleDelayMS(delayMs);
        QTRY_COMPARE(m_controller->getSubtitleDelayMS(), delayMs);
        seek(VLC_TICK_FROM_SEC(videoSeconds));
        QTest::qWait(800);
        { vlc_player_locker lock{m_player}; vlc_player_Resume(m_player); }
        QTRY_COMPARE(m_controller->getPlayingState(), PlayerController::PLAYING_STATE_PLAYING);
        QTRY_VERIFY(time() > VLC_TICK_FROM_SEC(videoSeconds) + VLC_TICK_FROM_MS(100));
        QVERIFY(time() < VLC_TICK_FROM_SEC(videoSeconds + 2));
        const auto before = time();
        m_controller->setSubtitleFPS(50.f);
        QTRY_COMPARE(m_controller->getSubtitleFPS(), 50.f);
        QTest::qWait(700);
        const auto actual = captureNativeFrame("delayed-first-playing-fps50");
        const auto after = time();
        QVERIFY(!actual.isNull());
        QVERIFY(after > before && after - before < VLC_TICK_FROM_SEC(3));
        QVERIFY(frameMarker(actual) >= unsigned(videoSeconds * 25)
                && frameMarker(actual) < unsigned((videoSeconds + 4) * 25));
        unsigned intersection = 0, united = 0;
        for (int y = actual.height() * 3 / 4; y < actual.height(); ++y)
            for (int x = 0; x < actual.width(); ++x)
            {
                const auto a = actual.pixelColor(x, y), b = expected.pixelColor(x, y);
                const bool ca = a.red() > 160 && a.green() > 160 && a.blue() > 160;
                const bool cb = b.red() > 160 && b.green() > 160 && b.blue() > 160;
                intersection += ca && cb;
                united += ca || cb;
            }
        qInfo("First FPS edit delay=%d caption intersection=%u union=%u clock=%lld->%lld",
              delayMs, intersection, united, static_cast<long long>(before), static_cast<long long>(after));
        QVERIFY2(united > 100 && intersection * 1000 >= united * 995,
                 "First FPS change must replace and retime the cue with the selected delay");
    }

    // Run alone with VLC_TEST_INITIAL_SUBTITLE_FPS=50 in a fresh process.
    // The FPS exists before input creation and associated-source attachment.
    void inheritedFpsDisplaysFirstAssociatedCue()
    {
        if (qEnvironmentVariable("VLC_TEST_INITIAL_SUBTITLE_FPS") != "50")
            QSKIP("Run alone with VLC_TEST_INITIAL_SUBTITLE_FPS=50");
        const auto directory = qEnvironmentVariable("VLC_FPS_SNAPSHOTS");
        QVERIFY2(!directory.isEmpty() && QDir(directory).exists(), "Set snapshot output directory");
        { vlc_player_locker lock{m_player};
          QCOMPARE(vlc_player_GetAssociatedSubsFPS(m_player), 50.f); }
        QCOMPARE(select(m_first.get()), 1u);
        QTRY_VERIFY(roles() == qMakePair(m_first.get(), static_cast<vlc_es_id_t *>(nullptr)));
        { vlc_player_locker lock{m_player}; vlc_player_SetTime(m_player, VLC_TICK_FROM_SEC(12)); }
        QTest::qWait(800);
        const auto inherited = captureNativeFrame("inherited-fps-first-associated-cue");
        QVERIFY(!inherited.isNull());
        QCOMPARE(frameMarker(inherited), 300u);
        m_controller->setSubtitleFPS(0.f);
        QTRY_COMPARE(m_controller->getSubtitleFPS(), 0.f);
        { vlc_player_locker lock{m_player}; vlc_player_SetTime(m_player, VLC_TICK_FROM_SEC(24)); }
        QTest::qWait(800);
        const auto reference = captureNativeFrame("inherited-fps-default-reference");
        QVERIFY(!reference.isNull());
        QCOMPARE(frameMarker(reference), 600u);
        unsigned intersection = 0, united = 0;
        for (int y = inherited.height() * 3 / 4; y < inherited.height(); ++y)
            for (int x = 0; x < inherited.width(); ++x)
            {
                const auto a = inherited.pixelColor(x, y), b = reference.pixelColor(x, y);
                const bool ca = a.red() > 160 && a.green() > 160 && a.blue() > 160;
                const bool cb = b.red() > 160 && b.green() > 160 && b.blue() > 160;
                intersection += ca && cb;
                united += ca || cb;
            }
        QVERIFY2(united > 100 && intersection * 1000 >= united * 995,
                 "An associated source added with pre-set 50 FPS must show its correctly retimed cue");
    }

    void associatedFpsRetimesPlayingCaption()
    {
        const auto directory = qEnvironmentVariable("VLC_FPS_SNAPSHOTS");
        if (directory.isEmpty())
            QSKIP("Set VLC_FPS_SNAPSHOTS and use the 60-second frame-marker fixture");
        QVERIFY(QDir(directory).exists());
        QCOMPARE(select(m_first.get()), 1u);
        QTRY_VERIFY(roles() == qMakePair(m_first.get(), static_cast<vlc_es_id_t *>(nullptr)));
        auto snapshot = [this, &directory](const QString &name) {
            const auto path = QDir(directory).filePath(name + ".png");
            vout_thread_t *output;
            {
                vlc_player_locker lock{m_player};
                output = vlc_player_vout_Hold(m_player);
            }
            if (!output) return QImage();
            var_Create(output, "snapshot-width", VLC_VAR_INTEGER);
            var_SetInteger(output, "snapshot-width", 960);
            var_Create(output, "snapshot-height", VLC_VAR_INTEGER);
            var_SetInteger(output, "snapshot-height", 540);
            var_Create(output, "snapshot-path", VLC_VAR_STRING);
            var_SetString(output, "snapshot-path", qtu(path));
            var_Create(output, "snapshot-format", VLC_VAR_STRING);
            var_SetString(output, "snapshot-format", "png");
            var_TriggerCallback(output, "video-snapshot");
            vout_Release(output);
            QImage image;
            for (int i = 0; i < 100 && image.isNull(); ++i) {
                QTest::qWait(20);
                image.load(path);
            }
            return image.convertToFormat(QImage::Format_RGBA8888);
        };
        auto captionMask = [](const QImage &image) {
            QByteArray result;
            for (int y = image.height() * 3 / 4; y < image.height(); ++y)
                for (int x = 0; x < image.width(); ++x) {
                    const auto color = image.pixelColor(x, y);
                    result.append(char(color.red() > 160 && color.green() > 160 && color.blue() > 160));
                }
            return result;
        };
        auto matches = [](const QByteArray &actual, const QByteArray &expected) {
            if (actual.size() != expected.size()) return false;
            qsizetype intersection = 0, united = 0;
            for (qsizetype i = 0; i < actual.size(); ++i) {
                intersection += actual[i] && expected[i];
                united += actual[i] || expected[i];
            }
            return united > 100 && intersection * 1000 >= united * 995;
        };
        auto time = [this] {
            vlc_player_locker lock{m_player};
            return vlc_player_GetTime(m_player);
        };
        m_controller->setSubtitleFPS(50.f);
        {
            vlc_player_locker lock{m_player};
            vlc_player_SetTime(m_player, VLC_TICK_FROM_SEC(12));
        }
        QTest::qWait(700);
        const auto middle = snapshot("playing-middle-reference");
        QVERIFY(!middle.isNull());
        m_controller->setSubtitleFPS(0.f);
        QTest::qWait(700);
        const auto first = snapshot("playing-first-reference");
        QVERIFY(!first.isNull());
        const auto firstMask = captionMask(first), middleMask = captionMask(middle);
        QVERIFY(firstMask != middleMask);
        QCOMPARE(frameMarker(first), 300u);
        QCOMPARE(frameMarker(middle), 300u);
        {
            vlc_player_locker lock{m_player};
            vlc_player_Resume(m_player);
        }
        QTRY_COMPARE(m_controller->getPlayingState(), PlayerController::PLAYING_STATE_PLAYING);
        qInfo("Resume from frame 300: first raw clock=%lld", static_cast<long long>(time()));
        QTRY_VERIFY(time() > VLC_TICK_FROM_SEC(12) + VLC_TICK_FROM_MS(100));
        qInfo("Resume from frame 300: updated raw clock=%lld", static_cast<long long>(time()));
        const auto resumedDiagnostic = snapshot("resume-frame-300-diagnostic");
        QVERIFY(!resumedDiagnostic.isNull());
        qInfo("Resume diagnostic decoded frame=%u", frameMarker(resumedDiagnostic));
        // The timer can keep its paused point until the first output update.
        // Qualify the actual decoded progress too, within two media seconds.
        QVERIFY(time() < VLC_TICK_FROM_SEC(14));
        QVERIFY(frameMarker(resumedDiagnostic) >= 300u);
        QVERIFY(frameMarker(resumedDiagnostic) < 350u);
        bool captionsMatch = true, clocksAdvance = true;
        for (const auto fps : {50.f, 0.f, 29.97f, 23.976f, 0.f}) {
            const auto before = time();
            m_controller->setSubtitleFPS(fps);
            QTRY_COMPARE(m_controller->getSubtitleFPS(), fps);
            QTest::qWait(350);
            const auto image = snapshot(QString("playing-%1-%2").arg(fps, 0, 'f', 3).arg(before));
            QVERIFY(!image.isNull());
            const auto after = time();
            const bool caption = matches(captionMask(image), fps == 50.f ? middleMask : firstMask);
            const bool clock = after > before && after - before < VLC_TICK_FROM_SEC(2)
                            && after < VLC_TICK_FROM_SEC(16);
            qInfo("Playing FPS %.3f caption=%d advancingClock=%d before=%lld after=%lld", double(fps),
                  caption, clock, static_cast<long long>(before), static_cast<long long>(after));
            captionsMatch &= caption;
            clocksAdvance &= clock;
        }
        {
            vlc_player_locker lock{m_player};
            vlc_player_Pause(m_player);
        }
        QTRY_COMPARE(m_controller->getPlayingState(), PlayerController::PLAYING_STATE_PAUSED);
        // At 50 FPS all three external cues have ended by 30 seconds. Reset
        // while playing must revive that EOF source, without a primary seek.
        m_controller->setSubtitleFPS(50.f);
        {
            vlc_player_locker lock{m_player};
            vlc_player_SetTime(m_player, VLC_TICK_FROM_SEC(32));
        }
        QTest::qWait(700);
        const auto ended = snapshot("playing-eof-reference");
        QVERIFY(!ended.isNull());
        QCOMPARE(captionMask(ended).count(char(1)), qsizetype(0));
        QCOMPARE(frameMarker(ended), 800u); // actual 25 FPS frame at 32 seconds
        m_controller->setSubtitleFPS(0.f);
        QTest::qWait(700);
        const auto revived = snapshot("playing-revived-reference");
        QVERIFY(!revived.isNull());
        QCOMPARE(frameMarker(revived), 800u);
        const auto revivedMask = captionMask(revived);
        QVERIFY(revivedMask.count(char(1)) > 100);
        m_controller->setSubtitleFPS(50.f);
        QTest::qWait(700);
        qInfo("EOF paused raw clock=%lld state=%d", static_cast<long long>(time()),
              int(m_controller->getPlayingState()));
        {
            vlc_player_locker lock{m_player};
            vlc_player_Resume(m_player);
        }
        QTRY_COMPARE(m_controller->getPlayingState(), PlayerController::PLAYING_STATE_PLAYING);
        // The raw main timer can retain its last playing point during a paused
        // seek. The snapshots above prove the landed frame independently.
        // Establish the real resumed clock before testing a playing FPS edit.
        QTRY_VERIFY(time() > VLC_TICK_FROM_SEC(32) + VLC_TICK_FROM_MS(100));
        QVERIFY(time() < VLC_TICK_FROM_SEC(34));
        for (const auto fps : {0.f, 50.f, 0.f}) {
            const auto before = time();
            m_controller->setSubtitleFPS(fps);
            QTRY_COMPARE(m_controller->getSubtitleFPS(), fps);
            QTest::qWait(350);
            const auto image = snapshot(QString("playing-eof-%1-%2").arg(fps).arg(before));
            QVERIFY(!image.isNull());
            const auto mask = captionMask(image);
            const bool caption = fps == 0.f ? matches(mask, revivedMask) : mask.count(char(1)) == 0;
            const auto after = time();
            const bool clock = after > before && after - before < VLC_TICK_FROM_SEC(2)
                            && after < VLC_TICK_FROM_SEC(36);
            qInfo("Playing EOF FPS %.3f caption=%d advancingClock=%d before=%lld after=%lld", double(fps),
                  caption, clock, static_cast<long long>(before), static_cast<long long>(after));
            captionsMatch &= caption;
            clocksAdvance &= clock;
        }
        {
            vlc_player_locker lock{m_player};
            vlc_player_Pause(m_player);
        }
        QTRY_COMPARE(m_controller->getPlayingState(), PlayerController::PLAYING_STATE_PAUSED);
        QCOMPARE(select(m_first.get(), m_second.get()), 2u);
        QTRY_VERIFY(roles() == qMakePair(m_first.get(), m_second.get()));
        QVERIFY2(captionsMatch, "Playing FPS changes must replace queued cues and revive subtitle EOF");
        QVERIFY2(clocksAdvance, "The primary clock must continue without an FPS-induced seek");
    }
    void associatedFpsNotificationsAndValidation()
    {
        QSignalSpy changes(m_controller.get(), &PlayerController::subtitleFPSChanged);
        const int observationsBefore = m_fpsObservations;
        const auto connection = connect(m_controller.get(), &PlayerController::subtitleFPSChanged,
            this, [this](float) {
                QCOMPARE(QThread::currentThread(), m_controller->thread());
                // A direct observer must be able to re-lock the NORMAL player.
                (void)m_controller->getInput();
                ++m_fpsObservations;
            }, Qt::DirectConnection);
        {
            vlc_player_locker lock{m_player};
            vlc_player_SetAssociatedSubsFPS(m_player, 23.976f);
        }
        QCOMPARE(m_fpsObservations, observationsBefore);
        QTRY_COMPARE(m_controller->getSubtitleFPS(), 23.976f);
        QVERIFY(!changes.isEmpty());
        QCOMPARE(changes.last().front().toFloat(), 23.976f);

        const auto *input = m_controller->getInput();
        const auto selected = roles();
        const auto categoryBefore = category();
        m_controller->setSubtitleFPS(29.97f);
        {
            vlc_player_locker lock{m_player};
            QCOMPARE(vlc_player_GetAssociatedSubsFPS(m_player), 29.97f);
        }
        QTRY_COMPARE(m_controller->getSubtitleFPS(), 29.97f);
        const int before = changes.size();
        for (const auto invalid : {-1.f, std::numeric_limits<float>::infinity(),
                                  std::numeric_limits<float>::quiet_NaN()})
            m_controller->setSubtitleFPS(invalid);
        QTest::qWait(30);
        QCOMPARE(changes.size(), before);
        {
            vlc_player_locker lock{m_player};
            QCOMPARE(vlc_player_GetAssociatedSubsFPS(m_player), 29.97f);
        }
        m_controller->setSubtitleFPS(0.f);
        QTRY_COMPARE(m_controller->getSubtitleFPS(), 0.f);
        QCOMPARE(m_controller->getInput(), input);
        QCOMPARE(roles(), selected);
        QCOMPARE(category(), categoryBefore);
        QVERIFY(m_fpsObservations >= observationsBefore + 3);
        disconnect(connection);
    }
    void largeTrackDelaysKeepHeldFrameAndResume_data()
    {
        QTest::addColumn<bool>("inheritedSecondary");
        QTest::newRow("own-secondary") << false;
        QTest::newRow("inherited-secondary") << true;
    }
    void largeTrackDelaysKeepHeldFrameAndResume()
    {
        QFETCH(bool, inheritedSecondary);
        if (qEnvironmentVariable("VLC_FPS_SNAPSHOTS").isEmpty())
            QSKIP("Set VLC_FPS_SNAPSHOTS for the generated frame-marker fixture");
        HeldEs embedded;
        {
            vlc_player_locker lock{m_player};
            for (size_t i = 0; i < vlc_player_GetTrackCount(m_player, SPU_ES); ++i)
            {
                const auto *track = vlc_player_GetTrackAt(m_player, SPU_ES, i);
                if (track->fmt.psz_language && !strcmp(track->fmt.psz_language, "fin"))
                    embedded.reset(track->es_id);
            }
        }
        QVERIFY(embedded);
        QCOMPARE(select(m_first.get(), embedded.get()), 2u);
        QTRY_VERIFY(roles() == qMakePair(m_first.get(), embedded.get()));
        m_controller->setSubtitleFPS(0.f);
        m_controller->setSubtitleDelayMS(0);
        QCOMPARE(setOwn(embedded.get(), inheritedSecondary ? INT64_MAX : 0), VLC_SUCCESS);
        QTRY_COMPARE(m_controller->getSecondarySubtitleDelayMS(), 0);
        {
            vlc_player_locker lock{m_player};
            vlc_player_ChangeRate(m_player, 1.5f);
            vlc_player_SetTime(m_player, VLC_TICK_FROM_SEC(12));
        }
        QTest::qWait(800);
        const auto capture = [this, inheritedSecondary](const QString &name) {
            return captureNativeFrame((inheritedSecondary ? QStringLiteral("inherited-")
                                                         : QStringLiteral("own-")) + name);
        };
        const auto baseline = capture(QStringLiteral("large-delay-reference"));
        QVERIFY(!baseline.isNull());
        QCOMPARE(frameMarker(baseline), 300u);
        const auto time = [this] {
            vlc_player_locker lock{m_player}; return vlc_player_GetTime(m_player);
        };
        const auto heldTime = time();
        const auto observe = [this, heldTime, &time, &capture](const QString &name) {
            QTest::qWait(800);
            const auto image = capture(name);
            if (image.isNull()) return false;
            unsigned primaryPixels = 0;
            for (int y = image.height()*3/4; y < image.height(); ++y)
                for (int x = 0; x < image.width(); ++x) {
                    const auto c = image.pixelColor(x,y);
                    primaryPixels += c.red()>160 && c.green()>160 && c.blue()>160;
                }
            const auto actualTime = time();
            qInfo("Large delay %s marker=%u primaryCaption=%u time=%lld", qtu(name),
                  frameMarker(image), primaryPixels, static_cast<long long>(actualTime));
            return frameMarker(image) == 300u && primaryPixels > 100
                && qAbs(actualTime-heldTime) <= VLC_TICK_FROM_MS(100);
        };
        m_controller->setSubtitleFPS(50.f);
        QVERIFY(observe("large-fps-50"));
        m_controller->setSubtitleDelayMS(5000);
        QTRY_COMPARE(m_controller->getSecondarySubtitleDelayMS(), inheritedSecondary ? 5000 : 0);
        QCOMPARE(own(embedded.get()), inheritedSecondary ? INT64_MAX : vlc_tick_t(0));
        QVERIFY(observe("large-primary-5000"));
        m_controller->setSecondarySubtitleDelayMS(-9000);
        QVERIFY(observe("large-secondary-minus9000"));
        m_controller->setSubtitleDelayMS(0);
        QVERIFY(observe("large-primary-reset"));
        QCOMPARE(m_controller->getSecondarySubtitleDelayMS(), -9000);
        m_controller->setSecondarySubtitleDelayMS(0);
        QVERIFY(observe("large-secondary-reset"));
        m_controller->setSubtitleFPS(0.f);
        QVERIFY(observe("large-fps-reset"));
        {
            vlc_player_locker lock{m_player}; vlc_player_Resume(m_player);
        }
        QTRY_COMPARE(m_controller->getPlayingState(), PlayerController::PLAYING_STATE_PLAYING);
        QTRY_VERIFY_WITH_TIMEOUT(time() > heldTime + VLC_TICK_FROM_SEC(2), 6000);
        const auto resumed = capture(QStringLiteral("large-delay-resumed"));
        QVERIFY(!resumed.isNull());
        qInfo("Large delay resumed marker=%u time=%lld", frameMarker(resumed),
              static_cast<long long>(time()));
        QVERIFY(frameMarker(resumed) >= 350u && frameMarker(resumed) < 400u);
        {
            vlc_player_locker lock{m_player}; vlc_player_Pause(m_player);
        }
        QTRY_COMPARE(m_controller->getPlayingState(), PlayerController::PLAYING_STATE_PAUSED);
        {
            vlc_player_locker lock{m_player}; vlc_player_ChangeRate(m_player, 1.f);
        }
        QCOMPARE(select(m_first.get(), m_second.get()), 2u);
        QTRY_VERIFY(roles() == qMakePair(m_first.get(), m_second.get()));
    }
    void estimatorDirectionsAndCancellation()
    {
        DelayEstimator estimator;
        QSignalSpy changes(&estimator, &DelayEstimator::delayChanged);
        QVERIFY(!estimator.isHeardTimeMarked());
        QVERIFY(!estimator.isSpottedTimeMarked());
        estimator.markHeardTime();
        QVERIFY(estimator.isHeardTimeMarked());
        QTest::qWait(30);
        estimator.markSpottedTime();
        QCOMPARE(changes.size(), 1);
        QVERIFY(estimator.getDelay().valid());
        QVERIFY(estimator.getDelay().toVLCTick() <= -VLC_TICK_FROM_MS(20));
        QVERIFY(estimator.getDelay().toVLCTick() > -VLC_TICK_FROM_SEC(3));
        QVERIFY(!estimator.isHeardTimeMarked());
        QVERIFY(!estimator.isSpottedTimeMarked());
        estimator.reset();
        QVERIFY(!estimator.getDelay().valid());
        estimator.markSpottedTime();
        QTest::qWait(30);
        estimator.markHeardTime();
        QCOMPARE(changes.size(), 3);
        QVERIFY(estimator.getDelay().valid());
        QVERIFY(estimator.getDelay().toVLCTick() >= VLC_TICK_FROM_MS(20));
        QVERIFY(estimator.getDelay().toVLCTick() < VLC_TICK_FROM_SEC(3));
        estimator.reset();
        const auto beforeCancel = changes.size();
        estimator.markHeardTime();
        estimator.markHeardTime();
        QVERIFY(!estimator.isHeardTimeMarked());
        estimator.markSpottedTime();
        QCOMPARE(changes.size(), beforeCancel);
        QVERIFY(estimator.isSpottedTimeMarked());
        estimator.reset();
        QVERIFY(!estimator.isHeardTimeMarked());
        QVERIFY(!estimator.isSpottedTimeMarked());
        QVERIFY(!estimator.getDelay().valid());
    }
    void estimatorCancellationDoesNotPublishAnAdjustment()
    {
        DelayEstimator estimator;
        QSignalSpy changes(&estimator, &DelayEstimator::delayChanged);
        estimator.markHeardTime();
        estimator.cancel();
        QVERIFY(!estimator.isHeardTimeMarked());
        QVERIFY(!estimator.isSpottedTimeMarked());
        QVERIFY(changes.isEmpty());
        estimator.markSpottedTime();
        QVERIFY(estimator.isSpottedTimeMarked());
        QVERIFY(changes.isEmpty()); // cannot complete a pair from the old input
        QTest::qWait(30);
        estimator.markHeardTime();
        QCOMPARE(changes.size(), 1);
        const auto applied = estimator.getDelay().toVLCTick();
        QVERIFY(applied >= VLC_TICK_FROM_MS(20));
        estimator.markSpottedTime();
        estimator.cancel();
        estimator.cancel(); // repeated input-detach/rate notifications are safe
        QVERIFY(!estimator.isHeardTimeMarked());
        QVERIFY(!estimator.isSpottedTimeMarked());
        QCOMPARE(changes.size(), 1);
        QCOMPARE(estimator.getDelay().toVLCTick(), applied);
    }
    void categoryRelativeAdjustmentsAndResetNotifications()
    {
        QCOMPARE(select(m_first.get(), m_second.get()), 2u);
        QTRY_VERIFY(roles() == qMakePair(m_first.get(), m_second.get()));
        setCategory(350000);
        QCOMPARE(setOwn(m_first.get(), INT64_MAX), VLC_SUCCESS);
        QCOMPARE(setOwn(m_second.get(), 650000), VLC_SUCCESS);
        m_controller->setAudioDelayMS(350);
        QTRY_COMPARE(m_controller->getAudioDelayMS(), 350);
        QTRY_COMPARE(m_controller->getSubtitleDelayMS(), 350);
        QTRY_COMPARE(m_controller->getSecondarySubtitleDelayMS(), 650);
        QSignalSpy audioChanges(m_controller.get(), &PlayerController::audioDelayChanged);
        QSignalSpy subtitleChanges(m_controller.get(), &PlayerController::subtitleDelayChanged);
        const auto audioObserver = connect(m_controller.get(), &PlayerController::audioDelayChanged,
            this, &TestPlayerSubtitleDelay::observeAfterUnlock, Qt::DirectConnection);
        auto expected = vlc_tick_t(350000);
        for (const auto delta : {vlc_tick_t(50000), vlc_tick_t(-75750)})
        {
            const int observations = m_observations;
            m_controller->addAudioDelay(VLCDuration(delta));
            m_controller->addSubtitleDelay(VLCDuration(delta));
            QCOMPARE(m_observations, observations); // no direct signal under the lock
            expected += delta;
            QCOMPARE(category(), expected);
            QTRY_COMPARE(m_controller->getAudioDelay().toVLCTick(), expected);
            QTRY_COMPARE(m_controller->getSubtitleDelay().toVLCTick(), expected);
            QVERIFY(!audioChanges.isEmpty());
            QVERIFY(!subtitleChanges.isEmpty());
            QCOMPARE(audioChanges.last().front().value<VLCDuration>().toVLCTick(), expected);
            QCOMPARE(subtitleChanges.last().front().value<VLCDuration>().toVLCTick(), expected);
            QCOMPARE(own(m_second.get()), vlc_tick_t(650000));
            QCOMPARE(m_controller->getSecondarySubtitleDelayMS(), 650);
        }
        // Follow the ordinary Reset binding while a direct re-locking observer
        // remains connected, as when the legacy synchronization editor exists.
        DelayEstimator estimator;
        connect(&estimator, &DelayEstimator::delayChanged, this, [this, &estimator] {
            m_controller->addAudioDelay(estimator.getDelay());
            m_controller->addSubtitleDelay(estimator.getDelay());
            m_controller->addSecondarySubtitleDelay(estimator.getDelay());
        });
        const auto audioCount = audioChanges.size();
        const auto subtitleCount = subtitleChanges.size();
        estimator.markHeardTime();
        estimator.reset(); // an invalid estimate must not publish a delta
        QTest::qWait(30);
        QCOMPARE(audioChanges.size(), audioCount);
        QCOMPARE(subtitleChanges.size(), subtitleCount);
        QCOMPARE(category(), expected);
        QCOMPARE(own(m_second.get()), vlc_tick_t(650000));
        m_controller->setSubtitleDelayMS(0);
        estimator.reset();
        QTRY_COMPARE(m_controller->getSubtitleDelayMS(), 0);
        QCOMPARE(m_controller->getSecondarySubtitleDelayMS(), 650);
        m_controller->setSecondarySubtitleDelayMS(0);
        estimator.reset();
        QTRY_COMPARE(m_controller->getSecondarySubtitleDelayMS(), 0);
        QCOMPARE(own(m_second.get()), vlc_tick_t(0));
        m_controller->setAudioDelayMS(0);
        estimator.reset();
        QTRY_COMPARE(m_controller->getAudioDelayMS(), 0);
        QVERIFY(m_observations > 0);
        disconnect(audioObserver);
    }
    void estimatorAppliesMediaTime_data()
    {
        QTest::addColumn<QString>("route");
        QTest::addColumn<float>("rate");
        QTest::addColumn<bool>("positive");
        for (const auto *route : {"audio", "primary", "secondary"})
            for (float rate : {.5f, 1.f, 1.5f, 2.f})
                for (bool positive : {false, true})
                    QTest::newRow(qPrintable(QStringLiteral("%1-%2-%3").arg(
                        QString::fromLatin1(route)).arg(rate).arg(positive ? "delay" : "advance")))
                        << QString::fromLatin1(route) << rate << positive;
    }
    void estimatorAppliesMediaTime()
    {
        QFETCH(QString, route); QFETCH(float, rate); QFETCH(bool, positive);
        const bool audio = route == QStringLiteral("audio");
        const bool secondary = route == QStringLiteral("secondary");
        const QDir pages(qEnvironmentVariable("VLC_TRACKS_SOURCE_DIR"));
        QVERIFY2(!qEnvironmentVariable("VLC_TRACKS_SOURCE_DIR").isEmpty(),
                 "Supply frozen production synchronization QML pages");
        QFile source(pages.filePath(audio ? QStringLiteral("TracksPageAudio.qml")
                                         : QStringLiteral("TracksPageSubtitle.qml")));
        QVERIFY(source.open(QIODevice::ReadOnly));
        const QString text = QString::fromUtf8(source.readAll());
        const QString method = audio ? QStringLiteral("addAudioTimingIfCurrent")
            : secondary ? QStringLiteral("addSecondarySubtitleDelay") : QStringLiteral("addSubtitleDelay");
        const QRegularExpression call(QStringLiteral("Player\\.%1\\([^\\r\\n]+").arg(method));
        const auto match = call.match(text);
        QVERIFY(match.hasMatch());
        // Execute the actual production handler, including its duration conversion.
        // The native category/role values below are independent of the QML binding.
        QCOMPARE(select(m_first.get(), m_second.get()), 2u);
        QTRY_VERIFY(roles() == qMakePair(m_first.get(), m_second.get()));
        setCategory(350000);
        QCOMPARE(setOwn(m_first.get(), INT64_MAX), VLC_SUCCESS);
        QCOMPARE(setOwn(m_second.get(), 650000), VLC_SUCCESS);
        m_controller->setAudioDelayMS(350);
        m_controller->setRate(rate);
        QTRY_COMPARE(m_controller->getRate(), rate);
        QTRY_COMPARE(m_controller->getSubtitleDelayMS(), 350);
        QTRY_COMPARE(m_controller->getSecondarySubtitleDelayMS(), 650);
        QTRY_COMPARE(m_controller->getAudioDelayMS(), 350);
        DelayEstimator estimator;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty(QStringLiteral("Player"), m_controller.get());
        for (const auto *id : {"delayEstimator", "delayEstimatorPrimary", "delayEstimatorSecondary"})
            engine.rootContext()->setContextProperty(QString::fromLatin1(id), &estimator);
        auto handler = match.captured();
        if (audio) {
            const auto token = m_controller->beginAudioTiming();
            QVERIFY(!token.isEmpty());
            engine.rootContext()->setContextProperty(QStringLiteral("testToken"), token);
            handler.replace(QStringLiteral(", token)"), QStringLiteral(", testToken)"));
        }
        QQmlComponent component(&engine);
        component.setData(QByteArrayLiteral("import QtQml\nQtObject { property QtObject hook: Connections { target: delayEstimator; function onDelayChanged() { ")
            + handler.toUtf8() + QByteArrayLiteral(" } } }"),
            QUrl(QStringLiteral("file:///timing-rate-native.qml")));
        QScopedPointer<QObject> root(component.create());
        QVERIFY2(root, qPrintable(component.errorString()));
        if (positive) estimator.markSpottedTime(); else estimator.markHeardTime();
        QTest::qWait(80);
        if (positive) estimator.markHeardTime(); else estimator.markSpottedTime();
        const auto wall = estimator.getDelay().toVLCTick();
        QVERIFY(qAbs(wall) >= VLC_TICK_FROM_MS(60));
        QVERIFY(positive ? wall > 0 : wall < 0);
        const vlc_tick_t expected = (secondary ? 650000 : 350000) + vlc_tick_t(wall * rate);
        auto actual = [&] { return audio ? m_controller->getAudioDelay().toVLCTick()
            : secondary ? own(m_second.get()) : category(); };
        QTRY_VERIFY_WITH_TIMEOUT(qAbs(actual() - expected) <= 1, 750);
        if (!audio) QCOMPARE(m_controller->getAudioDelayMS(), 350);
        if (audio || secondary) QCOMPARE(category(), vlc_tick_t(350000));
        if (!secondary) QCOMPARE(own(m_second.get()), vlc_tick_t(650000));
        estimator.reset();
        QTest::qWait(20);
        QVERIFY(qAbs(actual() - expected) <= 1); // clearing marks must not change the applied delay
        if (audio) m_controller->setAudioDelayMS(0);
        else if (secondary) m_controller->setSecondarySubtitleDelayMS(0);
        else m_controller->setSubtitleDelayMS(0);
        QTRY_COMPARE(actual(), vlc_tick_t(0));
        m_controller->setRate(1.f);
        m_controller->setAudioDelayMS(0);
        setCategory(0);
        QCOMPARE(setOwn(m_second.get(), 0), VLC_SUCCESS);
    }
    void timingContextCancellation_data()
    {
        QTest::addColumn<bool>("subtitle");
        QTest::addColumn<QString>("change");
        for (const auto *page : {"audio", "subtitle"})
            for (const auto *change : {"qt-seek", "native-seek", "qt-rate", "native-rate", "native-resume", "native-pause"})
                QTest::newRow(qPrintable(QString::fromLatin1(page)+QLatin1Char('-')+QString::fromLatin1(change)))
                    << (QByteArray(page)=="subtitle") << QString::fromLatin1(change);
        for (const auto *change : {"native-all-unselect", "native-secondary-unselect"})
            QTest::newRow(qPrintable(QStringLiteral("subtitle-")+QString::fromLatin1(change)))
                << true << QString::fromLatin1(change);
        for (const auto *change : {"native-audio-unselect", "native-audio-select"})
            QTest::newRow(qPrintable(QStringLiteral("audio-")+QString::fromLatin1(change)))
                << false << QString::fromLatin1(change);
    }
    void timingContextCancellation()
    {
        QFETCH(bool, subtitle); QFETCH(QString, change);
        const bool roleChange=change==QStringLiteral("native-all-unselect")
                           || change==QStringLiteral("native-secondary-unselect");
        const bool audioSelectionChange=change==QStringLiteral("native-audio-unselect")
                                     || change==QStringLiteral("native-audio-select");
        const QDir pages(qEnvironmentVariable("VLC_TRACKS_SOURCE_DIR"));
        QVERIFY2(!qEnvironmentVariable("VLC_TRACKS_SOURCE_DIR").isEmpty(),
                 "Supply frozen production synchronization QML pages");
        QFile source(pages.filePath(subtitle ? QStringLiteral("TracksPageSubtitle.qml")
                                            : QStringLiteral("TracksPageAudio.qml")));
        QVERIFY(source.open(QIODevice::ReadOnly));
        const auto text=source.readAll();
        const auto start=text.indexOf("    Connections {");
        const auto close=text.indexOf("\n    }", start);
        const auto end=text.indexOf("    // Functions", close);
        QVERIFY(start>=0 && close>start && end>close);
        const auto connections=text.mid(start, close+6-start);
        const auto helpers=text.mid(close+6, end-close-6);
        m_controller->setRate(1.f);
        QTRY_COMPARE(m_controller->getRate(), 1.f);
        {
            vlc_player_locker lock{m_player};
            if (change == QStringLiteral("native-pause")) vlc_player_Resume(m_player);
            else vlc_player_Pause(m_player);
        }
        QTRY_COMPARE(m_controller->getPlayingState(), change == QStringLiteral("native-pause")
            ? PlayerController::PLAYING_STATE_PLAYING : PlayerController::PLAYING_STATE_PAUSED);
        m_controller->setTime(VLCTime(VLC_TICK_FROM_SEC(3)));
        QTRY_VERIFY(qAbs(m_controller->getTime().toVLCTick()-VLC_TICK_FROM_SEC(3)) <= VLC_TICK_FROM_MS(100));
        HeldEs audioId;
        auto selectedAudio = [this] {
            vlc_player_locker lock{m_player};
            const auto *track = vlc_player_GetSelectedTrack(m_player, AUDIO_ES);
            return track ? track->es_id : nullptr;
        };
        if (audioSelectionChange) {
            {
                vlc_player_locker lock{m_player};
                QVERIFY(vlc_player_GetTrackCount(m_player, AUDIO_ES) > 0);
                audioId.reset(vlc_player_GetTrackAt(m_player, AUDIO_ES, 0)->es_id);
                vlc_player_SelectEsId(m_player, audioId.get(), VLC_PLAYER_SELECT_EXCLUSIVE);
            }
            QTRY_VERIFY_WITH_TIMEOUT(selectedAudio() == audioId.get(), 5000);
            if (change == QStringLiteral("native-audio-select")) {
                vlc_player_locker lock{m_player};
                vlc_player_UnselectEsId(m_player, audioId.get());
            }
            if (change == QStringLiteral("native-audio-select"))
                QTRY_VERIFY_WITH_TIMEOUT(selectedAudio() == nullptr, 5000);
        }
        QTest::qWait(100); // drain the setup seek before marking a new pair
        DelayEstimator audio, primary, secondary;
        QSignalSpy audioChanges(&audio, &DelayEstimator::delayChanged);
        QSignalSpy primaryChanges(&primary, &DelayEstimator::delayChanged);
        QSignalSpy secondaryChanges(&secondary, &DelayEstimator::delayChanged);
        QQmlEngine engine;
        engine.rootContext()->setContextProperty(QStringLiteral("Player"), m_controller.get());
        engine.rootContext()->setContextProperty(QStringLiteral("delayEstimator"), &audio);
        engine.rootContext()->setContextProperty(QStringLiteral("delayEstimatorPrimary"), &primary);
        engine.rootContext()->setContextProperty(QStringLiteral("delayEstimatorSecondary"), &secondary);
        QQmlComponent component(&engine);
        component.setData(QByteArrayLiteral("import QtQml\nQtObject { id: root; ")
            +(subtitle ? QByteArray() : QByteArrayLiteral("property string audioTimingToken: \"\"; "))
            + QByteArrayLiteral("property QtObject hook: ")
            +connections+'\n'+helpers+'\n'+'}', QUrl(QStringLiteral("file:///timing-context-native.qml")));
        QScopedPointer<QObject> root(component.create());
        QVERIFY2(root, qPrintable(component.errorString()));
        connect(m_controller.get(), &PlayerController::rateChanged, root.data(), [this](float) {
            QCOMPARE(QThread::currentThread(), m_controller->thread());
            (void)m_controller->getInput(); // rate observers may re-lock too
        }, Qt::DirectConnection);
        connect(m_controller.get(), &PlayerController::audioSelectionChanged, root.data(), [this] {
            QCOMPARE(QThread::currentThread(), m_controller->thread());
            (void)m_controller->getInput(); // selection notification must follow the native unlock
        }, Qt::DirectConnection);
        auto marked=[&] { return subtitle ? primary.isHeardTimeMarked() || secondary.isSpottedTimeMarked()
                                          : audio.isHeardTimeMarked(); };
        if (subtitle) { primary.markHeardTime(); secondary.markSpottedTime(); }
        else {
            QVariant prepared;
            QVERIFY(QMetaObject::invokeMethod(root.data(), "prepareAudioMark", Q_RETURN_ARG(QVariant, prepared)));
            QVERIFY(prepared.toBool());
            audio.markHeardTime();
        }
        QVERIFY(marked());
        QTest::qWait(50);
        QVERIFY(marked()); // no delayed setup event may cancel this pair
        const auto input=m_controller->getInput();
        const auto selected=roles(); const auto savedCategory=category();
        const auto savedOwn=own(m_second.get());
        const auto savedAudioDelay=m_controller->getAudioDelay().toVLCTick();
        QSignalSpy seeks(m_controller.get(), &PlayerController::seekStarted);
        QSignalSpy audioSelections(m_controller.get(), &PlayerController::audioSelectionChanged);
        if (change==QStringLiteral("native-all-unselect")) {
            vlc_player_locker lock{m_player};
            vlc_player_UnselectTrackCategory(m_player, SPU_ES);
        }
        else if (change==QStringLiteral("native-secondary-unselect")) {
            vlc_player_locker lock{m_player};
            vlc_player_UnselectEsId(m_player, selected.second);
        }
        else if (audioSelectionChange) {
            vlc_player_locker lock{m_player};
            if (change == QStringLiteral("native-audio-unselect"))
                vlc_player_UnselectEsId(m_player, audioId.get());
            else
                vlc_player_SelectEsId(m_player, audioId.get(), VLC_PLAYER_SELECT_EXCLUSIVE);
        }
        else if (change==QStringLiteral("qt-seek")) m_controller->jumpToPos(.25);
        else if (change==QStringLiteral("qt-rate")) m_controller->setRate(1.5f);
        else {
            vlc_player_locker lock{m_player};
            if (change==QStringLiteral("native-seek")) vlc_player_SetTime(m_player, VLC_TICK_FROM_SEC(9));
            else if (change==QStringLiteral("native-rate")) vlc_player_ChangeRate(m_player, 1.5f);
            else if (change==QStringLiteral("native-resume")) vlc_player_Resume(m_player);
            else vlc_player_Pause(m_player);
        }
        QTRY_VERIFY_WITH_TIMEOUT(!marked(), 2500);
        if (change.endsWith(QStringLiteral("rate"))) QTRY_COMPARE(m_controller->getRate(), 1.5f);
        else if (change.endsWith(QStringLiteral("seek"))) {
            const auto requested=VLC_TICK_FROM_SEC(change==QStringLiteral("qt-seek") ? 15 : 9);
            QTRY_VERIFY(qAbs(m_controller->getTime().toVLCTick()-requested) <= VLC_TICK_FROM_MS(100));
        }
        QVERIFY(audioChanges.isEmpty() && primaryChanges.isEmpty() && secondaryChanges.isEmpty());
        if (audioSelectionChange) {
            QTRY_VERIFY(!audioSelections.isEmpty());
            QTRY_VERIFY(selectedAudio() == (change == QStringLiteral("native-audio-select") ? audioId.get() : nullptr));
            QCOMPARE(m_controller->getAudioDelay().toVLCTick(), savedAudioDelay);
        }
        QCOMPARE(m_controller->getInput(), input);
        if (change==QStringLiteral("native-all-unselect")) {
            QTRY_VERIFY(roles()==qMakePair(static_cast<vlc_es_id_t *>(nullptr), static_cast<vlc_es_id_t *>(nullptr)));
            QTRY_VERIFY(!m_controller->hasSecondarySubtitle());
        }
        else if (change==QStringLiteral("native-secondary-unselect")) {
            QTRY_VERIFY(roles()==qMakePair(selected.first, static_cast<vlc_es_id_t *>(nullptr)));
            QTRY_VERIFY(!m_controller->hasSecondarySubtitle());
        }
        else QCOMPARE(roles(), selected);
        if (roleChange || audioSelectionChange) QVERIFY(seeks.isEmpty()); // Selection alone cancels the marks.
        QCOMPARE(category(), savedCategory);
        QCOMPARE(own(m_second.get()), savedOwn);
        // The same hook/estimator objects survive the native event. Teardown
        // cannot account for cancellation and must not apply a zero override.
        QVERIFY(root);
        { vlc_player_locker lock{m_player}; vlc_player_Pause(m_player); }
        QTRY_COMPARE(m_controller->getPlayingState(), PlayerController::PLAYING_STATE_PAUSED);
        if (roleChange) {
            QCOMPARE(select(selected.first, selected.second), 2u);
            QTRY_VERIFY(roles()==selected);
            QTRY_VERIFY(m_controller->hasSecondarySubtitle());
        }
        if (audioSelectionChange && change == QStringLiteral("native-audio-unselect")) {
            { vlc_player_locker lock{m_player}; vlc_player_SelectEsId(m_player, audioId.get(), VLC_PLAYER_SELECT_EXCLUSIVE); }
            QTRY_VERIFY_WITH_TIMEOUT(selectedAudio() == audioId.get(), 5000);
        }
    }
    void audioTimingRevisionGuardsNativeChanges()
    {
        // Execute the production cancellation hook with a native NORMAL-lock
        // player. The first mark and the native setter deliberately run before
        // any queued GUI notification is delivered.
        const QDir pages(qEnvironmentVariable("VLC_TRACKS_SOURCE_DIR"));
        QVERIFY(!qEnvironmentVariable("VLC_TRACKS_SOURCE_DIR").isEmpty());
        QFile source(pages.filePath(QStringLiteral("TracksPageAudio.qml")));
        QVERIFY(source.open(QIODevice::ReadOnly));
        const auto text = source.readAll();
        const auto start = text.indexOf("    Connections {");
        const auto close = text.indexOf("\n    }", start);
        const auto end = text.indexOf("    // Functions", close);
        QVERIFY(start >= 0 && close > start && end > close);
        const auto connections = text.mid(start, close + 6 - start);
        const auto helpers = text.mid(close + 6, end - close - 6);
        HeldEs audioId;
        {
            vlc_player_locker lock{m_player};
            QVERIFY(vlc_player_GetTrackCount(m_player, AUDIO_ES) > 0);
            audioId.reset(vlc_player_GetTrackAt(m_player, AUDIO_ES, 0)->es_id);
            vlc_player_SelectEsId(m_player, audioId.get(), VLC_PLAYER_SELECT_EXCLUSIVE);
        }
        auto selectedAudio = [this] {
            vlc_player_locker lock{m_player};
            const auto *track = vlc_player_GetSelectedTrack(m_player, AUDIO_ES);
            return track ? track->es_id : nullptr;
        };
        QTRY_VERIFY_WITH_TIMEOUT(selectedAudio() == audioId.get(), 5000);
        auto audioCategory = [this] {
            vlc_player_locker lock{m_player};
            return vlc_player_GetAudioDelay(m_player);
        };
        auto setAudioCategory = [this](vlc_tick_t value) {
            vlc_player_locker lock{m_player};
            return vlc_player_SetCategoryDelay(m_player, AUDIO_ES, value, VLC_PLAYER_WHENCE_ABSOLUTE);
        };
        const auto savedCategory = audioCategory();
        const auto savedOwn = own(audioId.get());
        m_controller->setRate(1.f);
        QTRY_COMPARE(m_controller->getRate(), 1.f);
        QCOMPARE(setOwn(audioId.get(), INT64_MAX), VLC_SUCCESS);
        QCOMPARE(setAudioCategory(123000), VLC_SUCCESS);
        QTest::qWait(100); // drain setup events before the first observation

        DelayEstimator estimator;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty(QStringLiteral("Player"), m_controller.get());
        engine.rootContext()->setContextProperty(QStringLiteral("delayEstimator"), &estimator);
        QQmlComponent component(&engine);
        component.setData(QByteArrayLiteral("import QtQml\nQtObject { id: root; property string audioTimingToken: \"\"; property QtObject hook: ")
            + connections + '\n' + helpers + '\n' + '}', QUrl(QStringLiteral("file:///audio-revision-native.qml")));
        QScopedPointer<QObject> root(component.create());
        QVERIFY2(root, qPrintable(component.errorString()));
        const auto audioObserver = connect(m_controller.get(), &PlayerController::audioDelayChanged,
            this, &TestPlayerSubtitleDelay::observeAfterUnlock, Qt::DirectConnection);
        const auto ownObserver = connect(m_controller.get(), &PlayerController::audioTimingContextChanged,
            this, [this] { observeAfterUnlock(VLCDuration()); }, Qt::DirectConnection);
        QSignalSpy changes(m_controller.get(), &PlayerController::audioDelayChanged);
        auto prepare = [&] {
            QVariant result;
            return QMetaObject::invokeMethod(root.data(), "prepareAudioMark", Q_RETURN_ARG(QVariant, result))
                && result.toBool();
        };
        QVERIFY(prepare());
        estimator.markHeardTime();
        const auto first = root->property("audioTimingToken").toString();
        QVERIFY(!first.isEmpty());
        QCOMPARE(setAudioCategory(123000), VLC_SUCCESS); // no-op emits a native event
        QVERIFY(m_controller->isAudioTimingCurrent(first));
        QTRY_VERIFY(!changes.isEmpty());
        QVERIFY(estimator.isHeardTimeMarked());

        // Returning to the old numeric value must not revive the old mark.
        QCOMPARE(setAudioCategory(173000), VLC_SUCCESS);
        QCOMPARE(setAudioCategory(123000), VLC_SUCCESS);
        QVERIFY(!m_controller->isAudioTimingCurrent(first));
        QVERIFY(!m_controller->addAudioTimingIfCurrent(VLCDuration(50000), first));
        QCOMPARE(audioCategory(), vlc_tick_t(123000));
        QTRY_VERIFY(!estimator.isHeardTimeMarked());

        // The estimator's own previous write queues a notification. A new
        // mark acquired before its delivery must survive that old event.
        const auto beforeWrite = m_controller->beginAudioTiming();
        const auto previousChanges = changes.size();
        QVERIFY(m_controller->addAudioTimingIfCurrent(VLCDuration(50000), beforeWrite));
        QVERIFY(prepare());
        estimator.markHeardTime();
        const auto fresh = root->property("audioTimingToken").toString();
        QVERIFY(m_controller->isAudioTimingCurrent(fresh));
        QTRY_VERIFY(changes.size() > previousChanges);
        QVERIFY(estimator.isHeardTimeMarked());
        QVERIFY(m_controller->isAudioTimingCurrent(fresh));

        // A selected track override invalidates a mark, while its same-value
        // notification does not; guarded category changes preserve the override.
        QCOMPARE(setOwn(audioId.get(), 400000), VLC_SUCCESS);
        QVERIFY(!m_controller->isAudioTimingCurrent(fresh));
        QTRY_VERIFY(!estimator.isHeardTimeMarked());
        const auto overrideToken = m_controller->beginAudioTiming();
        QCOMPARE(setOwn(audioId.get(), 400000), VLC_SUCCESS);
        QVERIFY(m_controller->isAudioTimingCurrent(overrideToken));
        const auto previousCategory = audioCategory();
        QVERIFY(m_controller->addAudioTimingIfCurrent(VLCDuration(25000), overrideToken));
        QCOMPARE(audioCategory(), previousCategory + vlc_tick_t(25000));
        QCOMPARE(own(audioId.get()), vlc_tick_t(400000));
        QTest::qWait(100); // both direct observers re-lock after queued delivery
        disconnect(audioObserver);
        disconnect(ownObserver);
        QCOMPARE(setOwn(audioId.get(), savedOwn), VLC_SUCCESS);
        QCOMPARE(setAudioCategory(savedCategory), VLC_SUCCESS);
    }
    void audioTimingRevisionGuardsRateAndState()
    {
        const QDir pages(qEnvironmentVariable("VLC_TRACKS_SOURCE_DIR"));
        QVERIFY(!qEnvironmentVariable("VLC_TRACKS_SOURCE_DIR").isEmpty());
        QFile source(pages.filePath(QStringLiteral("TracksPageAudio.qml")));
        QVERIFY(source.open(QIODevice::ReadOnly));
        const auto text = source.readAll();
        const auto start = text.indexOf("    Connections {");
        const auto close = text.indexOf("\n    }", start);
        const auto end = text.indexOf("    // Functions", close);
        QVERIFY(start >= 0 && close > start && end > close);
        const auto connections = text.mid(start, close + 6 - start);
        const auto helpers = text.mid(close + 6, end - close - 6);

        auto requestRate = [this](float rate) {
            vlc_player_locker lock{m_player};
            vlc_player_ChangeRate(m_player, rate);
        };
        auto requestPause = [this](bool paused) {
            vlc_player_locker lock{m_player};
            if (paused) vlc_player_Pause(m_player);
            else vlc_player_Resume(m_player);
        };
        // Do not pump the Qt event queue here: wait for actual native input
        // transitions while their GUI notifications deliberately remain queued.
        auto waitRate = [this](float expected) {
            QElapsedTimer timer; timer.start();
            while (timer.elapsed() < 5000) {
                { vlc_player_locker lock{m_player};
                  if (qFuzzyCompare(vlc_player_GetRate(m_player), expected)) return true; }
                QThread::msleep(10);
            }
            return false;
        };
        auto waitState = [this](vlc_player_state expected) {
            QElapsedTimer timer; timer.start();
            while (timer.elapsed() < 5000) {
                { vlc_player_locker lock{m_player};
                  if (vlc_player_GetState(m_player) == expected) return true; }
                QThread::msleep(10);
            }
            return false;
        };
        requestRate(1.f);
        requestPause(true);
        QVERIFY(waitRate(1.f));
        QVERIFY(waitState(VLC_PLAYER_STATE_PAUSED));
        QTRY_COMPARE(m_controller->getRate(), 1.f);
        QTRY_COMPARE(m_controller->getPlayingState(), PlayerController::PLAYING_STATE_PAUSED);
        QTest::qWait(100); // setup events are not part of the experiments

        DelayEstimator estimator;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty(QStringLiteral("Player"), m_controller.get());
        engine.rootContext()->setContextProperty(QStringLiteral("delayEstimator"), &estimator);
        QQmlComponent component(&engine);
        component.setData(QByteArrayLiteral("import QtQml\nQtObject { id: root; property string audioTimingToken: \"\"; property QtObject hook: ")
            + connections + '\n' + helpers + '\n' + '}', QUrl(QStringLiteral("file:///audio-rate-state-native.qml")));
        QScopedPointer<QObject> root(component.create());
        QVERIFY2(root, qPrintable(component.errorString()));
        const auto rateObserver = connect(m_controller.get(), &PlayerController::rateChanged,
            this, [this](float) { observeAfterUnlock(VLCDuration()); }, Qt::DirectConnection);
        const auto stateObserver = connect(m_controller.get(), &PlayerController::playingStateChanged,
            this, [this](PlayerController::PlayingState) { observeAfterUnlock(VLCDuration()); }, Qt::DirectConnection);
        const auto contextObserver = connect(m_controller.get(), &PlayerController::audioTimingContextChanged,
            this, [this] { observeAfterUnlock(VLCDuration()); }, Qt::DirectConnection);
        QSignalSpy contexts(m_controller.get(), &PlayerController::audioTimingContextChanged);
        auto mark = [&] {
            QVariant result;
            if (!QMetaObject::invokeMethod(root.data(), "prepareAudioMark", Q_RETURN_ARG(QVariant, result)) || !result.toBool())
                return false;
            estimator.markHeardTime();
            return estimator.isHeardTimeMarked();
        };
        auto token = [&] { return root->property("audioTimingToken").toString(); };
        auto nativeDelay = [this] {
            vlc_player_locker lock{m_player};
            return vlc_player_GetAudioDelay(m_player);
        };
        const auto savedDelay = nativeDelay();

        QVERIFY(mark());
        const auto originalRateToken = token();
        requestRate(1.f); // same value may still emit a native callback
        QTest::qWait(100);
        QVERIFY(estimator.isHeardTimeMarked());
        QVERIFY(m_controller->isAudioTimingCurrent(originalRateToken));
        requestRate(1.5f);
        QVERIFY(waitRate(1.5f));
        requestRate(1.f);
        QVERIFY(waitRate(1.f));
        QVERIFY(!m_controller->isAudioTimingCurrent(originalRateToken));
        QVERIFY(!m_controller->addAudioTimingIfCurrent(VLCDuration(100000), originalRateToken));
        QCOMPARE(nativeDelay(), savedDelay);
        QTRY_VERIFY(!estimator.isHeardTimeMarked());

        requestRate(1.5f);
        QVERIFY(waitRate(1.5f));
        const auto previousContexts = contexts.size();
        QVERIFY(mark()); // newer than the queued rate notification
        const auto freshRateToken = token();
        QTRY_VERIFY(contexts.size() > previousContexts);
        QVERIFY(estimator.isHeardTimeMarked());
        QVERIFY(m_controller->isAudioTimingCurrent(freshRateToken));
        QVERIFY(m_controller->addAudioTimingIfCurrent(VLCDuration(100000), freshRateToken));
        QCOMPARE(nativeDelay(), savedDelay + vlc_tick_t(150000)); // native 1.5x rate
        QVERIFY(QMetaObject::invokeMethod(root.data(), "cancelAudioTiming"));
        requestRate(1.f);
        QVERIFY(waitRate(1.f));
        QTRY_COMPARE(m_controller->getRate(), 1.f);
        QTest::qWait(100);

        QVERIFY(mark());
        const auto originalStateToken = token();
        requestPause(true); // already paused: no context change
        QTest::qWait(100);
        QVERIFY(estimator.isHeardTimeMarked());
        QVERIFY(m_controller->isAudioTimingCurrent(originalStateToken));
        requestPause(false);
        QVERIFY(waitState(VLC_PLAYER_STATE_PLAYING));
        requestPause(true);
        QVERIFY(waitState(VLC_PLAYER_STATE_PAUSED));
        QVERIFY(!m_controller->isAudioTimingCurrent(originalStateToken));
        QVERIFY(!m_controller->addAudioTimingIfCurrent(VLCDuration(100000), originalStateToken));
        QTRY_VERIFY(!estimator.isHeardTimeMarked());

        requestPause(false);
        QVERIFY(waitState(VLC_PLAYER_STATE_PLAYING));
        const auto previousStateContexts = contexts.size();
        QVERIFY(mark()); // newer than the queued resume notification
        const auto freshStateToken = token();
        QTRY_VERIFY(contexts.size() > previousStateContexts);
        QVERIFY(estimator.isHeardTimeMarked());
        QVERIFY(m_controller->isAudioTimingCurrent(freshStateToken));
        QVERIFY(QMetaObject::invokeMethod(root.data(), "cancelAudioTiming"));
        requestPause(true);
        QVERIFY(waitState(VLC_PLAYER_STATE_PAUSED));
        { vlc_player_locker lock{m_player};
          QCOMPARE(vlc_player_SetCategoryDelay(m_player, AUDIO_ES, savedDelay, VLC_PLAYER_WHENCE_ABSOLUTE), VLC_SUCCESS); }
        QTest::qWait(100);
        disconnect(rateObserver);
        disconnect(stateObserver);
        disconnect(contextObserver);
    }
    void incrementalSelectionKeepsSecondaryTimingReachable()
    {
        // Follow the ordinary track list: keep one subtitle selected and add
        // another with SIMULTANEOUS, without clearing the existing selection.
        setCategory(0);
        QCOMPARE(setOwn(m_first.get(), INT64_MAX), VLC_SUCCESS);
        QCOMPARE(setOwn(m_second.get(), INT64_MAX), VLC_SUCCESS);
        QCOMPARE(select(m_first.get()), 1u);
        QTRY_VERIFY(roles() == qMakePair(m_first.get(), static_cast<vlc_es_id_t *>(nullptr)));
        QTRY_VERIFY(!m_controller->hasSecondarySubtitle());
        const auto primaryDelay=own(m_first.get());
        m_controller->setSecondarySubtitleDelay(VLCDuration(75000));
        m_controller->addSecondarySubtitleDelay(VLCDuration(75000));
        QCOMPARE(own(m_first.get()), primaryDelay);
        QCOMPARE(category(), vlc_tick_t(0));
        QSignalSpy availability(m_controller.get(), &PlayerController::secondarySubtitleSelectionChanged);
        {
            vlc_player_locker lock{m_player};
            QCOMPARE(vlc_player_SelectEsId(m_player, m_second.get(),
                     VLC_PLAYER_SELECT_SIMULTANEOUS), 2u);
            vlc_player_SetTime(m_player, VLC_TICK_FROM_MS(1500));
        }
        auto bothOutputs = [this] {
            vlc_player_locker lock{m_player};
            return vlc_player_GetEsIdVout(m_player, m_first.get(), nullptr)
                && vlc_player_GetEsIdVout(m_player, m_second.get(), nullptr);
        };
        QTRY_VERIFY_WITH_TIMEOUT(bothOutputs(), 5000);
        QTRY_VERIFY_WITH_TIMEOUT(roles() == qMakePair(m_first.get(), m_second.get()), 5000);
        QTRY_VERIFY(m_controller->hasSecondarySubtitle());
        QVERIFY(!availability.isEmpty());
        m_controller->setSecondarySubtitleDelay(VLCDuration(1200000));
        QCOMPARE(own(m_second.get()), vlc_tick_t(1200000));
        QCOMPARE(own(m_first.get()), vlc_tick_t(INT64_MAX));
        QTRY_COMPARE(m_controller->getSecondarySubtitleDelayMS(), 1200);
        setCategory(350000);
        QTRY_COMPARE(m_controller->getSubtitleDelayMS(), 350);
        QCOMPARE(own(m_second.get()), vlc_tick_t(1200000));
        QTRY_COMPARE(m_controller->getSecondarySubtitleDelayMS(), 1200);
        m_controller->setSecondarySubtitleDelay(VLCDuration(0));
        QCOMPARE(own(m_second.get()), vlc_tick_t(0));
        setCategory(450000);
        QTRY_COMPARE(m_controller->getSubtitleDelayMS(), 450);
        QTRY_COMPARE(m_controller->getSecondarySubtitleDelayMS(), 0);
        // Leave the same real pair ready for the remaining timing regressions.
        setCategory(0);
        QCOMPARE(setOwn(m_second.get(), INT64_MAX), VLC_SUCCESS);
        QTRY_COMPARE(m_controller->getSecondarySubtitleDelayMS(), 0);
    }
    void sentinelCategoryAndEffectivePayload()
    {
        QSignalSpy changes(m_controller.get(), &PlayerController::secondarySubtitleDelayChanged);
        setCategory(350000);
        QCOMPARE(setOwn(m_second.get(), 650000), VLC_SUCCESS);
        QTRY_COMPARE(m_controller->getSecondarySubtitleDelayMS(), 650);
        changes.clear();
        const int observations = m_observations;
        QCOMPARE(setOwn(m_second.get(), INT64_MAX), VLC_SUCCESS);
        QCOMPARE(m_observations, observations); // native callback stays queued
        QTRY_COMPARE(m_controller->getSecondarySubtitleDelayMS(), 350);
        QCOMPARE(own(m_second.get()), vlc_tick_t(INT64_MAX));
        QVERIFY(!changes.isEmpty());
        QCOMPARE(changes.last().front().value<VLCDuration>().toVLCTick(), vlc_tick_t(350000));
        setCategory(450000);
        QTRY_COMPARE(m_controller->getSecondarySubtitleDelayMS(), 450);
        QCOMPARE(changes.last().front().value<VLCDuration>().toVLCTick(), vlc_tick_t(450000));
        // Override/inherit transitions still notify when their effective number
        // is equal; zero is an explicit override, never the inheritance marker.
        const int before = changes.size();
        QCOMPARE(setOwn(m_second.get(), 450000), VLC_SUCCESS);
        QTRY_VERIFY(changes.size() > before);
        QCOMPARE(setOwn(m_second.get(), INT64_MAX), VLC_SUCCESS);
        QTRY_VERIFY(changes.size() > before + 1);
    }
    void relativeAdjustmentAndExactNativeUnits()
    {
        setCategory(350000);
        QCOMPARE(setOwn(m_second.get(), INT64_MAX), VLC_SUCCESS);
        QTRY_COMPARE(m_controller->getSecondarySubtitleDelayMS(), 350);
        const int observations = m_observations;
        m_controller->addSecondarySubtitleDelay(VLCDuration(50000));
        QCOMPARE(m_observations, observations);
        QCOMPARE(own(m_second.get()), vlc_tick_t(400000));
        QTRY_COMPARE(m_controller->getSecondarySubtitleDelayMS(), 400);
        m_controller->setSecondarySubtitleDelay(VLCDuration(-25555));
        QCOMPARE(own(m_second.get()), vlc_tick_t(-25555));
        QTRY_COMPARE(m_controller->getSecondarySubtitleDelay().toVLCTick(), vlc_tick_t(-25555));
    }
    void currentRoleWinsBeforeQueuedCacheRefresh()
    {
        QCOMPARE(setOwn(m_first.get(), 125000), VLC_SUCCESS);
        QCOMPARE(setOwn(m_second.get(), 650000), VLC_SUCCESS);
        QTRY_COMPARE(m_controller->getSecondarySubtitleDelayMS(), 650);
        QCOMPARE(select(m_second.get(), m_first.get()), 2u);
        QVERIFY(waitNativeRoles(m_second.get(), m_first.get()));
        // GUI callbacks have not been dispatched, so this proves the setter
        // avoids the former secondary role rather than just a fresh cache.
        QCOMPARE(m_controller->getSecondarySubtitleDelayMS(), 650);
        m_controller->setSecondarySubtitleDelay(VLCDuration(333333));
        QCOMPARE(own(m_first.get()), vlc_tick_t(333333));
        QCOMPARE(own(m_second.get()), vlc_tick_t(650000));
        QTRY_COMPARE(m_controller->getSecondarySubtitleDelay().toVLCTick(), vlc_tick_t(333333));
        QCOMPARE(own(m_second.get()), vlc_tick_t(650000)); // refresh must not reapply
    }
    void actualDefaultClearsAllPerTrackOverrides()
    {
        // Native role metadata precedes asynchronous subtitle output creation.
        // Default requires those real outputs, as in the initial fixture setup.
        QTRY_VERIFY_WITH_TIMEOUT(m_presets->canApply(), 5000);
        QVERIFY(m_presets->apply("default"));
        QTRY_VERIFY(!m_presets->restoring());
        QCOMPARE(m_presets->error(), QString());
        QCOMPARE(own(m_first.get()), vlc_tick_t(INT64_MAX));
        QCOMPARE(own(m_second.get()), vlc_tick_t(INT64_MAX));
        QCOMPARE(category(), vlc_tick_t(0));
        QTRY_COMPARE(m_controller->getSubtitleDelayMS(), 0);
        QTRY_COMPARE(m_controller->getSecondarySubtitleDelayMS(), 0);
        QCOMPARE(roles(), qMakePair(m_second.get(), m_first.get()));
    }
    void unselectAndNewRoleDoNotCarryAnOverride()
    {
        QCOMPARE(setOwn(m_first.get(), 725000), VLC_SUCCESS);
        QTRY_COMPARE(m_controller->getSecondarySubtitleDelayMS(), 725);
        QCOMPARE(select(m_second.get()), 1u);
        QTRY_VERIFY(roles() == qMakePair(m_second.get(), static_cast<vlc_es_id_t *>(nullptr)));
        QTRY_COMPARE(m_controller->getSecondarySubtitleDelayMS(), 0);
        // The track retains its genuine native override while unselected.
        // Selecting the other track as secondary must inherit its own sentinel.
        QCOMPARE(select(m_first.get(), m_second.get()), 2u);
        QTRY_VERIFY(roles() == qMakePair(m_first.get(), m_second.get()));
        QCOMPARE(own(m_first.get()), vlc_tick_t(725000));
        QCOMPARE(own(m_second.get()), vlc_tick_t(INT64_MAX));
        setCategory(275000);
        QTRY_COMPARE(m_controller->getSecondarySubtitleDelayMS(), 275);
    }
    void queuedOldMediaEventsAndDetachedInputClearCache()
    {
        QCOMPARE(setOwn(m_second.get(), 975000), VLC_SUCCESS);
        QTRY_COMPARE(m_controller->getSecondarySubtitleDelayMS(), 975);
        const auto uri = QUrl::fromLocalFile(m_fixture).toEncoded();
        auto *replacement = input_item_New(uri.constData(), "Unstarted replacement input");
        QVERIFY(replacement);
        int changed;
        {
            vlc_player_locker lock{m_player};
            // Both callbacks remain queued; the old delay must not survive the
            // native input detachment or be applied to the new input item.
            vlc_player_SetEsIdDelay(m_player, m_second.get(), 1110000, VLC_PLAYER_WHENCE_ABSOLUTE);
            vlc_player_Stop(m_player);
            changed = vlc_player_SetCurrentMedia(m_player, replacement);
        }
        input_item_Release(replacement);
        QCOMPARE(changed, VLC_SUCCESS);
        QTRY_COMPARE_WITH_TIMEOUT(m_controller->getPlayingState(), PlayerController::PLAYING_STATE_STOPPED, 10000);
        QTRY_VERIFY(!m_controller->hasSecondarySubtitle());
        QTRY_COMPARE(m_controller->getSecondarySubtitleDelayMS(), 0);
        QTRY_COMPARE(m_controller->getSubtitleDelayMS(), 0);
        QCOMPARE(roles(), qMakePair(static_cast<vlc_es_id_t *>(nullptr), static_cast<vlc_es_id_t *>(nullptr)));
        m_controller->setSecondarySubtitleDelay(VLCDuration(75000));
        m_controller->addSecondarySubtitleDelay(VLCDuration(75000));
        QCOMPARE(category(), vlc_tick_t(0));
        {
            vlc_player_locker lock{m_player};
            QCOMPARE(vlc_player_SetCurrentMedia(m_player, nullptr), VLC_SUCCESS);
        }
        QTRY_VERIFY(!m_controller->hasInput());
        QCOMPARE(m_controller->getSecondarySubtitleDelayMS(), 0);
        QVERIFY(m_observations > 0);
    }
    void attachmentScopeMappingsAndRestart()
    {
        QTemporaryDir temporary; QVERIFY(temporary.isValid());
        auto create = [&](const QString &name) {
            QFile file(temporary.filePath(name));
            if (!file.open(QIODevice::WriteOnly)) return QUrl{};
            file.write("1\n00:00:00,000 --> 00:01:00,000\nAttachment test\n"); file.close();
            return QUrl::fromLocalFile(file.fileName());
        };
        const auto a = create("Show.S01E01.mp4"), b = create("Show.S01E02.mp4");
        const auto sa = create("Show.S01E01.srt"), sb = create("Show.S01E02.srt");
        const auto playlistSub = create("Playlist choice.srt"), fileSub = create("File choice %.srt");
        QVERIFY(!a.isEmpty() && !fileSub.isEmpty());
        SubtitleAttachmentModel::Context context;
        auto *item = input_item_New(a.toEncoded().constData(), "Attachment mapping fixture"); QVERIFY(item);
        context.item.reset(item); input_item_Release(item); context.playlistId = 42;
        context.playlistItems = {QVariantMap{{"url",a},{"title","Episode one"}}, QVariantMap{{"url",b},{"title","Episode two"}}};
        QSettings settings(temporary.filePath("attachments.ini"), QSettings::IniFormat);
        const auto key = QStringLiteral("Juxtopposed/SubtitleAttachments/v1");
        auto provider = [&](bool) { return context; };
        auto model = std::make_unique<SubtitleAttachmentModel>(nullptr, &settings, provider);
        auto selectFile = [&](int row, const QUrl &url) {
            const auto files = model->subtitleRows();
            for (qsizetype i = 0; i < files.size(); ++i) if (files[i].toMap().value("url").toUrl() == url) {
                model->toggle(row, int(i)); return true;
            }
            return false;
        };
        auto encoded = [](const QUrl &url) { return url.toString(QUrl::FullyEncoded); };
        QVERIFY(model->begin()); model->setScope(SubtitleAttachmentModel::ThisFolder);
        QCOMPARE(model->mediaRows().size(), 2); model->matchNames();
        QCOMPARE(model->mediaRows()[0].toMap().value("count").toInt(), 1);
        QCOMPARE(model->mediaRows()[1].toMap().value("count").toInt(), 1);
        QVERIFY(model->save());
        QCOMPARE(model->effectiveFiles(a, 0), QStringList{encoded(sa)});
        QCOMPARE(model->effectiveFiles(b, 0), QStringList{encoded(sb)});
        QVERIFY(model->begin()); model->setScope(SubtitleAttachmentModel::ThisPlaylist);
        QVERIFY(selectFile(0, playlistSub)); QVERIFY(selectFile(1, playlistSub)); QVERIFY(model->save());
        QCOMPARE(model->effectiveFiles(a, 42), QStringList{encoded(playlistSub)});
        QCOMPARE(model->effectiveFiles(b, 42), QStringList{encoded(playlistSub)});
        QCOMPARE(model->effectiveFiles(b, 43), QStringList{encoded(sb)});
        const auto completePlaylist = context.playlistItems;
        context.playlistItems = {completePlaylist.first()};
        QVERIFY(model->begin()); model->setScope(SubtitleAttachmentModel::ThisPlaylist);
        QCOMPARE(model->mediaRows().size(), 1); model->clearRow(0); QVERIFY(selectFile(0, sa)); QVERIFY(model->save());
        QCOMPARE(model->effectiveFiles(a, 42), QStringList{encoded(sa)});
        QCOMPARE(model->effectiveFiles(b, 42), QStringList{encoded(playlistSub)}); // Unseen member retained.
        context.playlistItems = completePlaylist;
        QVERIFY(model->begin()); model->setScope(SubtitleAttachmentModel::ThisPlaylist);
        model->clearRow(0); QVERIFY(selectFile(0, playlistSub)); QVERIFY(model->save());
        QVERIFY(model->begin()); QVERIFY(selectFile(0, fileSub)); QVERIFY(selectFile(0, sa)); QVERIFY(model->save());
        QCOMPARE(model->effectiveFiles(a, 42), (QStringList{encoded(fileSub), encoded(sa)}));
        const auto saved = settings.value(key).toByteArray();
        QVERIFY(model->begin()); model->clearRow(0); model->cancel();
        QCOMPARE(settings.value(key).toByteArray(), saved);
        model.reset(); model = std::make_unique<SubtitleAttachmentModel>(nullptr, &settings, provider);
        QCOMPARE(model->effectiveFiles(a, 42), (QStringList{encoded(fileSub), encoded(sa)}));
        QCOMPARE(model->effectiveFiles(b, 42), QStringList{encoded(playlistSub)});
        QCOMPARE(model->effectiveFiles(b, 0), QStringList{encoded(sb)});
        QVERIFY(model->effectiveFiles(QUrl::fromLocalFile(temporary.filePath("Moved.S01E01.mp4")), 42).isEmpty());

        // Missing sources do not overwrite persisted matches or fall back to a
        // less-specific association. The deliberate replacement is reviewable.
        QVERIFY(QFile::rename(fileSub.toLocalFile(), temporary.filePath("Moved choice.srt")));
        QVERIFY(model->begin()); QVERIFY(!model->save()); QVERIFY(!model->error().isEmpty());
        QCOMPARE(settings.value(key).toByteArray(), saved);
        QCOMPARE(model->effectiveFiles(a, 42).first(), encoded(fileSub));
        model->clearRow(0); QVERIFY(selectFile(0, sa)); QVERIFY(model->save());
        QCOMPARE(model->effectiveFiles(a, 42), QStringList{encoded(sa)});
        QVERIFY(model->begin()); model->clearRow(0); QVERIFY(model->save());
        QCOMPARE(model->effectiveFiles(a, 42), QStringList{encoded(playlistSub)});

        // A queue-source change cannot save an old draft against the next item.
        QVERIFY(model->begin()); QVERIFY(selectFile(0, sa));
        auto *other = input_item_New(b.toEncoded().constData(), "Replacement mapping fixture"); QVERIFY(other);
        context.item.reset(other); input_item_Release(other);
        const auto beforeStale = settings.value(key).toByteArray();
        QVERIFY(!model->save()); QVERIFY(!model->canSave());
        QCOMPARE(settings.value(key).toByteArray(), beforeStale);
    }
    void attachmentAmbiguousMatchesStayUnselected()
    {
        QTemporaryDir temporary; QVERIFY(temporary.isValid());
        for (const auto &name : {"Show.S01E01.mp4","Show.S01E02.mp4","Show.S01E01.en.srt","Show.S01E01.fi.srt","Other.S01E02.srt"}) {
            QFile file(temporary.filePath(name)); QVERIFY(file.open(QIODevice::WriteOnly)); file.write("fixture");
        }
        const auto url = QUrl::fromLocalFile(temporary.filePath("Show.S01E01.mp4"));
        SubtitleAttachmentModel::Context context;
        auto *item = input_item_New(url.toEncoded().constData(), "Ambiguous matching fixture"); QVERIFY(item);
        context.item.reset(item); input_item_Release(item);
        QSettings settings(temporary.filePath("attachments.ini"), QSettings::IniFormat);
        SubtitleAttachmentModel model(nullptr, &settings, [&](bool) { return context; });
        QVERIFY(model.begin()); model.setScope(SubtitleAttachmentModel::ThisFolder); model.matchNames();
        QCOMPARE(model.mediaRows().size(), 2);
        QCOMPARE(model.mediaRows()[0].toMap().value("count").toInt(), 0);
        QCOMPARE(model.mediaRows()[1].toMap().value("count").toInt(), 1);
        QVERIFY(model.save()); QVERIFY(model.effectiveFiles(url, 0).isEmpty());
    }
    void attachmentStandardPickerFormats()
    {
        QTemporaryDir temporary; QVERIFY(temporary.isValid());
        for (const auto &name : {"Movie.3gp", "Movie.webm", "Song.flac", "Captions.MKS", "Captions.usf", "Captions.scc"}) {
            QFile file(temporary.filePath(name)); QVERIFY(file.open(QIODevice::WriteOnly)); file.write("format-discovery fixture");
        }
        const auto url = QUrl::fromLocalFile(temporary.filePath("Movie.3gp"));
        SubtitleAttachmentModel::Context context;
        auto *item = input_item_New(url.toEncoded().constData(), "Picker format fixture"); QVERIFY(item);
        context.item.reset(item); input_item_Release(item);
        QSettings settings(temporary.filePath("formats.ini"), QSettings::IniFormat);
        SubtitleAttachmentModel model(nullptr, &settings, [&](bool) { return context; });
        QVERIFY(model.begin()); QCOMPARE(model.subtitleRows().size(), 3);
        model.setScope(SubtitleAttachmentModel::ThisFolder); QCOMPARE(model.mediaRows().size(), 3);
        QTemporaryDir elsewhere; QVERIFY(elsewhere.isValid());
        const auto extra = QUrl::fromLocalFile(elsewhere.filePath("Imported.dfxp"));
        QFile file(extra.toLocalFile()); QVERIFY(file.open(QIODevice::WriteOnly)); file.write("picker fixture"); file.close();
        model.addFiles({extra}); QCOMPARE(model.subtitleRows().size(), 4);
        model.toggle(0, 3); QVERIFY(model.save());
        QCOMPARE(model.effectiveFiles(url, 0), QStringList{extra.toString(QUrl::FullyEncoded)});
        // Discovery and checked persistence only; these bytes are not a decode fixture.
    }
    void attachmentCheckedSaveFailureAndCorruptStorage()
    {
        QTemporaryDir temporary; QVERIFY(temporary.isValid());
        const auto mediaPath = temporary.filePath("media.mp4"), subtitlePath = temporary.filePath("caption.srt");
        for (const auto &path : {mediaPath, subtitlePath}) { QFile file(path); QVERIFY(file.open(QIODevice::WriteOnly)); file.write("fixture"); }
        SubtitleAttachmentModel::Context context;
        auto *item = input_item_New(QUrl::fromLocalFile(mediaPath).toEncoded().constData(), "Checked save fixture"); QVERIFY(item);
        context.item.reset(item); input_item_Release(item);
        const auto blockedPath = temporary.filePath("blocked.ini"); QVERIFY(QDir().mkpath(blockedPath));
        QSettings settings(blockedPath, QSettings::IniFormat);
        SubtitleAttachmentModel model(nullptr, &settings, [&](bool) { return context; });
        QVERIFY(model.begin()); QCOMPARE(model.subtitleRows().size(), 1); model.toggle(0, 0);
        QVERIFY(model.canSave()); QVERIFY(!model.save()); QVERIFY(model.active()); QVERIFY(!model.error().isEmpty());
        QVERIFY(model.effectiveFiles(QUrl::fromLocalFile(mediaPath), 0).isEmpty());
        QVERIFY(QDir().rmdir(blockedPath));
        QVERIFY(model.save()); // Fresh checked settings session recovers the old access error.
        QVERIFY(!model.effectiveFiles(QUrl::fromLocalFile(mediaPath), 0).isEmpty());
        const auto key = QStringLiteral("Juxtopposed/SubtitleAttachments/v1");
        QSettings corrupt(temporary.filePath("corrupt.ini"), QSettings::IniFormat);
        const QByteArray bytes("{\"schema\":1,\"records\":[{\"scope\":2,\"owner\":\"wrong\",\"files\":{}}]}");
        corrupt.setValue(key, bytes); corrupt.sync();
        SubtitleAttachmentModel invalid(nullptr, &corrupt, [&](bool) { return context; });
        QVERIFY(invalid.begin()); invalid.toggle(0, 0);
        QVERIFY(!invalid.editable()); QVERIFY(!invalid.error().isEmpty());
        QVERIFY(!invalid.canSave()); QVERIFY(!invalid.save());
        QCOMPARE(corrupt.value(key).toByteArray(), bytes);
    }
    void attachmentNativePausedAndFreshInputRestore()
    {
        QTemporaryDir temporary; QVERIFY(temporary.isValid());
        const auto mediaPath = temporary.filePath("Attachment media %.mp4");
        QVERIFY(QFile::copy(m_fixture, mediaPath));
        const auto firstPath = temporary.filePath("First Ã¤ %.srt"), secondPath = temporary.filePath("Second.srt");
        for (const auto &path : {firstPath, secondPath}) {
            QFile file(path); QVERIFY(file.open(QIODevice::WriteOnly));
            file.write("1\n00:00:00,000 --> 00:01:00,000\nEXTERNAL ATTACHMENT\n");
        }
        QSettings settings(temporary.filePath("native.ini"), QSettings::IniFormat);
        auto model = std::make_unique<SubtitleAttachmentModel>(m_player, &settings);
        auto openFresh = [&] {
            auto *item = input_item_New(QUrl::fromLocalFile(mediaPath).toEncoded().constData(), "Native attachment fixture");
            if (!item) return false;
            vlc_player_locker lock{m_player};
            const auto changed = vlc_player_SetCurrentMedia(m_player, item); input_item_Release(item);
            return changed == VLC_SUCCESS && vlc_player_Start(m_player) == VLC_SUCCESS;
        };
        auto stop = [&] {
            { vlc_player_locker lock{m_player}; vlc_player_Stop(m_player); }
            QElapsedTimer deadline; deadline.start();
            while (deadline.elapsed() < 10000) {
                bool stopped; { vlc_player_locker lock{m_player}; stopped = vlc_player_GetState(m_player) == VLC_PLAYER_STATE_STOPPED; }
                if (stopped) return true;
                QTest::qWait(20);
            }
            return false;
        };
        auto count = [&] { vlc_player_locker lock{m_player}; return vlc_player_GetTrackCount(m_player, SPU_ES); };
        auto defaultSelected = [&] {
            const auto uri = QUrl::fromLocalFile(firstPath).toEncoded();
            const QByteArray prefix = QCryptographicHash::hash(uri, QCryptographicHash::Md5).toHex() + '/';
            vlc_player_locker lock{m_player};
            for (size_t i = 0; i < vlc_player_GetTrackCount(m_player, SPU_ES); ++i) {
                const auto *track = vlc_player_GetTrackAt(m_player, SPU_ES, i);
                if (track->selected && QByteArray(vlc_es_id_GetStrId(track->es_id)).startsWith(prefix)) return true;
            }
            return false;
        };
        QVERIFY(stop()); QVERIFY(openFresh());
        QTRY_COMPARE_WITH_TIMEOUT(m_controller->getPlayingState(), PlayerController::PLAYING_STATE_PLAYING, 10000);
        QTRY_COMPARE_WITH_TIMEOUT(count(), size_t(2), 10000);
        { vlc_player_locker lock{m_player}; vlc_player_Pause(m_player); vlc_player_SetTime(m_player, VLC_TICK_FROM_SEC(3)); }
        QTRY_COMPARE(m_controller->getPlayingState(), PlayerController::PLAYING_STATE_PAUSED);
        QTest::qWait(200);
        qint64 before; size_t audioCount;
        { vlc_player_locker lock{m_player}; before = vlc_player_GetTime(m_player); audioCount = vlc_player_GetTrackCount(m_player, AUDIO_ES); }
        unsigned heldMarker = 0;
        if (qEnvironmentVariableIsSet("VLC_FPS_SNAPSHOTS")) {
            const auto image = captureNativeFrame("attachment-held-before");
            QVERIFY(!image.isNull());
            heldMarker = frameMarker(image);
            vlc_player_locker lock{m_player};
            qInfo("Attachment paused baseline=%lld afterSnapshot=%lld marker=%u",
                  static_cast<long long>(before), static_cast<long long>(vlc_player_GetTime(m_player)), frameMarker(image));
        }
        QVERIFY(model->begin());
        auto files = model->subtitleRows(); QCOMPARE(files.size(), 2);
        for (int i = 0; i < files.size(); ++i) model->toggle(0, i);
        QVERIFY(model->save());
        QTRY_COMPARE_WITH_TIMEOUT(count(), size_t(4), 10000);
        QTRY_VERIFY_WITH_TIMEOUT(defaultSelected(), 5000);
        if (qEnvironmentVariableIsSet("VLC_FPS_SNAPSHOTS")) {
            const auto image = captureNativeFrame("attachment-held-after");
            QVERIFY(!image.isNull());
            // Adding a caption changes the D3D snapshot blending/conversion
            // path. The fixture's encoded frame ID survives that color rounding.
            QCOMPARE(frameMarker(image), heldMarker);
            vlc_player_locker lock{m_player};
            qInfo("Attachment paused final=%lld delta=%lld marker=%u",
                  static_cast<long long>(vlc_player_GetTime(m_player)),
                  static_cast<long long>(vlc_player_GetTime(m_player) - before), frameMarker(image));
        }
        { vlc_player_locker lock{m_player};
          QCOMPARE(vlc_player_GetState(m_player), VLC_PLAYER_STATE_PAUSED);
          QVERIFY(qAbs(vlc_player_GetTime(m_player) - before) < VLC_TICK_FROM_MS(100));
          QCOMPARE(vlc_player_GetTrackCount(m_player, AUDIO_ES), audioCount); }
        model->refresh(); model->refresh(); QTest::qWait(150); QCOMPARE(count(), size_t(4));
        const auto saved = settings.value("Juxtopposed/SubtitleAttachments/v1").toByteArray();
        QVERIFY(stop()); model.reset();
        model = std::make_unique<SubtitleAttachmentModel>(m_player, &settings);
        QVERIFY(openFresh());
        QTRY_COMPARE_WITH_TIMEOUT(count(), size_t(4), 10000);
        QTRY_VERIFY_WITH_TIMEOUT(defaultSelected(), 5000);
        QCOMPARE(settings.value("Juxtopposed/SubtitleAttachments/v1").toByteArray(), saved);
        QVERIFY(stop()); QVERIFY(QFile::rename(firstPath, firstPath + ".moved.srt"));
        QVERIFY(openFresh());
        QTRY_COMPARE_WITH_TIMEOUT(m_controller->getPlayingState(), PlayerController::PLAYING_STATE_PLAYING, 10000);
        QTRY_VERIFY_WITH_TIMEOUT(!model->error().isEmpty(), 5000);
        QCOMPARE(count(), size_t(2)); // No partial attach or lower-priority fallback.
        QCOMPARE(settings.value("Juxtopposed/SubtitleAttachments/v1").toByteArray(), saved);
        QVERIFY(stop()); model.reset();
        { vlc_player_locker lock{m_player}; QCOMPARE(vlc_player_SetCurrentMedia(m_player, nullptr), VLC_SUCCESS); }
    }
    void attachmentDecodeFailureAndReplacement()
    {
        QTemporaryDir temporary; QVERIFY(temporary.isValid());
        const auto mediaPath = temporary.filePath("Decode recovery.mp4");
        QVERIFY(QFile::copy(m_fixture, mediaPath));
        const auto badPath = temporary.filePath("Broken.srt");
        const auto goodPath = temporary.filePath("Replacement.srt");
        {
            QFile bad(badPath); QVERIFY(bad.open(QIODevice::WriteOnly));
            bad.write("This readable file contains no subtitle records.\n");
            QFile good(goodPath); QVERIFY(good.open(QIODevice::WriteOnly));
            good.write("1\n00:00:00,000 --> 00:01:00,000\nRECOVERED SUBTITLE\n");
        }
        { vlc_player_locker lock{m_player}; vlc_player_Stop(m_player); }
        QTRY_VERIFY_WITH_TIMEOUT(([&] { vlc_player_locker lock{m_player};
            return vlc_player_GetState(m_player) == VLC_PLAYER_STATE_STOPPED; })(), 10000);
        QSettings settings(temporary.filePath("decode.ini"), QSettings::IniFormat);
        SubtitleAttachmentModel model(m_player, &settings);
        QSignalSpy failures(&model, &SubtitleAttachmentModel::loadFailed);
        connect(&model, &SubtitleAttachmentModel::loadFailed, this, [&](const QString &) {
            // A direct UI observer must be able to query the NORMAL-lock player.
            vlc_player_locker lock{m_player};
            QCOMPARE(vlc_player_GetState(m_player), VLC_PLAYER_STATE_PAUSED);
        }, Qt::DirectConnection);
        {
            auto *item = input_item_New(QUrl::fromLocalFile(mediaPath).toEncoded().constData(), "Decode recovery fixture");
            QVERIFY(item);
            vlc_player_locker lock{m_player};
            const auto result = vlc_player_SetCurrentMedia(m_player, item); input_item_Release(item);
            QCOMPARE(result, VLC_SUCCESS); QCOMPARE(vlc_player_Start(m_player), VLC_SUCCESS);
        }
        auto count = [&] { vlc_player_locker lock{m_player}; return vlc_player_GetTrackCount(m_player, SPU_ES); };
        QTRY_COMPARE_WITH_TIMEOUT(count(), size_t(2), 10000);
        { vlc_player_locker lock{m_player}; vlc_player_SetTime(m_player, VLC_TICK_FROM_SEC(3)); vlc_player_Pause(m_player); }
        QTRY_COMPARE(m_controller->getPlayingState(), PlayerController::PLAYING_STATE_PAUSED);
        QTest::qWait(200);
        vlc_tick_t pausedTime; size_t audioCount;
        { vlc_player_locker lock{m_player}; pausedTime = vlc_player_GetTime(m_player);
          audioCount = vlc_player_GetTrackCount(m_player, AUDIO_ES); }
        unsigned heldMarker = 0;
        if (qEnvironmentVariableIsSet("VLC_FPS_SNAPSHOTS")) {
            const auto image = captureNativeFrame("decode-held-before");
            QVERIFY(!image.isNull());
            heldMarker = frameMarker(image);
            vlc_player_locker lock{m_player};
            qInfo("Decode paused baseline=%lld afterSnapshot=%lld marker=%u",
                  static_cast<long long>(pausedTime), static_cast<long long>(vlc_player_GetTime(m_player)), frameMarker(image));
        }
        auto index = [&](const QString &path) {
            const auto rows = model.subtitleRows();
            for (int i = 0; i < rows.size(); ++i)
                if (rows[i].toMap().value("url").toUrl() == QUrl::fromLocalFile(path)) return i;
            return -1;
        };
        QVERIFY(model.begin()); QVERIFY(index(badPath) >= 0);
        model.toggle(0, index(badPath)); QVERIFY(model.save());
        const auto key = QStringLiteral("Juxtopposed/SubtitleAttachments/v1");
        const auto saved = settings.value(key).toByteArray(); QVERIFY(!saved.isEmpty());
        // Submission success cannot establish that the asynchronous demuxer
        // produced a track. The paused player must receive useful failure state.
        QTRY_VERIFY_WITH_TIMEOUT(!model.error().isEmpty(), 10000);
        QCOMPARE(failures.count(), 1);
        QCOMPARE(count(), size_t(2)); QCOMPARE(settings.value(key).toByteArray(), saved);
        const auto error = model.error();
        QVERIFY(model.begin()); QCOMPARE(model.error(), error);
        model.clearRow(0); QVERIFY(index(goodPath) >= 0);
        model.toggle(0, index(goodPath)); QVERIFY(model.save());
        QTRY_COMPARE_WITH_TIMEOUT(count(), size_t(3), 10000);
        const QByteArray prefix = QCryptographicHash::hash(QUrl::fromLocalFile(goodPath).toEncoded(), QCryptographicHash::Md5).toHex() + '/';
        QTRY_VERIFY_WITH_TIMEOUT(([&] {
            vlc_player_locker lock{m_player};
            for (size_t i = 0; i < vlc_player_GetTrackCount(m_player, SPU_ES); ++i) {
                const auto *track = vlc_player_GetTrackAt(m_player, SPU_ES, i);
                if (track->selected && QByteArray(vlc_es_id_GetStrId(track->es_id)).startsWith(prefix)) return true;
            }
            return false;
        })(), 5000);
        QVERIFY(model.error().isEmpty());
        // Supersede an unresolved request before its observation expires.
        QVERIFY(model.begin()); model.clearRow(0); model.toggle(0, index(badPath)); QVERIFY(model.save());
        QVERIFY(model.begin()); model.clearRow(0); model.toggle(0, index(goodPath)); QVERIFY(model.save());
        QTest::qWait(8500); // A superseded timer must not replace the success.
        QVERIFY(model.error().isEmpty()); QCOMPARE(count(), size_t(3));
        QCOMPARE(failures.count(), 1);
        if (qEnvironmentVariableIsSet("VLC_FPS_SNAPSHOTS")) {
            const auto image = captureNativeFrame("decode-held-after");
            QVERIFY(!image.isNull());
            QCOMPARE(frameMarker(image), heldMarker);
            vlc_player_locker lock{m_player};
            qInfo("Decode paused final=%lld delta=%lld marker=%u",
                  static_cast<long long>(vlc_player_GetTime(m_player)),
                  static_cast<long long>(vlc_player_GetTime(m_player) - pausedTime), frameMarker(image));
        }
        { vlc_player_locker lock{m_player};
          QCOMPARE(vlc_player_GetState(m_player), VLC_PLAYER_STATE_PAUSED);
          QVERIFY(qAbs(vlc_player_GetTime(m_player) - pausedTime) < VLC_TICK_FROM_MS(100));
          QCOMPARE(vlc_player_GetTrackCount(m_player, AUDIO_ES), audioCount);
          vlc_player_Stop(m_player); }
        QTRY_VERIFY_WITH_TIMEOUT(([&] { vlc_player_locker lock{m_player};
            return vlc_player_GetState(m_player) == VLC_PLAYER_STATE_STOPPED; })(), 10000);
        { vlc_player_locker lock{m_player}; QCOMPARE(vlc_player_SetCurrentMedia(m_player, nullptr), VLC_SUCCESS); }
    }
    void attachmentAndPresetRestore_data()
    {
        QTest::addColumn<bool>("attachmentsFirst");
        QTest::addColumn<bool>("lateCue");
        QTest::addColumn<bool>("resetFpsBeforeSeek");
        QTest::newRow("production-preset-first") << false << false << false;
        QTest::newRow("attachment-first") << true << false << false;
        QTest::newRow("preset-first-before-cue") << false << true << false;
        QTest::newRow("attachment-first-before-cue") << true << true << false;
        QTest::newRow("preset-first-before-cue-fps-reset") << false << true << true;
        QTest::newRow("attachment-first-before-cue-fps-reset") << true << true << true;
    }
    void attachmentAndPresetRestore()
    {
        QFETCH(bool, attachmentsFirst);
        QFETCH(bool, lateCue);
        QFETCH(bool, resetFpsBeforeSeek);
        auto resetPausedStart = [this](vlc_player_t *) {
            vlc_player_locker lock{m_player}; vlc_player_SetStartPaused(m_player, false);
        };
        std::unique_ptr<vlc_player_t, decltype(resetPausedStart)> pausedStartOwner(m_player, resetPausedStart);
        struct AttachmentTrace {
            QElapsedTimer timer;
            std::atomic_bool restoring{false};
            static void snapshot(vlc_player_t *player, void *opaque, const char *event) {
                auto *trace = static_cast<AttachmentTrace *>(opaque);
                qInfo("ATTACHMENT_TRACE %lld restore=%d %s", trace->timer.elapsed(), int(trace->restoring.load()), event);
                for (size_t i = 0; i < vlc_player_GetTrackCount(player, SPU_ES); ++i) {
                    const auto *track = vlc_player_GetTrackAt(player, SPU_ES, i);
                    vlc_vout_order order = VLC_VOUT_ORDER_NONE;
                    const auto output = vlc_player_GetEsIdVout(player, track->es_id, &order);
                    qInfo("ATTACHMENT_TRACK %s selected=%d output=%d order=%d", vlc_es_id_GetStrId(track->es_id),
                          int(track->selected), int(output != nullptr), int(order));
                }
            }
        } trace;
        trace.timer.start();
        vlc_player_listener_id *traceListener = nullptr;
        const bool tracing = qEnvironmentVariableIsSet("VLC_ATTACHMENT_TRACE");
        if (tracing) {
            static const vlc_player_cbs callbacks = [] {
                vlc_player_cbs c{};
                c.on_track_selection_changed = [](vlc_player_t *player, vlc_es_id_t *oldId, vlc_es_id_t *newId, void *data) {
                    qInfo("ATTACHMENT_SELECTION old=%s new=%s", oldId ? vlc_es_id_GetStrId(oldId) : "none", newId ? vlc_es_id_GetStrId(newId) : "none");
                    AttachmentTrace::snapshot(player, data, "selection");
                };
                c.on_track_list_changed = [](vlc_player_t *player, vlc_player_list_action, const vlc_player_track *track, void *data) {
                    if (track->fmt.i_cat == SPU_ES) AttachmentTrace::snapshot(player, data, "tracks");
                };
                c.on_vout_changed = [](vlc_player_t *player, vlc_player_vout_action, vout_thread_t *, vlc_vout_order, vlc_es_id_t *, void *data) {
                    AttachmentTrace::snapshot(player, data, "output");
                };
                return c;
            }();
            vlc_player_locker lock{m_player};
            traceListener = vlc_player_AddListener(m_player, &callbacks, &trace);
            QVERIFY(traceListener);
        }
        auto removeTrace = [this](vlc_player_listener_id *listener) {
            vlc_player_locker lock{m_player}; vlc_player_RemoveListener(m_player, listener);
        };
        std::unique_ptr<vlc_player_listener_id, decltype(removeTrace)> traceOwner(traceListener, removeTrace);
        QTemporaryDir temporary; QVERIFY(temporary.isValid());
        const auto mediaPath = temporary.filePath("Preset attachment.mp4");
        const auto nextPath = temporary.filePath("Next attachment.mp4");
        QVERIFY(QFile::copy(m_fixture, mediaPath));
        QVERIFY(QFile::copy(m_fixture, nextPath));
        const auto first = temporary.filePath("First.en.srt"), second = temporary.filePath("Second.fi.srt");
        for (const auto &path : {first, second}) {
            QFile file(path); QVERIFY(file.open(QIODevice::WriteOnly));
            file.write(lateCue ? "1\n00:00:20,000 --> 00:01:00,000\nPRESET ATTACHMENT\n"
                               : "1\n00:00:00,000 --> 00:01:00,000\nPRESET ATTACHMENT\n");
        }
        auto stop = [&] {
            { vlc_player_locker lock{m_player}; vlc_player_Stop(m_player); }
            QElapsedTimer timer; timer.start();
            while (timer.elapsed() < 10000) {
                bool stopped;
                { vlc_player_locker lock{m_player}; stopped = vlc_player_GetState(m_player) == VLC_PLAYER_STATE_STOPPED; }
                if (stopped) return true;
                QTest::qWait(20);
            }
            return false;
        };
        auto openFresh = [&](const QString &path) {
            auto *item = input_item_New(QUrl::fromLocalFile(path).toEncoded().constData(), "Preset attachment fixture");
            if (!item) return false;
            vlc_player_locker lock{m_player};
            const auto result = vlc_player_SetCurrentMedia(m_player, item); input_item_Release(item);
            return result == VLC_SUCCESS && vlc_player_Start(m_player) == VLC_SUCCESS;
        };
        auto prefixFor = [](const QString &path) -> QByteArray {
            return QCryptographicHash::hash(QUrl::fromLocalFile(path).toEncoded(), QCryptographicHash::Md5).toHex() + '/';
        };
        auto selected = [&](const QString &path) {
            const auto prefix = prefixFor(path);
            vlc_player_locker lock{m_player};
            for (size_t i = 0; i < vlc_player_GetTrackCount(m_player, SPU_ES); ++i) {
                const auto *track = vlc_player_GetTrackAt(m_player, SPU_ES, i);
                if (track->selected && QByteArray(vlc_es_id_GetStrId(track->es_id)).startsWith(prefix)) return true;
            }
            return false;
        };
        auto count = [&] { vlc_player_locker lock{m_player}; return vlc_player_GetTrackCount(m_player, SPU_ES); };
        QVERIFY(stop());
        {
            SubtitleAttachmentModel::Context future;
            auto *item = input_item_New(QUrl::fromLocalFile(nextPath).toEncoded().constData(), "Next attachment fixture");
            QVERIFY(item); future.item.reset(item); input_item_Release(item);
            SubtitleAttachmentModel seed(nullptr, m_settings.get(), [&](bool) { return future; });
            QVERIFY(seed.begin()); QCOMPARE(seed.subtitleRows().size(), 2); seed.toggle(0, 0); QVERIFY(seed.save());
        }
        auto makeAttachments = [&] {
            return std::make_unique<SubtitleAttachmentModel>(m_player, m_settings.get(),
                SubtitleAttachmentModel::ContextProvider{}, [this](input_item_t *item) {
                    return !m_presets || !m_presets->hasTrackSelectionPolicy(item);
                });
        };
        auto attachments = makeAttachments();
        QVERIFY(openFresh(mediaPath));
        QTRY_COMPARE_WITH_TIMEOUT(count(), size_t(2), 10000);
        { vlc_player_locker lock{m_player}; vlc_player_SetTime(m_player, VLC_TICK_FROM_SEC(3)); vlc_player_Pause(m_player); }
        QTRY_COMPARE(m_controller->getPlayingState(), PlayerController::PLAYING_STATE_PAUSED);
        QVERIFY(attachments->begin()); QCOMPARE(attachments->subtitleRows().size(), 2);
        attachments->toggle(0, 0); attachments->toggle(0, 1); QVERIFY(attachments->save());
        QTRY_COMPARE_WITH_TIMEOUT(count(), size_t(4), 10000);
        QTRY_VERIFY_WITH_TIMEOUT(selected(first), 5000);
        // Finish any Default restore left by the preceding media before this
        // fixture deliberately edits tracks/timing and captures a named preset.
        QTRY_VERIFY_WITH_TIMEOUT(m_presets->canSave(), 5000);
        {
            const auto prefix = prefixFor(second);
            vlc_player_locker lock{m_player};
            bool found = false;
            for (size_t i = 0; i < vlc_player_GetTrackCount(m_player, SPU_ES); ++i) {
                const auto *track = vlc_player_GetTrackAt(m_player, SPU_ES, i);
                if (!QByteArray(vlc_es_id_GetStrId(track->es_id)).startsWith(prefix)) continue;
                vlc_player_SelectTrack(m_player, track, VLC_PLAYER_SELECT_EXCLUSIVE);
                QCOMPARE(vlc_player_SetEsIdDelay(m_player, track->es_id, 175000, VLC_PLAYER_WHENCE_ABSOLUTE), VLC_SUCCESS);
                found = true;
            }
            QVERIFY(found);
            QCOMPARE(vlc_player_SetCategoryDelay(m_player, SPU_ES, 250000, VLC_PLAYER_WHENCE_ABSOLUTE), VLC_SUCCESS);
            vlc_player_SetAssociatedSubsFPS(m_player, 30.f);
        }
        QTRY_VERIFY_WITH_TIMEOUT(selected(second), 5000);
        QTRY_VERIFY_WITH_TIMEOUT(m_presets->canSave(), 5000);
        QCOMPARE(category(), vlc_tick_t(250000));
        { vlc_player_locker lock{m_player}; QCOMPARE(vlc_player_GetAssociatedSubsFPS(m_player), 30.f); }
        QVERIFY2(m_presets->save(QStringLiteral("Attachment restore %1 %2 %3").arg(attachmentsFirst).arg(lateCue).arg(resetFpsBeforeSeek)),
                 qPrintable(m_presets->error()));
        const auto savedId = m_presets->selectedId();
        if (tracing) qInfo("ATTACHMENT_SAVED %s", m_settings->value("Juxtopposed/SubtitlePresets/v1").toByteArray().constData());
        QVERIFY(stop()); attachments.reset(); m_presets.reset();
        { vlc_player_locker lock{m_player}; QCOMPARE(vlc_player_SetCurrentMedia(m_player, nullptr), VLC_SUCCESS); }
        auto makePresets = [&] {
            m_presets = std::make_unique<SubtitlePresetModel>(m_player, m_controller->getSubtitleAppearance(),
                m_settings.get(), VLC_OBJECT(m_environment->intf));
            connect(m_presets.get(), &SubtitlePresetModel::stateChanged, m_presets.get(), [this] {
                if (!m_presets->error().isEmpty()) qInfo("Preset restore error: %s", qPrintable(m_presets->error()));
            });
        };
        if (attachmentsFirst) {
            attachments = makeAttachments(); makePresets();
        } else {
            makePresets(); attachments = makeAttachments();
        }
        trace.restoring = true;
        { vlc_player_locker lock{m_player}; vlc_player_SetStartPaused(m_player, true); }
        QVERIFY(openFresh(mediaPath));
        QTRY_COMPARE_WITH_TIMEOUT(count(), size_t(4), 10000);
        QTRY_COMPARE(m_controller->getPlayingState(), PlayerController::PLAYING_STATE_PAUSED);
        QTRY_VERIFY2_WITH_TIMEOUT(m_presets->selectedId() == savedId,
            qPrintable(m_presets->error() + ' ' + m_presets->statusText()), 6000);
        QTRY_VERIFY_WITH_TIMEOUT(!m_presets->restoring() && selected(second), 6000);
        QTest::qWait(500);
        QVERIFY(selected(second)); QVERIFY(!selected(first));
        QVERIFY(!m_presets->edited()); QCOMPARE(m_presets->error(), QString());
        if (resetFpsBeforeSeek) {
            m_controller->setSubtitleFPS(0.f);
            QTRY_COMPARE(m_controller->getSubtitleFPS(), 0.f);
            QTest::qWait(500);
        }
        // Restoration must finish before the first cue without advancing the
        // paused input. A later cue then establishes the actual renderer role.
        { vlc_player_locker lock{m_player};
          QVERIFY(vlc_player_GetTime(m_player) < VLC_TICK_FROM_MS(200));
          vlc_player_SetTime(m_player, VLC_TICK_FROM_SEC(25)); }
        QTRY_VERIFY_WITH_TIMEOUT(([&] {
            const auto prefix = prefixFor(second);
            vlc_player_locker lock{m_player};
            if (qAbs(vlc_player_GetTime(m_player) - VLC_TICK_FROM_SEC(25)) > VLC_TICK_FROM_MS(100)) return false;
            for (size_t i = 0; i < vlc_player_GetTrackCount(m_player, SPU_ES); ++i) {
                const auto *track = vlc_player_GetTrackAt(m_player, SPU_ES, i);
                if (!QByteArray(vlc_es_id_GetStrId(track->es_id)).startsWith(prefix)) continue;
                vlc_vout_order order = VLC_VOUT_ORDER_NONE;
                return track->selected && vlc_player_GetEsIdVout(m_player, track->es_id, &order)
                    && order == VLC_VOUT_ORDER_PRIMARY;
            }
            return false;
        })(), 5000);
        QTRY_COMPARE_WITH_TIMEOUT(m_presets->edited(), resetFpsBeforeSeek, 5000);
        QCOMPARE(m_presets->selectedId(), savedId); QVERIFY(m_presets->error().isEmpty());
        if (lateCue) {
            QTest::qWait(700);
            const auto image = captureNativeFrame(QString("preset-late-%1-%2").arg(attachmentsFirst).arg(resetFpsBeforeSeek));
            QVERIFY(!image.isNull());
            unsigned bright=0;
            for (int y=image.height()*3/4; y<image.height(); ++y)
                for (int x=0; x<image.width(); ++x) {
                    const auto color=image.pixelColor(x,y);
                    bright += color.red()>160 && color.green()>160 && color.blue()>160;
                }
            qInfo("Preset late cue reset=%d caption pixels=%u", int(resetFpsBeforeSeek), bright);
            QVERIFY2(bright>100, "Restored late subtitles must render after retaining/resetting FPS before the first cue");
        }
        {
            vlc_player_locker lock{m_player};
            QCOMPARE(vlc_player_GetSubtitleDelay(m_player), vlc_tick_t(250000));
            QCOMPARE(vlc_player_GetAssociatedSubsFPS(m_player), resetFpsBeforeSeek ? 0.f : 30.f);
            const auto prefix = prefixFor(second);
            for (size_t i = 0; i < vlc_player_GetTrackCount(m_player, SPU_ES); ++i) {
                const auto *track = vlc_player_GetTrackAt(m_player, SPU_ES, i);
                if (QByteArray(vlc_es_id_GetStrId(track->es_id)).startsWith(prefix))
                    QCOMPARE(vlc_player_GetEsIdDelay(m_player, track->es_id), vlc_tick_t(175000));
            }
        }
        // A-B transition: A's named preset must not suppress B's attachment
        // default, even when attachment callbacks are registered first.
        { vlc_player_locker lock{m_player}; vlc_player_SetStartPaused(m_player, false); }
        QVERIFY(openFresh(nextPath));
        // Keep Qt callbacks queued while B's native input becomes ready, then
        // exercise the attachment refresh against A's still-cached preset.
        bool nextReady = false;
        QElapsedTimer nextDeadline; nextDeadline.start();
        while (nextDeadline.elapsed() < 10000) {
            {
                vlc_player_locker lock{m_player};
                auto *current = vlc_player_GetCurrentMedia(m_player);
                char *uri = current ? input_item_GetURI(current) : nullptr;
                nextReady = uri && QByteArray(uri) == QUrl::fromLocalFile(nextPath).toEncoded()
                    && vlc_player_GetState(m_player) == VLC_PLAYER_STATE_PLAYING
                    && vlc_player_GetTrackCount(m_player, SPU_ES) == 2;
                free(uri);
            }
            if (nextReady) break;
            QThread::msleep(10);
        }
        QVERIFY(nextReady); QCOMPARE(m_presets->selectedId(), savedId);
        attachments->refresh();
        QTRY_COMPARE_WITH_TIMEOUT(count(), size_t(3), 10000);
        QTRY_VERIFY_WITH_TIMEOUT(selected(first), 5000);
        QTRY_VERIFY_WITH_TIMEOUT(!m_presets->restoring(), 5000);
        QCOMPARE(m_presets->selectedId(), QStringLiteral("default"));
        QVERIFY(stop()); attachments.reset();
        { vlc_player_locker lock{m_player}; QCOMPARE(vlc_player_SetCurrentMedia(m_player, nullptr), VLC_SUCCESS); }
    }
    void attachmentColdCaptionAtOrigin_data()
    {
        QTest::addColumn<bool>("attachmentsFirst");
        QTest::newRow("presets-first") << false;
        QTest::newRow("attachments-first") << true;
    }
    void attachmentColdCaptionAtOrigin()
    {
        QFETCH(bool, attachmentsFirst);
        const auto stop = [&] {
            { vlc_player_locker lock{m_player}; vlc_player_Stop(m_player); }
            QElapsedTimer deadline; deadline.start();
            while (deadline.elapsed() < 10000) {
                bool stopped;
                { vlc_player_locker lock{m_player}; stopped = vlc_player_GetState(m_player) == VLC_PLAYER_STATE_STOPPED; }
                if (stopped) return true;
                QTest::qWait(20);
            }
            return false;
        };
        auto restoreStart = [this](vlc_player_t *) {
            vlc_player_locker lock{m_player}; vlc_player_SetStartPaused(m_player, false);
        };
        std::unique_ptr<vlc_player_t, decltype(restoreStart)> pausedOwner(m_player, restoreStart);
        m_presets.reset();
        QVERIFY(stop());
        { vlc_player_locker lock{m_player};
          QCOMPARE(vlc_player_SetCurrentMedia(m_player, nullptr), VLC_SUCCESS);
          vlc_player_SetAssociatedSubsFPS(m_player, 0.f);
          vlc_player_SetStartPaused(m_player, true); }
        m_settings->clear();
        QTemporaryDir temporary; QVERIFY(temporary.isValid());
        const int requestedCycles = qEnvironmentVariableIntValue("VLC_COLD_START_CYCLES");
        const int cycles = requestedCycles > 0 ? qBound(1, requestedCycles, 6) : 3;
        for (int cycle = 0; cycle < cycles; ++cycle) {
            const auto folder = temporary.filePath(QString::number(cycle));
            QVERIFY(QDir().mkpath(folder));
            const auto mediaPath = QDir(folder).filePath("Cold origin.mp4");
            const auto subtitlePath = QDir(folder).filePath("Cold origin.srt");
            QVERIFY(QFile::copy(m_fixture, mediaPath));
            { QFile file(subtitlePath); QVERIFY(file.open(QIODevice::WriteOnly));
              QVERIFY(file.write("1\n00:00:00,000 --> 00:00:02,000\nORIGIN CAPTION\n") > 0); }
            auto *item = input_item_New(QUrl::fromLocalFile(mediaPath).toEncoded().constData(), "Cold origin fixture");
            QVERIFY(item);
            SubtitleAttachmentModel::Context future; future.item.reset(item); input_item_Release(item);
            { SubtitleAttachmentModel seed(nullptr, m_settings.get(), [&](bool) { return future; });
              QVERIFY(seed.begin()); QCOMPARE(seed.subtitleRows().size(), 1);
              seed.toggle(0, 0); QVERIFY(seed.save()); }
            std::unique_ptr<SubtitleAttachmentModel> attachments;
            const auto makeAttachments = [&] {
                attachments = std::make_unique<SubtitleAttachmentModel>(m_player, m_settings.get(),
                    SubtitleAttachmentModel::ContextProvider{}, [this](input_item_t *media) {
                        return !m_presets || !m_presets->hasTrackSelectionPolicy(media);
                    });
            };
            const auto makePresets = [&] {
                m_presets = std::make_unique<SubtitlePresetModel>(m_player, m_controller->getSubtitleAppearance(),
                    m_settings.get(), VLC_OBJECT(m_environment->intf));
            };
            if (attachmentsFirst) { makeAttachments(); makePresets(); }
            else { makePresets(); makeAttachments(); }
            { vlc_player_locker lock{m_player};
              QCOMPARE(vlc_player_SetCurrentMedia(m_player, future.item.get()), VLC_SUCCESS);
              QCOMPARE(vlc_player_Start(m_player), VLC_SUCCESS); }
            const QByteArray prefix = QCryptographicHash::hash(QUrl::fromLocalFile(subtitlePath).toEncoded(), QCryptographicHash::Md5).toHex() + '/';
            QTRY_VERIFY_WITH_TIMEOUT(([&] {
                vlc_player_locker lock{m_player};
                if (vlc_player_GetState(m_player) != VLC_PLAYER_STATE_PAUSED) return false;
                for (size_t i = 0; i < vlc_player_GetTrackCount(m_player, SPU_ES); ++i) {
                    const auto *track = vlc_player_GetTrackAt(m_player, SPU_ES, i);
                    if (track->selected && QByteArray(vlc_es_id_GetStrId(track->es_id)).startsWith(prefix)) return true;
                }
                return false;
            })(), 10000);
            QTRY_VERIFY_WITH_TIMEOUT(!m_presets->restoring(), 5000);
            QTest::qWait(700);
            if (qEnvironmentVariableIsSet("VLC_TEST_NATIVE_VERBOSE")) {
                vlc_player_locker lock{m_player};
                qInfo("Cold selected clock=%lld fps=%f categoryDelay=%lld", static_cast<long long>(vlc_player_GetTime(m_player)),
                      double(vlc_player_GetAssociatedSubsFPS(m_player)), static_cast<long long>(vlc_player_GetSubtitleDelay(m_player)));
                for (size_t i = 0; i < vlc_player_GetTrackCount(m_player, SPU_ES); ++i) {
                    const auto *track = vlc_player_GetTrackAt(m_player, SPU_ES, i);
                    qInfo("Cold track %s selected=%d output=%d delay=%lld", vlc_es_id_GetStrId(track->es_id),
                          int(track->selected), int(vlc_player_GetEsIdVout(m_player, track->es_id, nullptr) != nullptr),
                          static_cast<long long>(vlc_player_GetEsIdDelay(m_player, track->es_id)));
                }
            }
            vlc_tick_t held;
            { vlc_player_locker lock{m_player}; held = vlc_player_GetTime(m_player); }
            QVERIFY(held < VLC_TICK_FROM_MS(200));
            const auto image = captureNativeFrame(QString("cold-origin-%1-%2").arg(attachmentsFirst).arg(cycle));
            QVERIFY(!image.isNull());
            unsigned bright = 0;
            for (int y=image.height()*3/4; y<image.height(); ++y)
                for (int x=0; x<image.width(); ++x) {
                    const auto color = image.pixelColor(x,y);
                    bright += color.red()>160 && color.green()>160 && color.blue()>160;
                }
            qInfo("Cold origin attachmentsFirst=%d cycle=%d held=%lld caption=%u marker=%u",
                  int(attachmentsFirst), cycle, static_cast<long long>(held), bright, frameMarker(image));
            if (bright <= 100) {
                QTest::qWait(500);
                const auto later = captureNativeFrame(QString("cold-origin-later-%1-%2").arg(attachmentsFirst).arg(cycle));
                QVERIFY(!later.isNull());
                unsigned laterBright = 0;
                for (int y=later.height()*3/4; y<later.height(); ++y)
                    for (int x=0; x<later.width(); ++x) {
                        const auto color = later.pixelColor(x,y);
                        laterBright += color.red()>160 && color.green()>160 && color.blue()>160;
                    }
                qInfo("Cold origin second snapshot caption=%u marker=%u", laterBright, frameMarker(later));
            }
            QVERIFY2(bright>100, "A saved zero-delay origin cue must render before the first user seek or resume");
            { vlc_player_locker lock{m_player}; QCOMPARE(vlc_player_GetTime(m_player), held); }
            QVERIFY(stop()); attachments.reset(); m_presets.reset();
            { vlc_player_locker lock{m_player}; QCOMPARE(vlc_player_SetCurrentMedia(m_player, nullptr), VLC_SUCCESS); }
        }
        m_presets = std::make_unique<SubtitlePresetModel>(m_player, m_controller->getSubtitleAppearance(),
            m_settings.get(), VLC_OBJECT(m_environment->intf));
    }
    void attachmentSelectedJustBeforePause()
    {
        { vlc_player_locker lock{m_player}; vlc_player_Stop(m_player); }
        QTRY_VERIFY_WITH_TIMEOUT(([&] { vlc_player_locker lock{m_player};
            return vlc_player_GetState(m_player) == VLC_PLAYER_STATE_STOPPED; })(), 10000);
        QTemporaryDir temporary; QVERIFY(temporary.isValid());
        const auto subtitlePath = temporary.filePath("Selection before pause.srt");
        { QFile file(subtitlePath); QVERIFY(file.open(QIODevice::WriteOnly));
          QVERIFY(file.write("1\n00:00:00,000 --> 00:01:00,000\nSELECTED BEFORE PAUSE\n") > 0); }
        struct SelectionPause {
            QByteArray prefix;
            std::atomic_bool requested{false};
        } trace;
        trace.prefix = QCryptographicHash::hash(QUrl::fromLocalFile(subtitlePath).toEncoded(), QCryptographicHash::Md5).toHex() + '/';
        static const vlc_player_cbs callbacks = [] {
            vlc_player_cbs c{};
            c.on_track_selection_changed = [](vlc_player_t *player, vlc_es_id_t *, vlc_es_id_t *selected, void *data) {
                auto *trace = static_cast<SelectionPause *>(data);
                if (selected && QByteArray(vlc_es_id_GetStrId(selected)).startsWith(trace->prefix)) {
                    qInfo("Selection-before-pause native state=%d", int(vlc_player_GetState(player)));
                    vlc_player_Pause(player);
                    trace->requested = true;
                }
            };
            return c;
        }();
        vlc_player_listener_id *listener;
        { vlc_player_locker lock{m_player}; listener = vlc_player_AddListener(m_player, &callbacks, &trace); }
        QVERIFY(listener);
        auto remove = [this](vlc_player_listener_id *id) {
            vlc_player_locker lock{m_player}; vlc_player_RemoveListener(m_player, id);
        };
        std::unique_ptr<vlc_player_listener_id, decltype(remove)> listenerOwner(listener, remove);
        auto *item = input_item_New(QUrl::fromLocalFile(m_fixture).toEncoded().constData(), "Selection-before-pause fixture");
        QVERIFY(item);
        { vlc_player_locker lock{m_player};
          vlc_player_SetStartPaused(m_player, false);
          QCOMPARE(vlc_player_SetCurrentMedia(m_player, item), VLC_SUCCESS);
          input_item_Release(item);
          vlc_player_SetAssociatedSubsFPS(m_player, 0.f);
          QCOMPARE(vlc_player_Start(m_player), VLC_SUCCESS); }
        QTRY_VERIFY_WITH_TIMEOUT(([&] { vlc_player_locker lock{m_player};
            return vlc_player_GetState(m_player) == VLC_PLAYER_STATE_PLAYING
                && vlc_player_GetTime(m_player) > VLC_TICK_FROM_MS(200); })(), 10000);
        const auto uri = QUrl::fromLocalFile(subtitlePath).toEncoded();
        { vlc_player_locker lock{m_player};
          QCOMPARE(vlc_player_AddAssociatedMedia(m_player, SPU_ES, uri.constData(), true, false, false), VLC_SUCCESS); }
        QTRY_VERIFY_WITH_TIMEOUT(trace.requested.load(), 5000);
        QTRY_COMPARE_WITH_TIMEOUT(m_controller->getPlayingState(), PlayerController::PLAYING_STATE_PAUSED, 5000);
        QTest::qWait(700);
        const auto image = captureNativeFrame("selection-before-pause");
        QVERIFY(!image.isNull());
        unsigned bright = 0;
        for (int y=image.height()*3/4; y<image.height(); ++y)
            for (int x=0; x<image.width(); ++x) {
                const auto color=image.pixelColor(x,y);
                bright += color.red()>160 && color.green()>160 && color.blue()>160;
            }
        qInfo("Selection-before-pause caption=%u marker=%u", bright, frameMarker(image));
        QVERIFY2(bright>100, "A subtitle selected immediately before pausing must appear without a user seek/resume");
        listenerOwner.reset();
        { vlc_player_locker lock{m_player}; vlc_player_Stop(m_player); }
        QTRY_VERIFY_WITH_TIMEOUT(([&] { vlc_player_locker lock{m_player};
            return vlc_player_GetState(m_player) == VLC_PLAYER_STATE_STOPPED; })(), 10000);
        { vlc_player_locker lock{m_player}; QCOMPARE(vlc_player_SetCurrentMedia(m_player, nullptr), VLC_SUCCESS); }
    }
    void subtitleRefreshExpiryKeepsAtoBDeadline()
    {
        { vlc_player_locker lock{m_player}; vlc_player_Stop(m_player); }
        QTRY_VERIFY_WITH_TIMEOUT(([&] { vlc_player_locker lock{m_player};
            return vlc_player_GetState(m_player) == VLC_PLAYER_STATE_STOPPED; })(), 10000);
        auto clearLoop = [this](vlc_player_t *) {
            vlc_player_locker lock{m_player};
            vlc_player_SetAtoBLoop(m_player, VLC_PLAYER_ABLOOP_NONE);
            vlc_player_Stop(m_player);
        };
        std::unique_ptr<vlc_player_t, decltype(clearLoop)> loopOwner(m_player, clearLoop);
        QTemporaryDir temporary; QVERIFY(temporary.isValid());
        const auto mediaPath = temporary.filePath("A-B fresh source.mp4");
        QVERIFY(QFile::copy(m_fixture, mediaPath));
        auto *item = input_item_New(QUrl::fromLocalFile(mediaPath).toEncoded().constData(), "Subtitle refresh and A-B fixture");
        QVERIFY(item);
        { vlc_player_locker lock{m_player};
          vlc_player_SetStartPaused(m_player, false);
          QCOMPARE(vlc_player_SetCurrentMedia(m_player, item), VLC_SUCCESS); input_item_Release(item);
          vlc_player_SetAssociatedSubsFPS(m_player, 0.f);
          vlc_player_ChangeRate(m_player, 1.f);
          QCOMPARE(vlc_player_Start(m_player), VLC_SUCCESS); }
        const auto time = [&] { vlc_player_locker lock{m_player}; return vlc_player_GetTime(m_player); };
        QTRY_VERIFY_WITH_TIMEOUT(([&] { vlc_player_locker lock{m_player};
            return vlc_player_GetState(m_player) == VLC_PLAYER_STATE_PLAYING
                && vlc_player_GetTime(m_player) > VLC_TICK_FROM_MS(200); })(), 10000);
        { vlc_player_locker lock{m_player}; vlc_player_ChangeRate(m_player, 1.f); }
        QTRY_COMPARE_WITH_TIMEOUT(([&] { vlc_player_locker lock{m_player}; return vlc_player_GetRate(m_player); })(), 1.f, 5000);
        QTest::qWait(200);
        QTRY_VERIFY_WITH_TIMEOUT(time() > VLC_TICK_FROM_SEC(1), 10000);
        const auto loopStart = time();
        { vlc_player_locker lock{m_player};
          // Keep the established playing clock; an unrelated startup seek
          // can retain the audio clock's older point until buffering settles.
          QCOMPARE(vlc_player_SetAtoBLoopTime(m_player, loopStart, loopStart + VLC_TICK_FROM_SEC(8)), VLC_SUCCESS); }
        { vlc_player_locker lock{m_player}; vlc_player_SetAssociatedSubsFPS(m_player, 30.f); }
        QTest::qWait(5300); // The pending subtitle request expires before B.
        const auto afterExpiry = time();
        qInfo("Subtitle refresh expiry start=%lld clock=%lld before A-B boundary",
              static_cast<long long>(loopStart), static_cast<long long>(afterExpiry));
        { vlc_player_locker lock{m_player}; QCOMPARE(vlc_player_GetRate(m_player), 1.f); }
        QVERIFY(afterExpiry - loopStart > VLC_TICK_FROM_SEC(5));
        QVERIFY(afterExpiry - loopStart < VLC_TICK_FROM_MS(6500));
        QTRY_VERIFY_WITH_TIMEOUT(time() < loopStart + VLC_TICK_FROM_SEC(2), 5000);
        loopOwner.reset();
        QTRY_VERIFY_WITH_TIMEOUT(([&] { vlc_player_locker lock{m_player};
            return vlc_player_GetState(m_player) == VLC_PLAYER_STATE_STOPPED; })(), 10000);
        { vlc_player_locker lock{m_player};
          QCOMPARE(vlc_player_SetCurrentMedia(m_player, nullptr), VLC_SUCCESS);
          vlc_player_SetAssociatedSubsFPS(m_player, 0.f); }
    }
    void attachmentReusedMediaCaption_data()
    {
        QTest::addColumn<bool>("reuseItem");
        QTest::addColumn<bool>("preferEnglish");
        QTest::newRow("fresh-item-control") << false << false;
        QTest::newRow("reused-playlist-item") << true << false;
        QTest::newRow("fresh-item-english-default") << false << true;
        QTest::newRow("reused-item-english-default") << true << true;
    }
    void attachmentReusedMediaCaption()
    {
        QFETCH(bool, reuseItem);
        QFETCH(bool, preferEnglish);
        { vlc_player_locker lock{m_player}; vlc_player_Stop(m_player); }
        QTRY_VERIFY_WITH_TIMEOUT(([&] { vlc_player_locker lock{m_player};
            return vlc_player_GetState(m_player) == VLC_PLAYER_STATE_STOPPED; })(), 10000);
        m_presets.reset(); m_settings->clear();
        QTemporaryDir temporary; QVERIFY(temporary.isValid());
        const auto aPath=temporary.filePath("A.mp4"), bPath=temporary.filePath("B.mp4");
        const auto subtitlePath=temporary.filePath("Recovery.srt");
        QVERIFY(QFile::copy(m_fixture,aPath)); QVERIFY(QFile::copy(m_fixture,bPath));
        { QFile file(subtitlePath); QVERIFY(file.open(QIODevice::WriteOnly));
          QVERIFY(file.write("1\n00:00:00,000 --> 00:01:00,000\nRECOVERED CAPTION\n")>0); }
        auto makeItem = [&](const QString &path) {
            SubtitleAttachmentModel::Context context;
            auto *item=input_item_New(QUrl::fromLocalFile(path).toEncoded().constData(),"A-B-A attachment fixture");
            if (item) { context.item.reset(item); input_item_Release(item); }
            return context;
        };
        auto a=makeItem(aPath), b=makeItem(bPath);
        QVERIFY(a.item); QVERIFY(b.item);
        { SubtitleAttachmentModel seed(nullptr,m_settings.get(),[&](bool){return a;});
          QVERIFY(seed.begin()); QCOMPARE(seed.subtitleRows().size(),1); seed.toggle(0,0); QVERIFY(seed.save()); }
        m_presets=std::make_unique<SubtitlePresetModel>(m_player,m_controller->getSubtitleAppearance(),
            m_settings.get(),VLC_OBJECT(m_environment->intf));
        auto attachments=std::make_unique<SubtitleAttachmentModel>(m_player,m_settings.get(),
            SubtitleAttachmentModel::ContextProvider{},[this](input_item_t *item){
                return !m_presets || !m_presets->hasTrackSelectionPolicy(item);
            });
        const QByteArray prefix=QCryptographicHash::hash(QUrl::fromLocalFile(subtitlePath).toEncoded(),QCryptographicHash::Md5).toHex()+'/';
        auto selected = [&] {
            vlc_player_locker lock{m_player};
            for (size_t i=0;i<vlc_player_GetTrackCount(m_player,SPU_ES);++i) {
                const auto *track=vlc_player_GetTrackAt(m_player,SPU_ES,i);
                if (track->selected && QByteArray(vlc_es_id_GetStrId(track->es_id)).startsWith(prefix)) return true;
            }
            return false;
        };
        auto captionPixels=[](const QImage &image) {
            unsigned bright=0;
            for (int y=image.height()*3/4;y<image.height();++y)
                for (int x=0;x<image.width();++x) {
                    const auto color=image.pixelColor(x,y);
                    bright+=color.red()>160 && color.green()>160 && color.blue()>160;
                }
            return bright;
        };
        QByteArray previousLanguage;
        { vlc_player_locker lock{m_player};
          char *language = vlc_player_GetCategoryLanguage(m_player, SPU_ES);
          previousLanguage = language; free(language);
          vlc_player_SelectSubtitleLanguage(m_player, preferEnglish ? "eng" : "");
          vlc_player_SetStartPaused(m_player,true);
          QCOMPARE(vlc_player_SetCurrentMedia(m_player,a.item.get()),VLC_SUCCESS);
          QCOMPARE(vlc_player_Start(m_player),VLC_SUCCESS); }
        QTRY_VERIFY_WITH_TIMEOUT(selected(),10000);
        QTRY_COMPARE_WITH_TIMEOUT(m_controller->getPlayingState(),PlayerController::PLAYING_STATE_PAUSED,10000);
        QTest::qWait(700);
        const auto first=captureNativeFrame(QString("reuse-%1-english-%2-first-a").arg(reuseItem).arg(preferEnglish));
        QVERIFY(!first.isNull()); QVERIFY(captionPixels(first)>100);
        // Keep the player started across source changes, as real Next/Previous
        // does. An intervening Stop would discard the reusable video output.
        { vlc_player_locker lock{m_player};
          vlc_player_SetStartPaused(m_player,false);
          QCOMPARE(vlc_player_SetCurrentMedia(m_player,b.item.get()),VLC_SUCCESS);
          QCOMPARE(vlc_player_Start(m_player),VLC_SUCCESS); }
        QTRY_VERIFY_WITH_TIMEOUT(([&]{vlc_player_locker lock{m_player};
            return vlc_player_GetCurrentMedia(m_player)==b.item.get()
                && vlc_player_GetState(m_player)==VLC_PLAYER_STATE_PLAYING
                && vlc_player_GetTrackCount(m_player,SPU_ES)==2;})(),10000);
        QTest::qWait(1200); QVERIFY(!selected());
        auto returned=reuseItem?a:makeItem(aPath); QVERIFY(returned.item);
        { vlc_player_locker lock{m_player};
          QCOMPARE(vlc_player_SetCurrentMedia(m_player,returned.item.get()),VLC_SUCCESS);
          QCOMPARE(vlc_player_Start(m_player),VLC_SUCCESS); }
        QTRY_VERIFY_WITH_TIMEOUT(selected(),10000);
        QTRY_VERIFY_WITH_TIMEOUT(!m_presets->restoring(),5000);
        QTest::qWait(1200);
        const auto image=captureNativeFrame(QString("reuse-%1-english-%2-returned-a").arg(reuseItem).arg(preferEnglish));
        QVERIFY(!image.isNull());
        vlc_tick_t clock;
        {vlc_player_locker lock{m_player};clock=vlc_player_GetTime(m_player);}
        qInfo("A-B-A reuse=%d english=%d selected=%d caption=%u marker=%u clock=%lld",int(reuseItem),int(preferEnglish),int(selected()),
              captionPixels(image),frameMarker(image),static_cast<long long>(clock));
        const bool captionVisible = captionPixels(image)>100;
        {vlc_player_locker lock{m_player};vlc_player_Stop(m_player);}
        QTRY_VERIFY_WITH_TIMEOUT(([&]{vlc_player_locker lock{m_player};
            return vlc_player_GetState(m_player)==VLC_PLAYER_STATE_STOPPED;})(),10000);
        attachments.reset(); m_presets.reset();
        {vlc_player_locker lock{m_player};
         QCOMPARE(vlc_player_SetCurrentMedia(m_player,nullptr),VLC_SUCCESS);
         vlc_player_SelectSubtitleLanguage(m_player, previousLanguage.constData());}
        m_presets=std::make_unique<SubtitlePresetModel>(m_player,m_controller->getSubtitleAppearance(),
            m_settings.get(),VLC_OBJECT(m_environment->intf));
        QVERIFY2(captionVisible,"The saved subtitle must render after A-B-A without a user seek/resume");
    }
    void fpsResetBeforeFirstCue_data()
    {
        QTest::addColumn<bool>("resetFps");
        QTest::addColumn<bool>("seekExactCueFirst");
        QTest::newRow("retain-30-fps") << false << false;
        QTest::newRow("reset-to-normal-before-cue") << true << false;
        QTest::newRow("retain-30-fps-exact-cue-first") << false << true;
        QTest::newRow("reset-to-normal-exact-cue-first") << true << true;
    }
    void fpsResetBeforeFirstCue()
    {
        QFETCH(bool, resetFps);
        QFETCH(bool, seekExactCueFirst);
        m_presets.reset();
        auto state = [this] { vlc_player_locker lock{m_player}; return vlc_player_GetState(m_player); };
        { vlc_player_locker lock{m_player}; vlc_player_Stop(m_player); }
        QTRY_COMPARE_WITH_TIMEOUT(state(), VLC_PLAYER_STATE_STOPPED, 10000);
        QTemporaryDir temporary; QVERIFY(temporary.isValid());
        const auto subtitle = temporary.filePath("Late.srt");
        QFile file(subtitle); QVERIFY(file.open(QIODevice::WriteOnly));
        QVERIFY(file.write("1\n00:00:20,000 --> 00:00:40,000\nLATE FPS CUE\n") > 0); file.close();
        const auto uri = QUrl::fromLocalFile(subtitle).toEncoded();
        const QByteArray prefix = QCryptographicHash::hash(uri, QCryptographicHash::Md5).toHex() + '/';
        auto *media = input_item_New(QUrl::fromLocalFile(m_fixture).toEncoded().constData(), "Late FPS fixture");
        QVERIFY(media);
        const auto release = std::unique_ptr<input_item_t, decltype(&input_item_Release)>(media, &input_item_Release);
        auto *slave = input_item_slave_New(uri.constData(), SLAVE_TYPE_SPU, SLAVE_PRIORITY_USER);
        QVERIFY(slave); slave->b_forced = true;
        QCOMPARE(input_item_AddSlave(media, slave), VLC_SUCCESS);
        auto restoreStart = [this](vlc_player_t *) { vlc_player_locker lock{m_player}; vlc_player_SetStartPaused(m_player, false); };
        std::unique_ptr<vlc_player_t, decltype(restoreStart)> owner(m_player, restoreStart);
        {
            vlc_player_locker lock{m_player};
            vlc_player_SetAssociatedSubsFPS(m_player, 30.f);
            vlc_player_SetStartPaused(m_player, true);
            QCOMPARE(vlc_player_SetCurrentMedia(m_player, media), VLC_SUCCESS);
            QCOMPARE(vlc_player_Start(m_player), VLC_SUCCESS);
        }
        QTRY_COMPARE_WITH_TIMEOUT(state(), VLC_PLAYER_STATE_PAUSED, 10000);
        QTRY_VERIFY_WITH_TIMEOUT(([&] {
            vlc_player_locker lock{m_player};
            const auto *track = vlc_player_GetSelectedTrack(m_player, SPU_ES);
            return track && QByteArray(vlc_es_id_GetStrId(track->es_id)).startsWith(prefix);
        })(), 5000);
        { vlc_player_locker lock{m_player}; QVERIFY(vlc_player_GetTime(m_player) < VLC_TICK_FROM_MS(200)); }
        if (resetFps) {
            m_controller->setSubtitleFPS(0.f);
            QTRY_COMPARE(m_controller->getSubtitleFPS(), 0.f);
            QTest::qWait(500);
        }
        if (seekExactCueFirst) {
            if (resetFps) {
                { vlc_player_locker lock{m_player}; vlc_player_SetTime(m_player, VLC_TICK_FROM_SEC(19)); }
                QTest::qWait(800);
                const auto before = captureNativeFrame(QStringLiteral("late-before-boundary"));
                QVERIFY(!before.isNull());
                unsigned bright = 0;
                for (int y=before.height()*3/4; y<before.height(); ++y)
                    for (int x=0; x<before.width(); ++x) {
                        const auto color = before.pixelColor(x,y);
                        bright += color.red()>160 && color.green()>160 && color.blue()>160;
                    }
                QCOMPARE(frameMarker(before), 475u);
                QCOMPARE(bright, 0u);
            }
            { vlc_player_locker lock{m_player}; vlc_player_SetTime(m_player, VLC_TICK_FROM_SEC(20)); }
            QTest::qWait(800);
            const auto boundary = captureNativeFrame(QString("late-boundary-%1").arg(resetFps));
            QVERIFY(!boundary.isNull());
            unsigned bright = 0;
            for (int y=boundary.height()*3/4; y<boundary.height(); ++y)
                for (int x=0; x<boundary.width(); ++x) {
                    const auto color = boundary.pixelColor(x,y);
                    bright += color.red()>160 && color.green()>160 && color.blue()>160;
                }
            qInfo("Late boundary reset=%d caption pixels=%u marker=%u", int(resetFps), bright, frameMarker(boundary));
            QVERIFY2(bright>100, "A paused exact-start seek must display the selected cue");
        }
        { vlc_player_locker lock{m_player}; vlc_player_SetTime(m_player, VLC_TICK_FROM_SEC(25)); }
        QTest::qWait(800);
        const auto directory = qEnvironmentVariable("VLC_FPS_SNAPSHOTS");
        QVERIFY(!directory.isEmpty());
        const auto path = QDir(directory).filePath(QString("late-fps-%1-%2.png").arg(resetFps).arg(seekExactCueFirst));
        auto *output = vlc_player_vout_Hold(m_player); QVERIFY(output);
        var_Create(output, "snapshot-width", VLC_VAR_INTEGER); var_SetInteger(output, "snapshot-width", 960);
        var_Create(output, "snapshot-height", VLC_VAR_INTEGER); var_SetInteger(output, "snapshot-height", 540);
        var_Create(output, "snapshot-path", VLC_VAR_STRING); var_SetString(output, "snapshot-path", qtu(path));
        var_Create(output, "snapshot-format", VLC_VAR_STRING); var_SetString(output, "snapshot-format", "png");
        var_TriggerCallback(output, "video-snapshot"); vout_Release(output);
        QImage image;
        for (int i=0; i<100 && image.isNull(); ++i) { QTest::qWait(20); image.load(path); }
        QVERIFY(!image.isNull());
        unsigned bright=0;
        for (int y=image.height()*3/4; y<image.height(); ++y)
            for (int x=0; x<image.width(); ++x) {
                const auto color=image.pixelColor(x,y);
                bright += color.red()>160 && color.green()>160 && color.blue()>160;
            }
        qInfo("Late FPS reset=%d caption pixels=%u marker=%u", int(resetFps), bright, frameMarker(image));
        QVERIFY2(bright>100, "The selected late cue must render after seeking with FPS retained/reset before its first output");
        QCOMPARE(state(), VLC_PLAYER_STATE_PAUSED);
        if (seekExactCueFirst) {
            { vlc_player_locker lock{m_player}; vlc_player_SetTime(m_player, VLC_TICK_FROM_SEC(40)); }
            QTest::qWait(800);
            const auto after = captureNativeFrame(QString("late-after-end-%1").arg(resetFps));
            QVERIFY(!after.isNull());
            unsigned remaining = 0;
            for (int y=after.height()*3/4; y<after.height(); ++y)
                for (int x=0; x<after.width(); ++x) {
                    const auto color = after.pixelColor(x,y);
                    remaining += color.red()>160 && color.green()>160 && color.blue()>160;
                }
            QCOMPARE(frameMarker(after), 1000u);
            QCOMPARE(remaining, 0u);
        }
    }
    void cleanupTestCase()
    {
        // Listener owners and held IDs die before their real native interface,
        // playlist/player and LibVLC instance; no controller survives the env.
        m_presets.reset();
        m_controller.reset();
        m_first.reset(); m_second.reset();
        if (m_environment && m_environment->intf)
            m_environment->intf->p_mainPlayerController = nullptr;
        m_player = nullptr;
        m_environment.reset();
        m_settings.reset();
    }
};
QTEST_GUILESS_MAIN(TestPlayerSubtitleDelay)
#include "test_player_subtitle_delay.moc"
