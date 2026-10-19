// SPDX-License-Identifier: GPL-2.0-or-later
// Juxtopposed's 31-tile palette, with bounded movement and keyboard alternatives.
// Draft preview never invokes playback actions. The native coordinator owns Save.
import QtQuick
import QtQuick.Controls.Basic as C
import QtQuick.Layouts
import VLC.Style

C.Dialog {
    id: root
    objectName: "juxtopposedPlayerCustomizer"
    title: qsTr("Customize Player")
    property var controller: null
    readonly property var layout: controller ? controller.layout : null
    property real uiScale: 1
    property url assetBase: "qrc:/juxtopposed/"
    property string iconFontFamily: ""
    property var tileGlyph: function(id) { return layout ? layout.tileInfo(id).glyph ?? "" : "" }
    property Component previewComponent
    property Item focusReturnItem: null
    property int playerIndex: 0
    property int selectedPaletteId: -1
    property var selection: null
    property var drag: null
    readonly property bool busy: !!controller && controller.busy
    readonly property bool compact: width < 650 * uiScale || height < 560 * uiScale
    readonly property var draftData: layout ? (playerIndex === 0 ? layout.draft.video : layout.draft.audio) : null
    readonly property bool canSave: !!controller && controller.available && !!layout && layout.editing && !busy
    readonly property string error: (controller ? controller.lastError : qsTr("Player profiles are unavailable.")) || (layout ? layout.lastError : "")
    signal fallbackFocusRequested()
    modal: true
    focus: true
    x: 0; y: 0
    width: parent ? parent.width : 0
    height: parent ? parent.height : 0
    padding: 0
    header: null; footer: null
    background: Item {}
    C.Overlay.modal: Rectangle {color: JuxtopposedPalette.color("#80000000")}
    closePolicy: busy ? C.Popup.NoAutoClose : C.Popup.CloseOnEscape

    function showDialog(returnItem) {
        if (visible) return
        if (!controller || !controller.beginLayoutEdit()) { fallbackFocusRequested(); return }
        focusReturnItem = returnItem
        playerIndex = 0; selection = null; selectedPaletteId = -1; drag = null
        panel.requestedX = NaN; panel.requestedY = NaN
        open()
        Qt.callLater(() => cancelButton.forceActiveFocus(Qt.PopupFocusReason))
    }
    function save() { if (canSave && controller.saveLayoutDraft()) accept() }
    function reveal(item) {
        const flick = scroll.contentItem
        if (!flick || flick.contentY === undefined || !flick.contentItem) return
        const point = item.mapToItem(flick.contentItem, 0, 0)
        if (point.y < flick.contentY) flick.contentY = Math.max(0, point.y)
        else if (point.y + item.height > flick.contentY + flick.height)
            flick.contentY = Math.min(Math.max(0, flick.contentHeight - flick.height), point.y + item.height - flick.height)
    }
    function changedPlayer(index) { if (!busy) {drag = null; selection = null; playerIndex = index} }
    function addSelected(column) {
        if (!canSave || selectedPaletteId < 0) return
        selection = null
        layout.insertTile(playerIndex, column, draftData.columns[column].length, selectedPaletteId)
    }
    function removeSelection() {
        if (!canSave || !selection) return
        const previous = selection; selection = null
        layout.removeTile(playerIndex, previous.column, previous.index)
    }
    function moveSelection(direction) {
        if (!canSave || !selection) return
        const previous = selection; selection = null
        const count = draftData.columns[previous.column].length
        if (direction < 0 && previous.index === 0) {
            if (previous.column > 0) layout.moveTile(playerIndex, previous.column, previous.index, previous.column - 1, draftData.columns[previous.column - 1].length)
        } else if (direction > 0 && previous.index === count - 1) {
            if (previous.column < 2) layout.moveTile(playerIndex, previous.column, previous.index, previous.column + 1, 0)
        } else layout.moveTile(playerIndex, previous.column, previous.index, previous.column, previous.index + (direction > 0 ? 2 : -1))
    }
    function beginDrag(column, index, tile, item, point) {
        if (!canSave) return
        const mapped = item.mapToItem(surface, point.x, point.y)
        drag = {column: column, index: index, tile: tile, start: mapped, point: mapped, moved: false, player: playerIndex}
    }
    function moveDrag(item, point) {
        if (!drag) return
        const mapped = item.mapToItem(surface, point.x, point.y)
        const moved = drag.moved || Math.abs(mapped.x - drag.start.x) + Math.abs(mapped.y - drag.start.y) > 6 * uiScale
        drag = {column: drag.column, index: drag.index, tile: drag.tile, start: drag.start, point: mapped, moved: moved, player: drag.player}
    }
    function endDrag(item, point) {
        if (!drag) return
        moveDrag(item, point)
        const previous = drag; drag = null
        if (!previous.moved || !canSave || previous.player !== playerIndex) return
        const destination = preview.item?.layoutRow?.destination(item, point)
        selection = null
        if (destination) {
            if (previous.column < 0) layout.insertTile(playerIndex, destination.column, destination.index, previous.tile)
            else layout.moveTile(playerIndex, previous.column, previous.index, destination.column, destination.index)
        } else if (previous.column >= 0 && previous.point.x >= panel.x && previous.point.x <= panel.x + panel.width
                && previous.point.y >= panel.y && previous.point.y <= panel.y + panel.height) {
            layout.removeTile(playerIndex, previous.column, previous.index)
        }
    }
    onBusyChanged: if (busy) drag = null
    onWidthChanged: drag = null
    onHeightChanged: drag = null
    onErrorChanged: if (visible && error.length > 0) Qt.callLater(() => root.reveal(errorLabel))
    Connections {
        target: root.layout
        function onDraftChanged() { root.drag = null; root.selection = null }
    }
    onClosed: {
        drag = null
        if (layout) layout.cancelEdit()
        if (focusReturnItem && focusReturnItem.visible && focusReturnItem.enabled) focusReturnItem.forceActiveFocus(Qt.PopupFocusReason)
        else fallbackFocusRequested()
        focusReturnItem = null
    }
    Shortcut {sequence: "Ctrl+Return"; enabled: root.visible && root.canSave; onActivated: root.save()}
    FontLoader { id: inter; source: root.assetBase + "InterVariable.ttf" }
    component Label: C.Label {
        font.family: inter.name; font.pixelSize: 14 * root.uiScale
        color: JuxtopposedPalette.color("#969696")
        textFormat: Text.PlainText; wrapMode: Text.WordWrap
    }
    component Button: C.Button {
        id: button
        property bool scrollEntry: false
        implicitHeight: (root.compact ? 28 : 34) * root.uiScale
        implicitWidth: Math.max(90 * root.uiScale, contentItem.implicitWidth + 24 * root.uiScale)
        activeFocusOnTab: true
        enabled: !root.busy
        opacity: enabled ? 1 : 0.45
        onActiveFocusChanged: if (activeFocus && scrollEntry) Qt.callLater(() => root.reveal(button))
        contentItem: Label {text: button.text; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter}
        background: Rectangle {radius: 6 * root.uiScale; color: JuxtopposedPalette.color(button.checked ? "#371F06" : button.hovered ? "#404040" : "#303030");
            border.width: button.visualFocus ? 2 * root.uiScale : 0; border.color: JuxtopposedPalette.color("#D37918")}
    }
    contentItem: Item {
        id: surface
        Rectangle {
            id: panel
            objectName: "customizerPalettePanel"
            property real requestedX: NaN
            property real requestedY: NaN
            width: Math.min(1680 * root.uiScale, Math.max(0, parent.width - (root.compact ? 16 : 32) * root.uiScale))
            height: Math.min(840 * root.uiScale, Math.max(0, preview.y - (root.compact ? 16 : 32) * root.uiScale))
            x: Math.max(0, Math.min(isFinite(requestedX) ? requestedX : (parent.width - width) / 2, parent.width - width))
            y: Math.max(0, Math.min(isFinite(requestedY) ? requestedY : (preview.y - height) / 2, preview.y - height))
            function moveFromHeader(newX, newY) { requestedX = newX; requestedY = newY }
            radius: 14 * root.uiScale
            color: JuxtopposedPalette.color("#111111")
            ColumnLayout {
                anchors.fill: parent; anchors.margins: (root.compact ? 12 : 24) * root.uiScale; spacing: (root.compact ? 8 : 12) * root.uiScale
                RowLayout {
                    Layout.fillWidth: true
                    Item {
                        objectName: "customizerHeader"
                        Layout.fillWidth: true; implicitHeight: (root.compact ? 28 : 32) * root.uiScale
                        JuxtopposedDialogDragArea {anchors.fill: parent; popup: panel}
                        Label {text: qsTr("Customize Player"); font.pixelSize: 20 * root.uiScale; anchors.verticalCenter: parent.verticalCenter}
                    }
                    Button {objectName: "customizerClose"; text: qsTr("Close"); implicitWidth: 70 * root.uiScale; onClicked: root.reject()}
                }
                C.ScrollView {
                    id: scroll
                    Layout.fillWidth: true; Layout.fillHeight: true
                    clip: true; contentWidth: availableWidth
                    ColumnLayout {
                        width: scroll.availableWidth; spacing: 12 * root.uiScale
                        Label {Layout.fillWidth: true; text: qsTr("Drag items out of the box into the player to add or reorder them. Drag items back into the box to remove them.")}
                        Flow {
                            Layout.fillWidth: true; Layout.preferredHeight: childrenRect.height; spacing: 8 * root.uiScale
                            Button {objectName: "customizerVideo"; text: qsTr("Video player"); checkable: true; checked: root.playerIndex === 0; onClicked: root.changedPlayer(0)}
                            Button {objectName: "customizerAudio"; text: qsTr("Music player"); checkable: true; checked: root.playerIndex === 1; onClicked: root.changedPlayer(1)}
                            C.CheckBox {
                                id: timelineChoice
                                objectName: "customizerTimelineAbove"
                                text: qsTr("Timeline above controls")
                                font.family: inter.name; font.pixelSize: 14 * root.uiScale
                                checked: root.draftData ? root.draftData.timelineAbove : false
                                enabled: root.canSave
                                onActiveFocusChanged: if (activeFocus) Qt.callLater(() => root.reveal(this))
                                onToggled: root.layout.setTimelineAbove(root.playerIndex, checked)
                                palette.windowText: JuxtopposedPalette.color("#969696")
                                indicator: Rectangle {
                                    implicitWidth: 18 * root.uiScale; implicitHeight: 18 * root.uiScale
                                    x: timelineChoice.leftPadding; y: (timelineChoice.height - height) / 2
                                    radius: 4 * root.uiScale
                                    color: JuxtopposedPalette.color(timelineChoice.checked ? "#371F06" : "#303030")
                                    border.width: timelineChoice.visualFocus ? 2 * root.uiScale : root.uiScale
                                    border.color: JuxtopposedPalette.color(timelineChoice.visualFocus ? "#D37918" : "#454545")
                                    Text {anchors.centerIn: parent; text: timelineChoice.checked ? "✓" : "";
                                        font.pixelSize: 14 * root.uiScale; color: JuxtopposedPalette.color("#D37918")}
                                }
                                contentItem: Label {
                                    text: timelineChoice.text
                                    leftPadding: timelineChoice.indicator.width + 8 * root.uiScale
                                    verticalAlignment: Text.AlignVCenter
                                }
                            }
                        }
                        GridLayout {
                            id: palette
                            Layout.fillWidth: true
                            columns: Math.max(1, Math.min(9, Math.floor(width / (126 * root.uiScale))))
                            columnSpacing: 8 * root.uiScale; rowSpacing: 8 * root.uiScale
                            Repeater {
                                model: root.layout ? root.layout.palette : []
                                delegate: C.Button {
                                    id: tile
                                    required property var modelData
                                    required property int index
                                    objectName: "customizerPaletteTile" + index
                                    Layout.fillWidth: true; Layout.preferredHeight: (root.compact ? 76 : 110) * root.uiScale
                                    activeFocusOnTab: true; enabled: root.canSave
                                    Accessible.name: modelData.name
                                    contentItem: ColumnLayout {
                                        spacing: (root.compact ? 4 : 12) * root.uiScale
                                        Text {Layout.alignment: Qt.AlignHCenter; text: root.tileGlyph(Number(tile.modelData.id)); font.family: root.layout ? (root.layout.tileInfo(Number(tile.modelData.id)).fontFamily ?? root.iconFontFamily) : root.iconFontFamily;
                                            font.pixelSize: (root.compact ? 22 : 30) * root.uiScale; color: JuxtopposedPalette.color("#969696")}
                                        Label {Layout.fillWidth: true; Layout.fillHeight: true; text: tile.modelData.name; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignTop}
                                    }
                                    background: Rectangle {
                                        radius: 5 * root.uiScale; color: JuxtopposedPalette.color(root.selectedPaletteId === Number(tile.modelData.id) ? "#371F06" : tile.hovered ? "#191919" : "#111111")
                                        border.width: tile.visualFocus ? 2 * root.uiScale : 0; border.color: JuxtopposedPalette.color("#D37918")
                                    }
                                    onClicked: root.selectedPaletteId = Number(modelData.id)
                                    onActiveFocusChanged: if (activeFocus) Qt.callLater(() => root.reveal(tile))
                                    MouseArea {
                                        anchors.fill: parent; preventStealing: true
                                        cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                                        onPressed: mouse => {tile.forceActiveFocus(Qt.MouseFocusReason); root.beginDrag(-1, -1, Number(tile.modelData.id), tile, Qt.point(mouse.x, mouse.y))}
                                        onPositionChanged: mouse => {if (pressed) root.moveDrag(tile, Qt.point(mouse.x, mouse.y))}
                                        onReleased: mouse => root.endDrag(tile, Qt.point(mouse.x, mouse.y))
                                        onClicked: root.selectedPaletteId = Number(tile.modelData.id)
                                        onCanceled: root.drag = null
                                    }
                                }
                            }
                        }
                        Flow {
                            Layout.fillWidth: true; Layout.preferredHeight: childrenRect.height; spacing: 8 * root.uiScale
                            Button {objectName: "customizerAddLeft"; scrollEntry: true; text: qsTr("Add left"); enabled: root.canSave && root.selectedPaletteId >= 0; onClicked: root.addSelected(0)}
                            Button {objectName: "customizerAddCentre"; scrollEntry: true; text: qsTr("Add centre"); enabled: root.canSave && root.selectedPaletteId >= 0; onClicked: root.addSelected(1)}
                            Button {objectName: "customizerAddRight"; scrollEntry: true; text: qsTr("Add right"); enabled: root.canSave && root.selectedPaletteId >= 0; onClicked: root.addSelected(2)}
                            Button {scrollEntry: true; text: qsTr("Move left"); enabled: root.canSave && !!root.selection; onClicked: root.moveSelection(-1)}
                            Button {scrollEntry: true; text: qsTr("Move right"); enabled: root.canSave && !!root.selection; onClicked: root.moveSelection(1)}
                            Button {scrollEntry: true; text: qsTr("Remove"); enabled: root.canSave && !!root.selection; onClicked: root.removeSelection()}
                        }
                        Label {id: errorLabel; objectName: "customizerError"; Layout.fillWidth: true; visible: root.error.length > 0; text: root.error; color: JuxtopposedPalette.color("#ED9B45"); Accessible.role: Accessible.AlertMessage}
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Button {id: cancelButton; objectName: "customizerCancel"; text: qsTr("Cancel"); onClicked: root.reject()}
                    Item {Layout.fillWidth: true}
                    Button {objectName: "customizerSave"; text: qsTr("Save"); enabled: root.canSave; onClicked: root.save()}
                }
            }
        }
        Loader {
            id: preview
            objectName: "customizerPreview"
            anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
            height: item ? item.implicitHeight : 94 * root.uiScale
            sourceComponent: root.previewComponent
        }
        Rectangle {
            visible: !!root.drag && root.drag.moved
            x: root.drag ? root.drag.point.x - width / 2 : 0
            y: root.drag ? root.drag.point.y - height / 2 : 0
            width: 140 * root.uiScale; height: 44 * root.uiScale; z: 100
            color: JuxtopposedPalette.color("#303030"); radius: 6 * root.uiScale
            border.color: JuxtopposedPalette.color("#D37918")
            Label {anchors.fill: parent; anchors.margins: 4 * root.uiScale; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter;
                text: root.drag && root.layout ? root.layout.tileInfo(root.drag.tile).name : ""}
        }
    }
}
