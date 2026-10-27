/*****************************************************************************
 * Copyright (C) 2021 VLC authors and VideoLAN
 *
 * Authors: Benjamin Arnaud <bunjee@omega.gg>
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

#ifndef MLVIDEOFOLDERSMODEL_HPP
#define MLVIDEOFOLDERSMODEL_HPP

// MediaLibrary includes
#include "mlvideomodel.hpp"

// Forward declarations
class MLFolder;

class MLVideoFoldersModel : public MLBaseModel
{
    Q_OBJECT

public:
    enum Roles
    {
        FOLDER_ID = Qt::UserRole + 1,
        FOLDER_TITLE,
        FOLDER_TITLE_FIRST_SYMBOL,
        FOLDER_THUMBNAIL,
        FOLDER_DURATION,
        FOLDER_COUNT,
        FOLDER_NB_VIDEOS
    };

public:
    explicit MLVideoFoldersModel(QObject * parent = nullptr);

public: // MLBaseModel reimplementation
    QHash<int, QByteArray> roleNames() const override;

protected:
    virtual vlc_ml_media_type_t mediaType() const { return VLC_ML_MEDIA_TYPE_VIDEO; }
    QVariant itemRoleData(const MLItem *item, int role = Qt::DisplayRole) const override;

    vlc_ml_sorting_criteria_t nameToCriteria(QByteArray name) const override;

    std::unique_ptr<MLListCacheLoader> createMLLoader() const override;

protected: // MLBaseModel reimplementation
    void onVlcMlEvent(const MLEvent & event) override;

private:
    struct Loader : public MLListCacheLoader::MLOp
    {
        explicit Loader(const MLVideoFoldersModel &model)
            : MLOp(model), m_type(model.mediaType()) {}
        size_t count(vlc_medialibrary_t * ml, const vlc_ml_query_params_t* queryParams) const override;

        std::vector<std::unique_ptr<MLItem>> load(vlc_medialibrary_t * ml,
                                                  const vlc_ml_query_params_t* queryParams) const override;

        std::unique_ptr<MLItem> loadItemById(vlc_medialibrary_t* ml, MLItemId itemId) const override;
    private:
        vlc_ml_media_type_t m_type;
    };
};

// Share folder identity, paging and events with the established video model.
class MLAudioFoldersModel : public MLVideoFoldersModel
{
    Q_OBJECT
public:
    explicit MLAudioFoldersModel(QObject *parent = nullptr) : MLVideoFoldersModel(parent) {}
protected:
    vlc_ml_media_type_t mediaType() const override { return VLC_ML_MEDIA_TYPE_AUDIO; }
};

#endif // MLVIDEOFOLDERSMODEL_HPP
