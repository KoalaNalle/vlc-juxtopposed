// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef VLC_QT_SUBTITLES_PREFERENCES_MODEL_HPP
#define VLC_QT_SUBTITLES_PREFERENCES_MODEL_HPP
#include <QObject>
#include <QVariantMap>
#include <QByteArray>
#include <QPointer>
struct qt_intf_t;
class SubtitlePresetModel;
class SubtitlesPreferencesModel final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap values READ values NOTIFY valuesChanged FINAL)
    Q_PROPERTY(QVariantMap capabilities READ capabilities NOTIFY capabilitiesChanged FINAL)
    Q_PROPERTY(QVariantMap choices READ choices NOTIFY choicesChanged FINAL)
    Q_PROPERTY(QVariantMap applyModes READ applyModes CONSTANT FINAL)
    Q_PROPERTY(QVariantMap supportNotes READ supportNotes NOTIFY valuesChanged FINAL)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged FINAL)
    Q_PROPERTY(bool restartRequired READ restartRequired NOTIFY restartRequiredChanged FINAL)
public:
    struct Consumers {
        // Enable ONLY after Root's original/fixed native renderer pair passes.
        bool primaryAlignment = false;
        bool feedbackDuration = false;
        bool shortcutFeedback = false;
    };
    explicit SubtitlesPreferencesModel(qt_intf_t *, QObject *parent = nullptr);
    SubtitlesPreferencesModel(qt_intf_t *, SubtitlePresetModel *, Consumers, QObject *parent = nullptr);
    ~SubtitlesPreferencesModel() override;
    QVariantMap values() const { return m_values; }
    QVariantMap capabilities() const { return m_capabilities; }
    QVariantMap choices() const { return m_choices; }
    QVariantMap applyModes() const;
    QVariantMap supportNotes() const;
    QString lastError() const { return m_error; }
    bool restartRequired() const { return m_restartRequired; }
    void profileDefaultsChanged(const QStringList &nativeKeys);
    Q_INVOKABLE bool setValue(const QString &, const QVariant &);
    Q_INVOKABLE void reload();
signals:
    void valuesChanged();
    void capabilitiesChanged();
    void choicesChanged();
    void lastErrorChanged();
    void restartRequiredChanged();
private:
    void refresh();
    void setError(const QString &);
    bool fail(const QString &);
    bool commit(const QVariantMap &, const QString &);
    QByteArray configurationFingerprint() const;
    qt_intf_t *m_intf;
    QPointer<SubtitlePresetModel> m_presets;
    Consumers m_consumers;
    QVariantMap m_values, m_capabilities, m_choices;
    QString m_error;
    QByteArray m_startupFingerprint;
    bool m_restartRequired = false;
};
#endif
