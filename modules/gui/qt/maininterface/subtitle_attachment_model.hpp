// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#ifndef VLC_QT_SUBTITLE_ATTACHMENT_MODEL_HPP
#define VLC_QT_SUBTITLE_ATTACHMENT_MODEL_HPP
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "qt.hpp"
#include "util/shared_input_item.hpp"
#include <vlc_player.h>
#include <QMap>
#include <QPointer>
#include <QSettings>
#include <QSet>
#include <QUrl>
#include <QVariantList>
#include <atomic>
#include <functional>

// Attachment scope is independent of appearance/timing presets. MainCtx owns
// this service and supplies the existing saved-playlist origin of the current
// queue item. No queue hash or transient row number becomes a persisted ID.
class SubtitleAttachmentModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int scope READ scope WRITE setScope NOTIFY changed FINAL)
    Q_PROPERTY(bool active READ active NOTIFY changed FINAL)
    Q_PROPERTY(bool editable READ editable NOTIFY changed FINAL)
    Q_PROPERTY(bool canSave READ canSave NOTIFY changed FINAL)
    Q_PROPERTY(bool folderAvailable READ folderAvailable NOTIFY changed FINAL)
    Q_PROPERTY(bool playlistAvailable READ playlistAvailable NOTIFY changed FINAL)
    Q_PROPERTY(QString sourceLabel READ sourceLabel NOTIFY changed FINAL)
    Q_PROPERTY(QUrl sourceFolder READ sourceFolder NOTIFY changed FINAL)
    Q_PROPERTY(QVariantList mediaRows READ mediaRows NOTIFY changed FINAL)
    Q_PROPERTY(QVariantList subtitleRows READ subtitleRows NOTIFY changed FINAL)
    Q_PROPERTY(QString error READ error NOTIFY changed FINAL)
    Q_PROPERTY(QString statusText READ statusText NOTIFY changed FINAL)
public:
    enum Scope { ThisFile, ThisFolder, ThisPlaylist };
    Q_ENUM(Scope)
    struct Context {
        SharedInputItem item;
        qint64 playlistId = 0;
        QVariantList playlistItems;
    };
    using ContextProvider = std::function<Context(bool includeItems)>;
    using DefaultSelectionPolicy = std::function<bool(input_item_t *)>;
    SubtitleAttachmentModel(vlc_player_t *, QSettings *, ContextProvider = {},
                            DefaultSelectionPolicy = {}, QObject *parent = nullptr);
    ~SubtitleAttachmentModel();
    int scope() const { return m_scope; }
    bool active() const { return m_active; }
    bool editable() const { return m_active && !m_stale && m_storageValid; }
    bool canSave() const;
    bool folderAvailable() const { return !m_mediaUrl.isEmpty() && m_mediaUrl.isLocalFile(); }
    bool playlistAvailable() const { return m_draftContext.playlistId > 0; }
    QString sourceLabel() const { return m_sourceFolder.toLocalFile(); }
    QUrl sourceFolder() const { return m_sourceFolder; }
    QVariantList mediaRows() const;
    QVariantList subtitleRows() const;
    QString error() const { return m_error; }
    QString statusText() const { return m_status; }
    Q_INVOKABLE bool begin();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void setScope(int);
    Q_INVOKABLE void setSourceFolder(const QUrl &);
    Q_INVOKABLE void addFiles(const QVariantList &);
    Q_INVOKABLE void toggle(int mediaRow, int subtitleRow);
    Q_INVOKABLE void clearRow(int mediaRow);
    Q_INVOKABLE void matchNames();
    Q_INVOKABLE bool save();
    void reload();
    void refresh();
    // Shared by the restore path and native tests; returns only the winning
    // association, in default-selection order. Missing sources are not replaced
    // silently with a lower-priority association.
    QStringList effectiveFiles(const QUrl &, qint64 playlistId) const;
signals:
    void changed();
    void loadFailed(const QString &message);
private:
    struct Record { int scope; QString owner; QMap<QString, QStringList> files; };
    struct Row { QUrl url; QString title, key; QStringList subtitles; };
    static QString identity(const QUrl &);
    static QUrl localUrl(const QUrl &);
    static bool subtitleFile(const QUrl &, bool requireExists);
    static void mediaChanged(vlc_player_t *, input_item_t *, void *);
    static void stateChanged(vlc_player_t *, vlc_player_state, void *);
    static void tracksChanged(vlc_player_t *, vlc_player_list_action, const vlc_player_track *, void *);
    void queueRefresh();
    Context context(bool includeItems) const;
    void rebuildRows();
    bool scan(const QUrl &, bool subtitles, QList<Row> *, QStringList *);
    QString owner(int scope, const QUrl &, qint64 playlistId) const;
    const Record *find(int scope, const QString &) const;
    bool persist(const QList<Record> &);
    void fail(const QString &);
    void failLoad(const QString &);
    void clearLoadFailure();
    void confirmLoads(); // Native player is locked; no QObject notification.
    void selectDefault(); // Native player is locked; no QObject notification.
    vlc_player_t *m_player;
    QPointer<QSettings> m_settings;
    ContextProvider m_context;
    DefaultSelectionPolicy m_defaultSelection;
    vlc_player_listener_id *m_listener = nullptr;
    std::atomic_uint64_t m_generation{0};
    std::atomic_bool m_refreshQueued{false};
    uint64_t m_draftGeneration = 0, m_appliedGeneration = UINT64_MAX, m_submittedGeneration = UINT64_MAX;
    uint64_t m_explicitDefaultGeneration = UINT64_MAX;
    uint64_t m_loadGeneration = UINT64_MAX, m_loadSerial = 0;
    qint64 m_appliedPlaylist = 0;
    Context m_draftContext;
    QUrl m_mediaUrl, m_sourceFolder;
    QList<Record> m_records;
    QList<Row> m_rows;
    QStringList m_subtitles;
    QSet<QString> m_submitted;
    QSet<QByteArray> m_pendingLoads;
    QByteArray m_pendingDefault;
    int m_scope = ThisFile;
    QString m_error, m_status, m_loadError;
    bool m_active = false, m_stale = false, m_storageValid = true, m_scanValid = true;
};
#endif
