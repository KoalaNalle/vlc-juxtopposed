// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QObject>
#include <QVariantMap>
#include <memory>
struct qt_intf_t;
class PluginCacheRescan;

class PerformancePreferencesModel final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap values READ values NOTIFY valuesChanged FINAL)
    Q_PROPERTY(QVariantMap capabilities READ capabilities NOTIFY valuesChanged FINAL)
    Q_PROPERTY(QVariantMap choices READ choices NOTIFY valuesChanged FINAL)
    Q_PROPERTY(QVariantMap applyModes READ applyModes NOTIFY valuesChanged FINAL)
    Q_PROPERTY(QVariantMap supportNotes READ supportNotes NOTIFY valuesChanged FINAL)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged FINAL)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged FINAL)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged FINAL)
public:
    struct Consumers {
        bool decoding = false, postprocessing = false, clocks = false;
        bool encoder = false, priority = false, rescan = false;
    };
    PerformancePreferencesModel(qt_intf_t *, Consumers, QObject *parent = nullptr);
    ~PerformancePreferencesModel() override;
    QVariantMap values() const { return m_values; }
    QVariantMap capabilities() const { return m_capabilities; }
    QVariantMap choices() const { return m_choices; }
    QVariantMap applyModes() const;
    QVariantMap supportNotes() const;
    QString lastError() const { return m_error; }
    QString status() const { return m_status; }
    bool busy() const;
    Q_INVOKABLE bool setValue(const QString &, const QVariant &);
    Q_INVOKABLE bool rescanPlugins();
    Q_INVOKABLE void reload();
signals:
    void valuesChanged();
    void lastErrorChanged();
    void statusChanged();
    void busyChanged();
private:
    void refresh();
    bool save(const QVariantMap &);
    bool priority(const QString &, const QVariant &);
    bool fail(const QString &);
    void setError(const QString &);
    void setStatus(const QString &);
    bool commandLineOverride(const QString &) const;
    qt_intf_t *m_intf;
    Consumers m_consumers;
    std::unique_ptr<PluginCacheRescan> m_scan;
    QVariantMap m_values, m_capabilities, m_choices;
    QString m_error, m_status;
};
