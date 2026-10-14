// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
// Original visual design by Juxtopposed: https://www.youtube.com/@juxtopposed
import QtQuick
import QtQuick.Controls.Basic as C
import QtQuick.Layouts

C.Dialog {
    id: root
    objectName: "juxtopposedNewPlaylistDialog"
    property var controller
    property real uiScale: 1
    property url assetBase: "qrc:/juxtopposed/"
    property Item focusReturnItem
    readonly property bool editable: !!controller && !controller.busy && !controller.partialSave
    signal filesRequested()
    signal thumbnailRequested()
    signal playlistRequested(var playlist)
    signal fallbackFocusRequested()

    title: qsTr("New Playlist")
    modal: true
    focus: true
    anchors.centerIn: parent
    implicitWidth: 713 * uiScale
    implicitHeight: 589 * uiScale
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
        playlistName.forceActiveFocus(Qt.PopupFocusReason)
    }
    function save() {
        if (controller && controller.canSave) controller.save()
    }
    function appendDroppedFiles(urls) {
        return root.editable && controller.appendFiles(urls)
    }
    function firstFileAction(index) {
        const row = selectedFiles.itemAtIndex(index)
        return row ? (row.upButton.enabled ? row.upButton : row.downButton.enabled ? row.downButton : row.removeButton) : filesButton
    }
    function focusFile(index, reason, lastAction) {
        selectedFiles.positionViewAtIndex(index, ListView.Contain)
        selectedFiles.forceLayout()
        const row = selectedFiles.itemAtIndex(index)
        if (row) (lastAction ? row.removeButton : firstFileAction(index)).forceActiveFocus(reason)
        else filesButton.forceActiveFocus(reason)
    }
    function reveal(item) {
        const flick = formScroll.contentItem
        if (!flick || flick.contentY === undefined || !flick.contentItem) return
        const point = item.mapToItem(flick.contentItem, 0, 0)
        if (point.y < flick.contentY) flick.contentY = Math.max(0, point.y)
        else if (point.y + item.height > flick.contentY + flick.height)
            flick.contentY = Math.min(Math.max(0, flick.contentHeight - flick.height), point.y + item.height - flick.height)
    }
    onClosed: {
        if (focusReturnItem && focusReturnItem.visible && focusReturnItem.enabled)
            focusReturnItem.forceActiveFocus(Qt.PopupFocusReason)
        else fallbackFocusRequested()
        focusReturnItem = null
    }
    Connections {
        target: root.controller
        function onFinished(success) {
            if (success) {
                root.playlistRequested(root.controller.savedPlaylist)
                root.accept()
            } else closeButton.forceActiveFocus(Qt.OtherFocusReason)
        }
    }
    Shortcut { sequence: "Ctrl+Return"; enabled: root.visible && !!root.controller && root.controller.canSave; onActivated: root.save() }
    Shortcut { sequence: StandardKey.Save; enabled: root.visible && !!root.controller && root.controller.canSave; onActivated: root.save() }
    FontLoader { id: inter; source: root.assetBase + "InterVariable.ttf" }
    background: Rectangle { radius: 14 * root.uiScale; color: Qt.rgba(25 / 255, 25 / 255, 25 / 255, .95) }
    C.Overlay.modal: Rectangle { color: "#80000000" }

    component FormLabel: Text {
        color: "#C6C6C6"
        font.family: inter.name
        font.pixelSize: 14 * root.uiScale
        textFormat: Text.PlainText
        wrapMode: Text.WordWrap
    }
    component FormButton: C.Button {
        id: button
        property bool primary: false
        opacity: button.primary && !button.enabled ? .5 : 1
        implicitWidth: 100 * root.uiScale
        implicitHeight: 32 * root.uiScale
        activeFocusOnTab: true
        font.family: inter.name
        font.pixelSize: 14 * root.uiScale
        font.weight: Font.DemiBold
        Keys.onReturnPressed: clicked()
        Keys.onEnterPressed: clicked()
        contentItem: Text {
            text: button.text
            font: button.font
            textFormat: Text.PlainText
            color: button.primary ? "#D37918" : "#8F8F8F"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        background: Rectangle {
            radius: 6 * root.uiScale
            color: button.primary ? (button.down ? "#4D2C09" : "#371F06")
                : (button.hovered ? "#303030" : "#1B1B1B")
            border.width: button.visualFocus ? root.uiScale : 0
            border.color: "#D37918"
        }
    }

    header: Rectangle {
        implicitHeight: 50 * root.uiScale
        color: "#090909"
        radius: 14 * root.uiScale
        Rectangle { anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; height: parent.height / 2; color: parent.color }
        FormLabel { anchors.left: parent.left; anchors.leftMargin: 19 * root.uiScale; anchors.verticalCenter: parent.verticalCenter; text: root.title; color: "#8F8F8F" }
        C.ToolButton {
            id: closeButton
            objectName: "newPlaylistClose"
            anchors.right: parent.right
            anchors.rightMargin: 9 * root.uiScale
            anchors.verticalCenter: parent.verticalCenter
            width: 32 * root.uiScale; height: width
            enabled: !root.controller || !root.controller.busy
            activeFocusOnTab: true
            Accessible.name: qsTr("Close new playlist dialog")
            Accessible.description: root.controller ? root.controller.error : ""
            onClicked: root.reject()
            KeyNavigation.tab: playlistName.enabled ? playlistName : cancelButton
            KeyNavigation.backtab: saveButton.enabled ? saveButton : cancelButton
            background: Rectangle { radius: 4 * root.uiScale; color: closeButton.hovered ? "#303030" : "transparent"; border.width: closeButton.visualFocus ? root.uiScale : 0; border.color: "#D37918" }
            contentItem: Item {
                Image { objectName: "newPlaylistOriginalClose"; anchors.centerIn: parent; width: 24; height: 24; scale: root.uiScale; rotation: 180; transform: Scale { origin.x: 12; origin.y: 12; yScale: -1 } source: root.assetBase + "new-playlist-close.svg"; Accessible.ignored: true }
            }
        }
    }

    contentItem: C.ScrollView {
        id: formScroll
        clip: true
        contentWidth: availableWidth
        C.ScrollBar.horizontal.policy: C.ScrollBar.AlwaysOff
        ColumnLayout {
            width: formScroll.availableWidth
            spacing: 0
            Item { Layout.preferredHeight: 22 * root.uiScale }
            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 20 * root.uiScale; Layout.rightMargin: 20 * root.uiScale
                spacing: 20 * root.uiScale
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    spacing: 0
                    FormLabel { objectName: "newPlaylistDetailsTitle"; text: qsTr("Playlist Details"); font.pixelSize: 16 * root.uiScale; lineHeightMode: Text.FixedHeight; lineHeight: 28 * root.uiScale }
                    FormLabel { objectName: "newPlaylistDetailsDescription"; Layout.fillWidth: true; Layout.topMargin: 5 * root.uiScale; text: qsTr("The playlist will be saved and displayed with this title and thumbnail."); color: "#6B6B6B"; font.pixelSize: 14 * root.uiScale; lineHeightMode: Text.FixedHeight; lineHeight: 24.5 * root.uiScale }
                    C.TextField {
                        id: playlistName
                        objectName: "newPlaylistName"
                        Layout.fillWidth: true
                        Layout.topMargin: 20 * root.uiScale
                        Layout.preferredHeight: 41 * root.uiScale
                        text: root.controller ? root.controller.name : ""
                        enabled: root.editable
                        placeholderText: qsTr("e.g., Favorite Videos")
                        placeholderTextColor: "#404040"
                        color: "#C6C6C6"
                        selectionColor: "#6B431C"
                        font.family: inter.name; font.pixelSize: 14 * root.uiScale
                        leftPadding: 12 * root.uiScale; rightPadding: 18 * root.uiScale
                        activeFocusOnTab: true
                        selectByMouse: true
                        Accessible.name: qsTr("Playlist name")
                        KeyNavigation.priority: KeyNavigation.BeforeItem
                        KeyNavigation.tab: thumbnailButton
                        KeyNavigation.backtab: closeButton
                        onTextEdited: root.controller.name = text
                        onAccepted: root.save()
                        onActiveFocusChanged: if (activeFocus) root.reveal(playlistName)
                        background: Rectangle { color: "#090909"; radius: 6 * root.uiScale; border.width: 2 * root.uiScale; border.color: playlistName.activeFocus ? "#D37918" : "#191919" }
                    }
                }
                C.Button {
                    id: thumbnailButton
                    objectName: "newPlaylistThumbnail"
                    Layout.preferredWidth: 112 * root.uiScale; Layout.preferredHeight: 112 * root.uiScale; Layout.alignment: Qt.AlignBottom
                    padding: 0
                    enabled: root.editable
                    activeFocusOnTab: true
                    Accessible.name: qsTr("Choose playlist thumbnail, optional")
                    C.ToolTip.visible: hovered || visualFocus
                    C.ToolTip.text: qsTr("Choose thumbnail")
                    KeyNavigation.tab: removeThumbnail.visible ? removeThumbnail : selectedFiles.count > 0 ? root.firstFileAction(0) : emptyFilesButton
                    Keys.onTabPressed: event => {
                        if (!removeThumbnail.visible && selectedFiles.count > 0) { root.focusFile(0, Qt.TabFocusReason, false); event.accepted = true }
                        else event.accepted = false
                    }
                    KeyNavigation.backtab: playlistName
                    onClicked: root.thumbnailRequested()
                    onActiveFocusChanged: if (activeFocus) root.reveal(thumbnailButton)
                    background: Rectangle { color: "#090909"; radius: 6 * root.uiScale; border.width: 2 * root.uiScale; border.color: thumbnailButton.visualFocus ? "#D37918" : "#191919" }
                    contentItem: Item {
                        Image { anchors.fill: parent; anchors.margins: root.uiScale; source: root.controller ? root.controller.thumbnail : ""; fillMode: Image.PreserveAspectCrop; asynchronous: true; visible: source.toString().length > 0 }
                        Rectangle { anchors.centerIn: parent; width: 30 * root.uiScale; height: 30 * root.uiScale; radius: 5 * root.uiScale; color: "#161616"; visible: !root.controller || !root.controller.thumbnail.toString().length; Image { objectName: "newPlaylistOriginalThumbnailAdd"; anchors.centerIn: parent; width: 24; height: 24; scale: root.uiScale; source: root.assetBase + "new-playlist-add.svg"; Accessible.ignored: true } }
                    }
                }
            }
            FormButton {
                id: removeThumbnail
                objectName: "newPlaylistClearThumbnail"
                Layout.alignment: Qt.AlignRight
                Layout.rightMargin: 20 * root.uiScale
                Layout.topMargin: 6 * root.uiScale
                visible: !!root.controller && root.controller.thumbnail.toString().length > 0
                enabled: root.editable
                implicitWidth: 116 * root.uiScale
                text: qsTr("Remove image")
                KeyNavigation.tab: selectedFiles.count > 0 ? root.firstFileAction(0) : emptyFilesButton
                Keys.onTabPressed: event => {
                    if (selectedFiles.count > 0) { root.focusFile(0, Qt.TabFocusReason, false); event.accepted = true }
                    else event.accepted = false
                }
                KeyNavigation.backtab: thumbnailButton
                onClicked: root.controller.thumbnail = ""
            }
            Item { objectName: "newPlaylistDividerSlot"; Layout.fillWidth: true; Layout.leftMargin: 20 * root.uiScale; Layout.rightMargin: 20 * root.uiScale; Layout.topMargin: 20 * root.uiScale; Layout.preferredHeight: 0; Item { width: parent.width; height: root.uiScale; y: -.5 * root.uiScale; clip: true; Image { objectName: "newPlaylistOriginalDivider"; width: 673; height: 1; scale: root.uiScale; transformOrigin: Item.TopLeft; source: root.assetBase + "new-playlist-divider.svg"; Accessible.ignored: true } } }
            FormLabel { objectName: "newPlaylistFilesTitle"; Layout.leftMargin: 20 * root.uiScale; Layout.topMargin: 20 * root.uiScale; text: qsTr("Select Files"); font.pixelSize: 16 * root.uiScale; lineHeightMode: Text.FixedHeight; lineHeight: 28 * root.uiScale }
            FormLabel { Layout.fillWidth: true; Layout.leftMargin: 20 * root.uiScale; Layout.rightMargin: 20 * root.uiScale; Layout.topMargin: 5 * root.uiScale; text: qsTr("These files will be added to the playlist."); color: "#6B6B6B"; font.pixelSize: 14 * root.uiScale; lineHeightMode: Text.FixedHeight; lineHeight: 24.5 * root.uiScale }
            Rectangle {
                id: fileBox
                objectName: "newPlaylistDropBox"
                Layout.fillWidth: true
                Layout.leftMargin: 20 * root.uiScale; Layout.rightMargin: 20 * root.uiScale
                Layout.topMargin: 20 * root.uiScale
                Layout.preferredHeight: 186 * root.uiScale
                color: dropArea.containsDrag ? "#20160C" : "#090909"
                radius: 6 * root.uiScale
                border.width: dropArea.containsDrag ? root.uiScale : 0
                border.color: "#D37918"
                // Dashed primitives work with the configured static Qt build,
                // whose Canvas module is unavailable.
                Repeater {
                    model: Math.floor(fileBox.width / (7 * root.uiScale))
                    delegate: Item {
                        required property int index
                        x: index * 7 * root.uiScale
                        width: 4 * root.uiScale; height: fileBox.height
                        Rectangle { anchors.top: parent.top; width: parent.width; height: 2 * root.uiScale; color: "#2D2D2D" }
                        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 2 * root.uiScale; color: "#2D2D2D" }
                    }
                }
                Repeater {
                    model: Math.floor(fileBox.height / (7 * root.uiScale))
                    delegate: Item {
                        required property int index
                        y: index * 7 * root.uiScale
                        width: fileBox.width; height: 4 * root.uiScale
                        Rectangle { anchors.left: parent.left; height: parent.height; width: 2 * root.uiScale; color: "#2D2D2D" }
                        Rectangle { anchors.right: parent.right; height: parent.height; width: 2 * root.uiScale; color: "#2D2D2D" }
                    }
                }
                ColumnLayout {
                    anchors.centerIn: parent
                    width: parent.width - 20 * root.uiScale
                    spacing: 20 * root.uiScale
                    visible: !root.controller || root.controller.files.length === 0
                    FormLabel { Layout.fillWidth: true; text: qsTr("Drag files into the box or click here to select files."); horizontalAlignment: Text.AlignHCenter; color: "#404040"; font.pixelSize: 14 * root.uiScale }
                    FormButton { id: emptyFilesButton; objectName: "newPlaylistSelectFiles"; Layout.alignment: Qt.AlignHCenter; implicitWidth: 84 * root.uiScale; implicitHeight: 30 * root.uiScale; text: ""; contentItem: Item { Image { objectName: "newPlaylistOriginalFilesAdd"; anchors.centerIn: parent; width: 24; height: 24; scale: root.uiScale; source: root.assetBase + "new-playlist-add.svg"; Accessible.ignored: true } } background: Rectangle { radius: 5 * root.uiScale; color: "#161616"; border.width: emptyFilesButton.visualFocus ? root.uiScale : 0; border.color: "#D37918" } enabled: root.editable; onClicked: root.filesRequested(); Accessible.name: qsTr("Select files"); KeyNavigation.tab: cancelButton; KeyNavigation.backtab: removeThumbnail.visible ? removeThumbnail : thumbnailButton; onActiveFocusChanged: if (activeFocus) root.reveal(fileBox) }
                }
                C.ScrollView {
                    anchors.fill: parent
                    anchors.margins: 6 * root.uiScale
                    anchors.bottomMargin: 38 * root.uiScale
                    clip: true
                    visible: !!root.controller && root.controller.files.length > 0
                    C.ScrollBar.horizontal.policy: C.ScrollBar.AlwaysOff
                    ListView {
                        id: selectedFiles
                        objectName: "newPlaylistSelectedFiles"
                        clip: true
                        model: root.controller ? root.controller.files : []
                        delegate: RowLayout {
                            required property var modelData
                            required property int index
                            property alias upButton: moveUp
                            property alias downButton: moveDown
                            property alias removeButton: removeFile
                            width: selectedFiles.width
                            height: 36 * root.uiScale
                            spacing: 4 * root.uiScale
                            FormLabel { text: (index + 1) + "."; color: "#686868"; font.pixelSize: 11 * root.uiScale; Layout.preferredWidth: 24 * root.uiScale }
                            FormLabel { Layout.fillWidth: true; Layout.minimumWidth: 0; text: modelData.title; font.pixelSize: 12 * root.uiScale; wrapMode: Text.NoWrap; elide: Text.ElideRight; C.ToolTip.visible: fileHover.hovered; C.ToolTip.text: modelData.path; HoverHandler { id: fileHover } }
                            FormButton { id: moveUp; objectName: "newPlaylistUp" + index; implicitWidth: 24 * root.uiScale; implicitHeight: 24 * root.uiScale; text: "↑"; enabled: root.editable && index > 0; Accessible.name: qsTr("Move %1 up").arg(modelData.title); KeyNavigation.tab: moveDown.enabled ? moveDown : removeFile; KeyNavigation.backtab: index > 0 ? (selectedFiles.itemAtIndex(index - 1)?.removeButton ?? thumbnailButton) : removeThumbnail.visible ? removeThumbnail : thumbnailButton; onClicked: root.controller.moveFile(index, index - 1) }
                            FormButton { id: moveDown; objectName: "newPlaylistDown" + index; implicitWidth: 24 * root.uiScale; implicitHeight: 24 * root.uiScale; text: "↓"; enabled: root.editable && index + 1 < root.controller.files.length; Accessible.name: qsTr("Move %1 down").arg(modelData.title); KeyNavigation.tab: removeFile; KeyNavigation.backtab: moveUp.enabled ? moveUp : removeThumbnail.visible ? removeThumbnail : thumbnailButton; onClicked: root.controller.moveFile(index, index + 1) }
                            FormButton { id: removeFile; objectName: "newPlaylistRemove" + index; implicitWidth: 24 * root.uiScale; implicitHeight: 24 * root.uiScale; text: "×"; enabled: root.editable; Accessible.name: qsTr("Remove %1").arg(modelData.title); KeyNavigation.tab: index + 1 < selectedFiles.count ? root.firstFileAction(index + 1) : filesButton; KeyNavigation.backtab: moveDown.enabled ? moveDown : moveUp.enabled ? moveUp : removeThumbnail.visible ? removeThumbnail : thumbnailButton; Keys.onTabPressed: event => { if (index + 1 < selectedFiles.count) { root.focusFile(index + 1, Qt.TabFocusReason, false); event.accepted = true } else event.accepted = false }; onClicked: root.controller.removeFile(index) }
                        }
                    }
                }
                FormButton {
                    id: filesButton
                    objectName: "newPlaylistFiles"
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 6 * root.uiScale
                    enabled: root.editable
                    visible: !!root.controller && root.controller.files.length > 0
                    text: qsTr("Add files")
                    KeyNavigation.tab: cancelButton
                    KeyNavigation.backtab: selectedFiles.count > 0 ? (selectedFiles.itemAtIndex(selectedFiles.count - 1)?.removeButton ?? thumbnailButton) : thumbnailButton
                    Keys.onBacktabPressed: event => { if (selectedFiles.count > 0) { root.focusFile(selectedFiles.count - 1, Qt.BacktabFocusReason, true); event.accepted = true } else event.accepted = false }
                    onActiveFocusChanged: if (activeFocus) root.reveal(fileBox)
                    onClicked: root.filesRequested()
                }
                DropArea {
                    id: dropArea
                    anchors.fill: parent
                    enabled: root.editable
                    onEntered: drag => { drag.accepted = drag.hasUrls && root.editable }
                    onDropped: drop => {
                        if (drop.hasUrls && root.appendDroppedFiles(drop.urls)) drop.acceptProposedAction()
                        else drop.accepted = false
                    }
                }
                TapHandler {
                    enabled: root.editable && (!root.controller || root.controller.files.length === 0)
                    onTapped: root.filesRequested()
                }
            }
            FormLabel {
                objectName: "newPlaylistError"
                Layout.fillWidth: true
                Layout.leftMargin: 20 * root.uiScale; Layout.rightMargin: 20 * root.uiScale; Layout.topMargin: 12 * root.uiScale
                text: root.controller ? root.controller.error : ""
                visible: text.length > 0 && !!root.controller && (!root.controller.available || root.controller.name.length > 0 || root.controller.files.length > 0 || root.controller.thumbnail.toString().length > 0)
                color: "#D98779"
                font.pixelSize: 12 * root.uiScale
                Accessible.role: Accessible.StaticText
                Accessible.name: text
            }
            Item { Layout.preferredHeight: 22 * root.uiScale }
        }
    }
    footer: Item {
        implicitHeight: 72 * root.uiScale
        FormButton {
            id: cancelButton
            objectName: "newPlaylistCancel"
            anchors.left: parent.left; anchors.leftMargin: 20 * root.uiScale
            anchors.bottom: parent.bottom; anchors.bottomMargin: 20 * root.uiScale
            text: root.controller && root.controller.partialSave ? qsTr("Close") : qsTr("Cancel")
            enabled: !root.controller || !root.controller.busy
            KeyNavigation.tab: saveButton.enabled ? saveButton : closeButton
            KeyNavigation.backtab: filesButton.visible ? filesButton : emptyFilesButton
            onClicked: root.reject()
        }
        FormButton {
            id: saveButton
            objectName: "newPlaylistSave"
            anchors.right: parent.right; anchors.rightMargin: 20 * root.uiScale
            anchors.bottom: parent.bottom; anchors.bottomMargin: 20 * root.uiScale
            primary: true
            text: root.controller && root.controller.busy ? qsTr("Saving…") : root.controller && root.controller.partialSave ? qsTr("View playlist") : qsTr("Save")
            enabled: !!root.controller && (root.controller.canSave || root.controller.partialSave)
            KeyNavigation.tab: closeButton
            KeyNavigation.backtab: cancelButton
            onClicked: {
                if (root.controller.partialSave) {
                    root.playlistRequested(root.controller.savedPlaylist)
                    root.close()
                } else root.save()
            }
        }
    }
}
