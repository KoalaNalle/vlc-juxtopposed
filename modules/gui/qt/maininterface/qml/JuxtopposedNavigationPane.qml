// SPDX-License-Identifier: GPL-2.0-or-later
// Reuses the original sidebar geometry/artwork and the existing native pin service.
import QtQuick
import VLC.MainInterface
import VLC.Style
import VLC.Menus
import VLC.Util

JuxtopposedNavigation {
    id: navigation
    property bool activeSurface: true
    signal destinationOpened(int reason)
    uiScale: VLCStyle.dp(1, VLCStyle.scale)
    iconFontFamily: VLCIcons.fontFamily
    preferencesIcon: VLCIcons.settings
    pinModel: MainCtx.sidebarPins
    addIcon: VLCIcons.add
    pinIcons: ({media: VLCIcons.play, folder: VLCIcons.topbar_discover,
                source: VLCIcons.topbar_network, playlist: VLCIcons.playlist, artist: VLCIcons.topbar_music})
    onPinRequested: (key, reason) => MainCtx.sidebarPins.activate(key, reason)
    onPinMenuRequested: (key, position) => sidebarPinMenu.showPin(key, position)
    onAddRequested: position => sidebarAddMenu.popup(position, [qsTr("Files…"), qsTr("Folder…"), qsTr("Source address…")])
    activeRoute: History.viewPath[0] ?? "home"
    libraryBusy: MainCtx.mediaLibraryAvailable && !MainCtx.mediaLibrary.idle
    libraryAvailable: MainCtx.mediaLibraryAvailable
    routes: [
        {key: "home", artwork: "sidebar-home-504e1.svg", inactiveArtwork: "sidebar-home-inactive-27471.svg", mirror: true, text: qsTr("Home"), icon: VLCIcons.home,
         uri: ["home"], enabled: true},
        {key: "video", text: qsTr("Videos"), icon: VLCIcons.topbar_video,
         uri: ["video", "all"], enabled: MainCtx.mediaLibraryAvailable},
        {key: "music", text: qsTr("Music"), icon: VLCIcons.topbar_music,
         uri: ["music"], enabled: MainCtx.mediaLibraryAvailable},
        {key: "network", artwork: "sidebar-network-6db55.svg", mirror: true, text: qsTr("Network"), icon: VLCIcons.topbar_network,
         uri: ["network"], enabled: true},
        {key: "playlists", artwork: "sidebar-playlists-c9326.svg", mirror: false, text: qsTr("Playlists"), icon: VLCIcons.playlist,
         uri: ["playlists"], enabled: MainCtx.mediaLibraryAvailable},
        {key: "recent", artwork: "sidebar-recent-50e8b.svg", mirror: true, text: qsTr("Recent"), icon: VLCIcons.history,
         uri: ["recent"], enabled: MainCtx.playbackHistory.available}
    ]

    StringListMenu {
        id: sidebarAddMenu
        ctx: MainCtx
        onSelected: (index, _) => {
            if (index === 0) MainCtx.sidebarPins.chooseFiles(MainCtx.intfMainWindow)
            else if (index === 1) MainCtx.sidebarPins.chooseFolder(MainCtx.intfMainWindow)
            else MainCtx.sidebarPins.chooseSource(MainCtx.intfMainWindow)
        }
    }
    StringListMenu {
        id: sidebarPinMenu
        ctx: MainCtx
        property string pinKey: ""
        property var menuActions: []
        function indexOfKey(key) {
            for (let i = 0; i < MainCtx.sidebarPins.count; ++i)
                if (MainCtx.sidebarPins.get(i).key === key) return i
            return -1
        }
        function showPin(key, point) {
            pinKey = key
            const row = indexOfKey(key)
            if (row < 0 || MainCtx.sidebarPins.canRetry) return
            const actions = [{text: qsTr("Open"), action: "open"}]
            if (!MainCtx.sidebarPins.busy && row > 0) actions.push({text: qsTr("Move up"), action: "up"})
            if (!MainCtx.sidebarPins.busy && row + 1 < MainCtx.sidebarPins.count) actions.push({text: qsTr("Move down"), action: "down"})
            if (!MainCtx.sidebarPins.busy) actions.push({text: qsTr("Unpin"), action: "remove"})
            menuActions = actions
            popup(point, actions.map(entry => entry.text))
        }
        onSelected: (index, _) => {
            const action = menuActions[index]?.action
            const key = pinKey
            const row = indexOfKey(key)
            if (row < 0) return
            if (action === "open") MainCtx.sidebarPins.activate(key, Qt.PopupFocusReason)
            else if (action === "remove") {
                if (MainCtx.sidebarPins.remove(key)) Qt.callLater(() => navigation.focusPin(Math.min(row, MainCtx.sidebarPins.count - 1), Qt.PopupFocusReason))
            } else {
                const destination = row + (action === "up" ? -1 : 1)
                if (MainCtx.sidebarPins.move(key, destination)) Qt.callLater(() => navigation.focusPinKey(key, Qt.PopupFocusReason))
            }
        }
    }
    Connections {
        target: MainCtx.sidebarPins
        enabled: navigation.activeSurface
        function onCollectionRequested(kind, itemId, title, reason) {
            if (kind === "playlist") History.push(["playlists", "list"], {parentId: itemId, title: title}, reason)
            else History.push(["music", "artists", "albums"], {artistId: itemId}, reason)
            MainCtx.playerView = false
            navigation.destinationOpened(reason)
        }
        function onBrowseRequested(tree, reason) {
            History.push(["network", "browse"], {tree: tree}, reason)
            MainCtx.playerView = false
            navigation.destinationOpened(reason)
        }
    }
    Connections {
        target: History
        enabled: navigation.activeSurface
        // Page normalization/focus updates also emit viewPath/PropChanged.
        // Only a navigation request supersedes a pending collection open.
        function onNavigate() { MainCtx.sidebarPins?.cancelActivation() }
    }

}
