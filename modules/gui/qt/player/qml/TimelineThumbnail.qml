// SPDX-License-Identifier: GPL-2.0-or-later
// Original visual design: Juxtopposed, CC BY 4.0. Figma 2151:9673.
import QtQuick
import QtQuick.Templates as T
import VLC.Widgets as Widgets
import VLC.Style

T.Popup {
    id: root
    objectName: "timelineThumbnail"
    property url imageSource
    property int seconds: 0
    property real uiScale: VLCStyle.scale
    property real anchorX: 0
    property real radius: 14 * uiScale
    property var appearance: JuxtopposedPalette
    implicitWidth: 272 * uiScale
    implicitHeight: 189 * uiScale
    padding: 10 * uiScale
    margins: 0
    x: anchorX - width / 2
    y: -height - 8 * uiScale
    focus: false
    // A picture card follows the pointer and clamps at the window edge.
    // ToolTip can instead flip it across the entire parent timeline on opening.
    // Pointer eligibility owns visibility; no tooltip delay/timeout is needed.
    closePolicy: T.Popup.CloseOnEscape | T.Popup.CloseOnPressOutsideParent | T.Popup.CloseOnReleaseOutsideParent
    // A local preview is an inline popup. Closing/reopening a separate tooltip
    // window can clear the parent's hover while the pointer is still moving.
    Component.onCompleted: if (root.popupType !== undefined) root.popupType = 0 // Popup.Item
    font.family: inter.name
    font.pixelSize: 15 * uiScale
    property string text: [Math.floor(seconds / 3600), Math.floor(seconds / 60) % 60, seconds % 60]
        .map(value => value < 10 ? "0" + value : String(value)).join(":")
    FontLoader { id: inter; source: "qrc:/juxtopposed/InterVariable.ttf" }
    background: Rectangle {
        readonly property color fill: root.appearance.color("#191919")
        color: Qt.rgba(fill.r, fill.g, fill.b, .95)
        radius: root.radius
    }
    contentItem: Column {
        id: content
        spacing: 10 * root.uiScale
        Widgets.ImageExt {
            objectName: "timelineThumbnailImage"
            width: 252 * root.uiScale
            height: 142 * root.uiScale
            radius: 4 * root.uiScale
            source: root.visible ? root.imageSource : ""
            asynchronous: false
            cache: false
            fillMode: Image.PreserveAspectCrop
            Accessible.ignored: true
        }
        Text {
            objectName: "timelineThumbnailTime"
            width: 252 * root.uiScale
            height: 17 * root.uiScale
            text: root.text
            Accessible.name: qsTr("Video preview at %1").arg(root.text)
            textFormat: Text.PlainText
            color: root.appearance.color("#8F8F8F")
            font: root.font
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }
}
