// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
// Original visual design and SVG artwork: Juxtopposed, CC BY 4.0.
import QtQuick
import QtQuick.Controls.Basic as C
import QtQuick.Layouts
import VLC.Style

JuxtopposedMovableDialog {
    id: root
    objectName: "juxtopposedSubtitleAttachments"
    property var controller
    property real uiScale: 1
    property url assetBase: "qrc:/juxtopposed/"
    property Item backdropSource: null
    property Item focusReturnItem: null
    property int activeMediaRow: 0
    property bool online: false
    readonly property var media: controller ? controller.mediaRows : []
    readonly property var subtitles: controller ? controller.subtitleRows : []
    readonly property bool multiple: !!controller && controller.scope !== 0
    readonly property real rowHeight: 40 * uiScale
    readonly property bool narrow: width < 740 * uiScale
    signal filesRequested()
    signal folderRequested()
    signal onlineRequested()
    signal fallbackFocusRequested()

    title: qsTr("Add Subtitles")
    modal: true
    focus: true
    implicitWidth: 1048 * uiScale
    implicitHeight: 800 * uiScale
    width: Math.min(implicitWidth, Math.max(0, parent.width - 32 * uiScale))
    height: Math.min(implicitHeight, Math.max(0, parent.height - 32 * uiScale))
    padding: 0
    closePolicy: C.Popup.NoAutoClose
    function showDialog(returnItem) {
        if (visible) return
        focusReturnItem = returnItem
        activeMediaRow = 0
        online = false
        if (controller) controller.begin()
        open()
        scopeChoice.forceActiveFocus(Qt.PopupFocusReason)
    }
    function submit() { if (controller && controller.canSave && controller.save()) accept() }
    function chooseSubtitle(index) {
        if (!controller) return
        controller.toggle(multiple ? activeMediaRow : 0, index)
        Qt.callLater(() => { if (visible) subtitleRepeater.itemAt(index)?.forceActiveFocus(Qt.OtherFocusReason) })
    }
    function links() {
        if (!multiple) return []
        const result = []
        for (let sub = 0; sub < subtitles.length; ++sub)
            for (const row of subtitles[sub].selectedRows) result.push({from: row, to: sub})
        return result
    }
    onRejected: if (controller) controller.cancel()
    onClosed: {
        if (controller && controller.active) controller.cancel()
        if (focusReturnItem && focusReturnItem.visible && focusReturnItem.enabled)
            focusReturnItem.forceActiveFocus(Qt.PopupFocusReason)
        else fallbackFocusRequested()
        focusReturnItem = null
    }
    Shortcut { sequence: "Escape"; enabled: root.visible; onActivated: root.reject() }
    Shortcut { sequence: "Ctrl+Return"; enabled: root.visible && !!root.controller && root.controller.canSave; onActivated: root.submit() }
    FontLoader { id: inter; source: root.assetBase + "InterVariable.ttf" }
    background: JuxtopposedSettingsBackdrop {
        sourceItem: root.backdropSource
        active: root.visible
        uiScale: root.uiScale
        color: JuxtopposedPalette.color("#0E0E0E")
    }
    C.Overlay.modal: Rectangle { color: JuxtopposedPalette.color("#80000000") }
    component Label: Text {
        color: JuxtopposedPalette.color("#969696")
        font.family: inter.name
        font.pixelSize: 14 * root.uiScale
        textFormat: Text.PlainText
        elide: Text.ElideRight
        verticalAlignment: Text.AlignVCenter
    }
    component Button: C.Button {
        id: button
        property bool primary: false
        implicitWidth: Math.max(100 * root.uiScale, contentItem.implicitWidth + 24 * root.uiScale)
        implicitHeight: 32 * root.uiScale
        activeFocusOnTab: true
        opacity: enabled ? 1 : .5
        Keys.onReturnPressed: clicked()
        Keys.onEnterPressed: clicked()
        background: Rectangle {
            radius: 6 * root.uiScale
            color: button.primary ? JuxtopposedPalette.color("#371F06") : JuxtopposedPalette.color(button.hovered ? "#303030" : "#1B1B1B")
            border.width: button.visualFocus ? root.uiScale : 0
            border.color: JuxtopposedPalette.color("#D37918")
        }
        contentItem: Label {
            text: button.text
            horizontalAlignment: Text.AlignHCenter
            color: JuxtopposedPalette.color(button.primary ? "#D37918" : "#8F8F8F")
            font.weight: Font.DemiBold
        }
    }
    component Choice: C.ComboBox {
        id: choice
        implicitHeight: 34 * root.uiScale
        activeFocusOnTab: true
        textRole: "name"
        indicator: Item {}
        background: Rectangle {
            radius: 6 * root.uiScale
            color: JuxtopposedPalette.color("#151515")
            border.width: choice.visualFocus ? root.uiScale : 0
            border.color: JuxtopposedPalette.color("#D37918")
        }
        contentItem: Item {
            Label { x: 12 * root.uiScale; width: Math.max(0, parent.width - 46 * root.uiScale); height: parent.height; text: choice.displayText }
            Image { anchors.right: parent.right; anchors.rightMargin: 8 * root.uiScale; anchors.verticalCenter: parent.verticalCenter; width: 24; height: 24; scale: root.uiScale; source: root.assetBase + "recording-settings-764c9.svg"; rotation: 180; transform: Scale { origin.x: 12; origin.y: 12; yScale: -1 } Accessible.ignored: true }
        }
        delegate: C.ItemDelegate {
            required property var modelData
            required property int index
            objectName: choice.objectName + "Option" + index
            width: choice.width
            height: 36 * root.uiScale
            text: modelData.name
            Accessible.name: modelData.name
            enabled: modelData.enabled !== false
            opacity: enabled ? 1 : .45
            highlighted: choice.highlightedIndex === index
            contentItem: Label { text: modelData.name; leftPadding: 12 * root.uiScale }
            background: Rectangle { radius: 4 * root.uiScale; color: JuxtopposedPalette.color(parent.highlighted ? "#371F06" : "#151515") }
        }
        popup: C.Popup {
            y: choice.height + 2 * root.uiScale
            width: choice.width
            padding: 2 * root.uiScale
            implicitHeight: contentItem.implicitHeight + padding * 2
            contentItem: ListView { clip: true; implicitHeight: contentHeight; model: choice.popup.visible ? choice.delegateModel : null; currentIndex: choice.highlightedIndex }
            background: Rectangle { color: JuxtopposedPalette.color("#151515"); radius: 6 * root.uiScale; border.width: root.uiScale; border.color: JuxtopposedPalette.color("#303030") }
        }
    }
    header: Rectangle {
        implicitHeight: (root.narrow ? 140 : 96) * root.uiScale
        color: JuxtopposedPalette.color("#090909")
        radius: 14 * root.uiScale
        Rectangle { anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; height: parent.height / 2; color: parent.color }
        Label { x: 19 * root.uiScale; y: 9 * root.uiScale; height: 32 * root.uiScale; text: root.title; color: JuxtopposedPalette.color("#8F8F8F") }
        JuxtopposedDialogDragArea { popup: root; x: 0; y: 0; width: Math.max(0, parent.width - 50 * root.uiScale); height: 48 * root.uiScale }
        C.ToolButton {
            id: closeButton
            objectName: "subtitleAttachmentClose"
            x: parent.width - 41 * root.uiScale; y: 9 * root.uiScale
            width: 32 * root.uiScale; height: width
            activeFocusOnTab: true
            Accessible.name: qsTr("Close Add Subtitles")
            KeyNavigation.tab: scopeChoice
            KeyNavigation.backtab: saveButton.enabled ? saveButton : cancelButton
            onClicked: root.reject()
            background: Rectangle { radius: 4 * root.uiScale; color: closeButton.hovered ? JuxtopposedPalette.color("#303030") : "transparent"; border.width: closeButton.visualFocus ? root.uiScale : 0; border.color: JuxtopposedPalette.color("#D37918") }
            contentItem: Item { Image { anchors.centerIn: parent; width: 24; height: 24; scale: root.uiScale; source: root.assetBase + "recording-settings-9c2da.svg"; rotation: 180; transform: Scale { origin.x: 12; origin.y: 12; yScale: -1 } Accessible.ignored: true } }
        }
        Choice {
            id: scopeChoice
            objectName: "subtitleAttachmentScope"
            x: 9 * root.uiScale; y: 53 * root.uiScale
            width: root.narrow ? (parent.width - 28 * root.uiScale) / 2 : 200 * root.uiScale
            model: [{name:qsTr("To This File"),enabled:true}, {name:qsTr("To This Folder"),enabled:!!root.controller && root.controller.folderAvailable}, {name:qsTr("To This Playlist"),enabled:!!root.controller && root.controller.playlistAvailable}]
            currentIndex: root.controller ? root.controller.scope : 0
            enabled: !!root.controller && root.controller.editable
            Accessible.name: qsTr("Subtitle attachment scope")
            Accessible.description: qsTr("File choices take precedence over playlist choices, then folder choices. Playlist scope requires a saved playlist.")
            KeyNavigation.backtab: closeButton
            KeyNavigation.tab: sourceChoice
            onActivated: index => { if (controller) controller.setScope(index); root.activeMediaRow = 0 }
        }
        Choice {
            id: sourceChoice
            objectName: "subtitleAttachmentSource"
            x: scopeChoice.x + scopeChoice.width + 10 * root.uiScale; y: scopeChoice.y
            width: scopeChoice.width
            model: [{name:qsTr("From Computer")}, {name:qsTr("From OpenSubtitles")}]
            currentIndex: root.online ? 1 : 0
            enabled: !!root.controller && root.controller.editable
            Accessible.name: qsTr("Subtitle source")
            KeyNavigation.tab: folderButton
            KeyNavigation.backtab: scopeChoice
            onActivated: index => { root.online = index === 1 }
        }
        C.Button {
            id: folderButton
            objectName: "subtitleAttachmentFolder"
            x: root.narrow ? 9 * root.uiScale : sourceChoice.x + sourceChoice.width + 10 * root.uiScale
            y: root.narrow ? 97 * root.uiScale : scopeChoice.y
            width: Math.max(0, parent.width - x - 9 * root.uiScale)
            height: 34 * root.uiScale
            activeFocusOnTab: true
            enabled: !root.online && !!root.controller && root.controller.editable
            Accessible.name: qsTr("Choose subtitle folder")
            Accessible.description: root.controller ? root.controller.sourceLabel : ""
            KeyNavigation.backtab: sourceChoice
            onClicked: root.folderRequested()
            background: Rectangle { color: JuxtopposedPalette.color("#151515"); radius: 6 * root.uiScale; border.width: folderButton.visualFocus ? root.uiScale : 0; border.color: JuxtopposedPalette.color("#D37918") }
            contentItem: Item {
                Image { x: 8 * root.uiScale; anchors.verticalCenter: parent.verticalCenter; width: 24; height: 24; scale: root.uiScale; source: root.assetBase + "recording-settings-cf6e1.svg"; rotation: 180; transform: Scale { origin.x: 12; origin.y: 12; yScale: -1 } Accessible.ignored: true }
                Label { x: 42 * root.uiScale; width: Math.max(0, parent.width - x - 8 * root.uiScale); height: parent.height; text: root.controller ? root.controller.sourceLabel : ""; elide: Text.ElideMiddle }
            }
        }
    }
    contentItem: ColumnLayout {
        spacing: 0
        C.ScrollView {
            id: scroll
            objectName: "subtitleAttachmentFiles"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            visible: !root.online
            contentWidth: availableWidth
            C.ScrollBar.horizontal.policy: C.ScrollBar.AlwaysOff
            Item {
                id: rows
                width: scroll.availableWidth
                implicitHeight: Math.max(1, Math.max(root.media.length, root.subtitles.length)) * root.rowHeight + 44 * root.uiScale
                readonly property real rightX: root.multiple ? width / 2 + 4 * root.uiScale : 20 * root.uiScale
                Repeater {
                    model: root.links()
                    delegate: Item {
                        required property var modelData
                        z: 1
                        objectName: "subtitleAttachmentLink" + modelData.from + "_" + modelData.to
                        x: Math.min(260 * root.uiScale, rows.width * .29)
                        y: 22 * root.uiScale + (modelData.from + .5) * root.rowHeight
                        readonly property real endX: rows.rightX - 12 * root.uiScale
                        readonly property real dy: (modelData.to - modelData.from) * root.rowHeight
                        readonly property real length: Math.sqrt((endX - x) * (endX - x) + dy * dy)
                        width: length; height: root.uiScale
                        rotation: Math.atan2(dy, endX - x) * 180 / Math.PI
                        transformOrigin: Item.Left
                        Image { anchors.fill: parent; source: root.assetBase + "subtitle-attachment-b0920.svg"; fillMode: Image.Stretch; Accessible.ignored: true }
                    }
                }
                Repeater {
                    id: mediaRepeater
                    model: root.multiple ? root.media : []
                    delegate: C.ItemDelegate {
                        id: mediaButton
                        objectName: "subtitleAttachmentMedia" + index
                        required property var modelData
                        required property int index
                        x: 20 * root.uiScale; y: 22 * root.uiScale + index * root.rowHeight
                        width: Math.max(0, rows.width / 2 - 26 * root.uiScale); height: 36 * root.uiScale
                        activeFocusOnTab: true
                        enabled: !!root.controller && root.controller.editable
                        Accessible.name: modelData.title
                        Accessible.description: modelData.subtitles.length ? qsTr("Matched subtitles: %1").arg(modelData.subtitles.join(", ")) : qsTr("No subtitles matched")
                        onClicked: root.activeMediaRow = index
                        background: Rectangle { radius: 6 * root.uiScale; color: JuxtopposedPalette.color(mediaButton.index === root.activeMediaRow || mediaDrop.containsDrag ? "#151515" : "#0E0E0E"); border.width: mediaButton.visualFocus ? root.uiScale : 0; border.color: JuxtopposedPalette.color("#D37918") }
                        contentItem: Item {
                            Image { x: 8 * root.uiScale; anchors.verticalCenter: parent.verticalCenter; width: 24; height: 24; scale: root.uiScale; source: root.assetBase + "subtitle-attachment-a8689.svg"; rotation: 180; transform: Scale { origin.x: 12; origin.y: 12; yScale: -1 } Accessible.ignored: true }
                            Label { x: 46 * root.uiScale; width: Math.max(0, Math.min(parent.width - x - 8 * root.uiScale, 190 * root.uiScale)); height: parent.height; text: mediaButton.modelData.title }
                        }
                        DropArea {
                            id: mediaDrop
                            anchors.fill: parent
                            onEntered: drag => { drag.accepted = !!drag.source && "subtitleIndex" in drag.source }
                            onDropped: drop => {
                                if (drop.source && "subtitleIndex" in drop.source) {
                                    const service = root.controller
                                    const row = mediaButton.index
                                    const subtitle = drop.source.subtitleIndex
                                    root.activeMediaRow = row
                                    drop.acceptProposedAction()
                                    // changed() rebuilds the delegate models;
                                    // finish using this row before the call.
                                    service.toggle(row, subtitle)
                                }
                            }
                        }
                    }
                }
                Repeater {
                    id: subtitleRepeater
                    model: root.subtitles
                    delegate: C.ItemDelegate {
                        id: subtitleButton
                        objectName: "subtitleAttachmentSubtitle" + index
                        required property var modelData
                        required property int index
                        property int subtitleIndex: index
                        readonly property bool selected: modelData.selectedRows.indexOf(root.multiple ? root.activeMediaRow : 0) >= 0
                        x: rows.rightX; y: 22 * root.uiScale + index * root.rowHeight
                        width: Math.max(0, rows.width - x - 20 * root.uiScale); height: 36 * root.uiScale
                        activeFocusOnTab: true
                        enabled: !!root.controller && root.controller.editable
                        checked: selected
                        Accessible.role: Accessible.CheckBox
                        Accessible.name: modelData.title
                        Accessible.checkable: true
                        Accessible.checked: selected
                        Accessible.description: !modelData.available ? qsTr("Missing subtitle file") : selected ? qsTr("Attached to the selected media file") : qsTr("Choose this subtitle for the selected media file")
                        opacity: modelData.available ? 1 : .5
                        onClicked: root.chooseSubtitle(index)
                        Item {
                            id: dragProxy
                            parent: rows
                            width: 1; height: 1
                            property int subtitleIndex: subtitleButton.index
                            Drag.active: subtitleDrag.active
                            Drag.source: dragProxy
                        }
                        DragHandler {
                            id: subtitleDrag
                            enabled: root.multiple
                            target: dragProxy
                            onActiveChanged: {
                                if (active) {
                                    dragProxy.x = subtitleButton.x + centroid.position.x
                                    dragProxy.y = subtitleButton.y + centroid.position.y
                                } else dragProxy.Drag.drop()
                            }
                        }
                        background: Rectangle { radius: 6 * root.uiScale; color: JuxtopposedPalette.color(subtitleButton.selected || subtitleButton.hovered ? "#090909" : "#0E0E0E"); border.width: subtitleButton.visualFocus ? root.uiScale : 0; border.color: JuxtopposedPalette.color("#D37918") }
                        contentItem: Item {
                            Image { x: 8 * root.uiScale; anchors.verticalCenter: parent.verticalCenter; width: 24; height: 24; scale: root.uiScale; source: root.assetBase + "subtitle-attachment-8abe2.svg"; Accessible.ignored: true }
                            Label { x: 46 * root.uiScale; width: Math.max(0, parent.width - x - 38 * root.uiScale); height: parent.height; text: subtitleButton.modelData.title }
                            Image { visible: subtitleButton.selected; anchors.right: parent.right; anchors.rightMargin: 8 * root.uiScale; anchors.verticalCenter: parent.verticalCenter; width: 24; height: 24; scale: root.uiScale; source: root.assetBase + "subtitle-attachment-d74b3.svg"; Accessible.ignored: true }
                        }
                    }
                }
                Label {
                    x: 28 * root.uiScale; y: 22 * root.uiScale
                    width: Math.max(0, parent.width - 56 * root.uiScale)
                    visible: root.subtitles.length === 0
                    text: qsTr("No subtitle files in this folder. Choose another folder or add files.")
                    wrapMode: Text.WordWrap
                }
            }
        }
        Item {
            Layout.fillWidth: true; Layout.fillHeight: true
            visible: root.online
            ColumnLayout {
                anchors.centerIn: parent
                width: Math.max(0, parent.width - 56 * root.uiScale)
                Label { Layout.fillWidth: true; text: qsTr("Search OpenSubtitles, then choose the downloaded file to save its matches here."); wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter }
                Button { Layout.alignment: Qt.AlignHCenter; text: qsTr("Search OpenSubtitles…"); onClicked: { root.reject(); Qt.callLater(() => root.onlineRequested()) } }
            }
        }
        Label {
            objectName: "subtitleAttachmentError"
            Layout.fillWidth: true
            Layout.leftMargin: 20 * root.uiScale; Layout.rightMargin: 20 * root.uiScale
            Layout.topMargin: text.length ? 8 * root.uiScale : 0
            Layout.bottomMargin: text.length ? 8 * root.uiScale : 0
            visible: text.length > 0
            text: root.controller ? root.controller.error : ""
            color: JuxtopposedPalette.color("#D98779")
            wrapMode: Text.WordWrap
            elide: Text.ElideNone
            Accessible.role: Accessible.AlertMessage
            Accessible.name: text
        }
    }
    footer: Item {
        implicitHeight: 92 * root.uiScale
        Label {
            x: 20 * root.uiScale; y: 0
            width: Math.max(0, parent.width - 40 * root.uiScale)
            text: root.multiple ? qsTr("Select a media file, then choose its subtitles. File choices override playlist, then folder choices.") : qsTr("The first selected subtitle is the default; other selections remain in the track list.")
            font.pixelSize: 12 * root.uiScale
            wrapMode: Text.WordWrap
            maximumLineCount: 2
        }
        RowLayout {
            anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
            anchors.margins: 20 * root.uiScale
            spacing: 10 * root.uiScale
            Button { id: cancelButton; objectName: "subtitleAttachmentCancel"; text: qsTr("Cancel"); onClicked: root.reject() }
            Button { objectName: "subtitleAttachmentChooseFiles"; text: qsTr("Add files…"); enabled: !root.online && !!root.controller && root.controller.editable; onClicked: root.filesRequested() }
            Button { objectName: "subtitleAttachmentMatchNames"; visible: root.multiple && !root.narrow; text: qsTr("Match names"); enabled: !!root.controller && root.controller.editable; onClicked: root.controller.matchNames() }
            Item { Layout.fillWidth: true }
            Button { id: saveButton; objectName: "subtitleAttachmentSave"; text: qsTr("Save"); primary: true; enabled: !root.online && !!root.controller && root.controller.canSave; KeyNavigation.tab: closeButton; onClicked: root.submit() }
        }
    }
}
