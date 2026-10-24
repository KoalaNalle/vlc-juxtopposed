// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
import QtQuick
import QtQuick.Window
import VLC.Widgets as Widgets
import VLC.Player as Player

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
    property var setupNativePalette: null
    property var setupViewColors: null
    property var setupWindowColors: null
    property var setupInterfaceShortcuts: ({})
    property var playerPreviewState: ({})
    property bool setupPostponed: false
    readonly property var setupPreferences: root.mainCtx.generalPreferences
    readonly property int setupDraftTheme: setupLoader.item ? setupLoader.item.draftTheme : 2
    readonly property bool setupVisible: !!setupLoader.item && setupLoader.item.visible
    readonly property bool qmlModalVisible: globalSettingsDialog.visible || networkStreamDialog.visible
                                             || newPlaylistDialog.visible || conversionDialog.visible || playerProfilesDialog.visible || customizer.visible || setupVisible
    signal playlistRequested(var playlist)
    signal playlistSaved()

    function restoreViewFocus() {
        if (fallbackFocusItem && fallbackFocusItem.visible && fallbackFocusItem.enabled)
            fallbackFocusItem.forceActiveFocus(Qt.PopupFocusReason)
    }
    function routeTo(dialog, options) {
        if (dialogsProvider.prefsDialogVisible || setupVisible) return false
        const dialogs = [globalSettingsDialog, networkStreamDialog, newPlaylistDialog, conversionDialog, playerProfilesDialog, customizer]
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
        dialog.showDialog(root.Window.window?.activeFocusItem ?? null, options)
        return true
    }

    function tryOpenSetup() {
        if (setupPostponed || setupLoader.active || qmlModalVisible || dialogsProvider.prefsDialogVisible
                || !("juxtopposedControls" in mainCtx) || !mainCtx.juxtopposedControls
                || !setupPreferences || !("setupReady" in setupPreferences) || !setupPreferences.setupReady
                || ("firstRunDialogVisible" in dialogsProvider && dialogsProvider.firstRunDialogVisible)) return
        // Supply bindings before construction: preview resources and services
        // must never briefly initialize against an unrelated default provider.
        setupLoader.setSource(Qt.resolvedUrl("JuxtopposedSetupDialog.qml"), {
            parent: root,
            preferences: Qt.binding(() => root.setupPreferences),
            uiScale: Qt.binding(() => root.uiScale),
            assetBase: Qt.binding(() => root.assetBase),
            previewNativePalette: Qt.binding(() => root.setupNativePalette),
            previewViewColors: Qt.binding(() => root.setupViewColors),
            previewWindowColors: Qt.binding(() => root.setupWindowColors),
            interfaceShortcuts: Qt.binding(() => root.setupInterfaceShortcuts),
            activeShortcuts: Qt.binding(() => "controlsPreferences" in root.mainCtx
                && root.mainCtx.controlsPreferences ? root.mainCtx.controlsPreferences.activeShortcuts : ({}))
        })
        setupLoader.active = true
    }
    Component.onCompleted: Qt.callLater(tryOpenSetup)
    onQmlModalVisibleChanged: if (!qmlModalVisible) Qt.callLater(tryOpenSetup)
    Loader {
        id: setupLoader
        active: false
        onLoaded: item.showDialog(root.Window.window?.activeFocusItem ?? null)
    }
    Connections {
        target: setupLoader.item
        function onClosed() {
            root.setupPostponed = !!root.setupPreferences?.setupRequired
            Qt.callLater(() => { setupLoader.active = false })
        }
        function onFallbackFocusRequested() { root.restoreViewFocus() }
    }
    Connections {
        target: root.setupPreferences
        ignoreUnknownSignals: true // Older supplied services have no setup contract.
        function onSetupStateChanged() { Qt.callLater(root.tryOpenSetup) }
    }
    Connections {
        target: root.mainCtx
        ignoreUnknownSignals: true
        function onJuxtopposedControlsChanged() { Qt.callLater(root.tryOpenSetup) }
    }
    Connections {
        target: root.dialogsProvider
        ignoreUnknownSignals: true
        function onFirstRunDialogVisibleChanged() { Qt.callLater(root.tryOpenSetup) }
        function onPrefsDialogVisibleChanged() { Qt.callLater(root.tryOpenSetup) }
    }

    JuxtopposedGlobalSettings {
        id: globalSettingsDialog
        parent: root
        backdropSource: root.backdropSource
        preferences: root.mainCtx.generalPreferences
        appearance: root.appearance
        uiScale: root.uiScale
        assetBase: root.assetBase
        title: selectedCategory === "extensions" ? qsTr("Add-ons & Scripts") : selectedCategory === "recording" ? qsTr("Recording, Casting & Streaming Out")
            : selectedCategory === "performance" ? qsTr("Performance Settings")
            : selectedCategory === "network" ? qsTr("Network Connections & Security")
            : selectedCategory === "privacy" ? qsTr("Privacy Settings")
            : selectedCategory === "controls" ? qsTr("Mouse, Keyboard & Remote Controls")
            : selectedCategory === "library" ? qsTr("Playlists, Folders & Files")
            : selectedCategory === "subtitles" ? qsTr("Subtitle Defaults")
            : selectedCategory === "video" ? qsTr("Video Defaults")
            : selectedCategory === "audio" ? qsTr("Audio Defaults")
            : selectedCategory === "playback" ? qsTr("Playback Preferences")
            : selectedCategory === "interface" ? qsTr("Interface Settings") : qsTr("Global Settings")
        categoryPages: {
            const pages = {}
            if ("extensionsPreferences" in root.mainCtx && root.mainCtx.extensionsPreferences) pages.extensions = extensionsPage
            if ("interfacePreferences" in root.mainCtx && root.mainCtx.interfacePreferences) pages.interface = interfacePage
            if ("playbackPreferences" in root.mainCtx && root.mainCtx.playbackPreferences) pages.playback = playbackPage
            if ("audioPreferences" in root.mainCtx && root.mainCtx.audioPreferences) pages.audio = audioPage
            if ("videoPreferences" in root.mainCtx && root.mainCtx.videoPreferences) pages.video = videoPage
            if ("subtitlesPreferences" in root.mainCtx && root.mainCtx.subtitlesPreferences) pages.subtitles = subtitlesPage
            if ("libraryPreferences" in root.mainCtx && root.mainCtx.libraryPreferences) pages.library = libraryPage
            if ("privacyPreferences" in root.mainCtx && root.mainCtx.privacyPreferences) pages.privacy = privacyPage
            if ("controlsPreferences" in root.mainCtx && root.mainCtx.controlsPreferences) pages.controls = controlsPage
            if ("networkPreferences" in root.mainCtx && root.mainCtx.networkPreferences) pages.network = networkPage
            if ("performancePreferences" in root.mainCtx && root.mainCtx.performancePreferences) pages.performance = performancePage
            if ("recordingPreferences" in root.mainCtx && root.mainCtx.recordingPreferences) pages.recording = recordingPage
            return pages
        }
        categorySearchTerms: ({extensions: [qsTr("Add-ons & Scripts"),qsTr("Add-ons"),qsTr("Search"),qsTr("Find more extensions"),qsTr("Add"),qsTr("Remove"),qsTr("Load add-ons on startup"),qsTr("Advanced Options")], recording: [qsTr("Recording"),qsTr("Save recordings to"),qsTr("Browse"),qsTr("Record without re-encoding when possible"),qsTr("Casting"),qsTr("Discover cast devices"),qsTr("Cast quality"),qsTr("Streaming"),qsTr("Play locally while streaming"),qsTr("Include audio"),qsTr("Include video"),qsTr("include subtitles"),qsTr("Encoding preset"),qsTr("Tuning"),qsTr("Advanced Options")], performance: [qsTr("Decoding"),qsTr("Hardware-accelerated decoding"),qsTr("Video post-processing"),qsTr("Skip the H.264 loop filter"),qsTr("Decoding threads"),qsTr("Smooth Playback"),qsTr("Drop frames when the system can’t keep up"),qsTr("Give VLC real-time priority"),qsTr("Process priority"),qsTr("Rescan plugins"),qsTr("Advanced Options"),qsTr("Developer"),qsTr("Encoder"),qsTr("Decoder"),qsTr("Clock synchronisation"),qsTr("Clock jitter"),qsTr("Clock reference averaging")], network: [qsTr("Buffering"),qsTr("Local files"),qsTr("Discs"),qsTr("Network streams"),qsTr("Live capture"),qsTr("Proxy"),qsTr("HTTP proxy"),qsTr("HTTP proxy password"),qsTr("Security"),qsTr("Verify certificates against the system trust store"),qsTr("Cipher preference"),qsTr("Advanced Options")], controls: [qsTr("Keyboard Shortcuts"),qsTr("Defaults"),qsTr("Edit Keyboard Shortcuts"),qsTr("Reset all shortcuts"),qsTr("Skip Amounts"),qsTr("Very short skip"),qsTr("Short skip"),qsTr("Medium skip"),qsTr("Long skip"),qsTr("Mouse"),qsTr("Single click on the video"),qsTr("Double click on the video"),qsTr("Scroll wheel"),qsTr("Horizontal scroll"),qsTr("Reverse scroll direction"),qsTr("Remotes"),qsTr("Web remote control"),qsTr("Port"),qsTr("Use the same wheel settings")], privacy: [qsTr("History"), qsTr("Network"), qsTr("Save recently played items"), qsTr("Remember the playback position"), qsTr("Never remember items matching"), qsTr("Clear history now"), qsTr("Fetch titles and cover art from the internet"), qsTr("Submit played tracks to last.fm"), qsTr("Last.fm username"), qsTr("Last.fm password"), qsTr("Last.fm submission URL")], library: [qsTr("Opening Files"), qsTr("Metadata & Artwork"), qsTr("Only allow one VLC window"), qsTr("Apply to files opened from the file manager"), qsTr("Add to the playlist instead of replacing it"), qsTr("When opening a folder"), qsTr("Show hidden files"), qsTr("Fetch titles and artwork"), qsTr("Use the media library"), qsTr("Advanced Options")], subtitles: [qsTr("Tracks & Languages"), qsTr("Loading Subtitles"), qsTr("Appearance"), qsTr("Timing"), qsTr("On-Screen Messages"), qsTr("Advanced Options"), qsTr("Enable subtitles"), qsTr("Preferred language"), qsTr("Text encoding"), qsTr("Load subtitle files automatically"), qsTr("File name matching"), qsTr("Font"), qsTr("Size values"), qsTr("Font size"), qsTr("Bold"), qsTr("Text color"), qsTr("Text opacity"), qsTr("Outline thickness"), qsTr("Outline color"), qsTr("Shadow"), qsTr("Shadow color"), qsTr("Shadow distance"), qsTr("Background"), qsTr("Background color"), qsTr("Background opacity"), qsTr("Vertical position"), qsTr("Subtitle delay"), qsTr("Show feedback for shortcuts"), qsTr("Show feedback for"), qsTr("Show filename when a video starts"), qsTr("Show filename for"), qsTr("Load subtitles from these folders"), qsTr("Ignore styling in ASS and SSA subtitles"), qsTr("Shadow angle"), qsTr("Text direction"), qsTr("Monospace font"), qsTr("Visualization size")], video: [qsTr("Output"), qsTr("Picture"), qsTr("Deinterlacing"), qsTr("Framing"), qsTr("Snapshots"), qsTr("Advanced Options"), qsTr("Enable video"), qsTr("Start videos in fullscreen"), qsTr("Stop the screensaver during playback"), qsTr("Play video as desktop wallpaper"), qsTr("Hue"), qsTr("Brightness"), qsTr("Contrast"), qsTr("Saturation"), qsTr("Gamma"), qsTr("Reset picture"), qsTr("Method"), qsTr("Aspect ratio"), qsTr("Crop"), qsTr("Scale video to the window"), qsTr("Zoom"), qsTr("Position in the window"), qsTr("Save to"), qsTr("File name prefix"), qsTr("Format"), qsTr("Size"), qsTr("Number files sequentially"), qsTr("Show a preview after capture"), qsTr("Custom aspect ratios"), qsTr("Custom crop ratios"), qsTr("Monitor pixel aspect ratio"), qsTr("Time-shift buffer size"), qsTr("Reset picture adjustments")], audio: [qsTr("Output"), qsTr("Volume"), qsTr("Loudness"), qsTr("Effects"), qsTr("Tracks"), qsTr("Visualization"), qsTr("Advanced Options"), qsTr("Enable audio"), qsTr("Output device"), qsTr("Speaker setup"), qsTr("Maximum volume"), qsTr("Startup volume"), qsTr("Volume step"), qsTr("Normalize loudness"), qsTr("Target level"), qsTr("Night mode"), qsTr("Replay gain"), qsTr("Replay gain preamp"), qsTr("Equalizer"), qsTr("Equalizer preset"), qsTr("Two-pass"), qsTr("Spatial audio"), qsTr("Spatial audio mode"), qsTr("Stereo widening"), qsTr("Compressor"), qsTr("Pitch shift"), qsTr("Chorus/flanger"), qsTr("Karaoke"), qsTr("Preferred audio language"), qsTr("Audio delay"), qsTr("Visualizer"), qsTr("Pass compressed audio to the receiver"), qsTr("Detect Dolby Surround"), qsTr("Resampling quality"), qsTr("Media role"), qsTr("Visualization size"), qsTr("Open Equalizer"), qsTr("Preamp"), qsTr("Equalizer bands"), qsTr("Equalizer preamp")], playback: [qsTr("Startup"), qsTr("Tracks"), qsTr("Order"), qsTr("Seeking"), qsTr("Files & Discs"), qsTr("Advanced Options"), qsTr("Continue where you left off"), qsTr("Start playing as soon as a file opens"), qsTr("Default playback speed"), qsTr("Pause when another app uses audio"), qsTr("Preferred video quality"), qsTr("Preferred menu language"), qsTr("Default audio track"), qsTr("Default subtitle track"), qsTr("Repeat"), qsTr("Shuffle"), qsTr("Stop after each item"), qsTr("When playback finishes"), qsTr("Seek precisely"), qsTr("Show thumbnail while scrubbing"), qsTr("Default optical drive"), qsTr("Repair damaged AVI files"), qsTr("Load matching .mkv files from the same folder"), qsTr("Time-shift buffer folder"), qsTr("Time-shift buffer size")], interface: [qsTr("Skin"), qsTr("Use a custom skin"), qsTr("Skin file"), qsTr("Window"), qsTr("Play video inside main window"), qsTr("Resize window to fit video"), qsTr("Remember window size and position"), qsTr("Keep window above other windows"), qsTr("Show window decorations"), qsTr("Show the file name in the title bar"), qsTr("Open fullscreen on"), qsTr("Playback Controls"), qsTr("Start in compact mode"), qsTr("Auto-hide the controls"), qsTr("Hide after"), qsTr("Show controls in fullscreen"), qsTr("Notifications and System Tray"), qsTr("Show a system tray icon"), qsTr("Start minimized to the tray"), qsTr("Show a popup when the track changes"), qsTr("Popup duration"), qsTr("Playlist"), qsTr("Show the playlist as a tree")]})
        function reloadCategory(category) {
            const key = category === "extensions" ? "extensionsPreferences" : category === "recording" ? "recordingPreferences" : category === "performance" ? "performancePreferences" : category === "network" ? "networkPreferences" : category === "controls" ? "controlsPreferences" : category === "privacy" ? "privacyPreferences" : category === "library" ? "libraryPreferences" : category === "subtitles" ? "subtitlesPreferences" : category === "video" ? "videoPreferences" : category === "audio" ? "audioPreferences" : category === "playback" ? "playbackPreferences" : category === "interface" ? "interfacePreferences" : ""
            if (key.length && key in root.mainCtx && root.mainCtx[key]) root.mainCtx[key].reload()
        }
        onAboutToShow: reloadCategory(selectedCategory)
        onCategoryRequested: category => reloadCategory(category)
        onAdvancedRequested: Qt.callLater(() => {
            if (!root.qmlModalVisible && !root.dialogsProvider.prefsDialogVisible)
                root.dialogsProvider.expertPrefsDialog()
        })
        onFallbackFocusRequested: root.restoreViewFocus()
    }
    Component {
        id: privacyPage
        JuxtopposedPrivacySettings { privacyPreferences: "privacyPreferences" in root.mainCtx ? root.mainCtx.privacyPreferences : null }
    }
    Component {
        id: extensionsPage
        JuxtopposedExtensionsSettings {
            extensionsPreferences: "extensionsPreferences" in root.mainCtx ? root.mainCtx.extensionsPreferences : null
            onAdvancedRequested: { globalSettingsDialog.close(); globalSettingsDialog.advancedRequested() }
        }
    }
    Component {
        id: recordingPage
        JuxtopposedRecordingSettings {
            recordingPreferences: "recordingPreferences" in root.mainCtx ? root.mainCtx.recordingPreferences : null
            onAdvancedRequested: {
                globalSettingsDialog.close()
                globalSettingsDialog.advancedRequested()
            }
        }
    }
    Component {
        id: performancePage
        JuxtopposedPerformanceSettings {
            performancePreferences: "performancePreferences" in root.mainCtx ? root.mainCtx.performancePreferences : null
        }
    }
    Component {
        id: networkPage
        JuxtopposedNetworkSettings {
            networkPreferences: "networkPreferences" in root.mainCtx ? root.mainCtx.networkPreferences : null
            onAdvancedRequested: {
                globalSettingsDialog.close()
                globalSettingsDialog.advancedRequested()
            }
        }
    }
    Component {
        id: controlsPage
        JuxtopposedControlsSettings {
            controlsPreferences: "controlsPreferences" in root.mainCtx ? root.mainCtx.controlsPreferences : null
        }
    }
    Component {
        id: libraryPage
        JuxtopposedLibrarySettings {
            libraryPreferences: "libraryPreferences" in root.mainCtx ? root.mainCtx.libraryPreferences : null
            onAdvancedRequested: {
                globalSettingsDialog.close()
                globalSettingsDialog.advancedRequested()
            }
        }
    }
    Component {
        id: subtitlesPage
        JuxtopposedSubtitlesSettings {
            subtitlesPreferences: "subtitlesPreferences" in root.mainCtx ? root.mainCtx.subtitlesPreferences : null
        }
    }
    Component {
        id: audioPage
        JuxtopposedAudioSettings {
            audioPreferences: "audioPreferences" in root.mainCtx ? root.mainCtx.audioPreferences : null
        }
    }
    Component {
        id: videoPage
        JuxtopposedVideoSettings {
            videoPreferences: "videoPreferences" in root.mainCtx ? root.mainCtx.videoPreferences : null
        }
    }
    Component {
        id: playbackPage
        JuxtopposedPlaybackSettings {
            playbackPreferences: "playbackPreferences" in root.mainCtx ? root.mainCtx.playbackPreferences : null
        }
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
    JuxtopposedPlayerProfilesDialog {
        id: playerProfilesDialog
        parent: root
        controller: "playerProfiles" in root.mainCtx ? root.mainCtx.playerProfiles : null
        uiScale: root.uiScale
        assetBase: root.assetBase
        onFallbackFocusRequested: root.restoreViewFocus()
    }
    JuxtopposedPlayerCustomizer {
        id: customizer
        parent: root
        controller: "playerProfiles" in root.mainCtx ? root.mainCtx.playerProfiles : null
        uiScale: root.uiScale
        assetBase: root.assetBase
        onFallbackFocusRequested: root.restoreViewFocus()
        previewComponent: Component {
            Player.JuxtopposedPlaybackBar {
                id: draftPlayer
                uiScale: root.uiScale
                assetBase: root.assetBase
                previewOnly: true
                editing: true
                controlLayout: customizer.draftData
                elapsed: root.playerPreviewState.elapsed ?? "00:00:00"
                duration: root.playerPreviewState.duration ?? "00:00:00"
                live: root.playerPreviewState.live === true
                position: root.playerPreviewState.position ?? 0
                volume: root.playerPreviewState.volume ?? 0
                muted: root.playerPreviewState.muted === true
                playing: root.playerPreviewState.playing === true
                mediaMetadata: root.playerPreviewState.metadata ?? ({})
                describeTile: id => customizer.layout.tileInfo(id)
                tileState: function(id) {
                    const info = customizer.layout.tileInfo(id)
                    return info.kind === "play" ? {enabled: false, playing: draftPlayer.playing, glyph: "", source: root.assetBase + (draftPlayer.playing ? "pause.svg" : "play.svg")}
                        : {enabled: false, glyph: info.glyph, fontFamily: info.fontFamily}
                }
                onLayoutTilePressed: (column, index, tile, item, point) => customizer.beginDrag(column, index, tile, item, point)
                onLayoutTileMoved: (item, point) => customizer.moveDrag(item, point)
                onLayoutTileReleased: (item, point) => customizer.endDrag(item, point)
                onLayoutSelectionRequested: (column, index) => customizer.selection = {column: column, index: index}
                onLayoutRemovalRequested: (column, index) => {customizer.selection = {column: column, index: index}; customizer.removeSelection()}
            }
        }
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
        function onShowPlayerProfilesDialog(create) { root.routeTo(playerProfilesDialog, create) }
        function onShowToolbarEditorDialog() { if (root.mainCtx.juxtopposedControls) root.routeTo(customizer) }
    }
}
