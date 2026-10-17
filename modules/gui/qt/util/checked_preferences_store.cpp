// SPDX-License-Identifier: GPL-2.0-or-later
// Generated from immutable General V5; retain its save/recovery behavior.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "checked_preferences_store.hpp"
#include "qt.hpp"
#include <vlc_configuration.h>
#include <QCoreApplication>
#include <QSettings>
#include <QFile>
#include <QDir>
#include <memory>
#include <cmath>
#include <QLocale>
namespace {
QString tr(const char *text) { return QCoreApplication::translate("GeneralPreferencesModel", text); }
bool reject(QString &error, const QString &message) { error = message; return false; }
QString nativeString(const char *key)
{
    if (config_GetType(key) != VLC_VAR_STRING) return {};
    char *raw = config_GetPsz(key);
    const auto result = QString::fromUtf8(raw ? raw : "");
    free(raw);
    return result;
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
    QVariantMap before;
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
        before.insert(it.key(), type == VLC_VAR_STRING ? QVariant(nativeString(key.constData()))
            : type == VLC_VAR_FLOAT ? QVariant::fromValue(config_GetFloat(key.constData()))
            : QVariant(qlonglong(config_GetInt(key.constData()))));
    }
    auto assign = [](const QVariantMap &values) {
        // Do not expose an old proxy secret with a new authority while the
        // per-key native setters run. Do the same before a failed write rolls
        // back to its previous endpoint. Core snapshots this pair together.
        if (values.contains(QStringLiteral("http-proxy"))
            && values.contains(QStringLiteral("http-proxy-pwd"))
            && nativeString("http-proxy") != values.value(QStringLiteral("http-proxy")).toString())
            config_PutPsz("http-proxy-pwd", "");
        for (auto it = values.cbegin(); it != values.cend(); ++it) {
            const auto key = it.key().toUtf8();
            if (it.value().typeId() == QMetaType::QString) {
                const auto text = it.value().toString().toUtf8();
                config_PutPsz(key.constData(), text.constData());
            } else if (it.value().typeId() == QMetaType::Float)
                config_PutFloat(key.constData(), it.value().toFloat());
            else config_PutInt(key.constData(), it.value().toLongLong());
        }
    };
    assign(requested);
    bool matches = true;
    for (auto it = requested.cbegin(); it != requested.cend(); ++it) {
        const auto key = it.key().toUtf8();
        matches &= it.value().typeId() == QMetaType::QString
            ? nativeString(key.constData()) == it.value().toString()
            : it.value().typeId() == QMetaType::Float
            ? config_GetFloat(key.constData()) == it.value().toFloat()
            : config_GetInt(key.constData()) == it.value().toLongLong();
    }
    if (!matches) {
        assign(before);
        return reject(error, tr("The native preference could not be updated. The previous value was restored."));
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
        for (auto it = requested.cbegin(); it != requested.cend(); ++it) {
            const QByteArray expected = it.key().toUtf8() + '=' + (it.value().typeId() == QMetaType::QString
                ? it.value().toString().toUtf8() : it.value().typeId() == QMetaType::Float
                ? QByteArray::number(double(it.value().toFloat()), 'f', 6)
                : QByteArray::number(it.value().toLongLong()));
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
        return reject(error, tr("The VLC configuration could not be saved. The previous in-memory values were restored."));
    }
    return true;
}
bool CheckedPreferencesStore::saveSettings(QSettings *settings, const QString &key,
                                           const QVariant &value, bool remove, QString &error)
{
    error.clear();
    if (!settings)
        return reject(error, tr("The interface preferences storage is not writable."));
    // Flush unrelated caller changes using its own policy before opening a
    // fresh session. Never interpret its permanently retained first error as
    // a current failure after permissions/file-sharing have been repaired.
    settings->sync();
    auto transaction = settingsSession(settings);
    auto primary = settingsSession(settings);
    if (!transaction || !primary)
        return reject(error, tr("The interface preferences storage location has changed."));
    transaction->sync();
    primary->setFallbacksEnabled(false);
    primary->sync();
    const bool existed = primary->contains(key);
    const auto before = primary->value(key);
    if (!transaction->isWritable() || transaction->status() != QSettings::NoError
        || primary->status() != QSettings::NoError)
        return reject(error, tr("The interface preferences storage is not writable."));
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
    return reject(error, tr("The interface preference could not be saved. The requested change was not accepted."));
}
