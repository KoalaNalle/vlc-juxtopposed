/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * Visual design by Juxtopposed: https://www.youtube.com/@juxtopposed
 * Code licensed under GPLv2 or later; artwork under CC BY 4.0.
 *****************************************************************************/
import QtQuick
import QtQuick.Controls as C

C.ToolButton {
    id: root

    required property url source
    required property string description
    property real uiScale: 1
    property bool mirrorIcon: false
    property color fillColor: "transparent"

    implicitWidth: 30 * uiScale
    implicitHeight: 32 * uiScale
    padding: 0
    activeFocusOnTab: true

    Accessible.name: description
    C.ToolTip.visible: hovered || visualFocus
    C.ToolTip.text: description
    C.ToolTip.delay: 500

    background: Rectangle {
        radius: 4 * root.uiScale
        color: root.down ? "#404040" : root.hovered ? "#303030" : root.fillColor
        border.width: root.visualFocus ? 2 * root.uiScale : 0
        border.color: "#D37918"
    }

    contentItem: Item {
        Image {
            anchors.centerIn: parent
            source: root.source
            sourceSize: Qt.size(24 * root.uiScale, 24 * root.uiScale)
            width: 24 * root.uiScale
            height: 24 * root.uiScale
            fillMode: Image.PreserveAspectFit
            mirror: root.mirrorIcon
            opacity: root.enabled ? 1 : 0.35
            Accessible.ignored: true
        }
    }
}
