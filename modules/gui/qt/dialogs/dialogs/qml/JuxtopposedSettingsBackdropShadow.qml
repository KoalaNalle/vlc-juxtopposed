// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
import QtQuick
import VLC.Widgets as Widgets

// Separate from capture so native video can retain this exact shadow while
// the unavailable Qt scene capture is disabled. Load only on an RHI renderer.
Item {
    id: root
    objectName: "globalSettingsBackdropShadowLayer"
    property real uiScale: 1

    Widgets.RoundedRectangleShadow {
        objectName: "globalSettingsBackdropShadow"
        // CSS shadow 0px 8px 15px 0px rgba(0,0,0,.46): its Gaussian
        // standard deviation is half the CSS shadow blur-radius.
        blurRadius: 7.5 * root.uiScale
        xOffset: 0
        yOffset: 8 * root.uiScale
        radius: 14 * root.uiScale
        color: Qt.rgba(0, 0, 0, .46)
        compensationFactor: 3
    }
}
