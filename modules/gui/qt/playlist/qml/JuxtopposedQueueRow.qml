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

// Presentation only: PlaylistDelegate retains selection, keyboard, drag/drop
// and playback actions. Geometry follows Figma queue row 463:848.
C.Control {
    id: root

    required property string title
    required property string durationText
    property color titleColor: JuxtopposedPalette.color("#969696")
    property color durationColor: JuxtopposedPalette.color("#6B6B6B")
    property real uiScale: 1
    property bool showMove: true
    property int chapterNumber: 0
    property url assetBase: "qrc:/juxtopposed/"
    readonly property bool titleElided: titleText.truncated

    implicitWidth: 360 * uiScale
    implicitHeight: 32 * uiScale
    padding: 0
    leftPadding: chapterNumber > 0 ? 4 * uiScale : 0
    rightPadding: 4 * uiScale
    background: null
    Accessible.ignored: true

    FontLoader {
        id: inter
        source: root.assetBase + "InterVariable.ttf"
    }

    contentItem: RowLayout {
        spacing: (root.chapterNumber > 0 ? 12 : 10) * root.uiScale

        Text {
            objectName: "queueRowNumber"
            visible: root.chapterNumber > 0
            Layout.preferredWidth: 32 * root.uiScale
            text: root.chapterNumber.toString()
            color: JuxtopposedPalette.color("#D37918")
            font.family: inter.name
            font.pixelSize: 14 * root.uiScale
            font.weight: Font.Medium
            Accessible.ignored: true
        }

        Item {
            objectName: "queueRowMoveSlot"
            visible: root.showMove && root.chapterNumber === 0
            Layout.preferredWidth: 32 * root.uiScale
            Layout.preferredHeight: 32 * root.uiScale

            Image {
                objectName: "queueRowMove"
                anchors.centerIn: parent
                source: root.showMove ? root.assetBase + "queue-move.svg" : ""
                // Preserve the original SVG's intrinsic dimensions. The visual
                // transform follows VLC's logical interface scale.
                scale: root.uiScale
                Accessible.ignored: true
            }
        }

        Text {
            id: titleText
            objectName: "queueRowTitle"
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            text: root.title
            textFormat: Text.PlainText
            elide: Text.ElideRight
            color: root.titleColor
            font.family: inter.name
            font.pixelSize: 14 * root.uiScale
            font.weight: Font.Normal
            verticalAlignment: Text.AlignVCenter
            Accessible.ignored: true
        }

        Text {
            objectName: "queueRowDuration"
            Layout.preferredWidth: root.durationText.length <= 5 ? 39 * root.uiScale
                                                                : Math.max(39 * root.uiScale, implicitWidth)
            text: root.durationText
            textFormat: Text.PlainText
            color: root.durationColor
            font.family: inter.name
            font.pixelSize: 14 * root.uiScale
            font.weight: Font.Normal
            horizontalAlignment: Text.AlignRight
            verticalAlignment: Text.AlignVCenter
            Accessible.ignored: true
        }
    }
}
