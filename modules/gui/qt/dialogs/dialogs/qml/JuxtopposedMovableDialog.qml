// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
import QtQuick
import QtQuick.Controls.Basic as C

// Keep an item popup in its owning window. Coordinates remain bound so a
// window resize or scale change cannot strand its header outside the viewport.
C.Dialog {
    id: root
    property real _requestedX: NaN
    property real _requestedY: NaN

    function bounded(value, extent, size) {
        return Math.max(0, Math.min(value, Math.max(0, extent - size)))
    }
    function moveFromHeader(newX, newY) {
        if (!parent) return
        _requestedX = bounded(newX, parent.width, width)
        _requestedY = bounded(newY, parent.height, height)
    }

    x: parent ? bounded(isFinite(_requestedX) ? _requestedX : (parent.width - width) / 2, parent.width, width) : 0
    y: parent ? bounded(isFinite(_requestedY) ? _requestedY : (parent.height - height) / 2, parent.height, height) : 0

    Connections {
        target: root
        function onAboutToShow() {
            root._requestedX = NaN
            root._requestedY = NaN
        }
    }
}
