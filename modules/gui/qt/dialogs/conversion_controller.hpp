// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#ifndef VLC_QT_CONVERSION_CONTROLLER_HPP
#define VLC_QT_CONVERSION_CONTROLLER_HPP

#include "qt.hpp"
#include <QObject>
#include <QThreadPool>
#include <QTimer>
#include <QUrl>
#include <memory>

// One bounded, independent local audio -> FLAC job. The borrowed native parent
// must outlive this object; destruction cancels and joins its single worker.
// The controller never receives or accesses the primary player or playlist.
class ConversionController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUrl sourceUrl READ sourceUrl WRITE setSourceUrl NOTIFY changed FINAL)
    Q_PROPERTY(QUrl destinationUrl READ destinationUrl WRITE setDestinationUrl NOTIFY changed FINAL)
    Q_PROPERTY(QString profileName READ profileName CONSTANT FINAL)
    Q_PROPERTY(State state READ state NOTIFY changed FINAL)
    Q_PROPERTY(bool busy READ busy NOTIFY changed FINAL)
    Q_PROPERTY(bool canStart READ canStart NOTIFY changed FINAL)
    Q_PROPERTY(double progress READ progress NOTIFY changed FINAL)
    Q_PROPERTY(qint64 sourceTimeMs READ sourceTimeMs NOTIFY changed FINAL)
    Q_PROPERTY(qint64 sourceDurationMs READ sourceDurationMs NOTIFY changed FINAL)
    Q_PROPERTY(QString error READ error NOTIFY changed FINAL)
    Q_PROPERTY(QUrl outputUrl READ outputUrl NOTIFY changed FINAL)
public:
    enum State { Idle, Starting, Encoding, Validating, Publishing, Cancelling,
                 Succeeded, Failed, Cancelled };
    Q_ENUM(State)
    explicit ConversionController(vlc_object_t *, QObject *parent = nullptr);
    ~ConversionController();
    QUrl sourceUrl() const { return m_source; }
    QUrl destinationUrl() const { return m_destination; }
    QString profileName() const { return tr("Audio - FLAC"); }
    State state() const { return m_state; }
    bool busy() const;
    bool canStart() const;
    double progress() const { return m_progress; } // -1 until native length exists
    qint64 sourceTimeMs() const { return m_time; }
    qint64 sourceDurationMs() const { return m_duration; }
    QString error() const { return m_error; }
    QUrl outputUrl() const { return m_output; }
    // The actual native input has started and has not yet been deleted. Useful
    // to verify cancellation/shutdown without treating queued work as playback.
    bool nativeInputActive() const;
    Q_INVOKABLE bool start();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void reset();
    void setSourceUrl(const QUrl &);
    void setDestinationUrl(const QUrl &);
signals:
    void changed();
private:
    struct Job;
    struct Result;
    QString validate(QString *source = nullptr, QString *destination = nullptr) const;
    void poll();
    Result convert(const QString &, const QString &, const std::shared_ptr<Job> &);
    vlc_object_t *m_parent;
    QThreadPool m_pool;
    QTimer m_poll;
    QUrl m_source, m_destination, m_output;
    QString m_error;
    State m_state = Idle;
    double m_progress = -1;
    qint64 m_time = 0, m_duration = 0;
    quint64 m_generation = 0;
    std::shared_ptr<Job> m_job;
};
#endif
