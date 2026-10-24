/*****************************************************************************
 * Copyright (C) 2020 VLC authors and VideoLAN
 *
 * Authors: Benjamin Arnaud <bunjee@omega.gg>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * ( at your option ) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

import QtQuick
import QtQml.Models


import VLC.Style
import VLC.MainInterface
import VLC.Widgets as Widgets
import VLC.Util
import VLC.Network

FocusScope {
    id: root

    // Properties

    required property BaseModel model

    readonly property int rowHeight: _currentView?.rowHeight ?? 0

    readonly property int contentHeight: _currentView?.contentHeight ?? 0

    readonly property int contentLeftMargin: _currentView?.contentLeftMargin ?? 0
    readonly property int contentRightMargin: _currentView?.contentRightMargin ?? 0

    property int displayMarginBeginning: 0
    property int displayMarginEnd: 0

    readonly property int currentIndex: _currentView?.currentIndex ?? -1

    property int maximumRows: -1

    readonly property int maximumCount: _currentView?.maximumCount ?? -1

    readonly property int nbItemPerRow: _currentView?.nbItemPerRow ?? 1

    property bool isSearchable: true

    readonly property bool hasGridListMode: true

    // NOTE: Currently only respected by the table view:
    property bool enableBeginningFade: true
    property bool enableEndFade: true

    property bool interactive: true

    property bool reuseItems: true

    // Aliases

    property alias leftPadding: loader.anchors.leftMargin
    property alias rightPadding: loader.anchors.rightMargin

    // Saved row restoration uses this view's existing selection and cursor.
    property int initialIndex: -1
    property bool _pendingInitialIndexReset: false
    property int _rememberedIndex: -1
    property Item _trackedView: null

    onInitialIndexChanged: {
        _pendingInitialIndexReset = initialIndex !== -1
        _restoreInitialIndex()
    }
    onModelChanged: {
        _rememberedIndex = -1
        _pendingInitialIndexReset = initialIndex !== -1
        _restoreInitialIndex()
    }
    onCurrentIndexChanged: {
        if (!_pendingInitialIndexReset && _currentView === _trackedView && currentIndex >= 0)
            _rememberedIndex = currentIndex
    }
    Connections {
        target: root.model
        function onCountChanged() { if (root._pendingInitialIndexReset) root._restoreInitialIndex() }
        function onLoadingChanged() { if (root._pendingInitialIndexReset) root._restoreInitialIndex() }
    }
    Connections {
        target: modelSelect
        function onModelChanged() { if (root._pendingInitialIndexReset) root._restoreInitialIndex() }
    }
    Connections {
        target: root._currentView
        function onModelChanged() { if (root._pendingInitialIndexReset) root._restoreInitialIndex() }
    }

    function _setViewIndex(index) {
        _currentView.setCurrentItem(index)
        if (typeof _currentView.positionViewAtIndex === "function")
            _currentView.positionViewAtIndex(index, ItemView.Contain)
    }
    function _restoreInitialIndex() {
        if (!_pendingInitialIndexReset || !model || modelSelect.model !== model
                || model.loading || model.count === 0 || !_currentView || _currentView.model !== model)
            return
        const index = initialIndex >= 0 && initialIndex < model.count ? initialIndex : 0
        modelSelect.select(model.index(index, 0), ItemSelectionModel.ClearAndSelect)
        _setViewIndex(index)
        _rememberedIndex = index
        _pendingInitialIndexReset = false
    }

    // Private

    property alias _currentView: loader.item

    // Signals

    signal browse(var tree, int reason)

    signal seeAll(int reason)

    // Events

    onFocusChanged: {
        if (activeFocus === false || model.count === 0 || model.currentIndex !== -1)
            return

        model.currentIndex = 0
    }

    // Connections

    // Functions

    function playAt(index) {
        model.addAndPlay(index)
    }

    function setCurrentItemFocus(reason) {
        _currentView.setCurrentItemFocus(reason)
    }

    function getItemY(index) {
        if (_currentView === null)
            return 0

        return _currentView.getItemY(index)
    }

    // Events

    function onAction(index) {
        const indexes = modelSelect.selectedIndexes

        if (indexes.length > 1) {
            model.addAndPlay(indexes)

            return
        }

        const data = model.getDataAt(index)

        const type = data.type

        if (type === NetworkMediaModel.TYPE_DIRECTORY
                || type === NetworkMediaModel.TYPE_NODE
                || type === NetworkMediaModel.TYPE_PLAYLIST)
            browse(data.tree, Qt.TabFocusReason)
        else
            playAt(index);
    }

    function onClicked(model, index, modifier, select) {
        if (select)
            modelSelect.updateSelection(modifier, model.currentIndex, index)

        _currentView.currentIndex = index

        _currentView.setCurrentItemFocus(Qt.MouseFocusReason)
    }

    function onDoubleClicked(model, index) {
        const type = model.type

        if (type === NetworkMediaModel.TYPE_NODE
                || type === NetworkMediaModel.TYPE_DIRECTORY
                || type === NetworkMediaModel.TYPE_PLAYLIST)
            browse(model.tree, Qt.MouseFocusReason)
        else
            playAt(index);
    }

    // Children

    ListSelectionModel {
        id: modelSelect

        model: root.model
    }

    Loader {
        id: loader

        anchors.fill: parent

        focus: (model.count !== 0)

        sourceComponent: (MainCtx.gridView) ? grid : list

        onLoaded: {
            const previousIndex = root._rememberedIndex
            root._trackedView = item
            if (root._pendingInitialIndexReset)
                root._restoreInitialIndex()
            else if (previousIndex >= 0 && previousIndex < root.model.count)
                root._setViewIndex(previousIndex)
            else if (root.currentIndex >= 0)
                root._rememberedIndex = root.currentIndex
        }
    }

    Component {
        id: grid

        Widgets.ExpandGridItemView {
            id: gridView

            basePictureWidth: VLCStyle.gridCover_network_width
            basePictureHeight: VLCStyle.gridCover_network_height

            maxNbItemPerRow: 12

            readonly property int maximumCount: (root.maximumRows === -1)
                                                ? -1
                                                : root.maximumRows * nbItemPerRow

            displayMarginBeginning: root.displayMarginBeginning
            displayMarginEnd: root.displayMarginEnd

            model: root.model

            selectionModel: modelSelect

            interactive: root.interactive

            reuseItems: root.reuseItems

            Navigation.parentItem: root

            onActionAtIndex: (index) => { root.onAction(index) }

            delegate: NetworkGridItem {
                width: gridView.cellWidth;
                height: gridView.cellHeight;

                pictureWidth: gridView.maxPictureWidth
                pictureHeight: gridView.maxPictureHeight

                onItemClicked: (modifier, select) => { root.onClicked(model, index, modifier, select) }

                onItemDoubleClicked: root.onDoubleClicked(model, index)

                onPlayClicked: root.playAt(index)
            }
        }
    }

    Component {
        id: list

        Widgets.TableViewExt {
            id: listView

            // Properties

            readonly property int maximumCount: root.maximumRows

            readonly property int nbItemPerRow: 1

            property var _modelSmall: [{
                weight: 1,

                model: ({
                    criteria: "name",

                    title: "name",

                    subCriterias: [ "mrl" ],

                    text: qsTr("Name"),

                    headerDelegate: tableColumns.titleHeaderDelegate,
                    colDelegate: artworkColumn
                })
            }]

            property var _modelMedium: [{
                size: 1,

                model: {
                    criteria: "artwork",

                    text: qsTr("Cover"),

                    isSortable: false,

                    headerDelegate: tableColumns.titleHeaderDelegate,
                    colDelegate: artworkColumn
                }
            }, {
                weight: 1,

                model: {
                    criteria: "name",

                    text: qsTr("Name")
                }
            }, {
                weight: 1,

                model: {
                    criteria: "mrl",

                    text: qsTr("Url"),

                    colDelegate: mrlColumn
                }
            }]

            // Settings

            rowContextMenu: null

            rowHeight: VLCStyle.tableCoverRow_height

            displayMarginBeginning: root.displayMarginBeginning
            displayMarginEnd: root.displayMarginEnd

            fadingEdge.enableBeginningFade: root.enableBeginningFade
            fadingEdge.enableEndFade: root.enableEndFade

            model: root.model

            sortModel: (_availableRowWidth < VLCStyle.colWidth(4)) ? _modelSmall
                                                                   : _modelMedium

            preferredHeader: root.header

            selectionModel: modelSelect

            interactive: root.interactive

            reuseItems: root.reuseItems

            Navigation.parentItem: root

            onActionForSelection: selection => root.onAction(selection[0].row)

            onItemDoubleClicked: (index, model) => root.onDoubleClicked(model, index)

            Widgets.TableColumns {
                id: tableColumns

                titleCover_width: VLCStyle.listAlbumCover_width
                titleCover_height: VLCStyle.listAlbumCover_height

                showTitleText: listView.sortModel === listView._modelSmall
            }

            Component {
                id: artworkColumn

                NetworkThumbnailItem { onPlayClicked: index => root.playAt(index) }
            }

            Component {
                id: mrlColumn

                Widgets.TableRowDelegate {
                    id: itemText

                    Widgets.TextAutoScroller {

                        anchors.fill: parent

                        clip: scrolling

                        label: itemLabel

                        forceScroll: itemText.currentlyFocused

                        Widgets.ListLabel {
                            id: itemLabel

                            anchors.verticalCenter: parent.verticalCenter

                            text: {
                                if (itemText.rowModel === null)
                                    return ""

                                const text = itemText.rowModel[itemText.colModel.criteria]

                                if (text.toString() === "vlc://nop")
                                    return ""
                                else
                                    return text
                            }

                            color: itemText.selected
                                ? itemText.colorContext.fg.highlight
                                : itemText.colorContext.fg.primary
                        }
                    }
                }
            }
        }
    }
}
