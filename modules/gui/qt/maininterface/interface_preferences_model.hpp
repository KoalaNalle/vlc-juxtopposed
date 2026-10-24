// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef VLC_QT_INTERFACE_PREFERENCES_MODEL_HPP
#define VLC_QT_INTERFACE_PREFERENCES_MODEL_HPP
#include <QObject>
#include <QPointer>
#include <QVariantMap>
#include <QVariantList>
struct qt_intf_t;
class QSettings;

class InterfacePreferencesModel final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap values READ values NOTIFY valuesChanged FINAL)
    Q_PROPERTY(QVariantMap capabilities READ capabilities NOTIFY capabilitiesChanged FINAL)
    Q_PROPERTY(QVariantList screens READ screens NOTIFY screensChanged FINAL)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged FINAL)
    Q_PROPERTY(bool restartRequired READ restartRequired NOTIFY restartRequiredChanged FINAL)
    Q_PROPERTY(bool pickerActive READ pickerActive NOTIFY pickerActiveChanged FINAL)
public:
    // False until Root has linked AND verified the paired consumer changes.
    // Existing flags with genuine startup consumers do not require these gates.
    struct Consumers {
        bool rememberGeometry = false;
        bool titleFileName = false; // Includes visible custom title, not only OS caption.
        bool fullscreenScreen = false; // Shared INTERFACE|VIDEO hold boundary; native gate required.
        bool controlAutoHide = false;
        bool controlHideDelay = false;
        bool popupDuration = false;
        bool fullscreenControls = false; // Embedded main-window player only.
    };
    explicit InterfacePreferencesModel(qt_intf_t *, QSettings *, QObject *parent = nullptr);
    InterfacePreferencesModel(qt_intf_t *, QSettings *, Consumers, QObject *parent = nullptr);
    ~InterfacePreferencesModel() override;
    QVariantMap values() const { return m_values; }
    QVariantMap capabilities() const { return m_capabilities; }
    QVariantList screens() const { return m_screens; }
    QString lastError() const { return m_lastError; }
    bool restartRequired() const { return m_restartRequired; }
    bool pickerActive() const { return m_pickerActive; }
    Q_INVOKABLE bool setValue(const QString &, const QVariant &);
    Q_INVOKABLE void reload();
    Q_INVOKABLE bool chooseSkinFile();
signals:
    void valuesChanged();
    void capabilitiesChanged();
    void screensChanged();
    void lastErrorChanged();
    void restartRequiredChanged();
    void pickerActiveChanged();
private:
    bool fail(const QString &);
    void setError(const QString &);
    void refresh();
    void updateRestartRequired();
    bool validateSkinFile(const QString &, QString &, QString &) const;
    qt_intf_t *m_intf;
    QPointer<QSettings> m_settings;
    Consumers m_consumers;
    QVariantMap m_values, m_startupValues, m_capabilities;
    QVariantList m_screens;
    QString m_lastError;
    bool m_restartRequired = false;
    bool m_pickerActive = false;
};
#endif
