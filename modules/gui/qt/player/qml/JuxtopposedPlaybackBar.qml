/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * Visual design by Juxtopposed: https://www.youtube.com/@juxtopposed
 * Original design: https://www.figma.com/community/file/1691465999134370980
 * Code licensed under GPLv2 or later; artwork under CC BY 4.0.
 *****************************************************************************/
import QtQuick
import QtQuick.Controls.Basic as C
import QtQuick.Layouts

C.Pane {
    id: root

    property real uiScale: 1
    property url assetBase: "qrc:/juxtopposed/"
    property string elapsed: "00:00:00"
    property string duration: "00:00:00"
    property real position: 0
    property real volume: 0
    property real maximumVolume: 1.25
    property bool playing: false
    property bool muted: false
    property bool seekable: false
    property bool queueVisible: false
    property bool canGoPrevious: true
    property bool canGoNext: true

    // VLC supplies its SliderBar here so seeking, chapters and shortcuts retain
    // their existing behavior. The default slider also makes this component
    // independently previewable without a running VLC engine.
    property Component seekComponent: defaultSeekComponent
    readonly property Item seekItem: seekLoader.item
    property alias audioButton: audioButton
    property alias subtitlesButton: subtitlesButton
    property alias settingsButton: settingsButton
    readonly property real sliderY: timeline.y + seekLoader.y
                                   + (stacked ? 0 : row.y + timelineHost.y)
    readonly property bool stacked: width < 860 * uiScale
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
        const controls = [previousButton, playButton, nextButton, seekLoader.item,
                          duration, muteButton, volume, audioButton,
                          subtitlesButton, settingsButton, queueButton]
        const current = controls.findIndex((item) => item && item.activeFocus)
        for (let offset = 1; offset <= controls.length; ++offset) {
            const index = (current + offset * direction + controls.length) % controls.length
            const item = controls[index]
            if (item && item.enabled && item.visible) {
                item.forceActiveFocus(direction > 0 ? Qt.TabFocusReason : Qt.BacktabFocusReason)
                return
            }
        }
    }

    FontLoader {
        id: inter
        source: root.assetBase + "InterVariable.ttf"
    }

    background: Rectangle { color: "#1B1B1B" }

    RowLayout {
        id: row
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
                objectName: "juxtopposedPlay"
                uiScale: root.uiScale
                implicitWidth: 54 * uiScale
                implicitHeight: 28 * uiScale
                source: root.assetBase + (root.playing ? "pause.svg" : "play.svg")
                description: root.playing ? qsTr("Pause") : qsTr("Play")
                fillColor: root.playing ? "#6B6B6B" : "#D37918"
                background: Rectangle {
                    radius: 6 * root.uiScale
                    color: parent.down ? "#B16614" : parent.fillColor
                    border.width: parent.visualFocus ? 2 * root.uiScale : 0
                    border.color: "#FFFFFF"
                }
                onClicked: root.playPauseRequested()
            }

            JuxtopposedButton {
                id: nextButton
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
            parent: root.stacked ? root.contentItem : timelineHost

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
                color: "#8F8F8F"
                font.family: inter.name
                font.pixelSize: 15 * root.uiScale
                horizontalAlignment: Text.AlignHCenter
                Accessible.ignored: true
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
                activeFocusOnTab: true
                Accessible.name: qsTr("Duration or remaining time: %1").arg(text)
                background: Rectangle {
                    radius: 6 * root.uiScale
                    color: parent.hovered ? "#303030" : "transparent"
                    border.width: parent.visualFocus ? root.uiScale : 0
                    border.color: "#D37918"
                }
                contentItem: Text {
                    text: duration.text
                    font.family: inter.name
                    font.pixelSize: 15 * root.uiScale
                    color: "#8F8F8F"
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

        RowLayout {
            spacing: 8 * root.uiScale

            JuxtopposedButton {
                id: muteButton
                objectName: "juxtopposedMute"
                uiScale: root.uiScale
                implicitWidth: 24 * uiScale
                source: root.assetBase + (root.muted ? "volume-muted.svg" : "volume.svg")
                mirrorIcon: true
                description: root.muted ? qsTr("Unmute") : qsTr("Mute")
                onClicked: root.muteRequested()
            }

            C.Slider {
                id: volume
                objectName: "juxtopposedVolume"
                visible: !root.compactVolume
                implicitWidth: 88 * root.uiScale
                implicitHeight: 28 * root.uiScale
                padding: 0
                from: 0
                to: Math.max(0.01, root.maximumVolume)
                value: root.volume
                stepSize: 0.05
                activeFocusOnTab: true
                Accessible.name: qsTr("Volume")
                opacity: root.muted ? 0.5 : 1
                onMoved: root.volumeRequested(value)
                onPressedChanged: root.requestLockUnlockAutoHide(pressed)
                C.ToolTip.visible: hovered || visualFocus || pressed
                C.ToolTip.text: Math.round(value * 100) + "%"
                C.ToolTip.delay: 500

                handle: Item {
                    implicitWidth: 0
                    implicitHeight: 0
                }

                background: Item {
                    implicitWidth: 88 * root.uiScale
                    implicitHeight: 28 * root.uiScale
                    width: volume.width
                    height: volume.height

                    Image {
                        x: 0
                        y: 0.32 * root.uiScale
                        width: 89 * root.uiScale
                        height: 27.36 * root.uiScale
                        source: root.assetBase + "volume-background.svg"
                        sourceSize: Qt.size(width, height)
                    }

                    Item {
                        x: 1.5 * root.uiScale
                        y: 1.72 * root.uiScale
                        width: Math.max(0, Math.min(1, volume.position)) * 86 * root.uiScale
                        height: 24.56 * root.uiScale
                        clip: true
                        Image {
                            width: 86 * root.uiScale
                            height: 24.56 * root.uiScale
                            source: root.assetBase + "volume-fill.svg"
                            sourceSize: Qt.size(width, height)
                        }
                    }

                    Rectangle {
                        anchors.fill: parent
                        color: "transparent"
                        radius: 4 * root.uiScale
                        border.width: volume.visualFocus ? 2 * root.uiScale : 0
                        border.color: "#D37918"
                    }
                }

                WheelHandler {
                    acceptedDevices: PointerDevice.AllDevices
                    onWheel: (event) => {
                        const delta = event.angleDelta.y || event.angleDelta.x
                        root.volumeRequested(Math.max(volume.from, Math.min(volume.to, volume.value + delta / 120 * 0.05)))
                        event.accepted = true
                    }
                }
            }
        }

        RowLayout {
            spacing: 12 * root.uiScale

            JuxtopposedButton {
                id: audioButton
                objectName: "juxtopposedAudio"
                uiScale: root.uiScale
                source: root.assetBase + "audio.svg"
                description: qsTr("Audio tracks")
                onClicked: root.audioRequested()
            }
            JuxtopposedButton {
                id: subtitlesButton
                objectName: "juxtopposedSubtitles"
                uiScale: root.uiScale
                source: root.assetBase + "subtitles.svg"
                description: qsTr("Subtitle tracks")
                onClicked: root.subtitlesRequested()
            }
            JuxtopposedButton {
                id: settingsButton
                objectName: "juxtopposedSettings"
                uiScale: root.uiScale
                source: root.assetBase + "settings.svg"
                description: qsTr("Playback settings")
                onClicked: root.settingsRequested()
            }
        }

        JuxtopposedButton {
            id: queueButton
            objectName: "juxtopposedQueue"
            uiScale: root.uiScale
            source: root.assetBase + "queue.svg"
            description: qsTr("Play queue")
            fillColor: root.queueVisible ? "#371F06" : "transparent"
            Accessible.checkable: true
            Accessible.checked: root.queueVisible
            onClicked: root.queueRequested()
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
            activeFocusOnTab: true
            Accessible.name: qsTr("Progress")
            onMoved: root.seekRequested(value)
            onPressedChanged: root.requestLockUnlockAutoHide(pressed)
            handle: Item { implicitWidth: 0; implicitHeight: 0 }
            background: JuxtopposedSeekBackground {
                width: parent.width
                height: parent.height
                position: parent.position
                uiScale: root.uiScale
            }
        }
    }
}
