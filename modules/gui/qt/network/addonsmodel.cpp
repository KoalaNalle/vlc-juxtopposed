/*****************************************************************************
 * Copyright (C) 2020 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#include "addonsmodel.hpp"

#include "util/locallistbasemodel.hpp"

#include "medialibrary/mlhelper.hpp"

#include "playlist/media.hpp"
#include "playlist/playlist_controller.hpp"

#include <memory>
#include <QPixmap>
#include <QQmlFile>
#include <QQueue>
#include <QSet>
#include <QHash>

#include <vlc_media_source.h>
#include <vlc_addons.h>
#include <vlc_cxx_helpers.hpp>
#include <vlc_addons.h>

#define CHECK_ADDON_TYPE_MATCH(type) \
    static_assert(static_cast<addon_type_t>(AddonsModel::Type::TYPE_ ## type) == ADDON_##type)

CHECK_ADDON_TYPE_MATCH(UNKNOWN);
CHECK_ADDON_TYPE_MATCH(PLAYLIST_PARSER);
CHECK_ADDON_TYPE_MATCH(SERVICE_DISCOVERY);
CHECK_ADDON_TYPE_MATCH(SKIN2);
CHECK_ADDON_TYPE_MATCH(PLUGIN);
CHECK_ADDON_TYPE_MATCH(INTERFACE);
CHECK_ADDON_TYPE_MATCH(META);
CHECK_ADDON_TYPE_MATCH(OTHER);

#undef CHECK_ADDON_TYPE_MATCH

#define CHECK_ADDON_STATE_MATCH(type) \
    static_assert(static_cast<addon_state_t>(AddonsModel::State::STATE_ ## type) == ADDON_##type)

CHECK_ADDON_STATE_MATCH(INSTALLING);
CHECK_ADDON_STATE_MATCH(INSTALLED);
CHECK_ADDON_STATE_MATCH(UNINSTALLING);

#undef CHECK_ADDON_STATE_MATCH

namespace {

using AddonPtr = ::vlc::vlc_shared_data_ptr<addon_entry_t,
                                     &addon_entry_Hold, &addon_entry_Release>;

class AddonItem {
public:
    //
    AddonItem( AddonPtr addon, quint64 generation )
    {
        vlc_mutex_locker locker{&addon->lock};
        name = qfu( addon->psz_name );
        summary = qfu( addon->psz_summary ).trimmed();
        description = qfu( addon->psz_description ).trimmed();
        author = qfu( addon->psz_author );
        sourceUrl = QUrl( addon->psz_source_uri );
        entry = addon;
        uuid = QByteArray( reinterpret_cast<const char*>(addon->uuid), sizeof( addon_uuid_t ));
        key = uuid != QByteArray(ADDON_UUID_SIZE, '\0') ? QString::fromLatin1(uuid.toHex())
            : QStringLiteral("local:%1:%2").arg(generation).arg(quintptr(addon.get()), 0, 16);

        if ( addon->psz_image_data ) {
            char *cDir = config_GetUserDir( VLC_CACHE_DIR );
            if (likely(cDir != nullptr))
            {
                QDir dir( cDir );
                free(cDir);
                dir.mkdir("art");
                dir.cd("art");
                dir.mkdir("qt-addon-covers");
                dir.cd("qt-addon-covers");
                char* uuidStr = addons_uuid_to_psz( &addon->uuid );
                QString filename = QString("addon_thumbnail_%1.png").arg(uuidStr);
                free(uuidStr);
                QString absoluteFilePath =  dir.absoluteFilePath(filename);

                if ( !QFileInfo::exists( absoluteFilePath )) {
                    QPixmap pixmap;
                    pixmap.loadFromData( QByteArray::fromBase64( QByteArray( addon->psz_image_data ) ),
                                        0,
                                        Qt::AutoColor
                                        );
                    pixmap.save(absoluteFilePath);
                }
                artworkUrl = QUrl::fromLocalFile( absoluteFilePath );
            }
        }
        else if ( addon->e_flags & ADDON_BROKEN )
            artworkUrl = QUrl( ":/addons/addon_broken.svg" );
        else
            artworkUrl = QUrl( ":/addons/addon_default.svg" );
    }

public:
    QByteArray  uuid;
    QString key;
    QString name;
    QString summary;
    QString description;
    QString author;
    QUrl sourceUrl;
    QUrl artworkUrl;
    AddonPtr entry;
};

} //namespace

using AddonItemPtr = std::shared_ptr<AddonItem> ;
using AddonItemList = std::vector<AddonItemPtr> ;

// ListCache specialisation

template<>
bool ListCache<AddonItemPtr>::compareItems(const AddonItemPtr& a, const AddonItemPtr& b)
{
    //just compare the pointers here
    return a->key == b->key;
}

// AddonsModelPrivate

class AddonsModelPrivate
    : public LocalListBaseModelPrivate<AddonItemPtr>
{

public:
    Q_DECLARE_PUBLIC(AddonsModel)

public: //ctor/dtor
    AddonsModelPrivate(AddonsModel* pub)
        : LocalListBaseModelPrivate<AddonItemPtr>(pub)
    {
    }

    ~AddonsModelPrivate()
    {
        ++m_generation;
        if ( m_manager )
            addons_manager_Delete( m_manager );
    }

public:
    const AddonItem* getItemForRow(int row) const
    {
        const AddonItemPtr* ref = item(row);
        if (ref)
            return ref->get();
        return nullptr;
    }

    bool gatherRepository(const QUrl &uri, bool installPackage = false);
    bool change(const QString &key, bool install);
    bool nativeBusy() const { return !m_discoveries.isEmpty() || !m_operations.isEmpty(); }
    AddonItemPtr findKey(const QString &key) const
    {
        for (const auto &item : m_items) if (item->key == key) return item;
        return {};
    }
    bool matches(const AddonItemPtr &item, const QString &pattern) const
    {
        vlc_mutex_locker locker{&item->entry->lock};
        return (m_stateFilter == AddonsModel::State::STATE_NONE || static_cast<AddonsModel::State>(item->entry->e_state) == m_stateFilter)
            && (m_typeFilter == AddonsModel::Type::TYPE_NONE || static_cast<AddonsModel::Type>(item->entry->e_type) == m_typeFilter)
            && (pattern.isEmpty() || item->name.contains(pattern, Qt::CaseInsensitive)
                || item->author.contains(pattern, Qt::CaseInsensitive) || item->summary.contains(pattern, Qt::CaseInsensitive));
    }
    bool actionable(const QString &key, bool install) const
    {
        if (!m_manager || nativeBusy()) return false;
        const auto item = findKey(key);
        if (!item || !matches(item, m_searchPattern)) return false;
        vlc_mutex_locker locker{&item->entry->lock};
        return (item->entry->e_flags & ADDON_MANAGEABLE)
            && item->uuid != QByteArray(ADDON_UUID_SIZE, '\0')
            && item->entry->e_type != ADDON_UNKNOWN && item->entry->e_type != ADDON_PLUGIN && item->entry->e_type != ADDON_OTHER
            && item->entry->e_state == (install ? ADDON_NOTINSTALLED : ADDON_INSTALLED);
    }
    void error(const QString &value)
    { Q_Q(AddonsModel); if (m_error != value) { m_error = value; emit q->lastErrorChanged(); } }
    bool fail(const QString &value) { error(value); return false; }
    void status(const QString &value)
    { Q_Q(AddonsModel); if (m_status != value) { m_status = value; emit q->statusTextChanged(); } }
    void dirty()
    { Q_Q(AddonsModel); ++m_revision; invalidateCache(); emit q->selectionChanged(); }
    void busyChanged()
    { Q_Q(AddonsModel); m_loading = !m_discoveries.isEmpty(); emit q->busyChanged(); emit q->loadingChanged(); emit q->selectionChanged(); }
    void saveNeeded(bool value)
    { Q_Q(AddonsModel); if (m_saveNeeded != value) { m_saveNeeded = value; emit q->catalogSaveNeededChanged(); } }

public: //BaseModelPrivateT implementation
    bool initializeModel() override;

    LocalListCacheLoader<AddonItemPtr>::ItemCompare getSortFunction() const override
    {
        if (m_sortOrder == Qt::SortOrder::DescendingOrder)
            return [](const AddonItemPtr& a, const AddonItemPtr& b){
                return QString::compare(a->name, b->name) > 0;
            };
        else
            return [](const AddonItemPtr& a, const AddonItemPtr& b) {
                return QString::compare(a->name, b->name) < 0;
            };
    }



public: //discovery callbacks
    void addonFound( AddonPtr addon );
    void addonChanged( AddonPtr addon, bool complete, int storage, int catalog );
    void discoveryEnded(int result);

public: //LocalListCacheLoader implementation
    //return the data matching the pattern
    AddonItemList getModelData(const QString& pattern) const override
    {
        if (pattern.isEmpty()
            && m_typeFilter == AddonsModel::Type::TYPE_NONE
            && m_stateFilter == AddonsModel::State::STATE_NONE)
            return m_items;

        AddonItemList items;
        std::copy_if(
            m_items.cbegin(), m_items.cend(),
            std::back_inserter(items),
            [this, &pattern](const AddonItemPtr& item) { return matches(item, pattern); });
        return items;
    }

public: // data
    MainCtx* m_ctx = nullptr;
    addons_manager_t* m_manager = nullptr;
    AddonItemList m_items;
    struct CallbackContext { AddonsModelPrivate *model; quint64 generation; };
    std::unique_ptr<CallbackContext> m_callbackContext;
    quint64 m_generation = 0;
    struct Discovery { QUrl uri; bool installPackage; QSet<QString> keys; };
    QQueue<Discovery> m_discoveries;
    QHash<QString, bool> m_operations;
    QString m_selected, m_error, m_status;
    bool m_saveNeeded = false;
    AddonsModel::Type m_typeFilter = AddonsModel::Type::TYPE_NONE;
    AddonsModel::State m_stateFilter = AddonsModel::State::STATE_NONE;
};

AddonsModel::AddonsModel( QObject* parent )
    : BaseModel( new AddonsModelPrivate(this), parent )
{
    connect(this, &BaseModel::searchPatternChanged, this, [this] { setSelectedKey({}); });
    connect(this, &AddonsModel::stateFilterChanged, this, [this] { setSelectedKey({}); });
    connect(this, &AddonsModel::typeFilterChanged, this, [this] { setSelectedKey({}); });
}

QVariant AddonsModel::data( const QModelIndex& index, int role ) const
{
    Q_D(const AddonsModel);
    if (!d->m_ctx)
        return {};

    const AddonItem* item = d->getItemForRow(index.row());
    if (!item) {
        // Cache counts arrive before their rows. Typed QML delegates need
        // stable empty values until the queued page load emits dataChanged.
        if (role == Role::NAME || role == Role::AUTHOR || role == Role::SELECTION_KEY || role == Role::STATE_LABEL)
            return QString{};
        if (role == Role::MANAGEABLE) return false;
        return {};
    }

    switch ( role )
    {
        case Role::SELECTION_KEY:
            return item->key;
        case Role::STATE_LABEL:
        {
            vlc_mutex_locker locker{&item->entry->lock};
            if (!(item->entry->e_flags & ADDON_MANAGEABLE)) return qtr("Installed manually");
            if (item->entry->e_state == ADDON_INSTALLING) return qtr("Installing…");
            if (item->entry->e_state == ADDON_UNINSTALLING) return qtr("Removing…");
            if (item->entry->e_type == ADDON_UNKNOWN || item->entry->e_type == ADDON_PLUGIN || item->entry->e_type == ADDON_OTHER)
                return qtr("Unsupported type");
            if (item->entry->e_flags & ADDON_BROKEN) return qtr("Needs attention");
            return item->entry->e_state == ADDON_INSTALLED ? qtr("Installed") : qtr("Available");
        }
        case Qt::DisplayRole:
        case Role::NAME:
            return item->name;
        case Role::AUTHOR:
            return item->author;
        case Role::SUMMARY:
            return item->summary;
        case Role::DESCRIPTION:
            return item->description;
        case Role::DOWNLOADS:
        {
            vlc_mutex_locker locker{&item->entry->lock};
            return QVariant::fromValue( item->entry->i_downloads );
        }
        case Role::SCORE:
        {
            vlc_mutex_locker locker{&item->entry->lock};
            return item->entry->i_score;
        }
        case Role::STATE:
        {
            vlc_mutex_locker locker{&item->entry->lock};
            return item->entry->e_state;
        }
        case Role::ARTWORK:
        {
            vlc_mutex_locker locker{&item->entry->lock};
            return item->artworkUrl;
        }
        case Role::TYPE:
        {
            vlc_mutex_locker locker{&item->entry->lock};
            return QVariant::fromValue(item->entry->e_type);
        }
        case Role::LINK:
        {
            vlc_mutex_locker locker{&item->entry->lock};
            return QString{ item->entry->psz_source_uri };
        }
        case Role::ADDON_VERSION:
        {
            vlc_mutex_locker locker{&item->entry->lock};
            return qfu(item->entry->psz_version);
        }
        case Role::UUID:
            return item->uuid;
        case Role::DOWNLOAD_COUNT:
        {
            vlc_mutex_locker locker{&item->entry->lock};
            return QVariant::fromValue(item->entry->i_downloads);
        }
        case Role::FILENAME:
        {
            vlc_mutex_locker locker{&item->entry->lock};
            QList<QString> list;
            addon_file_t *p_file;
            ARRAY_FOREACH( p_file, item->entry->files )
            list << qfu( p_file->psz_filename );
            return QVariant{ list };
        }
        case  Role::BROKEN:
        {
            vlc_mutex_locker locker{&item->entry->lock};
            return item->entry->e_flags & ADDON_BROKEN;
        }
        case  Role::MANAGEABLE:
        {
            vlc_mutex_locker locker{&item->entry->lock};
            return item->entry->e_flags & ADDON_MANAGEABLE;
        }
        case  Role::UPDATABLE:
        {
            vlc_mutex_locker locker{&item->entry->lock};
            return item->entry->e_flags & ADDON_UPDATABLE;
        }

        case Qt::ToolTipRole:
        {
            vlc_mutex_locker locker{&item->entry->lock};
            if ( !( item->entry->e_flags & ADDON_MANAGEABLE ) )
            {
                return qtr("This addon has been installed manually. VLC can't manage it by itself.");
            }
            return QVariant{};
        }
        case Qt::DecorationRole:
            return QPixmap{QQmlFile::urlToLocalFileOrQrc(item->artworkUrl)};
        default:
            return {};
    }
}

bool AddonsModel::setData(const QModelIndex& index, const QVariant &value, int role)
{
    if (!index.isValid() || index.model() != this || role != Role::STATE) return false;
    const auto state = value.value<AddonsModel::State>();
    if (state == State::STATE_INSTALLING) return installService(index.row());
    if (state == State::STATE_UNINSTALLING) return removeService(index.row());
    return false;
}

Qt::ItemFlags AddonsModel::flags( const QModelIndex &index ) const
{
    Q_D(const AddonsModel);
    Qt::ItemFlags qtFlags = BaseModel::flags(index);

    const AddonItem* item = d->getItemForRow(index.row());
    if (!item)
        return qtFlags;

    {
        vlc_mutex_locker locker{&item->entry->lock};
        addon_state_t addonState = item->entry->e_state;
        if (addonState == ADDON_INSTALLING || addonState == ADDON_UNINSTALLING)
            qtFlags &= ~Qt::ItemIsEnabled;

    }
    qtFlags |= Qt::ItemIsEditable;

    return qtFlags;
}

QHash<int, QByteArray> AddonsModel::roleNames() const
{
    return {
        { Role::NAME, QByteArrayLiteral("name") },
        { Role::AUTHOR, QByteArrayLiteral("author") },
        { Role::SUMMARY, QByteArrayLiteral("summary") },
        { Role::DESCRIPTION, QByteArrayLiteral("description") },
        { Role::DOWNLOADS, QByteArrayLiteral("downloads") },
        { Role::SCORE, QByteArrayLiteral("score") },
        { Role::STATE, QByteArrayLiteral("state") },
        { Role::ARTWORK, QByteArrayLiteral("artwork") },
        { Role::TYPE, QByteArrayLiteral("type") },
        { Role::LINK, QByteArrayLiteral("link") },
        { Role::FILENAME, QByteArrayLiteral("filename") },
        { Role::ADDON_VERSION, QByteArrayLiteral("version") },
        { Role::UUID, QByteArrayLiteral("uuid") },
        { Role::DOWNLOAD_COUNT, QByteArrayLiteral("downloadCount") },
        { Role::BROKEN, QByteArrayLiteral("broken") },
        { Role::MANAGEABLE, QByteArrayLiteral("manageable") },
        { Role::UPDATABLE, QByteArrayLiteral("updatable") },
        { Role::SELECTION_KEY, QByteArrayLiteral("selectionKey") },
        { Role::STATE_LABEL, QByteArrayLiteral("stateLabel") },
    };
}

bool AddonsModel::installService(int idx)
{
    Q_D(AddonsModel);

    const AddonItem* item = d->getItemForRow(idx);
    return item ? installByKey(item->key) : d->fail(qtr("Choose an available add-on first."));
}

bool AddonsModel::removeService(int idx)
{
    Q_D(AddonsModel);

    const AddonItem* item = d->getItemForRow(idx);
    return item ? removeByKey(item->key) : d->fail(qtr("Choose an installed add-on first."));
}

bool AddonsModel::loadFromDefaultRepository()
{
    Q_D(AddonsModel);
    return d->gatherRepository(QUrl(QStringLiteral("repo://")));
}

bool AddonsModel::loadFromExternalRepository(QUrl uri)
{
    Q_D(AddonsModel);
    return d->gatherRepository(uri);
}

bool AddonsModel::installByKey(const QString &key) { Q_D(AddonsModel); return d->change(key, true); }
bool AddonsModel::removeByKey(const QString &key) { Q_D(AddonsModel); return d->change(key, false); }
bool AddonsModel::installSelected() { return installByKey(selectedKey()); }
bool AddonsModel::removeSelected() { return removeByKey(selectedKey()); }
int AddonsModel::selectionIndex() const
{
    Q_D(const AddonsModel);
    auto items=d->getModelData(d->m_searchPattern);
    std::sort(items.begin(),items.end(),d->getSortFunction());
    const auto found=std::find_if(items.begin(),items.end(),[&](const auto &item){return item->key==d->m_selected;});
    return found==items.end()?-1:int(std::distance(items.begin(),found));
}
bool AddonsModel::available() const { Q_D(const AddonsModel); return d->m_manager != nullptr; }
bool AddonsModel::busy() const { Q_D(const AddonsModel); return d->nativeBusy(); }
bool AddonsModel::discovering() const { Q_D(const AddonsModel); return !d->m_discoveries.isEmpty(); }
QString AddonsModel::lastError() const { Q_D(const AddonsModel); return d->m_error; }
QString AddonsModel::statusText() const { Q_D(const AddonsModel); return d->m_status; }
bool AddonsModel::catalogSaveNeeded() const { Q_D(const AddonsModel); return d->m_saveNeeded; }
QString AddonsModel::selectedKey() const { Q_D(const AddonsModel); return d->m_selected; }
bool AddonsModel::canInstall() const { Q_D(const AddonsModel); return d->actionable(d->m_selected, true); }
bool AddonsModel::canRemove() const { Q_D(const AddonsModel); return d->actionable(d->m_selected, false); }
void AddonsModel::clearError() { Q_D(AddonsModel); d->error({}); }
void AddonsModel::showAll() { setTypeFilter(Type::TYPE_NONE); setStateFilter(State::STATE_NONE); }
void AddonsModel::setSelectedKey(const QString &key)
{
    Q_D(AddonsModel);
    const auto item = d->findKey(key);
    const auto selected = item && d->matches(item, d->m_searchPattern) ? key : QString{};
    if (d->m_selected == selected) return;
    d->m_selected = selected; emit selectionChanged();
}
bool AddonsModel::retryCatalogSave()
{
    Q_D(AddonsModel);
    if (!d->m_manager || busy()) return d->fail(qtr("Wait for the current add-on operation to finish."));
    if (addons_manager_SaveCatalog(d->m_manager) != VLC_SUCCESS)
        return d->fail(qtr("The add-on catalog could not be saved. Check the profile folder and retry."));
    d->saveNeeded(false); d->error({}); d->status(qtr("Add-on catalog saved.")); return true;
}
bool AddonsModel::reloadCatalog()
{
    Q_D(AddonsModel);
    if (busy()) return d->fail(qtr("Wait for the current add-on operation to finish."));
    return d->initializeModel();
}
bool AddonsModel::addFromPackage(const QUrl &uri)
{
    Q_D(AddonsModel);
    if (busy()) return d->fail(qtr("Wait for the current add-on operation to finish."));
    const QFileInfo file(uri.toLocalFile());
    if (!uri.isLocalFile() || uri.hasQuery() || uri.hasFragment() || !file.isFile() || !file.isReadable()
        || file.suffix().compare(QStringLiteral("vlp"), Qt::CaseInsensitive))
        return d->fail(qtr("Choose a readable local VLC package (.vlp)."));
    return d->gatherRepository(QUrl::fromLocalFile(file.absoluteFilePath()), true);
}

void AddonsModel::setCtx(MainCtx* ctx)
{
    Q_D(AddonsModel);

    if (ctx == d->m_ctx)
        return;

    d->m_ctx = ctx;
    d->initializeModel();
    emit ctxChanged();
}

MainCtx* AddonsModel::getCtx() const
{
    Q_D(const AddonsModel);
    return d->m_ctx;
}

AddonsModel::Type AddonsModel::getTypeFilter() const
{
    Q_D(const AddonsModel);
    return d->m_typeFilter;
}

void AddonsModel::setTypeFilter(AddonsModel::Type type)
{
    Q_D(AddonsModel);
    if (type == d->m_typeFilter)
        return;
    d->m_typeFilter = type;

    d->m_revision++;
    invalidateCache();

    emit typeFilterChanged();
}

AddonsModel::State AddonsModel::getStateFilter() const
{
    Q_D(const AddonsModel);
    return d->m_stateFilter;
}

void AddonsModel::setStateFilter(AddonsModel::State state)
{
    Q_D(AddonsModel);
    if (state == d->m_stateFilter)
        return;
    d->m_stateFilter = state;

    d->m_revision++;
    invalidateCache();

    emit stateFilterChanged();
}

QString AddonsModel::getLabelForType(AddonsModel::Type type)
{
    switch ( type )
    {
    case Type::TYPE_SKIN2 :
        return qtr( "Skins" );
    case Type::TYPE_PLAYLIST_PARSER:
        return qtr("Playlist parsers");
    case Type::TYPE_SERVICE_DISCOVERY:
        return qtr("Service Discovery");
    case Type::TYPE_INTERFACE:
        return qtr("Interfaces");
    case Type::TYPE_META:
        return qtr("Art and meta fetchers");
    case Type::TYPE_EXTENSION:
        return qtr("Extensions");
    default:
        return qtr("Unknown");
    }
}

QColor AddonsModel::getColorForType(AddonsModel::Type type)
{
    QColor color;
    switch( type )
    {
    case Type::TYPE_EXTENSION:
        color = QColor(0xDB, 0xC5, 0x40);
        break;
    case Type::TYPE_PLAYLIST_PARSER:
        color = QColor(0x36, 0xBB, 0x59);
        break;
    case Type::TYPE_SERVICE_DISCOVERY:
        color = QColor(0xDB, 0x52, 0x40);
        break;
    case Type::TYPE_SKIN2:
        color = QColor(0x8B, 0xD6, 0xFC);
        break;
    case Type::TYPE_INTERFACE:
        color = QColor(0x00, 0x13, 0x85);
        break;
    case Type::TYPE_META:
        color = QColor(0xCD, 0x23, 0xBF);
        break;
    case Type::TYPE_PLUGIN:
    case Type::TYPE_UNKNOWN:
    case Type::TYPE_OTHER:
    default:
        break;
    }
    return color;
}

QString AddonsModel::getIconForType(AddonsModel::Type type)
{
    switch( type )
    {
    case AddonsModel::Type::TYPE_EXTENSION:
        return QStringLiteral("qrc:///addons/addon_yellow.svg");
    case AddonsModel::Type::TYPE_PLAYLIST_PARSER:
        return QStringLiteral("qrc:///addons/addon_green.svg");
    case AddonsModel::Type::TYPE_SERVICE_DISCOVERY:
        return QStringLiteral("qrc:///addons/addon_red.svg");
    case AddonsModel::Type::TYPE_SKIN2:
        return QStringLiteral("qrc:///addons/addon_cyan.svg");
    case AddonsModel::Type::TYPE_INTERFACE:
        return QStringLiteral("qrc:///addons/addon_blue.svg");
    case AddonsModel::Type::TYPE_META:
        return QStringLiteral("qrc:///addons/addon_magenta.svg");
    default:
        return QStringLiteral("qrc:///addons/addon_default.svg");
    }
    vlc_assert_unreachable();
}

int AddonsModel::getMaxScore()
{
    return ADDON_MAX_SCORE;
}

static void addonFoundCallback( addons_manager_t *manager, addon_entry_t *entry )
{
    const auto cookie = *static_cast<AddonsModelPrivate::CallbackContext*>(manager->owner.sys);
    QMetaObject::invokeMethod( cookie.model->q_func(), [cookie, entryPtr = AddonPtr(entry)]()
        {
            if (cookie.generation == cookie.model->m_generation)
                cookie.model->addonFound( std::move( entryPtr ) );
        }, Qt::QueuedConnection);
}

static void addonsDiscoveryEndedCallback( addons_manager_t *manager )
{
    const auto cookie = *static_cast<AddonsModelPrivate::CallbackContext*>(manager->owner.sys);
    const int result = addons_manager_GetDiscoveryResult(manager);
    QMetaObject::invokeMethod( cookie.model->q_func(), [cookie, result]()
        {
            if (cookie.generation == cookie.model->m_generation)
                cookie.model->discoveryEnded(result);
        }, Qt::QueuedConnection);
}

static void addonChangedCallback( addons_manager_t *manager, addon_entry_t *entry )
{
    const auto cookie = *static_cast<AddonsModelPrivate::CallbackContext*>(manager->owner.sys);
    int storage = VLC_EGENERIC, catalog = VLC_EGENERIC;
    const bool complete = addon_entry_GetOperationResult(entry, &storage, &catalog);
    QMetaObject::invokeMethod( cookie.model->q_func(), [cookie, entryPtr = AddonPtr(entry), complete, storage, catalog]()
        {
            if (cookie.generation == cookie.model->m_generation)
                cookie.model->addonChanged( std::move( entryPtr ), complete, storage, catalog );
        }, Qt::QueuedConnection);
}

bool AddonsModelPrivate::initializeModel()
{
    Q_Q(AddonsModel);
    if (m_qmlInitializing)
        return false;
    ++m_generation;
    if ( m_manager )
        addons_manager_Delete( m_manager );
    m_manager = nullptr;
    m_callbackContext.reset();
    m_items.clear(); m_discoveries.clear(); m_operations.clear(); m_selected.clear();
    error({}); status({}); saveNeeded(false);
    m_loading = true; emit q->loadingChanged();
    m_callbackContext = std::make_unique<CallbackContext>(CallbackContext{this, m_generation});
    struct addons_manager_owner owner =
    {
        m_callbackContext.get(),
        addonFoundCallback,
        addonsDiscoveryEndedCallback,
        addonChangedCallback,
    };

    if (m_ctx) m_manager = addons_manager_New( VLC_OBJECT( m_ctx->getIntf() ), &owner );
    if (!m_manager) error(qtr("The add-on manager is unavailable in this session."));
    else if (addons_manager_LoadCatalog(m_manager) != VLC_SUCCESS)
        error(qtr("The installed add-on catalog could not be read. Existing scripts are shown where possible."));
    m_revision++;
    m_loading = false;
    resetCache();
    emit q->availabilityChanged();
    emit q->busyChanged(); emit q->selectionChanged();
    emit q->loadingChanged();
    return m_manager != nullptr;
}


void AddonsModelPrivate::addonFound( AddonPtr addon )
{
    const auto item = std::make_shared<AddonItem>(addon, m_generation);
    auto existing = std::find_if(m_items.begin(), m_items.end(), [&](const auto &value) { return value->key == item->key; });
    if (existing == m_items.end()) m_items.push_back(item);
    else *existing = item;
    if (!m_discoveries.isEmpty()) m_discoveries.head().keys.insert(item->key);
    dirty();
}

void AddonsModelPrivate::addonChanged( AddonPtr addon, bool complete, int storage, int catalog )
{
    const auto item = std::make_shared<AddonItem>(addon, m_generation);
    auto existing = std::find_if(m_items.begin(), m_items.end(), [&](const auto &value) { return value->key == item->key; });
    if (existing == m_items.end()) m_items.push_back(item);
    else *existing = item;
    if (m_cache) { auto cached = item; m_cache->updateItem(std::move(cached)); }
    if (!m_discoveries.isEmpty()) m_discoveries.head().keys.insert(item->key);
    if (complete && m_operations.contains(item->key)) {
        const bool install = m_operations.take(item->key);
        if (storage != VLC_SUCCESS || catalog != VLC_SUCCESS) status({});
        saveNeeded(catalog != VLC_SUCCESS);
        if (storage != VLC_SUCCESS) {
            bool incomplete;
            { vlc_mutex_locker locker{&addon->lock}; incomplete = (addon->e_flags & ADDON_BROKEN) && addon->e_state == ADDON_INSTALLED; }
            error(install ? (incomplete
                ? qtr("Installation is incomplete. Use Remove to clean up its remaining files before retrying Add.")
                : qtr("The add-on could not be installed. Existing files were preserved; check the package and profile folder before retrying."))
                : qtr("The add-on could not be fully removed. Close anything using its files, then retry Remove."));
        }
        if (catalog != VLC_SUCCESS) error(storage == VLC_SUCCESS
            ? qtr("The files were processed, but the add-on catalog could not be saved. Retry saving the catalog before closing VLC.")
            : qtr("The add-on operation failed and its catalog could not be saved. Check the profile folder, retry saving the catalog, then retry the operation."));
        if (storage == VLC_SUCCESS && catalog == VLC_SUCCESS) status(install ? qtr("Add-on installed. Restart VLC to refresh extension menus.") : qtr("Add-on removed. Restart VLC to refresh extension menus."));
        busyChanged();
    }
    dirty();
}

void AddonsModelPrivate::discoveryEnded(int result)
{
    Q_Q(AddonsModel);
    if (m_discoveries.isEmpty()) return;
    const auto request = m_discoveries.dequeue();
    busyChanged(); dirty();
    if (result != VLC_SUCCESS) {
        error(request.installPackage ? qtr("The selected VLC package could not be read.") : qtr("The add-on catalog could not be loaded. Check the connection or source and retry."));
        status({}); return;
    }
    if (request.installPackage) {
        status({});
        if (request.keys.size() != 1) { error(qtr("Choose a VLC package containing one identifiable add-on.")); return; }
        q->setSearchPattern({}); q->setTypeFilter(AddonsModel::Type::TYPE_NONE); q->setStateFilter(AddonsModel::State::STATE_NONE);
        q->setSelectedKey(*request.keys.cbegin());
        if (q->canInstall()) q->installSelected();
        else error(qtr("This add-on is already installed or cannot be managed here. Remove an installed version before adding its replacement."));
    } else status(request.keys.isEmpty() ? qtr("No add-ons found.") : qtr("Add-on discovery complete."));
}

bool AddonsModelPrivate::gatherRepository(const QUrl &uri, bool installPackage)
{
    if (!m_manager || !uri.isValid() || uri.isEmpty()) return fail(qtr("The add-on source is unavailable."));
    if (!m_operations.isEmpty() || (installPackage && nativeBusy())) return fail(qtr("Wait for the current add-on operation to finish."));
    if (m_discoveries.isEmpty()) error({});
    const auto encoded = uri.toEncoded();
    if (addons_manager_GatherChecked(m_manager, encoded.constData()) != VLC_SUCCESS)
        return fail(qtr("The add-on search could not be started."));
    m_discoveries.enqueue(Discovery{uri, installPackage, {}});
    status(qtr("Looking for add-ons…")); busyChanged(); return true;
}

bool AddonsModelPrivate::change(const QString &key, bool install)
{
    error({});
    if (!actionable(key, install)) return fail(install ? qtr("Choose an available manageable add-on and wait for other operations to finish.") : qtr("Choose an installed manageable add-on and wait for other operations to finish."));
    const auto item = findKey(key);
    addon_uuid_t uuid; memcpy(uuid, item->uuid.constData(), sizeof(uuid));
    const int result = install ? addons_manager_Install(m_manager, uuid) : addons_manager_Remove(m_manager, uuid);
    if (result != VLC_SUCCESS) return fail(qtr("The add-on operation was rejected. Refresh the list and try again."));
    m_operations.insert(key, install);
    status(install ? qtr("Installing add-on…") : qtr("Removing add-on…")); busyChanged(); dirty(); return true;
}
