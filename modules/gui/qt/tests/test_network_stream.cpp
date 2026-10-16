// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "dialogs/network_stream_model.hpp"
#include <vlc/vlc.h>
#include <vlc_media_library.h>
#include <QSignalSpy>
#include <QTest>
#include <atomic>
#include <cmath>

extern "C" vlc_playlist_t *vlc_test_stream_playlist(libvlc_instance_t *);
extern "C" vlc_medialibrary_t *vlc_test_stream_library(libvlc_instance_t *);
extern "C" void vlc_test_stream_audio(vlc_playlist_t *,
    void (*)(void *, const void *, unsigned, int64_t), void *);

class TestNetworkStream : public QObject
{
    Q_OBJECT
    libvlc_instance_t *m_instance = nullptr;
    vlc_playlist_t *m_playlist = nullptr;
    vlc_medialibrary_t *m_library = nullptr;
    std::unique_ptr<NetworkStreamModel> m_model;
    QString m_uri;
    qint64 m_savedId = 0;
    std::atomic_uint m_buffers{0};
    std::atomic<float> m_rms{0};
    static void audio(void *opaque, const void *data, unsigned count, int64_t)
    {
        auto *self = static_cast<TestNetworkStream *>(opaque);
        const auto *values = static_cast<const float *>(data);
        double sum = 0;
        for (unsigned i = 0; i < count; ++i) sum += values[i] * values[i];
        self->m_rms = count ? std::sqrt(sum / count) : 0;
        ++self->m_buffers;
    }
    libvlc_instance_t *instance()
    {
        const char *args[] = {"--ignore-config", "--media-library", "--no-auto-preparse",
            "--aout=amem,none", "--no-video", "--verbose=2", "--file-logging", "--logfile=network-stream-native.log"};
        return libvlc_new(ARRAY_SIZE(args), args);
    }
    QStringList queueUris()
    {
        QStringList uris;
        vlc_playlist_Lock(m_playlist);
        for (size_t i = 0; i < vlc_playlist_Count(m_playlist); ++i)
        {
            auto *item = vlc_playlist_item_GetMedia(vlc_playlist_Get(m_playlist, i));
            char *uri = input_item_GetURI(item);
            uris.append(QString::fromUtf8(uri));
            free(uri);
        }
        vlc_playlist_Unlock(m_playlist);
        return uris;
    }
private slots:
    void initTestCase()
    {
        m_uri = qEnvironmentVariable("VLC_STREAM_FIXTURE_URL");
        QVERIFY2(m_uri.startsWith("http://127.0.0.1:"), "Use the isolated loopback HTTP fixture runner");
        QVERIFY(!qEnvironmentVariable("VLC_USERDATA_PATH").isEmpty());
        m_instance = instance();
        QVERIFY(m_instance);
        m_library = vlc_test_stream_library(m_instance);
        QVERIFY(m_library);
        QCOMPARE(vlc_ml_count_media(m_library, nullptr), size_t(0));
        QCOMPARE(vlc_ml_count_playlists(m_library, nullptr, VLC_ML_PLAYLIST_TYPE_ALL), size_t(0));
        m_playlist = vlc_test_stream_playlist(m_instance);
        QVERIFY(m_playlist);
        vlc_test_stream_audio(m_playlist, audio, this);
        m_model = std::make_unique<NetworkStreamModel>(m_playlist, m_library);
    }
    void validationAndUnavailableLibrary()
    {
        for (const auto &value : {"relative/path", "https:///missing-host", "file:///C:/media.wav", "https://example.invalid/%wrong"})
        {
            m_model->setUrlText(value);
            QVERIFY(!m_model->canSubmit());
            QVERIFY(!m_model->error().isEmpty());
        }
        m_model->setUrlText("\n\r\nhttps://example.invalid/a\ninvalid");
        QVERIFY(m_model->error().contains("4"));
        m_model->setUrlText("udp://@:1234");
        QVERIFY(m_model->canSubmit());
        m_model->setUrlText(QString("https://example.invalid/a\n").repeated(513));
        QVERIFY(!m_model->canSubmit());
        m_model->setUrlText(QString(65537, 'x'));
        QVERIFY(!m_model->canSubmit());
        NetworkStreamModel withoutLibrary(m_playlist, nullptr);
        withoutLibrary.setUrlText(m_uri);
        QVERIFY(withoutLibrary.canSubmit());
        withoutLibrary.setPlaylistName("Unavailable playlist");
        QVERIFY(!withoutLibrary.canSubmit());
        QVERIFY(!withoutLibrary.error().isEmpty());
        withoutLibrary.submit(false);
        QVERIFY(queueUris().isEmpty());
        m_model->reset();
        QVERIFY(!m_model->canSubmit());
        QVERIFY(m_model->error().isEmpty());
    }
    void unnamedQueueOrderingAndDuplicateActivation()
    {
        const QString second = m_uri + "?second=1";
        m_model->setUrlText(m_uri + "\r\n\n" + second + "\r" + m_uri);
        QVERIFY(m_model->canSubmit());
        QSignalSpy result(m_model.get(), &NetworkStreamModel::finished);
        m_model->submit(false);
        QCOMPARE(result.count(), 1);
        QVERIFY(result.at(0)[0].toBool());
        QCOMPARE(queueUris(), QStringList({m_uri, second, m_uri}));
        m_model->submit(false);
        QCOMPARE(result.count(), 1);
        QCOMPARE(queueUris().size(), 3);
        QCOMPARE(vlc_ml_count_playlists(m_library, nullptr, VLC_ML_PLAYLIST_TYPE_ALL), size_t(0));
    }
    void namedPlaylistSaveAndActualHttpPlayback()
    {
        m_model->reset();
        const QString second = m_uri + "?saved=1";
        m_model->setUrlText(m_uri + '\n' + second + '\n' + m_uri);
        m_model->setPlaylistName("  Slice streams – native  ");
        QSignalSpy result(m_model.get(), &NetworkStreamModel::finished);
        QVERIFY(QMetaObject::invokeMethod(m_model.get(), "submit", Qt::DirectConnection));
        QVERIFY(m_model->busy());
        m_model->setUrlText("https://example.invalid/should-not-replace-pending");
        m_model->setPlaylistName("Wrong title");
        m_model->reset();
        m_model->submit();
        QTRY_COMPARE_WITH_TIMEOUT(result.count(), 1, 15000);
        QVERIFY2(result.at(0)[0].toBool(), qPrintable(m_model->error()));
        QCOMPARE(queueUris(), QStringList({m_uri, m_uri + "?second=1", m_uri, m_uri, second, m_uri}));
        auto *playlists = vlc_ml_list_playlists(m_library, nullptr, VLC_ML_PLAYLIST_TYPE_ALL);
        QVERIFY(playlists);
        QCOMPARE(playlists->i_nb_items, size_t(1));
        QCOMPARE(QString::fromUtf8(playlists->p_items[0].psz_name), QString::fromUtf8("Slice streams – native"));
        m_savedId = playlists->p_items[0].i_id;
        vlc_ml_playlist_list_release(playlists);
        auto *items = vlc_ml_list_playlist_media(m_library, nullptr, m_savedId);
        QVERIFY(items);
        QCOMPARE(items->i_nb_items, size_t(3));
        QCOMPARE(items->p_items[0].i_id, items->p_items[2].i_id);
        QVERIFY(items->p_items[0].i_id != items->p_items[1].i_id);
        vlc_ml_media_list_release(items);
        QTRY_VERIFY_WITH_TIMEOUT(m_buffers > 3, 15000);
        QVERIFY(m_rms > .01f);
        qInfo("Actual HTTP audio: buffers=%u RMS=%.6f", m_buffers.load(), m_rms.load());
        vlc_playlist_Lock(m_playlist);
        const auto current = vlc_playlist_GetCurrentIndex(m_playlist);
        vlc_playlist_Stop(m_playlist);
        vlc_playlist_Unlock(m_playlist);
        QCOMPARE(current, ssize_t(3));
    }
    void restartPersistence()
    {
        m_model.reset();
        vlc_playlist_Delete(m_playlist);
        m_playlist = nullptr;
        libvlc_release(m_instance);
        m_instance = instance();
        QVERIFY(m_instance);
        m_library = vlc_test_stream_library(m_instance);
        QVERIFY(m_library);
        auto *playlist = vlc_ml_get_playlist(m_library, m_savedId);
        QVERIFY(playlist);
        QCOMPARE(QString::fromUtf8(playlist->psz_name), QString::fromUtf8("Slice streams – native"));
        vlc_ml_playlist_release(playlist);
        auto *items = vlc_ml_list_playlist_media(m_library, nullptr, m_savedId);
        QVERIFY(items);
        QCOMPARE(items->i_nb_items, size_t(3));
        QCOMPARE(items->p_items[0].i_id, items->p_items[2].i_id);
        vlc_ml_media_list_release(items);
    }
    void cleanupTestCase()
    {
        m_model.reset();
        if (m_playlist) vlc_playlist_Delete(m_playlist);
        if (m_instance) libvlc_release(m_instance);
    }
};
QTEST_GUILESS_MAIN(TestNetworkStream)
#include "test_network_stream.moc"
