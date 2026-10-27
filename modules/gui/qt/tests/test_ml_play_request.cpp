// SPDX-License-Identifier: GPL-2.0-or-later
// Real service/playlist lifetime with an explicitly delayed row lookup.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "medialibrary/mlbasemodel.hpp"
#include "playlist/playlist_controller.hpp"
#include <vlc/vlc.h>
#include <vlc_playlist.h>
#include <QDir>
#include <QSemaphore>
#include <QTest>
#include <QUrl>

extern "C" vlc_medialibrary_t *vlc_test_stream_library(libvlc_instance_t *);
extern "C" vlc_playlist_t *vlc_test_stream_playlist(libvlc_instance_t *);
extern "C" const char vlc_module_name[] = "ml_play_request";

struct RequestRows : MLListCacheLoader::MLOp
{
    using ItemType = MLListCacheLoader::ItemType;
    explicit RequestRows(const MLBaseModel &model, std::vector<MLItemId> rows)
        : MLOp(model), rows(std::move(rows)) {}
    std::vector<MLItemId> rows;
    mutable QSemaphore entered, release;
    bool delay = false;
    mutable std::vector<std::pair<unsigned, unsigned>> queries;
    size_t count(vlc_medialibrary_t *, const vlc_ml_query_params_t *) const override
    { return rows.size(); }
    ItemType loadItemById(vlc_medialibrary_t *, MLItemId id) const override
    { return std::make_unique<MLItem>(id); }
    std::vector<ItemType> load(vlc_medialibrary_t *, const vlc_ml_query_params_t *query) const override
    {
        if (delay) {
            entered.release();
            // Bound the fixture even when a test assertion fails.
            if (!release.tryAcquire(1, 10000)) return {};
        }
        queries.emplace_back(query->i_offset, query->i_nbResults);
        std::vector<ItemType> result;
        const size_t end = std::min(rows.size(), size_t(query->i_offset) + query->i_nbResults);
        for (size_t i = query->i_offset; i < end; ++i)
            result.emplace_back(std::make_unique<MLItem>(rows[i]));
        return result;
    }
};

class RequestModel : public MLBaseModel
{
public:
    std::shared_ptr<RequestRows> operation;
    QVariant itemRoleData(const MLItem *item, int) const override
    { return item ? QVariant::fromValue(item->getId()) : QVariant{}; }
    QModelIndex selected(int row) const { return createIndex(row, 0); }
protected:
    std::unique_ptr<MLListCacheLoader> createMLLoader() const override
    { return std::make_unique<MLListCacheLoader>(m_mediaLib, operation); }
};

class TestMLPlayRequest : public QObject
{
    Q_OBJECT
    libvlc_instance_t *m_instance = nullptr;
    vlc_medialibrary_t *m_backend = nullptr;
    vlc_playlist_t *m_playlist = nullptr;
    std::unique_ptr<vlc::playlist::PlaylistController> m_controller;
    std::unique_ptr<MediaLib> m_service;
    std::vector<MLItemId> m_rows;
    QStringList m_urls;

    std::unique_ptr<RequestModel> model()
    {
        auto result = std::make_unique<RequestModel>();
        result->classBegin();
        result->setMl(m_service.get());
        result->operation = std::make_shared<RequestRows>(*result, m_rows);
        return result;
    }
    size_t queueCount() const
    {
        vlc::playlist::vlc_playlist_locker lock(m_playlist);
        return vlc_playlist_Count(m_playlist);
    }
    QStringList queueUrls() const
    {
        vlc::playlist::vlc_playlist_locker lock(m_playlist);
        QStringList result;
        for (size_t i = 0; i < vlc_playlist_Count(m_playlist); ++i)
            result.append(QString::fromUtf8(vlc_playlist_item_GetMedia(vlc_playlist_Get(m_playlist, i))->psz_uri));
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
        m_backend = vlc_test_stream_library(m_instance); QVERIFY(m_backend);
        QCOMPARE(vlc_ml_count_media(m_backend, nullptr), size_t(0));
        const auto uri = QUrl::fromLocalFile(fixtures).toEncoded();
        QCOMPARE(vlc_ml_add_folder(m_backend, uri.constData()), VLC_SUCCESS);
        QTRY_COMPARE_WITH_TIMEOUT(vlc_ml_count_audio_media(m_backend, nullptr), size_t(5), 45000);
        auto query = vlc_ml_query_params_create(); query.i_sort = VLC_ML_SORTING_ALPHA;
        auto *media = vlc_ml_list_audio_media(m_backend, &query); QVERIFY(media);
        for (size_t i = 0; i < media->i_nb_items; ++i) {
            m_rows.emplace_back(media->p_items[i].i_id, VLC_ML_PARENT_UNKNOWN);
            auto *input = vlc_ml_get_input_item(m_backend, media->p_items[i].i_id);
            QVERIFY(input); m_urls.append(QString::fromUtf8(input->psz_uri)); input_item_Release(input);
        }
        vlc_ml_media_list_release(media);
        QCOMPARE(m_rows.size(), size_t(5));
        m_playlist = vlc_test_stream_playlist(m_instance); QVERIFY(m_playlist);
        m_controller = std::make_unique<vlc::playlist::PlaylistController>(m_playlist);
        // The existing history-source factory uses the real backend and drains
        // its workers on teardown; it needs no constructed interface pointer.
        m_service = MediaLib::createHistorySource(nullptr, m_controller.get(), m_backend, nullptr);
        QVERIFY(m_service);
    }
    void selectedPlaySurvivesViewRemoval()
    {
        auto source = model();
        auto operation = source->operation; operation->delay = true;
        auto *origin = vlc_ml_playlist_create(m_backend, "Owned request origin"); QVERIFY(origin);
        const auto originId = origin->i_id; vlc_ml_playlist_release(origin);
        source->setParentId({originId, VLC_ML_PARENT_PLAYLIST});
        // Offset and non-sorted selection order are both part of the command.
        source->setOffset(1);
        source->addAndPlay({source->selected(2), source->selected(0)}, {":input-repeat=0"});
        const bool entered = operation->entered.tryAcquire(1, 5000);
        source->setParentId({}); source.reset();
        operation->release.release();
        QVERIFY(entered);
        QTRY_COMPARE_WITH_TIMEOUT(queueCount(), size_t(2), 7000);
        QCOMPARE(queueUrls(), (QStringList{m_urls[3], m_urls[1]}));
        QTRY_COMPARE_WITH_TIMEOUT(m_controller->currentSourcePlaylistId(), originId, 3000);
        QCOMPARE(m_controller->sourcePlaylistItems(originId).size(), qsizetype(2));
        QCOMPARE(operation->queries, (std::vector<std::pair<unsigned, unsigned>>{{1, 3}}));
    }
    void ordinaryReadStillCancelsWithView()
    {
        auto source = model();
        auto operation = source->operation; operation->delay = true;
        auto reader = std::make_unique<MLListCacheLoader>(m_service.get(), operation);
        bool called = false;
        reader->loadItemsTask(0, {0}, [&](quint64, auto &) { called = true; });
        const bool entered = operation->entered.tryAcquire(1, 5000);
        source.reset(); reader.reset();
        std::weak_ptr<RequestRows> finished = operation;
        operation->release.release(); operation.reset();
        QVERIFY(entered);
        QTRY_VERIFY_WITH_TIMEOUT(finished.expired(), 3000);
        QVERIFY(!called);
    }
    void removedAndInvalidRowsAreSkipped()
    {
        auto source = model();
        source->operation->rows.resize(3);
        MLListCacheLoader reader(m_service.get(), source->operation);
        bool called = false;
        std::vector<int64_t> ids;
        reader.loadItemsTask(2, {1, -1, 0}, [&](quint64, auto &items) {
            for (const auto &item : items) ids.push_back(item ? item->getId().id : 0);
            called = true;
        });
        QTRY_VERIFY_WITH_TIMEOUT(called, 3000);
        QCOMPARE(ids, (std::vector<int64_t>{0, 0, m_rows[2].id}));
        QCOMPARE(source->operation->queries, (std::vector<std::pair<unsigned, unsigned>>{{2, 2}}));
    }
    void serviceShutdownCancelsPendingPlay()
    {
        const auto before = queueCount();
        auto source = model();
        auto operation = source->operation; operation->delay = true;
        source->addAndPlay({source->selected(4)});
        const bool entered = operation->entered.tryAcquire(1, 5000);
        source.reset();
        std::weak_ptr<RequestRows> finished = operation;
        operation->release.release(); operation.reset();
        // A normal owner shutdown drains the worker before its queued UI reply.
        m_service.reset();
        QVERIFY(entered);
        QTRY_VERIFY_WITH_TIMEOUT(finished.expired(), 3000);
        QCOMPARE(queueCount(), before);
    }
    void cleanupTestCase()
    {
        m_service.reset();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QCoreApplication::processEvents();
        m_controller.reset();
        if (m_playlist) vlc_playlist_Delete(m_playlist);
        if (m_instance) libvlc_release(m_instance);
        m_playlist = nullptr; m_instance = nullptr; m_backend = nullptr;
    }
};
QTEST_GUILESS_MAIN(TestMLPlayRequest)
#include "test_ml_play_request.moc"
