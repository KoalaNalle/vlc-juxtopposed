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
