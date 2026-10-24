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
    onPinMenuRequested: (key, position, reason) => sidebarPinMenu.showPin(key, position, reason)
    onAddRequested: position => sidebarAddMenu.popup(position, [qsTr("Files…"), qsTr("Folder…"), qsTr("Source address…")])
    activeRoute: History.viewPath[0] ?? "home"
    libraryBusy: MainCtx.mediaLibraryAvailable && !MainCtx.mediaLibrary.idle
    libraryAvailable: MainCtx.mediaLibraryAvailable
    routes: [
        {key: "home", artwork: "sidebar-home-504e1.svg", inactiveArtwork: "sidebar-home-inactive-27471.svg", mirror: true, text: qsTr("Home"), icon: VLCIcons.home,
         uri: ["home"], enabled: true},
        {key: "video", artwork: "sidebar-videos-7c4e4.svg", mirror: false, text: qsTr("Videos"), icon: VLCIcons.topbar_video,
         uri: ["video", "all"], enabled: MainCtx.mediaLibraryAvailable},
        {key: "music", artwork: "sidebar-music-61ad2.svg", mirror: false, text: qsTr("Music"), icon: VLCIcons.topbar_music,
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
        property int openingFocusReason: Qt.MouseFocusReason
        property bool actionSelected: false
        property int generation: 0
        function indexOfKey(key) {
            for (let i = 0; i < MainCtx.sidebarPins.count; ++i)
                if (MainCtx.sidebarPins.get(i).key === key) return i
            return -1
        }
        function showPin(key, point, reason) {
            const row = indexOfKey(key)
            if (row < 0 || MainCtx.sidebarPins.canRetry) return
            pinKey = key
            openingFocusReason = reason
            actionSelected = false
            generation++
            const actions = [{text: qsTr("Open"), action: "open"}]
            if (!MainCtx.sidebarPins.busy && row > 0) actions.push({text: qsTr("Move up"), action: "up"})
            if (!MainCtx.sidebarPins.busy && row + 1 < MainCtx.sidebarPins.count) actions.push({text: qsTr("Move down"), action: "down"})
            if (!MainCtx.sidebarPins.busy) actions.push({text: qsTr("Unpin"), action: "remove"})
            menuActions = actions
            popup(point, actions.map(entry => entry.text))
        }
        onVisibleChanged: {
            if (visible || openingFocusReason !== Qt.ShortcutFocusReason) return
            const key = pinKey
            const closedGeneration = generation
            // Wait until the native popup has returned focus to the QML window.
            // A selected action owns its own destination/fallback focus instead.
            Qt.callLater(() => {
                if (!sidebarPinMenu.visible && !sidebarPinMenu.actionSelected
                        && sidebarPinMenu.generation === closedGeneration
                        && navigation.activeSurface && navigation.visible)
                    navigation.focusPinKey(key, Qt.ShortcutFocusReason)
            })
        }
        onSelected: (index, _) => {
            actionSelected = true
            const action = menuActions[index]?.action
            const key = pinKey
            const row = indexOfKey(key)
            const reason = openingFocusReason === Qt.ShortcutFocusReason ? Qt.ShortcutFocusReason : Qt.PopupFocusReason
            if (row < 0) return
            if (action === "open") MainCtx.sidebarPins.activate(key, reason)
            else if (action === "remove") {
                if (MainCtx.sidebarPins.remove(key)) Qt.callLater(() => navigation.focusPin(Math.min(row, MainCtx.sidebarPins.count - 1), reason))
            } else {
                const destination = row + (action === "up" ? -1 : 1)
                if (MainCtx.sidebarPins.move(key, destination)) Qt.callLater(() => navigation.focusPinKey(key, reason))
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
