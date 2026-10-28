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
    property string historyMessage: ""
    function positionContentAtBeginning() { dashboard.positionContentAtBeginning() }
    function setCurrentItemFocus(reason) { dashboard.setCurrentItemFocus(reason) }
    function filteredActivity(entries) {
        return HomeSorting.activityEntries(entries, root.search.pattern, root.sort.criteria, root.sort.order)
    }
    StringListMenu {
        id: pinMenu
        ctx: MainCtx
        property var itemId
        property var historyModel: null
        property int pinIndex: -1
        property int removeIndex: -1
        onSelected: (index, _) => {
            if (index === pinIndex) MainCtx.sidebarPins.pinItem(itemId)
            else if (index === removeIndex && !historyModel.historyBusy) {
                root.historyMessage = ""
                historyModel.removeFromHistory(itemId)
            }
        }
    }
    function showItemMenu(entry, position, historyModel = null) {
        if (!entry?.id) return
        const actions = []
        pinMenu.itemId = entry.id
        pinMenu.historyModel = historyModel
        pinMenu.pinIndex = -1
        pinMenu.removeIndex = -1
        if (!MainCtx.sidebarPins.canRetry) {
            pinMenu.pinIndex = actions.length
            actions.push(qsTr("Pin to sidebar"))
        }
        if (historyModel && !historyModel.historyBusy) {
            pinMenu.removeIndex = actions.length
            actions.push(qsTr("Remove from this list"))
        }
        if (actions.length) pinMenu.popup(position, actions)
    }
    StringListMenu {
        id: historyMenu
        ctx: MainCtx
        property var historyModel
        property string sectionTitle
        onSelected: (index, _) => {
            if (index !== 0 || historyModel.historyBusy) return
            if (DialogsProvider.questionDialog(qsTr("Clear all items from %1? This includes items hidden by search. Your media files will be kept.").arg(sectionTitle))) {
                root.historyMessage = ""
                historyModel.clearHistory()
            }
        }
    }
    Connections {
        target: videos
        function onHistoryEditFinished(success, error) { root.historyMessage = success ? "" : error }
    }
    Connections {
        target: watchedVideos
        function onHistoryEditFinished(success, error) { root.historyMessage = success ? "" : error }
    }
    MLRecentVideoModel {
        id: videos
        ml: MediaLib
        historyMode: MLRecentVideoModel.Unfinished
        limit: 4
        searchPattern: root.search.pattern
        sortCriteria: HomeSorting.mediaCriteria(root.sort.criteria)
        sortOrder: HomeSorting.mediaOrder(root.sort.criteria, root.sort.order)
    }
    MLRecentVideoModel {
        id: watchedVideos
        ml: MediaLib
        historyMode: MLRecentVideoModel.Completed
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
        moreIcon: VLCIcons.more
        videoIcon: VLCIcons.topbar_video
        audioIcon: VLCIcons.topbar_music
        folderIcon: VLCIcons.topbar_discover
        playlistIcon: VLCIcons.playlist
        artistIcon: VLCIcons.topbar_music
        folderArtwork: "qrc:/menu/folder.svg"
        artistArtwork: VLCStyle.noArtArtist
        videoEntries: videos
        watchAgainEntries: watchedVideos
        audioEntries: audio
        folderEntries: root.filteredActivity(MediaLib.homeActivity.folders)
        playlistEntries: root.filteredActivity(MediaLib.homeActivity.playlists)
        artistEntries: root.filteredActivity(MediaLib.homeActivity.artists)
        historyLoading: videos.loading || watchedVideos.loading || audio.loading
        historyBusy: videos.historyBusy || watchedVideos.historyBusy
        historyMessage: root.historyMessage
        activityLoading: MediaLib.homeActivity.loading
        libraryBusy: !MediaLib.idle
        mediaEmpty: !MediaLib.homeActivity.loading && MediaLib.homeActivity.mediaCount === 0
        errorText: MediaLib.homeActivity.error
        gridMode: MainCtx.gridView
        searchPattern: root.search.pattern
        Navigation.parentItem: root
        Navigation.upItem: root.header
        onMediaRequested: (entry, resume, reason) => {
            MediaLib.addAndPlay(entry.id, resume ? [":restore-playback-pos=2"] : entry.isVideo ? [":restore-playback-pos=0"] : [])
            MainCtx.playerView = true
        }
        onFolderRequested: (entry, reason) => root.collectionRequested("folder", entry, reason)
        onPlaylistRequested: (entry, reason) => root.collectionRequested("playlist", entry, reason)
        onArtistRequested: (entry, reason) => root.collectionRequested("artist", entry, reason)
        onSeeAllRequested: (category, reason) => root.seeAllButtonClicked(category, reason)
        onAddFolderRequested: DialogsProvider.addMediaLibraryFolder()
        onRetryRequested: MediaLib.homeActivity.refresh()
        onPinMenuRequested: (entry, position) => root.showItemMenu(entry, position)
        onVideoMenuRequested: (entry, category, position) => root.showItemMenu(entry, position,
            category === "watchAgain" ? watchedVideos : videos)
        onHistoryMenuRequested: (category, position) => {
            historyMenu.historyModel = category === "watchAgain" ? watchedVideos : videos
            historyMenu.sectionTitle = category === "watchAgain" ? qsTr("Watch Again") : qsTr("Continue Watching")
            if (!historyMenu.historyModel.historyBusy)
                historyMenu.popup(position, [qsTr("Clear all items")])
        }
    }
}
