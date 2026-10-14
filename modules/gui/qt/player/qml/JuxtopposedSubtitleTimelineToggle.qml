// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
// Original visual design: Juxtopposed, Figma 5187:24487 / 536:1336.
import QtQuick
import QtQuick.Controls.Basic as C

C.Switch {
    id: root
    objectName: "juxtopposedSubtitleTimelineToggle"
    property real uiScale: 1
    property url assetBase: "qrc:/juxtopposed/"
    text: qsTr("Show Subtitle Timeline on Seekbar")
    implicitWidth: 360 * uiScale
    implicitHeight: 52 * uiScale
    leftPadding: 4 * uiScale
    rightPadding: 84 * uiScale
    topPadding: 10 * uiScale
    bottomPadding: 10 * uiScale
    activeFocusOnTab: true
    Accessible.name: text
    font.family: inter.name
    font.pixelSize: 14 * uiScale
    FontLoader { id: inter; source: root.assetBase + "InterVariable.ttf" }
    contentItem: C.Label {
        text: root.text
        textFormat: Text.PlainText
        color: "#6B6B6B"
        font: root.font
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    indicator: Rectangle {
        objectName: "subtitleTimelineToggleIndicator"
        x: root.width - width
        y: (root.height - height) / 2
        width: 68 * root.uiScale
        height: 32 * root.uiScale
        color: "#090909"
        radius: 6 * root.uiScale
        border.width: root.visualFocus ? root.uiScale : 0
        border.color: "#D37918"
        Rectangle {
            x: (root.checked ? 3 : 32) * root.uiScale
            y: 3 * root.uiScale
            width: 33 * root.uiScale
            height: 26 * root.uiScale
            radius: 3 * root.uiScale
            color: root.checked ? "#371F06" : "#191919"
        }
        Rectangle {
            objectName: "subtitleTimelineToggleKnob"
            x: (root.checked ? 39 : 3) * root.uiScale
            y: 3 * root.uiScale
            width: 26 * root.uiScale
            height: width
            radius: 3 * root.uiScale
            color: root.checked ? "#AB6314" : "#404040"
        }
        C.Label {
            x: (root.checked ? 10 : 36) * root.uiScale
            y: 9 * root.uiScale
            text: root.checked ? qsTr("ON") : qsTr("OFF")
            font.family: inter.name
            font.pixelSize: 12 * root.uiScale
            color: root.checked ? "#D37918" : "#6B6B6B"
            Accessible.ignored: true
        }
    }
}
