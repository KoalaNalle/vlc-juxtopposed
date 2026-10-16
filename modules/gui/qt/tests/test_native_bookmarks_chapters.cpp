// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
// Production models/controller on real local ML and an owned NORMAL playlist.
// No supplied title list, fabricated database IDs or executable static modules.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "maininterface/compositor.hpp"
#include "medialibrary/medialib.hpp"
#include "medialibrary/mlbookmarkmodel.hpp"
#include "medialibrary/mlhelper.hpp"
#include "player/player_controller.hpp"
#include <vlc/vlc.h>
#include <vlc_input_item.h>
#include <vlc_playlist.h>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QPointer>
#include <QSettings>
#include <QSignalSpy>
#include <QTest>
#include <QThread>
#include <atomic>
#include <cmath>
#include <memory>

// Production controller msg_* helpers need a module name in a standalone test.
extern "C" const char vlc_module_name[] = "native_bookmarks_chapters";

extern "C" vlc_object_t *vlc_test_native_instance_object(libvlc_instance_t *);
extern "C" vlc_playlist_t *vlc_test_stream_playlist(libvlc_instance_t *);
extern "C" vlc_medialibrary_t *vlc_test_stream_library(libvlc_instance_t *);

class TestNativeBookmarksChapters : public QObject
{
    Q_OBJECT
    libvlc_instance_t *m_instance = nullptr;
    vlc_medialibrary_t *m_nativeMl = nullptr;
    vlc_playlist_t *m_playlist = nullptr;
    vlc_player_t *m_player = nullptr;
    qt_intf_t *m_context = nullptr;
    vlc_player_timer_id *m_timer = nullptr;
    std::unique_ptr<MediaLib> m_ml;
    std::unique_ptr<PlayerController> m_controller;
    std::unique_ptr<MLBookmarkModel> m_bookmarks;
    std::unique_ptr<QSettings> m_settings;
    QString m_directory, m_chapterFile, m_plainFile, m_lateFile;
    int64_t m_chapterId = 0, m_plainId = 0;
    qint64 m_bookmarkTime = -1;
    std::atomic_uint m_requests{0}, m_completions{0};
    std::atomic<vlc_tick_t> m_requestedTime{VLC_TICK_INVALID};
    int m_modelNotifications = 0;
    unsigned m_instanceSerial = 0;

    static void onUpdate(const vlc_player_timer_point *, void *) {}
    static void onSeek(const vlc_player_timer_point *point, void *opaque)
    {
        auto *self = static_cast<TestNativeBookmarksChapters *>(opaque);
        if (point)
        {
            self->m_requestedTime.store(point->ts, std::memory_order_relaxed);
            self->m_requests.fetch_add(1, std::memory_order_release);
        }
        else self->m_completions.fetch_add(1, std::memory_order_release);
    }
    vlc_player_state state() const
    {
        vlc_player_locker lock{m_player};
        return vlc_player_GetState(m_player);
    }
    vlc_tick_t time() const
    {
        vlc_player_locker lock{m_player};
        return vlc_player_GetTime(m_player);
    }
    ssize_t chapter() const
    {
        vlc_player_locker lock{m_player};
        return vlc_player_GetSelectedChapterIdx(m_player);
    }
    QString currentUri() const
    {
        vlc_player_locker lock{m_player};
        auto *item = vlc_player_GetCurrentMedia(m_player);
        return item ? QString::fromUtf8(item->psz_uri) : QString{};
    }
    bool start(const QString &file)
    {
        const auto uri = QUrl::fromLocalFile(file).toEncoded();
        auto *item = input_item_New(uri.constData(), nullptr);
        if (!item) return false;
        // Deferred native replacements start through input_Start directly;
        // configure this owned input, as well as the player, to start paused.
        if (input_item_AddOption(item, ":start-paused", VLC_INPUT_OPTION_TRUSTED) != VLC_SUCCESS)
        { input_item_Release(item); return false; }
        int configured, started;
        {
            vlc_player_locker lock{m_player};
            vlc_player_SetStartPaused(m_player, true);
            configured = vlc_player_SetCurrentMedia(m_player, item);
            started = configured == VLC_SUCCESS ? vlc_player_Start(m_player) : VLC_EGENERIC;
        }
        input_item_Release(item);
        return configured == VLC_SUCCESS && started == VLC_SUCCESS;
    }
    bool waitNativeWithoutQt(const QString &file, vlc_player_state expected)
    {
        const auto uri = QUrl::fromLocalFile(file).toString(QUrl::FullyEncoded);
        QElapsedTimer deadline; deadline.start();
        while (deadline.elapsed() < 10000)
        {
            if (currentUri() == uri && state() == expected) return true;
            QThread::msleep(5); // Deliberately preserve old queued QObject state.
        }
        return false;
    }
    int64_t id(const QString &file)
    {
        const auto uri = QUrl::fromLocalFile(file).toEncoded();
        ml_unique_ptr<vlc_ml_media_t> media{vlc_ml_get_media_by_mrl(m_nativeMl, uri.constData())};
        return media ? media->i_id : 0;
    }
    qsizetype bookmarkCount(int64_t mediaId) const
    {
        ml_unique_ptr<vlc_ml_bookmark_list_t> rows{vlc_ml_list_media_bookmarks(m_nativeMl, nullptr, mediaId)};
        return rows ? qsizetype(rows->i_nb_items) : -1;
    }
    QString bookmarkName(int64_t mediaId, qint64 at) const
    {
        ml_unique_ptr<vlc_ml_bookmark_list_t> rows{vlc_ml_list_media_bookmarks(m_nativeMl, nullptr, mediaId)};
        if (rows) for (size_t i = 0; i < rows->i_nb_items; ++i)
            if (rows->p_items[i].i_time == at) return QString::fromUtf8(rows->p_items[i].psz_name);
        return {};
    }
    QModelIndex rowAt(qint64 at) const
    {
        if (m_bookmarks) for (int i = 0; i < m_bookmarks->rowCount(); ++i)
        {
            auto index = m_bookmarks->index(i, 0);
            if (m_bookmarks->data(index, MLBookmarkModel::TimeMsRole).toLongLong() == at) return index;
        }
        return {};
    }
    void observeModelUnlocked()
    {
        // A direct Qt observer re-locks the actual NORMAL player. Emitting
        // under that lock hangs, and the outer runner rejects the timeout.
        QCOMPARE(QThread::currentThread(), thread());
        (void)time();
        ++m_modelNotifications;
    }
    bool openContext()
    {
        const QByteArray logfile = QByteArray("--logfile=bookmarks-chapters-native-")
            + QByteArray::number(++m_instanceSerial) + ".log";
        const char *args[] = {"--ignore-config", "--media-library", "--no-auto-preparse",
            "--no-ml-network-discovery", "--no-video-title-show", "--vout=dummy",
            "--aout=dummy", "--text-renderer=freetype", "--verbose=2", "--file-logging",
            logfile.constData()};
        m_instance = libvlc_new(ARRAY_SIZE(args), args);
        if (!m_instance) return false;
        m_nativeMl = vlc_test_stream_library(m_instance);
        m_playlist = vlc_test_stream_playlist(m_instance); // Production NORMAL-lock player.
        if (!m_nativeMl || !m_playlist) return false;
        m_player = vlc_playlist_GetPlayer(m_playlist);
        m_context = vlc_object_create<qt_intf_t>(vlc_test_native_instance_object(m_instance));
        if (!m_context) return false;
        m_context->intf = nullptr;
        m_context->p_mi = nullptr;
        m_context->mainSettings = nullptr; // No unrelated Home service in this diagnostic.
        m_context->p_playlist = m_playlist;
        m_context->p_player = m_player;
        m_context->p_mainPlaylistController = nullptr;
        m_context->p_mainPlayerController = nullptr;
        m_context->isShuttingDown = false;
        m_context->b_isDialogProvider = false;
        m_ml = std::make_unique<MediaLib>(m_context, nullptr);
        m_context->mainSettings = m_settings.get();
        m_controller = std::make_unique<PlayerController>(m_context);
        m_context->p_mainPlayerController = m_controller.get();
        m_bookmarks = std::make_unique<MLBookmarkModel>();
        m_bookmarks->setMl(m_ml.get());
        m_bookmarks->setPlayer(m_controller.get());
        connect(m_bookmarks.get(), &MLBookmarkModel::canAddChanged,
            this, &TestNativeBookmarksChapters::observeModelUnlocked, Qt::DirectConnection);
        connect(m_bookmarks.get(), &QAbstractItemModel::modelReset,
            this, &TestNativeBookmarksChapters::observeModelUnlocked, Qt::DirectConnection);
        connect(m_controller->getChapters(), &QAbstractItemModel::dataChanged,
            this, &TestNativeBookmarksChapters::observeModelUnlocked, Qt::DirectConnection);
        static const vlc_player_timer_cbs callbacks = {&onUpdate, nullptr, &onSeek};
        m_timer = vlc_player_AddTimer(m_player, VLC_TICK_FROM_MS(10), &callbacks, this);
        return m_timer != nullptr;
    }
    void closeContext()
    {
        m_bookmarks.reset();
        if (m_player)
        {
            { vlc_player_locker lock{m_player}; vlc_player_Stop(m_player); }
            QElapsedTimer deadline; deadline.start();
            while (deadline.elapsed() < 5000 && state() != VLC_PLAYER_STATE_STOPPED) QTest::qWait(5);
            { vlc_player_locker lock{m_player}; if (m_timer) vlc_player_RemoveTimer(m_player, m_timer); }
        }
        m_timer = nullptr;
        m_controller.reset();
        m_ml.reset();
        // Destroying a settled MediaLib schedules its ThreadRunner deletion.
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QCoreApplication::processEvents();
        if (m_context) vlc_object_delete(m_context);
        m_context = nullptr;
        if (m_playlist) vlc_playlist_Delete(m_playlist);
        m_playlist = nullptr; m_player = nullptr;
        if (m_instance) libvlc_release(m_instance);
        m_instance = nullptr; m_nativeMl = nullptr;
    }
private slots:
    void initTestCase()
    {
        m_directory = qEnvironmentVariable("VLC_USERDATA_PATH");
        const QDir fixtures(qEnvironmentVariable("VLC_CHAPTER_FIXTURE_DIR"));
        QVERIFY2(!m_directory.isEmpty() && QDir(m_directory).exists(), "Use fresh isolated native userdata");
        QVERIFY(fixtures.exists("chaptered-colors.mkv") && fixtures.exists("paused-seek-colors.mp4"));
        const auto indexed = QDir(m_directory).filePath("indexed Unicode # %");
        QVERIFY(QDir().mkpath(indexed));
        m_chapterFile = QDir(indexed).filePath("chaptered-colors.mkv");
        m_plainFile = QDir(indexed).filePath("paused-seek-colors.mp4");
        m_lateFile = QDir(m_directory).filePath("late external video.mkv");
        QVERIFY(QFile::copy(fixtures.filePath("chaptered-colors.mkv"), m_chapterFile));
        QVERIFY(QFile::copy(fixtures.filePath("paused-seek-colors.mp4"), m_plainFile));
        QVERIFY(QFile::copy(fixtures.filePath("chaptered-colors.mkv"), m_lateFile));
        m_settings = std::make_unique<QSettings>(QDir(m_directory).filePath("native-settings.ini"), QSettings::IniFormat);
        qRegisterMetaType<VLCTime>(); qRegisterMetaType<VLCDuration>();
        QVERIFY(openContext());
        QCOMPARE(vlc_ml_count_media(m_nativeMl, nullptr), size_t(0));
        const auto uri = QUrl::fromLocalFile(indexed).toEncoded();
        QCOMPARE(vlc_ml_add_folder(m_nativeMl, uri.constData()), VLC_SUCCESS);
        QTRY_COMPARE_WITH_TIMEOUT(vlc_ml_count_video_media(m_nativeMl, nullptr), size_t(2), 45000);
        m_chapterId = id(m_chapterFile); m_plainId = id(m_plainFile);
        QVERIFY(m_chapterId > 0 && m_plainId > 0 && m_chapterId != m_plainId);
        QVERIFY(start(m_chapterFile));
        QTRY_COMPARE_WITH_TIMEOUT(m_controller->getPlayingState(), PlayerController::PLAYING_STATE_PAUSED, 10000);
        QTRY_VERIFY_WITH_TIMEOUT(m_bookmarks->canAdd(), 10000);
        QTRY_COMPARE_WITH_TIMEOUT(m_controller->getChapters()->rowCount(), 3, 10000);
    }
    void actualThreeChapterMetadataAndSelection()
    {
        auto *model = m_controller->getChapters();
        QSignalSpy changes(model, &QAbstractItemModel::dataChanged);
        const char *names[] = {"Red", "Blue", "Green"};
        for (int i = 0; i < 3; ++i)
        {
            const auto index = model->index(i, 0);
            QString nativeName;
            {
                vlc_player_locker lock{m_player};
                const auto *title = vlc_player_GetSelectedTitle(m_player);
                if (title && title->chapter_count == 3)
                    nativeName = QString::fromUtf8(title->chapters[i].name);
            }
            QVERIFY(!nativeName.isEmpty());
            // Native MKV metadata includes its authored hierarchy prefix.
            QCOMPARE(nativeName.trimmed(), QString::fromUtf8(names[i]));
            QCOMPARE(model->data(index, Qt::DisplayRole).toString(), nativeName);
            QCOMPARE(model->data(index, ChapterListModel::TimeRole).value<VLCTime>().toVLCTick(), VLC_TICK_FROM_SEC(i * 6));
            QVERIFY(std::abs(model->data(index, ChapterListModel::StartPositionRole).toDouble() - double(i) / 3) < .001);
            QVERIFY(std::abs(model->data(index, ChapterListModel::EndPositionRole).toDouble() - double(i + 1) / 3) < .001);
        }
        model->selectChapter(2); // No outer native lock.
        QTRY_COMPARE_WITH_TIMEOUT(chapter(), ssize_t(2), 5000);
        QTRY_VERIFY_WITH_TIMEOUT(std::abs(time() - VLC_TICK_FROM_SEC(12)) <= VLC_TICK_FROM_MS(50), 5000);
        QTRY_VERIFY(model->data(model->index(2, 0), Qt::CheckStateRole).toBool());
        QVERIFY(!changes.isEmpty());
        QCOMPARE(state(), VLC_PLAYER_STATE_PAUSED);
        QVERIFY(model->setData(model->index(1, 0), true, Qt::CheckStateRole));
        QTRY_COMPARE_WITH_TIMEOUT(chapter(), ssize_t(1), 5000);
        QTRY_VERIFY(model->data(model->index(1, 0), Qt::CheckStateRole).toBool());
        QVERIFY(!model->data(model->index(2, 0), Qt::CheckStateRole).toBool());
        m_controller->chapterPrev();
        QTRY_COMPARE_WITH_TIMEOUT(chapter(), ssize_t(0), 5000);
        m_controller->chapterNext();
        QTRY_COMPARE_WITH_TIMEOUT(chapter(), ssize_t(1), 5000);
        const auto before = m_requests.load();
        model->selectChapter(-1); model->selectChapter(3);
        QCOMPARE(m_requests.load(), before);
        QCOMPARE(currentUri(), QUrl::fromLocalFile(m_chapterFile).toString(QUrl::FullyEncoded));
    }
    void actualBookmarkAddRenameSeekAndRemove()
    {
        { vlc_player_locker lock{m_player}; vlc_player_SetTime(m_player, 4352555); }
        QTRY_VERIFY_WITH_TIMEOUT(std::abs(time() - 4352555) <= VLC_TICK_FROM_MS(50), 5000);
        m_bookmarkTime = MS_FROM_VLC_TICK(time());
        QVERIFY(m_bookmarkTime >= 4300 && m_bookmarkTime <= 4400);
        m_bookmarks->add();
        QTRY_COMPARE_WITH_TIMEOUT(bookmarkCount(m_chapterId), qsizetype(1), 5000);
        QTRY_VERIFY_WITH_TIMEOUT(rowAt(m_bookmarkTime).isValid(), 5000);
        const auto index = rowAt(m_bookmarkTime);
        QCOMPARE(m_bookmarks->data(index, MLBookmarkModel::TimeMsRole).toLongLong(), m_bookmarkTime);
        QCOMPARE(m_bookmarks->data(index, MLBookmarkModel::TimeRole).toString(), QString("00:04"));
        QVERIFY(std::abs(m_bookmarks->data(index, MLBookmarkModel::PositionRole).toDouble()
            - double(m_bookmarkTime) / 18000) < .005);
        const QString renamed = QString::fromUtf8("Chapter note — ää # %");
        QVERIFY(m_bookmarks->setData(index, renamed, MLBookmarkModel::NameRole));
        QTRY_COMPARE_WITH_TIMEOUT(bookmarkName(m_chapterId, m_bookmarkTime), renamed, 5000);
        QTRY_COMPARE_WITH_TIMEOUT(m_bookmarks->data(rowAt(m_bookmarkTime), MLBookmarkModel::NameRole).toString(), renamed, 5000);
        { vlc_player_locker lock{m_player}; vlc_player_SetTime(m_player, VLC_TICK_FROM_SEC(9)); }
        QTRY_VERIFY_WITH_TIMEOUT(std::abs(time() - VLC_TICK_FROM_SEC(9)) <= VLC_TICK_FROM_MS(50), 5000);
        const auto before = m_requests.load();
        m_bookmarks->select(rowAt(m_bookmarkTime));
        QCOMPARE(m_requests.load(), before + 1);
        QCOMPARE(m_requestedTime.load(), VLC_TICK_FROM_MS(m_bookmarkTime));
        QTRY_VERIFY_WITH_TIMEOUT(std::abs(time() - VLC_TICK_FROM_MS(m_bookmarkTime)) <= VLC_TICK_FROM_MS(50), 5000);
        QCOMPARE(state(), VLC_PLAYER_STATE_PAUSED);
        // Native selection includes all columns; removal must deduplicate.
        const auto row = rowAt(m_bookmarkTime).row();
        m_bookmarks->remove({m_bookmarks->index(row, 0), m_bookmarks->index(row, 1), m_bookmarks->index(row, 2)});
        QTRY_COMPARE_WITH_TIMEOUT(bookmarkCount(m_chapterId), qsizetype(0), 5000);
        QTRY_COMPARE_WITH_TIMEOUT(m_bookmarks->rowCount(), 0, 5000);
        QCOMPARE(bookmarkCount(m_plainId), qsizetype(0));
        QVERIFY(m_modelNotifications > 0);
    }
    void actualMediaSwitchRejectsOldBookmarksBeforeGuiDispatch()
    {
        { vlc_player_locker lock{m_player}; vlc_player_SetTime(m_player, VLC_TICK_FROM_SEC(5)); }
        QTRY_VERIFY_WITH_TIMEOUT(std::abs(time() - VLC_TICK_FROM_SEC(5)) <= VLC_TICK_FROM_MS(50), 5000);
        m_bookmarkTime = MS_FROM_VLC_TICK(time());
        m_bookmarks->add();
        QTRY_VERIFY_WITH_TIMEOUT(rowAt(m_bookmarkTime).isValid(), 5000);
        const auto stale = rowAt(m_bookmarkTime);
        const auto name = bookmarkName(m_chapterId, m_bookmarkTime);
        QVERIFY(start(m_plainFile));
        QVERIFY(waitNativeWithoutQt(m_plainFile, VLC_PLAYER_STATE_PAUSED));
        QVERIFY(!m_bookmarks->canAdd());
        const auto seeks = m_requests.load();
        m_bookmarks->add(); m_bookmarks->remove({stale}); m_bookmarks->clear(); m_bookmarks->select(stale);
        QVERIFY(!m_bookmarks->setData(stale, "Wrong media", MLBookmarkModel::NameRole));
        QCOMPARE(m_requests.load(), seeks);
        QCOMPARE(bookmarkCount(m_plainId), qsizetype(0));
        QCOMPARE(bookmarkName(m_chapterId, m_bookmarkTime), name);
        QTRY_VERIFY_WITH_TIMEOUT(m_bookmarks->canAdd(), 5000);
        QTRY_COMPARE_WITH_TIMEOUT(m_bookmarks->rowCount(), 0, 5000);
        QTRY_COMPARE_WITH_TIMEOUT(m_controller->getChapters()->rowCount(), 0, 5000);
        QVERIFY(!m_controller->hasChapters());
        QVERIFY(start(m_chapterFile));
        QTRY_COMPARE_WITH_TIMEOUT(m_controller->getPlayingState(), PlayerController::PLAYING_STATE_PAUSED, 10000);
        QTRY_COMPARE_WITH_TIMEOUT(m_controller->getChapters()->rowCount(), 3, 5000);
        QTRY_VERIFY_WITH_TIMEOUT(rowAt(m_bookmarkTime).isValid(), 5000);
    }
    void actualLateNativeMediaRegistrationRestoresAdd()
    {
        QCOMPARE(id(m_lateFile), int64_t(0));
        QVERIFY(start(m_lateFile));
        QVERIFY(waitNativeWithoutQt(m_lateFile, VLC_PLAYER_STATE_PAUSED));
        QTest::qWait(100);
        QVERIFY(!m_bookmarks->canAdd());
        const auto uri = QUrl::fromLocalFile(m_lateFile).toEncoded();
        ml_unique_ptr<vlc_ml_media_t> created{vlc_ml_new_external_media(m_nativeMl, uri.constData())};
        QVERIFY(created && created->i_id > 0);
        // This is real native external-media registration, not folder parsing.
        const auto lateId = created->i_id;
        QTRY_VERIFY_WITH_TIMEOUT(m_bookmarks->canAdd(), 5000);
        m_bookmarks->add();
        QTRY_COMPARE_WITH_TIMEOUT(bookmarkCount(lateId), qsizetype(1), 5000);
        QTRY_COMPARE_WITH_TIMEOUT(m_bookmarks->rowCount(), 1, 5000);
        m_bookmarks->clear();
        QTRY_COMPARE_WITH_TIMEOUT(bookmarkCount(lateId), qsizetype(0), 5000);
        QTRY_COMPARE_WITH_TIMEOUT(m_bookmarks->rowCount(), 0, 5000);
        QVERIFY(start(m_chapterFile));
        QTRY_COMPARE_WITH_TIMEOUT(m_controller->getPlayingState(), PlayerController::PLAYING_STATE_PAUSED, 10000);
        QTRY_VERIFY_WITH_TIMEOUT(rowAt(m_bookmarkTime).isValid(), 5000);
    }
    void actualStopSameMediaRestartRetainsBookmark()
    {
        const auto name = bookmarkName(m_chapterId, m_bookmarkTime);
        { vlc_player_locker lock{m_player}; vlc_player_Stop(m_player); }
        QTRY_COMPARE_WITH_TIMEOUT(state(), VLC_PLAYER_STATE_STOPPED, 5000);
        QTRY_COMPARE_WITH_TIMEOUT(m_bookmarks->rowCount(), 0, 5000);
        QCOMPARE(bookmarkName(m_chapterId, m_bookmarkTime), name);
        const auto before = bookmarkCount(m_chapterId);
        m_bookmarks->add(); // Invalid native clock must not create a bookmark.
        QTest::qWait(100);
        QCOMPARE(bookmarkCount(m_chapterId), before);
        {
            vlc_player_locker lock{m_player};
            vlc_player_SetStartPaused(m_player, true);
            QCOMPARE(vlc_player_Start(m_player), VLC_SUCCESS);
        }
        QTRY_COMPARE_WITH_TIMEOUT(m_controller->getPlayingState(), PlayerController::PLAYING_STATE_PAUSED, 10000);
        QTRY_VERIFY_WITH_TIMEOUT(rowAt(m_bookmarkTime).isValid(), 5000);
        QCOMPARE(m_bookmarks->data(rowAt(m_bookmarkTime), MLBookmarkModel::NameRole).toString(), name);
    }
    void actualNativeRestartPersistsBookmarkAndMetadata()
    {
        const auto name = bookmarkName(m_chapterId, m_bookmarkTime);
        QVERIFY(!name.isEmpty());
        closeContext();
        QVERIFY(openContext());
        QCOMPARE(id(m_chapterFile), m_chapterId);
        QCOMPARE(id(m_plainFile), m_plainId);
        QCOMPARE(bookmarkCount(m_chapterId), qsizetype(1));
        QCOMPARE(bookmarkName(m_chapterId, m_bookmarkTime), name);
        QVERIFY(start(m_chapterFile));
        QTRY_COMPARE_WITH_TIMEOUT(m_controller->getPlayingState(), PlayerController::PLAYING_STATE_PAUSED, 10000);
        QTRY_COMPARE_WITH_TIMEOUT(m_controller->getChapters()->rowCount(), 3, 5000);
        QTRY_VERIFY_WITH_TIMEOUT(rowAt(m_bookmarkTime).isValid(), 5000);
        const auto index = rowAt(m_bookmarkTime);
        QCOMPARE(m_bookmarks->data(index, MLBookmarkModel::NameRole).toString(), name);
        m_bookmarks->clear();
        QTRY_COMPARE_WITH_TIMEOUT(bookmarkCount(m_chapterId), qsizetype(0), 5000);
        QTRY_COMPARE_WITH_TIMEOUT(m_bookmarks->rowCount(), 0, 5000);
    }
    void actualModelDetachAndOwnerDestruction()
    {
        m_bookmarks->setMl(nullptr);
        QVERIFY(!m_bookmarks->canAdd());
        QCOMPARE(m_bookmarks->rowCount(), 0);
        m_bookmarks->setMl(m_ml.get());
        QTRY_VERIFY_WITH_TIMEOUT(m_bookmarks->canAdd(), 5000);
        auto *player = m_player;
        const auto before = currentUri();
        m_bookmarks.reset(); // Synchronizes/removes native player and ML listeners.
        QVERIFY(start(m_plainFile));
        QVERIFY(waitNativeWithoutQt(m_plainFile, VLC_PLAYER_STATE_PAUSED));
        QTest::qWait(100);
        QCOMPARE(m_player, player);
        QVERIFY(currentUri() != before);
        QCOMPARE(bookmarkCount(m_plainId), qsizetype(0));
        QTRY_COMPARE_WITH_TIMEOUT(m_controller->getChapters()->rowCount(), 0, 5000);
    }
    void cleanupTestCase() { closeContext(); m_settings.reset(); }
};
QTEST_GUILESS_MAIN(TestNativeBookmarksChapters)
#include "test_native_bookmarks_chapters.moc"
