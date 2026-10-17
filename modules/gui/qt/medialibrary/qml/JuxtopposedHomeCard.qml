/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * Visual design by Juxtopposed: https://www.youtube.com/@juxtopposed
 * Code licensed under GPLv2 or later.
 *****************************************************************************/
import QtQuick
import VLC.Style
import QtQuick.Controls.Basic as C
import VLC.Widgets as Widgets

C.AbstractButton {
    id: root
    property real uiScale: 1
    property string fontFamily: "Inter"
    property string iconFontFamily: ""
    property string fallbackIcon: ""
    property url fallbackArtwork: ""
    property string kind: "video"
    property var entry: ({})
    property bool gridMode: true
    property color surfaceColor: JuxtopposedPalette.color("#0F0F0F")
    property color textColor: JuxtopposedPalette.color("#969696")
    property color accentColor: JuxtopposedPalette.color("#D37918")
    readonly property bool video: kind === "video"
    readonly property bool audio: kind === "audio"
    readonly property string title: video ? (entry?.fileName || entry?.title || qsTr("Unknown title"))
                                          : (entry?.title || entry?.name || qsTr("Unknown title"))
    readonly property string artist: audio ? (entry?.main_artist || "") : ""
    readonly property string subtitle: artist || (entry?.available === false ? qsTr("Unavailable") : "")
    readonly property real coverWidth: (gridMode ? (video ? 320 : 200) : 32) * uiScale
    readonly property real coverHeight: (gridMode ? (video ? 180 : 200) : 32) * uiScale
    readonly property url artwork: entry?.small_cover || entry?.thumbnail || entry?.cover || entry?.artwork || ""
    readonly property real progress: Math.max(0, Math.min(1, entry?.progress || 0))
    implicitWidth: gridMode ? coverWidth : 400 * uiScale
    implicitHeight: gridMode ? coverHeight + 18 * uiScale + 22 * uiScale + (subtitle ? 26 * uiScale : 0) : 44 * uiScale
    padding: 0
    focusPolicy: Qt.StrongFocus
    enabled: entry?.available !== false
    opacity: enabled ? 1 : 0.5
    Keys.onReturnPressed: clicked()
    Keys.onEnterPressed: clicked()
    Accessible.name: artist ? title + " \u2014 " + artist : title
    Accessible.description: !enabled ? qsTr("This item is unavailable") : video && progress > 0 ? qsTr("Continue watching") : ""
    C.ToolTip.visible: hovered && label.truncated
    C.ToolTip.text: Accessible.name
    C.ToolTip.delay: 600

    background: Rectangle {
        color: root.down ? JuxtopposedPalette.color("#303030") : root.hovered ? JuxtopposedPalette.color("#1E1E1E") : "transparent"
        radius: 8 * root.uiScale
        border.width: root.visualFocus ? 2 * root.uiScale : 0
        border.color: root.accentColor
    }

    contentItem: Item {
        Rectangle {
            id: cover
            x: 0
            y: root.gridMode ? 0 : (parent.height - height) / 2
            width: Math.min(root.coverWidth, parent.width)
            height: root.gridMode ? width * (root.video ? 9 / 16 : 1) : root.coverHeight
            radius: (root.gridMode ? 10 : 2) * root.uiScale
            color: root.video || root.audio ? JuxtopposedPalette.color("#000000") : JuxtopposedPalette.color("#1A1A1A")
            clip: true
            Widgets.ImageExt {
                id: mediaArt
                anchors.fill: parent
                source: root.artwork
                radius: cover.radius
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                visible: status === Image.Ready
            }
            C.Label {
                anchors.centerIn: parent
                visible: mediaArt.status !== Image.Ready && fallbackArt.status !== Image.Ready
                text: root.fallbackIcon
                color: root.textColor
                font.family: root.iconFontFamily
                font.pixelSize: (root.gridMode ? 44 : 26) * root.uiScale
                Accessible.ignored: true
            }
            Image {
                id: fallbackArt
                anchors.centerIn: parent
                width: Math.min(parent.width, (root.gridMode ? 44 : 26) * root.uiScale)
                height: width
                source: root.fallbackArtwork
                visible: mediaArt.status !== Image.Ready && status === Image.Ready
                fillMode: Image.PreserveAspectFit
            }
            Rectangle {
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                width: parent.width * root.progress
                height: (root.gridMode ? 4 : 3) * root.uiScale
                visible: root.video && root.progress > 0
                color: root.accentColor
            }

        }
        C.Label {
            id: label
            anchors.left: root.gridMode ? parent.left : cover.right
            anchors.leftMargin: root.gridMode ? 0 : 12 * root.uiScale
            anchors.right: parent.right
            y: root.gridMode ? cover.y + cover.height + 18 * root.uiScale : (parent.height - height) / 2
            text: !root.gridMode && root.subtitle ? root.title + " - " + root.subtitle : root.title
            textFormat: Text.PlainText
            font.family: root.fontFamily
            font.pixelSize: 16 * root.uiScale
            color: root.textColor
            elide: Text.ElideRight
            Accessible.ignored: true
        }
        C.Label {
            anchors.top: label.bottom
            anchors.topMargin: 6 * root.uiScale
            anchors.left: parent.left
            anchors.right: parent.right
            visible: root.gridMode && root.subtitle.length > 0
            text: root.subtitle
            textFormat: Text.PlainText
            font.family: root.fontFamily
            font.pixelSize: 16 * root.uiScale
            color: JuxtopposedPalette.color("#626262")
            elide: Text.ElideRight
            Accessible.ignored: true
        }
    }
}
