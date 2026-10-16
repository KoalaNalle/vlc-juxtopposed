/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License, version 2 or later.
 *****************************************************************************/
#pragma once

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "mlaudiomodel.hpp"

class MLRecentAudioModel : public MLAudioModel
{
    Q_OBJECT
public:
    explicit MLRecentAudioModel(QObject *parent = nullptr);

protected:
    std::unique_ptr<MLListCacheLoader> createMLLoader() const override;
    void onVlcMlEvent(const MLEvent &event) override;

private:
    struct Loader : public MLListCacheLoader::MLOp
    {
        using MLListCacheLoader::MLOp::MLOp;
        size_t count(vlc_medialibrary_t *, const vlc_ml_query_params_t *) const override;
        std::vector<std::unique_ptr<MLItem>> load(vlc_medialibrary_t *, const vlc_ml_query_params_t *) const override;
        std::unique_ptr<MLItem> loadItemById(vlc_medialibrary_t *, MLItemId) const override;
    };
};
