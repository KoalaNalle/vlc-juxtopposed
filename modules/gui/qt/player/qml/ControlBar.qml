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
            return (Player.time.isSubHour() && Player.length.isSubHour()) ? "00--00" : "00--00--00"
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
            property Item trackReturnItem: bar.audioButton
            uiScale: VLCStyle.dp(100, VLCStyle.scale) / 100
            background: Rectangle { color: JuxtopposedPalette.color(root.subtitleTimelineVisible ? "#F2191919" : "#1B1B1B") }
            elapsed: root.subtitleTimelineVisible ? root.formatRangeTime(bar.seekItem?.windowStartTicks ?? 0)
                                                  : root.formatTime(Player.time)
            duration: root.subtitleTimelineVisible ? root.formatRangeTime(bar.seekItem?.windowEndTicks ?? 0)
                     : MainCtx.showRemainingTime && Player.remainingTime.valid()
                      ? "-" + root.formatTime(Player.remainingTime)
                      : root.formatTime(Player.length)
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
            onSettingsRequested: playbackSettings.showSettings()
            onQueueRequested: MainCtx.playqueuePanel.visible = !MainCtx.playqueuePanel.visible
            onDurationRequested: {
                if (root.subtitleTimelineVisible) bar.seekItem?.followPosition()
                else MainCtx.showRemainingTime = !MainCtx.showRemainingTime
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
                    bar.trackReturnItem.forceActiveFocus(Qt.PopupFocusReason)
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
                    bar.settingsButton.forceActiveFocus(Qt.PopupFocusReason)
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
                        return Player.time.formatHMS(length.isSubSecond() ? VLCTick.SubSecondFormattedAsMS : 0)
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

                    text: showRemainingTime
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
                        onTapped: (eventPoint, button) => {
                            MainCtx.showRemainingTime = !MainCtx.showRemainingTime
                        }
                    }

                    HoverHandler {
                        id: mediaRemainingTimeHover
                    }

                    Widgets.ToolTipExt {
                        colorContext.palette: theme.palette

                        visible: mediaRemainingTimeHover.hovered && MainCtx.showRemainingTime && Player.rate !== 1
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
