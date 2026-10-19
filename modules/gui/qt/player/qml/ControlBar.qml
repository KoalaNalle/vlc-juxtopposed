/*****************************************************************************
 * Copyright (C) 2019 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * ( at your option ) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import QtQml.Models


import VLC.MainInterface
import VLC.Style
import VLC.Widgets as Widgets
import VLC.Player
import VLC.PlayerControls
import VLC.Util
import VLC.Playlist
import VLC.Dialogs

T.Pane {
    id: root

    enum TimeTextPosition {
        Hide,
        AboveSlider,
        LeftRightSlider
    }

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    topInset: redesigned ? 0 : sliderY

    bottomPadding: VLCStyle.margin_xsmall

    readonly property bool redesigned: Qt.platform.os === "windows"
                                       && MainCtx.juxtopposedControls
                                       && identifier !== PlayerControlbarModel.Miniplayer
    readonly property real sliderY: contentItem.y + (contentItem.item?.sliderY ?? 0)
    property int textPosition: ControlBar.TimeTextPosition.AboveSlider
    property int identifier: -1
    property real sliderHeight: VLCStyle.heightBar_xxsmall
    property real bookmarksHeight: VLCStyle.controlBarBookmarksHeight
    property bool showRemainingTime: true
    property bool subtitleTimelineVisible: false

    Connections {
        target: Player
        function onInputChanged(hasInput) { root.subtitleTimelineVisible = false }
    }

    property var menu: undefined

    signal requestLockUnlockAutoHide(bool lock)

    ///incomming signal to request all menu and popup to close
    signal forceUnlock()

    enabled: visible

    Keys.priority: Keys.AfterItem
    Keys.onPressed: (event) => root.Navigation.defaultKeyAction(event)
    Navigation.cancelAction: function() { History.previous(Qt.BacktabFocusReason) }

    Accessible.name: qsTr("Player controls")
    AccessibleCompat.id: "playerControlBar"

    function showChapterMarks() {
        const slider = redesigned ? contentItem.item?.seekItem
                                  : contentItem.item?.trackPositionSlider
        if (slider)
            slider.showChapterMarks()
    }

    function applyMenu(menu) {
        if (root.menu === menu)
            return

        // NOTE: When applying a new menu we hide the previous one.
        if (menu)
            dismiss()

        root.menu = menu
    }

    function dismiss() {
        if ((typeof menu === undefined) || !menu)
            return
        if (menu.hasOwnProperty("dismiss"))
            menu.dismiss()
        else if (menu.hasOwnProperty("close"))
            menu.close()
    }

    readonly property ColorContext colorContext: ColorContext {
        id: theme
        colorSet: ColorContext.Window
    }

    TextMetrics {
        id: timeTextMetrics
        font.pixelSize: (textPosition === ControlBar.TimeTextPosition.LeftRightSlider) ? VLCStyle.fontSize_small
                                                                                       : VLCStyle.fontSize_normal
        text: {
            // This text metrics is used for both the time and length, as we want them to have the same width.
            // Length might not be reported in some cases, for that reason we should check both and use the bigger one.

            // Some extra space to compensate non-monospaced fonts ("-" takes more space than ":", double "-" for guarantee)
            return (Player.displayTime.isSubHour() && Player.length.isSubHour()) ? "00--00" : "00--00--00"
        }
    }

    background: Rectangle {
        color: theme.bg.primary
    }

    contentItem: Loader {
        focus: true
        sourceComponent: root.redesigned ? redesignedControls : traditionalControls
    }

    Component {
        id: redesignedControls

        JuxtopposedPlaybackBar {
            id: bar
            objectName: "juxtopposedLivePlaybackBar"
            property Item trackReturnItem: bar.audioButton
            property Item settingsReturnItem: bar.settingsButton
            readonly property var profileLayout: MainCtx.playerProfiles.layout
            // Keep the native seek/control objects alive beneath the modal.
            // Only the inert draft preview is drawn while the layout is edited.
            opacity: profileLayout.editing ? 0 : 1
            controlLayout: root.identifier === PlayerControlbarModel.Audioplayer ? profileLayout.current.audio
                : profileLayout.defaultVideo ? null : profileLayout.current.video
            describeTile: id => profileLayout.tileInfo(id)
            mediaMetadata: ({file: Player.fileName, artist: Player.artist, folder: Player.folderName,
                              playlist: MainCtx.currentMedia.playlistName, artwork: Player.artwork.toString().length ? VLCAccessImage.uri(Player.artwork) : VLCStyle.noArtAlbumCover})
            tileState: function(id) {
                const info = profileLayout.tileInfo(id)
                const result = {enabled: Player.isStarted, glyph: info.glyph, fontFamily: info.fontFamily}
                switch (id) {
                case ControlListModel.PLAY_BUTTON:
                    result.enabled = !MainPlaylistController.empty
                    result.name = bar.playing ? qsTr("Pause") : qsTr("Play")
                    result.playing = bar.playing
                    result.source = bar.assetBase + (bar.playing ? "pause.svg" : "play.svg")
                    result.glyph = ""; break
                case ControlListModel.PREVIOUS_BUTTON:
                    result.enabled = MainPlaylistController.hasPrev; result.source = bar.assetBase + "previous.svg"; result.glyph = ""; break
                case ControlListModel.NEXT_BUTTON:
                    result.enabled = MainPlaylistController.hasNext; result.source = bar.assetBase + "next.svg"; result.glyph = ""; break
                case ControlListModel.JUXTO_AUDIO_TRACKS:
                    result.enabled = Player.audioTracks.count > 0; result.source = bar.assetBase + JuxtopposedPalette.icon("audio.svg"); result.glyph = ""; break
                case ControlListModel.LANG_BUTTON:
                    result.enabled = Player.isStarted; result.source = bar.assetBase + JuxtopposedPalette.icon("subtitles.svg"); result.glyph = ""; break
                case ControlListModel.EXTENDED_BUTTON:
                    result.enabled = true; result.source = bar.assetBase + "settings.svg"; result.glyph = ""; break
                case ControlListModel.PLAYLIST_BUTTON: case ControlListModel.JUXTO_RIGHT_SIDEBAR:
                    result.enabled = true; result.checked = MainCtx.playqueuePanel.visible; break
                case ControlListModel.PLAYBACK_SPEED_BUTTON: result.enabled = Player.isStarted && Player.ratechangable; break
                case ControlListModel.SKIP_FW_BUTTON: case ControlListModel.SKIP_BACK_BUTTON:
                case ControlListModel.JUXTO_FORWARD_15: case ControlListModel.JUXTO_BACKWARD_15:
                case ControlListModel.JUXTO_REPLAY: result.enabled = Player.isStarted && Player.seekable; break
                case ControlListModel.LOOP_BUTTON:
                    result.enabled = true; result.checked = MainPlaylistController.repeatMode !== PlaylistController.PLAYBACK_REPEAT_NONE
                    result.glyph = MainPlaylistController.repeatMode === PlaylistController.PLAYBACK_REPEAT_CURRENT ? VLCIcons.repeat_one : VLCIcons.repeat_all
                    result.name = MainPlaylistController.repeatMode === PlaylistController.PLAYBACK_REPEAT_CURRENT ? qsTr("Repeat current item")
                        : MainPlaylistController.repeatMode === PlaylistController.PLAYBACK_REPEAT_ALL ? qsTr("Repeat all") : qsTr("Repeat off"); break
                case ControlListModel.RANDOM_BUTTON: result.enabled = true; result.checked = MainPlaylistController.random; break
                case ControlListModel.RECORD_BUTTON: result.checked = Player.recording; break
                case ControlListModel.SNAPSHOT_BUTTON: case ControlListModel.JUXTO_ZOOM:
                    result.enabled = Player.hasVideoOutput; break
                case ControlListModel.JUXTO_PICTURE_IN_PICTURE:
                    result.enabled = Player.hasVideoOutput && MainCtx.canShowVideoPIP
                    result.note = result.enabled ? qsTr("Picture in Picture") : qsTr("This video output cannot keep an embedded picture-in-picture view."); break
                case ControlListModel.JUXTO_HDR:
                    result.enabled = false; result.note = qsTr("HDR switching is not available in this build."); break
                case ControlListModel.JUXTO_FAVORITE:
                    // Keep keyboard focus while the checked native save or a
                    // background refresh is pending. The action guard owns busy.
                    result.enabled = MainCtx.currentMedia.available
                    result.busy = MainCtx.currentMedia.busy
                    result.checked = MainCtx.currentMedia.favorite
                    result.name = result.checked ? qsTr("Remove from favorites") : qsTr("Add to favorites")
                    result.note = MainCtx.currentMedia.error || (MainCtx.currentMedia.busy ? qsTr("Updating favorite…")
                        : !MainCtx.mediaLibraryAvailable ? qsTr("Favorites require the media library.")
                        : !MainCtx.currentMedia.available ? qsTr("This media is not in the library.") : result.name); break
                case ControlListModel.BOOKMARK_BUTTON:
                    result.enabled = Player.isStarted && Player.seekable && (bookmarkActions.item?.canAdd ?? false)
                    result.note = MainCtx.mediaLibraryAvailable ? qsTr("Add a bookmark at the current position") : qsTr("Bookmarks require the media library."); break
                case ControlListModel.JUXTO_BOOKMARKS:
                    result.enabled = MainCtx.mediaLibraryAvailable; break
                case ControlListModel.JUXTO_CHAPTERS:
                    result.enabled = Player.hasChapters; break
                case ControlListModel.JUXTO_PLAYLISTS: result.enabled = MainCtx.mediaLibraryAvailable; break
                case ControlListModel.JUXTO_VLC_SETTINGS: case ControlListModel.JUXTO_GO_HOME: case ControlListModel.QUIT_BUTTON:
                case ControlListModel.FULLSCREEN_BUTTON: result.enabled = true; break
                }
                return result
            }
            uiScale: VLCStyle.dp(100, VLCStyle.scale) / 100
            background: Rectangle { color: JuxtopposedPalette.color(root.subtitleTimelineVisible ? "#F2191919" : "#1B1B1B") }
            elapsed: root.subtitleTimelineVisible ? root.formatRangeTime(bar.seekItem?.windowStartTicks ?? 0)
                                                  : root.formatTime(Player.displayTime)
            duration: root.subtitleTimelineVisible ? root.formatRangeTime(bar.seekItem?.windowEndTicks ?? 0)
                     : Player.live ? qsTr("LIVE")
                     : MainCtx.showRemainingTime && Player.remainingTime.valid()
                      ? "-" + root.formatTime(Player.remainingTime)
                      : root.formatTime(Player.length)
            live: Player.live && !root.subtitleTimelineVisible
            durationInteractive: root.subtitleTimelineVisible || !Player.live
            volume: Player.volume
            volumeStep: ("audioPreferences" in MainCtx && MainCtx.audioPreferences
                         && MainCtx.audioPreferences.capabilities.volumeStep === true
                         && typeof MainCtx.audioPreferences.values.volumeStep === "number"
                         && Number.isFinite(Number(MainCtx.audioPreferences.values.volumeStep))
                         && MainCtx.audioPreferences.values.volumeStep > 0)
                ? MainCtx.audioPreferences.values.volumeStep / 100 : 0.05
            maximumVolume: MainCtx.maxVolume / 100
            muted: Player.muted
            playing: Player.playingState === Player.PLAYING_STATE_PLAYING
            seekable: Player.seekable && (Player.playingState === Player.PLAYING_STATE_PLAYING
                                         || Player.playingState === Player.PLAYING_STATE_PAUSED)
            queueVisible: MainCtx.playqueuePanel.visible
            canGoPrevious: MainPlaylistController.hasPrev
            canGoNext: MainPlaylistController.hasNext

            onPreviousRequested: MainPlaylistController.prev()
            onPlayPauseRequested: MainPlaylistController.togglePlayPause()
            onNextRequested: MainPlaylistController.next()
            onMuteRequested: Player.muted = !Player.muted
            onVolumeRequested: (value) => {
                Player.muted = false
                Player.volume = value
            }
            onAudioRequested: {
                trackReturnItem = audioButton
                tracksMenu.showTracks(1)
            }
            onSubtitlesRequested: {
                if (root.subtitleTimelineVisible) {
                    root.subtitleTimelineVisible = false
                    return
                }
                trackReturnItem = subtitlesButton
                tracksMenu.showTracks(0)
            }
            onSettingsRequested: {settingsReturnItem = settingsButton; playbackSettings.showSettings()}
            onTileRequested: (tile, item) => {
                const state = tileState(tile)
                if (state.enabled !== true || state.busy === true) return
                switch (tile) {
                case ControlListModel.PLAYBACK_SPEED_BUTTON: settingsReturnItem = item; playbackSettings.showSettings(0,0); break
                case ControlListModel.SKIP_FW_BUTTON: Player.jumpSeconds(10); break
                case ControlListModel.SKIP_BACK_BUTTON: Player.jumpSeconds(-10); break
                case ControlListModel.JUXTO_FORWARD_15: Player.jumpSeconds(15); break
                case ControlListModel.JUXTO_BACKWARD_15: Player.jumpSeconds(-15); break
                case ControlListModel.STOP_BUTTON: MainPlaylistController.stop(); break
                case ControlListModel.LOOP_BUTTON: MainPlaylistController.toggleRepeatMode(); break
                case ControlListModel.RANDOM_BUTTON: MainPlaylistController.toggleRandom(); break
                case ControlListModel.RECORD_BUTTON: Player.toggleRecord(); break
                case ControlListModel.SNAPSHOT_BUTTON: Player.snapshot(); break
                case ControlListModel.JUXTO_PICTURE_IN_PICTURE: MainCtx.playerView = false; break
                case ControlListModel.JUXTO_ZOOM: settingsReturnItem = item; playbackSettings.showSettings(1,4); break
                case ControlListModel.JUXTO_RIGHT_SIDEBAR: MainCtx.playqueuePanel.visible = !MainCtx.playqueuePanel.visible; break
                case ControlListModel.QUIT_BUTTON: MainCtx.close(); break
                case ControlListModel.JUXTO_VLC_SETTINGS: DialogsProvider.prefsDialog(); break
                case ControlListModel.JUXTO_GO_HOME: History.push(["home"], {}, Qt.TabFocusReason); MainCtx.playerView = false; break
                case ControlListModel.INFO_BUTTON: DialogsProvider.mediaInfoDialog(); break
                case ControlListModel.JUXTO_REPLAY: Player.jumpToPos(0); break
                case ControlListModel.JUXTO_FAVORITE: MainCtx.currentMedia.setFavorite(!MainCtx.currentMedia.favorite); break
                case ControlListModel.BOOKMARK_BUTTON:
                    bookmarkActions.item.add(); MainCtx.playqueuePanel.activeTab = 1; MainCtx.playqueuePanel.visible = true; break
                case ControlListModel.JUXTO_BOOKMARKS: MainCtx.playqueuePanel.activeTab = 1; MainCtx.playqueuePanel.visible = true; break
                case ControlListModel.JUXTO_CHAPTERS: MainCtx.playqueuePanel.activeTab = 2; MainCtx.playqueuePanel.visible = true; break
                case ControlListModel.JUXTO_PLAYLISTS: History.push(["playlists"], {}, Qt.TabFocusReason); MainCtx.playerView = false; break
                case ControlListModel.FULLSCREEN_BUTTON: Player.toggleFullscreen(); break
                }
            }
            onQueueRequested: MainCtx.playqueuePanel.visible = !MainCtx.playqueuePanel.visible
            onDurationRequested: {
                if (root.subtitleTimelineVisible) bar.seekItem?.followPosition()
                else if (!Player.live) MainCtx.showRemainingTime = !MainCtx.showRemainingTime
            }
            onRequestLockUnlockAutoHide: (lock) => root.requestLockUnlockAutoHide(lock)

            // Retain VLC's native slider and chapter-mark behavior. The
            // selected-source cue view replaces only its visible presentation.
            seekComponent: Component {
                FocusScope {
                    id: seekHost
                    activeFocusOnTab: true
                    readonly property real windowStartTicks: cueLane.windowStartTicks
                    readonly property real windowEndTicks: cueLane.windowEndTicks
                    function showChapterMarks() { nativeSeek.showChapterMarks() }
                    function followPosition() { cueLane.followPosition() }
                    onActiveFocusChanged: if (activeFocus)
                        (root.subtitleTimelineVisible ? cueLane : nativeSeek).forceActiveFocus(Qt.TabFocusReason)
                    SliderBar {
                        id: nativeSeek
                        anchors.fill: parent
                        visible: !root.subtitleTimelineVisible
                        activeFocusOnTab: true
                        Accessible.role: Accessible.Slider
                        Accessible.name: qsTr("Progress")
                        AccessibleCompat.id: "progressSlider"
                        barHeight: VLCStyle.dp(28, VLCStyle.scale)
                        showHandle: false
                        waveformStatus: Player.videoTracks.count === 0 && Player.audioTracks.count > 1
                                        ? qsTr("Waveform is available for files with a single audio track.")
                                        : (Player.waveform.enabled
                                           ? (Player.waveform.loading ? qsTr("Preparing waveform…")
                                                                     : Player.waveform.error) : "")
                        Navigation.parentItem: root
                        Navigation.upItem: root.Navigation.upItem
                        Keys.onPressed: (event) => Navigation.defaultKeyAction(event)
                        background: JuxtopposedSeekBackground {
                            width: nativeSeek.availableWidth
                            height: nativeSeek.barHeight
                            position: nativeSeek.visualPosition
                            uiScale: bar.uiScale
                            waveformPeaks: Player.waveform.available ? Player.waveform.peaks : []
                        }
                    }
                    JuxtopposedSubtitleTimelineAdapter {
                        id: cueLane
                        anchors.fill: parent
                        visible: root.subtitleTimelineVisible
                        onCloseRequested: { root.subtitleTimelineVisible = false; nativeSeek.forceActiveFocus(Qt.PopupFocusReason) }
                        onRequestLockUnlockAutoHide: lock => root.requestLockUnlockAutoHide(lock)
                    }
                }
            }

            Loader {
                id: bookmarkActions
                active: MainCtx.mediaLibraryAvailable
                source: "qrc:///qt/qml/VLC/Playlist/JuxtopposedBookmarkActions.qml"
            }

            TracksMenu {
                id: tracksMenu
                parent: root
                x: Math.max(0, root.width - width)
                y: -height
                colorContext.palette: theme.palette
                onOpened: {
                    root.requestLockUnlockAutoHide(true)
                    root.applyMenu(tracksMenu)
                }
                onClosed: {
                    root.requestLockUnlockAutoHide(false)
                    if (root.menu === tracksMenu)
                        root.menu = undefined
                    const target = bar.trackReturnItem && bar.trackReturnItem.visible && bar.trackReturnItem.enabled ? bar.trackReturnItem : bar.seekItem
                    target?.forceActiveFocus(Qt.PopupFocusReason)
                }
            }

            PlaybackSettingsMenu {
                id: playbackSettings
                subtitleTimelineVisible: root.subtitleTimelineVisible
                onSubtitleTimelineRequested: enabled => {
                    root.subtitleTimelineVisible = enabled
                    close()
                    bar.seekItem?.forceActiveFocus(Qt.PopupFocusReason)
                }
                parent: root
                x: Math.max(0, root.width - width)
                y: -height
                maximumHeight: MainCtx.intfMainWindow.height - root.height - VLCStyle.margin_normal * 2
                colorContext.palette: theme.palette
                onOpened: {
                    root.requestLockUnlockAutoHide(true)
                    root.applyMenu(playbackSettings)
                }
                onClosed: {
                    root.requestLockUnlockAutoHide(false)
                    if (root.menu === playbackSettings)
                        root.menu = undefined
                    const target = bar.settingsReturnItem && bar.settingsReturnItem.visible && bar.settingsReturnItem.enabled ? bar.settingsReturnItem : bar.seekItem
                    target?.forceActiveFocus(Qt.PopupFocusReason)
                }
            }

            Connections {
                target: root
                function onForceUnlock() {
                    tracksMenu.close()
                    playbackSettings.close()
                }
            }
        }
    }

    // The original cue variant labels the visible range endpoints with *.
    // They are source/media times, distinct from the ordinary elapsed clock.
    function formatRangeTime(ticks) {
        const seconds = Math.max(0, Math.floor(ticks / 1000000))
        return String(Math.floor(seconds / 3600)).padStart(2, "0") + ":"
             + String(Math.floor(seconds / 60) % 60).padStart(2, "0") + ":"
             + String(seconds % 60).padStart(2, "0") + "*"
    }

    function formatTime(tick) {
        if (!tick.valid())
            return "--:--:--"
        const text = tick.formatHMS(0)
        return tick.isSubHour() ? "00:" + text : text
    }

    Component {
        id: traditionalControls
        ColumnLayout {
            spacing: VLCStyle.margin_xsmall

            readonly property real sliderY: row2.y

            property alias trackPositionSlider: trackPositionSlider

            property alias mediaRemainingTime: mediaRemainingTime

            property alias playerControlLayout: playerControlLayout

            readonly property list<Item> strayItems: [
                T.Label {
                    id: mediaTime

                    Layout.preferredWidth: (textPosition === ControlBar.TimeTextPosition.LeftRightSlider) ? timeTextMetrics.width : -1

                    text: {
                        const length = Player.length
                        return Player.displayTime.formatHMS(length.isSubSecond() ? VLCTick.SubSecondFormattedAsMS : 0)
                    }
                    color: theme.fg.primary
                    font: timeTextMetrics.font
                    horizontalAlignment: Text.AlignHCenter

                    // TODO: Do not use `Binding` when minimum Qt is 6.12:
                    Binding {
                        target: mediaTime
                        property: "mutabilityGroup"
                        when: (mediaTime.mutabilityGroup !== undefined)
                        value: (Player.playingState === Player.PLAYING_STATE_PLAYING) ? Item.ModerateMutabilityGroup
                                                                                      : Item.StaticMutabilityGroup
                    }

                    anchors.left: (parent === pseudoRow) ? parent.left : undefined
                    anchors.verticalCenter: (parent === pseudoRow) ? parent.verticalCenter : undefined
                },
                T.Label {
                    id: mediaRemainingTime

                    Layout.preferredWidth: (textPosition === ControlBar.TimeTextPosition.LeftRightSlider) ? timeTextMetrics.width : -1

                    readonly property bool showRemainingTime: (MainCtx.showRemainingTime && Player.remainingTime.valid())

                    text: Player.live ? qsTr("LIVE") : showRemainingTime
                          ? "-" + Player.remainingTime.formatHMS()
                          : Player.length.formatHMS()
                    color: mediaTime.color
                    font: timeTextMetrics.font
                    horizontalAlignment: Text.AlignHCenter

                    visible: root.showRemainingTime

                    // TODO: Do not use `Binding` when minimum Qt is 6.12:
                    Binding {
                        target: mediaRemainingTime
                        property: "mutabilityGroup"
                        when: (mediaRemainingTime.mutabilityGroup !== undefined)
                        value: ((Player.playingState === Player.PLAYING_STATE_PLAYING) &&
                                mediaRemainingTime.showRemainingTime) ? Item.ModerateMutabilityGroup
                                                                      : Item.StaticMutabilityGroup
                    }

                    anchors.right: (parent === pseudoRow) ? parent.right : undefined
                    anchors.verticalCenter: (parent === pseudoRow) ? parent.verticalCenter : undefined

                    TapHandler {
                        enabled: !Player.live
                        onTapped: (eventPoint, button) => {
                            MainCtx.showRemainingTime = !MainCtx.showRemainingTime
                        }
                    }

                    HoverHandler {
                        id: mediaRemainingTimeHover
                    }

                    Widgets.ToolTipExt {
                        colorContext.palette: theme.palette

                        visible: !Player.live && mediaRemainingTimeHover.hovered && MainCtx.showRemainingTime && Player.rate !== 1
                        delay: VLCStyle.delayToolTipAppear
                        text: {
                            if (!visible)
                                return ""

                            const playbackSpeed = Math.abs(Player.rate)
                            const adjustedRemainingTime = Player.remainingTime.scale(1 / playbackSpeed)
                                ? Player.remainingTime.scale(1 / playbackSpeed)
                                : Player.remainingTime

                            return qsTr("%1 (%2x)")
                                .arg(adjustedRemainingTime.formatHMS())
                                .arg(+Player.rate.toFixed(2))
                        }

                        x: (parent.width - width) / 2
                    }
                },
                SliderBar {
                    id: trackPositionSlider

                    barHeight: root.sliderHeight

                    Accessible.role: Accessible.Slider
                    Accessible.name: qsTr("Progress")
                    AccessibleCompat.id: "progressSlider"

                    Layout.fillWidth: true
                    enabled: Player.playingState === Player.PLAYING_STATE_PLAYING || Player.playingState === Player.PLAYING_STATE_PAUSED

                    Navigation.parentItem: root
                    Navigation.downItem: playerControlLayout

                    activeFocusOnTab: true

                    focus: true

                    Keys.onPressed: (event) => {
                        Navigation.defaultKeyAction(event)
                    }
                },
                Loader {
                    id: bookmarksLoader

                    parent: root
                    active: MainCtx.mediaLibraryAvailable
                    source: "qrc:///qt/qml/VLC/MediaLibrary/Bookmarks.qml"

                    x: root.leftPadding + trackPositionSlider.x + row2.Layout.leftMargin
                    y: row2.y + row2.height + VLCStyle.margin_xxsmall
                    width: trackPositionSlider.width

                    onLoaded: {
                       item.barHeight = Qt.binding(function() { return bookmarksHeight })
                       item.controlBarHovered = Qt.binding(function() { return root.hovered })
                       item.yShift = Qt.binding(function() { return row2.height + VLCStyle.margin_xxsmall })
                    }
                }
            ]

            Item {
                // BUG: RowLayout can not be used here
                // because of a Qt bug. (Height is
                // incorrectly determined. Could be
                // about nested layouting).

                id: pseudoRow
                Layout.fillWidth: true
                Layout.fillHeight: false
                Layout.leftMargin: VLCStyle.margin_normal
                Layout.rightMargin: VLCStyle.margin_normal

                implicitHeight: visible ? Math.max(mediaTime.implicitHeight, mediaRemainingTime.implicitHeight) : 0

                visible: children.length > 0

                children: {
                    switch (textPosition) {
                    case ControlBar.TimeTextPosition.AboveSlider:
                        return [mediaTime, mediaRemainingTime]
                    case ControlBar.TimeTextPosition.Hide:
                    case ControlBar.TimeTextPosition.LeftRightSlider:
                    default:
                        return []
                    }
                }
            }

            RowLayout {
                id: row2

                children: {
                    switch (textPosition) {
                    case ControlBar.TimeTextPosition.Hide:
                    case ControlBar.TimeTextPosition.AboveSlider:
                        return [trackPositionSlider]
                    case ControlBar.TimeTextPosition.LeftRightSlider:
                        return [mediaTime, trackPositionSlider, mediaRemainingTime]
                    default:
                        return []
                    }
                }

                visible: children.length > 0

                spacing: VLCStyle.margin_xsmall

                Layout.fillWidth: true
                Layout.fillHeight: false
                Layout.leftMargin: (textPosition === ControlBar.TimeTextPosition.LeftRightSlider) ? VLCStyle.margin_xsmall
                                                                                                  : 0

                Layout.rightMargin: Layout.leftMargin
            }

            PlayerControlLayout {
                id: playerControlLayout

                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.leftMargin: VLCStyle.margin_large
                Layout.rightMargin: VLCStyle.margin_large

                identifier: root.identifier

                implicitHeight: MainCtx.pinVideoControls ? VLCStyle.controlLayoutHeightPinned
                                                         : VLCStyle.controlLayoutHeight

                visible: !model?.empty ?? false

                Navigation.upItem: trackPositionSlider.enabled ? trackPositionSlider : root.Navigation.upItem

                onRequestLockUnlockAutoHide: (lock) => root.requestLockUnlockAutoHide(lock)

                onMenuOpened: (menu) => root.applyMenu(menu)

                Component.onCompleted: {
                    root.forceUnlock.connect(playerControlLayout.forceUnlock)
                }
            }
        }
    }
}
