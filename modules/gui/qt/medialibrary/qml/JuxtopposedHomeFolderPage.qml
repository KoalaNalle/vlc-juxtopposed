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

Widgets.PageExt {
    id: root
    property alias parentId: media.parentId
    title: qsTr("Folder")
    hasGridListMode: true
    isSearchable: true
    libraryModel: media
    sortModel: [
        { text: qsTr("Alphabetic"), criteria: "title" },
        { text: qsTr("Duration"), criteria: "duration" }
    ]
    function positionContentAtBeginning() { view.positionContentAtBeginning() }
    function setCurrentItemFocus(reason) { view.setCurrentItemFocus(reason) }
    MLMediaModel {
        id: media
        ml: MediaLib
        searchPattern: root.search.pattern
        sortCriteria: root.sort.criteria || "title"
        sortOrder: root.sort.order
    }
    Component.onCompleted: MediaLib.homeActivity.recordVisit(parentId)
    contentItem: JuxtopposedHomeCollectionView {
        id: view
        entries: media
        kind: "media"
        gridMode: MainCtx.gridView
        uiScale: VLCStyle.dp(1, VLCStyle.scale)
        iconFontFamily: VLCIcons.fontFamily
        fallbackIcon: VLCIcons.topbar_video
        loading: media.loading
        emptyText: root.search.pattern ? qsTr("No matching media") : qsTr("This folder has no available media.")
        Navigation.parentItem: root
        Navigation.upItem: root.header
        onActivated: (entry, kind, reason) => {
            MediaLib.addAndPlay(entry.id, kind === "video" && entry.progress > 0 ? [":restore-playback-pos=2"] : [])
            MainCtx.playerView = true
        }
    }
}
