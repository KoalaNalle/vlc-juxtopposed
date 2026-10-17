// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QObject>
#include <QVariantMap>
#include <QByteArray>
#include <functional>
struct qt_intf_t;
class QSettings;
class QThread;

class PrivacyPreferencesModel final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap values READ values NOTIFY valuesChanged FINAL)
    Q_PROPERTY(QVariantMap capabilities READ capabilities NOTIFY valuesChanged FINAL)
    Q_PROPERTY(QVariantMap supportNotes READ supportNotes CONSTANT FINAL)
    Q_PROPERTY(QVariantMap applyModes READ applyModes CONSTANT FINAL)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged FINAL)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged FINAL)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged FINAL)
    Q_PROPERTY(bool restartRequired READ restartRequired NOTIFY valuesChanged FINAL)
public:
    struct LocalHistory {
        std::function<void()> invalidatePendingVisits;
        // Empty result means success. Native clear cannot be rolled back if a
        // later local store fails; the UI must report that partial result.
        std::function<QString()> clearVisits;
    };
    PrivacyPreferencesModel(qt_intf_t *, QSettings *, LocalHistory, QObject *parent=nullptr);
    ~PrivacyPreferencesModel() override;
    QVariantMap values() const { return m_values; }
    QVariantMap capabilities() const { return m_capabilities; }
    QVariantMap supportNotes() const;
    QVariantMap applyModes() const;
    QString lastError() const { return m_error; }
    QString status() const { return m_status; }
    bool busy() const { return m_worker != nullptr; }
    bool restartRequired() const { return m_restart; }
    Q_INVOKABLE bool setValue(const QString &, const QVariant &);
    Q_INVOKABLE bool clearHistory();
    Q_INVOKABLE void reload();
signals:
    void valuesChanged();
    void lastErrorChanged();
    void busyChanged();
    void statusChanged();
    void metadataPreferenceChanged();
    void fileDialogHistoryCleared();
    void networkHistoryCleared();
    void historyPolicyChanged();
private:
    void refresh();
    void finishLocalClear();
    void setError(const QString &);
    void setStatus(const QString &);
    bool fail(const QString &);
    QByteArray startupFingerprint() const;
    QByteArray historyFingerprint() const;
    bool applySavedHistoryPolicy();
    qt_intf_t *m_intf;
    QSettings *m_settings;
    LocalHistory m_local;
    QThread *m_worker=nullptr;
    QVariantMap m_values, m_capabilities;
    QString m_error, m_status;
    QByteArray m_startup, m_historyApplied;
    bool m_restart=false, m_destroying=false;
};
