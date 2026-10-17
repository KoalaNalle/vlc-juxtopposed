// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "maininterface/subtitle_preset_model.hpp"
#include <vlc/vlc.h>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMutex>
#include <QMutexLocker>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>
#include <array>
#include <atomic>

extern "C" vlc_player_t *vlc_test_media_player_core(libvlc_media_player_t *);
extern "C" vlc_object_t *vlc_test_media_player_object(libvlc_media_player_t *);

class TestSubtitlePresets : public QObject
{
    Q_OBJECT
    libvlc_instance_t *m_instance = nullptr;
    libvlc_media_player_t *m_mediaPlayer = nullptr;
    vout_thread_t *m_output = nullptr;
    std::unique_ptr<SubtitleAppearance> m_style;
    std::unique_ptr<SubtitlePresetModel> m_presets;
    std::unique_ptr<QSettings> m_settings;
    QTemporaryDir m_temporary;
    QString m_fixture, m_directory, m_savedId;
    SubtitleAppearance::Snapshot m_savedStyle;
    QMutex m_mutex;
    alignas(32) std::array<unsigned char, 320 * 180 * 4> m_buffer;
    QImage m_image;
    std::atomic_uint m_frames{0};
    static void *lockVideo(void *opaque, void **planes)
    {
        auto *self = static_cast<TestSubtitlePresets *>(opaque);
        self->m_mutex.lock(); *planes = self->m_buffer.data(); return nullptr;
    }
    static void unlockVideo(void *opaque, void *, void *const *)
    {
        auto *self = static_cast<TestSubtitlePresets *>(opaque);
        self->m_image = QImage(self->m_buffer.data(), 320, 180, 1280, QImage::Format_RGB32).copy();
        self->m_mutex.unlock(); ++self->m_frames;
    }
    static void displayVideo(void *, void *) {}
    QImage frame(const QString &name = {})
    {
        if (!name.isEmpty()) QTest::qWait(300);
        QMutexLocker guard(&m_mutex);
        auto image = m_image;
        if (!name.isEmpty()) image.save(m_directory + '/' + name + ".png");
        return image;
    }
    static QRect colored(const QImage &image, bool red)
    {
        QRect bounds;
        for (int y = 0; y < image.height(); ++y) for (int x = 0; x < image.width(); ++x) {
            const auto color = image.pixelColor(x, y);
            if ((red ? color.red() > 180 && color.green() < 80 : color.green() > 180 && color.red() < 80)
                && color.blue() < 80) bounds |= QRect(x, y, 1, 1);
        }
        return bounds;
    }
    void closePlayer()
    {
        m_presets.reset(); m_style.reset();
        if (m_output) { vout_Release(m_output); m_output = nullptr; }
        if (m_mediaPlayer) { libvlc_media_player_stop_async(m_mediaPlayer); libvlc_media_player_release(m_mediaPlayer); m_mediaPlayer = nullptr; }
        { QMutexLocker guard(&m_mutex); m_image = {}; }
        m_frames = 0;
    }
    bool openPlayer(const QString &fixture)
    {
        m_mediaPlayer = libvlc_media_player_new(m_instance, nullptr, nullptr);
        if (!m_mediaPlayer) return false;
        libvlc_video_set_callbacks(m_mediaPlayer, lockVideo, unlockVideo, displayVideo, this);
        libvlc_video_set_format(m_mediaPlayer, "BGRA", 320, 180, 1280);
        auto *player = vlc_test_media_player_core(m_mediaPlayer);
        m_style = std::make_unique<SubtitleAppearance>(player);
        // Production MainCtx creates the service before later vout attachment.
        // A saved association must not apply into partly attached proxies.
        m_presets = std::make_unique<SubtitlePresetModel>(player, m_style.get(), m_settings.get(),
            vlc_test_media_player_object(m_mediaPlayer));
        auto *media = libvlc_media_new_path(qtu(fixture));
        if (!media) return false;
        libvlc_media_player_set_media(m_mediaPlayer, media); libvlc_media_release(media);
        if (libvlc_media_player_play(m_mediaPlayer) != 0) return false;
        for (int i = 0; i < 200 && m_frames.load() < 4; ++i) QTest::qWait(50);
        if (m_frames.load() < 4) return false;
        vlc_player_Lock(player); m_output = vlc_player_vout_Hold(player); vlc_player_Unlock(player);
        if (!m_output) return false;
        const auto subtitle = QUrl::fromLocalFile(QFileInfo(m_fixture).absolutePath() + "/subtitle-style.srt").toEncoded();
        if (libvlc_media_player_add_slave(m_mediaPlayer, libvlc_media_slave_type_subtitle, subtitle.constData(), true) != 0) return false;
        for (int i = 0; i < 200 && colored(frame(), false).isEmpty(); ++i) QTest::qWait(50);
        if (colored(frame(), false).isEmpty()) return false;
        // Text is already selected and rendered when the output is attached,
        // making the formerly partial-proxy restoration race deterministic.
        m_style->resetOutput(m_output);
        return true;
    }
    void pause()
    {
        libvlc_media_player_set_pause(m_mediaPlayer, true);
        QTRY_COMPARE(libvlc_media_player_get_state(m_mediaPlayer), libvlc_Paused);
    }
    void holdCue()
    {
        // The saved mode adds 1.5 s to the cue, with a 650 ms own delay. Seek
        // into that active interval after pausing, rather than racing expiry.
        pause();
        const auto before = m_frames.load();
        QCOMPARE(libvlc_media_player_set_time(m_mediaPlayer, 1500000, false), 0);
        QTRY_VERIFY_WITH_TIMEOUT(m_frames.load() > before, 5000);
    }
    QPair<QString, QString> subtitleRoles() const
    {
        QPair<QString, QString> ids;
        auto *player = vlc_test_media_player_core(m_mediaPlayer);
        vlc_player_Lock(player);
        for (size_t i = 0; i < vlc_player_GetTrackCount(player, SPU_ES); ++i) {
            const auto *track = vlc_player_GetTrackAt(player, SPU_ES, i);
            if (!track->selected) continue;
            vlc_vout_order order = VLC_VOUT_ORDER_NONE;
            vlc_player_GetEsIdVout(player, track->es_id, &order);
            if (order == VLC_VOUT_ORDER_PRIMARY) ids.first = QString::fromUtf8(vlc_es_id_GetStrId(track->es_id));
            if (order == VLC_VOUT_ORDER_SECONDARY) ids.second = QString::fromUtf8(vlc_es_id_GetStrId(track->es_id));
        }
        vlc_player_Unlock(player); return ids;
    }
    qint64 selectedTrackDelay() const
    {
        auto *player = vlc_test_media_player_core(m_mediaPlayer);
        vlc_player_Lock(player);
        const auto *track = vlc_player_GetSelectedTrack(player, SPU_ES);
        const auto value = track ? vlc_player_GetEsIdDelay(player, track->es_id) : INT64_MAX;
        vlc_player_Unlock(player); return value;
    }
    qint64 delay() const
    {
        auto *player = vlc_test_media_player_core(m_mediaPlayer);
        vlc_player_Lock(player); const auto delay = vlc_player_GetSubtitleDelay(player); vlc_player_Unlock(player); return delay;
    }
private slots:
    void initTestCase()
    {
        m_fixture = qEnvironmentVariable("VLC_SUBTITLE_FIXTURE");
        if (m_fixture.isEmpty()) QSKIP("Set VLC_SUBTITLE_FIXTURE to the generated subtitle-style.mp4");
        QVERIFY(m_temporary.isValid());
        m_directory = qEnvironmentVariable("VLC_SUBTITLE_OUTPUT", m_temporary.path());
        QVERIFY(QDir().mkpath(m_directory));
        m_settings = std::make_unique<QSettings>(m_temporary.filePath("presets.ini"), QSettings::IniFormat);
        const char *args[] = {"--ignore-config", "--no-media-library", "--no-audio", "--no-osd",
            "--no-video-title-show", "--no-sub-autodetect-file", "--no-hw-dec", "--verbose=2",
            "--file-logging", "--logfile=subtitle-presets-native.log"};
        m_instance = libvlc_new(ARRAY_SIZE(args), args); QVERIFY(m_instance);
        QVERIFY(openPlayer(m_fixture));
        QTRY_VERIFY_WITH_TIMEOUT(m_style->available(), 10000);
        QTRY_VERIFY_WITH_TIMEOUT(!colored(frame(), false).isEmpty(), 10000);
        holdCue();
        QVERIFY(m_presets->canSave()); QCOMPARE(m_presets->entries().size(), 1);
    }
    void saveApplyDefaultAndActualPixels()
    {
        m_style->setEmbeddedFormat(false); m_style->setTextColor("#FF0000"); m_style->setFontFamily("Courier New");
        m_style->setSizeMode(0); m_style->setFontSize(8); m_style->setOutlineSize(5);
        m_style->setSizeMode(1); m_style->setFontSize(28); m_style->setOutlineSize(2);
        m_style->setOffset(20); m_style->setBackgroundOpacity(25);
        var_SetInteger(m_output, "subsdelay-mode", 0); m_style->setDurationFactor(1.5f);
        auto *player = vlc_test_media_player_core(m_mediaPlayer);
        vlc_player_Lock(player);
        vlc_player_SetSubtitleTextScale(player, 150);
        vlc_player_SetSubtitleDelay(player, 350000, VLC_PLAYER_WHENCE_ABSOLUTE);
        const auto *selected = vlc_player_GetSelectedTrack(player, SPU_ES);
        const int setOverride = selected ? vlc_player_SetEsIdDelay(player, selected->es_id, 650000, VLC_PLAYER_WHENCE_ABSOLUTE) : VLC_EGENERIC;
        vlc_player_SetAssociatedSubsFPS(player, 25);
        vlc_player_Unlock(player);
        QCOMPARE(setOverride, VLC_SUCCESS); QCOMPARE(selectedTrackDelay(), qint64(650000));
        QTRY_VERIFY_WITH_TIMEOUT(!colored(frame(), true).isEmpty(), 5000);
        QVERIFY(m_style->snapshot(&m_savedStyle));
        QCOMPARE(m_savedStyle.pixelSize, 28.f); QCOMPARE(m_savedStyle.relativeSize, 8.f);
        QCOMPARE(m_savedStyle.pixelOutline, 2.f); QCOMPARE(m_savedStyle.relativeOutline, 5.f);
        QCOMPARE(m_savedStyle.scale, 150); QCOMPARE(m_savedStyle.durationMode, 0);
        QSignalSpy changed(m_presets.get(), &SubtitlePresetModel::stateChanged);
        QVERIFY(m_presets->save("  Series – subtitles  "));
        QCOMPARE(m_presets->entries().size(), 2); QVERIFY(!m_presets->edited());
        m_savedId = m_presets->selectedId(); QVERIFY(m_savedId != "default");
        QCOMPARE(m_presets->selectedName(), QString("Series – subtitles"));
        QVERIFY(changed.count() > 0); QVERIFY(!colored(frame("saved-red"), true).isEmpty());
        QVERIFY(!m_presets->save("series – subtitles")); QVERIFY(!m_presets->error().isEmpty());
        QCOMPARE(m_presets->entries().size(), 2);
        QVERIFY(m_presets->apply("default"));
        QTRY_VERIFY_WITH_TIMEOUT(!m_presets->restoring(), 5000);
        QTRY_VERIFY_WITH_TIMEOUT(!colored(frame(), false).isEmpty(), 5000);
        QCOMPARE(delay(), qint64(0)); QCOMPARE(selectedTrackDelay(), qint64(INT64_MAX));
        QVERIFY(!var_GetBool(m_output, "sub-text-style-enabled"));
        QCOMPARE(m_style->durationFactor(), 0.f); QVERIFY(!m_presets->edited());
        QVERIFY(!colored(frame("default-green"), false).isEmpty());
        QVERIFY(m_presets->apply(m_savedId));
        QTRY_VERIFY_WITH_TIMEOUT(!m_presets->restoring(), 5000);
        QTRY_VERIFY_WITH_TIMEOUT(!colored(frame(), true).isEmpty(), 5000);
        QCOMPARE(delay(), qint64(350000)); QCOMPARE(selectedTrackDelay(), qint64(650000));
        QCOMPARE(m_style->durationUnit(), QString(" s"));
        QCOMPARE(m_style->durationFactor(), 1.5f);
        SubtitleAppearance::Snapshot applied; QVERIFY(m_style->snapshot(&applied)); QVERIFY(applied == m_savedStyle);
        QVERIFY(!m_presets->edited());
        m_style->setTextColor("#0000FF"); QTRY_VERIFY(m_presets->edited());
        QCOMPARE(m_presets->selectedName(), QString("Custom settings"));
        m_presets->refreshDefaults(); QCOMPARE(m_style->textColor(), QString("#0000FF"));
        QVERIFY(m_presets->edited());
        QVERIFY(m_presets->apply(m_savedId)); QTRY_VERIFY(!m_presets->edited());
    }
    void batchedSnapshotNotifications()
    {
        QTest::qWait(50);
        SubtitleAppearance::Snapshot original; QVERIFY(m_style->snapshot(&original));
        QObject observer; unsigned notifications = 0; bool observedAvailable = false;
        connect(m_style.get(), &SubtitleAppearance::stateChanged, &observer, [&] {
            ++notifications; observedAvailable = m_style->available();
        }, Qt::DirectConnection);
        auto *player = vlc_test_media_player_core(m_mediaPlayer);
        vlc_player_Lock(player); auto *item = input_item_Hold(vlc_player_GetCurrentMedia(player)); vlc_player_Unlock(player);
        auto changed = original; changed.text = 0x00FF00; changed.pixelSize = 24; changed.offset = 10;
        const bool applied = m_style->applySnapshot(changed, item);
        input_item_Release(item);
        QVERIFY(applied); QCOMPARE(notifications, 1u); QVERIFY(observedAvailable);
        // This fixture core is reentrant; exactly one final notification is
        // what protects production QML using the normal-lock playlist player.
        vlc_player_Lock(player); item = input_item_Hold(vlc_player_GetCurrentMedia(player)); vlc_player_Unlock(player);
        notifications = 0;
        const bool restored = m_style->applySnapshot(original, item);
        input_item_Release(item);
        QVERIFY(restored); QCOMPARE(notifications, 1u); QVERIFY(observedAvailable);
    }
    void submittedWritesAreNotReplayedDuringConfirmation()
    {
        QTest::qWait(100);
        struct Counts { std::atomic_uint category{0}, track{0}, fps{0}; } counts;
        const vlc_player_cbs callbacks = [] {
            vlc_player_cbs c{};
            c.on_category_delay_changed = [](vlc_player_t *, es_format_category_e cat, vlc_tick_t, void *opaque) {
                if (cat == SPU_ES) ++static_cast<Counts *>(opaque)->category;
            };
            c.on_track_delay_changed = [](vlc_player_t *, vlc_es_id_t *id, vlc_tick_t, void *opaque) {
                if (vlc_es_id_GetCat(id) == SPU_ES) ++static_cast<Counts *>(opaque)->track;
            };
            c.on_associated_subs_fps_changed = [](vlc_player_t *, float, void *opaque) {
                ++static_cast<Counts *>(opaque)->fps;
            };
            return c;
        }();
        auto *player = vlc_test_media_player_core(m_mediaPlayer);
        auto remove = [player](vlc_player_listener_id *listener) {
            if (!listener) return;
            vlc_player_Lock(player); vlc_player_RemoveListener(player, listener); vlc_player_Unlock(player);
        };
        std::unique_ptr<vlc_player_listener_id, decltype(remove)> listener(nullptr, remove);
        vlc_player_Lock(player); listener.reset(vlc_player_AddListener(player, &callbacks, &counts)); vlc_player_Unlock(player);
        QVERIFY(listener.get());
        QObject observer; bool interruptFirstConfirmation = true;
        connect(m_style.get(), &SubtitleAppearance::stateChanged, &observer, [&] {
            if (interruptFirstConfirmation) {
                interruptFirstConfirmation = false;
                var_SetBool(m_output, "sub-text-style-supported", false);
            }
        }, Qt::DirectConnection);
        const bool accepted = m_presets->apply(m_savedId);
        const bool pending = m_presets->restoring();
        // Restore readiness before dispatching any callbacks. The old retry
        // path would submit a second complete set of writes on this refresh.
        var_SetBool(m_output, "sub-text-style-supported", true);
        QVERIFY(accepted); QVERIFY(pending);
        for (unsigned i = 0; i < 12; ++i) m_presets->refresh();
        QTRY_VERIFY_WITH_TIMEOUT(!m_presets->restoring(), 3000);
        QTest::qWait(100);
        QCOMPARE(counts.category.load(), 1u); QCOMPARE(counts.track.load(), 1u); QCOMPARE(counts.fps.load(), 1u);
        QVERIFY(!m_presets->edited()); QCOMPARE(m_presets->selectedId(), m_savedId);
    }
    void freshPlayerAndSettingsRestore()
    {
        const auto settingsFile = m_settings->fileName();
        closePlayer(); m_settings.reset();
        m_settings = std::make_unique<QSettings>(settingsFile, QSettings::IniFormat);
        QVERIFY(openPlayer(m_fixture));
        QTRY_VERIFY_WITH_TIMEOUT(m_presets->selectedId() == m_savedId && !m_presets->restoring(), 10000);
        holdCue(); QTRY_VERIFY_WITH_TIMEOUT(!colored(frame(), true).isEmpty(), 5000);
        QCOMPARE(delay(), qint64(350000)); QCOMPARE(selectedTrackDelay(), qint64(650000));
        SubtitleAppearance::Snapshot restored; QVERIFY(m_style->snapshot(&restored)); QVERIFY(restored == m_savedStyle);
        QVERIFY(!m_presets->edited()); QCOMPARE(m_presets->entries().size(), 2);
        QVERIFY(!colored(frame("restart-restored-red"), true).isEmpty());
        // The stored policy is value data; the previous player and ES pointers
        // have been destroyed. Real selected tracks must exist in the new core.
        auto *player = vlc_test_media_player_core(m_mediaPlayer);
        vlc_player_Lock(player); const auto *track = vlc_player_GetSelectedTrack(player, SPU_ES);
        const QString trackId = track ? QString::fromUtf8(vlc_es_id_GetStrId(track->es_id)) : QString{};
        vlc_player_Unlock(player); QVERIFY(!trackId.isEmpty());
    }
    void actualTrackSelectionIsRestored()
    {
        auto *player = vlc_test_media_player_core(m_mediaPlayer);
        const auto savedTrack = subtitleRoles().first; QVERIFY(!savedTrack.isEmpty());
        const auto other = QUrl::fromLocalFile(QFileInfo(m_fixture).absolutePath() + "/duration-cues.srt").toEncoded();
        QCOMPARE(libvlc_media_player_add_slave(m_mediaPlayer, libvlc_media_slave_type_subtitle, other.constData(), false), 0);
        auto trackCount = [&] {
            vlc_player_Lock(player); const auto count = vlc_player_GetTrackCount(player, SPU_ES); vlc_player_Unlock(player); return count;
        };
        QTRY_COMPARE_WITH_TIMEOUT(trackCount(), 2u, 5000);
        vlc_player_Lock(player);
        auto *second = vlc_es_id_Hold(vlc_player_GetTrackAt(player, SPU_ES, 1)->es_id);
        const QString secondId = QString::fromUtf8(vlc_es_id_GetStrId(second));
        vlc_es_id_t *list[] = {second, nullptr};
        const auto selected = vlc_player_SelectEsIdList(player, SPU_ES, list);
        vlc_player_Unlock(player); vlc_es_id_Release(second);
        QCOMPARE(selected, 1u);
        holdCue();
        QTRY_VERIFY_WITH_TIMEOUT(subtitleRoles() == qMakePair(secondId, QString{}), 5000);
        QTRY_VERIFY(m_presets->edited());
        QVERIFY(m_presets->apply(m_savedId));
        holdCue();
        QTRY_VERIFY_WITH_TIMEOUT(subtitleRoles() == qMakePair(savedTrack, QString{}) && !m_presets->restoring(), 5000);
        QTRY_VERIFY_WITH_TIMEOUT(!colored(frame(), true).isEmpty(), 5000);
        QVERIFY(!m_presets->edited()); QCOMPARE(delay(), qint64(350000));
    }
    void dualTrackOutputRolesAndLiveDurationCommands()
    {
        auto *player = vlc_test_media_player_core(m_mediaPlayer);
        vlc_player_Lock(player);
        const size_t count = vlc_player_GetTrackCount(player, SPU_ES);
        vlc_es_id_t *first = count > 0 ? vlc_es_id_Hold(vlc_player_GetTrackAt(player, SPU_ES, 0)->es_id) : nullptr;
        vlc_es_id_t *second = count > 1 ? vlc_es_id_Hold(vlc_player_GetTrackAt(player, SPU_ES, 1)->es_id) : nullptr;
        vlc_player_Unlock(player);
        QVERIFY(first); QVERIFY(second);
        const QString firstId = QString::fromUtf8(vlc_es_id_GetStrId(first));
        const QString secondId = QString::fromUtf8(vlc_es_id_GetStrId(second));
        auto select = [&](vlc_es_id_t *primary, vlc_es_id_t *secondary) {
            vlc_es_id_t *list[] = {primary, secondary, nullptr};
            vlc_player_Lock(player);
            // The native core retains roles of already selected ESs. Queue
            // deselection first so this establishes a real reversed role.
            vlc_player_UnselectTrackCategory(player, SPU_ES);
            const auto selected = vlc_player_SelectEsIdList(player, SPU_ES, list);
            vlc_player_Unlock(player);
            return selected;
        };
        QCOMPARE(select(second, first), 2u);
        holdCue();
        QTRY_VERIFY_WITH_TIMEOUT(subtitleRoles() == qMakePair(secondId, firstId), 5000);
        QVERIFY(m_presets->save("Dual subtitle roles")); const auto dual = m_presets->selectedId();
        QCOMPARE(select(first, second), 2u);
        holdCue();
        QTRY_VERIFY_WITH_TIMEOUT(subtitleRoles() == qMakePair(firstId, secondId), 5000);
        QTRY_VERIFY(m_presets->edited());
        QVERIFY(m_presets->apply(dual));
        holdCue();
        QTRY_VERIFY_WITH_TIMEOUT(subtitleRoles() == qMakePair(secondId, firstId) && !m_presets->restoring(), 5000);
        QVERIFY(!m_presets->edited());
        vlc_es_id_Release(first); vlc_es_id_Release(second);
        QVERIFY(m_presets->apply(m_savedId)); holdCue(); QTRY_VERIFY(!m_presets->restoring());
        // Initial inline options are real configuration. A later native command
        // becomes authoritative even if it writes the mapping's existing value.
        var_SetString(m_output, "sub-filter", "subsdelay{mode=0,factor=1.25,overlap=3}");
        QTest::qWait(200);
        QCOMPARE(m_style->durationFactor(), 1.25f); QCOMPARE(m_style->durationUnit(), QString(" s"));
        const float previousMapping = var_GetFloat(m_output, "subsdelay-factor");
        var_SetFloat(m_output, "subsdelay-factor", previousMapping);
        QTRY_COMPARE(m_style->durationFactor(), previousMapping);
        var_SetFloat(m_output, "subsdelay-factor", 3);
        var_SetInteger(m_output, "subsdelay-mode", 1);
        var_SetInteger(m_output, "subsdelay-overlap", 4);
        QTRY_VERIFY(m_presets->edited());
        SubtitleAppearance::Snapshot commanded; QVERIFY(m_style->snapshot(&commanded));
        QCOMPARE(commanded.durationFactor, 3.f); QCOMPARE(commanded.durationMode, 1); QCOMPARE(commanded.durationOverlap, 4);
        QVERIFY(m_presets->save("Native duration commands"));
        QVERIFY(m_presets->apply("default")); QTRY_VERIFY(!m_presets->restoring());
        QVERIFY(m_presets->apply(m_savedId)); QTRY_VERIFY(!m_presets->restoring());
        QCOMPARE(m_style->durationFactor(), 1.5f); QCOMPARE(m_style->durationUnit(), QString(" s"));
        SubtitleAppearance::Snapshot restored; QVERIFY(m_style->snapshot(&restored)); QVERIFY(restored == m_savedStyle);
    }
    void unavailableCorruptStorageAndWriteFailure()
    {
        const auto previous = m_presets->selectedId();
        QVERIFY(!m_presets->apply("deleted-id")); QCOMPARE(m_presets->selectedId(), previous);
        var_SetBool(m_output, "sub-text-style-supported", false);
        QVERIFY(!m_presets->canSave()); QVERIFY(!m_presets->apply("default"));
        QCOMPARE(m_presets->selectedId(), previous);
        var_SetBool(m_output, "sub-text-style-supported", true);
        const QString key = "Juxtopposed/SubtitlePresets/v1";
        QSettings damaged(m_temporary.filePath("damaged.ini"), QSettings::IniFormat);
        const QByteArray future("{\"schema\":2,\"presets\":[],\"media\":{}}");
        damaged.setValue(key, future); damaged.sync();
        {
            SubtitlePresetModel invalid(vlc_test_media_player_core(m_mediaPlayer), m_style.get(), &damaged,
                vlc_test_media_player_object(m_mediaPlayer));
            QVERIFY(!invalid.canSave()); QVERIFY(!invalid.error().isEmpty());
            QVERIFY(!invalid.save("Must not overwrite future data"));
        }
        QCOMPARE(damaged.value(key).toByteArray(), future);
        auto document = QJsonDocument::fromJson(m_settings->value(key).toByteArray()).object();
        auto records = document["presets"].toArray(); auto record = records[0].toObject();
        auto style = record["style"].toObject(); style["pixelSize"] = -1; record["style"] = style;
        records[0] = record; document["presets"] = records;
        damaged.setValue(key, QJsonDocument(document).toJson(QJsonDocument::Compact)); damaged.sync();
        SubtitlePresetModel invalidRange(vlc_test_media_player_core(m_mediaPlayer), m_style.get(), &damaged,
            vlc_test_media_player_object(m_mediaPlayer));
        QVERIFY(!invalidRange.canSave()); QVERIFY(!invalidRange.apply("default"));
        const QString blockedPath = m_temporary.filePath("blocked.ini"); QVERIFY(QDir().mkpath(blockedPath));
        QVERIFY(m_presets->apply("default")); QTRY_VERIFY(!m_presets->restoring());
        SubtitleAppearance::Snapshot beforeFailedSave; QVERIFY(m_style->snapshot(&beforeFailedSave));
        QVERIFY(!beforeFailedSave.enabled);
        QSettings blocked(blockedPath, QSettings::IniFormat);
        SubtitlePresetModel unwritable(vlc_test_media_player_core(m_mediaPlayer), m_style.get(), &blocked,
            vlc_test_media_player_object(m_mediaPlayer));
        QVERIFY(!unwritable.save("Write must fail")); QVERIFY(!unwritable.error().isEmpty());
        QCOMPARE(unwritable.entries().size(), 1); QVERIFY(!blocked.contains(key));
        SubtitleAppearance::Snapshot afterFailedSave; QVERIFY(m_style->snapshot(&afterFailedSave));
        QVERIFY(afterFailedSave == beforeFailedSave);
        // Retain both the service and QSettings after the rejected write.
        QVERIFY(QDir().rmdir(blockedPath));
        QVERIFY(unwritable.save("Retry same native service"));QVERIFY(unwritable.error().isEmpty());
        QCOMPARE(unwritable.entries().size(),2);
        QSettings recovered(blockedPath,QSettings::IniFormat);recovered.sync();
        const auto recoveredRecords=QJsonDocument::fromJson(recovered.value(key).toByteArray()).object().value("presets").toArray();
        QCOMPARE(recoveredRecords.size(),1);QCOMPARE(recoveredRecords[0].toObject().value("name").toString(),QString("Retry same native service"));
        QVERIFY(m_presets->apply(m_savedId)); QTRY_VERIFY(!m_presets->restoring());
    }
    void staleOutputAndResetDoNotResurrectPresets()
    {
        SubtitleAppearance::Snapshot current; QVERIFY(m_style->snapshot(&current));
        auto *other = input_item_New("file:///unrelated.mp4", "Unrelated"); QVERIFY(other);
        auto changed = current; changed.text = 0x00FF00;
        QVERIFY(!m_style->applySnapshot(changed, other)); input_item_Release(other);
        auto *player = vlc_test_media_player_core(m_mediaPlayer);
        vlc_player_Lock(player); auto *sameItem = input_item_Hold(vlc_player_GetCurrentMedia(player)); vlc_player_Unlock(player);
        std::atomic_uint64_t generation{2};
        QVERIFY(!m_style->applySnapshot(changed, sameItem, &generation, 1)); input_item_Release(sameItem);
        SubtitleAppearance::Snapshot unchanged; QVERIFY(m_style->snapshot(&unchanged)); QVERIFY(unchanged == current);
        m_style->resetOutput(nullptr); QVERIFY(!m_presets->canSave());
        QVERIFY(!m_presets->save("Detached output"));
        m_style->resetOutput(m_output);
        m_settings->clear(); m_settings->sync(); m_presets->reload();
        QCOMPARE(m_presets->entries().size(), 1);
        QTRY_VERIFY_WITH_TIMEOUT(!m_presets->restoring(), 5000);
        QTRY_VERIFY_WITH_TIMEOUT(!colored(frame(), false).isEmpty(), 5000);
        QVERIFY(!var_GetBool(m_output, "sub-text-style-enabled"));
        QCOMPARE(delay(), qint64(0)); QCOMPARE(selectedTrackDelay(), qint64(INT64_MAX));
        m_presets.reset(); m_settings->sync();
        QSettings restarted(m_settings->fileName(), QSettings::IniFormat);
        QVERIFY(!restarted.contains("Juxtopposed/SubtitlePresets/v1"));
    }
    void cleanupTestCase()
    {
        closePlayer(); m_settings.reset();
        if (m_instance) { libvlc_release(m_instance); m_instance = nullptr; }
    }
};
QTEST_GUILESS_MAIN(TestSubtitlePresets)
#include "test_subtitle_presets.moc"
