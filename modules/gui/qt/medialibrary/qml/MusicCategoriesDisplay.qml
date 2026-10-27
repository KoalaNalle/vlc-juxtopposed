/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * Visual design by Juxtopposed: https://www.youtube.com/@juxtopposed
 * Code licensed under GPLv2 or later.
 *****************************************************************************/
import QtQuick
import VLC.MainInterface
import VLC.MediaLibrary
import VLC.Dialogs
import VLC.Style
import VLC.Widgets as Widgets
import VLC.Util

Widgets.PageExt {
    id: root
    title: qsTr("Music")
    background: Rectangle { color: JuxtopposedPalette.color("#0E0E0E") }
    libraryItemCount: categories.routes.length
    // Match the other music pages' shared shell contract.
    property int displayMarginBeginning: 0
    property int displayMarginEnd: 0
    property bool enableBeginningFade: true
    property bool enableEndFade: true
    function setCurrentItemFocus(reason) { categories.setCurrentItemFocus(reason) }

    Widgets.JuxtopposedCategoryList {
        id: categories
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.leftMargin: 20 * uiScale
        anchors.topMargin: 8 * uiScale
        width: Math.min(337 * uiScale, Math.max(0, parent.width - 40 * uiScale))
        uiScale: VLCStyle.dp(1, VLCStyle.scale)
        iconFontFamily: VLCIcons.fontFamily
        addIcon: VLCIcons.add
        addAvailable: MainCtx.mediaLibraryAvailable
        routes: [
            {key: "tracks", text: qsTr("Songs"), iconSource: "music-songs-018cc.svg", iconMirrored: true, uri: ["music", "tracks"]},
            {key: "albums", text: qsTr("Albums"), iconSource: "music-albums-beb21.svg", iconMirrored: true, uri: ["music", "albums"]},
            {key: "artists", text: qsTr("Artists"), iconSource: "music-artists-7bb62.svg", uri: ["music", "artists"]},
            {key: "genres", text: qsTr("Genres"), iconSource: "music-genres-66b4e.svg", uri: ["music", "genres"]}
        ]
        Navigation.parentItem: root
        Navigation.upItem: root.header
        onAddRequested: DialogsProvider.addMediaLibraryFolder()
        onRouteRequested: (uri, reason) => History.push(uri, {}, reason)
    }
}
