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

#ifndef ML_RECENT_VIDEO_MODEL_H
#define ML_RECENT_VIDEO_MODEL_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mlvideomodel.hpp"
#include <optional>

class QThread;

class MLRecentVideoModel : public MLVideoModel
{
    Q_OBJECT
    Q_PROPERTY(HistoryMode historyMode READ historyMode WRITE setHistoryMode NOTIFY historyModeChanged FINAL)
    Q_PROPERTY(bool historyBusy READ historyBusy NOTIFY historyBusyChanged FINAL)

public:
    enum HistoryMode { All, Unfinished, Completed };
    Q_ENUM(HistoryMode)
    explicit MLRecentVideoModel(QObject * parent = nullptr);

    ~MLRecentVideoModel() override;
    HistoryMode historyMode() const { return m_historyMode; }
    void setHistoryMode(HistoryMode mode);
    bool historyBusy() const { return m_historyWorker != nullptr; }
    Q_INVOKABLE void clearHistory();
    Q_INVOKABLE void removeFromHistory(MLItemId itemId);

signals:
    void historyModeChanged();
    void historyBusyChanged();
    void historyEditFinished(bool success, QString error);

protected: // MLBaseModel implementation
    std::unique_ptr<MLListCacheLoader> createMLLoader() const override;

protected: // MLVideoModel reimplementation
    void onVlcMlEvent(const MLEvent & event) override;

private:
    void editHistory(std::optional<int64_t> itemId);
    struct Loader : public MLListCacheLoader::MLOp
    {
        explicit Loader(const MLRecentVideoModel &model)
            : MLOp(model), mode(model.historyMode()) {}
        const HistoryMode mode;

        size_t count(vlc_medialibrary_t* ml, const vlc_ml_query_params_t*) const override;

        std::vector<std::unique_ptr<MLItem>> load(vlc_medialibrary_t* ml, const vlc_ml_query_params_t*) const override;

        std::unique_ptr<MLItem> loadItemById(vlc_medialibrary_t* ml, MLItemId itemId) const override;
    };
    HistoryMode m_historyMode = All;
    QThread *m_historyWorker = nullptr;
};

#endif // ML_RECENT_VIDEO_MODEL_H
