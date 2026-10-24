/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * Visual design by Juxtopposed: https://www.youtube.com/@juxtopposed
 * Code licensed under GPLv2 or later.
 *****************************************************************************/
import QtQuick
import VLC.MediaLibrary
import VLC.MainInterface
import VLC.Widgets as Widgets
import VLC.Style
import VLC.Util
import "JuxtopposedHomeSorting.js" as HomeSorting

Widgets.PageExt {
    id: root
    property bool isVideo: true
    title: isVideo ? qsTr("Continue Watching") : qsTr("Listen Again")
    hasGridListMode: true
    isSearchable: true
    libraryModel: isVideo ? videos : audio
    sortModel: [
        { text: qsTr("Recent activity"), criteria: "activity" },
        { text: qsTr("Alphabetic"), criteria: "title" },
        { text: qsTr("Duration"), criteria: "duration" }
    ]
    function positionContentAtBeginning() { view.positionContentAtBeginning() }
    function setCurrentItemFocus(reason) { view.setCurrentItemFocus(reason) }
    MLRecentVideoModel {
        id: videos
        ml: MediaLib
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
        emptyText: root.search.pattern ? qsTr("No matching media") : qsTr("No playback history yet")
        Navigation.parentItem: root
        Navigation.upItem: root.header
        onActivated: (entry, kind, reason) => {
            MediaLib.addAndPlay(entry.id, root.isVideo && entry.progress > 0 ? [":restore-playback-pos=2"] : [])
            MainCtx.playerView = true
        }
    }
}
