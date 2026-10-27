/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * Visual design by Juxtopposed: https://www.youtube.com/@juxtopposed
 * Code licensed under GPLv2 or later; artwork under CC BY 4.0.
 *****************************************************************************/
import QtQuick
import QtQuick.Templates as T
import QtQml.Models
import VLC.MainInterface
import VLC.MediaLibrary
import VLC.Player
import VLC.Style
import VLC.Util
import VLC.Widgets as Widgets

// Load this adapter only when the media-library QML module is available.
Widgets.ListViewExt {
    id: root

    property real uiScale: VLCStyle.scale
    property Item navigationParent
    readonly property bool canAdd: bookmarkModel.canAdd && Player.isStarted && Player.seekable
    property int selectionRevision: 0
    readonly property int selectionCount: {
        selectionRevision // Model resets clear selection without selectionChanged.
        return selectionModel.selectedIndexesFlat.length
    }

    model: MLBookmarkModel {
        id: bookmarkModel
        player: Player
        ml: MediaLib
    }
    selectionModel: ListSelectionModel { model: root.model }
    spacing: 12 * uiScale
    clip: true
    focus: true

    Accessible.role: Accessible.List
    Accessible.name: qsTr("Bookmarks")
    Navigation.parentItem: navigationParent

    Connections {
        target: bookmarkModel
        function onRowsInserted() {
            if (root.currentIndex < 0 && root.count > 0)
                root.currentIndex = 0
        }
        function onModelReset() {
            root.selectionRevision++
            const count = bookmarkModel.rowCount()
            root.currentIndex = count > 0 ? Math.min(Math.max(root.currentIndex, 0), count - 1) : -1
        }
        function onRowsRemoved() {
            root.selectionRevision++
            const count = bookmarkModel.rowCount()
            root.currentIndex = count > 0 ? Math.min(Math.max(root.currentIndex, 0), count - 1) : -1
        }
    }

    function formatTimeMs(milliseconds) : string {
        if (!Number.isFinite(milliseconds) || milliseconds < 0)
            return "—"
        const value = Math.floor(milliseconds)
        const seconds = Math.floor(value / 1000)
        const hours = Math.floor(seconds / 3600).toString().padStart(2, "0")
        const minutes = Math.floor(seconds / 60 % 60).toString().padStart(2, "0")
        const remainder = (seconds % 60).toString().padStart(2, "0")
        const fraction = (value % 1000).toString().padStart(3, "0")
        return hours + ":" + minutes + ":" + remainder + ":" + fraction
    }

    function addBookmark() {
        if (canAdd)
            bookmarkModel.add()
    }

    function removeSelected() {
        const indexes = selectionModel.selectedIndexesFlat.map((row) => bookmarkModel.index(row, 0))
        if (indexes.length > 0)
            bookmarkModel.remove(indexes)
        root.forceActiveFocus()
    }

    function activate(index) {
        if (index >= 0 && index < root.count)
            bookmarkModel.select(bookmarkModel.index(index, 0))
    }

    function selectAndActivate(index) {
        if (index < 0 || index >= root.count)
            return
        root.selectionModel.select(index, ItemSelectionModel.ClearAndSelect)
        root.currentIndex = index
        root.activate(index)
    }

    onActionAtIndex: (index) => root.activate(index)
    Keys.onDeletePressed: root.removeSelected()

    delegate: T.Control {
        id: row
        required property int index
        required property string name
        required property var timeMs
        readonly property bool selected: root.selectionModel.selectedIndexesFlat.includes(index)

        width: root.width
        height: 32 * root.uiScale
        padding: 0
        hoverEnabled: true
        Accessible.role: Accessible.ListItem
        Accessible.name: name
        Accessible.description: contentItem.durationText
        Accessible.selected: selected
        Accessible.onPressAction: root.selectAndActivate(index)

        readonly property ColorContext colorContext: ColorContext {
            id: theme
            colorSet: ColorContext.Item
            focused: row.activeFocus
            hovered: row.hovered
            enabled: row.enabled
        }

        background: Widgets.AnimatedBackground {
            color: row.selected ? theme.bg.highlight : row.hovered ? theme.bg.primary : "transparent"
            border.color: row.visualFocus ? theme.visualFocus : "transparent"
            enabled: theme.initialized
        }
        contentItem: JuxtopposedQueueRow {
            uiScale: root.uiScale
            showMove: false
            title: row.name
            durationText: root.formatTimeMs(row.timeMs)
            titleColor: row.selected ? theme.fg.primary : JuxtopposedPalette.color("#969696")
        }

        // A Control consumes pointer events before its own handlers.
        // Match PlaylistDelegate's child-item input ownership.
        Item {
            id: bookmarkPointerArea
            anchors.fill: parent
            TapHandler {
                acceptedDevices: PointerDevice.AllDevices & ~PointerDevice.TouchScreen
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                gesturePolicy: TapHandler.ReleaseWithinBounds
                onSingleTapped: {
                    row.forceActiveFocus(Qt.MouseFocusReason)
                    root.selectionModel.updateSelection(point.modifiers, root.currentIndex, row.index)
                    root.currentIndex = row.index
                }
                onDoubleTapped: (eventPoint, button) => {
                    if (button === Qt.LeftButton)
                        root.activate(row.index)
                }
            }
            DelegateTouchTapHandler {
                delegate: row
                onSingleTapped: root.selectAndActivate(row.index)
            }
        }
    }

    T.Label {
        anchors.centerIn: parent
        visible: root.count === 0
        text: qsTr("No bookmarks yet")
        color: JuxtopposedPalette.color("#969696")
        font.pixelSize: 14 * root.uiScale
    }
}
