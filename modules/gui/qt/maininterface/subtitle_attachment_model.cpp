// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "subtitle_attachment_model.hpp"
#include "util/checked_preferences_store.hpp"
#include <vlc_interface.h>
#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTimer>
#include <algorithm>

using AttachmentPlayerLock = vlc_locker<vlc_player_t, vlc_player_Lock, vlc_player_Unlock>;

namespace {
const QString SettingsKey = QStringLiteral("Juxtopposed/SubtitleAttachments/v1");
constexpr int MaxRecords = 256, MaxRows = 1024, MaxFiles = 8, MaxEntries = 4096, MaxBytes = 2 * 1024 * 1024;
QSet<QString> subtitleExtensions()
{
    QSet<QString> result;
    for (const auto &pattern : QStringLiteral(EXTENSIONS_SUBTITLE).split(';', Qt::SkipEmptyParts))
        result.insert(pattern.mid(2)); // The same *.extension patterns used by the native picker.
    return result;
}
const QSet<QString> SubtitleExtensions = subtitleExtensions();
const QSet<QString> MediaExtensions { EXTENSIONS_VIDEO_CSV, EXTENSIONS_AUDIO_CSV };
QString stem(const QUrl &url) { return QFileInfo(url.toLocalFile()).completeBaseName().toCaseFolded(); }
QString episode(const QString &name)
{
    static const QRegularExpression expression(QStringLiteral("(?:s(\\d{1,3})[ ._-]*e(\\d{1,4})|(?<!\\d)(\\d{1,3})x(\\d{1,4})(?!\\d))"), QRegularExpression::CaseInsensitiveOption);
    const auto match = expression.match(name);
    if (!match.hasMatch()) return {};
    const auto season = match.captured(1).isEmpty() ? match.captured(3) : match.captured(1);
    const auto number = match.captured(2).isEmpty() ? match.captured(4) : match.captured(2);
    return QString::number(season.toInt()) + ':' + QString::number(number.toInt());
}
QUrl itemUrl(input_item_t *item)
{
    if (!item) return {};
    char *uri = input_item_GetURI(item);
    const auto result = uri ? QUrl::fromEncoded(uri) : QUrl{};
    free(uri); return result;
}
}

QUrl SubtitleAttachmentModel::localUrl(const QUrl &url)
{
    if (!url.isValid() || !url.isLocalFile() || !url.userInfo().isEmpty()
        || url.hasQuery() || url.hasFragment() || url.toLocalFile().contains(QChar::Null)) return {};
    const auto path = url.toLocalFile();
    if (path.isEmpty() || !QDir::isAbsolutePath(path)) return {};
    return QUrl::fromLocalFile(QDir::cleanPath(QFileInfo(path).absoluteFilePath()));
}
QString SubtitleAttachmentModel::identity(const QUrl &url)
{
    auto normalized = localUrl(url);
    if (normalized.isEmpty()) return {};
    QString path = normalized.toLocalFile();
#ifdef _WIN32
    path = path.toCaseFolded();
#endif
    return QString::fromLatin1(QCryptographicHash::hash(QUrl::fromLocalFile(path).toEncoded(), QCryptographicHash::Sha256).toHex());
}
bool SubtitleAttachmentModel::subtitleFile(const QUrl &url, bool requireExists)
{
    const auto normalized = localUrl(url);
    if (normalized.isEmpty() || normalized.toString().size() > 4096) return false;
    const QFileInfo info(normalized.toLocalFile());
    return SubtitleExtensions.contains(info.suffix().toLower())
        && (!requireExists || (info.isFile() && info.isReadable()));
}
QString SubtitleAttachmentModel::owner(int scope, const QUrl &url, qint64 playlistId) const
{
    if (scope == ThisFile) return identity(url);
    if (scope == ThisFolder && url.isLocalFile())
        return identity(QUrl::fromLocalFile(QFileInfo(url.toLocalFile()).absolutePath()));
    return scope == ThisPlaylist && playlistId > 0 ? QString::number(playlistId) : QString{};
}
const SubtitleAttachmentModel::Record *SubtitleAttachmentModel::find(int scope, const QString &owner) const
{
    for (const auto &record : m_records) if (record.scope == scope && record.owner == owner) return &record;
    return nullptr;
}
SubtitleAttachmentModel::SubtitleAttachmentModel(vlc_player_t *player, QSettings *settings,
    ContextProvider provider, DefaultSelectionPolicy selectionPolicy, QObject *parent)
    : QObject(parent), m_player(player), m_settings(settings), m_context(std::move(provider)),
      m_defaultSelection(std::move(selectionPolicy))
{
    if (player) {
        static const vlc_player_cbs callbacks = [] {
            vlc_player_cbs c{}; c.on_current_media_changed = mediaChanged; c.on_state_changed = stateChanged;
            c.on_track_list_changed = tracksChanged; return c;
        }();
        AttachmentPlayerLock lock{player}; m_listener = vlc_player_AddListener(player, &callbacks, this);
    }
    reload();
}
SubtitleAttachmentModel::~SubtitleAttachmentModel()
{
    if (m_listener) { AttachmentPlayerLock lock{m_player}; vlc_player_RemoveListener(m_player, m_listener); }
}
void SubtitleAttachmentModel::mediaChanged(vlc_player_t *, input_item_t *, void *opaque)
{
    auto *self = static_cast<SubtitleAttachmentModel *>(opaque); ++self->m_generation; self->queueRefresh();
}
void SubtitleAttachmentModel::stateChanged(vlc_player_t *, vlc_player_state state, void *opaque)
{
    auto *self = static_cast<SubtitleAttachmentModel *>(opaque);
    if (state == VLC_PLAYER_STATE_STARTED) ++self->m_generation;
    self->queueRefresh();
}
void SubtitleAttachmentModel::tracksChanged(vlc_player_t *, vlc_player_list_action, const vlc_player_track *track, void *opaque)
{
    if (track->fmt.i_cat == SPU_ES) static_cast<SubtitleAttachmentModel *>(opaque)->queueRefresh();
}
void SubtitleAttachmentModel::queueRefresh()
{
    if (m_refreshQueued.exchange(true)) return;
    QMetaObject::invokeMethod(this, [this] { m_refreshQueued = false; refresh(); }, Qt::QueuedConnection);
}
SubtitleAttachmentModel::Context SubtitleAttachmentModel::context(bool includeItems) const
{
    if (m_context) return m_context(includeItems);
    Context result;
    if (m_player) { AttachmentPlayerLock lock{m_player}; result.item.reset(vlc_player_GetCurrentMedia(m_player)); }
    return result;
}
void SubtitleAttachmentModel::fail(const QString &error) { m_error = error; emit changed(); }
void SubtitleAttachmentModel::failLoad(const QString &error)
{
    const bool notify = m_loadError != error;
    m_loadError = error;
    fail(error);
    if (notify) emit loadFailed(qtr("Subtitles did not load. Open Add Subtitles to review them."));
}
void SubtitleAttachmentModel::clearLoadFailure()
{
    if (m_loadError.isEmpty()) return;
    if (m_error == m_loadError) m_error.clear();
    m_loadError.clear(); emit changed();
}
void SubtitleAttachmentModel::reload()
{
    cancel(); m_records.clear(); m_storageValid = bool(m_settings); m_error.clear();
    m_explicitDefaultGeneration = UINT64_MAX;
    ++m_loadSerial; m_pendingLoads.clear(); m_loadError.clear();
    const auto bytes = m_settings ? m_settings->value(SettingsKey).toByteArray() : QByteArray{};
    if (bytes.size() > MaxBytes) {
        m_storageValid = false;
        fail(qtr("Saved subtitle attachments are too large to read. They have not been overwritten."));
        return;
    }
    if (!bytes.isEmpty()) {
        QJsonParseError parse; const auto document = QJsonDocument::fromJson(bytes, &parse);
        const auto root = document.object(); const auto entries = root.value("records");
        bool valid = bytes.size() <= MaxBytes && parse.error == QJsonParseError::NoError && document.isObject()
            && root.value("schema").toInt(-1) == 1 && entries.isArray() && entries.toArray().size() <= MaxRecords;
        static const QRegularExpression hash(QStringLiteral("^[0-9a-f]{64}$"));
        static const QRegularExpression playlist(QStringLiteral("^[1-9][0-9]{0,18}$"));
        QSet<QString> seen;
        for (const auto &value : entries.toArray()) {
            const auto o = value.toObject(); Record r;
            r.scope = o.value("scope").toInt(-1); r.owner = o.value("owner").toString();
            const auto rows = o.value("files"); bool idOk = false;
            const auto numericId = r.owner.toLongLong(&idOk);
            const bool ownerOk = r.scope == ThisPlaylist
                ? playlist.match(r.owner).hasMatch() && idOk && numericId > 0
                : hash.match(r.owner).hasMatch();
            valid &= value.isObject() && o.value("scope").isDouble() && o.value("scope").toDouble() == r.scope
                && r.scope >= ThisFile && r.scope <= ThisPlaylist && ownerOk && rows.isObject()
                && !rows.toObject().isEmpty() && rows.toObject().size() <= MaxRows;
            const auto recordKey = QString::number(r.scope) + ':' + r.owner;
            valid &= !seen.contains(recordKey); seen.insert(recordKey);
            const auto mappings = rows.toObject();
            for (auto it = mappings.begin(); it != mappings.end(); ++it) {
                valid &= hash.match(it.key()).hasMatch() && it.value().isArray()
                    && !it.value().toArray().isEmpty() && it.value().toArray().size() <= MaxFiles;
                QStringList urls; QSet<QString> used;
                for (const auto &file : it.value().toArray()) {
                    const auto url = QUrl::fromEncoded(file.toString().toUtf8());
                    const auto key = identity(url);
                    valid &= file.isString() && subtitleFile(url, false) && !used.contains(key);
                    used.insert(key); urls.append(localUrl(url).toString(QUrl::FullyEncoded));
                }
                r.files.insert(it.key(), urls);
            }
            if (r.scope == ThisFile) valid &= r.files.size() == 1 && r.files.contains(r.owner);
            m_records.append(std::move(r));
        }
        if (!valid) { m_storageValid = false; m_records.clear(); m_error = qtr("Saved subtitle attachments could not be read. They have not been overwritten."); }
    }
    m_appliedGeneration = UINT64_MAX; refresh(); emit changed();
}
QStringList SubtitleAttachmentModel::effectiveFiles(const QUrl &url, qint64 playlistId) const
{
    const auto key = identity(url);
    if (!m_storageValid || key.isEmpty()) return {};
    for (const int scope : {ThisFile, ThisPlaylist, ThisFolder}) {
        const auto *record = find(scope, owner(scope, url, playlistId));
        if (record && record->files.contains(key)) return record->files.value(key);
    }
    return {};
}
bool SubtitleAttachmentModel::scan(const QUrl &folder, bool subtitles, QList<Row> *media, QStringList *files)
{
    const QDir directory(folder.toLocalFile());
    if (localUrl(folder).isEmpty() || !directory.exists()) { fail(qtr("This folder is unavailable. Choose another folder.")); return false; }
    QDirIterator it(directory.absolutePath(), QDir::Files | QDir::Readable | QDir::NoDotAndDotDot);
    int entries = 0;
    while (it.hasNext()) {
        it.next();
        if (++entries > MaxEntries) { fail(qtr("This folder is too large to review at once. Use This File or a smaller folder.")); return false; }
        const auto info = it.fileInfo(); const auto url = localUrl(QUrl::fromLocalFile(info.absoluteFilePath()));
        if (subtitles && SubtitleExtensions.contains(info.suffix().toLower())) files->append(url.toString(QUrl::FullyEncoded));
        else if (!subtitles && MediaExtensions.contains(info.suffix().toLower())) media->append({url, info.fileName(), identity(url), {}});
        if ((media && media->size() > MaxRows) || (files && files->size() > MaxRows)) {
            fail(qtr("Too many files to review at once. Choose a smaller folder.")); return false;
        }
    }
    if (media) std::sort(media->begin(), media->end(), [](const Row &a, const Row &b) { return a.title.localeAwareCompare(b.title) < 0; });
    if (files) std::sort(files->begin(), files->end());
    return true;
}
bool SubtitleAttachmentModel::begin()
{
    cancel(); m_error = m_storageValid ? m_loadError : qtr("Saved subtitle attachments are unavailable. Existing data has not been overwritten.");
    m_status.clear(); m_draftContext = context(true);
    m_mediaUrl = localUrl(itemUrl(m_draftContext.item.get()));
    if (m_mediaUrl.isEmpty()) { fail(qtr("Open a local media file to save subtitle attachments.")); return false; }
    bool sameMedia = true;
    if (m_player) { AttachmentPlayerLock lock{m_player}; sameMedia = vlc_player_GetCurrentMedia(m_player) == m_draftContext.item.get(); }
    if (!sameMedia) { fail(qtr("The media changed. Open Add Subtitles again.")); return false; }
    m_active = true; m_stale = false; m_draftGeneration = m_generation;
    m_scope = ThisFile; m_sourceFolder = QUrl::fromLocalFile(QFileInfo(m_mediaUrl.toLocalFile()).absolutePath());
    rebuildRows(); return true;
}
void SubtitleAttachmentModel::cancel()
{
    m_active = false; m_stale = false; m_draftContext = {}; m_mediaUrl = QUrl{}; m_rows.clear(); m_subtitles.clear();
    emit changed();
}
void SubtitleAttachmentModel::rebuildRows()
{
    m_rows.clear(); m_subtitles.clear(); m_scanValid = true;
    if (m_scope == ThisFile) m_rows.append({m_mediaUrl, QFileInfo(m_mediaUrl.toLocalFile()).fileName(), identity(m_mediaUrl), {}});
    else if (m_scope == ThisFolder) m_scanValid = scan(QUrl::fromLocalFile(QFileInfo(m_mediaUrl.toLocalFile()).absolutePath()), false, &m_rows, nullptr);
    else {
        QSet<QString> seen;
        for (const auto &value : m_draftContext.playlistItems) {
            const auto map = value.toMap(); const auto url = localUrl(map.value("url").toUrl()); const auto key = identity(url);
            if (key.isEmpty() || seen.contains(key)) continue;
            seen.insert(key); m_rows.append({url, map.value("title", QFileInfo(url.toLocalFile()).fileName()).toString(), key, {}});
            if (m_rows.size() > MaxRows) { m_scanValid = false; fail(qtr("This playlist is too large to review at once.")); break; }
        }
    }
    QStringList scanned;
    m_scanValid = scan(m_sourceFolder, true, nullptr, &scanned) && m_scanValid;
    m_subtitles = scanned;
    const auto *saved = find(m_scope, owner(m_scope, m_mediaUrl, m_draftContext.playlistId));
    if (saved) for (auto &row : m_rows) {
        row.subtitles = saved->files.value(row.key);
        for (const auto &url : row.subtitles) if (!m_subtitles.contains(url)) m_subtitles.append(url);
    }
    emit changed();
}
void SubtitleAttachmentModel::setScope(int scope)
{
    if (!editable() || scope < ThisFile || scope > ThisPlaylist || scope == m_scope
        || (scope == ThisFolder && !folderAvailable()) || (scope == ThisPlaylist && !playlistAvailable())) return;
    m_error.clear(); m_scope = scope; rebuildRows();
}
void SubtitleAttachmentModel::setSourceFolder(const QUrl &folder)
{
    if (!editable()) return;
    QStringList files;
    if (!scan(folder, true, nullptr, &files)) return;
    m_sourceFolder = localUrl(folder); m_subtitles = files; m_error.clear();
    // Keep deliberate mappings visible even when browsing a second source folder.
    for (const auto &row : m_rows) for (const auto &url : row.subtitles)
        if (!m_subtitles.contains(url)) m_subtitles.append(url);
    emit changed();
}
void SubtitleAttachmentModel::addFiles(const QVariantList &files)
{
    if (!editable() || files.isEmpty()) return;
    auto added = m_subtitles;
    for (const auto &value : files) {
        const auto url = localUrl(value.toUrl());
        if (!subtitleFile(url, true)) { fail(qtr("Choose an available subtitle file.")); return; }
        const auto encoded = url.toString(QUrl::FullyEncoded);
        if (!added.contains(encoded)) added.append(encoded);
    }
    if (added.size() > MaxRows) { fail(qtr("Too many subtitle files to review at once.")); return; }
    m_subtitles = std::move(added); m_error.clear(); emit changed();
}
QVariantList SubtitleAttachmentModel::mediaRows() const
{
    QVariantList result;
    for (const auto &row : m_rows) {
        QStringList names; for (const auto &url : row.subtitles) names.append(QFileInfo(QUrl(url).toLocalFile()).fileName());
        result.append(QVariantMap{{"url",row.url},{"title",row.title},{"subtitles",names},{"count",row.subtitles.size()}});
    }
    return result;
}
QVariantList SubtitleAttachmentModel::subtitleRows() const
{
    QVariantList result;
    for (const auto &url : m_subtitles) {
        const QUrl parsed(url); const QFileInfo info(parsed.toLocalFile());
        QVariantList selectedRows;
        for (qsizetype i = 0; i < m_rows.size(); ++i) if (m_rows[i].subtitles.contains(url)) selectedRows.append(i);
        result.append(QVariantMap{{"url",parsed},{"title",info.fileName()},{"available",info.isFile() && info.isReadable()},{"selectedRows",selectedRows}});
    }
    return result;
}
void SubtitleAttachmentModel::toggle(int mediaRow, int subtitleRow)
{
    if (!editable() || mediaRow < 0 || mediaRow >= m_rows.size() || subtitleRow < 0 || subtitleRow >= m_subtitles.size()) return;
    auto &selected = m_rows[mediaRow].subtitles; const auto &url = m_subtitles[subtitleRow];
    if (selected.contains(url)) selected.removeAll(url);
    else {
        if (!subtitleFile(QUrl(url), true)) { fail(qtr("This subtitle file is missing. Choose it again after moving or restoring it.")); return; }
        if (selected.size() >= MaxFiles) { fail(qtr("Choose at most eight subtitle files for each media file.")); return; }
        selected.append(url);
    }
    m_error.clear(); emit changed();
}
void SubtitleAttachmentModel::clearRow(int row)
{
    if (!editable() || row < 0 || row >= m_rows.size()) return;
    m_rows[row].subtitles.clear(); m_error.clear(); emit changed();
}
void SubtitleAttachmentModel::matchNames()
{
    if (!editable() || !m_scanValid) return;
    // Only unambiguous, one-to-one filename/episode matches are proposed.
    // Existing manual mappings are never rearranged by a suggestion action.
    QMap<QString, QList<int>> candidates; QMap<QString, int> counts;
    QSet<QString> used;
    for (const auto &row : m_rows) for (const auto &url : row.subtitles) used.insert(url);
    for (qsizetype row = 0; row < m_rows.size(); ++row) {
        if (!m_rows[row].subtitles.isEmpty()) continue;
        const auto base = stem(m_rows[row].url); const auto ep = episode(base);
        for (const auto &encoded : m_subtitles) {
            const QUrl url(encoded); const auto sub = stem(url);
            if (!used.contains(encoded) && subtitleFile(url, true)
                && (sub == base || sub.startsWith(base + '.') || (!ep.isEmpty() && episode(sub) == ep)))
                candidates[encoded].append(int(row));
        }
    }
    for (auto it = candidates.begin(); it != candidates.end(); ++it)
        if (it.value().size() == 1) ++counts[QString::number(it.value().first())];
    for (auto it = candidates.begin(); it != candidates.end(); ++it)
        if (it.value().size() == 1 && counts.value(QString::number(it.value().first())) == 1)
            m_rows[it.value().first()].subtitles.append(it.key());
    m_error.clear(); emit changed();
}
bool SubtitleAttachmentModel::canSave() const
{
    if (!m_active || m_stale || !m_storageValid || !m_scanValid || m_rows.isEmpty()) return false;
    for (const auto &row : m_rows) if (!row.subtitles.isEmpty()) return true;
    return find(m_scope, owner(m_scope, m_mediaUrl, m_draftContext.playlistId));
}
bool SubtitleAttachmentModel::persist(const QList<Record> &records)
{
    if (!m_settings || !m_storageValid || records.size() > MaxRecords) { fail(qtr("Subtitle attachment storage is unavailable or full.")); return false; }
    QJsonArray entries;
    for (const auto &record : records) {
        if (record.files.size() > MaxRows) { fail(qtr("Too many saved matches for this scope. Use a smaller playlist or save individual file choices.")); return false; }
        QJsonObject files;
        for (auto it = record.files.begin(); it != record.files.end(); ++it) files.insert(it.key(), QJsonArray::fromStringList(it.value()));
        entries.append(QJsonObject{{"scope",record.scope},{"owner",record.owner},{"files",files}});
    }
    const auto bytes = QJsonDocument(QJsonObject{{"schema",1},{"records",entries}}).toJson(QJsonDocument::Compact);
    if (bytes.size() > MaxBytes) { fail(qtr("Saved subtitle attachments are too large.")); return false; }
    QString error;
    if (!CheckedPreferencesStore(nullptr).saveSettings(m_settings, SettingsKey, bytes, false, error)) {
        fail(qtr("The attachments could not be saved. Check access to your settings folder, then try again.")); return false;
    }
    return true;
}
bool SubtitleAttachmentModel::save()
{
    if (!canSave()) return false;
    const auto current = context(true);
    if (current.item.get() != m_draftContext.item.get() || current.playlistId != m_draftContext.playlistId
        || m_draftGeneration != m_generation.load()) { m_stale = true; fail(qtr("The media changed. Open Add Subtitles again.")); return false; }
    QSet<QString> playlistMembers;
    if (m_scope == ThisPlaylist) for (const auto &entry : current.playlistItems)
        playlistMembers.insert(identity(entry.toMap().value("url").toUrl()));
    Record replacement{m_scope, owner(m_scope, m_mediaUrl, m_draftContext.playlistId), {}};
    // The queue may contain only part of a saved playlist. Preserve its unseen
    // matches; clearing a visible row removes only that row's explicit choice.
    if (m_scope == ThisPlaylist) if (const auto *existing = find(m_scope, replacement.owner))
        replacement.files = existing->files;
    for (const auto &row : m_rows) {
        if (row.subtitles.isEmpty()) { replacement.files.remove(row.key); continue; }
        const QFileInfo media(row.url.toLocalFile());
        if (!media.isFile() || !media.isReadable() || (m_scope == ThisPlaylist && !playlistMembers.contains(row.key))) {
            fail(qtr("A media file moved, disappeared or left this playlist. Reopen Add Subtitles to review the matches.")); return false;
        }
        for (const auto &file : row.subtitles) if (!subtitleFile(QUrl(file), true)) {
            fail(qtr("A selected subtitle file is missing. Restore it or choose another file, then try again.")); return false;
        }
        replacement.files.insert(row.key, row.subtitles);
    }
    auto records = m_records;
    records.erase(std::remove_if(records.begin(), records.end(), [&](const Record &r) {
        return r.scope == replacement.scope && r.owner == replacement.owner;
    }), records.end());
    if (!replacement.files.isEmpty()) records.append(std::move(replacement));
    if (!persist(records)) return false;
    m_records = std::move(records); m_error.clear(); m_loadError.clear(); m_appliedGeneration = UINT64_MAX;
    m_explicitDefaultGeneration = m_generation.load();
    m_status = qtr("Subtitle matches saved. The first subtitle is the default; other attached subtitles stay available in the track list.");
    cancel(); refresh(); return true;
}
void SubtitleAttachmentModel::refresh()
{
    const auto generation = m_generation.load();
    if (m_loadGeneration != generation) {
        m_loadGeneration = generation; ++m_loadSerial; m_pendingLoads.clear();
        clearLoadFailure();
    }
    if (m_active && generation != m_draftGeneration && !m_stale) {
        m_stale = true; fail(qtr("The media changed. Close and reopen Add Subtitles to review its files."));
    }
    if (!m_player || !m_storageValid) return;
    const auto current = context(false);
    // Evaluate the Qt service policy for the held media outside the native
    // playback lock. The other service's queued cache may still describe the
    // previous input. Explicit Save chooses now; automatic restore defers to
    // this media's saved preset while loading every associated file.
    const bool chooseDefault = m_explicitDefaultGeneration == generation
        || !m_defaultSelection || m_defaultSelection(current.item.get());
    if (!chooseDefault) m_pendingDefault.clear();
    SharedInputItem media;
    bool alreadyApplied = false;
    bool confirmedPending = false;
    {
        AttachmentPlayerLock lock{m_player};
        auto *item = vlc_player_GetCurrentMedia(m_player);
        const auto state = vlc_player_GetState(m_player);
        if (!item || current.item.get() != item
            || (state != VLC_PLAYER_STATE_PLAYING && state != VLC_PLAYER_STATE_PAUSED)) return;
        if (m_appliedGeneration == generation && m_appliedPlaylist == current.playlistId) {
            if (chooseDefault) selectDefault();
            const bool waiting = !m_pendingLoads.isEmpty();
            confirmLoads(); confirmedPending = waiting && m_pendingLoads.isEmpty();
            alreadyApplied = true;
        } else {
            media.reset(item);
        }
    }
    if (alreadyApplied) {
        if (confirmedPending) clearLoadFailure();
        return;
    }
    // Filesystem checks can stall on a removable or network-backed folder.
    // They never hold the native playback lock; revalidate before submitting.
    const auto files = effectiveFiles(itemUrl(media.get()), current.playlistId);
    bool available = true;
    for (const auto &file : files) if (!subtitleFile(QUrl(file), true)) { available = false; break; }
    const auto serial = ++m_loadSerial;
    m_pendingLoads.clear();
    bool submissionFailed = false;
    {
        AttachmentPlayerLock lock{m_player};
        const auto state = vlc_player_GetState(m_player);
        if (generation != m_generation.load() || vlc_player_GetCurrentMedia(m_player) != media.get()
            || (state != VLC_PLAYER_STATE_PLAYING && state != VLC_PLAYER_STATE_PAUSED)) return;
        if (m_submittedGeneration != generation) { m_submitted.clear(); m_submittedGeneration = generation; }
        m_pendingDefault.clear();
        m_appliedGeneration = generation; m_appliedPlaylist = current.playlistId;
        if (!available) {
            // No fallback changes the user's explicit higher-priority choice.
            QMetaObject::invokeMethod(this, [this, generation, serial] {
                if (generation == m_generation.load() && serial == m_loadSerial)
                    failLoad(qtr("A saved subtitle file is missing. Open Add Subtitles to choose its new location."));
            }, Qt::QueuedConnection);
            return;
        }
        auto *item = media.get();
        QMap<QString, QByteArray> existing;
        {
            vlc_mutex_locker itemLock(&item->lock);
            for (int i = 0; i < item->i_slaves; ++i)
                if (item->pp_slaves[i]->i_type == SLAVE_TYPE_SPU)
                    existing.insert(identity(QUrl::fromEncoded(QByteArray(item->pp_slaves[i]->psz_uri))), QByteArray(item->pp_slaves[i]->psz_uri));
        }
        if (chooseDefault && !files.isEmpty()) {
            const QUrl first(files.first());
            const auto bytes = existing.value(identity(first), first.toEncoded());
            m_pendingDefault = QCryptographicHash::hash(bytes, QCryptographicHash::Md5).toHex() + '/';
        }
        for (const auto &file : files) {
            const QUrl url(file);
            const auto bytes = existing.value(identity(url), url.toEncoded());
            m_pendingLoads.insert(QCryptographicHash::hash(bytes, QCryptographicHash::Md5).toHex() + '/');
        }
        for (qsizetype i = 0; i < files.size(); ++i) {
            const QUrl url(files[i]); const auto key = identity(url);
            if (existing.contains(key) || m_submitted.contains(key)) continue;
            const auto bytes = url.toEncoded();
            if (vlc_player_AddAssociatedMedia(m_player, SPU_ES, bytes.constData(), chooseDefault && i == 0, false, true) != VLC_SUCCESS) {
                submissionFailed = true;
                QMetaObject::invokeMethod(this, [this, generation, serial] {
                    if (generation == m_generation.load() && serial == m_loadSerial)
                        failLoad(qtr("VLC could not load the saved subtitles. Reopen Add Subtitles and choose another file."));
                }, Qt::QueuedConnection);
                break;
            }
            m_submitted.insert(key);
        }
        selectDefault();
        confirmLoads();
    }
    if (submissionFailed) return;
    if (m_pendingLoads.isEmpty()) {
        clearLoadFailure();
        return;
    }
    // Associated-media submission is asynchronous. Confirm actual native ES
    // appearance once, without repeatedly adding the same file or advancing
    // playback. A newer mapping/input invalidates this observation.
    QTimer::singleShot(8000, this, [this, generation, serial] {
        if (generation != m_generation.load() || serial != m_loadSerial) return;
        refresh();
        if (generation != m_generation.load() || serial != m_loadSerial || m_pendingLoads.isEmpty()) return;
        {
            AttachmentPlayerLock lock{m_player};
            const auto state = vlc_player_GetState(m_player);
            if (state != VLC_PLAYER_STATE_PLAYING && state != VLC_PLAYER_STATE_PAUSED) return;
        }
        failLoad(qtr("Some subtitle files did not load. Choose another file, or reopen the media after replacing them."));
    });
}
void SubtitleAttachmentModel::confirmLoads()
{
    for (size_t i = 0; i < vlc_player_GetTrackCount(m_player, SPU_ES); ++i) {
        const auto *track = vlc_player_GetTrackAt(m_player, SPU_ES, i);
        const QByteArray id(vlc_es_id_GetStrId(track->es_id));
        for (auto it = m_pendingLoads.begin(); it != m_pendingLoads.end();) {
            if (id.startsWith(*it)) it = m_pendingLoads.erase(it);
            else ++it;
        }
    }
}
void SubtitleAttachmentModel::selectDefault()
{
    if (m_pendingDefault.isEmpty()) return;
    for (size_t i = 0; i < vlc_player_GetTrackCount(m_player, SPU_ES); ++i) {
        const auto *track = vlc_player_GetTrackAt(m_player, SPU_ES, i);
        if (!QByteArray(vlc_es_id_GetStrId(track->es_id)).startsWith(m_pendingDefault)) continue;
        if (!track->selected) vlc_player_SelectTrack(m_player, track, VLC_PLAYER_SELECT_EXCLUSIVE);
        m_pendingDefault.clear(); return;
    }
}
