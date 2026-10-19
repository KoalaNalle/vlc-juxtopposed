// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#ifndef VLC_QT_GENERAL_PREFERENCES_MODEL_HPP
#define VLC_QT_GENERAL_PREFERENCES_MODEL_HPP

#include <QObject>
#include <QPointer>
#include <QVariantMap>
#include <QVariantList>
#include <memory>

struct qt_intf_t;
class ColorSchemeModel;
class QSettings;

// MainCtx owns this service. The interface, shared native colour model and
// main QSettings must outlive it. Values describe saved preferences; options
// consumed at startup are explicitly reported as requiring a restart.
class GeneralPreferencesModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap values READ values NOTIFY valuesChanged FINAL)
    Q_PROPERTY(QVariantList languages READ languages CONSTANT FINAL)
    Q_PROPERTY(bool updatesAvailable READ updatesAvailable CONSTANT FINAL)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged FINAL)
    Q_PROPERTY(bool restartRequired READ restartRequired NOTIFY restartRequiredChanged FINAL)
    Q_PROPERTY(bool setupRequired READ setupRequired NOTIFY setupStateChanged FINAL)
    Q_PROPERTY(bool setupReady READ setupReady NOTIFY setupStateChanged FINAL)
public:
    explicit GeneralPreferencesModel(qt_intf_t *, ColorSchemeModel *, QSettings *,
                                     QObject *parent = nullptr);

    // A narrow persistence seam for native tests: use a real isolated registry
    // key (or INI). Production uses HKCU/Software/VideoLAN/VLC. No test needs to
    // change the human's language key or override the process registry root.
    struct Storage { QSettings *languageSettings = nullptr; };
    GeneralPreferencesModel(qt_intf_t *, ColorSchemeModel *, QSettings *, Storage,
                            QObject *parent = nullptr);
    ~GeneralPreferencesModel() override;

    QVariantMap values() const { return m_values; }
    QVariantList languages() const;
    bool updatesAvailable() const;
    QString lastError() const { return m_lastError; }
    bool restartRequired() const { return m_restartRequired; }
    bool setupRequired() const { return m_setupRequired; }
    bool setupReady() const { return m_setupReady; }
    static QString setupVersionKey() { return QStringLiteral("Juxtopposed/setup-version"); }

    Q_INVOKABLE bool setValue(const QString &key, const QVariant &value);
    Q_INVOKABLE void reload();
    Q_INVOKABLE bool chooseLogFile();
    Q_INVOKABLE bool finishSetup(const QVariant &theme, const QVariant &accentIndex);

signals:
    void valuesChanged();
    void lastErrorChanged();
    void restartRequiredChanged();
    void setupStateChanged();

private:
    bool fail(const QString &);
    void setError(const QString &);
    void refresh();
    void updateRestartRequired();
    bool storeNative(const QVariantMap &changes);
    bool storeSettings(QSettings *, const QString &key, const QVariant &value,
                       bool remove = false);
    bool validateLogFile(const QString &input, QString &normalized, QString &error) const;
    QString configFile() const;
    qt_intf_t *m_intf;
    QPointer<ColorSchemeModel> m_scheme;
    QPointer<QSettings> m_settings;
    QPointer<QSettings> m_languageSettings;
    std::unique_ptr<QSettings> m_ownedLanguageSettings;
    QVariantMap m_values;
    QVariantMap m_startupValues;
    QString m_lastError;
    bool m_restartRequired = false;
    bool m_setupRequired = true;
    bool m_setupReady = false;
};
#endif
