/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * Visual design by Juxtopposed: https://www.youtube.com/@juxtopposed
 * Code licensed under GPLv2 or later.
 *****************************************************************************/
import QtQuick

Rectangle {
    id: root
    property real position: 0
    property real uiScale: 1
    readonly property real boundedPosition: Math.max(0, Math.min(1, position))
    readonly property real innerWidth: Math.max(0, width - 4 * uiScale)

    implicitHeight: 28 * uiScale
    color: "#090909"
    radius: 6 * uiScale

    Rectangle {
        id: played
        x: 2 * root.uiScale
        y: 2 * root.uiScale
        width: root.innerWidth * root.boundedPosition
        height: Math.max(0, parent.height - 4 * root.uiScale)
        radius: 4 * root.uiScale
        color: "#371F06"
    }

    Rectangle {
        visible: root.boundedPosition > 0
        width: Math.min(8 * root.uiScale, root.innerWidth)
        height: played.height
        x: 2 * root.uiScale + Math.max(0, played.width - width)
        y: played.y
        radius: 4 * root.uiScale
        color: "#D37918"
    }
}
