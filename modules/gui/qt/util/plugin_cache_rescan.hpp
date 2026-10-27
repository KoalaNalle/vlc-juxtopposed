// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QObject>
#include <QProcess>
#include <QThreadPool>
#include <QTimer>
#include <memory>
struct vlc_object_t;

// Refresh the on-disk inventory for the next launch. The live module bank is
// never unloaded or modified. Layout/limits are C++ inputs, not QML paths.
class PluginCacheRescan final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged FINAL)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged FINAL)
public:
    struct Layout { QString generator, plugins; };
    struct Limits { int stageMs = 30000, processMs = 30000; };
    explicit PluginCacheRescan(vlc_object_t *, QObject *parent = nullptr);
    PluginCacheRescan(vlc_object_t *, Layout, Limits, QObject *parent = nullptr);
    ~PluginCacheRescan() override;
    bool busy() const { return bool(m_job); }
    QString lastError() const { return m_error; }
    bool available() const;
    bool start();
    void cancel();
signals:
    void busyChanged();
    void lastErrorChanged();
    void finished(bool success);
private:
    struct Job;
    void launch(const std::shared_ptr<Job> &);
    void verify(const std::shared_ptr<Job> &);
    void finish(bool, const QString &);
    vlc_object_t *m_parent;
    Layout m_layout;
    Limits m_limits;
    QThreadPool m_pool;
    QProcess m_process;
    QTimer m_deadline;
    std::shared_ptr<Job> m_job;
    QString m_error;
    bool m_timedOut = false;
};
