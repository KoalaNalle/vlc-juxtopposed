// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef VLC_QT_PLAYBACK_PREFERENCES_MODEL_HPP
#define VLC_QT_PLAYBACK_PREFERENCES_MODEL_HPP
#include <QObject>
#include <QVariantMap>
struct qt_intf_t;

class PlaybackPreferencesModel final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap values READ values NOTIFY valuesChanged FINAL)
    Q_PROPERTY(QVariantMap capabilities READ capabilities NOTIFY capabilitiesChanged FINAL)
    Q_PROPERTY(QVariantMap choices READ choices CONSTANT FINAL)
    Q_PROPERTY(QVariantMap applyModes READ applyModes CONSTANT FINAL)
    Q_PROPERTY(QVariantMap supportNotes READ supportNotes NOTIFY valuesChanged FINAL)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged FINAL)
    Q_PROPERTY(bool restartRequired READ restartRequired NOTIFY restartRequiredChanged FINAL)
    Q_PROPERTY(bool pickerActive READ pickerActive NOTIFY pickerActiveChanged FINAL)
public:
    struct Consumers {
        // Root enables only after the real PlayerController seek patch is linked
        // and native timeline/time/relative-seek behavior has been verified.
        bool preciseSeeking = false;
    };
    explicit PlaybackPreferencesModel(qt_intf_t *, QObject *parent = nullptr);
    PlaybackPreferencesModel(qt_intf_t *, Consumers, QObject *parent = nullptr);
    ~PlaybackPreferencesModel() override;
    QVariantMap values() const { return m_values; }
    QVariantMap capabilities() const { return m_capabilities; }
    QVariantMap choices() const { return m_choices; }
    QVariantMap applyModes() const;
    QVariantMap supportNotes() const;
    QString lastError() const { return m_error; }
    bool restartRequired() const { return m_restartRequired; }
    bool pickerActive() const { return m_pickerActive; }
    void profileDefaultsChanged(const QStringList &nativeKeys);
    void transportDefaultsChanged();
    Q_INVOKABLE bool setValue(const QString &, const QVariant &);
    Q_INVOKABLE void reload();
    Q_INVOKABLE bool chooseTimeShiftFolder();
signals:
    void valuesChanged();
    void capabilitiesChanged();
    void lastErrorChanged();
    void restartRequiredChanged();
    void pickerActiveChanged();
private:
    void refresh();
    void setError(const QString &);
    bool fail(const QString &);
    bool saveRate(float);
    void applyLive(const QString &);
    qt_intf_t *m_intf;
    Consumers m_consumers;
    QVariantMap m_values, m_capabilities, m_choices;
    float m_startupRate = 1.f;
    bool m_startupCork = true;
    QString m_error;
    bool m_restartRequired = false;
    bool m_pickerActive = false;
};
#endif
