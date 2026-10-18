/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * Visual design by Juxtopposed: https://www.youtube.com/@juxtopposed
 * Code licensed under GPLv2 or later.
 *****************************************************************************/
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic as C
import VLC.MediaLibrary
import VLC.MainInterface
import VLC.Widgets as Widgets
import VLC.Style
import VLC.Util
import "JuxtopposedHomeSorting.js" as HomeSorting

Widgets.PageExt {
    id: root
    objectName: "juxtopposedRecentPage"
    readonly property real uiScale: VLCStyle.dp(1, VLCStyle.scale)
    property bool showPlaylists: false
    readonly property var historyService: MainCtx.playbackHistory
    readonly property var activity: MainCtx.mediaLibraryAvailable ? MediaLib.homeActivity : null
    function attachHistorySource() {
        if (historyService.source)
            media.ml = historyService.source
    }
    Component.onCompleted: {
        attachHistorySource()
        historyService.open()
    }
    Connections {
        target: root.historyService
        function onChanged() { root.attachHistorySource() }
    }
    title: showPlaylists ? qsTr("Recent Playlists") : qsTr("Recent")
    hasGridListMode: true
    isSearchable: true
    libraryModel: !root.showPlaylists ? media : null
    libraryItemCount: !root.showPlaylists ? media.count : playlistView.count
    sortModel: [
        { text: qsTr("Recent activity"), criteria: "activity" },
        { text: qsTr("Alphabetic"), criteria: "title" }
    ]
    function currentView() { return !root.showPlaylists ? mediaView : playlistView }
    function positionContentAtBeginning() { currentView().positionContentAtBeginning() }
    function setCurrentItemFocus(reason) { currentView().setCurrentItemFocus(reason) }
    readonly property var recentPlaylists: root.activity ? HomeSorting.recentActivityEntries(
        root.activity.playlists, root.search.pattern, root.sort.criteria, root.sort.order) : []

    MLRecentMediaModel {
        id: media
        objectName: "juxtopposedRecentMediaModel"
        searchPattern: root.search.pattern
        sortCriteria: HomeSorting.mediaCriteria(root.sort.criteria)
        sortOrder: HomeSorting.mediaOrder(root.sort.criteria, root.sort.order)
    }
    FontLoader { id: inter; source: "qrc:/juxtopposed/InterVariable.ttf" }
    component RecentTabButton: C.TabButton {
        id: control
        opacity: enabled ? 1 : 0.4
        implicitHeight: 40 * root.uiScale
        padding: 8 * root.uiScale
        font.family: inter.name
        font.pixelSize: 14 * root.uiScale
        Accessible.name: text
        contentItem: C.Label {
            text: control.text
            font: control.font
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            color: JuxtopposedPalette.color(control.checked ? "#C6C6C6" : "#8F8F8F")
            Accessible.ignored: true
        }
        background: Rectangle {
            color: JuxtopposedPalette.color(control.checked ? "#292929" : "#191919")
            border.width: control.visualFocus ? root.uiScale : 0
            border.color: JuxtopposedPalette.color("#D37918")
            Rectangle {
                anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
                height: 2 * root.uiScale
                color: JuxtopposedPalette.color("#D37918")
                visible: control.checked
            }
        }
    }
    contentItem: ColumnLayout {
        spacing: 0
        C.TabBar {
            id: tabs
            property bool ready: false
            currentIndex: root.showPlaylists ? 1 : 0
            Component.onCompleted: ready = true
            onCurrentIndexChanged: {
                if (ready && currentIndex !== (root.showPlaylists ? 1 : 0))
                    History.push(["recent", currentIndex === 0 ? "media" : "playlists"], {}, Qt.TabFocusReason)
            }
            Layout.fillWidth: true
            background: Rectangle { color: JuxtopposedPalette.color("#191919") }
            Navigation.parentItem: root
            Navigation.upItem: root.header
            Navigation.downAction: root.focusContent
            RecentTabButton { objectName: "juxtopposedRecentMediaTab"; text: qsTr("Media") }
            RecentTabButton {
                objectName: "juxtopposedRecentPlaylistsTab"
                text: qsTr("Playlists")
                enabled: MainCtx.mediaLibraryAvailable
                Accessible.description: enabled ? "" : qsTr("Turn on the media library to browse playlist activity.")
            }
        }
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: root.showPlaylists ? 1 : 0
            JuxtopposedHomeCollectionView {
                id: mediaView
                objectName: "juxtopposedRecentMediaView"
                entries: media
                kind: "media"
                showLocation: true
                gridMode: MainCtx.gridView
                uiScale: VLCStyle.dp(1, VLCStyle.scale)
                iconFontFamily: VLCIcons.fontFamily
                fallbackIcon: VLCIcons.topbar_video
                audioFallbackIcon: VLCIcons.topbar_music
                loading: root.historyService.busy || (!!root.historyService.source && media.loading)
                errorText: root.historyService.error
                emptyText: root.search.pattern ? qsTr("No matching media") : qsTr("No playback history yet")
                Navigation.parentItem: root
                Navigation.upItem: tabs
                onRetryRequested: root.historyService.open()
                onActivated: (entry, kind, reason) => {
                    root.historyService.source?.addAndPlay(entry.id, entry.isVideo && entry.progress > 0 ? [":restore-playback-pos=2"] : [])
                    MainCtx.playerView = true
                }
            }
            JuxtopposedHomeCollectionView {
                id: playlistView
                objectName: "juxtopposedRecentPlaylistsView"
                entries: root.recentPlaylists
                kind: "playlist"
                gridMode: MainCtx.gridView
                uiScale: VLCStyle.dp(1, VLCStyle.scale)
                iconFontFamily: VLCIcons.fontFamily
                fallbackIcon: VLCIcons.playlist
                loading: root.activity?.loading ?? false
                errorText: root.activity?.error ?? qsTr("Turn on the media library to browse playlist activity.")
                emptyText: root.search.pattern ? qsTr("No matching playlists") : qsTr("No playlist visits yet")
                Navigation.parentItem: root
                Navigation.upItem: tabs
                onActivated: (entry, kind, reason) => History.push(["playlists", "list"], {parentId: entry.id, title: entry.title}, reason)
                onRetryRequested: root.activity?.refresh()
            }
        }
    }
}
