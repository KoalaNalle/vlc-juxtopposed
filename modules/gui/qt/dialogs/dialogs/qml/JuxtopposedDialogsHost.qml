// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
import QtQuick
import QtQuick.Window
import VLC.Widgets as Widgets

// One owner for the existing Juxtopposed modals and their native hotkey guards.
// Services are supplied by MainInterface; this component creates no settings.
Item {
    id: root
    objectName: "juxtopposedDialogsHost"
    required property var dialogsProvider
    required property var mainCtx
    property var appearance: null
    property real uiScale: 1
    property url assetBase: "qrc:/juxtopposed/"
    property Item backdropSource: null
    property Item fallbackFocusItem: null
    readonly property bool qmlModalVisible: globalSettingsDialog.visible || networkStreamDialog.visible
                                             || newPlaylistDialog.visible || conversionDialog.visible
    signal playlistRequested(var playlist)
    signal playlistSaved()

    function restoreViewFocus() {
        if (fallbackFocusItem && fallbackFocusItem.visible && fallbackFocusItem.enabled)
            fallbackFocusItem.forceActiveFocus(Qt.PopupFocusReason)
    }
    function routeTo(dialog) {
        if (dialogsProvider.prefsDialogVisible) return false
        const dialogs = [globalSettingsDialog, networkStreamDialog, newPlaylistDialog, conversionDialog]
        // Check every operation before closing any modal. A request must never
        // cancel a running job or partly mutate the dialog state before failing.
        for (const other of dialogs) {
            if (other !== dialog && other.visible && "busy" in other && other.busy) return false
            if (other !== dialog && other.visible && "controller" in other
                    && other.controller && other.controller.busy) return false
        }
        for (const other of dialogs) {
            if (other === dialog || !other.visible) continue
            if (other === globalSettingsDialog) other.close()
            else if (other === conversionDialog) other.requestClose()
            else other.reject()
        }
        dialog.showDialog(root.Window.window?.activeFocusItem ?? null)
        return true
    }

    JuxtopposedGlobalSettings {
        id: globalSettingsDialog
        parent: root
        backdropSource: root.backdropSource
        preferences: root.mainCtx.generalPreferences
        appearance: root.appearance
        uiScale: root.uiScale
        assetBase: root.assetBase
        title: selectedCategory === "interface" ? qsTr("Interface Settings") : qsTr("Global Settings")
        categoryPages: ("interfacePreferences" in root.mainCtx && root.mainCtx.interfacePreferences)
            ? ({interface: interfacePage}) : ({})
        categorySearchTerms: ({interface: [qsTr("Skin"), qsTr("Use a custom skin"), qsTr("Skin file"), qsTr("Window"), qsTr("Play video inside main window"), qsTr("Resize window to fit video"), qsTr("Remember window size and position"), qsTr("Keep window above other windows"), qsTr("Show window decorations"), qsTr("Show the file name in the title bar"), qsTr("Open fullscreen on"), qsTr("Playback Controls"), qsTr("Start in compact mode"), qsTr("Auto-hide the controls"), qsTr("Hide after"), qsTr("Show controls in fullscreen"), qsTr("Notifications and System Tray"), qsTr("Show a system tray icon"), qsTr("Start minimized to the tray"), qsTr("Show a popup when the track changes"), qsTr("Popup duration"), qsTr("Playlist"), qsTr("Show the playlist as a tree")]})
        onAboutToShow: if (selectedCategory === "interface" && "interfacePreferences" in root.mainCtx && root.mainCtx.interfacePreferences)
            root.mainCtx.interfacePreferences.reload()
        onCategoryRequested: category => {
            if (category === "interface" && "interfacePreferences" in root.mainCtx && root.mainCtx.interfacePreferences)
                root.mainCtx.interfacePreferences.reload()
        }
        onAdvancedRequested: Qt.callLater(() => {
            if (!root.qmlModalVisible && !root.dialogsProvider.prefsDialogVisible)
                root.dialogsProvider.expertPrefsDialog()
        })
        onFallbackFocusRequested: root.restoreViewFocus()
    }
    Component {
        id: interfacePage
        JuxtopposedInterfaceSettings {
            interfacePreferences: "interfacePreferences" in root.mainCtx ? root.mainCtx.interfacePreferences : null
            onAdvancedRequested: {
                globalSettingsDialog.close()
                globalSettingsDialog.advancedRequested()
            }
        }
    }
    JuxtopposedNetworkDialog {
        id: networkStreamDialog
        parent: root
        controller: root.dialogsProvider.networkStream
        uiScale: root.uiScale
        assetBase: root.assetBase
        onFallbackFocusRequested: root.restoreViewFocus()
    }
    JuxtopposedNewPlaylistDialog {
        id: newPlaylistDialog
        parent: root
        controller: root.dialogsProvider.newPlaylist
        uiScale: root.uiScale
        assetBase: root.assetBase
        onFilesRequested: root.dialogsProvider.chooseNewPlaylistFiles()
        onThumbnailRequested: root.dialogsProvider.chooseNewPlaylistThumbnail()
        onPlaylistRequested: playlist => root.playlistRequested(playlist)
        onFallbackFocusRequested: root.restoreViewFocus()
    }
    JuxtopposedConversionDialog {
        id: conversionDialog
        parent: root
        controller: root.dialogsProvider.conversion
        uiScale: root.uiScale
        assetBase: root.assetBase
        onSourceRequested: root.dialogsProvider.chooseConversionSource()
        onDestinationRequested: root.dialogsProvider.chooseConversionDestination()
        onAllProfilesRequested: root.dialogsProvider.openLegacyTranscodingDialog()
        onFallbackFocusRequested: root.restoreViewFocus()
    }
    Widgets.TextInputFocus {
        id: textInputFocus
        focusItem: root.Window.window?.activeFocusItem ?? null
    }
    Binding {
        target: root.dialogsProvider
        property: "qmlDialogVisible"
        value: root.qmlModalVisible
        restoreMode: Binding.RestoreBindingOrValue
    }
    Binding {
        target: root.dialogsProvider
        property: "qmlInputFocused"
        value: textInputFocus.editing
        restoreMode: Binding.RestoreBindingOrValue
    }
    Connections {
        target: root.dialogsProvider
        function onNewPlaylistSaved(playlist) { root.playlistSaved() }
        function onShowGlobalSettingsDialog() { root.routeTo(globalSettingsDialog) }
        function onShowNetworkStreamDialog() { root.routeTo(networkStreamDialog) }
        function onShowNewPlaylistDialog() { root.routeTo(newPlaylistDialog) }
        function onShowConversionDialog() { root.routeTo(conversionDialog) }
    }
}
