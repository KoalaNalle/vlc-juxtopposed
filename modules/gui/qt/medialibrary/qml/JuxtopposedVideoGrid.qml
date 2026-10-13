/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * Visual design by Juxtopposed: https://www.youtube.com/@juxtopposed
 * Code licensed under GPLv2 or later.
 *****************************************************************************/
import QtQuick
import QtQml.Models
import VLC.MainInterface
import VLC.MediaLibrary
import VLC.Widgets as Widgets
import VLC.Style
import VLC.Util
import VLC.Player

Widgets.ExpandGridView {
    id: root
    property MLDragItem dragItem
    property MLContextMenu contextMenu
    property url fallbackArtwork: VLCStyle.noArtVideoCover
    signal focusInitializationRequested()
    signal itemDoubleClicked(var model)
    readonly property real uiScale: VLCStyle.dp(1, VLCStyle.scale)
    leftMargin: 28 * uiScale + leftPadding
    rightMargin: 28 * uiScale + rightPadding
    horizontalSpacing: 20 * uiScale
    verticalSpacing: 0
    topMargin: 28 * uiScale
    nbItemPerRow: Math.max(1, Math.min(6, Math.floor((_availableContentWidth + horizontalSpacing) / (400 * uiScale))))
    cellWidth: Math.max(1, Math.floor((_availableContentWidth - horizontalSpacing * (nbItemPerRow - 1)) / nbItemPerRow))
    cellHeight: 44 * uiScale
    activeFocusOnTab: true
    onActiveFocusChanged: {
        if (activeFocus && model && model.count > 0 && !selectionModel.hasSelection)
            focusInitializationRequested()
    }
    delegate: Widgets.JuxtopposedMediaRow {
        id: row
        // ExpandGridView's factory supplies this together with model/index/selected.
        property Item view
        property var model
        property int index: -1
        width: root.cellWidth
        height: root.cellHeight
        uiScale: root.uiScale
        title: model?.title ?? qsTr("Unknown title")
        artwork: model?.thumbnail ?? model?.small_cover ?? ""
        fallbackArtwork: root.fallbackArtwork
        durationText: model?.duration?.formatHMS() ?? ""
        progress: model?.progress ?? 0
        current: Player.isStarted && !!model?.mrl && model.mrl.toString() === Player.url.toString()
        onSelectionRequested: modifiers => root.leftClickOnItem(modifiers, index, true)
        onActivationRequested: keyboard => {
            if (keyboard)
                root.actionAtIndex(index)
            else
                root.itemDoubleClicked(model)
        }
        onDragRequested: (active, modifiers) => {
            if (!root.dragItem)
                return
            if (active) {
                if (!selected)
                    root.leftClickOnItem(modifiers, index, true)
                root.dragItem.Drag.active = true
            } else
                root.dragItem.Drag.drop()
        }
        onContextRequested: position => {
            root.rightClickOnItem(index)
            root.contextMenu?.popup(root.selectionModel.selectedIndexes, position)
        }
    }
}
