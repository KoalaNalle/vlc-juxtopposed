/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * Visual design by Juxtopposed: https://www.youtube.com/@juxtopposed
 * Code licensed under GPLv2 or later; artwork under CC BY 4.0.
 *****************************************************************************/
import QtQuick
import VLC.Style
import QtQuick.Controls.Basic as C
import QtQuick.Layouts

C.Pane {
    id: root

    property real uiScale: 1
    property url assetBase: "qrc:/juxtopposed/"
    property string iconFontFamily: ""
    property var routes: []
    property string activeRoute: ""
    property bool expanded: false
    property bool libraryBusy: false
    property bool libraryAvailable: true
    property string preferencesIcon: ""
    property color surfaceColor: JuxtopposedPalette.color("#1B1B1B")
    property color textColor: JuxtopposedPalette.color("#969696")
    property color accentColor: JuxtopposedPalette.color("#D37918")

    signal routeRequested(var route, int reason)
    signal preferencesRequested()
    signal contentFocusRequested(int reason)

    implicitWidth: (expanded ? 204 : 46) * uiScale
    padding: (expanded ? 5 : 0) * uiScale
    Accessible.name: qsTr("Library navigation")
    Accessible.description: libraryAvailable ? "" : qsTr("The media library is unavailable. You can still open files or use Network.")

    Keys.priority: Keys.AfterItem
    Keys.onUpPressed: moveFocus(-1)
    Keys.onDownPressed: moveFocus(1)
    Keys.onRightPressed: contentFocusRequested(Qt.TabFocusReason)

    function moveFocus(direction) {
        const buttons = []
        for (let i = 0; i < routeRepeater.count; ++i) {
            const button = routeRepeater.itemAt(i)
            if (button.enabled && button.visible)
                buttons.push(button)
        }
        buttons.push(preferencesButton)
        if (!buttons.length)
            return
        const current = buttons.findIndex(button => button.activeFocus)
        const index = (current + direction + buttons.length) % buttons.length
        buttons[index].forceActiveFocus(direction > 0 ? Qt.TabFocusReason : Qt.BacktabFocusReason)
    }

    FontLoader {
        id: inter
        source: root.assetBase + "InterVariable.ttf"
    }

    background: Rectangle {
        color: root.surfaceColor
        radius: root.expanded ? 5 * root.uiScale : 0
    }

    contentItem: ColumnLayout {
        spacing: 0

        Repeater {
            id: routeRepeater
            model: root.routes

            delegate: C.ToolButton {
                id: routeButton
                required property var modelData
                required property int index

                objectName: "juxtopposedNavigation" + modelData.key
                Layout.fillWidth: true
                Layout.preferredHeight: 46 * root.uiScale
                padding: 0
                enabled: modelData.enabled !== false
                checked: modelData.key === root.activeRoute
                focusPolicy: Qt.StrongFocus
                Accessible.name: modelData.text
                Accessible.description: checked ? qsTr("Current section") : ""
                C.ToolTip.visible: !root.expanded && (hovered || visualFocus)
                C.ToolTip.text: modelData.text
                C.ToolTip.delay: 500

                onClicked: root.routeRequested(modelData.uri, focusReason)

                background: Rectangle {
                    color: routeButton.down ? JuxtopposedPalette.color("#404040") : routeButton.hovered ? JuxtopposedPalette.color("#303030") : "transparent"
                    radius: 4 * root.uiScale
                    border.width: routeButton.visualFocus ? 2 * root.uiScale : 0
                    border.color: root.accentColor
                }

                contentItem: RowLayout {
                    spacing: 12 * root.uiScale
                    opacity: routeButton.enabled ? 1 : 0.35

                    C.Label {
                        Layout.preferredWidth: (root.expanded ? 41 : 46) * root.uiScale
                        Layout.fillHeight: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        text: routeButton.modelData.icon
                        font.family: root.iconFontFamily
                        font.pixelSize: 24 * root.uiScale
                        color: routeButton.checked ? root.accentColor : root.textColor
                        Accessible.ignored: true
                    }

                    C.Label {
                        Layout.fillWidth: true
                        visible: root.expanded
                        text: routeButton.modelData.text
                        elide: Text.ElideRight
                        font.family: inter.name
                        font.pixelSize: 14 * root.uiScale
                        color: routeButton.checked ? root.accentColor : root.textColor
                        Accessible.ignored: true
                    }
                }
            }
        }

        Item { Layout.fillHeight: true }

        C.Label {
            objectName: "juxtopposedLibraryUnavailable"
            Layout.fillWidth: true
            visible: root.expanded && !root.libraryAvailable
            text: qsTr("Media library unavailable")
            wrapMode: Text.WordWrap
            font.family: inter.name
            font.pixelSize: 12 * root.uiScale
            color: root.textColor
            horizontalAlignment: Text.AlignHCenter
            padding: 8 * root.uiScale
        }

        C.BusyIndicator {
            objectName: "juxtopposedLibraryScan"
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 28 * root.uiScale
            Layout.preferredHeight: 28 * root.uiScale
            visible: root.libraryBusy
            running: visible
            Accessible.name: qsTr("Scanning media library")
        }

        C.ToolButton {
            id: preferencesButton
            objectName: "juxtopposedLibraryPreferences"
            Layout.fillWidth: true
            Layout.preferredHeight: 46 * root.uiScale
            padding: 0
            focusPolicy: Qt.StrongFocus
            Accessible.name: qsTr("Preferences")
            C.ToolTip.visible: !root.expanded && (hovered || visualFocus)
            C.ToolTip.text: qsTr("Preferences")
            C.ToolTip.delay: 500
            onClicked: root.preferencesRequested()

            background: Rectangle {
                color: preferencesButton.down ? JuxtopposedPalette.color("#404040") : preferencesButton.hovered ? JuxtopposedPalette.color("#303030") : "transparent"
                radius: 4 * root.uiScale
                border.width: preferencesButton.visualFocus ? 2 * root.uiScale : 0
                border.color: root.accentColor
            }
            contentItem: RowLayout {
                spacing: 12 * root.uiScale
                C.Label {
                    Layout.preferredWidth: (root.expanded ? 41 : 46) * root.uiScale
                    Layout.fillHeight: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    text: root.preferencesIcon
                    font.family: root.iconFontFamily
                    font.pixelSize: 24 * root.uiScale
                    color: root.textColor
                    Accessible.ignored: true
                }
                C.Label {
                    Layout.fillWidth: true
                    visible: root.expanded
                    text: qsTr("Preferences")
                    font.family: inter.name
                    font.pixelSize: 14 * root.uiScale
                    color: root.textColor
                    elide: Text.ElideRight
                    Accessible.ignored: true
                }
            }
        }
    }
}
