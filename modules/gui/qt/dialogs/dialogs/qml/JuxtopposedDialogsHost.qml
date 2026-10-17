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
        title: selectedCategory === "privacy" ? qsTr("Privacy Settings")
            : selectedCategory === "controls" ? qsTr("Mouse, Keyboard & Remote Controls")
            : selectedCategory === "library" ? qsTr("Playlists, Folders & Files")
            : selectedCategory === "subtitles" ? qsTr("Subtitle Defaults")
            : selectedCategory === "video" ? qsTr("Video Defaults")
            : selectedCategory === "audio" ? qsTr("Audio Defaults")
            : selectedCategory === "playback" ? qsTr("Playback Preferences")
            : selectedCategory === "interface" ? qsTr("Interface Settings") : qsTr("Global Settings")
        categoryPages: {
            const pages = {}
            if ("interfacePreferences" in root.mainCtx && root.mainCtx.interfacePreferences) pages.interface = interfacePage
            if ("playbackPreferences" in root.mainCtx && root.mainCtx.playbackPreferences) pages.playback = playbackPage
            if ("audioPreferences" in root.mainCtx && root.mainCtx.audioPreferences) pages.audio = audioPage
            if ("videoPreferences" in root.mainCtx && root.mainCtx.videoPreferences) pages.video = videoPage
            if ("subtitlesPreferences" in root.mainCtx && root.mainCtx.subtitlesPreferences) pages.subtitles = subtitlesPage
            if ("libraryPreferences" in root.mainCtx && root.mainCtx.libraryPreferences) pages.library = libraryPage
            if ("privacyPreferences" in root.mainCtx && root.mainCtx.privacyPreferences) pages.privacy = privacyPage
            if ("controlsPreferences" in root.mainCtx && root.mainCtx.controlsPreferences) pages.controls = controlsPage
            return pages
        }
        categorySearchTerms: ({controls: [qsTr("Keyboard Shortcuts"),qsTr("Defaults"),qsTr("Edit Keyboard Shortcuts"),qsTr("Reset all shortcuts"),qsTr("Skip Amounts"),qsTr("Very short skip"),qsTr("Short skip"),qsTr("Medium skip"),qsTr("Long skip"),qsTr("Mouse"),qsTr("Single click on the video"),qsTr("Double click on the video"),qsTr("Scroll wheel"),qsTr("Horizontal scroll"),qsTr("Reverse scroll direction"),qsTr("Remotes"),qsTr("Web remote control"),qsTr("Port")], privacy: [qsTr("History"), qsTr("Network"), qsTr("Save recently played items"), qsTr("Remember the playback position"), qsTr("Never remember items matching"), qsTr("Clear history now"), qsTr("Fetch titles and cover art from the internet"), qsTr("Submit played tracks to last.fm"), qsTr("Last.fm username"), qsTr("Last.fm password"), qsTr("Last.fm submission URL")], library: [qsTr("Opening Files"), qsTr("Metadata & Artwork"), qsTr("Only allow one VLC window"), qsTr("Apply to files opened from the file manager"), qsTr("Add to the playlist instead of replacing it"), qsTr("When opening a folder"), qsTr("Show hidden files"), qsTr("Fetch titles and artwork"), qsTr("Use the media library"), qsTr("Advanced Options")], subtitles: [qsTr("Tracks & Languages"), qsTr("Loading Subtitles"), qsTr("Appearance"), qsTr("Timing"), qsTr("On-Screen Messages"), qsTr("Advanced Options"), qsTr("Enable subtitles"), qsTr("Preferred language"), qsTr("Text encoding"), qsTr("Load subtitle files automatically"), qsTr("File name matching"), qsTr("Font"), qsTr("Size values"), qsTr("Font size"), qsTr("Bold"), qsTr("Text color"), qsTr("Text opacity"), qsTr("Outline thickness"), qsTr("Outline color"), qsTr("Shadow"), qsTr("Shadow color"), qsTr("Shadow distance"), qsTr("Background"), qsTr("Background color"), qsTr("Background opacity"), qsTr("Vertical position"), qsTr("Subtitle delay"), qsTr("Show feedback for shortcuts"), qsTr("Show feedback for"), qsTr("Show filename when a video starts"), qsTr("Show filename for"), qsTr("Load subtitles from these folders"), qsTr("Ignore styling in ASS and SSA subtitles"), qsTr("Shadow angle"), qsTr("Text direction"), qsTr("Monospace font"), qsTr("Visualization size")], video: [qsTr("Output"), qsTr("Picture"), qsTr("Deinterlacing"), qsTr("Framing"), qsTr("Snapshots"), qsTr("Advanced Options"), qsTr("Enable video"), qsTr("Start videos in fullscreen"), qsTr("Stop the screensaver during playback"), qsTr("Play video as desktop wallpaper"), qsTr("Hue"), qsTr("Brightness"), qsTr("Contrast"), qsTr("Saturation"), qsTr("Gamma"), qsTr("Reset picture"), qsTr("Method"), qsTr("Aspect ratio"), qsTr("Crop"), qsTr("Scale video to the window"), qsTr("Zoom"), qsTr("Position in the window"), qsTr("Save to"), qsTr("File name prefix"), qsTr("Format"), qsTr("Size"), qsTr("Number files sequentially"), qsTr("Show a preview after capture"), qsTr("Custom aspect ratios"), qsTr("Custom crop ratios"), qsTr("Monitor pixel aspect ratio"), qsTr("Time-shift buffer size"), qsTr("Reset picture adjustments")], audio: [qsTr("Output"), qsTr("Volume"), qsTr("Loudness"), qsTr("Effects"), qsTr("Tracks"), qsTr("Visualization"), qsTr("Advanced Options"), qsTr("Enable audio"), qsTr("Output device"), qsTr("Speaker setup"), qsTr("Maximum volume"), qsTr("Startup volume"), qsTr("Volume step"), qsTr("Normalize loudness"), qsTr("Target level"), qsTr("Night mode"), qsTr("Replay gain"), qsTr("Replay gain preamp"), qsTr("Equalizer"), qsTr("Equalizer preset"), qsTr("Two-pass"), qsTr("Spatial audio"), qsTr("Spatial audio mode"), qsTr("Stereo widening"), qsTr("Compressor"), qsTr("Pitch shift"), qsTr("Chorus/flanger"), qsTr("Karaoke"), qsTr("Preferred audio language"), qsTr("Audio delay"), qsTr("Visualizer"), qsTr("Pass compressed audio to the receiver"), qsTr("Detect Dolby Surround"), qsTr("Resampling quality"), qsTr("Media role"), qsTr("Visualization size"), qsTr("Open Equalizer"), qsTr("Preamp"), qsTr("Equalizer bands"), qsTr("Equalizer preamp")], playback: [qsTr("Startup"), qsTr("Tracks"), qsTr("Order"), qsTr("Seeking"), qsTr("Files & Discs"), qsTr("Advanced Options"), qsTr("Continue where you left off"), qsTr("Start playing as soon as a file opens"), qsTr("Default playback speed"), qsTr("Pause when another app uses audio"), qsTr("Preferred video quality"), qsTr("Preferred menu language"), qsTr("Default audio track"), qsTr("Default subtitle track"), qsTr("Repeat"), qsTr("Shuffle"), qsTr("Stop after each item"), qsTr("When playback finishes"), qsTr("Seek precisely"), qsTr("Show thumbnail while scrubbing"), qsTr("Default optical drive"), qsTr("Repair damaged AVI files"), qsTr("Load matching .mkv files from the same folder"), qsTr("Time-shift buffer folder"), qsTr("Time-shift buffer size")], interface: [qsTr("Skin"), qsTr("Use a custom skin"), qsTr("Skin file"), qsTr("Window"), qsTr("Play video inside main window"), qsTr("Resize window to fit video"), qsTr("Remember window size and position"), qsTr("Keep window above other windows"), qsTr("Show window decorations"), qsTr("Show the file name in the title bar"), qsTr("Open fullscreen on"), qsTr("Playback Controls"), qsTr("Start in compact mode"), qsTr("Auto-hide the controls"), qsTr("Hide after"), qsTr("Show controls in fullscreen"), qsTr("Notifications and System Tray"), qsTr("Show a system tray icon"), qsTr("Start minimized to the tray"), qsTr("Show a popup when the track changes"), qsTr("Popup duration"), qsTr("Playlist"), qsTr("Show the playlist as a tree")]})
        function reloadCategory(category) {
            const key = category === "controls" ? "controlsPreferences" : category === "privacy" ? "privacyPreferences" : category === "library" ? "libraryPreferences" : category === "subtitles" ? "subtitlesPreferences" : category === "video" ? "videoPreferences" : category === "audio" ? "audioPreferences" : category === "playback" ? "playbackPreferences" : category === "interface" ? "interfacePreferences" : ""
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
