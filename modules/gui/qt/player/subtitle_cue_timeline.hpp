// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#pragma once
#include "subtitle_cue_data.hpp"
#include <QObject>
#include <QThreadPool>
#include <QTimer>
#include <QVariantMap>

class SubtitleCueTimeline : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY stateChanged FINAL)
    Q_PROPERTY(bool available READ available NOTIFY stateChanged FINAL)
    Q_PROPERTY(bool loading READ loading NOTIFY stateChanged FINAL)
    Q_PROPERTY(QString status READ status NOTIFY stateChanged FINAL)
    Q_PROPERTY(QUrl source READ source NOTIFY stateChanged FINAL)
    Q_PROPERTY(QString trackId READ trackId NOTIFY stateChanged FINAL)
    Q_PROPERTY(qint64 delayTicks READ delayTicks NOTIFY stateChanged FINAL)
    Q_PROPERTY(qint64 positionTicks READ positionTicks NOTIFY positionChanged FINAL)
    Q_PROPERTY(qint64 lengthTicks READ lengthTicks NOTIFY positionChanged FINAL)
    Q_PROPERTY(int cueCount READ cueCount NOTIFY cuesChanged FINAL)
    Q_PROPERTY(int revision READ revision NOTIFY cuesChanged FINAL)
public:
    SubtitleCueTimeline(vlc_player_t *, QObject *parent = nullptr);
    ~SubtitleCueTimeline() override;
    bool enabled() const { return m_enabled; }
    bool available() const { return m_available && m_validatedEpoch == m_nativeEpoch.load(); }
    bool loading() const { return m_reading && m_enabled && m_snapshot.available; }
    QString status() const { return m_status; }
    QUrl source() const { return m_snapshot.available ? m_snapshot.uri : QUrl{}; }
    QString trackId() const { return QString::fromUtf8(m_snapshot.trackId); }
    qint64 delayTicks() const { return m_snapshot.delay; }
    qint64 positionTicks() const { return m_snapshot.position; }
    qint64 lengthTicks() const { return m_snapshot.length; }
    int cueCount() const { return m_cues.size(); }
    int revision() const { return m_revision; }
    Q_INVOKABLE QVariantMap view(qint64 startTicks, qint64 endTicks) const;
    // A displayed cue carries its revision. Recheck the same source, native
    // epoch, effective delay, active input and bounds under one player lock.
    Q_INVOKABLE bool seekCue(int cueIndex, int revision);
    // Shift the whole selected source, retaining category inheritance or the
    // selected ES override. A drag submits once with its displayed revision.
    Q_INVOKABLE bool setDelayTicks(qint64 delayTicks, int revision);
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void reload();
public slots:
    void setEnabled(bool);
signals:
    void stateChanged();
    void cuesChanged();
    void positionChanged();
private:
    void invalidateNative();
    static QByteArray key(const SubtitleCueSource &);
    void clearCues();
    vlc_player_t *m_player;
    vlc_player_listener_id *m_listener = nullptr;
    std::atomic<quint64> m_nativeEpoch{0};
    quint64 m_validatedEpoch = 0;
    std::atomic_bool m_refreshQueued{false};
    QThreadPool m_pool;
    QTimer m_poll;
    SubtitleCueCancellation m_cancel;
    SubtitleCueSource m_snapshot;
    QVector<SubtitleSourceCue> m_cues;
    QByteArray m_loadedKey, m_readKey;
    QString m_status;
    bool m_enabled = false, m_available = false, m_reading = false;
    int m_revision = 0;
};
