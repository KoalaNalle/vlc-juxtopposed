/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * Visual design by Juxtopposed: https://www.youtube.com/@juxtopposed
 * Code licensed under GPLv2 or later.
 *****************************************************************************/
import QtQuick
import VLC.Style

Rectangle {
    id: root
    property real position: 0
    property real uiScale: 1
    property var appearance: JuxtopposedPalette
    property var waveformPeaks: []
    readonly property bool hasWaveform: waveformPeaks.length > 0
    readonly property real maximumPeak: hasWaveform ? Math.max(0.001, ...waveformPeaks) : 1
    readonly property real boundedPosition: Math.max(0, Math.min(1, position))
    readonly property real innerWidth: Math.max(0, width - 4 * uiScale)

    implicitHeight: 28 * uiScale
    color: root.appearance.color("#090909")
    radius: 6 * uiScale

    Rectangle {
        id: played
        visible: !root.hasWaveform
        x: 2 * root.uiScale
        y: 2 * root.uiScale
        width: root.innerWidth * root.boundedPosition
        height: Math.max(0, parent.height - 4 * root.uiScale)
        radius: 4 * root.uiScale
        color: root.appearance.color("#371F06")
    }

    Repeater {
        model: root.waveformPeaks
        delegate: Rectangle {
            objectName: "juxtopposedWaveformBar" + index
            required property int index
            required property var modelData
            readonly property real pitch: root.innerWidth / root.waveformPeaks.length
            width: Math.min(2 * root.uiScale, pitch * 0.65)
            x: 2 * root.uiScale + (index + 0.5) * pitch - width / 2
            anchors.verticalCenter: parent.verticalCenter
            height: Math.max(root.uiScale, Math.min(1, Math.max(0, modelData) / root.maximumPeak)
                             * Math.max(0, root.height - 8 * root.uiScale))
            radius: width / 2
            color: (index + 0.5) / root.waveformPeaks.length <= root.boundedPosition
                   ? root.appearance.color("#D37918") : root.appearance.color("#4F4F4F")
        }
    }

    Rectangle {
        visible: root.boundedPosition > 0
        width: Math.min(8 * root.uiScale, root.innerWidth)
        height: played.height
        x: 2 * root.uiScale + Math.max(0, played.width - width)
        y: played.y
        radius: 4 * root.uiScale
        color: root.appearance.color("#D37918")
    }
}
