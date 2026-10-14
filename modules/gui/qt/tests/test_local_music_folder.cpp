// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "player/local_music_folder.hpp"
#include <vlc/vlc.h>
#include <vlc_playlist.h>
#include <QDataStream>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>
#include <cmath>

extern "C" vlc_object_t *vlc_test_media_player_object(libvlc_media_player_t *);
extern "C" vlc_playlist_t *vlc_test_stream_playlist(libvlc_instance_t *);
extern "C" void vlc_test_stream_audio(vlc_playlist_t *,
    void (*)(void *, const void *, unsigned, int64_t), void *);

class TestLocalMusicFolder : public QObject
{
    Q_OBJECT
    libvlc_instance_t *m_instance = nullptr;
    libvlc_media_player_t *m_player = nullptr;
    vlc_playlist_t *m_playlist = nullptr;
    std::unique_ptr<LocalMusicFolderModel> m_model;
    QTemporaryDir m_directory;
    QUrl m_first, m_second, m_bad;
    std::atomic_uint m_buffers{0};
    static void audio(void *opaque, const void *, unsigned, int64_t)
    { ++static_cast<TestLocalMusicFolder *>(opaque)->m_buffers; }
    QUrl wav(const QString &name, const QByteArray &title, const QByteArray &artist)
    {
        const auto path = m_directory.filePath(name);
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) return {};
        QDataStream stream(&file);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream.writeRawData("RIFF", 4);
        stream << quint32(0);
        stream.writeRawData("WAVEfmt ", 8);
        stream << quint32(16) << quint16(1) << quint16(1) << quint32(44100)
               << quint32(88200) << quint16(2) << quint16(16);
        stream.writeRawData("data", 4);
        stream << quint32(44100 * 2 * 2);
        for (int i = 0; i < 44100 * 2; ++i)
            stream << qint16(12000 * std::sin(2 * M_PI * 220 * i / 44100));
        QByteArray info("INFO", 4);
        QDataStream tags(&info, QIODevice::Append);
        tags.setByteOrder(QDataStream::LittleEndian);
        for (const auto &tag : {QPair<QByteArray, QByteArray>{"INAM", title}, {"IART", artist}})
        {
            tags.writeRawData(tag.first.constData(), 4);
            const QByteArray text = tag.second + '\0';
            tags << quint32(text.size());
            tags.writeRawData(text.constData(), text.size());
            if (text.size() % 2) tags << quint8(0);
        }
        stream.writeRawData("LIST", 4);
        stream << quint32(info.size());
        stream.writeRawData(info.constData(), info.size());
        const auto size = file.size();
        file.seek(4);
        stream << quint32(size - 8);
        return QUrl::fromLocalFile(path);
    }
    QUrl url(int row) const
    { return m_model->data(m_model->index(row), LocalMusicFolderModel::UrlRole).toUrl(); }
    int find(const QUrl &value) const
    {
        for (int i = 0; i < m_model->rowCount(); ++i) if (url(i) == value) return i;
        return -1;
    }
    size_t queueCount() const
    {
        vlc_playlist_Lock(m_playlist);
        const auto count = vlc_playlist_Count(m_playlist);
        vlc_playlist_Unlock(m_playlist);
        return count;
    }
private slots:
    void initTestCase()
    {
        QVERIFY(m_directory.isValid());
        m_first = wav("source '10.wav", "A genuine title", "Fixture artist");
        m_second = wav("source 2.WAV", "Z genuine title", "Second artist");
        QVERIFY(!m_first.isEmpty() && !m_second.isEmpty());
        QFile bad(m_directory.filePath("corrupt.wav"));
        QVERIFY(bad.open(QIODevice::WriteOnly));
        bad.write("invalid audio"); bad.close();
        m_bad = QUrl::fromLocalFile(bad.fileName());
        QFile ignored(m_directory.filePath("ignored.txt"));
        QVERIFY(ignored.open(QIODevice::WriteOnly)); ignored.write("not media"); ignored.close();
        QVERIFY(QDir(m_directory.path()).mkdir("nested"));
        QVERIFY(QFile::copy(m_first.toLocalFile(), m_directory.filePath("nested/hidden.wav")));
        const char *args[] = {"--ignore-config", "--no-media-library", "--no-auto-preparse",
            "--aout=amem,none", "--no-video", "--verbose=2", "--file-logging", "--logfile=music-folder-native.log"};
        m_instance = libvlc_new(ARRAY_SIZE(args), args);
        QVERIFY(m_instance);
        m_player = libvlc_media_player_new(m_instance, nullptr, nullptr);
        QVERIFY(m_player);
        m_playlist = vlc_test_stream_playlist(m_instance);
        QVERIFY(m_playlist);
        vlc_test_stream_audio(m_playlist, audio, this);
        m_model = std::make_unique<LocalMusicFolderModel>(vlc_test_media_player_object(m_player), m_playlist);
    }
    void sourceFolderAndRealMetadata()
    {
        m_model->setSourceUrl(m_first);
        QVERIFY(!m_model->available());
        m_model->setEnabled(true);
        QVERIFY(m_model->available());
        QTRY_VERIFY_WITH_TIMEOUT(!m_model->busy(), 15000);
        QCOMPARE(m_model->rowCount(), 3);
        QCOMPARE(m_model->folderUrl(), QUrl::fromLocalFile(m_directory.path()));
        QCOMPARE(m_model->currentIndex(), find(m_first));
        const auto first = m_model->index(find(m_first));
        QCOMPARE(m_model->data(first, LocalMusicFolderModel::TitleRole).toString(), "A genuine title");
        QCOMPARE(m_model->data(first, LocalMusicFolderModel::ArtistRole).toString(), "Fixture artist");
        QVERIFY(m_model->data(first, LocalMusicFolderModel::ArtworkRole).toUrl().isEmpty());
        QCOMPARE(m_model->data(m_model->index(find(m_bad)), LocalMusicFolderModel::TitleRole).toString(), "corrupt.wav");
        QVERIFY2(!m_model->error().isEmpty(), "Failed metadata must be reported while readable tagged entries remain available");
    }
    void searchSortAndSameFolderTrackChange()
    {
        m_model->setSearchPattern("fixture ARTIST");
        QCOMPARE(m_model->rowCount(), 1);
        QCOMPARE(url(0), m_first);
        m_model->setSourceUrl(m_second);
        QVERIFY(!m_model->busy());
        QCOMPARE(m_model->currentIndex(), -1);
        m_model->setSearchPattern("");
        m_model->setSortAscending(false);
        QCOMPARE(url(0), m_second);
        QCOMPARE(m_model->currentIndex(), 0);
        m_model->setSortAscending(true);
        QCOMPARE(url(0), m_first);
    }
    void actualQueuePlaybackAndReuse()
    {
        QVERIFY(!m_model->addAndPlay(-1));
        QVERIFY(!m_model->addAndPlay(100));
        QVERIFY(m_model->addAndPlay(find(m_first)));
        QTRY_VERIFY_WITH_TIMEOUT(m_buffers > 2, 10000);
        QCOMPARE(queueCount(), size_t(1));
        QVERIFY(m_model->addAndPlay(find(m_first)));
        QCOMPARE(queueCount(), size_t(1));
        QVERIFY(m_model->addAndPlay(find(m_second)));
        QCOMPARE(queueCount(), size_t(2));
        vlc_playlist_Lock(m_playlist);
        const auto selected = vlc_playlist_GetCurrentIndex(m_playlist);
        char *firstUri = input_item_GetURI(vlc_playlist_item_GetMedia(vlc_playlist_Get(m_playlist, 0)));
        char *secondUri = input_item_GetURI(vlc_playlist_item_GetMedia(vlc_playlist_Get(m_playlist, 1)));
        const QUrl queuedFirst = QUrl::fromEncoded(firstUri);
        const QUrl queuedSecond = QUrl::fromEncoded(secondUri);
        free(firstUri);
        free(secondUri);
        vlc_playlist_Stop(m_playlist);
        vlc_playlist_Unlock(m_playlist);
        QCOMPARE(selected, ssize_t(1));
        QCOMPARE(queuedFirst, m_first);
        QCOMPARE(queuedSecond, m_second);
    }
    void missingRefreshStaleSourceAndShutdown()
    {
        QVERIFY(QFile::remove(m_bad.toLocalFile()));
        QVERIFY(!m_model->addAndPlay(find(m_bad)));
        QVERIFY(!m_model->error().isEmpty());
        m_model->refresh();
        QTRY_VERIFY_WITH_TIMEOUT(!m_model->busy(), 15000);
        QCOMPARE(m_model->rowCount(), 2);
        QVERIFY(m_model->error().isEmpty());
        m_model->setSourceUrl(QUrl("https://example.invalid/music.wav"));
        QVERIFY(!m_model->available());
        QCOMPARE(m_model->rowCount(), 0);
        m_model->setSourceUrl(QUrl("file:////unavailable-server/share/audio.wav"));
        QVERIFY(!m_model->available());
        QCOMPARE(m_model->rowCount(), 0);
        m_model->setSourceUrl(m_first);
        m_model->refresh();
        m_model->setSourceUrl(QUrl::fromLocalFile(m_directory.filePath("nested/hidden.wav")));
        QTRY_VERIFY_WITH_TIMEOUT(!m_model->busy(), 15000);
        QCOMPARE(m_model->rowCount(), 1);
        QCOMPARE(m_model->name(), "nested");
        m_model->refresh();
        QElapsedTimer timer; timer.start();
        m_model.reset();
        QVERIFY2(timer.elapsed() < 5000, "Cancellation must join without an unbounded preparser wait");
    }
    void runningMetadataWorkerShutdown()
    {
        QTemporaryDir bulk;
        QVERIFY(bulk.isValid());
        // Enough actual tagged files to observe the running worker, rather than
        // merely deleting a request that has not left the thread-pool queue.
        for (int i = 0; i < 512; ++i)
            QVERIFY(QFile::copy(m_first.toLocalFile(), bulk.filePath(QString("track-%1.wav").arg(i))));
        auto model = std::make_unique<LocalMusicFolderModel>(vlc_test_media_player_object(m_player), m_playlist);
        model->setSourceUrl(QUrl::fromLocalFile(bulk.filePath("track-0.wav")));
        model->setEnabled(true);
        QElapsedTimer waiting;
        waiting.start();
        while (model->rowCount() == 0 && waiting.elapsed() < 5000)
            QTest::qWait(1);
        QCOMPARE(model->rowCount(), 512);
        QVERIFY2(model->busy(), "The native metadata worker must still be running for the shutdown check");
        QElapsedTimer shutdown;
        shutdown.start();
        model.reset();
        QVERIFY2(shutdown.elapsed() < 3000, "Parse-only cancellation must join within the native parsing timeout");
    }
    void cleanupTestCase()
    {
        m_model.reset();
        if (m_playlist) vlc_playlist_Delete(m_playlist);
        if (m_player) libvlc_media_player_release(m_player);
        if (m_instance) libvlc_release(m_instance);
    }
};
QTEST_GUILESS_MAIN(TestLocalMusicFolder)
#include "test_local_music_folder.moc"
