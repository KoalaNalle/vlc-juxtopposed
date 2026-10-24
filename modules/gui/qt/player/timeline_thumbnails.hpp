// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#ifndef VLC_QT_TIMELINE_THUMBNAILS_HPP
#define VLC_QT_TIMELINE_THUMBNAILS_HPP
#include "util/shared_input_item.hpp"
#include <QCache>
#include <QImage>
#include <QObject>
#include <QThreadPool>
#include <QTimer>
#include <atomic>
#include <memory>

class QQuickImageProvider;
struct TimelineThumbnailImages;

// One independent, cancellable decode at a time. The current player is never
// sought or paused. Properties/cache belong to the owner thread; images are
// transferred to the engine through a small thread-safe image provider.
class TimelineThumbnails final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged FINAL)
    Q_PROPERTY(bool loading READ loading NOTIFY stateChanged FINAL)
    Q_PROPERTY(QString imageSource READ imageSource NOTIFY stateChanged FINAL)
    Q_PROPERTY(QString error READ error NOTIFY stateChanged FINAL)
    Q_PROPERTY(double requestedSeconds READ requestedSeconds NOTIFY stateChanged FINAL)
public:
    explicit TimelineThumbnails(vlc_object_t *, QObject *parent = nullptr);
    ~TimelineThumbnails() override;
    bool enabled() const { return m_enabled; }
    bool loading() const { return m_loading; }
    QString imageSource() const { return m_imageSource; }
    QString error() const { return m_error; }
    double requestedSeconds() const { return double(m_requested); }
    QQuickImageProvider *createImageProvider() const;
    void setInput(input_item_t *);
    void setEnabled(bool);
    Q_INVOKABLE bool request(double seconds);
    Q_INVOKABLE void cancel();
    Q_INVOKABLE QString registerClient() { return QString::number(++m_clientSerial); }
    Q_INVOKABLE bool requestFor(const QString &client, double seconds);
    Q_INVOKABLE void cancelFor(const QString &client);
signals:
    void enabledChanged();
    void stateChanged();
private:
    using Cancellation = std::shared_ptr<std::atomic_bool>;
    struct Result { QImage image; QString error; };
    static Result decode(vlc_object_t *, const SharedInputItem &, qint64, const Cancellation &);
    void invalidate();
    void startDecode();
    void publish(const QImage &);
    bool requestTime(double);
    vlc_object_t *m_parent;
    SharedInputItem m_input;
    bool m_enabled = false, m_loading = false;
    qint64 m_requested = -1;
    quint64 m_generation = 0, m_imageSerial = 0;
    quint64 m_clientSerial = 0;
    QString m_client;
    QString m_imageSource, m_error;
    QCache<qint64, QImage> m_cache{48};
    std::shared_ptr<TimelineThumbnailImages> m_images;
    Cancellation m_cancel;
    QThreadPool m_pool;
    QTimer m_timer;
};
#endif
