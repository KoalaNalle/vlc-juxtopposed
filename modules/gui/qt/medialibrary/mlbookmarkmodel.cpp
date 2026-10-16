/*****************************************************************************
 * Copyright (C) 2019 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * ( at your option ) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "mlbookmarkmodel.hpp"
#include "qt.hpp"

#include <vlc_cxx_helpers.hpp>

#include "medialib.hpp"
#include "mlhelper.hpp"

#include "util/shared_input_item.hpp"
#include "util/vlctick.hpp"
#include "player/player_controller.hpp"

#include <algorithm>

MLBookmarkModel::MLBookmarkModel( QObject *parent )
    : QAbstractListModel( parent )
    , m_currentItem( nullptr, &input_item_Release )
    , m_ml_event_handle( nullptr, [this](vlc_ml_event_callback_t* cb ) {
        if (!cb)
            return;
        if ( m_mediaLib )
            m_mediaLib->unregisterEventListener(cb);
    })
{}

MLBookmarkModel::~MLBookmarkModel()
{
    if(m_player && m_listener){
        vlc_player_locker lock{ m_player };
        vlc_player_RemoveListener( m_player, m_listener );
    }
}

QVariant MLBookmarkModel::data( const QModelIndex &index, int role ) const
{
    if ( !isValidBookmarkIndex(index) || m_player == nullptr )
        return QVariant{};

    const auto& bookmark = m_bookmarks->p_items[index.row()];

    if (role == Qt::DisplayRole || role == Qt::EditRole)
        role = columnToRole(index.column());

    switch(role){
        case BookmarkRoles::NameRole:
            return QVariant::fromValue( qfu( bookmark.psz_name ) );
        case BookmarkRoles::TimeRole:
            return QVariant::fromValue( VLCDuration::fromMS( bookmark.i_time ).formatHMS() );
        case BookmarkRoles::TimeMsRole:
            return QVariant::fromValue<qint64>( bookmark.i_time );
        case BookmarkRoles::PositionRole:
        {
            vlc_player_locker lock{ m_player };
            return !vlc_player_GetLength( m_player ) ? QVariant{} :
                    QVariant::fromValue<float> ((float) VLC_TICK_FROM_MS(bookmark.i_time) / vlc_player_GetLength( m_player ));
        }
        case BookmarkRoles::DescriptionRole:
            return QVariant::fromValue( qfu( bookmark.psz_description ) );
        default:
            return QVariant{};
    }
}

bool MLBookmarkModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if ( !isValidBookmarkIndex(index) || !canAdd() )
        return false;
    if ( role == Qt::EditRole )
        role = columnToRole(index.column());
    if ( role != BookmarkRoles::NameRole && role != BookmarkRoles::DescriptionRole )
        return false;
    if ( value.canConvert<QString>() == false )
        return false;

    size_t row = index.row();
    bool updateName = (role == BookmarkRoles::NameRole);

    auto& b = m_bookmarks->p_items[row];
    auto str = value.toString();

    struct Ctx {
        bool updateSucceed = false;
    };
    m_mediaLib->runOnMLThread<Ctx>(this,
    //ML thread
    [mediaId = b.i_media_id, bookmarkTime = b.i_time, updateName, str]
    (vlc_medialibrary_t* ml, Ctx& ctx)
    {
        int ret;
        if ( updateName )
            ret = vlc_ml_media_update_bookmark( ml, mediaId, bookmarkTime, qtu( str ), nullptr );
        else
            ret = vlc_ml_media_update_bookmark( ml, mediaId, bookmarkTime, nullptr, qtu( str ) );
        ctx.updateSucceed = (ret == VLC_SUCCESS);
    },
    //UI thread
    [this, updateName, mediaId = m_currentMediaId,
     revision = m_mediaIdRevision, bookmarkTime = b.i_time, str]
    (quint64, Ctx& ctx)
    {
        if (!ctx.updateSucceed)
            return;

        if (m_currentMediaId != mediaId || !isCurrentRevision(revision) || !m_bookmarks)
            return;

        // A deletion, refresh or sort may have changed the row while the edit
        // ran on the ML thread. Locate the original bookmark by its identity.
        size_t row = 0;
        for (; row < m_bookmarks->i_nb_items; ++row)
            if (m_bookmarks->p_items[row].i_time == bookmarkTime)
                break;
        if (row == m_bookmarks->i_nb_items)
            return;
        auto& b = m_bookmarks->p_items[row];
        if (updateName)
        {
            free( b.psz_name );
            b.psz_name = strdup( qtu( str ) );
            emit dataChanged(this->index(row, 0), this->index(row, 0),
                             {Qt::DisplayRole, BookmarkRoles::NameRole});
        }
        else
        {
            free( b.psz_description );
            b.psz_description = strdup( qtu( str ) );
            emit dataChanged(this->index(row, 2), this->index(row, 2),
                             {Qt::DisplayRole, BookmarkRoles::DescriptionRole});
        }
    });

    return true;
}

QHash<int, QByteArray> MLBookmarkModel::roleNames() const{
    return QHash<int, QByteArray>{
        {BookmarkRoles::NameRole, QByteArrayLiteral("name")},
        {BookmarkRoles::TimeRole, QByteArrayLiteral("time")},
        {BookmarkRoles::PositionRole, QByteArrayLiteral("position")},
        {BookmarkRoles::DescriptionRole, QByteArrayLiteral("description")},
        {BookmarkRoles::TimeMsRole, QByteArrayLiteral("timeMs")}
    };
}

int MLBookmarkModel::columnToRole(int column) const{
    switch (column)
    {
      case 0:
         return BookmarkRoles::NameRole;
      case 1:
         return BookmarkRoles::TimeRole;
      case 2:
        return BookmarkRoles::DescriptionRole;
      default:
         return -1;
    }
}

Qt::ItemFlags MLBookmarkModel::flags( const QModelIndex& index ) const
{
    if (!isValidBookmarkIndex(index))
        return Qt::NoItemFlags;
    auto f = QAbstractItemModel::flags( index );
    if ( index.column() != 1 )
        f |= Qt::ItemIsEditable;
    return f;
}

int MLBookmarkModel::rowCount(const QModelIndex &parent) const
{
    if ( parent.isValid() || m_bookmarks == nullptr )
        return 0;
    return m_bookmarks->i_nb_items;
}

int MLBookmarkModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 3;
}

QModelIndex MLBookmarkModel::index( int row, int column, const QModelIndex& parent ) const
{
    if (parent.isValid() || row < 0 || row >= rowCount() ||
        column < 0 || column >= columnCount())
        return {};
    return createIndex( row, column, nullptr );
}

QModelIndex MLBookmarkModel::parent(const QModelIndex &) const
{
    return {};
}

QVariant MLBookmarkModel::headerData( int section, Qt::Orientation orientation,
                                      int role ) const
{
    if (role != Qt::DisplayRole || orientation == Qt::Vertical)
        return QVariant{};

    switch ( section )
    {
        case 0:
            return QVariant::fromValue( qtr( "Name" ) );
        case 1:
            return QVariant::fromValue( qtr( "Time" ) );
        case 2:
            return QVariant::fromValue( qtr( "Description" ) );
        default:
            return QVariant{};
    }
}

void MLBookmarkModel::sort( int column, Qt::SortOrder order )
{
    switch ( column )
    {
        case 0:
            m_sort = VLC_ML_SORTING_ALPHA;
            break;
        case 1:
            // For bookmarks, default sort order is their time in the media
            m_sort = VLC_ML_SORTING_DEFAULT;
            break;
        case 2:
            // We don't support ordering by description, just ignore it
            return;
        default:
            // In all other cases, just list the items as they are inserted
            m_sort = VLC_ML_SORTING_INSERTIONDATE;
            break;
    }
    m_desc = order == Qt::DescendingOrder ? true : false;
    refresh( MLBOOKMARKMODEL_REFRESH );
}

bool MLBookmarkModel::isCurrentRevision(uint64_t revision) const
{
    vlc::threads::mutex_locker lock{ m_mutex };
    return m_revision == revision;
}

bool MLBookmarkModel::canAdd() const
{
    return m_player && m_mediaLib && m_currentMediaId != 0 &&
           isCurrentRevision(m_mediaIdRevision);
}

bool MLBookmarkModel::isValidBookmarkIndex(const QModelIndex& index) const
{
    return index.isValid() && index.model() == this && m_bookmarks &&
           index.row() >= 0 && (uint32_t)index.row() < m_bookmarks->i_nb_items &&
           index.column() >= 0 && index.column() < columnCount();
}

void MLBookmarkModel::resetBookmarks()
{
    ++m_refreshRevision;
    beginResetModel();
    m_bookmarks.reset();
    endResetModel();
}

void MLBookmarkModel::invalidateMediaId()
{
    m_currentMediaId = 0;
    ++m_mediaIdLookupRevision;
    m_mediaIdLookupPending = false;
    m_retryMediaIdLookup = false;
    resetBookmarks();
    // Always notify: the player thread may already have invalidated the
    // revision while a QML binding still contains its previous ready state.
    emit canAddChanged();
}

void MLBookmarkModel::retryMediaId()
{
    if (m_currentMediaId != 0 || !m_player || !m_mediaLib)
        return;
    if (m_mediaIdLookupPending)
    {
        // Coalesce indexing events while a lookup is already running.
        m_retryMediaIdLookup = true;
        return;
    }

    QString mediaUri;
    uint64_t revision;
    {
        vlc::threads::mutex_locker lock{ m_mutex };
        revision = m_revision;
        if (m_currentItem)
            mediaUri = m_currentItem->psz_uri;
    }
    if (!mediaUri.isEmpty())
        updateMediaId(revision, mediaUri);
}

void MLBookmarkModel::add()
{
    if (!canAdd())
        return;

    const uint64_t mediaId = m_currentMediaId;
    const uint64_t revision = m_mediaIdRevision;
    vlc_tick_t currentTime;
    {
        vlc_player_locker lock{ m_player };
        // Taking the player lock first matches the listener's lock order.
        // Reject a media switch that has not yet reached the GUI callback.
        if (!isCurrentRevision(revision))
            return;
        currentTime = vlc_player_GetTime( m_player );
    }

    if (currentTime == VLC_TICK_INVALID)
        return;

    m_mediaLib->runOnMLThread(this,
    //ML thread
    [mediaId, currentTime](vlc_medialibrary_t* ml)
    {
        int64_t time = MS_FROM_VLC_TICK(currentTime);

        vlc_ml_media_add_bookmark(ml, mediaId, time);

        ml_unique_ptr<vlc_ml_media_t> media { vlc_ml_get_media(ml, mediaId) };

        if (media)
        {
            QString name = qtr("Bookmark at %1").arg(VLCDuration::fromMS( time ).formatHMS());

            vlc_ml_media_update_bookmark(ml, mediaId, time, qtu(name), nullptr);
        }
    },
    //UI thread
    [this, mediaId, revision](){
        if (m_currentMediaId == mediaId && isCurrentRevision(revision))
            refresh( MLBOOKMARKMODEL_REFRESH );
    });
}

void MLBookmarkModel::remove( const QModelIndexList &indexes )
{
    if (!canAdd() || !m_bookmarks)
        return;

    std::vector<int64_t> bookmarkTimeList;
    for ( const auto& i : indexes )
    {
        if (!isValidBookmarkIndex(i))
            continue;
        auto& b = m_bookmarks->p_items[i.row()];
        bookmarkTimeList.push_back(b.i_time);
    }

    // Native table selection includes one index per column of each row.
    std::sort(bookmarkTimeList.begin(), bookmarkTimeList.end());
    bookmarkTimeList.erase(std::unique(bookmarkTimeList.begin(), bookmarkTimeList.end()),
                           bookmarkTimeList.end());
    if (bookmarkTimeList.empty())
        return;

    m_mediaLib->runOnMLThread(this,
    //ML thread
    [mediaId = m_currentMediaId, bookmarkTimeList]
    (vlc_medialibrary_t* ml)
    {
        for (int64_t bookmarkTime : bookmarkTimeList)
            vlc_ml_media_remove_bookmark(ml, mediaId, bookmarkTime);
    },
    //UI thread
    [this, mediaId = m_currentMediaId, revision = m_mediaIdRevision](){
        if (m_currentMediaId == mediaId && isCurrentRevision(revision))
            refresh( MLBOOKMARKMODEL_REFRESH );
    });
}

void MLBookmarkModel::clear()
{
    if (!canAdd())
        return;

    m_mediaLib->runOnMLThread(this,
    //ML thread
    [mediaId = m_currentMediaId](vlc_medialibrary_t* ml)
    {
        vlc_ml_media_remove_all_bookmarks( ml, mediaId );
    },
    //UI thread
    [this, mediaId = m_currentMediaId, revision = m_mediaIdRevision]()
    {
        if (mediaId == m_currentMediaId && isCurrentRevision(revision))
        {
            resetBookmarks();
        }
    });
}

void MLBookmarkModel::select(const QModelIndex &index)
{
    if (!isValidBookmarkIndex(index) || !canAdd())
        return;
    const auto& b = m_bookmarks->p_items[index.row()];
    vlc_player_locker lock{ m_player };
    if (!isCurrentRevision(m_mediaIdRevision))
        return;
    vlc_player_SetTime( m_player, VLC_TICK_FROM_MS( b.i_time ) );
}

void MLBookmarkModel::onCurrentMediaChanged( vlc_player_t* player, input_item_t* media,
                                             void *data )
{
    //Player thread
    auto self = static_cast<MLBookmarkModel*>( data );
    SharedInputItem item(media, true);
    // The listener runs with the player lock held. A continuous media change
    // need not emit STARTED again, so recover the state for this media rather
    // than relying on a queued state callback from the previous revision.
    const auto state = vlc_player_GetState(player);
    const bool clearBookmarks = state == VLC_PLAYER_STATE_STOPPING || state == VLC_PLAYER_STATE_STOPPED;
    QString mediaUri;
    uint64_t revision;
    {
        vlc::threads::mutex_locker lock{ self->m_mutex };
        self->m_currentItem.reset( media ? input_item_Hold( media ) : nullptr );
        revision = ++self->m_revision;
        if (media)
            mediaUri = media->psz_uri;
    }

    //UI Thread
    QMetaObject::invokeMethod(self,
    [self, revision, mediaUri, clearBookmarks]()
    {
        {
            vlc::threads::mutex_locker lock{ self->m_mutex };
            //did we start playing a new media in between
            if (self->m_revision != revision)
                return;
        }
        self->m_clearBookmarks = clearBookmarks;
        self->updateMediaId(revision, mediaUri);
    }, Qt::QueuedConnection);
}

void MLBookmarkModel::onPlaybackStateChanged( vlc_player_t *, vlc_player_state state,
                                              void *data )
{
    auto self = static_cast<MLBookmarkModel*>( data );

    uint64_t revision;
    {
        vlc::threads::mutex_locker lock{ self->m_mutex };
        revision = self->m_revision;
    }

    QMetaObject::invokeMethod(self, [self, state, revision](){
        if (!self->isCurrentRevision(revision))
            return;
        if ( state == VLC_PLAYER_STATE_STARTED )
        {
            self->m_clearBookmarks = false;
            self->refresh( MLBOOKMARKMODEL_REFRESH );
        }
        else if ( state == VLC_PLAYER_STATE_STOPPING || state == VLC_PLAYER_STATE_STOPPED )
        {
            // Retain the resolved database ID for a same-media restart.
            self->m_clearBookmarks = true;
            self->refresh( MLBOOKMARKMODEL_CLEAR );
        }
    }, Qt::QueuedConnection);
}


void MLBookmarkModel::updateMediaId(uint64_t revision, const QString &mediaUri)
{
    if (!isCurrentRevision(revision))
        return;

    invalidateMediaId();
    if (!m_player || !m_mediaLib || mediaUri.isEmpty())
        return;
    const uint64_t lookupRevision = m_mediaIdLookupRevision;
    m_mediaIdLookupPending = true;

    //retrieve the media id in the medialib
    struct Ctx{
        uint64_t newMLid = 0;
        BookmarkListPtr newBookmarks;
    };
    m_mediaLib->runOnMLThread<Ctx>(this,
    //ML thread
    [mediaUri, sort = m_sort, desc = m_desc](vlc_medialibrary_t* ml, Ctx& ctx){

        auto mlMedia = vlc_ml_get_media_by_mrl( ml, qtu(mediaUri) );
        if ( mlMedia != nullptr )
        {
            ctx.newMLid = mlMedia->i_id;
            vlc_ml_release( mlMedia );
        }
        vlc_ml_query_params_t params = vlc_ml_query_params_create();
        params.i_sort = sort;
        params.b_desc = desc;
        if (ctx.newMLid != 0)
            ctx.newBookmarks.reset( vlc_ml_list_media_bookmarks( ml, &params, ctx.newMLid ) );
    },
    //UI thread
    [this, revision, lookupRevision, sort = m_sort, desc = m_desc](quint64, Ctx& ctx) {
        if (!m_player || !m_mediaLib || !isCurrentRevision(revision) ||
            m_mediaIdLookupRevision != lookupRevision)
            return;

        const bool retry = ctx.newMLid == 0 && m_retryMediaIdLookup;
        m_mediaIdLookupPending = false;
        m_retryMediaIdLookup = false;
        ++m_refreshRevision;
        beginResetModel();
        if (!m_clearBookmarks)
            m_bookmarks = std::move(ctx.newBookmarks);
        m_currentMediaId = ctx.newMLid;
        m_mediaIdRevision = revision;
        endResetModel();
        emit canAddChanged();

        if (retry)
            retryMediaId();
        else if (ctx.newMLid != 0 && (sort != m_sort || desc != m_desc))
            refresh(MLBOOKMARKMODEL_REFRESH);
    });
}

void MLBookmarkModel::refresh(MLBookmarkModel::RefreshOperation forceClear )
{
    if (!canAdd() || m_clearBookmarks || forceClear == MLBOOKMARKMODEL_CLEAR)
    {
        resetBookmarks();
    }
    else
    {
        uint64_t mediaId = m_currentMediaId;
        const uint64_t revision = m_mediaIdRevision;
        const uint64_t refreshRevision = ++m_refreshRevision;
        struct Ctx
        {
            BookmarkListPtr newBookmarks;
        };
        m_mediaLib->runOnMLThread<Ctx>(this,
        //ML thread
        [mediaId, sort = m_sort, desc = m_desc]
        (vlc_medialibrary_t* ml, Ctx& ctx) {
            vlc_ml_query_params_t params = vlc_ml_query_params_create();
            params.i_sort = sort;
            params.b_desc = desc;
            ctx.newBookmarks.reset( vlc_ml_list_media_bookmarks( ml, &params, mediaId ) );
        },
        //UI thread
        [this, mediaId, revision, refreshRevision](quint64, Ctx& ctx)
        {
            if (m_currentMediaId != mediaId || !isCurrentRevision(revision) ||
                m_refreshRevision != refreshRevision)
                return;
            beginResetModel();
            m_bookmarks = std::move(ctx.newBookmarks);
            endResetModel();
        });
    }
}

vlc_player_t * MLBookmarkModel::player() const
{
    return m_player;
}

void MLBookmarkModel::setPlayer(vlc_player_t * player)
{
    if (m_player == player) return;

    if(m_player && m_listener){
        vlc_player_locker lock{ m_player };
        vlc_player_RemoveListener( m_player, m_listener );
        m_listener = nullptr;
    }

    m_player = player;
    {
        vlc::threads::mutex_locker lock{ m_mutex };
        ++m_revision;
        m_currentItem.reset();
    }
    invalidateMediaId();

    if (m_player && m_mediaLib)
        initModel();
}


PlayerController * MLBookmarkModel::playerController() const
{
    return m_player_controller;
}

void MLBookmarkModel::setPlayer(PlayerController * player)
{
    if (m_player_controller == player) return;

    if (m_player_controller)
        disconnect(m_player_controller, nullptr, this, nullptr);

    m_player_controller = player;

    if (m_player_controller)
        connect(m_player_controller, &PlayerController::lengthChanged, this, &MLBookmarkModel::playerLengthChanged);

    setPlayer(player ? player->getPlayer() : nullptr);
}

MediaLib* MLBookmarkModel::ml() const
{
    return m_mediaLib;
}

void MLBookmarkModel::playerLengthChanged()
{
    refresh(MLBOOKMARKMODEL_REFRESH);
}

void MLBookmarkModel::setMl(MediaLib* medialib)
{
    if(m_mediaLib == medialib) return;

    if(m_mediaLib)
        m_ml_event_handle.reset(nullptr);

    m_mediaLib = medialib;
    {
        vlc::threads::mutex_locker lock{ m_mutex };
        ++m_revision;
    }
    invalidateMediaId();

    if (m_player && m_mediaLib)
        initModel();
}

void MLBookmarkModel::initModel()
{
    if (!m_player || !m_mediaLib)
        return;

    static const vlc_player_cbs cbs = []{
        vlc_player_cbs cbs {};
        cbs.on_current_media_changed = &onCurrentMediaChanged;
        cbs.on_state_changed = &onPlaybackStateChanged;
        return cbs;
    }();
    QString uri;
    uint64_t revision;
    {
        vlc_player_locker lock{ m_player };
        vlc::threads::mutex_locker selflock{ m_mutex };
        if (!m_listener)
        {
            m_listener = vlc_player_AddListener( m_player, &cbs, this );
            if ( m_listener == nullptr )
                throw std::bad_alloc{};
        }
        revision = ++m_revision;
        auto currentItem = vlc_player_GetCurrentMedia( m_player );
        m_currentItem = vlc::wrap_cptr( currentItem ? input_item_Hold( currentItem ) : nullptr,
                                        &input_item_Release );
        if (m_currentItem)
        {
            uri = m_currentItem->psz_uri;
        }
        const auto state = vlc_player_GetState(m_player);
        m_clearBookmarks = state == VLC_PLAYER_STATE_STOPPING || state == VLC_PLAYER_STATE_STOPPED;
    }
    // Subscribe before the asynchronous lookup: indexing can finish between
    // an ID-zero read and completion, and media-added events are not replayed.
    if (m_ml_event_handle == nullptr)
        m_ml_event_handle.reset(m_mediaLib->registerEventListener(onVlcMlEvent, this));

    updateMediaId(revision, uri);
}

void MLBookmarkModel::onVlcMlEvent(void* data, const vlc_ml_event_t* event)
{
    auto self = static_cast<MLBookmarkModel*>(data);
    auto  mlEvent = std::make_shared<MLEvent>(event);
    uint64_t revision;
    {
        vlc::threads::mutex_locker lock{ self->m_mutex };
        revision = self->m_revision;
    }
    QMetaObject::invokeMethod(self, [self, mlEvent, revision] () mutable {
        if (self->isCurrentRevision(revision))
            self->onVlcMlEvent(*mlEvent);
    }, Qt::QueuedConnection);
}

void MLBookmarkModel::onVlcMlEvent(const MLEvent &event){
    switch (event.i_type)
    {
        case VLC_ML_EVENT_MEDIA_ADDED:
        case VLC_ML_EVENT_MEDIA_UPDATED:
            retryMediaId();
            break;
        case VLC_ML_EVENT_MEDIA_DELETED:
            if (m_currentMediaId != 0 &&
                m_currentMediaId == (uint64_t)event.deletion.i_entity_id)
                invalidateMediaId();
            break;
        case VLC_ML_EVENT_BOOKMARKS_ADDED:
        case VLC_ML_EVENT_BOOKMARKS_DELETED:
        case VLC_ML_EVENT_BOOKMARKS_UPDATED:
            refresh(MLBOOKMARKMODEL_REFRESH);
        break;
        default:
            break;
    }
}
