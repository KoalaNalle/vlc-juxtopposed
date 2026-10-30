// SPDX-License-Identifier: GPL-2.0-or-later
// Generated from immutable General V5; retain its save/recovery behavior.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "checked_preferences_store.hpp"
#include "qt.hpp"
#include <vlc_configuration.h>
#include <vlc_config_transaction.h>
#include <QVector>
#include <QCoreApplication>
#include <QSettings>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QLockFile>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QCryptographicHash>
#include <memory>
#include <cmath>
#include <QLocale>
namespace {
QString tr(const char *text) { return QCoreApplication::translate("GeneralPreferencesModel", text); }
bool reject(QString &error, const QString &message) { error = message; return false; }
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

using SettingsValue = CheckedPreferencesStore::SettingsValue;

bool iniStorage(const QSettings *settings)
{
#ifdef _WIN32
    return settings->format() == QSettings::IniFormat;
#else
    // Qt's NativeFormat uses INI storage on Unix, not the Windows registry.
    return settings->format() == QSettings::IniFormat
        || settings->format() == QSettings::NativeFormat;
#endif
}

bool readIniBytes(const QString &path, QByteArray &bytes, QString &error)
{
    bytes.clear();
    QFile file(path);
    if (!file.exists()) return true;
    if (!file.open(QIODevice::ReadOnly) || file.size() > 64 * 1024 * 1024)
        return reject(error, tr("The interface preferences file could not be read."));
    bytes = file.readAll();
    return file.error() == QFile::NoError
        || reject(error, tr("The interface preferences file could not be read."));
}

bool writeStagedIni(const QString &path, const QByteArray &bytes, QString &error)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.flush())
        return reject(error, tr("The interface preferences transaction could not be prepared."));
    return true;
}

// QSettings shares INI caches across instances in one process. Read physical
// snapshots under unique filenames so a failed or same-size prior write cannot
// supply the expected document for a later catalog transaction.
bool readStagedValue(const QString &path, const QSettings *source, const QString &key,
                     SettingsValue &result, QString &error)
{
    QSettings reader(path, QSettings::IniFormat);
    reader.setFallbacksEnabled(false);
    if (!source->group().isEmpty()) reader.beginGroup(source->group());
    result.present = reader.contains(key);
    result.value = result.present ? reader.value(key) : QVariant{};
    return reader.status() == QSettings::NoError
        || reject(error, tr("The interface preferences file could not be parsed."));
}

QStringList iniFallbackFiles(const QSettings *source)
{
    QStringList paths;
    if (!source->fallbacksEnabled() || source->organizationName().isEmpty()) return paths;
    const auto add = [&](QSettings::Scope scope, const QString &application) {
        QSettings fallback(source->format(), scope, source->organizationName(), application);
        const auto path = fallback.fileName();
        if (path != source->fileName() && !paths.contains(path)) paths.append(path);
    };
    // Qt's effective order is user/app, user/org, system/app, system/org.
    if (source->scope() == QSettings::UserScope) {
        add(QSettings::UserScope, {});
        add(QSettings::SystemScope, source->applicationName());
    }
    add(QSettings::SystemScope, {});
    return paths;
}

bool readEffectiveIniValue(const QSettings *source, const QString &key,
                           const QString &primarySnapshot, const QString &temporary,
                           SettingsValue &result, QString &error)
{
    if (!readStagedValue(primarySnapshot, source, key, result, error)) return false;
    int index = 0;
    for (const auto &path : iniFallbackFiles(source)) {
        if (result.present) break;
        QByteArray bytes;
        const auto snapshot = QDir(temporary).filePath(QStringLiteral("fallback-%1.ini").arg(index++));
        if (!readIniBytes(path, bytes, error) || !writeStagedIni(snapshot, bytes, error)
            || !readStagedValue(snapshot, source, key, result, error)) return false;
    }
    return true;
}

bool compareValue(const SettingsValue &current, const SettingsValue *expected,
                  bool *conflict, QString &error)
{
    if (!expected || current == *expected) return true;
    if (conflict) *conflict = true;
    return reject(error, tr("These preferences changed in another window. Reload them and retry."));
}

bool saveIniValue(QSettings *source, const QString &key, const QVariant &value,
                  bool remove, const SettingsValue *expected, bool *conflict,
                  QString &error)
{
    const QFileInfo destination(source->fileName());
    if (!destination.isAbsolute() || !QDir().mkpath(destination.absolutePath()))
        return reject(error, tr("The interface preferences folder is unavailable."));
    // This is the ordinary Qt INI writer's lock. Never sync a live/cache-sharing
    // QSettings while holding it: only the isolated temporary INI is mutated.
    QLockFile lock(source->fileName() + QStringLiteral(".lock"));
    if (!lock.tryLock(0))
        return reject(error, tr("Another window is saving interface preferences. Retry shortly."));
    QTemporaryDir temporary;
    if (!temporary.isValid())
        return reject(error, tr("The interface preferences transaction could not be prepared."));
    QByteArray before;
    const auto stagedPath = temporary.filePath(QStringLiteral("transaction.ini"));
    if (!readIniBytes(source->fileName(), before, error) || !writeStagedIni(stagedPath, before, error)) return false;
    SettingsValue current;
    if (!readEffectiveIniValue(source, key, stagedPath, temporary.path(), current, error)
        || !compareValue(current, expected, conflict, error)) return false;
    {
        QSettings staged(stagedPath, QSettings::IniFormat);
        staged.setFallbacksEnabled(false);
        if (!source->group().isEmpty()) staged.beginGroup(source->group());
        if (remove) staged.remove(key); else staged.setValue(key, value);
        staged.sync();
        if (staged.status() != QSettings::NoError)
            return reject(error, tr("The interface preferences transaction could not be saved."));
    }
    QByteArray after;
    if (!readIniBytes(stagedPath, after, error)) return false;
    const auto verificationPath = temporary.filePath(QStringLiteral("verification.ini"));
    SettingsValue prepared;
    if (!writeStagedIni(verificationPath, after, error)
        || !readStagedValue(verificationPath, source, key, prepared, error)) return false;
    if ((remove && prepared.present) || (!remove && (!prepared.present || prepared.value != value)))
        return reject(error, tr("The prepared interface preference could not be verified."));
    // Size must change even for an equal-length replacement within one clock
    // tick, otherwise QSettings may reuse a stale same-process physical cache.
    if (after.size() == before.size()) after.append('\n');
    QSaveFile file(source->fileName());
    file.setDirectWriteFallback(false);
    if (!file.open(QIODevice::WriteOnly) || file.write(after) != after.size() || !file.commit())
        return reject(error, tr("The interface preference could not be saved. The previous values were preserved."));
    // Publication is the commit point. A newer writer after unlocking is a
    // legitimate successor; never mistake it for our failure and roll it back.
    return true;
}

bool saveSettingsValue(QSettings *source, const QString &key, const QVariant &value,
                       bool remove, const SettingsValue *expected, bool *conflict,
                       QString &error)
{
    error.clear();
    if (conflict) *conflict = false;
    if (!source) return reject(error, tr("The interface preferences storage is not writable."));
    // Flush unrelated caller edits before taking any physical writer lock. A
    // fresh session checks current errors rather than source's sticky status.
    source->sync();
    auto session = settingsSession(source);
    if (!session) return reject(error, tr("The interface preferences storage location has changed."));
    session->sync();
    if (!session->isWritable() || session->status() != QSettings::NoError)
        return reject(error, tr("The interface preferences storage is not writable."));
    if (iniStorage(source)) {
        if (!saveIniValue(source, key, value, remove, expected, conflict, error)) return false;
        // The isolated transaction has released .lock before refreshing a live
        // QSettings cache. No failed transaction ever dirtied that cache.
        source->sync();
        return true;
    }
    const auto identity = QCryptographicHash::hash(source->fileName().toCaseFolded().toUtf8(),
                                                  QCryptographicHash::Sha256).toHex();
    QLockFile lock(QDir::temp().filePath(QStringLiteral("vlc-checked-native-%1.lock")
                                       .arg(QString::fromLatin1(identity))));
    if (!lock.tryLock(0))
        return reject(error, tr("Another window is saving interface preferences. Retry shortly."));
    session->sync();
    if (session->status() != QSettings::NoError)
        return reject(error, tr("The interface preferences could not be read."));
    const SettingsValue current{session->contains(key), session->value(key)};
    if (!compareValue(current, expected, conflict, error)) return false;
    // Windows NativeFormat is a single registry-value operation. There is no
    // readback-triggered rollback: another accepted writer must remain intact.
    if (remove) session->remove(key); else session->setValue(key, value);
    session->sync();
    return session->status() == QSettings::NoError
        || reject(error, tr("The interface preference could not be saved. Reload and retry."));
}
}
bool CheckedPreferencesStore::syncCurrentSettings(QSettings *source)
{
    if (!source) return true;
    source->sync();
    auto session = settingsSession(source);
    if (!session) return false;
    session->sync();
    return session->status() == QSettings::NoError;
}
QString CheckedPreferencesStore::configFile() const
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
bool CheckedPreferencesStore::saveNative(const QVariantMap &changes, QString &error)
{
    error.clear();
    if (!m_intf) return reject(error, tr("The native preferences service is unavailable."));
    QVariantMap requested = changes;
    for (auto it = requested.begin(); it != requested.end(); ++it) {
        const auto key = it.key().toUtf8();
        const int type = config_GetType(key.constData());
        if (type == VLC_VAR_FLOAT) {
            // Models supply strict numeric fields as float/double. Preserve
            // the exact native float and its physical six-decimal serializer
            // round trip; do not report success after silently losing digits.
            if (it.value().typeId() != QMetaType::Float && it.value().typeId() != QMetaType::Double)
                return reject(error, tr("This native preference requires a number."));
            const float value = it.value().toFloat();
            bool valid = false;
            const float serialized = QLocale::c().toFloat(QString::number(double(value), 'f', 6), &valid);
            if (!std::isfinite(value) || !valid || serialized != value)
                return reject(error, tr("This number cannot be saved without losing precision. Use at most six decimal places."));
            it.value() = QVariant::fromValue(value);
        } else if ((it.value().typeId() == QMetaType::QString && type != VLC_VAR_STRING)
            || (it.value().typeId() != QMetaType::QString
                && type != VLC_VAR_BOOL && type != VLC_VAR_INTEGER))
            return reject(error, tr("This build does not provide the requested native preference."));
        // Retain the existing integer/bool conversion contract even if an
        // internal caller supplied a numeric QVariant with a float metatype.
        if (type != VLC_VAR_FLOAT && type != VLC_VAR_STRING)
            it.value() = QVariant::fromValue(qlonglong(it.value().toLongLong()));
    }
    QVector<QByteArray> names, strings;
    QVector<vlc_config_change> native;
    names.reserve(requested.size()); strings.reserve(requested.size());
    native.reserve(requested.size());
    for (auto it = requested.cbegin(); it != requested.cend(); ++it) {
        names.push_back(it.key().toUtf8());
        vlc_config_change change{};
        change.name = names.back().constData();
        change.type = config_GetType(change.name);
        if (change.type == VLC_VAR_STRING) {
            const auto text = it.value().toString();
            if (text.contains(QChar::Null) || text.contains('\n') || text.contains('\r'))
                return reject(error, tr("This text cannot be stored in the VLC configuration."));
            strings.push_back(text.toUtf8());
            change.value.str = strings.back().constData();
        } else if (change.type == VLC_VAR_FLOAT)
            change.value.f = it.value().toFloat();
        else
            change.value.i = it.value().toLongLong();
        native.push_back(change);
    }
    // Core owns mutation, unrelated-key merge, durable replacement and rollback
    // within one transaction. A later writer's valid commit must not be judged
    // against our values by a racy post-transaction file readback.
    if (config_SaveChanges(m_intf, native.constData(), size_t(native.size())) != 0)
        return reject(error, tr("The VLC configuration could not be saved. The previous in-memory values were restored."));
    return true;
}
bool CheckedPreferencesStore::saveSettings(QSettings *settings, const QString &key,
                                           const QVariant &value, bool remove, QString &error)
{
    return saveSettingsValue(settings, key, value, remove, nullptr, nullptr, error);
}

bool CheckedPreferencesStore::compareAndSaveSettings(QSettings *settings, const QString &key,
    const QVariant &value, const SettingsValue &expected, QString &error, bool *conflict)
{
    return saveSettingsValue(settings, key, value, false, &expected, conflict, error);
}

bool CheckedPreferencesStore::readSettingsValue(QSettings *settings, const QString &key,
    SettingsValue &result, QString &error)
{
    error.clear(); result = {};
    if (!settings) return reject(error, tr("The interface preferences storage is unavailable."));
    settings->sync();
    auto session = settingsSession(settings);
    if (!session) return reject(error, tr("The interface preferences storage location has changed."));
    if (!iniStorage(settings)) {
        session->sync(); result = {session->contains(key), session->value(key)};
        return session->status() == QSettings::NoError
            || reject(error, tr("The interface preferences could not be read."));
    }
    QTemporaryDir temporary; QByteArray bytes;
    if (!temporary.isValid()) return reject(error, tr("The interface preferences could not be read."));
    const auto path = temporary.filePath(QStringLiteral("snapshot.ini"));
    return readIniBytes(settings->fileName(), bytes, error) && writeStagedIni(path, bytes, error)
        && readEffectiveIniValue(settings, key, path, temporary.path(), result, error);
}
