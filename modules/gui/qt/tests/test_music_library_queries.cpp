// SPDX-License-Identifier: GPL-2.0-or-later
// Native library query evidence; ordinary Qt model/desktop evidence is separate.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <vlc_common.h>
#include <vlc/vlc.h>
#include <vlc_media_library.h>
#include <QDir>
#include <QUrl>
#include <QTest>
#include <memory>

extern "C" vlc_medialibrary_t *vlc_test_stream_library(libvlc_instance_t *);
using MediaList = std::unique_ptr<vlc_ml_media_list_t, decltype(&vlc_ml_media_list_release)>;
using FolderList = std::unique_ptr<vlc_ml_folder_list_t, decltype(&vlc_ml_folder_list_release)>;
using ArtistList = std::unique_ptr<vlc_ml_artist_list_t, decltype(&vlc_ml_artist_list_release)>;

class TestMusicLibraryQueries : public QObject
{
    Q_OBJECT
    libvlc_instance_t *m_instance = nullptr;
    vlc_medialibrary_t *m_library = nullptr;
    int64_t m_folder = 0;
    static QStringList titles(const vlc_ml_media_list_t *list)
    {
        QStringList result;
        if (list) for (size_t i=0; i<list->i_nb_items; ++i)
            result.append(QString::fromUtf8(list->p_items[i].psz_title));
        return result;
    }
private slots:
    void initTestCase()
    {
        const auto fixtures = qEnvironmentVariable("VLC_MUSIC_FIXTURE_DIR");
        QVERIFY(!fixtures.isEmpty() && QDir(fixtures).exists());
        QVERIFY(!qEnvironmentVariable("VLC_USERDATA_PATH").isEmpty());
        const char *args[] = {"--ignore-config", "--media-library", "--no-metadata-network-access",
            "--no-ml-network-discovery", "--no-video", "--aout=dummy", "--verbose=2"};
        m_instance = libvlc_new(ARRAY_SIZE(args), args); QVERIFY(m_instance);
        m_library = vlc_test_stream_library(m_instance); QVERIFY(m_library);
        QCOMPARE(vlc_ml_count_media(m_library, nullptr), size_t(0));
        const auto uri = QUrl::fromLocalFile(fixtures).toEncoded();
        QCOMPARE(vlc_ml_add_folder(m_library, uri.constData()), VLC_SUCCESS);
        QTRY_COMPARE_WITH_TIMEOUT(vlc_ml_count_audio_media(m_library,nullptr),size_t(5),45000);
        QTRY_COMPARE_WITH_TIMEOUT(vlc_ml_count_video_media(m_library,nullptr),size_t(1),45000);
        // Wait for tags to be committed rather than accepting initial placeholders.
        auto params = vlc_ml_query_params_create(); params.psz_pattern="First light";
        QTRY_COMPARE_WITH_TIMEOUT(vlc_ml_count_audio_media(m_library,&params),size_t(1),45000);
    }
    void audioFolderMembership()
    {
        QCOMPARE(vlc_ml_count_folders_by_type(m_library,nullptr,VLC_ML_MEDIA_TYPE_AUDIO),size_t(3));
        FolderList folders(vlc_ml_list_folders_by_type(m_library,nullptr,VLC_ML_MEDIA_TYPE_AUDIO),vlc_ml_folder_list_release);
        QVERIFY(folders); QCOMPARE(folders->i_nb_items,size_t(3));
        QStringList names;
        for (size_t i=0;i<folders->i_nb_items;++i)
        {
            const auto &folder=folders->p_items[i]; names.append(QString::fromUtf8(folder.psz_name));
            QVERIFY(folder.i_nb_audio>0);
            if (QString::fromUtf8(folder.psz_name)==QStringLiteral("My Songs"))
            {
                m_folder=folder.i_id; QCOMPARE(folder.i_nb_audio,unsigned(3));
                QCOMPARE(folder.i_nb_video,unsigned(1));
            }
        }
        QVERIFY(names.contains(QStringLiteral("Nested collection")));
        QVERIFY(names.contains(QStringLiteral("Näive Songs")));
        QVERIFY(m_folder>0);
    }
    void typedFolderPagingAndSearch()
    {
        QVERIFY(m_folder>0);
        auto params=vlc_ml_query_params_create(); params.i_sort=VLC_ML_SORTING_ALPHA;
        QCOMPARE(vlc_ml_count_media_of(m_library,&params,VLC_ML_PARENT_FOLDER,m_folder),size_t(4));
        QCOMPARE(vlc_ml_count_audio_of(m_library,&params,VLC_ML_PARENT_FOLDER,m_folder),size_t(3));
        MediaList all(vlc_ml_list_audio_of(m_library,&params,VLC_ML_PARENT_FOLDER,m_folder),vlc_ml_media_list_release);
        QVERIFY(all); QCOMPARE(all->i_nb_items,size_t(3));
        for (size_t i=0;i<all->i_nb_items;++i) QCOMPARE(all->p_items[i].i_type,VLC_ML_MEDIA_TYPE_AUDIO);
        params.i_offset=1; params.i_nbResults=1;
        MediaList page(vlc_ml_list_audio_of(m_library,&params,VLC_ML_PARENT_FOLDER,m_folder),vlc_ml_media_list_release);
        QVERIFY(page); QCOMPARE(titles(page.get()),QStringList{QStringLiteral("First light")});
        params=vlc_ml_query_params_create(); params.psz_pattern="Video";
        QCOMPARE(vlc_ml_count_audio_of(m_library,&params,VLC_ML_PARENT_FOLDER,m_folder),size_t(0));
        QCOMPARE(vlc_ml_count_media_of(m_library,&params,VLC_ML_PARENT_FOLDER,m_folder),size_t(1));
        params.psz_pattern="Firs";
        QCOMPARE(vlc_ml_count_audio_of(m_library,&params,VLC_ML_PARENT_FOLDER,m_folder),size_t(1));
    }
    void mediaTrackOrderDistinctFromCollectionCount()
    {
        QVERIFY(m_folder>0);
        auto params=vlc_ml_query_params_create(); params.b_desc=true;
        params.i_sort=qEnvironmentVariableIsSet("VLC_MUSIC_OLD_TRACK_CRITERION")
            ? VLC_ML_SORTING_TRACKNUMBER : VLC_ML_SORTING_MEDIA_TRACKNUMBER;
        MediaList result(vlc_ml_list_audio_of(m_library,&params,VLC_ML_PARENT_FOLDER,m_folder),vlc_ml_media_list_release);
        QVERIFY(result);
        const QString longTitle=QStringLiteral("A long title for checking truncation and focus in the redesigned music library");
        QCOMPARE(titles(result.get()),(QStringList{longTitle,QStringLiteral("Second light"),QStringLiteral("First light")}));
        params.i_offset=1; params.i_nbResults=1;
        MediaList page(vlc_ml_list_audio_of(m_library,&params,VLC_ML_PARENT_FOLDER,m_folder),vlc_ml_media_list_release);
        QVERIFY(page); QCOMPARE(titles(page.get()),QStringList{QStringLiteral("Second light")});
    }
    void legacyArtistTrackCountRetained()
    {
        auto params=vlc_ml_query_params_create(); params.i_sort=VLC_ML_SORTING_TRACKNUMBER;
        ArtistList artists(vlc_ml_list_artists(m_library,&params,true),vlc_ml_artist_list_release);
        QVERIFY(artists);
        unsigned previous=0, tagged=0;
        for (size_t i=0;i<artists->i_nb_items;++i)
        {
            const auto &artist=artists->p_items[i];
            QVERIFY(artist.i_nb_tracks>=previous); previous=artist.i_nb_tracks;
            if (artist.i_nb_tracks>0) ++tagged;
        }
        QCOMPARE(tagged,unsigned(3));
    }
    void cleanupTestCase()
    {
        if (m_instance) libvlc_release(m_instance);
        m_instance=nullptr; m_library=nullptr;
    }
};
QTEST_GUILESS_MAIN(TestMusicLibraryQueries)
#include "test_music_library_queries.moc"
