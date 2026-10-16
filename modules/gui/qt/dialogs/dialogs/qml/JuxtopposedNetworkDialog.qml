// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
// Original visual design by Juxtopposed: https://www.youtube.com/@juxtopposed
import QtQuick
import VLC.Style
import QtQuick.Controls.Basic as C
import QtQuick.Layouts

C.Dialog {
    id: root
    property var controller
    property real uiScale: 1
    property url assetBase: "qrc:/juxtopposed/"
    property Item focusReturnItem
    signal fallbackFocusRequested()

    objectName: "juxtopposedNetworkDialog"
    title: qsTr("Open Network Stream")
    modal: true
    focus: true
    anchors.centerIn: parent
    implicitWidth: 900 * uiScale
    implicitHeight: 540 * uiScale
    width: Math.min(implicitWidth, Math.max(0, parent.width - 32 * uiScale))
    height: Math.min(implicitHeight, Math.max(0, parent.height - 32 * uiScale))
    padding: 0
    closePolicy: controller && controller.busy ? C.Popup.NoAutoClose : C.Popup.CloseOnEscape

    function showDialog(returnItem) {
        if (!visible) {
            focusReturnItem = returnItem
            controller.reset()
            open()
        }
        addresses.forceActiveFocus(Qt.PopupFocusReason)
    }
    function submit() {
        if (controller && controller.canSubmit)
            controller.submit()
    }
    function reveal(item) {
        const flick = formScroll.contentItem
        if (!flick || flick.contentY === undefined || !flick.contentItem) return
        const point = item.mapToItem(flick.contentItem, 0, 0)
        if (point.y < flick.contentY) flick.contentY = Math.max(0, point.y)
        else if (point.y + item.height > flick.contentY + flick.height)
            flick.contentY = Math.min(Math.max(0, flick.contentHeight - flick.height),
                                     point.y + item.height - flick.height)
    }
    onClosed: {
        if (focusReturnItem && focusReturnItem.visible && focusReturnItem.enabled)
            focusReturnItem.forceActiveFocus(Qt.PopupFocusReason)
        else
            fallbackFocusRequested()
        focusReturnItem = null
    }
    Connections {
        target: root.controller
        function onFinished(success) {
            if (success) root.accept()
            else closeButton.forceActiveFocus(Qt.OtherFocusReason)
        }
    }
    Shortcut {
        sequence: "Ctrl+Return"
        enabled: root.visible && !!root.controller && root.controller.canSubmit
        onActivated: root.submit()
    }
    Shortcut {
        sequence: StandardKey.Save
        enabled: root.visible && !!root.controller && root.controller.canSubmit
        onActivated: root.submit()
    }
    FontLoader { id: inter; source: root.assetBase + "InterVariable.ttf" }
    background: Rectangle { radius: 12 * root.uiScale; color: JuxtopposedPalette.color("#171717") }
    C.Overlay.modal: Rectangle { color: JuxtopposedPalette.color("#80000000") }

    header: Rectangle {
        implicitHeight: 44 * root.uiScale
        color: JuxtopposedPalette.color("#0B0B0B")
        radius: 12 * root.uiScale
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: parent.height / 2
            color: parent.color
        }
        Text {
            anchors.left: parent.left
            anchors.leftMargin: 18 * root.uiScale
            anchors.right: closeButton.left
            anchors.verticalCenter: parent.verticalCenter
            text: root.title
            color: JuxtopposedPalette.color("#8F8F8F")
            font.family: inter.name
            font.pixelSize: 14 * root.uiScale
            elide: Text.ElideRight
        }
        C.ToolButton {
            id: closeButton
            objectName: "networkClose"
            anchors.right: parent.right
            anchors.rightMargin: 12 * root.uiScale
            anchors.verticalCenter: parent.verticalCenter
            width: 28 * root.uiScale
            height: width
            enabled: !root.controller || !root.controller.busy
            activeFocusOnTab: true
            Accessible.name: qsTr("Close network stream dialog")
            Accessible.description: root.controller ? root.controller.error : ""
            KeyNavigation.tab: addresses
            KeyNavigation.backtab: saveButton.enabled ? saveButton : cancelButton
            onClicked: root.reject()
            background: Rectangle {
                radius: 4 * root.uiScale
                color: closeButton.hovered ? JuxtopposedPalette.color("#303030") : "transparent"
                border.width: closeButton.visualFocus ? root.uiScale : 0
                border.color: JuxtopposedPalette.color("#D37918")
            }
            contentItem: Item {
                Rectangle {
                    anchors.centerIn: parent
                    width: 16 * root.uiScale
                    height: root.uiScale
                    rotation: 45
                    color: JuxtopposedPalette.color("#8F8F8F")
                }
                Rectangle {
                    anchors.centerIn: parent
                    width: 16 * root.uiScale
                    height: root.uiScale
                    rotation: -45
                    color: JuxtopposedPalette.color("#8F8F8F")
                }
            }
        }
    }

    contentItem: C.ScrollView {
        id: formScroll
        Accessible.name: root.title
        clip: true
        contentWidth: availableWidth
        C.ScrollBar.horizontal.policy: C.ScrollBar.AlwaysOff
        ColumnLayout {
            width: formScroll.availableWidth
            spacing: 12 * root.uiScale
            anchors.margins: 0
            Item { Layout.preferredHeight: 12 * root.uiScale }
            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 18 * root.uiScale
                Layout.rightMargin: 18 * root.uiScale
                spacing: 12 * root.uiScale
                Text {
                    text: qsTr("Network URL")
                    font.family: inter.name
                    font.pixelSize: 14 * root.uiScale
                    color: JuxtopposedPalette.color("#BEBEBE")
                }
                Text {
                    Layout.fillWidth: true
                    text: qsTr("You can enter multiple links to load, one per line.")
                    font.family: inter.name
                    font.pixelSize: 13 * root.uiScale
                    color: JuxtopposedPalette.color("#686868")
                    wrapMode: Text.WordWrap
                }
                C.ScrollView {
                    id: urlViewport
                    Layout.fillWidth: true
                    Layout.preferredHeight: 164 * root.uiScale
                    Layout.topMargin: 6 * root.uiScale
                    clip: true
                    C.ScrollBar.horizontal.policy: C.ScrollBar.AlwaysOff
                    background: Rectangle { color: JuxtopposedPalette.color("#090909"); radius: 3 * root.uiScale }
                    C.TextArea {
                        id: addresses
                        objectName: "networkUrls"
                        text: root.controller ? root.controller.urlText : ""
                        enabled: !root.controller || !root.controller.busy
                        placeholderText: qsTr("e.g., https://www.example.com/stream.avi")
                        placeholderTextColor: JuxtopposedPalette.color("#494949")
                        color: JuxtopposedPalette.color("#BEBEBE")
                        selectionColor: JuxtopposedPalette.color("#6B431C")
                        font.family: inter.name
                        font.pixelSize: 13 * root.uiScale
                        padding: 10 * root.uiScale
                        wrapMode: TextEdit.Wrap
                        selectByMouse: true
                        activeFocusOnTab: true
                        Accessible.name: qsTr("Network URLs, one per line")
                        KeyNavigation.priority: KeyNavigation.BeforeItem
                        KeyNavigation.tab: playlistName.enabled ? playlistName : cancelButton
                        KeyNavigation.backtab: closeButton
                        onActiveFocusChanged: if (activeFocus) root.reveal(urlViewport)
                        onTextChanged: {
                            if (root.controller && text !== root.controller.urlText)
                                root.controller.urlText = text
                        }
                        background: Rectangle {
                            color: "transparent"
                            radius: 3 * root.uiScale
                            border.width: addresses.activeFocus ? root.uiScale : 0
                            border.color: JuxtopposedPalette.color("#D37918")
                        }
                    }
                }
                Rectangle { Layout.fillWidth: true; Layout.topMargin: 8 * root.uiScale; height: root.uiScale; color: JuxtopposedPalette.color("#202020") }
                Text {
                    Layout.topMargin: 8 * root.uiScale
                    text: qsTr("Playlist Name") + " <font color='#686868'>" + qsTr("(optional)") + "</font>"
                    textFormat: Text.RichText
                    font.family: inter.name
                    font.pixelSize: 14 * root.uiScale
                    color: JuxtopposedPalette.color("#BEBEBE")
                }
                Text {
                    Layout.fillWidth: true
                    text: root.controller && !root.controller.playlistAvailable
                          ? qsTr("Enable the media library to save named playlists.")
                          : qsTr("The playlist will be saved and displayed with this title.")
                    font.family: inter.name
                    font.pixelSize: 13 * root.uiScale
                    color: JuxtopposedPalette.color("#686868")
                    wrapMode: Text.WordWrap
                }
                C.TextField {
                    id: playlistName
                    objectName: "networkPlaylistName"
                    Layout.fillWidth: true
                    Layout.topMargin: 12 * root.uiScale
                    Layout.preferredHeight: 28 * root.uiScale
                    text: root.controller ? root.controller.playlistName : ""
                    enabled: !!root.controller && root.controller.playlistAvailable && !root.controller.busy
                    placeholderText: qsTr("e.g., Favorite TV Channels")
                    placeholderTextColor: JuxtopposedPalette.color("#494949")
                    color: JuxtopposedPalette.color("#BEBEBE")
                    selectionColor: JuxtopposedPalette.color("#6B431C")
                    font.family: inter.name
                    font.pixelSize: 13 * root.uiScale
                    leftPadding: 10 * root.uiScale
                    rightPadding: 10 * root.uiScale
                    topPadding: 4 * root.uiScale
                    bottomPadding: 4 * root.uiScale
                    activeFocusOnTab: true
                    Accessible.name: qsTr("Playlist name, optional")
                    KeyNavigation.tab: cancelButton
                    KeyNavigation.backtab: addresses
                    onActiveFocusChanged: if (activeFocus) root.reveal(playlistName)
                    onTextEdited: root.controller.playlistName = text
                    onAccepted: root.submit()
                    background: Rectangle {
                        color: JuxtopposedPalette.color("#090909")
                        radius: 3 * root.uiScale
                        border.width: playlistName.activeFocus ? root.uiScale : 0
                        border.color: JuxtopposedPalette.color("#D37918")
                    }
                }
                Text {
                    objectName: "networkError"
                    Layout.fillWidth: true
                    visible: text.length > 0
                    text: root.controller ? root.controller.error : ""
                    font.family: inter.name
                    font.pixelSize: 13 * root.uiScale
                    color: JuxtopposedPalette.color("#D98779")
                    wrapMode: Text.WordWrap
                    Accessible.role: Accessible.StaticText
                    Accessible.name: text
                }
            }
            Item { Layout.preferredHeight: 12 * root.uiScale }
        }
    }

    footer: Item {
        implicitHeight: 64 * root.uiScale
        C.Button {
            id: cancelButton
            objectName: "networkCancel"
            anchors.left: parent.left
            anchors.leftMargin: 18 * root.uiScale
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 18 * root.uiScale
            width: 84 * root.uiScale
            height: 28 * root.uiScale
            text: qsTr("Cancel")
            enabled: !root.controller || !root.controller.busy
            activeFocusOnTab: true
            font.family: inter.name
            font.pixelSize: 13 * root.uiScale
            onClicked: root.reject()
            KeyNavigation.tab: saveButton.enabled ? saveButton : closeButton
            KeyNavigation.backtab: playlistName.enabled ? playlistName : addresses
            contentItem: Text {
                text: parent.text
                font: parent.font
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                color: parent.enabled ? JuxtopposedPalette.color("#BEBEBE") : JuxtopposedPalette.color("#555555")
            }
            background: Rectangle {
                color: parent.hovered ? JuxtopposedPalette.color("#303030") : JuxtopposedPalette.color("#1B1B1B")
                radius: 3 * root.uiScale
                border.width: parent.visualFocus ? root.uiScale : 0
                border.color: JuxtopposedPalette.color("#D37918")
            }
        }
        C.Button {
            id: saveButton
            objectName: "networkSave"
            anchors.right: parent.right
            anchors.rightMargin: 18 * root.uiScale
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 18 * root.uiScale
            width: 84 * root.uiScale
            height: 28 * root.uiScale
            text: root.controller && root.controller.busy ? qsTr("Saving…") : qsTr("Save")
            enabled: !!root.controller && root.controller.canSubmit
            activeFocusOnTab: true
            font.family: inter.name
            font.pixelSize: 13 * root.uiScale
            onClicked: root.submit()
            KeyNavigation.tab: closeButton
            KeyNavigation.backtab: cancelButton
            contentItem: Text {
                text: parent.text
                font: parent.font
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                color: parent.enabled ? JuxtopposedPalette.color("#141414") : JuxtopposedPalette.color("#555555")
            }
            background: Rectangle {
                color: parent.enabled ? (parent.down ? JuxtopposedPalette.color("#B16614") : JuxtopposedPalette.color("#D37918")) : JuxtopposedPalette.color("#1B1B1B")
                radius: 3 * root.uiScale
                border.width: parent.visualFocus ? root.uiScale : 0
                border.color: JuxtopposedPalette.color("#FFFFFF")
            }
        }
    }
}
