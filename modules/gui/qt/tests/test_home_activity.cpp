// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
// Backend evidence uses the production store/snapshot with real VLC ML.
// Completion counts are changed with native Mark As Played; this does not
// claim a player listening journey or a native Qt Home window test.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "medialibrary/mlhomeactivitydata.hpp"
#include "medialibrary/mlhelper.hpp"
#include <vlc/vlc.h>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>
#include <limits>
#include <functional>

extern "C" vlc_medialibrary_t *vlc_test_stream_library(libvlc_instance_t *);

class TestHomeActivity : public QObject
{
    Q_OBJECT
    libvlc_instance_t *m_instance = nullptr;
    vlc_medialibrary_t *m_library = nullptr;
    QString m_fixtures, m_settingsPath;
    QList<MLHomeVisitStore::Visit> m_visits;
    int64_t m_playlistId = 0, m_folderId = 0;
    libvlc_instance_t *instance()
    {
        const char *args[] = {"--ignore-config", "--media-library", "--no-auto-preparse",
            "--no-video", "--aout=dummy", "--verbose=2", "--file-logging", "--logfile=home-activity-native.log"};
        return libvlc_new(ARRAY_SIZE(args), args);
    }
    int64_t media(const char *file)
    {
        const auto uri = QUrl::fromLocalFile(QDir(m_fixtures).filePath(QString::fromUtf8(file))).toEncoded();
        ml_unique_ptr<vlc_ml_media_t> result(vlc_ml_get_media_by_mrl(m_library, uri.constData()));
        return result ? result->i_id : 0;
    }
    QVariantMap named(const QVariantList &rows, const QString &name)
    {
        for (const auto &row : rows)
            if (row.toMap().value("title").toString() == name)
                return row.toMap();
        return {};
    }
private slots:
    void initTestCase()
    {
        m_fixtures = qEnvironmentVariable("VLC_HOME_FIXTURE_DIR");
        const auto userdata = qEnvironmentVariable("VLC_USERDATA_PATH");
        QVERIFY2(QDir(m_fixtures).exists() && !m_fixtures.isEmpty(), "Use generated Home fixture directory");
        QVERIFY2(!userdata.isEmpty(), "Use fresh isolated VLC_USERDATA_PATH");
        m_settingsPath = QDir(userdata).filePath("home-visits.ini");
        m_instance = instance();
        QVERIFY(m_instance);
        m_library = vlc_test_stream_library(m_instance);
        QVERIFY(m_library);
        QCOMPARE(vlc_ml_count_media(m_library, nullptr), size_t(0));
        QCOMPARE(vlc_ml_count_playlists(m_library, nullptr, VLC_ML_PLAYLIST_TYPE_ALL), size_t(0));
    }
    void persistentVisitTypesAndLifetime()
    {
        QTemporaryDir scratch;
        QVERIFY(scratch.isValid());
        const auto path = scratch.filePath("settings.ini");
        {
            QSettings settings(path, QSettings::IniFormat);
            MLHomeVisitStore store(&settings);
            QVERIFY(store.visits().isEmpty());
            QVERIFY(!store.record(MLItemId(0, VLC_ML_PARENT_FOLDER), 100));
            QVERIFY(!store.record(MLItemId(7, VLC_ML_PARENT_ARTIST), 100));
            QVERIFY(!store.record(MLItemId(7, VLC_ML_PARENT_FOLDER), -1));
            QVERIFY(store.record(MLItemId(7, VLC_ML_PARENT_FOLDER), 100));
            QVERIFY(store.record(MLItemId(7, VLC_ML_PARENT_PLAYLIST), 200));
            QVERIFY(store.record(MLItemId(7, VLC_ML_PARENT_FOLDER), 50));
        }
        {
            QSettings settings(path, QSettings::IniFormat);
            MLHomeVisitStore reopened(&settings);
            const auto visits = reopened.visits();
            QCOMPARE(visits.size(), 2);
            for (const auto &visit : visits)
            {
                QCOMPARE(visit.id.id, int64_t(7));
                if (visit.id.type == VLC_ML_PARENT_FOLDER)
                {
                    QCOMPARE(visit.count, quint64(2));
                    QCOMPARE(visit.lastVisit, qint64(100));
                }
                else
                {
                    QCOMPARE(visit.id.type, VLC_ML_PARENT_PLAYLIST);
                    QCOMPARE(visit.count, quint64(1));
                    QCOMPARE(visit.lastVisit, qint64(200));
                }
            }
        }
        auto settings = std::make_unique<QSettings>(scratch.filePath("lifetime.ini"), QSettings::IniFormat);
        MLHomeVisitStore detached(settings.get());
        settings.reset();
        QVERIFY(!detached.record(MLItemId(1, VLC_ML_PARENT_FOLDER), 100));
        QVERIFY(!detached.clear());
        QVERIFY(detached.visits().isEmpty());
        QVERIFY(!loadMLHomeActivity(nullptr, {}).error.isEmpty());
    }
    void malformedStoredVisits()
    {
        QTemporaryDir scratch;
        QSettings settings(scratch.filePath("malformed.ini"), QSettings::IniFormat);
        settings.setValue("Juxtopposed/home-visits-v1", QVariantList{
            QVariantMap{{"id", "VLC_ML_PARENT_FOLDER - 4"}, {"count", "bad"}, {"lastVisit", 10}},
            QVariantMap{{"id", "VLC_ML_PARENT_ARTIST - 4"}, {"count", 3}, {"lastVisit", 10}},
            QVariantMap{{"id", "VLC_ML_PARENT_FOLDER - 5"}, {"count", 0}, {"lastVisit", 10}},
            QVariantMap{{"id", "VLC_ML_PARENT_PLAYLIST - 6"}, {"count", quint64(3)}, {"lastVisit", qint64(20)}}});
        MLHomeVisitStore store(&settings);
        QCOMPARE(store.visits().size(), 1);
        QCOMPARE(store.visits().front().id, MLItemId(6, VLC_ML_PARENT_PLAYLIST));
        QVERIFY(store.clear());
        QVERIFY(store.visits().isEmpty());
        QSettings reopenedSettings(settings.fileName(), QSettings::IniFormat);
        MLHomeVisitStore reopened(&reopenedSettings);
        QVERIFY(reopened.visits().isEmpty());
    }
    void preferencesResetDoesNotRestoreCachedVisits()
    {
        QTemporaryDir scratch;
        QSettings settings(scratch.filePath("reset.ini"), QSettings::IniFormat);
        MLHomeVisitStore store(&settings);
        QVERIFY(store.record(MLItemId(7, VLC_ML_PARENT_FOLDER), 100));
        // The live preferences reset clears QSettings before MainCtx asks
        // the activity service to clear its cache and pending visit epoch.
        settings.clear();
        QVERIFY(store.clear());
        QVERIFY(store.record(MLItemId(8, VLC_ML_PARENT_PLAYLIST), 200));
        QSettings reopenedSettings(settings.fileName(), QSettings::IniFormat);
        MLHomeVisitStore reopened(&reopenedSettings);
        QCOMPARE(reopened.visits().size(), 1);
        QCOMPARE(reopened.visits().front().id, MLItemId(8, VLC_ML_PARENT_PLAYLIST));
    }
    void realFolderPlaylistAndArtistMetadata()
    {
        const auto uri = QUrl::fromLocalFile(m_fixtures).toEncoded();
        QCOMPARE(vlc_ml_add_folder(m_library, uri.constData()), VLC_SUCCESS);
        QTRY_COMPARE_WITH_TIMEOUT(vlc_ml_count_audio_media(m_library, nullptr), size_t(4), 45000);
        QTRY_COMPARE_WITH_TIMEOUT(vlc_ml_count_video_media(m_library, nullptr), size_t(1), 45000);
        const auto a1 = media("alpha-one.flac"), a2 = media("alpha-two.flac"), b1 = media("beta-one.flac"), b2 = media("beta-two.flac");
        QVERIFY(a1 && a2 && b1 && b2);
        ml_unique_ptr<vlc_ml_folder_list_t> folders(vlc_ml_list_folders(m_library, nullptr));
        QVERIFY(folders);
        for (const auto &folder : ml_range_iterate<vlc_ml_folder_t>(folders))
            if (folder.psz_mrl && QUrl::fromEncoded(folder.psz_mrl).adjusted(QUrl::StripTrailingSlash) == QUrl::fromLocalFile(m_fixtures).adjusted(QUrl::StripTrailingSlash))
                m_folderId = folder.i_id;
        QVERIFY(m_folderId > 0);
        ml_unique_ptr<vlc_ml_playlist_t> playlist(vlc_ml_playlist_create(m_library, "Home Mixed Playlist"));
        QVERIFY(playlist);
        m_playlistId = playlist->i_id;
        const int64_t items[] = {media("home-video.mp4"), a1, b1};
        QCOMPARE(vlc_ml_playlist_append(m_library, m_playlistId, items, ARRAY_SIZE(items)), VLC_SUCCESS);
        QCOMPARE(vlc_ml_media_set_played(m_library, a1, true), VLC_SUCCESS);
        QCOMPARE(vlc_ml_media_set_played(m_library, a1, true), VLC_SUCCESS);
        QCOMPARE(vlc_ml_media_set_played(m_library, a2, true), VLC_SUCCESS);
        QCOMPARE(vlc_ml_media_set_played(m_library, b1, true), VLC_SUCCESS);
        QSettings settings(m_settingsPath, QSettings::IniFormat);
        MLHomeVisitStore store(&settings);
        QVERIFY(store.record(MLItemId(m_folderId, VLC_ML_PARENT_FOLDER), 100));
        QVERIFY(store.record(MLItemId(m_folderId, VLC_ML_PARENT_FOLDER), 150));
        QVERIFY(store.record(MLItemId(m_playlistId, VLC_ML_PARENT_PLAYLIST), 200));
        m_visits = store.visits();
        auto snapshot = loadMLHomeActivity(m_library, m_visits);
        QVERIFY2(snapshot.error.isEmpty(), qPrintable(snapshot.error));
        QCOMPARE(snapshot.mediaCount, 5);
        QCOMPARE(snapshot.folders.size(), 1);
        QCOMPARE(snapshot.playlists.size(), 1);
        QCOMPARE(snapshot.folders.front().toMap().value("id").value<MLItemId>(), MLItemId(m_folderId, VLC_ML_PARENT_FOLDER));
        QCOMPARE(snapshot.folders.front().toMap().value("activityCount").toULongLong(), quint64(2));
        QCOMPARE(snapshot.playlists.front().toMap().value("title").toString(), QString("Home Mixed Playlist"));
        QCOMPARE(snapshot.playlists.front().toMap().value("count").toUInt(), unsigned(3));
        QCOMPARE(snapshot.artists.size(), 2);
        QCOMPARE(snapshot.artists.front().toMap().value("title").toString(), QString("Home Artist Alpha"));
        QCOMPARE(named(snapshot.artists, "Home Artist Alpha").value("activityCount").toULongLong(), quint64(3));
        QCOMPARE(named(snapshot.artists, "Home Artist Beta").value("activityCount").toULongLong(), quint64(1));
        // Favorite state never creates a fake visit or listening event.
        QCOMPARE(vlc_ml_playlist_set_favorite(m_library, m_playlistId, true), VLC_SUCCESS);
        QVERIFY(loadMLHomeActivity(m_library, {}).playlists.isEmpty());
        QCOMPARE(vlc_ml_media_set_played(m_library, b1, false), VLC_SUCCESS);
        snapshot = loadMLHomeActivity(m_library, m_visits);
        QCOMPARE(snapshot.artists.size(), 1);
    }
    void warmRestartAndDeletedPlaylist()
    {
        libvlc_release(m_instance);
        m_instance = instance();
        QVERIFY(m_instance);
        m_library = vlc_test_stream_library(m_instance);
        QSettings settings(m_settingsPath, QSettings::IniFormat);
        MLHomeVisitStore reopened(&settings);
        auto snapshot = loadMLHomeActivity(m_library, reopened.visits());
        QVERIFY(snapshot.error.isEmpty());
        QCOMPARE(snapshot.folders.size(), 1);
        QCOMPARE(snapshot.playlists.size(), 1);
        QCOMPARE(snapshot.artists.size(), 1);
        QCOMPARE(snapshot.artists.front().toMap().value("activityCount").toULongLong(), quint64(3));
        QCOMPARE(vlc_ml_playlist_delete(m_library, m_playlistId), VLC_SUCCESS);
        snapshot = loadMLHomeActivity(m_library, reopened.visits());
        QVERIFY(snapshot.playlists.isEmpty());
        QCOMPARE(snapshot.folders.size(), 1);
    }
    void blockedStoragePreservesAcceptedVisitsAndDocument()
    {
        QTemporaryDir scratch; QVERIFY(scratch.isValid());
        QDir root(scratch.path()); QVERIFY(root.mkdir("storage"));
        const auto path = root.filePath("storage/visits.ini");
        const auto retained = root.filePath("accepted/visits.ini");
        QByteArray acceptedBytes;
        {
            QSettings settings(path, QSettings::IniFormat);
            settings.setValue("unrelated-preference", "kept");
            MLHomeVisitStore store(&settings);
            const MLItemId accepted(7, VLC_ML_PARENT_FOLDER);
            QVERIFY(store.record(accepted, 100));
            const auto previous = settings.value("Juxtopposed/home-visits-v1");
            { QFile file(path); QVERIFY(file.open(QIODevice::ReadOnly)); acceptedBytes = file.readAll(); }
            QVERIFY(!acceptedBytes.isEmpty());
            // A file occupies the former parent-directory path. This reliably
            // prevents actual backend writes even under an elevated account.
            // The original accepted document is retained byte-for-byte nearby.
            QVERIFY(root.rename("storage", "accepted"));
            { QFile blocker(root.filePath("storage")); QVERIFY(blocker.open(QIODevice::WriteOnly));
              QCOMPARE(blocker.write("blocked parent"), qint64(14)); }
            QVERIFY(!store.record(MLItemId(8, VLC_ML_PARENT_PLAYLIST), 200));
            QCOMPARE(settings.status(), QSettings::AccessError);
            QCOMPARE(store.visits().size(), 1);
            QCOMPARE(store.visits().front().id, accepted);
            QCOMPARE(store.visits().front().count, quint64(1));
            QCOMPARE(store.visits().front().lastVisit, qint64(100));
            QCOMPARE(settings.value("Juxtopposed/home-visits-v1"), previous);
            QVERIFY(!store.clear()); // errored backend rejects clear without changing the document
            QCOMPARE(store.visits().size(), 1);
            QCOMPARE(settings.value("Juxtopposed/home-visits-v1"), previous);
            { QFile file(retained); QVERIFY(file.open(QIODevice::ReadOnly)); QCOMPARE(file.readAll(), acceptedBytes); }
            QVERIFY(QFile::remove(root.filePath("storage")));
            QVERIFY(root.rename("accepted", "storage"));
            // QSettings keeps the first error status. Do not submit a new
            // candidate that could reach disk yet still be reported rejected.
            QVERIFY(!store.record(MLItemId(9, VLC_ML_PARENT_FOLDER), 300));
            QCOMPARE(settings.value("Juxtopposed/home-visits-v1"), previous);
            // QSettings may flush its pending values on destruction now that
            // storage works; rollback must have removed every failed candidate.
        }
        QSettings restarted(path, QSettings::IniFormat);
        QCOMPARE(restarted.value("unrelated-preference").toString(), QString("kept"));
        MLHomeVisitStore reopened(&restarted);
        QCOMPARE(reopened.visits().size(), 1);
        QCOMPARE(reopened.visits().front().id, MLItemId(7, VLC_ML_PARENT_FOLDER));
        QCOMPARE(reopened.visits().front().count, quint64(1));
        { QFile file(path); QVERIFY(file.open(QIODevice::ReadOnly)); QCOMPARE(file.readAll(), acceptedBytes); }
    }
    void blockedClearPreservesAcceptedVisitsAndDocument()
    {
        QTemporaryDir scratch; QVERIFY(scratch.isValid());
        QDir root(scratch.path()); QVERIFY(root.mkdir("storage"));
        const auto path = root.filePath("storage/visits.ini");
        {
            QSettings settings(path, QSettings::IniFormat);
            MLHomeVisitStore store(&settings);
            QVERIFY(store.record(MLItemId(7, VLC_ML_PARENT_FOLDER), 100));
            const auto previous = settings.value("Juxtopposed/home-visits-v1");
            QVERIFY(root.rename("storage", "accepted"));
            { QFile blocker(root.filePath("storage")); QVERIFY(blocker.open(QIODevice::WriteOnly)); }
            QVERIFY(!store.clear()); // this sync itself is the first real failure
            QCOMPARE(settings.status(), QSettings::AccessError);
            QCOMPARE(store.visits().size(), 1);
            QCOMPARE(settings.value("Juxtopposed/home-visits-v1"), previous);
            QVERIFY(QFile::remove(root.filePath("storage")));
            QVERIFY(root.rename("accepted", "storage"));
        }
        QSettings restarted(path, QSettings::IniFormat);
        MLHomeVisitStore reopened(&restarted);
        QCOMPARE(reopened.visits().size(), 1);
        QCOMPARE(reopened.visits().front().id, MLItemId(7, VLC_ML_PARENT_FOLDER));
    }
    void addedFolderRequestStateAndReset()
    {
        MLHomeFolderAdditions pending;
        const QUrl folder("file:///C:/real%20folder/");
        QVERIFY(mlHomeFolderKey(folder) == mlHomeFolderKey(QUrl("file:///C:/real%20folder")));
        QVERIFY(mlHomeFolderKey(QUrl("https://example.invalid/folder")).isEmpty());
        const auto first = pending.begin(folder); QVERIFY(first);
        QCOMPARE(pending.begin(folder), first); // in-flight duplicate is coalesced
        pending.accepted(first, true, false);
        QVERIFY(pending.ready().isEmpty()); // accepted scheduling is not indexing
        pending.rootAdded(folder, true);
        QCOMPARE(pending.ready().size(), 1);
        QVERIFY(!pending.ready().front().deadline.hasExpired());
        const auto oldRequest = pending.ready().front();
        pending.clear(); // Reset Preferences/clear history invalidates callbacks
        QVERIFY(!pending.current(oldRequest));
        QVERIFY(!pending.consume(oldRequest));
        const auto newer = pending.begin(folder); QVERIFY(newer != first);
        pending.accepted(first, true, true); QVERIFY(pending.ready().isEmpty());
        pending.accepted(newer, false, false); QVERIFY(pending.empty());
        const auto existing = pending.begin(folder);
        pending.accepted(existing, true, true); QCOMPARE(pending.ready().size(), 1);
        QVERIFY(pending.consume(pending.ready().front())); QVERIFY(pending.empty());
        pending.rootAdded(folder, true); QVERIFY(pending.empty()); // no unsolicited visit
        const auto failed = pending.begin(folder);
        pending.accepted(failed, true, false);
        pending.rootAdded(folder, false); QVERIFY(pending.empty());
    }
    void actualAudioOnlyAddedFolderAndMixedRouteRestart()
    {
        QTemporaryDir imported; QVERIFY(imported.isValid());
        const auto audioPath = imported.path() + "/Audio only Unicode ä # percent %";
        QVERIFY(QDir().mkpath(audioPath));
        QVERIFY(QFile::copy(QDir(m_fixtures).filePath("alpha-one.flac"), audioPath + "/song.flac"));
        const QUrl audioUri = QUrl::fromLocalFile(audioPath);
        QCOMPARE(lookupMLHomeAddedFolder(m_library, audioUri).status, MLHomeFolderLookup::Missing);
        QCOMPARE(lookupMLHomeAddedFolder(nullptr, audioUri).status, MLHomeFolderLookup::Error);
        const auto settingsFile = imported.filePath("bootstrap.ini");
        QSettings settings(settingsFile, QSettings::IniFormat);
        MLHomeVisitStore store(&settings); MLHomeFolderAdditions pending;
        struct RootEvents { QObject receiver; MLHomeFolderAdditions *pending; } events;
        events.pending = &pending;
        // A real VLC event crosses to the same Qt-thread request state used
        // by MLHomeActivity. The fixture never fabricates native success.
        auto *callback = vlc_ml_event_register_callback(m_library,
            [](void *opaque, const vlc_ml_event_t *event) {
                if (event->i_type != VLC_ML_EVENT_ENTRY_POINT_ADDED) return;
                auto *events = static_cast<RootEvents *>(opaque);
                const auto folder = QUrl::fromEncoded(event->entry_point_added.psz_entry_point);
                const bool success = event->entry_point_added.b_success;
                QMetaObject::invokeMethod(&events->receiver, [events, folder, success] {
                    events->pending->rootAdded(folder, success);
                }, Qt::QueuedConnection);
            }, &events);
        QVERIFY(callback);
        std::unique_ptr<vlc_ml_event_callback_t, std::function<void(vlc_ml_event_callback_t *)>> listener(
            callback, [library = m_library](vlc_ml_event_callback_t *cb) { vlc_ml_event_unregister_callback(library, cb); });
        const auto serial = pending.begin(audioUri);
        QCOMPARE(vlc_ml_add_folder(m_library, audioUri.toEncoded().constData()), VLC_SUCCESS);
        pending.accepted(serial, true, false);
        QVERIFY(pending.ready().isEmpty()); QVERIFY(store.visits().isEmpty());
        QTRY_VERIFY_WITH_TIMEOUT(lookupMLHomeAddedFolder(m_library, audioUri).status == MLHomeFolderLookup::Ready, 45000);
        const auto nativeFolder = lookupMLHomeAddedFolder(m_library, audioUri);
        QVERIFY(nativeFolder.id.id > 0); QCOMPARE(nativeFolder.id.type, VLC_ML_PARENT_FOLDER);
        auto folderAudioCount = [&] {
            return vlc_ml_count_media_of(m_library, nullptr, VLC_ML_PARENT_FOLDER, nativeFolder.id.id);
        };
        QTRY_COMPARE_WITH_TIMEOUT(folderAudioCount(), size_t(1), 45000);
        QTRY_COMPARE_WITH_TIMEOUT(pending.ready().size(), 1, 45000);
        QVERIFY(pending.consume(pending.ready().front()));
        QVERIFY(store.record(nativeFolder.id, 100));
        auto snapshot = loadMLHomeActivity(m_library, store.visits());
        QCOMPARE(snapshot.folders.size(), 1);
        QCOMPARE(snapshot.folders.front().toMap().value("id").value<MLItemId>(), nativeFolder.id);
        {
            ml_unique_ptr<vlc_ml_media_list_t> media(vlc_ml_list_media_of(m_library, nullptr, VLC_ML_PARENT_FOLDER, nativeFolder.id.id));
            QVERIFY(media); QCOMPARE(media->i_nb_items, size_t(1));
            QCOMPARE(media->p_items[0].i_type, VLC_ML_MEDIA_TYPE_AUDIO);
            ml_unique_ptr<vlc_ml_folder_list_t> videos(vlc_ml_list_folders_by_type(m_library, nullptr, VLC_ML_MEDIA_TYPE_VIDEO));
            QVERIFY(videos);
            for (const auto &folder : ml_range_iterate<vlc_ml_folder_t>(videos)) QVERIFY(folder.i_id != nativeFolder.id.id);
        }
        const auto resetPath = imported.filePath("reset-before-index");
        QVERIFY(QDir().mkpath(resetPath));
        QVERIFY(QFile::copy(QDir(m_fixtures).filePath("alpha-one.flac"), resetPath + "/song.flac"));
        const auto resetUri = QUrl::fromLocalFile(resetPath);
        const auto resetSerial = pending.begin(resetUri);
        QCOMPARE(vlc_ml_add_folder(m_library, resetUri.toEncoded().constData()), VLC_SUCCESS);
        pending.accepted(resetSerial, true, false);
        pending.clear(); QVERIFY(store.clear()); // clear before queued indexing success
        QTRY_VERIFY_WITH_TIMEOUT(lookupMLHomeAddedFolder(m_library, resetUri).status == MLHomeFolderLookup::Ready, 45000);
        QCoreApplication::processEvents();
        QVERIFY(pending.empty()); QVERIFY(store.visits().isEmpty());
        QVERIFY(store.record(nativeFolder.id, 100));
        const QUrl mixedUri = QUrl::fromLocalFile(m_fixtures);
        const auto mixed = lookupMLHomeAddedFolder(m_library, mixedUri);
        QCOMPARE(mixed.status, MLHomeFolderLookup::Ready);
        const auto mixedSerial = pending.begin(mixedUri);
        pending.accepted(mixedSerial, true, true); // existing roots do not repeat root-added
        QVERIFY(pending.consume(pending.ready().front()));
        QVERIFY(store.record(mixed.id, 200));
        {
            ml_unique_ptr<vlc_ml_media_list_t> mixedMedia(vlc_ml_list_media_of(m_library, nullptr, VLC_ML_PARENT_FOLDER, mixed.id.id));
            QVERIFY(mixedMedia); QCOMPARE(mixedMedia->i_nb_items, size_t(5));
            bool audio = false, video = false;
            for (const auto &item : ml_range_iterate<vlc_ml_media_t>(mixedMedia))
            { audio |= item.i_type == VLC_ML_MEDIA_TYPE_AUDIO; video |= item.i_type == VLC_ML_MEDIA_TYPE_VIDEO; }
            QVERIFY(audio && video); // the actual Home folder parent API supports mixed content
        }
        settings.sync();
        listener.reset(); // unregister against the old library before restart
        libvlc_release(m_instance); m_instance = instance(); QVERIFY(m_instance);
        m_library = vlc_test_stream_library(m_instance); QVERIFY(m_library);
        QSettings reopenedSettings(settingsFile, QSettings::IniFormat);
        MLHomeVisitStore reopened(&reopenedSettings);
        snapshot = loadMLHomeActivity(m_library, reopened.visits());
        QCOMPARE(snapshot.folders.size(), 2);
        QVERIFY(named(snapshot.folders, QFileInfo(audioPath).fileName()).value("available").toBool());
        QCOMPARE(lookupMLHomeAddedFolder(m_library, audioUri).id, nativeFolder.id);
        const auto missing = QUrl::fromLocalFile(imported.filePath("not-indexed"));
        QCOMPARE(lookupMLHomeAddedFolder(m_library, missing).status, MLHomeFolderLookup::Missing);
        QCOMPARE(vlc_ml_ban_folder(m_library, audioUri.toEncoded().constData()), VLC_SUCCESS);
        QTRY_VERIFY_WITH_TIMEOUT(lookupMLHomeAddedFolder(m_library, audioUri).status == MLHomeFolderLookup::Unavailable, 45000);
        QVERIFY(named(loadMLHomeActivity(m_library, reopened.visits()).folders,
                      QFileInfo(audioPath).fileName()).isEmpty());
        QVERIFY(store.clear()); pending.clear();
        QSettings clearedSettings(settingsFile, QSettings::IniFormat);
        QVERIFY(MLHomeVisitStore(&clearedSettings).visits().isEmpty());
    }
    void cleanupTestCase()
    {
        if (m_instance)
            libvlc_release(m_instance);
    }
};
QTEST_GUILESS_MAIN(TestHomeActivity)
#include "test_home_activity.moc"
