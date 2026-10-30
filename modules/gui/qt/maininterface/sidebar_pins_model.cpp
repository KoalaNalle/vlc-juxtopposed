// SPDX-License-Identifier: GPL-2.0-or-later
#include "sidebar_pins_model.hpp"
#include "medialibrary/medialib.hpp"
#include "medialibrary/mlhelper.hpp"
#include "network/networktreeitem.hpp"
#include <QApplication>
#include <QFile>
#include <utility>
#include <QCryptographicHash>
#include <QDateTime>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QWindow>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#endif

namespace {
bool collection(const SidebarPin &pin)
{ return pin.kind == QStringLiteral("playlist") || pin.kind == QStringLiteral("artist"); }
QString databaseScope(const QString &path)
{
    // Database-local IDs must never resolve against a replacement database.
    // The file identity survives normal writes/restarts; contents/size do not.
    QByteArray identity;
#ifdef _WIN32
    const auto handle = CreateFileW(path.toStdWString().c_str(), FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, 0, nullptr);
    if (handle == INVALID_HANDLE_VALUE) return {};
    BY_HANDLE_FILE_INFORMATION info{};
    const bool ok = GetFileInformationByHandle(handle, &info); CloseHandle(handle);
    if (!ok) return {};
    for (const auto value : {info.dwVolumeSerialNumber, info.nFileIndexHigh, info.nFileIndexLow,
                            info.ftCreationTime.dwHighDateTime, info.ftCreationTime.dwLowDateTime})
        identity += QByteArray::number(quint64(value)) + '/';
#else
    struct stat info{};
    const auto encoded = QFile::encodeName(path);
    if (stat(encoded.constData(), &info)) return {};
    identity = QByteArray::number(quint64(info.st_dev)) + '/' + QByteArray::number(quint64(info.st_ino));
    const auto birth = QFileInfo(path).birthTime();
    if (birth.isValid()) identity += '/' + QByteArray::number(birth.toMSecsSinceEpoch());
#endif
    return QString::fromLatin1(QCryptographicHash::hash(identity, QCryptographicHash::Sha256).toHex());
}
QUrl address(const char *text) { return text ? QUrl::fromEncoded(text) : QUrl(); }
QString title(const char *text) { return text ? QString::fromUtf8(text) : QString(); }
SidebarPin describe(vlc_medialibrary_t *ml, MLItemId id, const QString &scope)
{
    SidebarPin pin;
    if (id.type == VLC_ML_PARENT_UNKNOWN)
    {
        ml_unique_ptr<vlc_ml_media_t> item(vlc_ml_get_media(ml, id.id));
        if (!item || !item->p_files) return {};
        for (const auto &file : ml_range_iterate<vlc_ml_file_t>(item->p_files))
            if (file.i_type == VLC_ML_FILE_TYPE_MAIN && file.psz_mrl)
            { pin.mrl = address(file.psz_mrl); break; }
        pin.kind = QStringLiteral("media"); pin.title = title(item->psz_title);
        pin.artwork = address(item->thumbnails[VLC_ML_THUMBNAIL_SMALL].psz_mrl);
    }
    else if (id.type == VLC_ML_PARENT_FOLDER)
    {
        ml_unique_ptr<vlc_ml_folder_t> item(vlc_ml_get_folder(ml, id.id));
        if (!item || !item->psz_mrl) return {};
        pin.kind = QStringLiteral("folder"); pin.title = title(item->psz_name); pin.mrl = address(item->psz_mrl);
    }
    else if (id.type == VLC_ML_PARENT_PLAYLIST)
    {
        ml_unique_ptr<vlc_ml_playlist_t> item(vlc_ml_get_playlist(ml, id.id));
        if (!item) return {};
        pin.kind = QStringLiteral("playlist"); pin.title = title(item->psz_name);
        pin.artwork = address(item->psz_artwork_mrl); pin.mrl = address(item->psz_mrl);
    }
    else if (id.type == VLC_ML_PARENT_ARTIST)
    {
        ml_unique_ptr<vlc_ml_artist_t> item(vlc_ml_get_artist(ml, id.id));
        if (!item) return {};
        pin.kind = QStringLiteral("artist"); pin.title = title(item->psz_name);
        pin.artwork = address(item->thumbnails[VLC_ML_THUMBNAIL_SMALL].psz_mrl);
    }
    if (collection(pin)) { pin.nativeScope = scope; pin.nativeId = QString::number(id.id); }
    QString error;
    if (!SidebarPin::normalize(pin, error)) return {};
    return pin;
}
MLItemId nativeId(const SidebarPin &pin)
{
    return {pin.nativeId.toLongLong(), pin.kind == QStringLiteral("playlist")
        ? VLC_ML_PARENT_PLAYLIST : VLC_ML_PARENT_ARTIST};
}
bool targetAvailable(vlc_medialibrary_t *ml, const SidebarPin &pin, const QString &scope)
{
    if (!collection(pin)) return !pin.mrl.isLocalFile() || QFileInfo::exists(pin.mrl.toLocalFile());
    if (!ml || scope.isEmpty() || pin.nativeScope != scope) return false;
    return !describe(ml, nativeId(pin), scope).kind.isEmpty();
}
}

SidebarPinsModel::SidebarPinsModel(MediaLib *library, QSettings *settings, QString databaseFile, QObject *parent)
    : QAbstractListModel(parent), m_library(library), m_store(settings),
      m_databaseFile(std::move(databaseFile)), m_rows(m_store.pins())
{
    m_refreshTimer.setSingleShot(true); m_refreshTimer.setInterval(100);
    connect(&m_refreshTimer, &QTimer::timeout, this, &SidebarPinsModel::beginRefresh);
    if (m_library) m_listener = m_library->registerEventListener(libraryEvent, this);
    if (!m_store.readable()) fail(m_store.readError(), [this] { reloadStore(); });
    refresh();
}
SidebarPinsModel::~SidebarPinsModel()
{
    m_refreshTimer.stop();
    delete m_picker.data();
    if (m_library) {
        if (m_listener) m_library->unregisterEventListener(m_listener);
        for (const auto task : std::as_const(m_tasks)) m_library->cancelMLTask(this, task);
    }
}
bool SidebarPinsModel::busy() const { return m_adding || !m_queue.isEmpty() || !m_picker.isNull(); }
void SidebarPinsModel::reloadStore()
{
    QString error;
    if (!m_store.reload(error)) { fail(error, [this] { reloadStore(); }); return; }
    beginResetModel(); m_rows = m_store.pins(); endResetModel(); emit countChanged(); refresh();
}
void SidebarPinsModel::retryEdit(std::function<void()> operation)
{
    if (m_store.needsReload()) {
        QString error;
        if (!m_store.reload(error)) {
            fail(error, [this, operation] { retryEdit(operation); });
            return;
        }
        // A peer may have added, removed or reordered rows. Publish the fresh
        // catalog before replaying the pending key-based operation.
        cancelActivation();
        beginResetModel(); m_rows = m_store.pins(); endResetModel();
        emit countChanged(); refresh();
    }
    operation();
}
int SidebarPinsModel::rowCount(const QModelIndex &parent) const { return parent.isValid() ? 0 : count(); }
bool SidebarPinsModel::available(const SidebarPin &pin) const
{ return m_available.value(pin.key, !collection(pin) || m_library); }
QVariant SidebarPinsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= count()) return {};
    const auto &pin = m_rows[index.row()];
    switch (role) {
    case KeyRole: return pin.key;
    case KindRole: return pin.kind;
    case TitleRole: return pin.title;
    case ArtworkRole: return pin.artwork;
    case AvailableRole: return available(pin);
    default: return {};
    }
}
QHash<int, QByteArray> SidebarPinsModel::roleNames() const
{ return {{KeyRole,"pinKey"}, {KindRole,"kind"}, {TitleRole,"title"}, {ArtworkRole,"artwork"}, {AvailableRole,"available"}}; }
QVariantMap SidebarPinsModel::get(int row) const
{
    if (row < 0 || row >= count()) return {};
    auto value = m_rows[row].toVariant(); value[QStringLiteral("available")] = available(m_rows[row]); return value;
}
int SidebarPinsModel::indexOf(const QString &key) const
{ for (int i = 0; i < count(); ++i) if (m_rows[i].key == key) return i; return -1; }
void SidebarPinsModel::fail(const QString &error, std::function<void()> retry)
{ m_error = error; m_retry = std::move(retry); emit stateChanged(); }
bool SidebarPinsModel::enqueue(Addition request)
{
    if (m_queue.size() >= SidebarPinsStore::maximumPins) { fail(tr("Wait for the pending pins to finish.")); return false; }
    m_queue.enqueue(std::move(request)); emit stateChanged(); drain(); return true;
}
bool SidebarPinsModel::pinItem(MLItemId id)
{
    if (m_retry) return false;
    if (!m_library || id.id <= 0 || (id.type != VLC_ML_PARENT_UNKNOWN && id.type != VLC_ML_PARENT_FOLDER
        && id.type != VLC_ML_PARENT_PLAYLIST && id.type != VLC_ML_PARENT_ARTIST))
    { fail(tr("This library item cannot be pinned.")); return false; }
    return enqueue({id, {}, false});
}
bool SidebarPinsModel::pinUrl(const QUrl &url, const QString &kind, const QString &name)
{
    if (m_retry) return false;
    SidebarPin pin; pin.kind = kind; pin.mrl = url; pin.title = name;
    QString error;
    if (!SidebarPin::normalize(pin, error)) { fail(error); return false; }
    return enqueue({{}, {pin}, true});
}
bool SidebarPinsModel::pinUrls(const QVariantList &urls)
{
    if (m_retry) return false;
    QVector<SidebarPin> pins;
    for (const auto &entry : urls) {
        const auto url = entry.toUrl();
        SidebarPin pin; pin.mrl = url;
        pin.kind = url.isLocalFile() && QFileInfo(url.toLocalFile()).isDir()
            ? QStringLiteral("folder") : QStringLiteral("media");
        QString error;
        if (!SidebarPin::normalize(pin, error)) { fail(error); return false; }
        pins.push_back(std::move(pin));
    }
    return pins.isEmpty() || enqueue({{}, std::move(pins), true});
}
void SidebarPinsModel::drain()
{
    if (m_adding || m_retry || m_queue.isEmpty()) return;
    if (m_queue.head().resolved) { finishAddition(); return; }
    m_adding = true; emit stateChanged();
    struct Context { SidebarPin pin; };
    const auto id = m_queue.head().id;
    const auto path = m_databaseFile;
    const auto task = m_library->runOnMLThread<Context>(this,
        [id, path](vlc_medialibrary_t *ml, Context &ctx) { const auto scope = databaseScope(path); ctx.pin = describe(ml, id, scope); if (scope != databaseScope(path)) ctx.pin = {}; },
        [this](quint64 task, Context &ctx) {
            m_tasks.remove(task); m_adding = false;
            if (ctx.pin.kind.isEmpty()) { m_failedAddition = true; fail(tr("The item is no longer available. Restore it and retry."), [this] { drain(); }); return; }
            m_queue.head().pins = {ctx.pin}; m_queue.head().resolved = true; if (!m_retry) finishAddition(); else emit stateChanged();
        }, "SIDEBAR_PIN_ADD_QUEUE");
    m_tasks.insert(task);
}
void SidebarPinsModel::publishAppend()
{
    const auto &saved = m_store.pins();
    if (saved.size() > m_rows.size()) {
        const auto first = count(); beginInsertRows({}, first, int(saved.size()) - 1);
        m_rows = saved; endInsertRows(); emit countChanged();
    }
    refresh();
}
void SidebarPinsModel::finishAddition()
{
    QString error;
    if (!m_store.add(m_queue.head().pins, error)) { m_failedAddition = true; fail(error, [this] { retryEdit([this] { drain(); }); }); return; }
    m_failedAddition = false; m_queue.dequeue(); publishAppend(); m_error.clear(); emit stateChanged();
    QTimer::singleShot(0, this, [this] { drain(); });
}
bool SidebarPinsModel::remove(const QString &key)
{
    if (m_retry || busy()) return false;
    const auto row = indexOf(key);
    if (row < 0) return true;
    QString error;
    if (!m_store.remove(key, error)) { fail(error, [this, key] { retryEdit([this, key] { remove(key); }); }); return false; }
    cancelActivation(); beginRemoveRows({}, row, row); m_rows.removeAt(row); endRemoveRows();
    m_available.remove(key); emit countChanged(); m_error.clear(); emit stateChanged(); refresh(); return true;
}
bool SidebarPinsModel::move(const QString &key, int destination)
{
    if (m_retry || busy()) return false;
    const auto row = indexOf(key);
    if (row < 0 || destination < 0 || destination >= count()) { fail(tr("The pinned item could not be moved.")); return false; }
    QString error;
    if (!m_store.move(key, destination, error)) { fail(error, [this, key, destination] { retryEdit([this, key, destination] { move(key, destination); }); }); return false; }
    if (row != destination) {
        beginMoveRows({}, row, row, {}, destination > row ? destination + 1 : destination);
        m_rows.move(row, destination); endMoveRows();
    }
    m_error.clear(); emit stateChanged(); return true;
}
void SidebarPinsModel::retry()
{
    auto operation = std::move(m_retry); m_retry = {}; m_failedAddition = false; m_error.clear(); emit stateChanged();
    if (operation) operation();
    drain();
}
void SidebarPinsModel::dismissError()
{
    if (m_failedAddition && !m_queue.isEmpty() && !m_adding) m_queue.dequeue();
    m_failedAddition = false;
    m_retry = {}; m_error.clear(); emit stateChanged(); drain();
}
void SidebarPinsModel::cancelActivation() { ++m_activation; }
void SidebarPinsModel::activate(const QString &key, int reason)
{
    if (m_retry) return;
    const auto row = indexOf(key);
    if (row < 0) return;
    const auto pin = m_rows[row]; const auto serial = ++m_activation;
    m_error.clear(); emit stateChanged();
    if (!collection(pin)) {
        if (pin.mrl.isLocalFile() && !QFileInfo::exists(pin.mrl.toLocalFile())) {
            m_available[key] = false; emit dataChanged(index(row), index(row), {AvailableRole});
            fail(tr("This pinned item is unavailable. Restore it or remove its pin.")); return;
        }
        if (!available(pin)) {
            m_available[key] = true; emit dataChanged(index(row), index(row), {AvailableRole});
        }
        if (pin.kind == QStringLiteral("media")) { emit mediaRequested(pin.mrl, pin.title); return; }
        const auto encoded = pin.mrl.toEncoded(), name = pin.title.toUtf8();
        auto *item = input_item_NewDirectory(encoded.constData(), name.constData(), pin.mrl.isLocalFile() ? ITEM_LOCAL : ITEM_NET);
        if (!item) { fail(tr("The pinned folder could not be opened.")); return; }
        SharedInputItem shared(item, false); MediaTreePtr tree(vlc_media_tree_New(), false);
        if (!tree.get()) { fail(tr("The pinned folder could not be opened.")); return; }
        { MediaTreeLocker lock{tree}; if (!vlc_media_tree_Add(tree.get(), &tree->root, item)) { fail(tr("The pinned folder could not be opened.")); return; } }
        emit browseRequested(QVariant::fromValue(NetworkTreeItem(tree, shared)), reason); return;
    }
    if (!m_library) { fail(tr("Enable the media library and restart to open this collection.")); return; }
    struct Context { bool valid = false; };
    const auto path = m_databaseFile;
    const auto task = m_library->runOnMLThread<Context>(this,
        [pin, path](vlc_medialibrary_t *ml, Context &ctx) { const auto scope = databaseScope(path); ctx.valid = targetAvailable(ml, pin, scope) && scope == databaseScope(path); },
        [this, pin, reason, serial](quint64 task, Context &ctx) {
            m_tasks.remove(task);
            if (serial != m_activation || indexOf(pin.key) < 0) return;
            if (!ctx.valid) { fail(tr("This collection is unavailable. Restore the original library or remove its pin.")); refresh(); return; }
            emit collectionRequested(pin.kind, nativeId(pin), pin.title, reason);
        }, "SIDEBAR_PIN_ACTIVATE_QUEUE");
    m_tasks.insert(task);
}
void SidebarPinsModel::refresh() { ++m_generation; m_refreshTimer.start(); }
void SidebarPinsModel::beginRefresh()
{
    if (!m_library || m_refreshing) return;
    m_refreshing = true;
    struct Context { QHash<QString, bool> available; };
    const auto generation = m_generation;
    const auto rows = m_rows;
    const auto path = m_databaseFile;
    const auto task = m_library->runOnMLThread<Context>(this,
        [rows, path](vlc_medialibrary_t *ml, Context &ctx) {
            const auto scope = databaseScope(path);
            for (const auto &pin : rows) ctx.available[pin.key] = targetAvailable(ml, pin, scope);
        }, [this, generation](quint64 task, Context &ctx) {
            m_tasks.remove(task); m_refreshing = false;
            if (generation != m_generation) { m_refreshTimer.start(); return; }
            m_available = std::move(ctx.available);
            if (count()) emit dataChanged(index(0), index(count() - 1), {AvailableRole});
        }, "SIDEBAR_PIN_REFRESH_QUEUE");
    m_tasks.insert(task);
}
void SidebarPinsModel::libraryEvent(void *data, const vlc_ml_event_t *event)
{
    switch (event->i_type) {
    case VLC_ML_EVENT_MEDIA_UPDATED: case VLC_ML_EVENT_MEDIA_DELETED:
    case VLC_ML_EVENT_FOLDER_UPDATED: case VLC_ML_EVENT_FOLDER_DELETED:
    case VLC_ML_EVENT_PLAYLIST_UPDATED: case VLC_ML_EVENT_PLAYLIST_DELETED:
    case VLC_ML_EVENT_ARTIST_UPDATED: case VLC_ML_EVENT_ARTIST_DELETED:
    {
        auto *self = static_cast<SidebarPinsModel *>(data);
        QMetaObject::invokeMethod(self, [self] { self->refresh(); }, Qt::QueuedConnection); break;
    }
    default: break;
    }
}
void SidebarPinsModel::openPicker(QDialog *dialog, QWindow *owner)
{
    m_picker = dialog;
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowModality(Qt::WindowModal);
    if (owner) { dialog->winId(); dialog->windowHandle()->setTransientParent(owner); }
    connect(dialog, &QDialog::finished, this, [this] { m_picker = nullptr; emit stateChanged(); });
    emit stateChanged(); dialog->open();
}
void SidebarPinsModel::chooseFiles(QWindow *owner)
{
    if (m_retry || m_picker || !qobject_cast<QApplication *>(QCoreApplication::instance())) return;
    auto *dialog = new QFileDialog(nullptr, tr("Pin files"));
    dialog->setFileMode(QFileDialog::ExistingFiles);
    connect(dialog, &QDialog::accepted, this, [this, dialog] {
        QVariantList urls; for (const auto &url : dialog->selectedUrls()) urls.push_back(url); pinUrls(urls);
    });
    openPicker(dialog, owner);
}
void SidebarPinsModel::chooseFolder(QWindow *owner)
{
    if (m_retry || m_picker || !qobject_cast<QApplication *>(QCoreApplication::instance())) return;
    auto *dialog = new QFileDialog(nullptr, tr("Pin folder"));
    dialog->setFileMode(QFileDialog::Directory); dialog->setOption(QFileDialog::ShowDirsOnly);
    connect(dialog, &QDialog::accepted, this, [this, dialog] {
        const auto urls = dialog->selectedUrls(); if (!urls.isEmpty()) pinUrl(urls[0], QStringLiteral("folder"));
    });
    openPicker(dialog, owner);
}
void SidebarPinsModel::chooseSource(QWindow *owner)
{
    if (m_retry || m_picker || !qobject_cast<QApplication *>(QCoreApplication::instance())) return;
    auto *dialog = new QInputDialog;
    dialog->setWindowTitle(tr("Pin source")); dialog->setLabelText(tr("Folder or source address"));
    dialog->setInputMode(QInputDialog::TextInput);
    connect(dialog, &QDialog::accepted, this, [this, dialog] {
        pinUrl(QUrl(dialog->textValue().trimmed(), QUrl::StrictMode), QStringLiteral("source"));
    });
    openPicker(dialog, owner);
}
