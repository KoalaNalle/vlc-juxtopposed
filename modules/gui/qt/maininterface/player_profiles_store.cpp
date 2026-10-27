// SPDX-License-Identifier: GPL-2.0-or-later
#include "player_profiles_store.hpp"
#include "util/checked_preferences_store.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLockFile>
#include <QSet>
#include <QSettings>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QUuid>
#include <cmath>
#include <memory>

namespace {
QString tr(const char *text) { return QCoreApplication::translate("PlayerProfiles", text); }
bool reject(QString &error, const char *text) { error = tr(text); return false; }
bool cleanText(const QString &text, int maximum, bool empty = true)
{
    if ((!empty && text.isEmpty()) || text.size() > maximum
            || QString::fromUtf8(text.toUtf8()) != text) return false;
    for (const auto character : text) if (character.unicode() < 32) return false;
    return true;
}
bool identifier(const QString &id)
{
    if (!cleanText(id, 64, false)) return false;
    for (const auto character : id)
        if (!(character >= QLatin1Char('a') && character <= QLatin1Char('z'))
                && !(character >= QLatin1Char('0') && character <= QLatin1Char('9'))
                && character != QLatin1Char('-')) return false;
    return true;
}
bool valueShape(const QVariant &value, int depth, int &remaining)
{
    if (--remaining < 0 || depth > 6) return false;
    switch (value.typeId()) {
    case QMetaType::Bool: return true;
    case QMetaType::Int: case QMetaType::UInt: case QMetaType::LongLong:
    case QMetaType::ULongLong: case QMetaType::Float: case QMetaType::Double:
        return std::isfinite(value.toDouble());
    case QMetaType::QString: return cleanText(value.toString(), 32760);
    case QMetaType::QVariantList:
        if (value.toList().size() > 256) return false;
        for (const auto &child : value.toList()) if (!valueShape(child, depth + 1, remaining)) return false;
        return true;
    case QMetaType::QVariantMap:
        if (value.toMap().size() > 128) return false;
        {
            const auto map = value.toMap();
            for (auto it = map.cbegin(); it != map.cend(); ++it)
                if (!cleanText(it.key(), 128, false) || !valueShape(it.value(), depth + 1, remaining)) return false;
        }
        return true;
    default: return false;
    }
}
std::unique_ptr<QSettings> readerFor(const QSettings *source)
{
    if (!source) return {};
    std::unique_ptr<QSettings> reader;
    if (!source->organizationName().isEmpty() && source->fallbacksEnabled())
        reader = std::make_unique<QSettings>(source->format(), source->scope(), source->organizationName(), source->applicationName());
    else reader = std::make_unique<QSettings>(source->fileName(), source->format());
    if (reader->fileName() != source->fileName()) return {};
    reader->setFallbacksEnabled(source->fallbacksEnabled());
    reader->setAtomicSyncRequired(source->isAtomicSyncRequired());
    if (!source->group().isEmpty()) reader->beginGroup(source->group());
    return reader;
}
bool readValue(QSettings &reader, std::optional<QString> &text, QString &error)
{
    text.reset();
    if (reader.contains(PlayerProfilesStore::settingsKey())) {
        const auto value = reader.value(PlayerProfilesStore::settingsKey());
        if (value.typeId() != QMetaType::QString || value.toString().size() > PlayerProfilesStore::maximumDocumentBytes)
            return reject(error, "Saved player profiles could not be read. Restore the preferences file and retry.");
        text = value.toString();
        if (text->toUtf8().size() > PlayerProfilesStore::maximumDocumentBytes)
            return reject(error, "Saved player profiles exceed the supported size.");
    }
    if (reader.status() != QSettings::NoError)
        return reject(error, "Saved player profiles could not be read. Restore the preferences file and retry.");
    return true;
}
bool physicalIni(const QSettings *source, QByteArray &bytes, QString &error)
{
    QFile file(source->fileName()); bytes.clear();
    if (!file.exists()) return true;
    if (!file.open(QIODevice::ReadOnly) || file.size() > 64 * 1024 * 1024)
        return reject(error, "The interface preferences file could not be read or exceeds the supported size.");
    bytes = file.readAll();
    return file.error() == QFile::NoError || reject(error, "The interface preferences file could not be read.");
}
bool stageIni(const QString &path, const QByteArray &bytes, QString &error)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.flush())
        return reject(error, "Player profiles could not prepare an isolated preferences transaction.");
    return true;
}
bool readSource(const QSettings *source, std::optional<QString> &text, QString &error)
{
    auto reader = readerFor(source);
    if (!reader) return reject(error, "Profile preferences are unavailable or moved. Restart and retry.");
    reader->sync();
    if (source->format() != QSettings::IniFormat) return readValue(*reader,text,error);
    // The new per-user catalog reads the primary physical INI. A temporary
    // filename bypasses QSettings' process-wide QConfFile cache, including a
    // stale cache retained after another settings transaction failed.
    QTemporaryDir temporary; QByteArray bytes;
    if (!temporary.isValid() || !physicalIni(source,bytes,error))
        return reject(error, "Saved player profiles could not be read from the interface preferences file.");
    const auto path = temporary.filePath(QStringLiteral("snapshot.ini"));
    if (!stageIni(path,bytes,error)) return false;
    QSettings physical(path,QSettings::IniFormat); physical.setFallbacksEnabled(false);
    if (!source->group().isEmpty()) physical.beginGroup(source->group());
    return readValue(physical,text,error);
}
bool saveIni(QSettings *source, const std::optional<QString> &expected, const QString &text, QString &error)
{
    // Share Qt's ordinary INI writer lock as well as the catalog-specific lock
    // held by our caller. Do not sync the borrowed/cache-sharing settings while
    // holding this lock. The caller flushed its unrelated changes beforehand.
    QLockFile fileLock(source->fileName() + QStringLiteral(".lock"));
    if (!fileLock.tryLock(0)) return reject(error, "Another window is saving interface preferences. Retry shortly.");
    QTemporaryDir temporary; QByteArray before;
    if (!temporary.isValid()) return reject(error, "Player profiles could not prepare an isolated preferences transaction.");
    if (!physicalIni(source,before,error)) return false;
    const auto stagedPath = temporary.filePath(QStringLiteral("transaction.ini"));
    if (!stageIni(stagedPath,before,error)) return false;
    QSettings staged(stagedPath,QSettings::IniFormat); staged.setFallbacksEnabled(false);
    if (!source->group().isEmpty()) staged.beginGroup(source->group());
    std::optional<QString> current;
    if (!readValue(staged,current,error)) return false;
    if (current != expected) return reject(error, "Player profiles changed in another window. Restart to reload them before editing.");
    staged.setValue(PlayerProfilesStore::settingsKey(),text); staged.sync();
    if (staged.status() != QSettings::NoError) return reject(error, "Player profiles could not prepare a complete preferences transaction.");
    QFile prepared(stagedPath);
    if (!prepared.open(QIODevice::ReadOnly)) return reject(error, "The prepared player profiles could not be read.");
    auto after = prepared.readAll();
    if (prepared.error() != QFile::NoError) return reject(error, "The prepared player profiles could not be read.");
    prepared.close();
    // A same-size write within one filesystem clock tick must still invalidate
    // the borrowed QSettings file cache. Harmless trailing INI whitespace makes
    // the physical size differ even when the two serialized values are equal
    // in length; it never changes an unrelated preference value.
    if (after.size() == before.size()) after.append('\n');
    QSaveFile destination(source->fileName()); destination.setDirectWriteFallback(false);
    if (!destination.open(QIODevice::WriteOnly) || destination.write(after) != after.size() || !destination.commit())
        return reject(error, "The player profiles could not be saved. The previous profiles and recovery record were preserved.");
    QByteArray readback;
    if (!physicalIni(source,readback,error) || readback != after)
        return reject(error, "The player profiles save could not be verified. Reload before editing again.");
    // Only the isolated staged cache was ever changed. A failed rename cannot
    // leave an old profile key queued for a later borrowed-QSettings sync.
    return true;
}
QJsonObject switchObject(const PlayerProfileSwitch &pending)
{
    return {{QStringLiteral("source"), pending.sourceId}, {QStringLiteral("target"), pending.targetId},
        {QStringLiteral("before"), QJsonObject::fromVariantMap(pending.before)},
        {QStringLiteral("after"), QJsonObject::fromVariantMap(pending.after)}};
}
}

QString PlayerProfilesStore::settingsKey() { return QStringLiteral("Juxtopposed/player-profiles"); }

bool PlayerProfilesStore::validSnapshot(const QVariantMap &snapshot, QString &error)
{
    const QSet<QString> groups{QStringLiteral("playback"), QStringLiteral("audio"),
        QStringLiteral("video"), QStringLiteral("subtitles"), QStringLiteral("layout")};
    if (snapshot.size() != groups.size()) return reject(error, "A player profile must contain media settings and player layout only.");
    int remaining = 4096;
    for (const auto &group : groups) {
        if (snapshot.value(group).typeId() != QMetaType::QVariantMap
                || !valueShape(snapshot.value(group), 0, remaining))
            return reject(error, "The player profile contains invalid settings.");
    }
    error.clear();
    return true;
}

int PlayerProfilesStore::indexOf(const QString &id) const
{
    for (qsizetype i = 0; i < m_profiles.size(); ++i) if (m_profiles[i].id == id) return int(i);
    return -1;
}

bool PlayerProfilesStore::reload(const QVariantMap &initial, QString &error)
{
    std::optional<QString> source;
    m_readable = false;
    if (!readSource(m_settings, source, error)) return false;
    QVector<PlayerProfile> profiles;
    QString selected;
    std::optional<PlayerProfileSwitch> pending;
    if (!source) {
        if (!validSnapshot(initial, error)) return false;
        const char *ids[] = {"movies", "shows", "music", "work"};
        const char *names[] = {QT_TRANSLATE_NOOP("PlayerProfiles", "Movies"), QT_TRANSLATE_NOOP("PlayerProfiles", "Shows"),
            QT_TRANSLATE_NOOP("PlayerProfiles", "Music"), QT_TRANSLATE_NOOP("PlayerProfiles", "Work")};
        for (int i = 0; i < 4; ++i) profiles.push_back({QString::fromLatin1(ids[i]), tr(names[i]), initial});
        selected = QStringLiteral("movies");
    } else {
        QJsonParseError parseError;
        const auto document = QJsonDocument::fromJson(source->toUtf8(), &parseError);
        const auto root = document.object();
        const auto rows = root.value(QStringLiteral("profiles"));
        if (parseError.error != QJsonParseError::NoError || !document.isObject()
                || root.value(QStringLiteral("version")).toDouble(-1) != 1
                || !rows.isArray() || rows.toArray().isEmpty() || rows.toArray().size() > maximumProfiles
                || !root.value(QStringLiteral("selected")).isString())
            return reject(error, "Saved player profiles could not be read. Restore the preferences file and retry.");
        QSet<QString> ids, names;
        for (const auto &row : rows.toArray()) {
            const auto item = row.toObject();
            PlayerProfile profile{item.value(QStringLiteral("id")).toString(), item.value(QStringLiteral("name")).toString(),
                item.value(QStringLiteral("snapshot")).toObject().toVariantMap()};
            if (!row.isObject() || !identifier(profile.id) || !cleanText(profile.name, 128, false)
                    || profile.name.trimmed() != profile.name || ids.contains(profile.id)
                    || names.contains(profile.name.toCaseFolded()) || !validSnapshot(profile.snapshot, error))
                return reject(error, "Saved player profiles contain invalid or duplicate entries. Restore the preferences file and retry.");
            ids.insert(profile.id); names.insert(profile.name.toCaseFolded()); profiles.push_back(std::move(profile));
        }
        selected = root.value(QStringLiteral("selected")).toString();
        if (!ids.contains(selected)) return reject(error, "The saved active player profile is unavailable.");
        const auto savedPending = root.value(QStringLiteral("pending"));
        if (!savedPending.isUndefined()) {
            const auto item = savedPending.toObject();
            PlayerProfileSwitch change{item.value(QStringLiteral("source")).toString(), item.value(QStringLiteral("target")).toString(),
                item.value(QStringLiteral("before")).toObject().toVariantMap(), item.value(QStringLiteral("after")).toObject().toVariantMap()};
            if (!savedPending.isObject() || change.sourceId != selected || change.sourceId == change.targetId
                    || !ids.contains(change.targetId) || !validSnapshot(change.before, error) || !validSnapshot(change.after, error))
                return reject(error, "The unfinished player profile change could not be read. Restore the preferences file and retry.");
            for (const auto &profile : profiles)
                if ((profile.id == change.sourceId && profile.snapshot != change.before)
                        || (profile.id == change.targetId && profile.snapshot != change.after))
                    return reject(error, "The unfinished player profile change does not match its saved profiles. Restore the preferences file and retry.");
            pending = std::move(change);
        }
    }
    m_profiles = std::move(profiles); m_selected = selected; m_pending = std::move(pending);
    m_source = std::move(source); m_readable = true; error.clear();
    return true;
}

bool PlayerProfilesStore::save(const QVector<PlayerProfile> &profiles, const QString &selected,
                             const std::optional<PlayerProfileSwitch> &pending, QString &error)
{
    if (!m_readable || !m_settings) return reject(error, "Reload the saved player profiles before editing them.");
    const QFileInfo destination(m_settings->fileName());
    if (!destination.isAbsolute() || !QDir().mkpath(destination.absolutePath()))
        return reject(error, "The player profiles folder is unavailable.");
    if (m_settings->format() == QSettings::IniFormat && !CheckedPreferencesStore::syncCurrentSettings(m_settings))
        return reject(error, "The interface preferences could not finish their current save. Retry after storage is available.");
    QLockFile lock(destination.absoluteFilePath() + QStringLiteral(".player-profiles.lock"));
    if (!lock.tryLock(0)) return reject(error, "Another window is saving player profiles. Retry shortly.");
    QJsonArray rows;
    for (const auto &profile : profiles)
        rows.append(QJsonObject{{QStringLiteral("id"), profile.id}, {QStringLiteral("name"), profile.name},
            {QStringLiteral("snapshot"), QJsonObject::fromVariantMap(profile.snapshot)}});
    QJsonObject root{{QStringLiteral("version"), 1}, {QStringLiteral("selected"), selected}, {QStringLiteral("profiles"), rows}};
    if (pending) root.insert(QStringLiteral("pending"), switchObject(*pending));
    const auto encoded = QJsonDocument(root).toJson(QJsonDocument::Compact);
    if (encoded.size() > maximumDocumentBytes) return reject(error, "Player profiles exceed the supported size.");
    const auto serialized = QString::fromUtf8(encoded);
    if (m_settings->format() == QSettings::IniFormat) {
        if (!saveIni(m_settings,m_source,serialized,error)) return false;
        m_settings->sync();
    } else {
        std::optional<QString> current;
        if (!readSource(m_settings,current,error)) return false;
        if (current != m_source) return reject(error, "Player profiles changed in another window. Reload and retry.");
        if (!CheckedPreferencesStore(nullptr).saveSettings(m_settings,settingsKey(),serialized,false,error)) return false;
    }
    m_profiles = profiles; m_selected = selected; m_pending = pending; m_source = serialized;
    error.clear();
    return true;
}

bool PlayerProfilesStore::checkpoint(const QVariantMap &current, QString &error)
{
    if (!m_readable || m_pending) return reject(error, "Finish or restore the current player profile change first.");
    if (!validSnapshot(current, error)) return false;
    const auto row = indexOf(m_selected);
    if (row < 0) return reject(error, "The active player profile is unavailable.");
    if (m_profiles[row].snapshot == current) { error.clear(); return true; }
    auto candidate = m_profiles; candidate[row].snapshot = current;
    return save(candidate, m_selected, {}, error);
}

bool PlayerProfilesStore::create(const QString &input, const QVariantMap &current, QString &newId, QString &error)
{
    newId.clear();
    if (!m_readable || m_pending) return reject(error, "Finish or restore the current player profile change first.");
    const auto name = input.trimmed();
    if (!cleanText(name, 128, false)) return reject(error, "Enter a profile name of up to 128 characters.");
    for (const auto &profile : m_profiles)
        if (profile.name.toCaseFolded() == name.toCaseFolded()) return reject(error, "A player profile already uses that name.");
    if (m_profiles.size() >= maximumProfiles) return reject(error, "The player supports up to 64 profiles.");
    if (!validSnapshot(current, error)) return false;
    const auto row = indexOf(m_selected);
    if (row < 0) return reject(error, "The active player profile is unavailable.");
    auto candidate = m_profiles; candidate[row].snapshot = current;
    const auto id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    candidate.push_back({id, name, current});
    if (!save(candidate, id, {}, error)) return false;
    newId = id;
    return true;
}

bool PlayerProfilesStore::beginSwitch(const QString &targetId, const QVariantMap &current, QString &error)
{
    if (!m_readable || m_pending) return reject(error, "Finish or restore the current player profile change first.");
    const auto source = indexOf(m_selected), target = indexOf(targetId);
    if (source < 0 || target < 0 || source == target) return reject(error, "Choose another available player profile.");
    if (!validSnapshot(current, error)) return false;
    auto candidate = m_profiles; candidate[source].snapshot = current;
    return save(candidate, m_selected, PlayerProfileSwitch{m_selected, targetId, current, candidate[target].snapshot}, error);
}

bool PlayerProfilesStore::finishSwitch(const QVariantMap &applied, QString &error)
{
    if (!m_readable || !m_pending) return reject(error, "No player profile change is pending.");
    if (applied != m_pending->after) return reject(error, "The selected player profile is not fully applied. Retry or restore the previous profile.");
    return save(m_profiles, m_pending->targetId, {}, error);
}

bool PlayerProfilesStore::cancelSwitch(const QVariantMap &restored, QString &error)
{
    if (!m_readable || !m_pending) return reject(error, "No player profile change is pending.");
    if (restored != m_pending->before) return reject(error, "The previous player profile is not fully restored. Retry before continuing.");
    return save(m_profiles, m_pending->sourceId, {}, error);
}
