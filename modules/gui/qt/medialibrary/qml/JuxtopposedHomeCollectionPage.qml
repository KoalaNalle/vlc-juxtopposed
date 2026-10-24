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
    property string category: "frequentFolders"
    readonly property string kind: category === "frequentPlaylists" ? "playlist" : category === "frequentArtists" ? "artist" : "folder"
    title: category === "frequentPlaylists" ? qsTr("Frequently Visited Playlists") :
           category === "frequentArtists" ? qsTr("Frequently Listened Artists") : qsTr("Frequently Visited Folders")
    hasGridListMode: true
    isSearchable: true
    libraryItemCount: view.count
    sortModel: [
        { text: qsTr("Frequent activity"), criteria: "activity" },
        { text: qsTr("Alphabetic"), criteria: "title" }
    ]
    signal collectionRequested(string kind, var entry, int reason)
    readonly property var entries: {
        const activity = MediaLib.homeActivity
        const source = root.kind === "playlist" ? activity.playlists : root.kind === "artist" ? activity.artists : activity.folders
        return HomeSorting.activityEntries(source, root.search.pattern, root.sort.criteria, root.sort.order)
    }
    function positionContentAtBeginning() { view.positionContentAtBeginning() }
    function setCurrentItemFocus(reason) { view.setCurrentItemFocus(reason) }
    contentItem: JuxtopposedHomeCollectionView {
        id: view
        entries: root.entries
        kind: root.kind
        gridMode: MainCtx.gridView
        uiScale: VLCStyle.dp(1, VLCStyle.scale)
        iconFontFamily: VLCIcons.fontFamily
        fallbackIcon: root.kind === "playlist" ? VLCIcons.playlist : root.kind === "artist" ? VLCIcons.topbar_music : VLCIcons.topbar_discover
        fallbackArtwork: root.kind === "folder" ? "qrc:/menu/folder.svg" : root.kind === "artist" ? VLCStyle.noArtArtist : ""
        loading: MediaLib.homeActivity.loading
        errorText: MediaLib.homeActivity.error
        emptyText: root.search.pattern ? qsTr("No matching items") : qsTr("No activity yet")
        Navigation.parentItem: root
        Navigation.upItem: root.header
        onActivated: (entry, kind, reason) => root.collectionRequested(kind, entry, reason)
        onRetryRequested: MediaLib.homeActivity.refresh()
    }
}
