// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
import QtQuick

// Cover only the noninteractive header: editors and close buttons remain
// outside this area. Moving a Popup needs explicit coordinates, not drag.target.
MouseArea {
    id: root
    required property var popup
    property point _pressPoint
    property point _origin
    acceptedButtons: Qt.LeftButton
    preventStealing: true
    cursorShape: Qt.SizeAllCursor
    Accessible.ignored: true

    onPressed: mouse => {
        _pressPoint = mapToItem(popup.parent, mouse.x, mouse.y)
        _origin = Qt.point(popup.x, popup.y)
    }
    onPositionChanged: mouse => {
        if (!pressed) return
        const point = mapToItem(popup.parent, mouse.x, mouse.y)
        popup.moveFromHeader(_origin.x + point.x - _pressPoint.x,
                             _origin.y + point.y - _pressPoint.y)
    }
}
