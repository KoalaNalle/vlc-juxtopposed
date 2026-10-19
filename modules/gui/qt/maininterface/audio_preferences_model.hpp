// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef VLC_QT_AUDIO_PREFERENCES_MODEL_HPP
#define VLC_QT_AUDIO_PREFERENCES_MODEL_HPP
#include <QObject>
#include <QVariantMap>
#include <QVariantList>
struct qt_intf_t;

// Startup defaults plus the linked live volume-step policy; Player.equalizer
// remains the separate live playback controller. No unchecked generic editor.
class AudioPreferencesModel final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap values READ values NOTIFY valuesChanged FINAL)
    Q_PROPERTY(QVariantMap capabilities READ capabilities NOTIFY capabilitiesChanged FINAL)
    Q_PROPERTY(QVariantMap options READ options NOTIFY optionsChanged FINAL)
    Q_PROPERTY(QVariantList devices READ devices NOTIFY devicesChanged FINAL)
    Q_PROPERTY(QVariantList equalizerFrequencies READ equalizerFrequencies NOTIFY equalizerFrequenciesChanged FINAL)
    Q_PROPERTY(QVariantMap ranges READ ranges CONSTANT FINAL)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged FINAL)
    Q_PROPERTY(bool restartRequired READ restartRequired NOTIFY restartRequiredChanged FINAL)
public:
    struct Consumers {
        bool windowsDevices = false; // Real Core Audio enumeration + matched MMDevice output, Root acceptance gate.
        bool authoredVolumeStep = false; // Real slider/wheel + native action closure, Root acceptance gate.
        bool windowsPassthrough = false; // Receiver acceptance is independent of endpoint/startup-volume support.
    };
    explicit AudioPreferencesModel(qt_intf_t *, QObject *parent = nullptr);
    AudioPreferencesModel(qt_intf_t *, Consumers, QObject *parent = nullptr);
    QVariantMap values() const { return m_values; }
    QVariantMap capabilities() const { return m_capabilities; }
    QVariantMap options() const { return m_options; }
    QVariantList devices() const { return m_devices; }
    QVariantList equalizerFrequencies() const;
    QVariantMap ranges() const;
    QString lastError() const { return m_error; }
    bool restartRequired() const { return m_restart; }
    Q_INVOKABLE bool setValue(const QString &, const QVariant &);
    Q_INVOKABLE void reload();
signals:
    void valuesChanged();
    void capabilitiesChanged();
    void optionsChanged();
    void devicesChanged();
    void equalizerFrequenciesChanged();
    void lastErrorChanged();
    void restartRequiredChanged();
private:
    void refresh();
    void refreshDevices();
    void setError(const QString &);
    bool fail(const QString &);
    void updateRestart();
    qt_intf_t *m_intf;
    Consumers m_consumers;
    QVariantMap m_values, m_capabilities, m_options, m_startupNative;
    QVariantList m_devices, m_frequencies;
    QString m_error;
    bool m_restart = false, m_deviceEnumerationSucceeded = false;
};
#endif
