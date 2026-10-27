// SPDX-License-Identifier: GPL-2.0-or-later
// Original onboarding player geometry/artwork: Juxtopposed, CC BY 4.0.
import QtQuick
import VLC.Player
import VLC.Style
import VLC.Util

Item {
    id: root
    property var appearance: JuxtopposedPalette
    property url assetBase: "qrc:/juxtopposed/"
    readonly property real previewScale: width / 1920
    implicitWidth: 1056
    implicitHeight: width * 9 / 16
    Accessible.role: Accessible.Graphic
    Accessible.name: qsTr("Theme and accent preview")

    Rectangle { anchors.fill: parent; color: root.appearance.color("#050505") }
    Rectangle {
        width: parent.width; height: 40 * root.previewScale
        color: root.appearance.color("#191919")
        Item {
            x: 6 * root.previewScale
            width: 38 * root.previewScale; height: parent.height
            Image {
                anchors.centerIn: parent
                width: 24; height: 24; scale: root.previewScale
                source: root.assetBase + "titlebar-logo.svg"
                Accessible.ignored: true
            }
        }
        Row {
            anchors.right: parent.right; anchors.rightMargin: 6 * root.previewScale
            anchors.verticalCenter: parent.verticalCenter
            Repeater {
                model: ["minimize", "maximize", "fullscreen", "close"]
                delegate: Item {
                    required property string modelData
                    width: 36 * root.previewScale; height: 24 * root.previewScale
                    Image {
                        anchors.centerIn: parent
                        width: 24; height: 24; scale: root.previewScale
                        source: SVGColorImage.colorize(root.assetBase + "titlebar-" + modelData + ".svg")
                            .any({"#8F8F8F": root.appearance.color("#8F8F8F")}).uri()
                        mirror: true
                        Accessible.ignored: true
                    }
                }
            }
        }
    }
    JuxtopposedPlaybackBar {
        objectName: "juxtopposedSetupPreviewControls"
        anchors.bottom: parent.bottom
        width: parent.width; height: implicitHeight
        uiScale: root.previewScale
        appearance: root.appearance
        assetBase: root.assetBase
        previewOnly: true
        elapsed: "00:02:10"; duration: "00:25:40"
        position: 130 / 1540; volume: 0.4; seekable: true
    }
    // This is an example appearance, with no native Player connections. It
    // cannot take focus or turn preview gestures into media/volume actions.
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.AllButtons
        onWheel: event => event.accepted = true
    }
}
