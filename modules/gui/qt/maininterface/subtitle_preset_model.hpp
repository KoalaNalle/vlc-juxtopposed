// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#ifndef VLC_QT_SUBTITLE_PRESET_MODEL_HPP
#define VLC_QT_SUBTITLE_PRESET_MODEL_HPP
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "player/subtitle_appearance.hpp"
#include "util/shared_input_item.hpp"
#include <QHash>
#include <QJsonObject>
#include <QPointer>
#include <QSettings>
#include <QVariantList>
#include <atomic>

// Owned by MainCtx: the Qt settings and native player outlive this service.
// Saved track policies contain values, never pointers to a terminated ES.
class SubtitlePresetModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList entries READ entries NOTIFY stateChanged FINAL)
    Q_PROPERTY(QString selectedId READ selectedId NOTIFY stateChanged FINAL)
    Q_PROPERTY(QString selectedName READ selectedName NOTIFY stateChanged FINAL)
    Q_PROPERTY(bool edited READ edited NOTIFY stateChanged FINAL)
    Q_PROPERTY(bool canSave READ canSave NOTIFY stateChanged FINAL)
    Q_PROPERTY(bool canApply READ canApply NOTIFY stateChanged FINAL)
    Q_PROPERTY(bool restoring READ restoring NOTIFY stateChanged FINAL)
    Q_PROPERTY(QString statusText READ statusText NOTIFY stateChanged FINAL)
    Q_PROPERTY(QString error READ error NOTIFY stateChanged FINAL)
public:
    explicit SubtitlePresetModel(vlc_player_t *, SubtitleAppearance *, QSettings *,
                                 vlc_object_t *configuration, QObject *parent = nullptr);
    ~SubtitlePresetModel();
    QVariantList entries() const;
    QString selectedId() const { return m_selectedId; }
    QString selectedName() const;
    bool edited() const { return m_edited; }
    bool canSave() const;
    bool canApply() const;
    bool hasTrackSelectionPolicy(input_item_t *) const;
    bool restoring() const { return !m_pendingId.isEmpty(); }
    QString statusText() const;
    QString error() const { return m_error; }
    Q_INVOKABLE bool save(const QString &name);
    Q_INVOKABLE bool apply(const QString &id);
    Q_INVOKABLE void clearError();
    Q_INVOKABLE void reload();
    // Ordinary preference changes update Default without discarding live edits.
    Q_INVOKABLE void refreshDefaults();
    // Also useful to native harnesses that attach a newly created video output.
    Q_INVOKABLE void refresh();
signals:
    void stateChanged();
private:
    struct TrackPolicy {
        QString id, language;
        int index = -1, order = VLC_VOUT_ORDER_NONE;
        bool ownDelay = false;
        qint64 delay = 0;
        bool operator==(const TrackPolicy &) const;
    };
    struct Preset {
        QString id, name;
        SubtitleAppearance::Snapshot style;
        qint64 delay = 0; // Native ticks, microseconds; no tenths-of-second conversion.
        float fps = 0.f;
        QList<TrackPolicy> tracks;
    };
    static QJsonObject encode(const Preset &);
    static bool decode(const QJsonObject &, Preset *);
    static bool same(const Preset &, const Preset &, bool compareTracks);
    static void playerStateChanged(vlc_player_t *, vlc_player_state, void *);
    static void mediaChanged(vlc_player_t *, input_item_t *, void *);
    static void tracksChanged(vlc_player_t *, vlc_player_list_action, const vlc_player_track *, void *);
    static void selectionChanged(vlc_player_t *, vlc_es_id_t *, vlc_es_id_t *, void *);
    static void delayChanged(vlc_player_t *, es_format_category_e, vlc_tick_t, void *);
    static void trackDelayChanged(vlc_player_t *, vlc_es_id_t *, vlc_tick_t, void *);
    static void fpsChanged(vlc_player_t *, float, void *);
    static void outputChanged(vlc_player_t *, vlc_player_vout_action, vout_thread_t *,
                              vlc_vout_order, vlc_es_id_t *, void *);
    void queueRefresh();
    void refreshMedia();
    bool capture(Preset *) const;
    bool tryApply();
    bool confirmSubmittedApply();
    void updateMatch();
    bool persist(const QList<Preset> &, const QHash<QString, QString> &);
    const Preset *find(const QString &) const;
    void fail(const QString &);
    vlc_player_t *m_player;
    QPointer<SubtitleAppearance> m_style;
    QPointer<QSettings> m_settings;
    vlc_object_t *m_configuration;
    vlc_player_listener_id *m_listener = nullptr;
    std::atomic_uint64_t m_generation{0};
    uint64_t m_contextGeneration = 0, m_requestSerial = 0;
    SharedInputItem m_media;
    QString m_mediaKey;
    QList<Preset> m_presets;
    QHash<QString, QString> m_associations;
    Preset m_default, m_applied, m_submitted;
    QString m_selectedId = QStringLiteral("default"), m_pendingId, m_error;
    bool m_storageValid = true, m_edited = false, m_applying = false;
    bool m_haveApplied = false, m_explicit = false, m_selectionRequested = false;
    // A request submits native writes once; later callbacks only confirm them.
    bool m_writesSubmitted = false;
    bool m_resetStyleNext = false;
};
#endif
