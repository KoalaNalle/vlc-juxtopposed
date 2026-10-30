/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * Visual design by Juxtopposed: https://www.youtube.com/@juxtopposed
 * Code licensed under GPLv2 or later.
 *****************************************************************************/
import QtQuick
import QtQuick.Controls.Basic as C
import VLC.MediaLibrary
import VLC.MainInterface
import VLC.Dialogs
import VLC.Menus
import VLC.Widgets as Widgets
import VLC.Style
import VLC.Util
import "JuxtopposedHomeSorting.js" as HomeSorting

Widgets.PageExt {
    id: root
    property bool isVideo: true
    property bool completedVideos: false
    property string historyMessage: ""
    title: isVideo ? (completedVideos ? qsTr("Watch Again") : qsTr("Continue Watching")) : qsTr("Listen Again")
    hasGridListMode: true
    isSearchable: true
    libraryModel: isVideo ? videos : audio
    libraryMoreMenu: isVideo ? historyHeaderMenu : null
    sortModel: [
        { text: qsTr("Recent activity"), criteria: "activity" },
        { text: qsTr("Alphabetic"), criteria: "title" },
        { text: qsTr("Duration"), criteria: "duration" }
    ]
    function positionContentAtBeginning() { view.positionContentAtBeginning() }
    function setCurrentItemFocus(reason) { view.setCurrentItemFocus(reason) }
    QtObject {
        id: historyHeaderMenu
        function popup(position) {
            if (!videos.historyBusy) clearMenu.popup(position, [qsTr("Clear all items")])
        }
    }
    StringListMenu {
        id: clearMenu
        ctx: MainCtx
        onSelected: (index, _) => {
            if (index !== 0 || videos.historyBusy) return
            if (DialogsProvider.questionDialog(qsTr("Clear all items from %1? This includes items hidden by search. Your media files will be kept.").arg(root.title))) {
                root.historyMessage = ""
                videos.clearHistory()
            }
        }
    }
    StringListMenu {
        id: itemMenu
        ctx: MainCtx
        property var itemId
        onSelected: (index, _) => {
            if (index === 0 && !videos.historyBusy) {
                root.historyMessage = ""
                videos.removeFromHistory(itemId)
            }
        }
    }
    Connections {
        target: videos
        function onHistoryEditFinished(success, error) { root.historyMessage = success ? "" : error }
    }
    footer: C.Label {
        objectName: "homeHistoryStatus"
        visible: root.isVideo && (videos.historyBusy || root.historyMessage.length > 0)
        padding: VLCStyle.dp(12, VLCStyle.scale)
        text: videos.historyBusy ? qsTr("Updating playback history…") : root.historyMessage
        wrapMode: Text.WordWrap
        color: JuxtopposedPalette.color("#969696")
    }
    MLRecentVideoModel {
        id: videos
        ml: MediaLib
        historyMode: root.completedVideos ? MLRecentVideoModel.Completed : MLRecentVideoModel.Unfinished
        searchPattern: root.search.pattern
        sortCriteria: HomeSorting.mediaCriteria(root.sort.criteria)
        sortOrder: HomeSorting.mediaOrder(root.sort.criteria, root.sort.order)
    }
    MLRecentAudioModel {
        id: audio
        ml: MediaLib
        searchPattern: root.search.pattern
        sortCriteria: HomeSorting.mediaCriteria(root.sort.criteria)
        sortOrder: HomeSorting.mediaOrder(root.sort.criteria, root.sort.order)
    }
    contentItem: JuxtopposedHomeCollectionView {
        id: view
        entries: root.libraryModel
        kind: root.isVideo ? "video" : "audio"
        gridMode: MainCtx.gridView
        uiScale: VLCStyle.dp(1, VLCStyle.scale)
        iconFontFamily: VLCIcons.fontFamily
        fallbackIcon: root.isVideo ? VLCIcons.topbar_video : VLCIcons.topbar_music
        loading: root.libraryModel.loading
        historyActionsAvailable: root.isVideo
        emptyText: root.search.pattern ? qsTr("No matching media")
            : root.isVideo ? (root.completedVideos ? qsTr("No completed videos yet") : qsTr("No unfinished videos"))
            : qsTr("No playback history yet")
        Navigation.parentItem: root
        Navigation.upItem: root.header
        onActivated: (entry, kind, reason) => {
            MediaLib.addAndPlay(entry.id, root.isVideo ? [root.completedVideos ? ":restore-playback-pos=0" : ":restore-playback-pos=2"] : [])
            MainCtx.playerView = true
        }
        onContextMenuRequested: (entry, position) => {
            if (root.isVideo && entry?.id && !videos.historyBusy) {
                itemMenu.itemId = entry.id
                itemMenu.popup(position, [qsTr("Remove from this list")])
            }
        }
    }
}
