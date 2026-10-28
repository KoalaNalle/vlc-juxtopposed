/*****************************************************************************
 * Copyright (C) 2019 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * ( at your option ) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#include "mlrecentvideomodel.hpp"
#include "mlhelper.hpp"
#include "mlvideohistory.hpp"
#include <vlc_player.h>
#include <QThread>

namespace {
class HistoryEditThread final : public QThread
{
public:
    HistoryEditThread(vlc_medialibrary_t *ml, vlc_player_t *player,
                      MLVideoHistoryScope scope, std::optional<int64_t> itemId)
        : m_ml(ml), m_player(player), m_scope(scope), m_itemId(itemId) {}
    MLVideoHistoryEditResult result;
private:
    void run() override
    {
        result = editMLVideoHistory(m_ml, m_scope, m_itemId, [this](int64_t id) {
            return removeMLVideoHistoryItem(m_ml, m_player, m_scope, id);
        });
    }
    vlc_medialibrary_t *m_ml;
    vlc_player_t *m_player;
    const MLVideoHistoryScope m_scope;
    const std::optional<int64_t> m_itemId;
};
}

// Ctor / dtor

MLRecentVideoModel::MLRecentVideoModel(QObject * parent) : MLVideoModel(parent) {}

MLRecentVideoModel::~MLRecentVideoModel()
{
    if (m_historyWorker)
    {
        // These workers borrow the main player/backend. Drain them while their
        // owning interface still lives, including close during a slow clear.
        m_historyWorker->disconnect(this);
        m_historyWorker->wait();
        delete m_historyWorker;
    }
}

void MLRecentVideoModel::clearHistory()
{
    editHistory(std::nullopt);
}

void MLRecentVideoModel::removeFromHistory(MLItemId itemId)
{
    if (itemId.type != VLC_ML_PARENT_UNKNOWN || itemId.id <= 0)
    {
        emit historyEditFinished(false, tr("This video is unavailable. Refresh the list and try again."));
        return;
    }
    editHistory(itemId.id);
}

void MLRecentVideoModel::editHistory(std::optional<int64_t> itemId)
{
    if (historyBusy())
    {
        emit historyEditFinished(false, tr("Wait for the current history change to finish."));
        return;
    }
    if (!m_mediaLib || !m_mediaLib->historyPlayer() || !m_mediaLib->historyBackend())
    {
        emit historyEditFinished(false, tr("Playback history is unavailable."));
        return;
    }
    const auto scope = m_historyMode == All ? MLVideoHistoryScope::All
        : m_historyMode == Completed ? MLVideoHistoryScope::Completed : MLVideoHistoryScope::Unfinished;
    auto *worker = new HistoryEditThread(m_mediaLib->historyBackend(),
                                        m_mediaLib->historyPlayer(), scope, itemId);
    m_historyWorker = worker;
    connect(worker, &QThread::finished, this, [this, worker] {
        worker->wait();
        const auto result = worker->result;
        m_historyWorker = nullptr;
        delete worker;
        emit historyBusyChanged();
        emit resetRequested();
        QString error;
        if (!result.enumerated)
            error = tr("Could not read playback history. Nothing was removed. Try again.");
        else if (result.failed)
            error = result.removed
                ? tr("Removed %1 items, but %2 could not be removed. Refresh the list and try again.")
                    .arg(qulonglong(result.removed)).arg(qulonglong(result.failed))
                : tr("Could not remove the selected history items. Refresh the list and try again.");
        emit historyEditFinished(result.success(), error);
    });
    emit historyBusyChanged();
    worker->start();
}

void MLRecentVideoModel::setHistoryMode(HistoryMode mode)
{
    if (m_historyMode == mode)
        return;
    m_historyMode = mode;
    emit historyModeChanged();
    emit resetRequested();
}

// Protected MLBaseModel implementation

std::unique_ptr<MLListCacheLoader>
MLRecentVideoModel::createMLLoader() const
/* override */
{
    return std::make_unique<MLListCacheLoader>(m_mediaLib, std::make_shared<MLRecentVideoModel::Loader>(*this));
}

// Private MLVideoModel reimplementation

void MLRecentVideoModel::onVlcMlEvent(const MLEvent & event) /* override */
{
    switch (event.i_type)
    {
        case VLC_ML_EVENT_MEDIA_UPDATED:
            if (m_historyMode == All)
                break;
            // Membership as well as the row data may have changed.
            emit resetRequested();
            return;
        case VLC_ML_EVENT_HISTORY_CHANGED:
            emit resetRequested();
            return;
        default:
            break;
    }

    MLVideoModel::onVlcMlEvent(event);
}

// Loader

size_t MLRecentVideoModel::Loader::count(vlc_medialibrary_t* ml, const vlc_ml_query_params_t* queryParams) const /* override */
{
    if (mode != All)
        return visitMLVideoHistory(ml, queryParams, mode == Completed,
            [](const vlc_ml_media_t &, size_t) { return true; });
    return vlc_ml_count_video_history(ml, queryParams);
}

std::vector<std::unique_ptr<MLItem>>
MLRecentVideoModel::Loader::load(vlc_medialibrary_t* ml, const vlc_ml_query_params_t* queryParams) const /* override */
{
    if (mode != All)
    {
        std::vector<std::unique_ptr<MLItem>> result;
        const size_t offset = queryParams ? queryParams->i_offset : 0;
        const size_t limit = queryParams ? queryParams->i_nbResults : 0;
        visitMLVideoHistory(ml, queryParams, mode == Completed,
            [&](const vlc_ml_media_t &media, size_t index) {
                if (index >= offset)
                    result.emplace_back(std::make_unique<MLVideo>(&media));
                return limit == 0 || result.size() < limit;
            });
        return result;
    }
    ml_unique_ptr<vlc_ml_media_list_t> media_list {
        vlc_ml_list_video_history(ml, queryParams)
    };

    if (media_list == nullptr)
        return {};

    std::vector<std::unique_ptr<MLItem>> res;

    for (const vlc_ml_media_t & media : ml_range_iterate<vlc_ml_media_t>(media_list))
    {
        res.emplace_back( std::make_unique<MLVideo>(&media));
    }

    return res;
}

std::unique_ptr<MLItem>
MLRecentVideoModel::Loader::loadItemById(vlc_medialibrary_t* ml, MLItemId itemId) const
{
    assert(itemId.type == VLC_ML_PARENT_UNKNOWN);
    ml_unique_ptr<vlc_ml_media_t> media(vlc_ml_get_media(ml, itemId.id));
    if (!media || (mode != All && !mlVideoHistoryMatches(*media, mode == Completed)))
        return nullptr;
    return std::make_unique<MLVideo>(media.get());
}
