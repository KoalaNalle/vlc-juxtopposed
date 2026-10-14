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
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>
#include <limits>

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
    void cleanupTestCase()
    {
        if (m_instance)
            libvlc_release(m_instance);
    }
};
QTEST_GUILESS_MAIN(TestHomeActivity)
#include "test_home_activity.moc"
