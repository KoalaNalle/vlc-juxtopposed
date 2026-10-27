// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "local_music_folder.hpp"
#include <vlc_interface.h>
#include <vlc_playlist.h>
#include <vlc_preparser.h>
#include <QCollator>
#include <QDirIterator>
#include <QFileInfo>
#include <QSemaphore>
#include <algorithm>
#include <vector>

namespace {
QString localDirectory(const QUrl &url, bool sourceFile)
{
    const auto path = url.toLocalFile();
    const QFileInfo info(path);
    if (!url.isLocalFile() || !url.host().isEmpty() || !url.userInfo().isEmpty()
        || url.hasQuery() || url.hasFragment() || !info.isAbsolute()
        || path.startsWith("//") || path.startsWith("\\\\") || !info.isReadable()
        || (sourceFile ? !info.isFile() : !info.isDir())) return {};
    return QDir::cleanPath(sourceFile ? info.absolutePath() : info.absoluteFilePath());
}
QString meta(input_item_t *item, vlc_meta_type_t type)
{
    char *value = input_item_GetMeta(item, type);
    const auto result = QString::fromUtf8(value ? value : "");
    free(value);
    return result;
}
struct ParseState { QSemaphore ended; int status = VLC_EGENERIC; };
const vlc_preparser_cbs Callbacks = [] {
    vlc_preparser_cbs cbs{};
    cbs.on_ended = [](vlc_preparser_req *, int status, void *data) {
        auto *state = static_cast<ParseState *>(data);
        state->status = status;
        state->ended.release();
    };
    return cbs;
}();
}

LocalMusicFolderModel::LocalMusicFolderModel(vlc_object_t *parent,
    vlc_playlist_t *playlist, QObject *objectParent)
    : QAbstractListModel(objectParent), m_parent(parent), m_playlist(playlist)
{
    m_pool.setMaxThreadCount(1);
}
LocalMusicFolderModel::~LocalMusicFolderModel()
{
    if (m_cancel) *m_cancel = true;
    m_pool.clear();
    m_pool.waitForDone(); // Native parent and callback contexts remain valid.
}
QHash<int, QByteArray> LocalMusicFolderModel::roleNames() const
{
    return {{TitleRole, "title"}, {ArtistRole, "artist"},
            {ArtworkRole, "artwork"}, {UrlRole, "url"}};
}
int LocalMusicFolderModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_view.size();
}
QVariant LocalMusicFolderModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_view.size()) return {};
    const auto &entry = m_entries[m_view[index.row()]];
    switch (role) {
    case TitleRole: return entry.title;
    case ArtistRole: return entry.artist;
    case ArtworkRole: return entry.artwork;
    case UrlRole: return entry.url;
    default: return {};
    }
}
QString LocalMusicFolderModel::name() const
{
    const auto basename = QFileInfo(m_folder).fileName();
    return basename.isEmpty() ? QDir::toNativeSeparators(m_folder) : basename;
}
int LocalMusicFolderModel::currentIndex() const
{
    for (int i = 0; i < m_view.size(); ++i)
        if (m_entries[m_view[i]].url == m_source) return i;
    return -1;
}
QVariantList LocalMusicFolderModel::breadcrumbs() const
{
    QVariantList result;
    for (QString path = m_folder; !path.isEmpty();)
    {
        const QFileInfo info(path);
        const auto title = info.fileName().isEmpty() ? QDir::toNativeSeparators(path) : info.fileName();
        result.prepend(QVariantMap{{QStringLiteral("text"), title},
                                   {QStringLiteral("uri"), QUrl::fromLocalFile(path)}});
        const auto parent = QDir::cleanPath(info.dir().absolutePath());
        if (parent == path) break;
        path = parent;
    }
    return result;
}
void LocalMusicFolderModel::setSourceUrl(const QUrl &source)
{
    if (m_source == source) return;
    m_source = source;
    // A track change within the same source keeps the user's browsing position.
    const auto sourceFolder = localDirectory(source, true);
    if (m_enabled && !sourceFolder.isEmpty() && sourceFolder == m_sourceFolder)
        emit stateChanged();
    else restart();
}
void LocalMusicFolderModel::setEnabled(bool enabled)
{
    if (m_enabled == enabled) return;
    m_enabled = enabled;
    restart();
}
void LocalMusicFolderModel::setSearchPattern(const QString &search)
{
    if (m_search == search) return;
    m_search = search;
    rebuildView();
}
void LocalMusicFolderModel::setSortAscending(bool ascending)
{
    if (m_ascending == ascending) return;
    m_ascending = ascending;
    rebuildView();
}
void LocalMusicFolderModel::rebuildView()
{
    beginResetModel();
    filterView();
    endResetModel();
    emit stateChanged();
}
void LocalMusicFolderModel::replaceEntries(QVector<Entry> entries)
{
    beginResetModel();
    m_entries = std::move(entries);
    filterView();
    endResetModel();
    emit stateChanged();
}
void LocalMusicFolderModel::filterView()
{
    m_view.clear();
    for (int i = 0; i < m_entries.size(); ++i)
        if (m_search.isEmpty() || m_entries[i].title.contains(m_search, Qt::CaseInsensitive)
            || m_entries[i].artist.contains(m_search, Qt::CaseInsensitive)) m_view.append(i);
    QCollator collator;
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    collator.setNumericMode(true);
    std::stable_sort(m_view.begin(), m_view.end(), [&](int a, int b) {
        const int order = collator.compare(m_entries[a].title, m_entries[b].title);
        return m_ascending ? order < 0 : order > 0;
    });
}
bool LocalMusicFolderModel::visitFolder(const QUrl &url, int historyIndex)
{
    const auto folder = localDirectory(url, false);
    if (!m_enabled || m_sourceFolder.isEmpty() || folder.isEmpty())
    {
        m_navigationError = tr("The folder is unavailable or cannot be read.");
        emit stateChanged();
        return false;
    }
    m_navigationError.clear();
    if (folder == m_folder) { emit stateChanged(); return true; }
    if (historyIndex >= 0) m_historyIndex = historyIndex;
    else
    {
        m_history.erase(m_history.begin() + m_historyIndex + 1, m_history.end());
        m_history.append(folder);
        // Browsing history is bounded per player and is never written to settings.
        if (m_history.size() > 64) m_history.removeFirst();
        m_historyIndex = m_history.size() - 1;
    }
    loadFolder(folder);
    return true;
}
bool LocalMusicFolderModel::browseFolder(const QUrl &url) { return visitFolder(url); }
bool LocalMusicFolderModel::browseSourceFolder()
{
    return visitFolder(QUrl::fromLocalFile(m_sourceFolder));
}
bool LocalMusicFolderModel::goBack()
{
    return canGoBack() && visitFolder(QUrl::fromLocalFile(m_history[m_historyIndex - 1]), m_historyIndex - 1);
}
bool LocalMusicFolderModel::goForward()
{
    return canGoForward() && visitFolder(QUrl::fromLocalFile(m_history[m_historyIndex + 1]), m_historyIndex + 1);
}
void LocalMusicFolderModel::refresh()
{
    if (!m_enabled || m_sourceFolder.isEmpty() || localDirectory(m_source, true) != m_sourceFolder)
    {
        restart();
        return;
    }
    const auto folder = localDirectory(QUrl::fromLocalFile(m_folder), false);
    if (folder.isEmpty())
    {
        m_navigationError = tr("The folder is unavailable or cannot be read.");
        emit stateChanged();
        return;
    }
    loadFolder(folder);
}
void LocalMusicFolderModel::clearFolder()
{
    ++m_generation;
    if (m_cancel) *m_cancel = true;
    m_pool.clear();
    m_folder.clear();
    m_sourceFolder.clear();
    m_history.clear();
    m_historyIndex = -1;
    m_error.clear();
    m_navigationError.clear();
    m_busy = false;
    replaceEntries({});
}
void LocalMusicFolderModel::restart()
{
    const auto sourceFolder = m_enabled ? localDirectory(m_source, true) : QString{};
    if (sourceFolder.isEmpty())
    {
        clearFolder();
        if (m_enabled && !m_source.isEmpty())
        {
            m_error = tr("A readable local audio file is needed to browse its source folder.");
            emit stateChanged();
        }
        return;
    }
    m_sourceFolder = sourceFolder;
    visitFolder(QUrl::fromLocalFile(sourceFolder));
}
void LocalMusicFolderModel::loadFolder(const QString &folder)
{
    const auto generation = ++m_generation;
    if (m_cancel) *m_cancel = true;
    m_pool.clear();
    m_folder = folder;
    m_error.clear();
    m_navigationError.clear();
    m_busy = true;
    replaceEntries({});
    m_cancel = std::make_shared<std::atomic_bool>(false);
    const auto cancel = m_cancel;
    emit stateChanged();
    m_pool.start([this, folder, generation, cancel] { scan(folder, generation, cancel); });
}
void LocalMusicFolderModel::scan(const QString &folder, quint64 generation,
                               const Cancellation &cancel)
{
    const QStringList extensions = { EXTENSIONS_AUDIO_CSV };
    QVector<Entry> entries;
    QDirIterator files(folder, QDir::Files | QDir::Readable | QDir::NoDotAndDotDot);
    QString error;
    while (files.hasNext() && !*cancel)
    {
        files.next();
        const auto info = files.fileInfo();
        if (!extensions.contains(info.suffix(), Qt::CaseInsensitive)) continue;
        if (entries.size() >= 4096)
        {
            error = tr("This folder has more than 4096 audio files; showing the first 4096.");
            break;
        }
        const QUrl url = QUrl::fromLocalFile(info.absoluteFilePath());
        const auto encoded = url.toEncoded();
        const auto filename = info.fileName().toUtf8();
        SharedInputItem item(input_item_New(encoded.constData(), filename.constData()), false);
        if (!item) { error = tr("Some files could not be loaded."); continue; }
        entries.append({url, info.fileName(), {}, {}, item});
    }
    if (*cancel) return;
    // Publish genuine filenames immediately; parsed title/artist/artwork arrive later.
    QMetaObject::invokeMethod(this, [this, generation, entries, error] {
        if (generation != m_generation) return;
        m_error = error;
        replaceEntries(entries);
    }, Qt::QueuedConnection);
    vlc_preparser_cfg cfg{};
    // A fetch stage can download an artwork URL even with FETCHMETA_LOCAL, and
    // the native fetcher is not cancellable per request. Keep discovery bounded
    // to parsing; the normal Player supplies its own current-track artwork.
    cfg.types = VLC_PREPARSER_TYPE_PARSE;
    cfg.max_parser_threads = 1;
    cfg.timeout = VLC_TICK_FROM_MS(1500);
    cfg.external_process = false;
    auto *parser = vlc_preparser_New(m_parent, &cfg);
    std::vector<std::unique_ptr<ParseState>> contexts;
    int metadataFailures = 0;
    if (parser)
    {
        for (int i = 0; i < entries.size() && !*cancel; ++i)
        {
            auto &entry = entries[i];
            contexts.push_back(std::make_unique<ParseState>());
            auto *state = contexts.back().get();
            auto *request = vlc_preparser_req_NewParse(parser, entry.item.get(), cfg.types, &Callbacks, state);
            if (request && vlc_preparser_Submit(parser, request) == VLC_SUCCESS)
            {
                while (!state->ended.tryAcquire(1, 20))
                    if (*cancel) { vlc_preparser_Cancel(parser, request); break; }
                if (!*cancel)
                {
                    if (state->status != VLC_SUCCESS) ++metadataFailures;
                    const auto title = meta(entry.item.get(), vlc_meta_Title);
                    if (!title.isEmpty()) entry.title = title;
                    entry.artist = meta(entry.item.get(), vlc_meta_Artist);
                    const QUrl artwork(meta(entry.item.get(), vlc_meta_ArtworkURL));
                    const auto artworkPath = artwork.toLocalFile();
                    if (artwork.isLocalFile() && artwork.host().isEmpty()
                        && !artworkPath.startsWith("//") && !artworkPath.startsWith("\\\\")
                        && QFileInfo(artworkPath).isFile() && QFileInfo(artworkPath).isReadable())
                        entry.artwork = artwork;
                    // Unresolved attachments and remote covers stay unavailable
                    // rather than starting extra downloads from the folder list.
                    QMetaObject::invokeMethod(this, [this, generation, i, entry] {
                        if (generation != m_generation || i >= m_entries.size()) return;
                        beginResetModel();
                        m_entries[i] = entry;
                        filterView();
                        endResetModel();
                        emit stateChanged();
                    }, Qt::QueuedConnection);
                }
            }
            else ++metadataFailures;
            if (request) vlc_preparser_req_Release(request);
        }
        // Joins all callbacks before destroying their contexts, including cancellation.
        vlc_preparser_Delete(parser);
    }
    else if (error.isEmpty()) error = tr("Metadata could not be read; file names are shown.");
    if (*cancel) return;
    if (metadataFailures > 0)
    {
        if (!error.isEmpty()) error += '\n';
        error += tr("Metadata could not be fully read for %n audio file(s); available tags and file names are shown.", nullptr, metadataFailures);
    }
    QMetaObject::invokeMethod(this, [this, generation, error] {
        if (generation != m_generation) return;
        m_busy = false;
        m_error = error;
        emit stateChanged();
    }, Qt::QueuedConnection);
}
bool LocalMusicFolderModel::addAndPlay(int row)
{
    if (!m_playlist || row < 0 || row >= m_view.size()) return false;
    m_navigationError.clear();
    const auto entry = m_entries[m_view[row]];
    const QFileInfo file(entry.url.toLocalFile());
    if (!file.isFile() || !file.isReadable())
    {
        m_error = tr("The selected file is no longer readable.");
        emit stateChanged();
        return false;
    }
    int result = VLC_EGENERIC;
    vlc_playlist_Lock(m_playlist);
    size_t target = vlc_playlist_Count(m_playlist);
    for (size_t i = 0; i < target; ++i)
    {
        char *uri = input_item_GetURI(vlc_playlist_item_GetMedia(vlc_playlist_Get(m_playlist, i)));
        const bool match = uri && QUrl::fromEncoded(uri) == entry.url;
        free(uri);
        if (match) { target = i; break; }
    }
    if (target == vlc_playlist_Count(m_playlist))
    {
        // Copy real metadata without carrying playback options from another source.
        input_item_t *item = input_item_Copy(entry.item.get());
        if (item)
        {
            result = vlc_playlist_Append(m_playlist, &item, 1);
            input_item_Release(item);
        }
    }
    else result = VLC_SUCCESS;
    if (result == VLC_SUCCESS) result = vlc_playlist_GoTo(m_playlist, target);
    if (result == VLC_SUCCESS) result = vlc_playlist_Start(m_playlist);
    vlc_playlist_Unlock(m_playlist);
    m_error = result == VLC_SUCCESS ? QString{} : tr("Could not start the selected audio file. Check the play queue.");
    emit stateChanged();
    return result == VLC_SUCCESS;
}
