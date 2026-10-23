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
#include "player/subtitle_appearance.hpp"
#include "maininterface/subtitle_preset_model.hpp"
#include <vlc_input_item.h>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QThread>
#include <limits>

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
        vlc_es_id_t *ids[] = {primary, secondary, nullptr};
        vlc_player_locker lock{m_player};
        const auto selected = vlc_player_SelectEsIdList(m_player, SPU_ES, ids);
        // The short second SRT must have a real cue while the video is paused.
        // This native seek does not dispatch any queued QObject callbacks.
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
        m_controller = std::make_unique<PlayerController>(m_environment->intf);
        m_environment->intf->p_mainPlayerController = m_controller.get();
        connect(m_controller.get(), &PlayerController::subtitleDelayChanged, this,
                &TestPlayerSubtitleDelay::observeAfterUnlock, Qt::DirectConnection);
        connect(m_controller.get(), &PlayerController::secondarySubtitleDelayChanged, this,
                &TestPlayerSubtitleDelay::observeAfterUnlock, Qt::DirectConnection);
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
        QTRY_VERIFY_WITH_TIMEOUT(m_controller->getSubtitleTracks()->rowCount() == 2, 10000);
        {
            vlc_player_locker lock{m_player};
            QCOMPARE(vlc_player_GetTrackCount(m_player, SPU_ES), size_t(2));
            m_first.reset(vlc_player_GetTrackAt(m_player, SPU_ES, 0)->es_id);
            m_second.reset(vlc_player_GetTrackAt(m_player, SPU_ES, 1)->es_id);
        }
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
