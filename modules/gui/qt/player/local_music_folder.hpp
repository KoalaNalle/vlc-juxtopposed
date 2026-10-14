// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#ifndef VLC_QT_LOCAL_MUSIC_FOLDER_HPP
#define VLC_QT_LOCAL_MUSIC_FOLDER_HPP
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "util/shared_input_item.hpp"
#include <QAbstractListModel>
#include <QThreadPool>
#include <QUrl>
#include <atomic>
#include <memory>
struct vlc_playlist;
typedef struct vlc_playlist vlc_playlist_t;

// Local source-folder browsing is independent of library indexing and queue order.
// All model mutations occur on the GUI thread; discovery/metadata use one worker.
class LocalMusicFolderModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QUrl sourceUrl READ sourceUrl WRITE setSourceUrl NOTIFY stateChanged FINAL)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY stateChanged FINAL)
    Q_PROPERTY(bool available READ available NOTIFY stateChanged FINAL)
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged FINAL)
    Q_PROPERTY(QString error READ error NOTIFY stateChanged FINAL)
    Q_PROPERTY(QString name READ name NOTIFY stateChanged FINAL)
    Q_PROPERTY(QUrl folderUrl READ folderUrl NOTIFY stateChanged FINAL)
    Q_PROPERTY(int count READ rowCount NOTIFY stateChanged FINAL)
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY stateChanged FINAL)
    Q_PROPERTY(QString searchPattern READ searchPattern WRITE setSearchPattern NOTIFY stateChanged FINAL)
    Q_PROPERTY(bool sortAscending READ sortAscending WRITE setSortAscending NOTIFY stateChanged FINAL)
public:
    enum Role { TitleRole = Qt::UserRole + 1, ArtistRole, ArtworkRole, UrlRole };
    explicit LocalMusicFolderModel(vlc_object_t *, vlc_playlist_t *, QObject *parent = nullptr);
    ~LocalMusicFolderModel();
    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &, int role) const override;
    QUrl sourceUrl() const { return m_source; }
    bool enabled() const { return m_enabled; }
    bool available() const { return !m_folder.isEmpty(); }
    bool busy() const { return m_busy; }
    QString error() const { return m_error; }
    QString name() const;
    QUrl folderUrl() const { return QUrl::fromLocalFile(m_folder); }
    int currentIndex() const;
    QString searchPattern() const { return m_search; }
    bool sortAscending() const { return m_ascending; }
    Q_INVOKABLE bool addAndPlay(int row);
    Q_INVOKABLE void refresh();
public slots:
    void setSourceUrl(const QUrl &);
    void setEnabled(bool);
    void setSearchPattern(const QString &);
    void setSortAscending(bool);
signals:
    void stateChanged();
private:
    struct Entry { QUrl url; QString title; QString artist; QUrl artwork; SharedInputItem item; };
    using Cancellation = std::shared_ptr<std::atomic_bool>;
    void restart();
    void rebuildView();
    void filterView();
    void replaceEntries(QVector<Entry>);
    void scan(const QString &, quint64, const Cancellation &);
    vlc_object_t *m_parent;
    vlc_playlist_t *m_playlist;
    QUrl m_source;
    QString m_folder, m_search, m_error;
    bool m_enabled = false, m_busy = false, m_ascending = true;
    quint64 m_generation = 0;
    QVector<Entry> m_entries;
    QVector<int> m_view;
    Cancellation m_cancel;
    QThreadPool m_pool;
};
#endif
