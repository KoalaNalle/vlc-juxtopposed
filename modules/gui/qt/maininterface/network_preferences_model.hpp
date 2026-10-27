// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QObject>
#include <QVariantMap>
struct qt_intf_t;

class NetworkPreferencesModel final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap values READ values NOTIFY valuesChanged FINAL)
    Q_PROPERTY(QVariantMap capabilities READ capabilities NOTIFY valuesChanged FINAL)
    Q_PROPERTY(QVariantMap choices READ choices CONSTANT FINAL)
    Q_PROPERTY(QVariantMap applyModes READ applyModes CONSTANT FINAL)
    Q_PROPERTY(QVariantMap supportNotes READ supportNotes CONSTANT FINAL)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged FINAL)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged FINAL)
public:
    struct Consumers {
        bool caching = false;
        bool proxy = false;
        bool tls = false;
        bool advanced = false;
    };
    NetworkPreferencesModel(qt_intf_t *, Consumers, QObject *parent = nullptr);
    QVariantMap values() const { return m_values; }
    QVariantMap capabilities() const { return m_capabilities; }
    QVariantMap choices() const { return m_choices; }
    QVariantMap applyModes() const;
    QVariantMap supportNotes() const;
    QString lastError() const { return m_error; }
    QString status() const { return m_status; }
    Q_INVOKABLE bool setValue(const QString &, const QVariant &);
    Q_INVOKABLE bool openAdvanced();
    Q_INVOKABLE void reload();
signals:
    void valuesChanged();
    void lastErrorChanged();
    void statusChanged();
    void advancedOptionsRequested();
private:
    void refresh();
    bool save(const QVariantMap &);
    bool fail(const QString &);
    void setError(const QString &);
    void setStatus(const QString &);
    qt_intf_t *m_intf;
    Consumers m_consumers;
    QVariantMap m_values, m_capabilities, m_choices;
    QString m_error, m_status;
    bool m_customEditing = false;
};
