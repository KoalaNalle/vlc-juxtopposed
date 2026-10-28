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
#include "medialibrary/mlvideohistory.hpp"
#include <vlc/vlc.h>
#include <vlc_playlist.h>
#include <vlc_player.h>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>
#include <limits>
#include <functional>
#include <atomic>
#include <algorithm>

extern "C" vlc_medialibrary_t *vlc_test_stream_library(libvlc_instance_t *);
extern "C" vlc_playlist_t *vlc_test_stream_playlist(libvlc_instance_t *);

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
            "--no-ml-network-discovery", "--no-metadata-network-access",
            "--no-video", "--vout=dummy", "--aout=dummy", "--verbose=2", "--file-logging", "--logfile=home-activity-native.log"};
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
            QCOMPARE(store.visits().size(), 1);
            QCOMPARE(store.visits().front().id, accepted);
            QCOMPARE(store.visits().front().count, quint64(1));
            QCOMPARE(store.visits().front().lastVisit, qint64(100));
            QVERIFY(!store.clear()); // errored backend rejects clear without changing the document
            QCOMPARE(store.visits().size(), 1);
            { QFile file(retained); QVERIFY(file.open(QIODevice::ReadOnly)); QCOMPARE(file.readAll(), acceptedBytes); }
            QVERIFY(QFile::remove(root.filePath("storage")));
            QVERIFY(root.rename("accepted", "storage"));
            // Sync reflects the deliberately missing file while blocked;
            // the store's accepted visits and retained file are authoritative.
            settings.sync();
            QCOMPARE(settings.value("Juxtopposed/home-visits-v1"), previous);
            // The production store uses a fresh checked session for retry;
            // the borrowed QSettings object's status is not the save result.
            QVERIFY(store.record(MLItemId(9, VLC_ML_PARENT_FOLDER), 300));
            QCOMPARE(store.visits().size(), 2);
            for (const auto &visit : store.visits())
            {
                QVERIFY(visit.id == accepted || visit.id == MLItemId(9, VLC_ML_PARENT_FOLDER));
                QCOMPARE(visit.count, quint64(1));
                QCOMPARE(visit.lastVisit, visit.id == accepted ? qint64(100) : qint64(300));
            }
        }
        QSettings restarted(path, QSettings::IniFormat);
        QCOMPARE(restarted.value("unrelated-preference").toString(), QString("kept"));
        MLHomeVisitStore reopened(&restarted);
        QCOMPARE(reopened.visits().size(), 2);
        QCOMPARE(reopened.visits().front().id, MLItemId(7, VLC_ML_PARENT_FOLDER));
        QCOMPARE(reopened.visits().front().count, quint64(1));
        QCOMPARE(reopened.visits().back().id, MLItemId(9, VLC_ML_PARENT_FOLDER));
        QCOMPARE(reopened.visits().back().lastVisit, qint64(300));
    }
    void blockedClearPreservesAcceptedVisitsAndDocument()
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
            QVERIFY(store.record(MLItemId(7, VLC_ML_PARENT_FOLDER), 100));
            const auto previous = settings.value("Juxtopposed/home-visits-v1");
            { QFile file(path); QVERIFY(file.open(QIODevice::ReadOnly)); acceptedBytes = file.readAll(); }
            QVERIFY(!acceptedBytes.isEmpty());
            QVERIFY(root.rename("storage", "accepted"));
            { QFile blocker(root.filePath("storage")); QVERIFY(blocker.open(QIODevice::WriteOnly)); }
            QVERIFY(!store.clear()); // this sync itself is the first real failure
            QCOMPARE(store.visits().size(), 1);
            QCOMPARE(store.visits().front().id, MLItemId(7, VLC_ML_PARENT_FOLDER));
            QCOMPARE(store.visits().front().count, quint64(1));
            QCOMPARE(store.visits().front().lastVisit, qint64(100));
            { QFile file(retained); QVERIFY(file.open(QIODevice::ReadOnly)); QCOMPARE(file.readAll(), acceptedBytes); }
            QVERIFY(QFile::remove(root.filePath("storage")));
            QVERIFY(root.rename("accepted", "storage"));
            settings.sync();
            QCOMPARE(settings.value("Juxtopposed/home-visits-v1"), previous);
            QVERIFY(store.clear());
            QVERIFY(store.visits().isEmpty());
            QVERIFY(!settings.contains("Juxtopposed/home-visits-v1"));
        }
        QSettings restarted(path, QSettings::IniFormat);
        MLHomeVisitStore reopened(&restarted);
        QVERIFY(reopened.visits().isEmpty());
        QCOMPARE(restarted.value("unrelated-preference").toString(), QString("kept"));
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
    void nativeVideoHistorySections()
    {
        const auto folder = QUrl::fromLocalFile(m_fixtures).toEncoded();
        QCOMPARE(vlc_ml_add_folder(m_library, folder.constData()), VLC_SUCCESS);
        QTRY_VERIFY_WITH_TIMEOUT(media("home-video.mp4") > 0, 45000);
        const auto id = media("home-video.mp4");
        ml_unique_ptr<vlc_ml_media_t> video(vlc_ml_get_media(m_library, id));
        QTRY_VERIFY_WITH_TIMEOUT((video.reset(vlc_ml_get_media(m_library, id)), video && video->i_duration > 0), 45000);
        QCOMPARE(vlc_ml_clear_history(m_library, VLC_ML_HISTORY_TYPE_GLOBAL), VLC_SUCCESS);
        std::atomic<unsigned> changes{0};
        auto *cb = vlc_ml_event_register_callback(m_library,
            [](void *opaque, const vlc_ml_event_t *event) {
                if (event->i_type == VLC_ML_EVENT_HISTORY_CHANGED)
                    ++*static_cast<std::atomic<unsigned> *>(opaque);
            }, &changes);
        QVERIFY(cb);
        std::unique_ptr<vlc_ml_event_callback_t, std::function<void(vlc_ml_event_callback_t *)>> listener(
            cb, [this](auto *value) { vlc_ml_event_unregister_callback(m_library, value); });
        QCOMPARE(vlc_ml_media_update_progress(m_library, id, .5), VLC_SUCCESS);
        video.reset(vlc_ml_get_media(m_library, id));
        QVERIFY(mlVideoHistoryMatches(*video, false));
        QVERIFY(!mlVideoHistoryMatches(*video, true));

        // Real primary-player EOF must replace the resume position with native
        // completion state; direct database writes cannot qualify this path.
        auto *playlist = vlc_test_stream_playlist(m_instance);
        QVERIFY(playlist);
        std::unique_ptr<vlc_playlist_t, decltype(&vlc_playlist_Delete)> owned(playlist, &vlc_playlist_Delete);
        auto *player = vlc_playlist_GetPlayer(playlist);
        auto *input = vlc_ml_get_input_item(m_library, id);
        QVERIFY(input);
        const auto start = QString(":start-time=%1").arg(std::max(0., video->i_duration / 1000. - 1.)).toUtf8();
        input_item_AddOption(input, start.constData(), VLC_INPUT_OPTION_TRUSTED);
        input_item_AddOption(input, ":restore-playback-pos=0", VLC_INPUT_OPTION_TRUSTED);
        input_item_AddOption(input, ":video=1", VLC_INPUT_OPTION_TRUSTED);
        input_item_AddOption(input, ":vout=dummy", VLC_INPUT_OPTION_TRUSTED);
        vlc_player_Lock(player);
        const auto selected = vlc_player_SetCurrentMedia(player, input);
        const auto started = selected == VLC_SUCCESS ? vlc_player_Start(player) : selected;
        vlc_player_Unlock(player);
        input_item_Release(input);
        QCOMPARE(started, VLC_SUCCESS);
        auto completed = [&] {
            ml_unique_ptr<vlc_ml_media_t> item(vlc_ml_get_media(m_library, id));
            return item && mlVideoHistoryMatches(*item, true) && !mlVideoHistoryMatches(*item, false);
        };
        QTRY_VERIFY_WITH_TIMEOUT(completed(), 15000);
        owned.reset();
        QTRY_VERIFY_WITH_TIMEOUT(changes.load() >= 2, 5000);
        for (const auto mode : {"manual", "stop-time", "run-time"})
        {
            qInfo("Video history stop case: %s", mode);
            QCOMPARE(vlc_ml_media_set_played(m_library, id, false), VLC_SUCCESS);
            playlist = vlc_test_stream_playlist(m_instance); QVERIFY(playlist);
            owned.reset(playlist);
            player = vlc_playlist_GetPlayer(playlist);
            input = vlc_ml_get_input_item(m_library, id); QVERIFY(input);
            input_item_AddOption(input, ":restore-playback-pos=0", VLC_INPUT_OPTION_TRUSTED);
            input_item_AddOption(input, ":video=1", VLC_INPUT_OPTION_TRUSTED);
            if (strcmp(mode, "manual") == 0) {
                input_item_AddOption(input, ":start-paused=1", VLC_INPUT_OPTION_TRUSTED);
            } else {
                const QByteArray option = QByteArray(":") + mode + "=0.8";
                input_item_AddOption(input, option.constData(), VLC_INPUT_OPTION_TRUSTED);
            }
            vlc_player_Lock(player);
            const auto selected = vlc_player_SetCurrentMedia(player, input);
            const auto started = selected == VLC_SUCCESS ? vlc_player_Start(player) : selected;
            vlc_player_Unlock(player);
            input_item_Release(input);
            QCOMPARE(started, VLC_SUCCESS);
            auto paused = [&] {
                vlc_player_Lock(player);
                const bool result = vlc_player_GetState(player) == VLC_PLAYER_STATE_PAUSED;
                vlc_player_Unlock(player); return result;
            };
            if (strcmp(mode, "manual") == 0) {
                QTRY_VERIFY_WITH_TIMEOUT(paused(), 5000);
                vlc_player_Lock(player); vlc_player_SetPosition(player, .4); vlc_player_Unlock(player);
                auto positioned = [&] {
                    vlc_player_Lock(player);
                    const auto position = vlc_player_GetPosition(player);
                    vlc_player_Unlock(player); return position > .3 && position < .6;
                };
                QTRY_VERIFY_WITH_TIMEOUT(positioned(), 5000);
                vlc_player_Lock(player); vlc_player_Stop(player); vlc_player_Unlock(player);
            }
            auto unfinished = [&] {
                ml_unique_ptr<vlc_ml_media_t> item(vlc_ml_get_media(m_library, id));
                return item && item->i_playcount == 0 && mlVideoHistoryMatches(*item, false);
            };
            QTRY_VERIFY2_WITH_TIMEOUT(unfinished(), mode, 10000);
            QVERIFY(!completed());
            owned.reset();
        }
        // Restore a completed watch before qualifying an unfinished replay.
        QCOMPARE(vlc_ml_media_update_progress(m_library, id, 1.), VLC_SUCCESS);
        // A stopped replay returns to Continue even after a previous completion.
        QCOMPARE(vlc_ml_media_update_progress(m_library, id, .4), VLC_SUCCESS);
        video.reset(vlc_ml_get_media(m_library, id));
        QVERIFY(video->i_playcount > 0 && mlVideoHistoryMatches(*video, false));
        QVERIFY(!mlVideoHistoryMatches(*video, true));
        QCOMPARE(vlc_ml_media_set_played(m_library, id, false), VLC_SUCCESS);
        video.reset(vlc_ml_get_media(m_library, id));
        QVERIFY(!mlVideoHistoryMatches(*video, true) && !mlVideoHistoryMatches(*video, false));

        // Cross the native page boundary with interleaved states. A view limit
        // or search must not be applied before completion filtering.
        QCOMPARE(vlc_ml_clear_history(m_library, VLC_ML_HISTORY_TYPE_GLOBAL), VLC_SUCCESS);
        std::vector<int64_t> unfinished, watched;
        for (int i = 0; i < 300; ++i)
        {
            const auto url = QUrl::fromLocalFile(QDir(m_fixtures).filePath(
                QString("owned-history-%1.mp4").arg(i, 3, 10, QLatin1Char('0')))).toEncoded();
            ml_unique_ptr<vlc_ml_media_t> entry(vlc_ml_new_external_media(m_library, url.constData()));
            QVERIFY(entry);
            QCOMPARE(vlc_ml_media_set_type(m_library, entry->i_id, VLC_ML_MEDIA_TYPE_VIDEO), VLC_SUCCESS);
            if (i % 3 == 0) {
                QCOMPARE(vlc_ml_media_update_progress(m_library, entry->i_id, .4), VLC_SUCCESS);
                unfinished.push_back(entry->i_id);
            } else if (i % 3 == 1) {
                QCOMPARE(vlc_ml_media_set_played(m_library, entry->i_id, true), VLC_SUCCESS);
                watched.push_back(entry->i_id);
            } else
                QCOMPARE(vlc_ml_media_update_progress(m_library, entry->i_id, 0.), VLC_SUCCESS);
        }
        auto query = vlc_ml_query_params_create(); query.i_sort = VLC_ML_SORTING_ALPHA;
        auto rows = [&](bool done, size_t offset, size_t limit) {
            std::vector<int64_t> result;
            visitMLVideoHistory(m_library, &query, done, [&](const auto &item, size_t index) {
                if (index >= offset) result.push_back(item.i_id);
                return limit == 0 || result.size() < limit;
            });
            return result;
        };
        QCOMPARE(vlc_ml_count_video_history(m_library, &query), size_t(300));
        QCOMPARE(rows(false, 0, 0), unfinished);
        QCOMPARE(rows(true, 0, 0), watched);
        query.i_nbResults = 4; query.i_offset = 90;
        QCOMPARE(rows(false, 90, 4), (std::vector<int64_t>(unfinished.begin()+90, unfinished.begin()+94)));
        QCOMPARE(visitMLVideoHistory(m_library, &query, false, [](const auto &, size_t) { return true; }), size_t(100));
        query.b_desc = true;
        std::reverse(watched.begin(), watched.end());
        QCOMPARE(rows(true, 0, 4), (std::vector<int64_t>(watched.begin(), watched.begin()+4)));
        query.psz_pattern = "owned-history-298";
        QCOMPARE(rows(true, 0, 0), (std::vector<int64_t>{watched.front()}));
        QVERIFY(rows(false, 0, 0).empty());
        QCOMPARE(vlc_ml_clear_history(m_library, VLC_ML_HISTORY_TYPE_GLOBAL), VLC_SUCCESS);
        query.psz_pattern = nullptr;
        QVERIFY(rows(false, 0, 0).empty() && rows(true, 0, 0).empty());
    }
    void nativeVideoHistoryCompletionBoundaries()
    {
        const auto folder = QUrl::fromLocalFile(m_fixtures).toEncoded();
        QCOMPARE(vlc_ml_add_folder(m_library, folder.constData()), VLC_SUCCESS);
        QTRY_VERIFY_WITH_TIMEOUT(media("home-video.mp4") > 0, 45000);
        const auto id = media("home-video.mp4");
        ml_unique_ptr<vlc_ml_media_t> video(vlc_ml_get_media(m_library, id));
        QTRY_VERIFY_WITH_TIMEOUT((video.reset(vlc_ml_get_media(m_library, id)), video && video->i_duration > 1000), 45000);
        const double duration = video->i_duration / 1000.;

        struct Row {
            const char *name;
            double start, stop;
            bool pauseAtEnd, seekBeforeStop, waitForSeek, expectCompleted;
        };
        const Row rows[] = {
            {"normal-eof", 0., 0., false, false, false, true},
            {"ignored-stop-before-start", .5, .25, false, false, false, true},
            {"ignored-stop-equal-start", .5, .5, false, false, false, true},
            {"stop-after-source-end", 0., duration + 1., false, false, false, true},
            {"pause-at-end-then-stop", 0., 0., true, false, false, true},
            {"pause-at-end-settled-seek-then-stop", 0., 0., true, true, true, false},
            {"pause-at-end-queued-seek-immediate-stop", 0., 0., true, true, false, false},
        };
        bool allPassed = true;
        for (const auto &row : rows)
        {
            qInfo("Video history completion boundary: %s", row.name);
            QCOMPARE(vlc_ml_media_set_played(m_library, id, false), VLC_SUCCESS);
            video.reset(vlc_ml_get_media(m_library, id));
            QVERIFY(video);
            QCOMPARE(video->i_playcount, uint32_t(0));
            QVERIFY(video->f_progress <= 0.);
            QVERIFY(!mlVideoHistoryMatches(*video, true));
            QVERIFY(!mlVideoHistoryMatches(*video, false));

            struct PositionTrace {
                std::atomic<double> position{-1.};
                std::atomic_bool pauseOnTarget{false}, reachedTarget{false};
            } trace;
            bool observed = true;
            auto *playlist = vlc_test_stream_playlist(m_instance);
            QVERIFY(playlist);
            std::unique_ptr<vlc_playlist_t, decltype(&vlc_playlist_Delete)> owned(playlist, &vlc_playlist_Delete);
            auto *player = vlc_playlist_GetPlayer(playlist);
            static const vlc_player_cbs callbacks = [] {
                vlc_player_cbs c{};
                c.on_position_changed = [](vlc_player_t *player, vlc_tick_t, double position, void *data) {
                    auto *trace = static_cast<PositionTrace *>(data);
                    trace->position = position;
                    if (trace->pauseOnTarget && position > .3 && position < .6)
                    {
                        trace->pauseOnTarget = false;
                        trace->reachedTarget = true;
                        vlc_player_Pause(player);
                    }
                };
                return c;
            }();
            vlc_player_Lock(player);
            auto *listener = vlc_player_AddListener(player, &callbacks, &trace);
            vlc_player_Unlock(player);
            QVERIFY(listener);
            auto removeListener = [player](vlc_player_listener_id *id) {
                vlc_player_Lock(player);
                vlc_player_RemoveListener(player, id);
                vlc_player_Unlock(player);
            };
            std::unique_ptr<vlc_player_listener_id, decltype(removeListener)> listenerOwner(listener, removeListener);
            auto *input = vlc_ml_get_input_item(m_library, id);
            QVERIFY(input);
            const QByteArray start = QString(":start-time=%1").arg(row.start).toUtf8();
            const QByteArray stop = QString(":stop-time=%1").arg(row.stop).toUtf8();
            for (const auto *option : {start.constData(), stop.constData(), ":run-time=0",
                                      ":restore-playback-pos=0", ":start-paused=0", ":video=1", ":vout=dummy"})
                input_item_AddOption(input, option, VLC_INPUT_OPTION_TRUSTED);
            vlc_player_Lock(player);
            vlc_player_SetStartPaused(player, false);
            vlc_player_SetPauseOnCork(player, false);
            vlc_player_SetPlayAndPause(player, row.pauseAtEnd);
            const auto selected = vlc_player_SetCurrentMedia(player, input);
            const auto started = selected == VLC_SUCCESS ? vlc_player_Start(player) : selected;
            vlc_player_Unlock(player);
            input_item_Release(input);
            QCOMPARE(started, VLC_SUCCESS);
            auto state = [&] {
                vlc_player_Lock(player);
                const auto value = vlc_player_GetState(player);
                vlc_player_Unlock(player);
                return value;
            };
            QTRY_VERIFY2_WITH_TIMEOUT(state() == VLC_PLAYER_STATE_PLAYING
                                      || state() == VLC_PLAYER_STATE_PAUSED, row.name, 5000);
            if (row.pauseAtEnd)
            {
                // Start-paused and pause-on-cork are disabled. Before this
                // first pause, no test pause request is issued: it is real EOF.
                QTRY_COMPARE_WITH_TIMEOUT(state(), VLC_PLAYER_STATE_PAUSED, 10000);
                vlc_player_Lock(player);
                const auto beforeStop = vlc_player_GetState(player);
                const auto positionAtPause = vlc_player_GetPosition(player);
                vlc_player_Unlock(player);
                QCOMPARE(beforeStop, VLC_PLAYER_STATE_PAUSED);
                qInfo("Paused at natural EOF before user Stop: progress %.17g", positionAtPause);
                if (row.seekBeforeStop)
                {
                    vlc_player_Lock(player);
                    trace.position = -1.;
                    trace.pauseOnTarget = row.waitForSeek;
                    vlc_player_SetPosition(player, .4);
                    if (row.waitForSeek)
                        vlc_player_Resume(player);
                    else
                        vlc_player_Stop(player); // same lock: no input event can intervene
                    vlc_player_Unlock(player);
                    if (row.waitForSeek)
                    {
                        // A paused output can retain its old clock after a
                        // seek. Resume until a real input position callback
                        // reaches the target, where the listener pauses it.
                        // A pending timer target alone cannot satisfy this.
                        const bool reached = QTest::qWaitFor([&] { return trace.reachedTarget.load(); }, 5000);
                        const bool paused = reached && QTest::qWaitFor([&] { return state() == VLC_PLAYER_STATE_PAUSED; }, 5000);
                        observed = reached && paused;
                        qInfo("Actual position reported after EOF seek: %.17g, target observed %s, paused %s",
                              trace.position.load(), reached ? "yes" : "no", paused ? "yes" : "no");
                        vlc_player_Lock(player);
                        trace.pauseOnTarget = false;
                        vlc_player_Stop(player);
                        vlc_player_Unlock(player);
                    }
                }
                else
                {
                    vlc_player_Lock(player);
                    vlc_player_Stop(player);
                    vlc_player_Unlock(player);
                }
            }
            QTRY_COMPARE_WITH_TIMEOUT(state(), VLC_PLAYER_STATE_STOPPED, 10000);
            listenerOwner.reset();
            owned.reset(); // joins the real player and its history write
            video.reset(vlc_ml_get_media(m_library, id));
            QVERIFY(video);
            // Completion clears the resume position; the native backend uses
            // an unset sentinel (-1), rather than promising an exact zero.
            const bool completed = video->i_playcount == 1 && video->f_progress <= 0.
                && mlVideoHistoryMatches(*video, true) && !mlVideoHistoryMatches(*video, false);
            const bool unfinished = video->i_playcount == 0 && video->f_progress > 0. && video->f_progress < 1.
                && mlVideoHistoryMatches(*video, false) && !mlVideoHistoryMatches(*video, true);
            const bool passed = observed && (row.expectCompleted ? completed : unfinished);
            qInfo("Completion boundary %s: progress %.17g, play count %u, expected %s, passed %s",
                  row.name, video->f_progress, unsigned(video->i_playcount),
                  row.expectCompleted ? "completed" : "unfinished", passed ? "yes" : "no");
            allPassed = allPassed && passed;
        }
        // Collect every row's result before failing, without changing the
        // native completion predicate or accepting an approximate position.
        QVERIFY2(allPassed, "Every full-source EOF must persist one completed watch, and a subsequent seek must retain unfinished state; inspect the named row results");
        // The fixture has no title/chapter boundaries. MRL-range playback needs
        // a separate generated chapter/title fixture, not a fabricated pass.
    }
    void nativeVideoHistoryRejectedRecordingSeek()
    {
        const auto folder = QUrl::fromLocalFile(m_fixtures).toEncoded();
        QCOMPARE(vlc_ml_add_folder(m_library, folder.constData()), VLC_SUCCESS);
        QTRY_VERIFY_WITH_TIMEOUT(media("home-video.mp4") > 0, 45000);
        const auto id = media("home-video.mp4");
        ml_unique_ptr<vlc_ml_media_t> video(vlc_ml_get_media(m_library, id));
        QTRY_VERIFY_WITH_TIMEOUT((video.reset(vlc_ml_get_media(m_library, id)), video && video->i_duration > 1000), 45000);
        QCOMPARE(vlc_ml_media_set_played(m_library, id, false), VLC_SUCCESS);
        QTemporaryDir recordingDirectory;
        QVERIFY(recordingDirectory.isValid());
        const auto recordingPath = recordingDirectory.path().toUtf8();

        auto *playlist = vlc_test_stream_playlist(m_instance);
        QVERIFY(playlist);
        std::unique_ptr<vlc_playlist_t, decltype(&vlc_playlist_Delete)> owned(playlist, &vlc_playlist_Delete);
        auto *player = vlc_playlist_GetPlayer(playlist);
        auto *input = vlc_ml_get_input_item(m_library, id);
        QVERIFY(input);
        for (const auto *option : {":restore-playback-pos=0", ":start-paused=1",
                                  ":start-time=0", ":stop-time=0", ":run-time=0",
                                  ":video=1", ":vout=dummy"})
            input_item_AddOption(input, option, VLC_INPUT_OPTION_TRUSTED);
        vlc_player_Lock(player);
        vlc_player_SetStartPaused(player, true);
        vlc_player_SetPauseOnCork(player, false);
        vlc_player_SetPlayAndPause(player, false);
        const auto selected = vlc_player_SetCurrentMedia(player, input);
        const auto started = selected == VLC_SUCCESS ? vlc_player_Start(player) : selected;
        vlc_player_Unlock(player);
        input_item_Release(input);
        QCOMPARE(started, VLC_SUCCESS);
        auto state = [&] {
            vlc_player_Lock(player);
            const auto value = vlc_player_GetState(player);
            vlc_player_Unlock(player);
            return value;
        };
        QTRY_COMPARE_WITH_TIMEOUT(state(), VLC_PLAYER_STATE_PAUSED, 5000);
        vlc_player_Lock(player);
        vlc_player_SetPosition(player, .4);
        vlc_player_Unlock(player);
        auto actuallyPositioned = [&] {
            vlc_player_Lock(player);
            // GetPosition excludes the pending seek target. Establish the
            // real held position before enabling the seek-rejecting recorder.
            const auto position = vlc_player_GetPosition(player);
            const bool paused = vlc_player_GetState(player) == VLC_PLAYER_STATE_PAUSED;
            vlc_player_Unlock(player);
            return paused && position > .3 && position < .6;
        };
        QTRY_VERIFY_WITH_TIMEOUT(actuallyPositioned(), 5000);
        vlc_player_Lock(player);
        vlc_player_SetRecordingEnabled(player, true, recordingPath.constData());
        vlc_player_Unlock(player);
        auto actuallyRecording = [&] {
            vlc_player_Lock(player);
            const bool recording = vlc_player_IsRecording(player);
            const bool paused = vlc_player_GetState(player) == VLC_PLAYER_STATE_PAUSED;
            vlc_player_Unlock(player);
            return recording && paused;
        };
        QTRY_VERIFY_WITH_TIMEOUT(actuallyRecording(), 5000);
        vlc_player_Lock(player);
        const bool recordingBeforeStop = vlc_player_IsRecording(player);
        const double actualBeforeStop = vlc_player_GetPosition(player);
        vlc_player_SetPosition(player, .95);
        const double pendingBeforeStop = vlc_player_GetPositionWithSeek(player);
        vlc_player_Stop(player); // same lock: the rejected target must not become history
        vlc_player_Unlock(player);
        QVERIFY(recordingBeforeStop);
        QVERIFY(actualBeforeStop > .3 && actualBeforeStop < .6);
        QCOMPARE(pendingBeforeStop, .95);
        QTRY_COMPARE_WITH_TIMEOUT(state(), VLC_PLAYER_STATE_STOPPED, 10000);
        owned.reset();
        video.reset(vlc_ml_get_media(m_library, id));
        QVERIFY(video);
        qInfo("Recording-rejected seek: actual %.17g, pending %.17g, saved %.17g, play count %u",
              actualBeforeStop, pendingBeforeStop, video->f_progress, unsigned(video->i_playcount));
        QCOMPARE(video->i_playcount, uint32_t(0));
        QVERIFY(video->f_progress > .3 && video->f_progress < .6);
        QVERIFY(qAbs(video->f_progress - actualBeforeStop) < .05);
        QVERIFY(mlVideoHistoryMatches(*video, false));
        QVERIFY(!mlVideoHistoryMatches(*video, true));
        // Real recording state qualifies the rejected-seek history route.
        // Starting/stopping while paused does not qualify recording content.
    }
    void cleanupTestCase()
    {
        if (m_instance)
            libvlc_release(m_instance);
    }
};
QTEST_GUILESS_MAIN(TestHomeActivity)
#include "test_home_activity.moc"
