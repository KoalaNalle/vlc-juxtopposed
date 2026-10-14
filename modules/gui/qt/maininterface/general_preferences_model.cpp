// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "general_preferences_model.hpp"
#include "qt.hpp"
#include "util/color_scheme_model.hpp"
#include "util/interface_language_map.hpp"
#include <vlc_configuration.h>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHash>
#include <QRegularExpression>
#include <QSettings>
#include <QTemporaryFile>
#include <cmath>

namespace {
const QStringList accents = {QStringLiteral("#D37918"), QStringLiteral("#E15032"),
    QStringLiteral("#C49E23"), QStringLiteral("#19A234"), QStringLiteral("#18AAD3"),
    QStringLiteral("#185DD3"), QStringLiteral("#8E42FF"), QStringLiteral("#F052B3"),
    QStringLiteral("#AFAFAF")};
const QStringList restartKeys = {QStringLiteral("language"), QStringLiteral("autoUpdates"),
    QStringLiteral("developerMode"), QStringLiteral("loggingEnabled"),
    QStringLiteral("logFile"), QStringLiteral("logDetails")};

QString nativeString(const char *key)
{
    if (config_GetType(key) != VLC_VAR_STRING) return {};
    char *raw = config_GetPsz(key);
    const auto result = QString::fromUtf8(raw ? raw : "");
    free(raw);
    return result;
}
bool nativeBool(const char *key, bool fallback = false)
{
    return config_GetType(key) == VLC_VAR_BOOL ? config_GetInt(key) != 0 : fallback;
}
int nativeInteger(const char *key, int fallback)
{
    return config_GetType(key) == VLC_VAR_INTEGER ? config_GetInt(key) : fallback;
}
// A QML number can arrive as Double. Accept exact finite integers, never a
// string/bool, fractional coercion, NaN, infinity or an out-of-range value.
bool integer(const QVariant &value, int minimum, int maximum, int &result)
{
    switch (value.typeId()) {
    case QMetaType::Int: case QMetaType::UInt:
    case QMetaType::LongLong: case QMetaType::ULongLong:
    case QMetaType::Double: case QMetaType::Float: break;
    default: return false;
    }
    const double number = value.toDouble();
    if (!std::isfinite(number) || std::floor(number) != number
        || number < minimum || number > maximum) return false;
    result = int(number);
    return true;
}
bool validText(const QString &text)
{
    // QString -> UTF-8 must not replace a lone UTF-16 surrogate. NUL/newline
    // must never become an embedded native string/config-file terminator.
    for (const auto character : text) if (character.unicode() < 32) return false;
    return QString::fromUtf8(text.toUtf8()) == text;
}

std::unique_ptr<QSettings> settingsSession(const QSettings *source)
{
    if (!source) return {};
    std::unique_ptr<QSettings> session;
    if (!source->organizationName().isEmpty() && source->fallbacksEnabled()) {
        // MainCtx uses organization/scope INI settings, which can have real
        // fallback stores. A filename-only clone would lose that policy.
        session = std::make_unique<QSettings>(source->format(), source->scope(),
            source->organizationName(), source->applicationName());
    } else {
        // The native language registry key, explicit native-test INIs, or
        // a primary-only policy which must not read unrelated fallbacks.
        session = std::make_unique<QSettings>(source->fileName(), source->format());
    }
    // A global QSettings::setPath change must not redirect a transaction away
    // from the borrowed store created earlier in this process.
    if (session->fileName() != source->fileName()) return {};
    session->setFallbacksEnabled(source->fallbacksEnabled());
    session->setAtomicSyncRequired(source->isAtomicSyncRequired());
    if (!source->group().isEmpty()) session->beginGroup(source->group());
    return session;
}
}

GeneralPreferencesModel::GeneralPreferencesModel(qt_intf_t *intf,
    ColorSchemeModel *scheme, QSettings *settings, QObject *parent)
    : GeneralPreferencesModel(intf, scheme, settings, Storage{}, parent)
{}

GeneralPreferencesModel::GeneralPreferencesModel(qt_intf_t *intf,
    ColorSchemeModel *scheme, QSettings *settings, Storage storage, QObject *parent)
    : QObject(parent), m_intf(intf), m_scheme(scheme), m_settings(settings)
{
    if (storage.languageSettings)
        m_languageSettings = storage.languageSettings;
    else {
#ifdef _WIN32
        m_ownedLanguageSettings = std::make_unique<QSettings>(
            QStringLiteral("HKEY_CURRENT_USER\\Software\\VideoLAN\\VLC"), QSettings::NativeFormat);
        m_languageSettings = m_ownedLanguageSettings.get();
#endif
    }
    if (m_scheme) connect(m_scheme, &ColorSchemeModel::currentChanged, this, [this] { refresh(); });
    reload();
    m_startupValues = m_values;
    updateRestartRequired();
}
GeneralPreferencesModel::~GeneralPreferencesModel() = default;

QVariantList GeneralPreferencesModel::languages() const
{
    QVariantList result;
    for (const auto &entry : interface_language_map)
        result.append(QVariantMap{{QStringLiteral("code"), QString::fromLatin1(entry.iso)},
            {QStringLiteral("name"), entry.iso == QByteArrayLiteral("auto")
                ? tr("Automatic") : QString::fromUtf8(entry.name)}});
    return result;
}
bool GeneralPreferencesModel::updatesAvailable() const
{
#ifdef UPDATE_CHECK
    return config_GetType("qt-updates-notif") == VLC_VAR_BOOL;
#else
    return false;
#endif
}
bool GeneralPreferencesModel::fail(const QString &message)
{ setError(message); return false; }
void GeneralPreferencesModel::setError(const QString &message)
{
    if (m_lastError == message) return;
    m_lastError = message;
    emit lastErrorChanged();
}

void GeneralPreferencesModel::refresh()
{
    const auto accent = nativeString("qt-accent-color");
    int accentIndex = accents.indexOf(accent.toUpper());
    if (accent.isEmpty() || accent == QStringLiteral("default")) accentIndex = 0;
    const int verbosity = nativeInteger("log-verbose", -1);
    int details = -1; // Preserve a legacy/default/INFO choice, with no false selection.
    const int filter = nativeInteger("log-message-filter", -1);
    if (filter == 0 && verbosity == 0) details = 0;
    else if (filter == 1 && verbosity == 1) details = 1;
    else if (filter == 2 && verbosity >= 2) details = 2;
    QVariantMap next {
        {QStringLiteral("language"), m_languageSettings
            ? m_languageSettings->value(QStringLiteral("Lang"), QStringLiteral("auto")).toString()
            : QStringLiteral("auto")},
        {QStringLiteral("theme"), m_scheme ? m_scheme->currentIndex() : 0},
        {QStringLiteral("accentIndex"), accentIndex},
        {QStringLiteral("showDialogs"), nativeBool("qt-error-dialogs", true)},
        {QStringLiteral("autoUpdates"), updatesAvailable() && nativeBool("qt-updates-notif")},
        {QStringLiteral("developerMode"), nativeBool("qt-verbose")},
        {QStringLiteral("loggingEnabled"), nativeBool("file-logging")},
        {QStringLiteral("logFile"), nativeString("logfile")},
        {QStringLiteral("logDetails"), details}
    };
    if (m_values != next) { m_values = std::move(next); emit valuesChanged(); }
    updateRestartRequired();
}
void GeneralPreferencesModel::updateRestartRequired()
{
    bool required = false;
    if (!m_startupValues.isEmpty())
        for (const auto &key : restartKeys)
            required |= m_values.value(key) != m_startupValues.value(key);
    if (required == m_restartRequired) return;
    m_restartRequired = required;
    emit restartRequiredChanged();
}
void GeneralPreferencesModel::reload()
{
    setError({});
    bool currentReadError = false;
    const auto syncCurrent = [&currentReadError](QSettings *source) {
        if (!source) return;
        // Preserve the caller's current policy and flush its pending keys.
        // Its first-error status is historical and cannot be reset by sync.
        source->sync();
        auto session = settingsSession(source);
        if (!session) { currentReadError = true; return; }
        session->sync();
        currentReadError |= session->status() != QSettings::NoError;
    };
    syncCurrent(m_settings);
    syncCurrent(m_languageSettings);
    const auto before = m_values;
    refresh();
    // Legacy preferences may change a custom native accent while its swatch
    // remains -1. Native palette consumers must still reread that colour.
    if (before == m_values) emit valuesChanged();
    if (!m_intf || !m_scheme || !m_settings)
        setError(tr("The native preferences service is unavailable."));
    else if (currentReadError)
        setError(tr("Some saved interface preferences could not be read."));
}

QString GeneralPreferencesModel::configFile() const
{
    if (!m_intf) return {};
    char *path = var_InheritString(m_intf, "config");
    if (path) { const auto result = QString::fromUtf8(path); free(path); return result; }
    char *directory = config_GetUserDir(VLC_CONFIG_DIR);
    if (!directory) return {};
    const auto result = QDir(QString::fromUtf8(directory)).filePath(QStringLiteral("vlcrc"));
    free(directory);
    return result;
}

bool GeneralPreferencesModel::storeNative(const QVariantMap &changes)
{
    if (!m_intf) return fail(tr("The native preferences service is unavailable."));
    QVariantMap before;
    for (auto it = changes.cbegin(); it != changes.cend(); ++it) {
        const auto key = it.key().toUtf8();
        const int type = config_GetType(key.constData());
        if ((it.value().typeId() == QMetaType::QString && type != VLC_VAR_STRING)
            || (it.value().typeId() != QMetaType::QString
                && type != VLC_VAR_BOOL && type != VLC_VAR_INTEGER))
            return fail(tr("This build does not provide the requested native preference."));
        before.insert(it.key(), type == VLC_VAR_STRING
            ? QVariant(nativeString(key.constData())) : QVariant(qlonglong(config_GetInt(key.constData()))));
    }
    auto assign = [](const QVariantMap &values) {
        for (auto it = values.cbegin(); it != values.cend(); ++it) {
            const auto key = it.key().toUtf8();
            if (it.value().typeId() == QMetaType::QString) {
                const auto text = it.value().toString().toUtf8();
                config_PutPsz(key.constData(), text.constData());
            } else config_PutInt(key.constData(), it.value().toLongLong());
        }
    };
    assign(changes);
    bool matches = true;
    for (auto it = changes.cbegin(); it != changes.cend(); ++it) {
        const auto key = it.key().toUtf8();
        matches &= it.value().typeId() == QMetaType::QString
            ? nativeString(key.constData()) == it.value().toString()
            : config_GetInt(key.constData()) == it.value().toLongLong();
    }
    if (!matches) {
        assign(before);
        return fail(tr("The native preference could not be updated. The previous value was restored."));
    }
    const auto fileName = configFile();
    QFile file(fileName);
    bool saved = config_SaveConfigFile(m_intf) == 0 && file.open(QIODevice::ReadOnly);
    QByteArray bytes;
    if (saved) { bytes = file.readAll(); saved = file.error() == QFile::NoError; file.close(); }
    // Verify the real saved representation too: older cores incorrectly
    // returned success on a failed final rename. Defaults are commented by
    // VLC's serializer, so remove just that one leading comment marker.
    if (saved) {
        for (auto it = changes.cbegin(); it != changes.cend(); ++it) {
            const QByteArray expected = it.key().toUtf8() + '=' + (it.value().typeId() == QMetaType::QString
                ? it.value().toString().toUtf8() : QByteArray::number(it.value().toLongLong()));
            bool found = false;
            for (auto line : bytes.split('\n')) {
                if (line.endsWith('\r')) line.chop(1);
                if (line.startsWith('#')) line.remove(0, 1);
                if (line == expected) { found = true; break; }
            }
            saved &= found;
        }
    }
    if (!saved) {
        assign(before);
        return fail(tr("The VLC configuration could not be saved. The previous in-memory values were restored."));
    }
    return true;
}

bool GeneralPreferencesModel::storeSettings(QSettings *settings, const QString &key,
                                           const QVariant &value, bool remove)
{
    if (!settings)
        return fail(tr("The interface preferences storage is not writable."));
    // Flush unrelated caller changes using its own policy before opening a
    // fresh session. Never interpret its permanently retained first error as
    // a current failure after permissions/file-sharing have been repaired.
    settings->sync();
    auto transaction = settingsSession(settings);
    auto primary = settingsSession(settings);
    if (!transaction || !primary)
        return fail(tr("The interface preferences storage location has changed."));
    transaction->sync();
    primary->setFallbacksEnabled(false);
    primary->sync();
    const bool existed = primary->contains(key);
    const auto before = primary->value(key);
    if (!transaction->isWritable() || transaction->status() != QSettings::NoError
        || primary->status() != QSettings::NoError)
        return fail(tr("The interface preferences storage is not writable."));
    if (remove) transaction->remove(key); else transaction->setValue(key, value);
    transaction->sync();
    std::unique_ptr<QSettings> readback;
    bool matches = false;
    if (transaction->status() == QSettings::NoError) {
        readback = settingsSession(settings);
        if (readback) {
            readback->setFallbacksEnabled(false);
            readback->sync();
            matches = remove ? !readback->contains(key) : readback->value(key) == value;
        }
    }
    if (transaction->status() == QSettings::NoError && readback
        && readback->status() == QSettings::NoError && matches)
        return true;
    // INI sessions share their cache with the borrowed object. Restore only
    // this primary key; never turn a fallback-only value into a user override.
    // Native registry sessions write the same exact injected/production key.
    if (existed) transaction->setValue(key, before); else transaction->remove(key);
    transaction->sync();
    return fail(tr("The interface preference could not be saved. The requested change was not accepted."));
}

bool GeneralPreferencesModel::validateLogFile(const QString &input, QString &normalized,
                                             QString &error) const
{
    if (!validText(input) || input.isEmpty() || input.toUtf8().size() > 32760) {
        error = tr("Choose a valid local log filename."); return false;
    }
    normalized = QDir::cleanPath(QDir::fromNativeSeparators(input));
    if (!QFileInfo(normalized).isAbsolute() || input.contains(QStringLiteral("://"))) {
        error = tr("The log filename must be an absolute local path."); return false;
    }
#ifdef _WIN32
    const auto tail = normalized.mid(normalized.size() > 1 && normalized.at(1) == ':' ? 2 : 0);
    if (tail.contains(QRegularExpression(QStringLiteral("[<>:\"|?*]")))) {
        error = tr("The log filename contains characters Windows cannot use."); return false;
    }
    for (const auto &part : tail.split('/', Qt::SkipEmptyParts)) {
        const auto stem = part.section('.', 0, 0).toUpper();
        if (part.endsWith('.') || part.endsWith(' ')
            || QRegularExpression(QStringLiteral("^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])$")).match(stem).hasMatch()) {
            error = tr("The log filename contains a reserved Windows name."); return false;
        }
    }
#endif
    const QFileInfo info(normalized);
    if (info.exists()) {
        QFile existing(normalized);
        if (!info.isFile() || !info.isWritable()
            || !existing.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::ExistingOnly)) {
            error = tr("The existing log file cannot be written."); return false;
        }
        existing.close(); // No bytes are appended or truncated.
    }
    const QFileInfo parent(info.absolutePath());
    if (!parent.isDir() || !parent.isWritable()) {
        error = tr("The log folder does not exist or is not writable."); return false;
    }
    QTemporaryFile probe(QDir(parent.absoluteFilePath()).filePath(QStringLiteral(".vlc-log-check-XXXXXX")));
    if (!probe.open()) { error = tr("The log folder cannot create files."); return false; }
    return true;
}

bool GeneralPreferencesModel::setValue(const QString &key, const QVariant &value)
{
    setError({});
    const auto before = m_values;
    if (!m_intf || !m_scheme || !m_settings)
        return fail(tr("The native preferences service is unavailable."));
    bool success = false;
    if (key == QStringLiteral("theme")) {
        int index;
        if (!integer(value, 0, 2, index)) return fail(tr("Choose System, Light or Dark."));
        if (!storeSettings(m_settings, QStringLiteral("MainWindow/color-scheme-index"), index)) return false;
        m_scheme->setCurrentIndex(index); // Existing MainCtx applies Qt's native colour scheme.
        success = true;
    } else if (key == QStringLiteral("accentIndex")) {
        int index;
        if (!integer(value, 0, 8, index)) return fail(tr("Choose one of the nine accent colours."));
        success = storeNative({{QStringLiteral("qt-accent-color"), accents[index]}});
    } else if (key == QStringLiteral("language")) {
        if (value.typeId() != QMetaType::QString || !validText(value.toString()))
            return fail(tr("Choose a supported interface language."));
        bool supported = false;
        for (const auto &entry : interface_language_map) supported |= value.toString() == QString::fromLatin1(entry.iso);
        if (!supported) return fail(tr("Choose a supported interface language."));
        success = storeSettings(m_languageSettings, QStringLiteral("Lang"), value,
                                value.toString() == QStringLiteral("auto"));
    } else if (key == QStringLiteral("logFile")) {
        if (value.typeId() != QMetaType::QString) return fail(tr("Choose a valid local log filename."));
        const auto input = value.toString();
        QString normalized, error;
        if (input.isEmpty() && !m_values.value(QStringLiteral("loggingEnabled")).toBool()) normalized.clear();
        else if (!validateLogFile(input, normalized, error)) return fail(error);
        success = storeNative({{QStringLiteral("logfile"), normalized}});
    } else if (key == QStringLiteral("logDetails")) {
        int details;
        if (!integer(value, 0, 2, details)) return fail(tr("Choose a supported log detail level."));
        success = storeNative({{QStringLiteral("log-verbose"), details},
            {QStringLiteral("log-message-filter"), details}});
    } else if (key == QStringLiteral("showDialogs") || key == QStringLiteral("autoUpdates")
        || key == QStringLiteral("developerMode") || key == QStringLiteral("loggingEnabled")) {
        if (value.typeId() != QMetaType::Bool) return fail(tr("This preference requires an on or off value."));
        if (key == QStringLiteral("autoUpdates") && !updatesAvailable())
            return value.toBool() ? fail(tr("Automatic updates are unavailable in this build.")) : true;
        const QHash<QString, QString> names {{QStringLiteral("showDialogs"), QStringLiteral("qt-error-dialogs")},
            {QStringLiteral("autoUpdates"), QStringLiteral("qt-updates-notif")},
            {QStringLiteral("developerMode"), QStringLiteral("qt-verbose")},
            {QStringLiteral("loggingEnabled"), QStringLiteral("file-logging")}};
        QVariantMap changes {{names.value(key), value}};
        if (key == QStringLiteral("loggingEnabled") && value.toBool()) {
            auto input = m_values.value(QStringLiteral("logFile")).toString();
            if (input.isEmpty()) input = QFileInfo(configFile()).dir().filePath(QStringLiteral("vlc-log.txt"));
            QString normalized, error;
            if (!validateLogFile(input, normalized, error)) return fail(error);
            changes.insert(QStringLiteral("logfile"), normalized);
        }
        success = storeNative(changes);
    } else return fail(tr("Unknown General preference."));
    if (success) {
        refresh();
        // Selecting authored Orange replaces native "default" without
        // changing swatch index0. Notify the real palette consumer too.
        if (before == m_values) emit valuesChanged();
    }
    return success;
}

bool GeneralPreferencesModel::chooseLogFile()
{
    setError({});
    const auto path = QFileDialog::getSaveFileName(nullptr, tr("Choose VLC log file"),
        m_values.value(QStringLiteral("logFile")).toString(), tr("Text log files (*.log *.txt);;All files (*)"));
    if (path.isEmpty()) return false; // A cancelled genuine native picker is not a saved change.
    return setValue(QStringLiteral("logFile"), path);
}
