// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef VLC_QT_VIDEO_PREFERENCES_MODEL_HPP
#define VLC_QT_VIDEO_PREFERENCES_MODEL_HPP
#include <QObject>
#include <QVariantMap>
#include <QByteArray>
struct qt_intf_t;

class VideoPreferencesModel final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap values READ values NOTIFY valuesChanged FINAL)
    Q_PROPERTY(QVariantMap capabilities READ capabilities NOTIFY capabilitiesChanged FINAL)
    Q_PROPERTY(QVariantMap choices READ choices NOTIFY choicesChanged FINAL)
    Q_PROPERTY(QVariantMap applyModes READ applyModes CONSTANT FINAL)
    Q_PROPERTY(QVariantMap supportNotes READ supportNotes NOTIFY valuesChanged FINAL)
    Q_PROPERTY(int nativeScreensaverMode READ nativeScreensaverMode NOTIFY valuesChanged FINAL)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged FINAL)
    Q_PROPERTY(bool restartRequired READ restartRequired NOTIFY restartRequiredChanged FINAL)
    Q_PROPERTY(bool pickerActive READ pickerActive NOTIFY pickerActiveChanged FINAL)
public:
    struct Consumers {
        // Existing Win32 desktop-parented window, not merely the core flag.
        // Root enables after actual desktop/provider/native-GUI verification.
        bool desktopWallpaper = false;
    };
    explicit VideoPreferencesModel(qt_intf_t *, QObject *parent = nullptr);
    VideoPreferencesModel(qt_intf_t *, Consumers, QObject *parent = nullptr);
    ~VideoPreferencesModel() override;
    QVariantMap values() const { return m_values; }
    QVariantMap capabilities() const { return m_capabilities; }
    QVariantMap choices() const { return m_choices; }
    QVariantMap applyModes() const;
    QVariantMap supportNotes() const;
    int nativeScreensaverMode() const;
    QString lastError() const { return m_error; }
    bool restartRequired() const { return m_restartRequired; }
    bool pickerActive() const { return m_pickerActive; }
    Q_INVOKABLE bool setValue(const QString &, const QVariant &);
    Q_INVOKABLE void reload();
    Q_INVOKABLE bool resetPicture();
    Q_INVOKABLE bool chooseSnapshotDirectory();
signals:
    void valuesChanged();
    void capabilitiesChanged();
    void choicesChanged();
    void lastErrorChanged();
    void restartRequiredChanged();
    void pickerActiveChanged();
private:
    void refresh();
    void setError(const QString &);
    bool fail(const QString &);
    bool commit(const QVariantMap &);
    QByteArray configurationFingerprint() const;
    qt_intf_t *m_intf;
    Consumers m_consumers;
    QVariantMap m_values, m_capabilities, m_choices;
    QString m_error;
    QByteArray m_startupFingerprint;
    bool m_restartRequired = false, m_pickerActive = false;
};
#endif
