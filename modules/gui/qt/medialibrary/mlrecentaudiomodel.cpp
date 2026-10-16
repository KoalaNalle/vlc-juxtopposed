/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License, version 2 or later.
 *****************************************************************************/
#include "mlrecentaudiomodel.hpp"
#include "mlhelper.hpp"

MLRecentAudioModel::MLRecentAudioModel(QObject *parent) : MLAudioModel(parent)
{
    setSortOrder(Qt::DescendingOrder);
}

std::unique_ptr<MLListCacheLoader> MLRecentAudioModel::createMLLoader() const
{
    return std::make_unique<MLListCacheLoader>(m_mediaLib, std::make_shared<Loader>(*this));
}

void MLRecentAudioModel::onVlcMlEvent(const MLEvent &event)
{
    if (event.i_type == VLC_ML_EVENT_HISTORY_CHANGED ||
        event.i_type == VLC_ML_EVENT_ALBUM_UPDATED ||
        event.i_type == VLC_ML_EVENT_ARTIST_UPDATED)
    {
        emit resetRequested();
        return;
    }
    MLMediaModel::onVlcMlEvent(event);
}

size_t MLRecentAudioModel::Loader::count(vlc_medialibrary_t *ml, const vlc_ml_query_params_t *params) const
{
    return vlc_ml_count_audio_history(ml, params);
}

std::vector<std::unique_ptr<MLItem>> MLRecentAudioModel::Loader::load(vlc_medialibrary_t *ml, const vlc_ml_query_params_t *params) const
{
    ml_unique_ptr<vlc_ml_media_list_t> list(vlc_ml_list_audio_history(ml, params));
    std::vector<std::unique_ptr<MLItem>> result;
    if (!list)
        return result;
    for (const auto &media : ml_range_iterate<vlc_ml_media_t>(list))
        result.emplace_back(std::make_unique<MLAudio>(ml, &media));
    return result;
}

std::unique_ptr<MLItem> MLRecentAudioModel::Loader::loadItemById(vlc_medialibrary_t *ml, MLItemId id) const
{
    if (id.type != VLC_ML_PARENT_UNKNOWN)
        return nullptr;
    ml_unique_ptr<vlc_ml_media_t> media(vlc_ml_get_media(ml, id.id));
    if (!media || media->i_type != VLC_ML_MEDIA_TYPE_AUDIO)
        return nullptr;
    return std::make_unique<MLAudio>(ml, media.get());
}
