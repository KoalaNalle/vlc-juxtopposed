// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QObject>
#include <QTimer>
#include "player_profile_preferences.hpp"
#include "player_profiles_store.hpp"

class PlayerProfileLayout;
class PlayerProfilesModel final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList profiles READ profiles NOTIFY stateChanged FINAL)
    Q_PROPERTY(QString selectedId READ selectedId NOTIFY stateChanged FINAL)
    Q_PROPERTY(QString selectedName READ selectedName NOTIFY stateChanged FINAL)
    Q_PROPERTY(QString lastError READ lastError NOTIFY stateChanged FINAL)
    Q_PROPERTY(bool available READ available NOTIFY stateChanged FINAL)
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged FINAL)
    Q_PROPERTY(bool unsaved READ unsaved NOTIFY stateChanged FINAL)
    Q_PROPERTY(bool needsRecovery READ needsRecovery NOTIFY stateChanged FINAL)
    Q_PROPERTY(PlayerProfileLayout* layout READ layout CONSTANT FINAL)
    Q_MOC_INCLUDE("maininterface/player_profile_layout.hpp")
public:
    explicit PlayerProfilesModel(qt_intf_t *, QSettings *, QObject *parent = nullptr);
    QVariantList profiles() const;
    QString selectedId() const { return m_store.selectedId(); }
    QString selectedName() const;
    QString lastError() const { return m_error; }
    bool available() const { return m_valid && m_store.readable(); }
    bool busy() const { return m_busy || m_externalBusy; }
    bool unsaved() const { return m_unsaved; }
    bool needsRecovery() const { return m_store.pendingSwitch().has_value(); }
    PlayerProfileLayout *layout() const { return m_layout; }
    // Called by typed media owners, not by a generic QML field editor.
    void preferencesChanged();
    void setExternalBusy(bool);
    Q_INVOKABLE bool selectProfile(const QString &id);
    Q_INVOKABLE bool createProfile(const QString &name);
    Q_INVOKABLE bool retrySave();
    Q_INVOKABLE bool retrySwitch();
    Q_INVOKABLE bool restorePreviousProfile();
    Q_INVOKABLE bool beginLayoutEdit();
    Q_INVOKABLE bool saveLayoutDraft();
signals:
    void stateChanged();
    // Emitted only after a checked native save, while checkpointing is paused.
    // Owners retain their original live/next-input/next-snapshot/restart modes.
    void defaultsApplied(const QStringList &nativeKeys);
private:
    bool capture(QVariantMap &, QString &error) const;
    bool editable(bool allowDraft = false);
    bool fail(const QString &);
    bool applyPending(bool forward);
    bool checkpoint();
    PlayerProfilesStore m_store;
    PlayerProfilePreferences m_preferences;
    PlayerProfileLayout *m_layout;
    QTimer m_checkpoint;
    QString m_error;
    bool m_valid = false, m_busy = false, m_externalBusy = false, m_unsaved = false;
};
