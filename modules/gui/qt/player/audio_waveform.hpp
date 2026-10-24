// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#ifndef VLC_QT_AUDIO_WAVEFORM_HPP
#define VLC_QT_AUDIO_WAVEFORM_HPP

#include <vlc_common.h>
#include <QObject>
#include <QMap>
#include <QThreadPool>
#include <QUrl>
#include <QVariantList>
#include <atomic>
#include <memory>

// An independent native audio decode. All properties live on the UI thread;
// the worker owns its player, temporary PCM and blocking player deletion.
class AudioWaveform : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged FINAL)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged FINAL)
    Q_PROPERTY(bool loading READ loading NOTIFY stateChanged FINAL)
    Q_PROPERTY(bool available READ available NOTIFY stateChanged FINAL)
    Q_PROPERTY(QString error READ error NOTIFY stateChanged FINAL)
    Q_PROPERTY(QVariantList peaks READ peaks NOTIFY stateChanged FINAL)
    Q_PROPERTY(double durationSeconds READ durationSeconds NOTIFY stateChanged FINAL)
public:
    explicit AudioWaveform(vlc_object_t *, QObject *parent = nullptr);
    ~AudioWaveform();
    QUrl source() const { return m_source; }
    bool enabled() const { return m_enabled; }
    bool loading() const { return m_loading; }
    bool available() const { return !m_result.peaks.isEmpty(); }
    QString error() const { return m_result.error; }
    QVariantList peaks() const { return m_result.peaks; }
    double durationSeconds() const { return m_result.duration; }
public slots:
    void setSource(const QUrl &);
    void setEnabled(bool);
signals:
    void sourceChanged();
    void enabledChanged();
    void stateChanged();
private:
    struct Result { QVariantList peaks; double duration = 0; QString error; };
    using Cancellation = std::shared_ptr<std::atomic_bool>;
    static Result analyze(vlc_object_t *, const QString &, const Cancellation &);
    void restart();
    vlc_object_t *m_parent;
    QUrl m_source;
    bool m_enabled = false;
    bool m_loading = false;
    quint64 m_generation = 0;
    Result m_result;
    QMap<QString, Result> m_cache;
    Cancellation m_cancel;
    QThreadPool m_pool;
};
#endif
