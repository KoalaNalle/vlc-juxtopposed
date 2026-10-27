/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * Visual design by Juxtopposed: https://www.youtube.com/@juxtopposed
 * Code licensed under GPLv2 or later; artwork under CC BY 4.0.
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
    title: qsTr("Songs")
    background: Rectangle { color: JuxtopposedPalette.color("#0E0E0E") }
    libraryModel: folders
    isSearchable: true
    sortModel: [{text: qsTr("Alphabetic"), criteria: "title"}]
    function setCurrentItemFocus(reason) { categories.setCurrentItemFocus(reason) }
    function positionContentAtBeginning() { categories.positionContentAtBeginning() }

    MLAudioFoldersModel {
        id: folders
        ml: MediaLib
        searchPattern: root.search.pattern
        sortCriteria: root.sort.criteria || "title"
        sortOrder: root.sort.order
    }
    Widgets.JuxtopposedCategoryList {
        id: categories
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.leftMargin: 20 * uiScale
        anchors.topMargin: 8 * uiScale
        width: Math.min(337 * uiScale, Math.max(0, parent.width - 40 * uiScale))
        uiScale: VLCStyle.dp(1, VLCStyle.scale)
        accessibleName: qsTr("Song folders")
        routes: folders
        fallbackIconSource: "controls-settings-cf6e1.svg"
        fallbackIconMirrored: true
        allSongsVisible: true
        emptyText: folders.loading ? qsTr("Loading song folders…")
                   : root.search.pattern ? qsTr("No matching song folders. Open All songs to search track titles.")
                   : qsTr("Add a folder containing music to see it here.")
        addAvailable: MainCtx.mediaLibraryAvailable
        Navigation.parentItem: root
        Navigation.upItem: root.header
        onAddRequested: DialogsProvider.addMediaLibraryFolder()
        onEntryRequested: (entry, reason) => {
            if (entry.id)
                History.push(["music", "tracks", "folder"], {parentId: entry.id, folderTitle: entry.title}, reason)
        }
        onAllSongsRequested: reason => History.push(["music", "tracks", "all"], {}, reason)
    }
}
