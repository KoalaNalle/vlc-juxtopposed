// SPDX-License-Identifier: GPL-2.0-or-later
// Production QObject/worker lifetime with real ML; no duplicate edit algorithm.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "medialibrary/mlrecentvideomodel.hpp"
#include "medialibrary/mlvideohistory.hpp"
#include "maininterface/compositor.hpp" // completes the unused context member
#include <vlc/vlc.h>
#include <vlc_playlist.h>
#include <QPointer>
#include <QSemaphore>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QThread>
#include <QTimer>
#include <atomic>
#include <limits>
#include <thread>

extern "C" vlc_medialibrary_t *vlc_test_stream_library(libvlc_instance_t *);
extern "C" vlc_playlist_t *vlc_test_stream_playlist(libvlc_instance_t *);
extern "C" const char vlc_module_name[] = "history_worker";

// Media::removeFromHistory -> MediaLibrary::onHistoryChanged ->
// vlc_ml_event_send invokes this callback synchronously on the mutating thread.
// Delay only this notification; never replace a database operation or worker.
class HistoryEventGate
{
public:
    explicit HistoryEventGate(vlc_medialibrary_t *library) : m_library(library)
    {
        m_callback = vlc_ml_event_register_callback(library,
            [](void *opaque, const vlc_ml_event_t *event) {
                auto &gate = *static_cast<HistoryEventGate *>(opaque);
                if (event->i_type != VLC_ML_EVENT_HISTORY_CHANGED || !gate.m_armed.exchange(false))
                    return;
                gate.thread = QThread::currentThread();
                gate.entered = true;
                if (!gate.m_proceed.tryAcquire(1, 10000))
                    gate.timedOut = true;
            }, this);
    }
    ~HistoryEventGate()
    {
        m_armed = false;
        release(); // Even a failed assertion cannot leave a blocked worker.
        if (m_callback)
            vlc_ml_event_unregister_callback(m_library, m_callback);
    }
    bool valid() const { return m_callback != nullptr; }
    void release() { m_proceed.release(); }
    std::atomic_bool entered{false}, timedOut{false};
    std::atomic<QThread *> thread{nullptr};
private:
    vlc_medialibrary_t *m_library;
    vlc_ml_event_callback_t *m_callback = nullptr;
    QSemaphore m_proceed;
    std::atomic_bool m_armed{true};
};

class TestHistoryWorker : public QObject
{
    Q_OBJECT
    libvlc_instance_t *m_instance = nullptr;
    vlc_medialibrary_t *m_library = nullptr;
    vlc_playlist_t *m_playlist = nullptr;
    // The existing history-source factory only borrows p_player. This is not a
    // constructed MainCtx/interface; the backend and primary player are real.
    qt_intf_t m_context{};
    std::unique_ptr<MediaLib> m_service;
    QTemporaryDir m_names;
    std::vector<int64_t> m_ids;

    static void modes()
    {
        QTest::addColumn<bool>("completed");
        QTest::newRow("continue") << false;
        QTest::newRow("watch-again") << true;
    }
    bool seed(bool completed)
    {
        if (vlc_ml_clear_history(m_library, VLC_ML_HISTORY_TYPE_GLOBAL) != VLC_SUCCESS)
            return false;
        for (size_t i = 0; i < m_ids.size(); ++i)
        {
            if (vlc_ml_media_set_played(m_library, m_ids[i], false) != VLC_SUCCESS)
                return false;
            const bool watched = i < 3 ? completed : !completed;
            if ((watched ? vlc_ml_media_set_played(m_library, m_ids[i], true)
                         : vlc_ml_media_update_progress(m_library, m_ids[i], .4)) != VLC_SUCCESS)
                return false;
        }
        return true;
    }
    size_t count(bool completed) const
    {
        return visitMLVideoHistory(m_library, nullptr, completed,
            [](const auto &, size_t) { return true; });
    }
    std::unique_ptr<MLRecentVideoModel> model(bool completed)
    {
        auto result = std::make_unique<MLRecentVideoModel>();
        result->classBegin();
        result->setMl(m_service.get());
        result->setHistoryMode(completed ? MLRecentVideoModel::Completed : MLRecentVideoModel::Unfinished);
        result->componentComplete();
        return result;
    }
    std::shared_ptr<std::atomic_bool> observeThreadFinish(QThread *thread)
    {
        auto finished = std::make_shared<std::atomic_bool>(false);
        connect(thread, &QThread::finished, this,
            [finished] { *finished = true; }, Qt::DirectConnection);
        return finished;
    }
private slots:
    void initTestCase()
    {
        QVERIFY2(!qEnvironmentVariable("VLC_USERDATA_PATH").isEmpty(), "Use fresh isolated VLC_USERDATA_PATH");
        QVERIFY(m_names.isValid());
        const char *args[] = {"--ignore-config", "--media-library", "--no-auto-preparse",
            "--no-ml-network-discovery", "--no-metadata-network-access", "--no-video",
            "--vout=dummy", "--aout=dummy", "--verbose=2", "--file-logging",
            "--logfile=history-worker-native.log"};
        m_instance = libvlc_new(ARRAY_SIZE(args), args); QVERIFY(m_instance);
        m_library = vlc_test_stream_library(m_instance); QVERIFY(m_library);
        QCOMPARE(vlc_ml_count_video_history(m_library, nullptr), size_t(0));
        m_playlist = vlc_test_stream_playlist(m_instance); QVERIFY(m_playlist);
        m_context.p_player = vlc_playlist_GetPlayer(m_playlist);
        m_service = MediaLib::createHistorySource(&m_context, nullptr, m_library, nullptr);
        QVERIFY(m_service);
        for (int i = 0; i < 4; ++i)
        {
            const auto uri = QUrl::fromLocalFile(m_names.filePath(QString("owned-worker-%1.mp4").arg(i))).toEncoded();
            ml_unique_ptr<vlc_ml_media_t> media(vlc_ml_new_external_media(m_library, uri.constData()));
            QVERIFY(media);
            QCOMPARE(vlc_ml_media_set_type(m_library, media->i_id, VLC_ML_MEDIA_TYPE_VIDEO), VLC_SUCCESS);
            m_ids.push_back(media->i_id);
        }
    }
    void busyStateAndCapturedMode_data() { modes(); }
    void busyStateAndCapturedMode()
    {
        QFETCH(bool, completed);
        QVERIFY(seed(completed));
        QCOMPARE(count(completed), size_t(3)); QCOMPARE(count(!completed), size_t(1));
        auto source = model(completed);
        HistoryEventGate gate(m_library); QVERIFY(gate.valid());
        QSignalSpy finished(source.get(), &MLRecentVideoModel::historyEditFinished);
        QList<bool> busyChanges, busyAtFinish;
        connect(source.get(), &MLRecentVideoModel::historyBusyChanged, this,
            [&] { busyChanges.push_back(source->historyBusy()); });
        connect(source.get(), &MLRecentVideoModel::historyEditFinished, this,
            [&](bool, const QString &) { busyAtFinish.push_back(source->historyBusy()); });
        source->clearHistory();
        QVERIFY(source->historyBusy());
        QTRY_VERIFY_WITH_TIMEOUT(gate.entered.load(), 5000);
        auto *thread = gate.thread.load(); QVERIFY(thread);
        QVERIFY(thread != QThread::currentThread()); QVERIFY(thread->isRunning());
        const auto workerFinished = observeThreadFinish(thread);
        QPointer<QThread> workerObject(thread);
        const auto heartbeat = std::make_shared<std::atomic_bool>(false);
        QTimer::singleShot(0, this, [heartbeat] { *heartbeat = true; });
        QTRY_VERIFY_WITH_TIMEOUT(heartbeat->load(), 1000);
        QVERIFY(source->historyBusy()); QVERIFY(!workerFinished->load());
        // A live view-mode change must not retarget the already accepted edit.
        source->setHistoryMode(completed ? MLRecentVideoModel::Unfinished : MLRecentVideoModel::Completed);
        source->clearHistory(); // Reject another operation while busy.
        QCOMPARE(finished.count(), qsizetype(1));
        QVERIFY(!finished.at(0).at(0).toBool()); QVERIFY(!finished.at(0).at(1).toString().isEmpty());
        QVERIFY(source->historyBusy());
        gate.release();
        QTRY_COMPARE_WITH_TIMEOUT(finished.count(), qsizetype(2), 10000);
        QVERIFY(finished.at(1).at(0).toBool()); QVERIFY(finished.at(1).at(1).toString().isEmpty());
        QVERIFY(!source->historyBusy()); QVERIFY(!gate.timedOut.load());
        QVERIFY(workerFinished->load()); QVERIFY(workerObject.isNull());
        QCOMPARE(busyChanges, (QList<bool>{true, false}));
        QCOMPARE(busyAtFinish, (QList<bool>{true, false}));
        QCOMPARE(count(completed), size_t(0)); QCOMPARE(count(!completed), size_t(1));
        QTRY_COMPARE_WITH_TIMEOUT(source->getCount(), unsigned(1), 5000);
    }
    void workerFailureAndRetry_data() { modes(); }
    void workerFailureAndRetry()
    {
        QFETCH(bool, completed);
        QVERIFY(seed(completed));
        auto source = model(completed);
        QSignalSpy finished(source.get(), &MLRecentVideoModel::historyEditFinished);
        QSignalSpy busy(source.get(), &MLRecentVideoModel::historyBusyChanged);
        // A real backend lookup fails on the worker. This is a logical missing
        // item, not injected disk corruption or a pretend database write error.
        source->removeFromHistory({std::numeric_limits<int64_t>::max(), VLC_ML_PARENT_UNKNOWN});
        QVERIFY(source->historyBusy());
        QTRY_COMPARE_WITH_TIMEOUT(finished.count(), qsizetype(1), 10000);
        QVERIFY(!finished.at(0).at(0).toBool()); QVERIFY(!finished.at(0).at(1).toString().isEmpty());
        QVERIFY(!source->historyBusy()); QCOMPARE(busy.count(), qsizetype(2));
        QCOMPARE(count(completed), size_t(3)); QCOMPARE(count(!completed), size_t(1));
        // The same QObject can retry successfully after its worker error.
        source->removeFromHistory({m_ids.front(), VLC_ML_PARENT_UNKNOWN});
        QVERIFY(source->historyBusy());
        QTRY_COMPARE_WITH_TIMEOUT(finished.count(), qsizetype(2), 10000);
        QVERIFY(finished.at(1).at(0).toBool()); QVERIFY(finished.at(1).at(1).toString().isEmpty());
        QVERIFY(!source->historyBusy()); QCOMPARE(busy.count(), qsizetype(4));
        QCOMPARE(count(completed), size_t(2)); QCOMPARE(count(!completed), size_t(1));
    }
    void deletionDrainsActiveWorker_data() { modes(); }
    void deletionDrainsActiveWorker()
    {
        QFETCH(bool, completed);
        QVERIFY(seed(completed));
        auto source = model(completed);
        HistoryEventGate gate(m_library); QVERIFY(gate.valid());
        QSignalSpy finished(source.get(), &MLRecentVideoModel::historyEditFinished);
        QPointer<MLRecentVideoModel> modelObject(source.get());
        source->clearHistory();
        QTRY_VERIFY_WITH_TIMEOUT(gate.entered.load(), 5000);
        QVERIFY(source->historyBusy()); QCOMPARE(finished.count(), qsizetype(0));
        auto *thread = gate.thread.load(); QVERIFY(thread);
        QVERIFY(thread != QThread::currentThread()); QVERIFY(thread->isRunning());
        const auto workerFinished = observeThreadFinish(thread);
        QPointer<QThread> workerObject(thread);
        QVERIFY(!workerFinished->load());
        std::atomic_bool released{false};
        // Delete on the QObject's own Qt thread. An independent bounded releaser
        // lets its real destructor join the worker while the callback is held.
        std::thread releaseLater([&] {
            QThread::msleep(150);
            released = true;
            gate.release();
        });
        source.reset();
        const bool returnedAfterRelease = released.load();
        releaseLater.join(); // No test assertion may bypass joining this thread.
        QVERIFY(returnedAfterRelease); QVERIFY(workerFinished->load());
        QVERIFY(workerObject.isNull()); QVERIFY(modelObject.isNull());
        QVERIFY(!gate.timedOut.load());
        QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
        QCoreApplication::processEvents();
        QCOMPARE(finished.count(), qsizetype(0)); // No queued owner callback after deletion.
        QCOMPARE(count(completed), size_t(0)); QCOMPARE(count(!completed), size_t(1));
        for (const auto id : m_ids)
        {
            ml_unique_ptr<vlc_ml_media_t> retained(vlc_ml_get_media(m_library, id));
            QVERIFY(retained); // Completing accepted history work does not delete media.
        }
        qInfo("Model deletion joined a real gated history worker; accepted batch completed, opposite history and indexed media retained");
    }
    void cleanupTestCase()
    {
        m_service.reset();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QCoreApplication::processEvents();
        if (m_playlist) vlc_playlist_Delete(m_playlist);
        if (m_instance) libvlc_release(m_instance);
        m_context.p_player = nullptr;
        m_playlist = nullptr; m_instance = nullptr; m_library = nullptr;
    }
};
QTEST_GUILESS_MAIN(TestHistoryWorker)
#include "test_history_worker.moc"
