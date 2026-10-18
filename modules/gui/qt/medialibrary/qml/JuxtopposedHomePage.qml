/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * Visual design by Juxtopposed: https://www.youtube.com/@juxtopposed
 * Code licensed under GPLv2 or later.
 *****************************************************************************/
import QtQuick
import VLC.MediaLibrary
import VLC.MainInterface
import VLC.Dialogs
import VLC.Widgets as Widgets
import VLC.Style
import VLC.Util
import VLC.Menus
import "JuxtopposedHomeSorting.js" as HomeSorting

Widgets.PageExt {
    id: root
    title: qsTr("Home")
    hasGridListMode: true
    isSearchable: true
    libraryItemCount: MediaLib.homeActivity.mediaCount
    sortModel: [
        { text: qsTr("Recent activity"), criteria: "activity" },
        { text: qsTr("Alphabetic"), criteria: "title" }
    ]
    signal seeAllButtonClicked(string category, int reason)
    signal collectionRequested(string kind, var entry, int reason)
    function positionContentAtBeginning() { dashboard.positionContentAtBeginning() }
    function setCurrentItemFocus(reason) { dashboard.setCurrentItemFocus(reason) }
    function filteredActivity(entries) {
        return HomeSorting.activityEntries(entries, root.search.pattern, root.sort.criteria, root.sort.order)
    }
    StringListMenu {
        id: pinMenu
        ctx: MainCtx
        property var itemId
        onSelected: (index, _) => { if (index === 0) MainCtx.sidebarPins.pinItem(itemId) }
    }
    MLRecentVideoModel {
        id: videos
        ml: MediaLib
        limit: 4
        searchPattern: root.search.pattern
        sortCriteria: HomeSorting.mediaCriteria(root.sort.criteria)
        sortOrder: HomeSorting.mediaOrder(root.sort.criteria, root.sort.order)
    }
    MLRecentAudioModel {
        id: audio
        ml: MediaLib
        limit: 4
        searchPattern: root.search.pattern
        sortCriteria: HomeSorting.mediaCriteria(root.sort.criteria)
        sortOrder: HomeSorting.mediaOrder(root.sort.criteria, root.sort.order)
    }
    contentItem: JuxtopposedHomeDashboard {
        id: dashboard
        uiScale: VLCStyle.dp(1, VLCStyle.scale)
        iconFontFamily: VLCIcons.fontFamily
        arrowIcon: VLCIcons.ic_fluent_chevron_right_24
        videoIcon: VLCIcons.topbar_video
        audioIcon: VLCIcons.topbar_music
        folderIcon: VLCIcons.topbar_discover
        playlistIcon: VLCIcons.playlist
        artistIcon: VLCIcons.topbar_music
        folderArtwork: "qrc:/menu/folder.svg"
        artistArtwork: VLCStyle.noArtArtist
        videoEntries: videos
        audioEntries: audio
        folderEntries: root.filteredActivity(MediaLib.homeActivity.folders)
        playlistEntries: root.filteredActivity(MediaLib.homeActivity.playlists)
        artistEntries: root.filteredActivity(MediaLib.homeActivity.artists)
        historyLoading: videos.loading || audio.loading
        activityLoading: MediaLib.homeActivity.loading
        libraryBusy: !MediaLib.idle
        mediaEmpty: !MediaLib.homeActivity.loading && MediaLib.homeActivity.mediaCount === 0
        errorText: MediaLib.homeActivity.error
        gridMode: MainCtx.gridView
        searchPattern: root.search.pattern
        Navigation.parentItem: root
        Navigation.upItem: root.header
        onMediaRequested: (entry, resume, reason) => {
            MediaLib.addAndPlay(entry.id, resume ? [":restore-playback-pos=2"] : [])
            MainCtx.playerView = true
        }
        onFolderRequested: (entry, reason) => root.collectionRequested("folder", entry, reason)
        onPlaylistRequested: (entry, reason) => root.collectionRequested("playlist", entry, reason)
        onArtistRequested: (entry, reason) => root.collectionRequested("artist", entry, reason)
        onSeeAllRequested: (category, reason) => root.seeAllButtonClicked(category, reason)
        onAddFolderRequested: DialogsProvider.addMediaLibraryFolder()
        onRetryRequested: MediaLib.homeActivity.refresh()
        onPinMenuRequested: (entry, position) => {
            if (entry?.id && !MainCtx.sidebarPins.canRetry) {
                pinMenu.itemId = entry.id
                pinMenu.popup(position, [qsTr("Pin to sidebar")])
            }
        }
    }
}
