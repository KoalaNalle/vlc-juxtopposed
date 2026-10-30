// SPDX-License-Identifier: GPL-2.0-or-later
#include "sidebar_pins_store.hpp"
#include "util/checked_preferences_store.hpp"

#include <QCryptographicHash>
#include <QCoreApplication>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSet>
#include <QSettings>
#include <algorithm>
#include <memory>

namespace {
QUrl normalizedAddress(QUrl url, bool directory)
{
    url = url.adjusted(QUrl::NormalizePathSegments);
    if (!url.password().isEmpty())
        url.setPassword(QString());
    auto path = url.path();
    const bool driveRoot = path.size() == 4 && path[0] == QLatin1Char('/')
        && path[1].isLetter() && path[2] == QLatin1Char(':')
        && path[3] == QLatin1Char('/');
    if (directory && path.size() > 1 && !driveRoot && path.endsWith(QLatin1Char('/')))
    {
        while (path.size() > 1 && path.endsWith(QLatin1Char('/')))
            path.chop(1);
        url.setPath(path);
    }
    return url;
}

bool positiveId(const QString &value)
{
    bool ok = false;
    const auto id = value.toLongLong(&ok);
    return ok && id > 0 && QString::number(id) == value;
}

QString identity(const SidebarPin &pin)
{
    const bool collection = pin.kind == QStringLiteral("playlist")
        || pin.kind == QStringLiteral("artist");
    const auto target = collection ? pin.nativeScope + QLatin1Char('/') + pin.nativeId
        : pin.mrl.toString(QUrl::FullyEncoded);
    return pin.kind + QLatin1Char(':') + QString::fromLatin1(QCryptographicHash::hash(
        target.toUtf8(), QCryptographicHash::Sha256).toHex());
}
}

QVariantMap SidebarPin::toVariant() const
{
    return {{QStringLiteral("key"), key}, {QStringLiteral("kind"), kind},
        {QStringLiteral("title"), title},
        {QStringLiteral("mrl"), mrl.toString(QUrl::FullyEncoded)},
        {QStringLiteral("artwork"), artwork.toString(QUrl::FullyEncoded)},
        {QStringLiteral("nativeId"), nativeId},
        {QStringLiteral("nativeScope"), nativeScope}};
}

bool SidebarPin::normalize(SidebarPin &pin, QString &error)
{
    const bool collection = pin.kind == QStringLiteral("playlist")
        || pin.kind == QStringLiteral("artist");
    const bool directory = pin.kind == QStringLiteral("folder")
        || pin.kind == QStringLiteral("source");
    if (!collection && !directory && pin.kind != QStringLiteral("media"))
    {
        error = QCoreApplication::translate("SidebarPins", "This item cannot be pinned.");
        return false;
    }
    pin.title = pin.title.trimmed();
    if (pin.title.size() > 1024 || pin.nativeScope.size() > 256
        || (!pin.nativeId.isEmpty() && !positiveId(pin.nativeId))
        || (pin.nativeId.isEmpty() != pin.nativeScope.isEmpty()))
    {
        error = QCoreApplication::translate("SidebarPins", "The pin contains invalid item information.");
        return false;
    }
    if (collection && (pin.nativeId.isEmpty() || pin.nativeScope.isEmpty()))
    {
        error = QCoreApplication::translate("SidebarPins", "The collection is no longer available.");
        return false;
    }
    if (!pin.mrl.isEmpty())
    {
        if (!pin.mrl.isValid() || pin.mrl.isRelative() || pin.mrl.scheme().isEmpty()
            || pin.mrl.toEncoded().size() > 8192
            || (pin.mrl.isLocalFile() && !QDir::isAbsolutePath(pin.mrl.toLocalFile())))
        {
            error = QCoreApplication::translate("SidebarPins", "Enter a complete file or source address.");
            return false;
        }
        pin.mrl = normalizedAddress(pin.mrl, directory);
    }
    if (!collection && pin.mrl.isEmpty())
    {
        error = QCoreApplication::translate("SidebarPins", "This item has no address to pin.");
        return false;
    }
    if (pin.title.isEmpty())
    {
        pin.title = pin.mrl.fileName(QUrl::FullyDecoded);
        if (pin.title.isEmpty())
            pin.title = pin.mrl.toDisplayString(QUrl::RemoveUserInfo | QUrl::RemoveQuery
                                               | QUrl::RemoveFragment);
        if (pin.title.isEmpty())
            pin.title = pin.kind;
        pin.title = pin.title.left(1024);
    }
    // Use local/native cached artwork. Rendering a pin must not start a new
    // metadata-network request or persist credentials in an artwork address.
    if (!pin.artwork.isValid() || pin.artwork.toEncoded().size() > 8192
        || (!pin.artwork.isEmpty() && pin.artwork.scheme() != QStringLiteral("file")
            && pin.artwork.scheme() != QStringLiteral("qrc")))
        pin.artwork = QUrl();
    pin.key = identity(pin);
    error.clear();
    return true;
}

bool SidebarPin::fromVariant(const QVariantMap &value, SidebarPin &pin, QString &error)
{
    for (const auto *name : {"key", "kind", "title", "mrl", "artwork", "nativeId", "nativeScope"})
    {
        const auto entry = value.value(QString::fromLatin1(name));
        if (entry.isValid() && entry.metaType().id() != QMetaType::QString)
        {
            error = QCoreApplication::translate("SidebarPins", "The saved pin contains invalid text.");
            return false;
        }
    }
    pin.kind = value.value(QStringLiteral("kind")).toString();
    pin.title = value.value(QStringLiteral("title")).toString();
    pin.mrl = QUrl(value.value(QStringLiteral("mrl")).toString(), QUrl::StrictMode);
    pin.artwork = QUrl(value.value(QStringLiteral("artwork")).toString(), QUrl::StrictMode);
    pin.nativeId = value.value(QStringLiteral("nativeId")).toString();
    pin.nativeScope = value.value(QStringLiteral("nativeScope")).toString();
    const auto savedKey = value.value(QStringLiteral("key")).toString();
    if (!normalize(pin, error))
        return false;
    if (!savedKey.isEmpty() && savedKey != pin.key)
    {
        error = QCoreApplication::translate("SidebarPins", "The saved pin identity is invalid.");
        return false;
    }
    return true;
}

QString SidebarPinsStore::settingsKey()
{
    return QStringLiteral("juxtopposed/sidebar-pins");
}

SidebarPinsStore::SidebarPinsStore(QSettings *settings) : m_settings(settings)
{
    QString error;
    reload(error);
}

bool SidebarPinsStore::reload(QString &error)
{
    if (!m_settings)
    {
        m_readable = false;
        error = m_readError = QCoreApplication::translate("SidebarPins", "Pin preferences are unavailable.");
        return false;
    }
    // A previous failed writer can leave the borrowed object's status sticky.
    // Read through a fresh session so fixing the file permits the same store
    // object to retry without discarding the last accepted in-memory order.
    CheckedPreferencesStore::SettingsValue source;
    if (!CheckedPreferencesStore::readSettingsValue(m_settings, settingsKey(), source, error))
    {
        m_readable = false;
        m_readError = error;
        return false;
    }
    QVector<SidebarPin> candidate;
    QSet<QString> keys;
    if (source.present)
    {
        const auto saved = source.value;
        QJsonParseError parseError;
        const auto document = QJsonDocument::fromJson(saved.toString().toUtf8(), &parseError);
        const auto envelope = document.object();
        const auto version = envelope.value(QStringLiteral("version"));
        const auto rowsValue = envelope.value(QStringLiteral("pins"));
        const auto rows = rowsValue.toArray();
        if (saved.metaType().id() != QMetaType::QString
            || parseError.error != QJsonParseError::NoError || !document.isObject()
            || !version.isDouble() || version.toDouble() != 1.0
            || !rowsValue.isArray() || rows.size() > maximumPins)
        {
            m_readable = false;
            error = m_readError = QCoreApplication::translate("SidebarPins", "Saved pins could not be read. Restore the preferences file and retry.");
            return false;
        }
        for (const auto &row : rows)
        {
            SidebarPin pin;
            QString detail;
            if (!row.isObject() || !SidebarPin::fromVariant(row.toObject().toVariantMap(), pin, detail))
            {
                m_readable = false;
                error = m_readError = QCoreApplication::translate("SidebarPins", "Saved pins contain invalid item information. Restore the preferences file and retry.");
                return false;
            }
            if (!keys.contains(pin.key))
            {
                keys.insert(pin.key);
                candidate.push_back(std::move(pin));
            }
        }
    }
    m_pins = std::move(candidate);
    m_sourceValue = source;
    m_readable = true;
    m_needsReload = false;
    m_readError.clear();
    error.clear();
    return true;
}

int SidebarPinsStore::indexOf(const QString &key) const
{
    for (qsizetype i = 0; i < m_pins.size(); ++i)
        if (m_pins[i].key == key)
            return static_cast<int>(i);
    return -1;
}

bool SidebarPinsStore::save(const QVector<SidebarPin> &candidate, QString &error)
{
    if (!m_readable)
    {
        error = m_readError;
        return false;
    }
    QVariantList rows;
    rows.reserve(candidate.size());
    for (const auto &pin : candidate)
        rows.push_back(pin.toVariant());
    const QVariantMap envelope{{QStringLiteral("version"), 1}, {QStringLiteral("pins"), rows}};
    // Store plain text, not QSettings' binary QVariant container stream.
    // A cold process must read the exact same IDs and Unicode titles without
    // relying on the writer process' shared INI cache or stream registrations.
    const auto serialized = QString::fromUtf8(QJsonDocument(
        QJsonObject::fromVariantMap(envelope)).toJson(QJsonDocument::Compact));
    bool conflict = false;
    if (!CheckedPreferencesStore(nullptr).compareAndSaveSettings(m_settings, settingsKey(),
            serialized, m_sourceValue, error, &conflict))
    {
        m_needsReload = m_needsReload || conflict;
        return false;
    }
    m_pins = candidate;
    m_sourceValue = {true, serialized};
    error.clear();
    return true;
}

bool SidebarPinsStore::add(const QVector<SidebarPin> &pins, QString &error)
{
    auto candidate = m_pins;
    QSet<QString> keys;
    for (const auto &pin : candidate)
        keys.insert(pin.key);
    for (auto pin : pins)
    {
        if (!SidebarPin::normalize(pin, error))
            return false;
        if (keys.contains(pin.key))
            continue;
        if (candidate.size() >= maximumPins)
        {
            error = QCoreApplication::translate("SidebarPins", "The sidebar can hold up to 512 pinned items.");
            return false;
        }
        keys.insert(pin.key);
        candidate.push_back(std::move(pin));
    }
    if (candidate.size() == m_pins.size())
    {
        if (!m_readable) { error = m_readError; return false; }
        error.clear();
        return true;
    }
    return save(candidate, error);
}

bool SidebarPinsStore::remove(const QString &key, QString &error)
{
    const auto row = indexOf(key);
    if (row < 0)
    {
        if (!m_readable) { error = m_readError; return false; }
        error.clear();
        return true;
    }
    auto candidate = m_pins;
    candidate.removeAt(row);
    return save(candidate, error);
}

bool SidebarPinsStore::move(const QString &key, int destination, QString &error)
{
    const auto row = indexOf(key);
    if (row < 0 || destination < 0 || destination >= m_pins.size())
    {
        error = QCoreApplication::translate("SidebarPins", "The pinned item could not be moved.");
        return false;
    }
    if (row == destination)
    {
        if (!m_readable) { error = m_readError; return false; }
        error.clear();
        return true;
    }
    auto candidate = m_pins;
    candidate.move(row, destination);
    return save(candidate, error);
}

