// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
// Native save/restart evidence is distinct from supplied-state QML checks.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "dialogs/new_playlist_model.hpp"
#include "medialibrary/mlhelper.hpp"
#include "medialibrary/mlmedia.hpp"
#include "medialibrary/mlplaylist.hpp"
#include <vlc/vlc.h>
#include <vlc_playlist.h>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QSignalSpy>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QTest>

extern "C" vlc_medialibrary_t *vlc_test_stream_library(libvlc_instance_t *);
extern "C" vlc_playlist_t *vlc_test_stream_playlist(libvlc_instance_t *);

class FaultBackend : public NewPlaylistBackend
{
public:
    int resolutions = 0, creations = 0, deletions = 0;
    int failResolution = 0;
    bool failCreate = false, failAppend = false, failDelete = false;
    QVector<int64_t> order;
    qint64 resolve(const QUrl &) override { ++resolutions; return failResolution == resolutions ? 0 : resolutions; }
    qint64 create(const QString &) override { ++creations; return failCreate ? 0 : 41; }
    bool append(qint64 id, const QVector<int64_t> &ids) override { Q_ASSERT(id == 41); order = ids; return !failAppend; }
    bool remove(qint64 id) override { Q_ASSERT(id == 41); ++deletions; return !failDelete; }
};

class TestNewPlaylist : public QObject
{
    Q_OBJECT
    libvlc_instance_t *m_instance = nullptr;
    vlc_medialibrary_t *m_library = nullptr;
    vlc_playlist_t *m_queue = nullptr;
    std::unique_ptr<NewPlaylistModel> m_model;
    QString m_fixtures, m_artDirectory, m_imagePath;
    QTemporaryDir m_art;
    qint64 m_saved = 0;
    libvlc_instance_t *instance()
    {
        const char *args[] = {"--ignore-config", "--media-library", "--no-auto-preparse",
            "--no-video", "--aout=dummy", "--verbose=2", "--file-logging", "--logfile=new-playlist-native.log"};
        return libvlc_new(ARRAY_SIZE(args), args);
    }
    QUrl file(const char *name) const { return QUrl::fromLocalFile(QDir(m_fixtures).filePath(QString::fromUtf8(name))); }
    void assertQueueEmpty()
    {
        vlc_playlist_Lock(m_queue);
        const auto count = vlc_playlist_Count(m_queue);
        const auto current = vlc_playlist_GetCurrentIndex(m_queue);
        vlc_playlist_Unlock(m_queue);
        QCOMPARE(count, size_t(0));
        QCOMPARE(current, ssize_t(-1));
    }
private slots:
    void initTestCase()
    {
        m_fixtures = qEnvironmentVariable("VLC_PLAYLIST_FIXTURE_DIR");
        QVERIFY2(!m_fixtures.isEmpty() && QDir(m_fixtures).exists(), "Use the generated Home audio/video fixtures as VLC_PLAYLIST_FIXTURE_DIR");
        QVERIFY2(!qEnvironmentVariable("VLC_USERDATA_PATH").isEmpty(), "Use fresh isolated VLC_USERDATA_PATH");
        QVERIFY(m_art.isValid());
        m_artDirectory = m_art.filePath("saved-art");
        m_imagePath = m_art.filePath("chosen.png");
        QImage image(120, 80, QImage::Format_RGB32);
        image.fill(Qt::darkRed);
        QVERIFY(image.save(m_imagePath));
        m_instance = instance();
        QVERIFY(m_instance);
        m_library = vlc_test_stream_library(m_instance);
        QVERIFY(m_library);
        QCOMPARE(vlc_ml_count_playlists(m_library, nullptr, VLC_ML_PLAYLIST_TYPE_ALL), size_t(0));
        QCOMPARE(vlc_ml_count_media(m_library, nullptr), size_t(0));
        m_queue = vlc_test_stream_playlist(m_instance);
        QVERIFY(m_queue);
        m_model = std::make_unique<NewPlaylistModel>(m_library, m_artDirectory);
    }
    void validationAndSelectionOrder()
    {
        NewPlaylistModel unavailable(nullptr, m_artDirectory);
        unavailable.setName("Unavailable");
        QVERIFY(!unavailable.canSave());
        QVERIFY(!unavailable.error().isEmpty());
        unavailable.save();
        QVERIFY(!unavailable.busy());
        QVERIFY(!m_model->canSave());
        m_model->setName("   ");
        QVERIFY(!m_model->canSave());
        m_model->setName(QString(256, 'a'));
        QVERIFY(!m_model->canSave());
        m_model->setName("Valid empty playlist");
        QVERIFY(m_model->canSave());
        QVERIFY(!m_model->appendFiles({QUrl("https://example.invalid/video.mp4")}));
        QVERIFY(!isLocalPlaylistFile(QUrl("file://server/share/video.mp4")));
        QVERIFY(!isLocalPlaylistFile(QUrl::fromLocalFile("//server/share/video.mp4")));
        // Windows Qt accepts these as authority-free local file URLs. Neither
        // UNC/device IO nor the process working directory is a playlist source.
        const QList<QUrl> unsafePaths {
            QUrl("file:%5C%5Cunavailable-server%5Cshare%5Cvideo.mp4"),
            QUrl("file:%5C%5C%3F%5CC:%5Ctemp%5Cvideo.mp4"),
            QUrl("file:%5C%5C.%5Cpipe%5Cvideo.mp4"),
            QUrl("file:%5C%5C%3F%5CUNC%5Cunavailable-server%5Cshare%5Cvideo.mp4"),
            QUrl("file:%5C%2Funavailable-server%5Cshare%5Cvideo.mp4"),
            QUrl("file:relative.mp4"), QUrl("file:C:relative.mp4"),
            QUrl("file:C:"), QUrl("file:"), QUrl()};
        for (const auto &url : unsafePaths)
            QVERIFY2(!isLocalPlaylistFile(url), qPrintable(url.toString()));
        QVERIFY(isLocalPlaylistFile(file("home-video.mp4")));
        QVERIFY(isLocalPlaylistFile(file("alpha-one.flac")));
        QVERIFY(!m_model->appendFiles({QUrl::fromLocalFile(m_fixtures)}));
        QVERIFY(m_model->files().isEmpty());
        QVERIFY(m_model->appendFiles({file("home-video.mp4"), file("alpha-one.flac"), file("home-video.mp4")}));
        QCOMPARE(m_model->files().size(), 3);
        m_model->moveFile(1, 0);
        QCOMPARE(m_model->files()[0].toMap()["url"].toUrl(), file("alpha-one.flac"));
        m_model->removeFile(1);
        QCOMPARE(m_model->files().size(), 2);
        const auto acceptedThumbnail = QUrl::fromLocalFile(m_imagePath);
        m_model->setThumbnail(acceptedThumbnail);
        QCOMPARE(m_model->thumbnail(), acceptedThumbnail);
        bool emittedUnsafeThumbnail = false;
        QObject thumbnailObserver;
        const auto observeThumbnail = connect(m_model.get(), &NewPlaylistModel::changed,
            &thumbnailObserver, [this, acceptedThumbnail, &emittedUnsafeThumbnail] {
                if (m_model->thumbnail() != acceptedThumbnail)
                    emittedUnsafeThumbnail = true;
            }, Qt::DirectConnection);
        auto unsafeThumbnails = unsafePaths;
        unsafeThumbnails.removeAll(QUrl()); // Empty is the valid Remove action.
        unsafeThumbnails.append(QUrl("https://example.invalid/cover.png"));
        unsafeThumbnails.append(QUrl("file://unavailable-server/share/cover.png"));
        for (const auto &url : unsafeThumbnails)
        {
            m_model->setThumbnail(url);
            QCOMPARE(m_model->thumbnail(), acceptedThumbnail);
            QVERIFY(!m_model->error().isEmpty());
        }
        QVERIFY(!emittedUnsafeThumbnail);
        disconnect(observeThumbnail);
        m_model->setThumbnail(QUrl::fromLocalFile(m_art.filePath("missing.png")));
        QVERIFY(!m_model->canSave());
        m_model->setThumbnail({});
        QVERIFY(m_model->canSave());
        m_model->reset();
        QVERIFY(m_model->files().isEmpty() && m_model->name().isEmpty());
        assertQueueEmpty();
    }
    void rollbackAndPartialFailure()
    {
        NewPlaylistRequest request{"Failure branch", {file("home-video.mp4"), file("alpha-one.flac")}, {}};
        FaultBackend resolveFailure;
        resolveFailure.failResolution = 2;
        auto result = saveNewPlaylist(resolveFailure, request, m_artDirectory);
        QVERIFY(!result.success && !result.persisted && result.id == 0);
        QCOMPARE(resolveFailure.creations, 0);
        QVERIFY(result.error.contains("No playlist"));
        FaultBackend createFailure;
        createFailure.failCreate = true;
        result = saveNewPlaylist(createFailure, request, m_artDirectory);
        QVERIFY(!result.success && !result.persisted);
        QCOMPARE(createFailure.deletions, 0);
        FaultBackend appendFailure;
        appendFailure.failAppend = true;
        result = saveNewPlaylist(appendFailure, request, m_artDirectory);
        QVERIFY(!result.success && !result.persisted && result.id == 0);
        QCOMPARE(appendFailure.deletions, 1);
        QCOMPARE(appendFailure.order, QVector<int64_t>({1, 2}));
        FaultBackend rollbackFailure;
        rollbackFailure.failAppend = rollbackFailure.failDelete = true;
        result = saveNewPlaylist(rollbackFailure, request, m_artDirectory);
        QVERIFY(!result.success && result.persisted && result.id == 41);
        QVERIFY(!result.error.isEmpty());
        assertQueueEmpty();
    }
    void actualMixedSaveWithoutPlayback()
    {
        const auto folder = QUrl::fromLocalFile(m_fixtures).toEncoded();
        QCOMPARE(vlc_ml_add_folder(m_library, folder.constData()), VLC_SUCCESS);
        QTRY_COMPARE_WITH_TIMEOUT(vlc_ml_count_audio_media(m_library, nullptr), size_t(4), 45000);
        QTRY_COMPARE_WITH_TIMEOUT(vlc_ml_count_video_media(m_library, nullptr), size_t(1), 45000);
        m_model->setName(QString::fromUtf8("  Fav Episodes – native  "));
        m_model->setThumbnail(QUrl::fromLocalFile(m_imagePath));
        QVERIFY(m_model->appendFiles({file("home-video.mp4"), file("alpha-one.flac"), file("home-video.mp4"), file("beta-one.flac")}));
        QSignalSpy finished(m_model.get(), &NewPlaylistModel::finished);
        m_model->save();
        QVERIFY(m_model->busy());
        m_model->setName("Must not replace pending title");
        m_model->removeFile(0);
        m_model->reset();
        m_model->save();
        QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 15000);
        QVERIFY2(finished.at(0)[0].toBool(), qPrintable(m_model->error()));
        m_saved = m_model->savedPlaylist().id;
        QVERIFY(m_saved > 0);
        QCOMPARE(m_model->savedPlaylist().type, VLC_ML_PARENT_PLAYLIST);
        QVERIFY(!m_model->canSave() && !m_model->partialSave());
        m_model->save();
        QCOMPARE(finished.count(), 1);
        QCOMPARE(vlc_ml_count_playlists(m_library, nullptr, VLC_ML_PLAYLIST_TYPE_ALL), size_t(1));
        ml_unique_ptr<vlc_ml_playlist_t> playlist(vlc_ml_get_playlist(m_library, m_saved));
        QVERIFY(playlist);
        QCOMPARE(QString::fromUtf8(playlist->psz_name), QString::fromUtf8("Fav Episodes – native"));
        QCOMPARE(playlist->i_nb_audio, unsigned(2));
        QCOMPARE(playlist->i_nb_video, unsigned(2));
        ml_unique_ptr<vlc_ml_media_list_t> media(vlc_ml_list_playlist_media(m_library, nullptr, m_saved));
        QVERIFY(media);
        QCOMPARE(media->i_nb_items, size_t(4));
        QCOMPARE(media->p_items[0].i_type, VLC_ML_MEDIA_TYPE_VIDEO);
        QCOMPARE(media->p_items[1].i_type, VLC_ML_MEDIA_TYPE_AUDIO);
        QCOMPARE(media->p_items[2].i_id, media->p_items[0].i_id);
        QCOMPARE(media->p_items[3].i_type, VLC_ML_MEDIA_TYPE_AUDIO);
        QVERIFY(!newPlaylistArtwork(m_artDirectory, m_saved).isEmpty());
        QVERIFY(QFile::remove(m_imagePath));
        QVERIFY(!QImage(QUrl(newPlaylistArtwork(m_artDirectory, m_saved)).toLocalFile()).isNull());
        assertQueueEmpty();
    }
    void actualExternalMediaUnknownDurationWithoutDiscovery()
    {
        QTemporaryDir unindexed;
        QVERIFY(unindexed.isValid());
        const auto path = unindexed.filePath("first-external-audio.flac");
        QVERIFY(QFile::copy(file("alpha-one.flac").toLocalFile(), path));
        const auto uri = QUrl::fromLocalFile(path).toEncoded();
        QVERIFY(!ml_unique_ptr<vlc_ml_media_t>(vlc_ml_get_media_by_mrl(m_library, uri.constData())));
        const auto rootsBefore = vlc_ml_count_entry_points(m_library, nullptr);
        const auto playlistsBefore = vlc_ml_count_playlists(m_library, nullptr, VLC_ML_PLAYLIST_TYPE_ALL);
        QCOMPARE(vlc_ml_count_audio_media(m_library, nullptr), size_t(4));
        QCOMPARE(vlc_ml_count_video_media(m_library, nullptr), size_t(1));

        ml_unique_ptr<vlc_ml_media_t> external(vlc_ml_new_external_media(m_library, uri.constData()));
        QVERIFY(external);
        QVERIFY(external->i_id > 0);
        QCOMPARE(external->i_type, VLC_ML_MEDIA_TYPE_UNKNOWN);
        QCOMPARE(external->i_duration, int64_t(-1));
        const MLMedia media(external.get());
        QCOMPARE(QUrl(media.mrl()), QUrl::fromEncoded(uri));
        QVERIFY(!media.duration().valid());
        QCOMPARE(media.duration().formatShort(), QString("--:--"));
        const auto signedDelay = VLCDuration::fromMS(-650);
        QVERIFY(signedDelay.valid());
        QCOMPARE(signedDelay.asMilliseconds(), int64_t(-650));

        ml_unique_ptr<vlc_ml_playlist_t> created(vlc_ml_playlist_create(m_library, "Transient unknown duration diagnostic"));
        QVERIFY(created);
        const auto id = created->i_id;
        auto cleanup = qScopeGuard([this, id] { vlc_ml_playlist_delete(m_library, id); });
        QCOMPARE(created->i_nb_media, unsigned(0));
        QCOMPARE(created->i_nb_duration_unknown, uint32_t(0));
        const MLPlaylist empty(created.get());
        QVERIFY(empty.getDuration().valid());
        QCOMPARE(empty.getDuration().asMilliseconds(), int64_t(0));
        const int64_t duplicates[] = {external->i_id, external->i_id};
        QCOMPARE(vlc_ml_playlist_append(m_library, id, duplicates, ARRAY_SIZE(duplicates)), VLC_SUCCESS);
        {
            ml_unique_ptr<vlc_ml_playlist_t> native(vlc_ml_get_playlist(m_library, id));
            QVERIFY(native);
            QCOMPARE(native->i_nb_media, unsigned(2));
            QCOMPARE(native->i_nb_duration_unknown, uint32_t(2));
            QCOMPARE(native->i_duration, int64_t(0));
            const MLPlaylist allUnknown(native.get());
            QVERIFY(!allUnknown.getDuration().valid());
            QCOMPARE(allUnknown.getDuration().formatShort(), QString("--:--"));
        }
        const auto knownUri = file("home-video.mp4").toEncoded();
        ml_unique_ptr<vlc_ml_media_t> known(vlc_ml_get_media_by_mrl(m_library, knownUri.constData()));
        QVERIFY(known && known->i_duration > 0);
        QCOMPARE(vlc_ml_playlist_append(m_library, id, &known->i_id, 1), VLC_SUCCESS);
        {
            ml_unique_ptr<vlc_ml_playlist_t> native(vlc_ml_get_playlist(m_library, id));
            QVERIFY(native);
            QCOMPARE(native->i_nb_media, unsigned(3));
            QCOMPARE(native->i_nb_duration_unknown, uint32_t(2));
            QCOMPARE(native->i_duration, known->i_duration);
            const MLPlaylist partialKnown(native.get());
            QVERIFY(partialKnown.getDuration().valid());
            QCOMPARE(partialKnown.getDuration().asMilliseconds(), known->i_duration);
        }
        QCOMPARE(vlc_ml_playlist_delete(m_library, id), VLC_SUCCESS);
        cleanup.dismiss();
        QCOMPARE(vlc_ml_count_playlists(m_library, nullptr, VLC_ML_PLAYLIST_TYPE_ALL), playlistsBefore);
        QCOMPARE(vlc_ml_count_entry_points(m_library, nullptr), rootsBefore);
        QCOMPARE(vlc_ml_count_audio_media(m_library, nullptr), size_t(4));
        QCOMPARE(vlc_ml_count_video_media(m_library, nullptr), size_t(1));
        assertQueueEmpty();
    }
    void actualThumbnailWriteFailureRollsBack()
    {
        const auto count = vlc_ml_count_playlists(m_library, nullptr, VLC_ML_PLAYLIST_TYPE_ALL);
        const auto blocked = m_art.filePath("blocked-art-path");
        QFile obstruction(blocked);
        QVERIFY(obstruction.open(QIODevice::WriteOnly));
        obstruction.close();
        NewPlaylistRequest request{"Thumbnail failure", {file("alpha-one.flac")}, QUrl(newPlaylistArtwork(m_artDirectory, m_saved))};
        const auto result = saveNewPlaylist(m_library, request, blocked);
        QVERIFY(!result.success && !result.persisted && result.id == 0);
        QVERIFY(result.error.contains("thumbnail"));
        QCOMPARE(vlc_ml_count_playlists(m_library, nullptr, VLC_ML_PLAYLIST_TYPE_ALL), count);
        assertQueueEmpty();
    }
    void actualEmptyPlaylist()
    {
        m_model->reset();
        m_model->setName("Empty playlist");
        QSignalSpy finished(m_model.get(), &NewPlaylistModel::finished);
        m_model->save();
        QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 15000);
        QVERIFY2(finished.at(0)[0].toBool(), qPrintable(m_model->error()));
        ml_unique_ptr<vlc_ml_playlist_t> playlist(vlc_ml_get_playlist(m_library, m_model->savedPlaylist().id));
        QVERIFY(playlist);
        QCOMPARE(playlist->i_nb_media, unsigned(0));
        assertQueueEmpty();
    }
    void persistenceAfterRestart()
    {
        m_model.reset();
        vlc_playlist_Delete(m_queue);
        m_queue = nullptr;
        libvlc_release(m_instance);
        m_instance = instance();
        QVERIFY(m_instance);
        m_library = vlc_test_stream_library(m_instance);
        QVERIFY(m_library);
        ml_unique_ptr<vlc_ml_playlist_t> playlist(vlc_ml_get_playlist(m_library, m_saved));
        QVERIFY(playlist);
        QCOMPARE(QString::fromUtf8(playlist->psz_name), QString::fromUtf8("Fav Episodes – native"));
        QCOMPARE(playlist->i_nb_audio, unsigned(2));
        QCOMPARE(playlist->i_nb_video, unsigned(2));
        ml_unique_ptr<vlc_ml_media_list_t> media(vlc_ml_list_playlist_media(m_library, nullptr, m_saved));
        QVERIFY(media);
        QCOMPARE(media->i_nb_items, size_t(4));
        QCOMPARE(media->p_items[0].i_id, media->p_items[2].i_id);
        QVERIFY(!QImage(QUrl(newPlaylistArtwork(m_artDirectory, m_saved)).toLocalFile()).isNull());
    }
    void submittedSaveCompletesBeforeModelDestruction()
    {
        const auto before = vlc_ml_count_playlists(m_library, nullptr, VLC_ML_PLAYLIST_TYPE_ALL);
        auto pending = std::make_unique<NewPlaylistModel>(m_library, m_artDirectory);
        pending->setName("Save while dialog owner closes");
        QVERIFY(pending->appendFiles({file("home-video.mp4"), file("beta-one.flac")}));
        pending->save();
        QVERIFY(pending->busy());
        pending.reset(); // Production destructor joins its sole save worker.
        QCoreApplication::sendPostedEvents();
        QCOMPARE(vlc_ml_count_playlists(m_library, nullptr, VLC_ML_PLAYLIST_TYPE_ALL), before + 1);
        // The queue remains absent after restart; the save path has no queue.
    }
    void cleanupTestCase()
    {
        m_model.reset();
        if (m_queue) vlc_playlist_Delete(m_queue);
        if (m_instance) libvlc_release(m_instance);
    }
};
QTEST_GUILESS_MAIN(TestNewPlaylist)
#include "test_new_playlist.moc"
