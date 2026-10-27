// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#pragma once
#include "new_playlist_data.hpp"
#include "medialibrary/mlqmltypes.hpp"
#include <QObject>
#include <QThreadPool>
#include <QVariantList>

class NewPlaylistModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY changed FINAL)
    Q_PROPERTY(QUrl thumbnail READ thumbnail WRITE setThumbnail NOTIFY changed FINAL)
    Q_PROPERTY(QVariantList files READ files NOTIFY changed FINAL)
    Q_PROPERTY(bool available READ available CONSTANT FINAL)
    Q_PROPERTY(bool busy READ busy NOTIFY changed FINAL)
    Q_PROPERTY(bool canSave READ canSave NOTIFY changed FINAL)
    Q_PROPERTY(bool partialSave READ partialSave NOTIFY changed FINAL)
    Q_PROPERTY(MLItemId savedPlaylist READ savedPlaylist NOTIFY changed FINAL)
    Q_PROPERTY(QString error READ error NOTIFY changed FINAL)
public:
    NewPlaylistModel(vlc_medialibrary_t *, QString artworkDirectory, QObject *parent = nullptr);
    ~NewPlaylistModel() override;
    QString name() const { return m_request.name; }
    QUrl thumbnail() const { return m_request.thumbnail; }
    QVariantList files() const;
    bool available() const { return m_library != nullptr; }
    bool busy() const { return m_busy; }
    bool canSave() const;
    bool partialSave() const { return m_committed && !m_success; }
    MLItemId savedPlaylist() const { return MLItemId(m_savedId, VLC_ML_PARENT_PLAYLIST); }
    QString error() const { return m_error.isEmpty() ? m_validation : m_error; }
    Q_INVOKABLE bool appendFiles(const QVariantList &urls);
    Q_INVOKABLE void removeFile(int index);
    Q_INVOKABLE void moveFile(int index, int destination);
    Q_INVOKABLE void reset();
    Q_INVOKABLE void save();
public slots:
    void setName(const QString &);
    void setThumbnail(const QUrl &);
signals:
    void changed();
    void finished(bool success);
private:
    void validate();
    bool editable() const { return !m_busy && !m_committed; }
    vlc_medialibrary_t *m_library;
    QString m_artworkDirectory;
    QThreadPool m_pool;
    NewPlaylistRequest m_request;
    QString m_error, m_validation;
    bool m_busy = false, m_committed = false, m_success = false;
    qint64 m_savedId = 0;
};
