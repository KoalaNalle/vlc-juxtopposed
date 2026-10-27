/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * Visual design by Juxtopposed: https://www.youtube.com/@juxtopposed
 * Original design: https://www.figma.com/community/file/1691465999134370980
 * Code licensed under GPLv2 or later; artwork under CC BY 4.0.
 *****************************************************************************/
import QtQuick
import VLC.Style
import QtQuick.Controls.Basic as C
import QtQuick.Layouts

C.Pane {
    id: root

    property real uiScale: 1
    property var appearance: JuxtopposedPalette
    property bool previewOnly: false
    property url assetBase: "qrc:/juxtopposed/"
    property string elapsed: "00:00:00"
    property string duration: "00:00:00"
    property bool live: false
    property bool durationInteractive: true
    property real position: 0
    property real volume: 0
    property real volumeStep: 0.05
    property real maximumVolume: 1.25
    property bool playing: false
    property bool muted: false
    property bool seekable: false
    property bool queueVisible: false
    property bool canGoPrevious: true
    property bool canGoNext: true
    property var controlLayout: null
    property var describeTile: function(id) { return ({name: String(id), kind: "button"}) }
    property var tileState: function(id) { return ({enabled: false}) }
    property var mediaMetadata: ({})
    property bool editing: false
    readonly property bool customized: !!controlLayout && controlLayout.columns !== undefined
    readonly property Item layoutRow: customRow

    // VLC supplies its SliderBar here so seeking, chapters and shortcuts retain
    // their existing behavior. The default slider also makes this component
    // independently previewable without a running VLC engine.
    property Component seekComponent: defaultSeekComponent
    readonly property Item seekItem: seekLoader.item
    readonly property Item audioButton: customized ? customRow.itemForKind("audio") : staticAudioButton
    readonly property Item subtitlesButton: customized ? customRow.itemForKind("subtitles") : staticSubtitlesButton
    readonly property Item settingsButton: customized ? customRow.itemForKind("settings") : staticSettingsButton
    readonly property real sliderY: timeline.y + seekLoader.y
                                   + (stacked ? 0 : customized ? customScroll.y + customRow.timelineHost.mapToItem(customRow, 0, 0).y : row.y + timelineHost.y)
    readonly property bool stacked: width < 860 * uiScale || (customized && controlLayout.timelineAbove === true)
    readonly property bool compactVolume: width < 600 * uiScale

    signal previousRequested()
    signal playPauseRequested()
    signal nextRequested()
    signal seekRequested(real position)
    signal volumeRequested(real volume)
    signal muteRequested()
    signal audioRequested()
    signal subtitlesRequested()
    signal settingsRequested()
    signal queueRequested()
    signal durationRequested()
    signal requestLockUnlockAutoHide(bool lock)
    signal tileRequested(int tile, var item)
    signal layoutTilePressed(int column, int index, int tile, var item, point position)
    signal layoutTileMoved(var item, point position)
    signal layoutTileReleased(var item, point position)
    signal layoutSelectionRequested(int column, int index)
    signal layoutRemovalRequested(int column, int index)

    padding: 0
    implicitWidth: 640 * uiScale
    implicitHeight: (stacked ? 94 : 54) * uiScale
    hoverEnabled: true

    Accessible.name: qsTr("Player controls")

    // Native sliders consume their seek/volume keys first. Buttons pass
    // left/right here so VLC's toolbar navigation remains available.
    Keys.priority: Keys.AfterItem
    Keys.onLeftPressed: moveFocus(-1)
    Keys.onRightPressed: moveFocus(1)

    function moveFocus(direction) {
        const controls = customized ? customRow.controls().concat(editing ? [] : [seekLoader.item, duration])
                        : [previousButton, playButton, nextButton, seekLoader.item,
                          duration, volumeControl.muteButton, volumeControl.slider, audioButton,
                          subtitlesButton, settingsButton, queueButton]
        const current = controls.findIndex((item) => item && item.activeFocus)
        for (let offset = 1; offset <= controls.length; ++offset) {
            const index = (current + offset * direction + controls.length) % controls.length
            const item = controls[index]
            if (item && item.enabled && item.visible) {
                item.forceActiveFocus(direction > 0 ? Qt.TabFocusReason : Qt.BacktabFocusReason)
                if (customized) revealControl(item)
                return
            }
        }
    }
    function revealControl(item) {
        const point = item.mapToItem(customRow, 0, 0)
        if (point.x < customScroll.contentX) customScroll.contentX = Math.max(0, point.x)
        else if (point.x + item.width > customScroll.contentX + customScroll.width)
            customScroll.contentX = Math.max(0, Math.min(customScroll.contentWidth - customScroll.width, point.x + item.width - customScroll.width))
    }
    function activateTile(id, item) {
        if (editing || previewOnly) return
        switch (describeTile(id).kind) {
        case "play": playPauseRequested(); break
        case "previous": previousRequested(); break
        case "next": nextRequested(); break
        case "audio": audioRequested(); break
        case "subtitles": subtitlesRequested(); break
        case "settings": settingsRequested(); break
        case "queue": queueRequested(); break
        default: tileRequested(id, item); break
        }
    }

    FontLoader {
        id: inter
        source: root.assetBase + "InterVariable.ttf"
    }

    background: Rectangle { color: root.appearance.color("#1B1B1B") }

    RowLayout {
        id: row
        visible: !root.customized
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 20 * root.uiScale
        anchors.rightMargin: 20 * root.uiScale
        y: (root.stacked ? 51 : 11) * root.uiScale
        height: 32 * root.uiScale
        spacing: (root.compactVolume ? 8 : 20) * root.uiScale

        RowLayout {
            spacing: 8 * root.uiScale

            JuxtopposedButton {
                id: previousButton
                appearance: root.appearance
                activeFocusOnTab: !root.previewOnly
                Accessible.ignored: root.previewOnly
                objectName: "juxtopposedPrevious"
                uiScale: root.uiScale
                implicitHeight: 28 * uiScale
                source: root.assetBase + "previous.svg"
                description: qsTr("Previous")
                enabled: root.canGoPrevious
                onClicked: root.previousRequested()
            }

            JuxtopposedButton {
                id: playButton
                appearance: root.appearance
                activeFocusOnTab: !root.previewOnly
                Accessible.ignored: root.previewOnly
                objectName: "juxtopposedPlay"
                uiScale: root.uiScale
                implicitWidth: 54 * uiScale
                implicitHeight: 28 * uiScale
                source: root.assetBase + (root.playing ? "pause.svg" : "play.svg")
                description: root.playing ? qsTr("Pause") : qsTr("Play")
                fillColor: root.playing ? root.appearance.color("#6B6B6B") : root.appearance.playButtonAccent
                background: Rectangle {
                    radius: 6 * root.uiScale
                    color: parent.down ? root.appearance.color("#B16614") : parent.fillColor
                    border.width: parent.visualFocus ? 2 * root.uiScale : 0
                    border.color: root.appearance.color("#FFFFFF")
                }
                onClicked: root.playPauseRequested()
            }

            JuxtopposedButton {
                id: nextButton
                appearance: root.appearance
                activeFocusOnTab: !root.previewOnly
                Accessible.ignored: root.previewOnly
                objectName: "juxtopposedNext"
                uiScale: root.uiScale
                implicitHeight: 28 * uiScale
                source: root.assetBase + "next.svg"
                description: qsTr("Next")
                enabled: root.canGoNext
                onClicked: root.nextRequested()
            }
        }

        Item {
            id: timelineHost
            visible: !root.stacked
            Layout.fillWidth: true
            Layout.minimumWidth: 200 * root.uiScale
            Layout.preferredHeight: 28 * root.uiScale
        }

        Item {
            id: timeline
            objectName: "juxtopposedTimeline"
            parent: root.stacked ? root.contentItem : root.customized ? customRow.timelineHost : timelineHost

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: root.stacked ? undefined : parent.verticalCenter
            anchors.top: root.stacked ? parent.top : undefined
            anchors.leftMargin: root.stacked ? 20 * root.uiScale : 0
            anchors.rightMargin: root.stacked ? 20 * root.uiScale : 0
            anchors.topMargin: root.stacked ? 11 * root.uiScale : 0
            height: 28 * root.uiScale

            Text {
                id: elapsed
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                width: Math.max(76 * root.uiScale, implicitWidth + 16 * root.uiScale)
                text: root.elapsed
                color: root.appearance.color("#8F8F8F")
                font.family: inter.name
                font.pixelSize: 15 * root.uiScale
                horizontalAlignment: Text.AlignHCenter
                Accessible.name: root.live ? qsTr("Position in live window: %1").arg(text)
                                          : qsTr("Elapsed time: %1").arg(text)
                Accessible.ignored: root.previewOnly
            }

            Loader {
                id: seekLoader
                objectName: "juxtopposedSeekLoader"
                anchors.left: elapsed.right
                anchors.right: duration.left
                anchors.leftMargin: 8 * root.uiScale
                anchors.rightMargin: 8 * root.uiScale
                anchors.verticalCenter: parent.verticalCenter
                height: 28 * root.uiScale
                sourceComponent: root.seekComponent
                enabled: root.seekable
            }

            C.ToolButton {
                id: duration
                objectName: "juxtopposedDuration"
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                width: root.duration.length <= 8 ? 76 * root.uiScale
                                                : Math.max(76 * root.uiScale, implicitWidth)
                height: 28 * root.uiScale
                padding: 8 * root.uiScale
                text: root.duration
                enabled: root.durationInteractive
                activeFocusOnTab: !root.previewOnly
                Accessible.ignored: root.previewOnly
                Accessible.name: root.live && !root.durationInteractive ? qsTr("Live stream")
                    : qsTr("Duration or remaining time: %1").arg(text)
                background: Rectangle {
                    radius: 6 * root.uiScale
                    color: parent.hovered ? root.appearance.color("#303030") : "transparent"
                    border.width: parent.visualFocus ? root.uiScale : 0
                    border.color: root.appearance.color("#D37918")
                }
                contentItem: Text {
                    text: duration.text
                    font.family: inter.name
                    font.pixelSize: 15 * root.uiScale
                    color: root.appearance.color("#8F8F8F")
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: root.durationRequested()
            }
        }

        Item {
            visible: root.stacked
            Layout.fillWidth: true
        }

        JuxtopposedVolumeControl {
            id: volumeControl
            uiScale: root.uiScale
            appearance: root.appearance
            assetBase: root.assetBase
            previewOnly: root.previewOnly
            compactVolume: root.compactVolume
            maximumVolume: root.maximumVolume
            volumeStep: root.volumeStep
            volume: root.volume
            muted: root.muted
            onVolumeRequested: value => root.volumeRequested(value)
            onMuteRequested: root.muteRequested()
            onRequestLockUnlockAutoHide: lock => root.requestLockUnlockAutoHide(lock)
        }

        RowLayout {
            spacing: 12 * root.uiScale

            JuxtopposedButton {
                id: staticAudioButton
                appearance: root.appearance
                activeFocusOnTab: !root.previewOnly
                Accessible.ignored: root.previewOnly
                objectName: "juxtopposedAudio"
                uiScale: root.uiScale
                source: root.assetBase + root.appearance.icon("audio.svg")
                description: qsTr("Audio tracks")
                onClicked: root.audioRequested()
            }
            JuxtopposedButton {
                id: staticSubtitlesButton
                appearance: root.appearance
                activeFocusOnTab: !root.previewOnly
                Accessible.ignored: root.previewOnly
                objectName: "juxtopposedSubtitles"
                uiScale: root.uiScale
                source: root.assetBase + root.appearance.icon("subtitles.svg")
                description: qsTr("Subtitle tracks")
                onClicked: root.subtitlesRequested()
            }
            JuxtopposedButton {
                id: staticSettingsButton
                appearance: root.appearance
                activeFocusOnTab: !root.previewOnly
                Accessible.ignored: root.previewOnly
                objectName: "juxtopposedSettings"
                uiScale: root.uiScale
                source: root.assetBase + "settings.svg"
                description: qsTr("Playback settings")
                onClicked: root.settingsRequested()
            }
        }

        JuxtopposedButton {
            id: queueButton
            appearance: root.appearance
            activeFocusOnTab: !root.previewOnly
            Accessible.ignored: root.previewOnly
            objectName: "juxtopposedQueue"
            uiScale: root.uiScale
            source: root.assetBase + root.appearance.icon("queue.svg")
            description: qsTr("Play queue")
            fillColor: root.queueVisible ? root.appearance.color("#371F06") : "transparent"
            Accessible.checkable: true
            Accessible.checked: root.queueVisible
            onClicked: root.queueRequested()
        }
    }

    Flickable {
        id: customScroll
        objectName: "juxtopposedCustomControls"
        visible: root.customized
        anchors.left: parent.left; anchors.right: parent.right
        anchors.leftMargin: 20 * root.uiScale; anchors.rightMargin: 20 * root.uiScale
        y: (root.stacked ? 51 : 11) * root.uiScale
        height: 32 * root.uiScale
        clip: true
        contentWidth: Math.max(width, customRow.implicitWidth)
        contentHeight: height
        interactive: contentWidth > width && !root.editing
        boundsBehavior: Flickable.StopAtBounds
        C.ScrollBar.horizontal: C.ScrollBar {policy: customScroll.contentWidth > customScroll.width ? C.ScrollBar.AsNeeded : C.ScrollBar.AlwaysOff}
        JuxtopposedLayoutRow {
            id: customRow
            dropViewport: customScroll
            width: customScroll.contentWidth; height: customScroll.height
            uiScale: root.uiScale; appearance: root.appearance; assetBase: root.assetBase
            columns: root.customized ? root.controlLayout.columns : [[], [], []]
            timelineInline: !root.stacked
            editing: root.editing
            describe: root.describeTile
            state: root.tileState
            metadata: root.mediaMetadata
            volumeComponent: Component {
                JuxtopposedVolumeControl {
                    uiScale: root.uiScale; appearance: root.appearance; assetBase: root.assetBase
                    previewOnly: root.previewOnly || root.editing
                    compactVolume: root.compactVolume; maximumVolume: root.maximumVolume
                    volumeStep: root.volumeStep; volume: root.volume; muted: root.muted
                    onVolumeRequested: value => {if (!root.editing && !root.previewOnly) root.volumeRequested(value)}
                    onMuteRequested: if (!root.editing && !root.previewOnly) root.muteRequested()
                    onRequestLockUnlockAutoHide: lock => root.requestLockUnlockAutoHide(lock)
                }
            }
            onActivated: (tile, item) => root.activateTile(tile, item)
            onTilePressed: (column, index, tile, item, position) => root.layoutTilePressed(column, index, tile, item, position)
            onTileMoved: (item, position) => root.layoutTileMoved(item, position)
            onTileReleased: (item, position) => root.layoutTileReleased(item, position)
            onSelectionRequested: (column, index) => root.layoutSelectionRequested(column, index)
            onRemovalRequested: (column, index) => root.layoutRemovalRequested(column, index)
            onRevealRequested: item => root.revealControl(item)
        }
    }

    Component {
        id: defaultSeekComponent
        C.Slider {
            objectName: "juxtopposedSeek"
            padding: 0
            from: 0
            to: 1
            value: root.position
            activeFocusOnTab: !root.previewOnly
            Accessible.ignored: root.previewOnly
            Accessible.name: qsTr("Progress")
            onMoved: root.seekRequested(value)
            onPressedChanged: root.requestLockUnlockAutoHide(pressed)
            handle: Item { implicitWidth: 0; implicitHeight: 0 }
            background: JuxtopposedSeekBackground {
                appearance: root.appearance
                width: parent.width
                height: parent.height
                position: parent.position
                uiScale: root.uiScale
            }
        }
    }
}
