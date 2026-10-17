// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QObject>
#include <QVariantMap>
class QSettings;
struct qt_intf_t;

class RecordingPreferencesModel final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap values READ values NOTIFY valuesChanged FINAL)
    Q_PROPERTY(QVariantMap capabilities READ capabilities NOTIFY valuesChanged FINAL)
    Q_PROPERTY(QVariantMap choices READ choices CONSTANT FINAL)
    Q_PROPERTY(QVariantMap applyModes READ applyModes CONSTANT FINAL)
    Q_PROPERTY(QVariantMap supportNotes READ supportNotes CONSTANT FINAL)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged FINAL)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged FINAL)
public:
    struct Consumers {
        bool recording = false;
        bool discovery = false;
        bool castQuality = false;
        bool streaming = false;
        bool advanced = false;
    };
    RecordingPreferencesModel(qt_intf_t *, QSettings *, Consumers, QObject *parent = nullptr);
    QVariantMap values() const { return m_values; }
    QVariantMap capabilities() const { return m_capabilities; }
    QVariantMap choices() const { return m_choices; }
    QVariantMap applyModes() const;
    QVariantMap supportNotes() const;
    QString lastError() const { return m_error; }
    bool busy() const { return m_pickerActive; }
    Q_INVOKABLE bool setValue(const QString &, const QVariant &);
    Q_INVOKABLE bool chooseRecordingDirectory();
    Q_INVOKABLE bool openAdvanced();
    Q_INVOKABLE void reload();
signals:
    void valuesChanged();
    void lastErrorChanged();
    void busyChanged();
    void discoveryDisabled();
    void advancedOptionsRequested();
private:
    void refresh();
    bool fail(const QString &);
    void setError(const QString &);
    bool commandLineOverride(const char *) const;
    qt_intf_t *m_intf;
    QSettings *m_settings;
    Consumers m_consumers;
    QVariantMap m_values, m_capabilities, m_choices;
    QString m_error;
    bool m_pickerActive = false;
};
