// SPDX-License-Identifier: GPL-2.0-or-later
// Original Juxtopposed volume geometry/assets shared by default and edited bars.
import QtQuick
import QtQuick.Controls.Basic as C
import QtQuick.Layouts
import VLC.Style

RowLayout {
    id: root
    property real uiScale: 1
    property var appearance: JuxtopposedPalette
    property url assetBase: "qrc:/juxtopposed/"
    property bool previewOnly: false
    property bool compactVolume: false
    property real maximumVolume: 1.25
    property real volumeStep: 0.05
    property real volume: 0
    property bool muted: false
    readonly property Item muteButton: muteButton
    readonly property Item slider: volume
    readonly property var keyboardControls: [muteButton, volume]
    signal volumeRequested(real value)
    signal muteRequested()
    signal requestLockUnlockAutoHide(bool lock)
            spacing: 8 * root.uiScale

            JuxtopposedButton {
                id: muteButton
                appearance: root.appearance
                activeFocusOnTab: !root.previewOnly
                Accessible.ignored: root.previewOnly
                objectName: "juxtopposedMute"
                uiScale: root.uiScale
                implicitWidth: 24 * uiScale
                source: root.assetBase + (root.muted ? "volume-muted.svg" : root.appearance.icon("volume.svg"))
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
                stepSize: root.volumeStep
                activeFocusOnTab: !root.previewOnly
                Accessible.ignored: root.previewOnly
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
                        objectName: "juxtopposedVolumeOriginalBackground"
                        source: root.assetBase + root.appearance.icon("volume-background.svg")
                        sourceSize: Qt.size(width, height)
                    }

                    Item {
                        objectName: "juxtopposedVolumeFillClip"
                        x: 1.5 * root.uiScale
                        y: 1.72 * root.uiScale
                        width: Math.max(0, Math.min(1, volume.position)) * 86 * root.uiScale
                        height: 24.56 * root.uiScale
                        clip: true
                        Image {
                            width: 86 * root.uiScale
                            height: 24.56 * root.uiScale
                            objectName: "juxtopposedVolumeOriginalFill"
                            source: root.assetBase + root.appearance.icon("volume-fill.svg")
                            sourceSize: Qt.size(width, height)
                        }
                    }

                    Rectangle {
                        anchors.fill: parent
                        color: "transparent"
                        radius: 4 * root.uiScale
                        border.width: volume.visualFocus ? 2 * root.uiScale : 0
                        border.color: root.appearance.color("#D37918")
                    }
                }

                WheelHandler {
                    acceptedDevices: PointerDevice.AllDevices
                    onWheel: (event) => {
                        const delta = event.angleDelta.y || event.angleDelta.x
                        root.volumeRequested(Math.max(volume.from, Math.min(volume.to, volume.value + delta / 120 * root.volumeStep)))
                        event.accepted = true
                    }
                }
            }
}
