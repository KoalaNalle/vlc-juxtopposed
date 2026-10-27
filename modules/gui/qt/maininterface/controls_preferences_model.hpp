// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QObject>
#include <QVariantMap>
#include <QHash>
#include <functional>
struct qt_intf_t;

class ControlsPreferencesModel final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap values READ values NOTIFY valuesChanged FINAL)
    Q_PROPERTY(QVariantMap capabilities READ capabilities NOTIFY valuesChanged FINAL)
    Q_PROPERTY(QVariantMap choices READ choices CONSTANT FINAL)
    Q_PROPERTY(QVariantMap applyModes READ applyModes NOTIFY valuesChanged FINAL)
    Q_PROPERTY(QVariantMap supportNotes READ supportNotes CONSTANT FINAL)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged FINAL)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged FINAL)
    Q_PROPERTY(bool restartRequired READ restartRequired NOTIFY valuesChanged FINAL)
    Q_PROPERTY(bool editorActive READ editorActive NOTIFY editorActiveChanged FINAL)
    Q_PROPERTY(bool remoteSetupActive READ remoteSetupActive NOTIFY remoteSetupActiveChanged FINAL)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged FINAL)
    Q_PROPERTY(QVariantMap activeShortcuts READ activeShortcuts CONSTANT FINAL)
    Q_PROPERTY(QString globalShortcutStatus READ globalShortcutStatus NOTIFY valuesChanged FINAL)
    Q_PROPERTY(bool globalShortcutWarning READ globalShortcutWarning NOTIFY valuesChanged FINAL)
public:
    struct Consumers {
        bool mousePolicy = false;
        bool remotePolicy = false;
        std::function<void(ControlsPreferencesModel *)> openShortcutEditor;
        std::function<void(ControlsPreferencesModel *)> openRemoteSetup;
    };
    ControlsPreferencesModel(qt_intf_t *, Consumers, QObject *parent = nullptr);
    QVariantMap values() const { return m_values; }
    QVariantMap capabilities() const { return m_capabilities; }
    QVariantMap choices() const { return m_choices; }
    QVariantMap applyModes() const;
    QVariantMap supportNotes() const;
    QString lastError() const { return m_error; }
    QString status() const { return m_status; }
    bool restartRequired() const { return m_restart; }
    bool editorActive() const { return m_editorActive; }
    bool remoteSetupActive() const { return m_remoteSetupActive; }
    bool busy() const { return m_editorActive || m_remoteSetupActive; }
    Q_INVOKABLE bool setValue(const QString &, const QVariant &);
    Q_INVOKABLE bool resetShortcuts();
    Q_INVOKABLE bool editShortcuts();
    Q_INVOKABLE void reload();
    // Native editor supplies a complete snapshot, never arbitrary config names.
    bool applyShortcuts(const QVariantMap &);
    void shortcutEditorClosed();
    bool applyRemoteSetup(const QString &password);
    void remoteSetupClosed();
    QVariantMap shortcutValues() const;
    QVariantMap activeShortcuts() const;
    QString globalShortcutStatus() const { return m_globalShortcutStatus; }
    bool globalShortcutWarning() const { return m_globalShortcutWarning; }
signals:
    void valuesChanged();
    void lastErrorChanged();
    void statusChanged();
    void editorActiveChanged();
    void remoteSetupActiveChanged();
    void busyChanged();
private:
    bool save(const QVariantMap &);
    bool saveRemoteEnabled(bool enabled, const QString &password = {});
    bool fail(const QString &);
    void setError(const QString &);
    void setStatus(const QString &);
    void refresh();
    QVariantMap restartSnapshot() const;
    QPair<QString, bool> globalShortcutFeedback() const;
    qt_intf_t *m_intf;
    Consumers m_consumers;
    QVariantMap m_values, m_capabilities, m_choices, m_defaults, m_startup;
    QString m_error, m_status;
    QHash<int, QString> m_globalShortcutLabels;
    QString m_globalShortcutStatus;
    bool m_globalShortcutWarning = false;
    bool m_restart = false, m_editorActive = false, m_remoteSetupActive = false;
};
