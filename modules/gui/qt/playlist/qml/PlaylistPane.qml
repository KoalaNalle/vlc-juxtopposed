/*****************************************************************************
 * Copyright (C) 2019 VLC authors and VideoLAN
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
import QtQuick.Window
import QtQuick.Templates as T
import QtQuick.Layouts
import QtQml.Models


import VLC.MainInterface
import VLC.Widgets as Widgets
import VLC.Util
import VLC.Playlist
import VLC.Style
import VLC.Dialogs
import VLC.Player

T.Pane {
    id: root

    property var model: PlaylistListModel {
        playlist: MainPlaylistController.playlist
    }
    readonly property ListSelectionModel selectionModel: listView?.selectionModel ?? null

    property bool useAcrylic: true
    readonly property bool juxtopposed: Qt.platform.os === "windows" && MainCtx.juxtopposedControls
    property int currentTab: 0
    onCurrentTabChanged: {
        if (currentTab !== 0)
            searchBox?.retract()
    }
    property int durationRevision: 0
    property int queueSelectionRevision: 0
    readonly property int queueSelectionCount: {
        queueSelectionRevision
        return root.selectionModel?.selectedIndexesFlat.length ?? 0
    }

    readonly property real minimumWidth: contentItem.Layout.minimumWidth +
                                         leftPadding +
                                         rightPadding +
                                         (juxtopposed ? 0 : 2 * VLCStyle.margin_xsmall)
    readonly property real preferredWidth: juxtopposed && MainCtx.playqueuePanel.width < minimumWidth
                                               ? VLCStyle.dp(400, VLCStyle.scale)
                                               : MainCtx.playqueuePanel.width

    readonly property ListView listView: contentItem.listView

    property alias contextMenu: contextMenu

    property alias dragItem: dragItem

    property alias isDropAcceptableFunc: listView.isDropAcceptableFunc
    property alias acceptDropFunc: listView.acceptDropFunc

    property alias view: listView

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    verticalPadding: juxtopposed ? 0 : VLCStyle.margin_normal

    Accessible.role: Accessible.Pane
    Accessible.name: qsTr("Play Queue")
    AccessibleCompat.id: "playqueuePane"

    readonly property ColorContext colorContext: ColorContext {
        id: theme
        colorSet: ColorContext.View

        focused: root.activeFocus
        hovered: root.hovered
        enabled: root.enabled
    }

    Widgets.DragItem {
        id: dragItem

        view: root.listView

        // PlaylistDelegate supplies source rows for dragged items. Texture
        // providers, unlike item data, are looked up in the filtered view.
        getTextureProvider: function(index) {
            const viewIndex = listView.mapSourceToView(index)
            return viewIndex < 0 ? null
                                 : listView.itemAtIndex.call(null, viewIndex)?.artworkTextureProvider ?? null
        }

        onRequestData: (indexes, resolve, reject) => {
            resolve(indexes.map((index) => {
                const item = root.model.itemAt(index)
                return {
                    "title": item.title,
                    "cover": (!!item.artwork && item.artwork.toString() !== "") ? item.artwork : VLCStyle.noArtAlbumCover,
                    "url": item.url
                }
            }))
        }

        onRequestInputItems: (indexes, data, resolve, reject) => {
            resolve(root.model.getItemsForIndexes((indexes?.length > 0) ? indexes : listView.mappedSelectedIndexes()))
        }
    }

    PlaylistContextMenu {
        id: contextMenu
        model: root.model
        proxyModel: (root.contentItem?.listView?.model instanceof AbstractProxyModel) ? root.contentItem.listView.model
                                                                                        : null
        selectionModel: root.selectionModel
        controler: MainPlaylistController
        ctx: MainCtx

        onJumpToCurrentPlaying: {
            const viewIndex = listView.mapSourceToView(MainPlaylistController.currentIndex)
            if (viewIndex >= 0)
                listView.positionViewAtIndex(viewIndex, ItemView.Center)
        }
    }

    background: Widgets.AcrylicBackground {
        enabled: root.useAcrylic
        tintColor: theme.bg.primary
    }

    Binding {
        target: root.background
        property: "visible"
        when: root.juxtopposed
        value: false
    }

    function queueDuration() : var {
        root.durationRevision // Refresh when duration roles change without a row-count change.
        const count = listView.model.rowCount()
        if (count <= 0)
            return null
        const durations = []
        for (let i = 0; i < count; ++i)
            durations.push(listView.model.data(listView.model.index(i, 0), PlaylistListModel.DurationRole))
        return durations[0].sum(durations)
    }

    function formatQueueDuration(duration) : string {
        if (!duration || !duration.valid())
            return ""
        const hours = duration.toHours()
        const minutes = duration.toMinutes()
        const seconds = duration.toSeconds()
        if (hours > 0)
            return qsTr("%1h %2m").arg(hours).arg(minutes % 60)
        if (minutes > 0)
            return qsTr("%1m %2s").arg(minutes).arg(seconds % 60)
        return qsTr("%1s").arg(seconds)
    }

    function formatChapterTime(time) : string {
        if (!time || !time.valid())
            return "—"
        const seconds = time.toSeconds()
        const hours = Math.floor(seconds / 3600).toString().padStart(2, "0")
        const minutes = Math.floor(seconds / 60 % 60).toString().padStart(2, "0")
        const remainder = (seconds % 60).toString().padStart(2, "0")
        return hours + ":" + minutes + ":" + remainder
    }

    function queueSummary() : string {
        const countText = listView.count === 1 ? qsTr("%1 item").arg(listView.count)
                                              : qsTr("%1 items").arg(listView.count)
        const durationText = root.formatQueueDuration(root.queueDuration())
        return durationText ? qsTr("%1 - %2").arg(countText).arg(durationText) : countText
    }

    function formatQueueRepeatMode(mode) : string {
        switch (mode) {
        case PlaylistController.PLAYBACK_REPEAT_ALL:
            return qsTr("Repeat all")
        case PlaylistController.PLAYBACK_REPEAT_CURRENT:
            return qsTr("Repeat current item")
        default:
            return qsTr("Repeat off")
        }
    }

    function showQueueSort() {
        const entries = sort.model.map((entry) => {
            const checked = entry.criteria === sort.sortKey
            return { "text": entry.text, "checked": checked,
                     "order": checked ? sort.sortOrder : undefined }
        })
        const button = queuePanel.sortButton
        sort._menu.popup(button.mapToGlobal(0, button.height), false, entries)
    }

    Binding {
        target: searchBox.popup
        property: "parent"
        when: root.juxtopposed
        value: queuePanel
    }
    Binding {
        target: searchBox.popup
        property: "x"
        when: root.juxtopposed
        value: Math.max(0, queuePanel.width - searchBox.popup.width - 20 * VLCStyle.scale)
    }
    Binding {
        target: searchBox.popup
        property: "y"
        when: root.juxtopposed
        value: 50 * VLCStyle.scale
    }
    Connections {
        target: searchBox.popup
        function onClosed() {
            if (!root.juxtopposed)
                return
            if (root.currentTab === 0)
                listView.forceActiveFocus(Qt.ShortcutFocusReason)
            else if (root.currentTab === 1)
                bookmarksLoader.item?.forceActiveFocus(Qt.ShortcutFocusReason)
            else
                chaptersView.forceActiveFocus(Qt.ShortcutFocusReason)
        }
    }

    contentItem: ColumnLayout {
        spacing: root.juxtopposed ? 0 : VLCStyle.margin_xxsmall

        Layout.minimumWidth: root.juxtopposed ? VLCStyle.dp(320, VLCStyle.scale)
                                             : Math.min(Math.max(noContentInfoColumn.implicitWidth,
                                               titleRowLayout.implicitWidth ),
                                      /* Minimum width should not be too large, we rely on elision otherwise: */
                                      Window.width / 3) + (headerColumn.Layout.leftMargin + headerColumn.Layout.rightMargin)

        readonly property ListView listView: listView

        JuxtopposedQueuePanel {
            id: queuePanel
            visible: root.juxtopposed
            Layout.fillWidth: true
            Layout.fillHeight: true
            uiScale: VLCStyle.scale
            activeTab: root.currentTab
            summary: root.currentTab === 1 ? ((bookmarksLoader.item?.count ?? 0) === 1
                                                 ? qsTr("%1 Bookmark").arg(bookmarksLoader.item.count)
                                                 : qsTr("%1 Bookmarks").arg(bookmarksLoader.item?.count ?? 0))
                                          : root.queueSummary()
            showSort: root.currentTab === 0
            showRepeat: root.currentTab === 0
            showShuffle: root.currentTab === 0
            showToolbar: root.currentTab !== 2
            showRemove: root.currentTab !== 2
            showAdd: root.currentTab !== 2
            toolbarContentHeight: root.currentTab === 0 ? 28 : 24
            sortEnabled: MainPlaylistController.count > 1
            repeatEnabled: !MainPlaylistController.empty
            shuffleEnabled: MainPlaylistController.count > 1
            removeEnabled: root.currentTab === 0 ? root.queueSelectionCount > 0
                          : root.currentTab === 1 && (bookmarksLoader.item?.selectionCount ?? 0) > 0
            addEnabled: root.currentTab === 0 || (root.currentTab === 1 && (bookmarksLoader.item?.canAdd ?? false))
            addDescription: root.currentTab === 1 ? qsTr("Add bookmark") : qsTr("Add files")
            removeDescription: root.currentTab === 1 ? qsTr("Remove selected bookmarks") : qsTr("Remove selected items")
            repeatActive: MainPlaylistController.repeatMode !== PlaylistController.PLAYBACK_REPEAT_NONE
            repeatDescription: root.formatQueueRepeatMode(MainPlaylistController.repeatMode)
            shuffleActive: MainPlaylistController.random
            onTabRequested: (index) => root.currentTab = index
            onSortRequested: root.showQueueSort()
            onRepeatRequested: MainPlaylistController.toggleRepeatMode()
            onShuffleRequested: MainPlaylistController.toggleRandom()
            onRemoveRequested: {
                if (root.currentTab === 1) {
                    bookmarksLoader.item?.removeSelected()
                    return
                }
                if (root.currentTab !== 0)
                    return
                const items = listView.mappedSelectedIndexes()
                if (items.length > 0)
                    root.model.removeItems(items)
            }
            onAddRequested: {
                if (root.currentTab === 0)
                    DialogsProvider.simpleOpenDialog(false)
                else if (root.currentTab === 1)
                    bookmarksLoader.item?.addBookmark()
            }
        }

        Loader {
            id: bookmarksLoader
            parent: queuePanel.bodyContainer
            anchors.fill: parent
            active: root.juxtopposed && MainCtx.mediaLibraryAvailable
            visible: root.currentTab === 1
            source: "JuxtopposedBookmarkList.qml"
            onLoaded: item.navigationParent = root
        }

        T.Label {
            parent: queuePanel.bodyContainer
            anchors.centerIn: parent
            visible: root.juxtopposed && root.currentTab === 1 && !MainCtx.mediaLibraryAvailable
            text: qsTr("Bookmarks require the media library")
            color: "#969696"
            font.pixelSize: 14 * VLCStyle.scale
        }

        Widgets.ListViewExt {
            id: chaptersView
            parent: queuePanel.bodyContainer
            anchors.fill: parent
            visible: root.juxtopposed && root.currentTab === 2
            enabled: visible
            model: Player.chapters
            selectionModel: ListSelectionModel { model: chaptersView.model }
            spacing: 12 * VLCStyle.scale
            clip: true
            focus: true
            Navigation.parentItem: root
            Navigation.upItem: queuePanel.chaptersButton
            Accessible.role: Accessible.List
            Accessible.name: qsTr("Chapters")

            function activate(index) {
                if (index >= 0 && index < count)
                    Player.chapters.selectChapter(index)
            }
            onActionAtIndex: (index) => activate(index)

            delegate: T.Control {
                id: chapterRow
                required property int index
                required property string display
                required property var time
                required property bool checked
                readonly property bool selected: chaptersView.selectionModel.selectedIndexesFlat.includes(index)
                width: chaptersView.width
                height: 32 * VLCStyle.scale
                padding: 0
                hoverEnabled: true
                Accessible.role: Accessible.ListItem
                Accessible.name: display
                Accessible.selected: selected
                Accessible.description: checked ? qsTr("Current chapter, %1").arg(contentItem.durationText)
                                                : contentItem.durationText
                Accessible.onPressAction: chaptersView.activate(index)

                readonly property ColorContext colorContext: ColorContext {
                    id: chapterTheme
                    colorSet: ColorContext.Item
                    focused: chapterRow.activeFocus
                    hovered: chapterRow.hovered
                    enabled: chapterRow.enabled
                }
                background: Widgets.AnimatedBackground {
                    color: chapterRow.selected || chapterRow.checked ? chapterTheme.bg.highlight
                           : chapterRow.hovered ? chapterTheme.bg.primary : "transparent"
                    border.color: chapterRow.visualFocus ? chapterTheme.visualFocus : "transparent"
                    enabled: chapterTheme.initialized
                }
                contentItem: JuxtopposedQueueRow {
                    uiScale: VLCStyle.scale
                    showMove: false
                    chapterNumber: chapterRow.index + 1
                    title: chapterRow.display
                    durationText: root.formatChapterTime(chapterRow.time)
                    titleColor: chapterRow.checked || chapterRow.selected ? chapterTheme.fg.primary : "#969696"
                }
                TapHandler {
                    acceptedDevices: PointerDevice.AllDevices & ~PointerDevice.TouchScreen
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    gesturePolicy: TapHandler.ReleaseWithinBounds
                    onSingleTapped: {
                        chapterRow.forceActiveFocus(Qt.MouseFocusReason)
                        chaptersView.selectionModel.updateSelection(point.modifiers, chaptersView.currentIndex, chapterRow.index)
                        chaptersView.currentIndex = chapterRow.index
                    }
                    onDoubleTapped: (eventPoint, button) => {
                        if (button === Qt.LeftButton)
                            chaptersView.activate(chapterRow.index)
                    }
                }
                DelegateTouchTapHandler {
                    delegate: chapterRow
                    onSingleTapped: chaptersView.activate(chapterRow.index)
                }
            }

            T.Label {
                anchors.centerIn: parent
                visible: chaptersView.count === 0
                text: qsTr("No chapters in this media")
                color: "#969696"
                font.pixelSize: 14 * VLCStyle.scale
            }
        }

        Column {
            id: headerColumn
            visible: !root.juxtopposed

            Layout.fillHeight: false
            Layout.fillWidth: true
            Layout.leftMargin: VLCStyle.margin_normal
            Layout.rightMargin: VLCStyle.margin_normal

            spacing: VLCStyle.margin_xxxsmall

            RowLayout {
                id: titleRowLayout

                anchors.left: parent.left
                anchors.right: parent.right

                spacing: VLCStyle.margin_normal

                Widgets.SubtitleLabel {
                    id: titleLabel

                    Layout.fillWidth: true

                    text: qsTr("Play Queue")
                    color: theme.fg.primary
                    font.weight: Font.Bold
                    font.pixelSize: VLCStyle.dp(24, VLCStyle.scale)
                }

                Widgets.SortControl {
                    id: sort

                    font.pixelSize: VLCStyle.icon_banner

                    description : qsTr("Sort")

                    AccessibleCompat.id: "playqueueSort"

                    enabled: MainPlaylistController.count > 1

                    checked: MainPlaylistController.sortKey !== PlaylistController.SORT_KEY_NONE

                    popupAbove: false

                    model: MainPlaylistController.sortKeyTitleList

                    onSortSelected: key => {
                        MainPlaylistController.sortKey = key
                    }

                    onSortOrderSelected: type => {
                        if (type === Qt.AscendingOrder)
                            MainPlaylistController.sortOrder = PlaylistController.SORT_ORDER_ASC
                        else if (type === Qt.DescendingOrder)
                            MainPlaylistController.sortOrder = PlaylistController.SORT_ORDER_DESC

                        MainPlaylistController.sort()
                    }

                    sortOrder: {
                        if (MainPlaylistController.sortOrder === PlaylistController.SORT_ORDER_ASC) {
                            Qt.AscendingOrder
                        }
                        else if (MainPlaylistController.sortOrder === PlaylistController.SORT_ORDER_DESC) {
                            Qt.DescendingOrder
                        }
                    }

                    sortKey: MainPlaylistController.sortKey

                    Navigation.parentItem: root
                    Navigation.rightItem: searchBox
                    Navigation.downItem: listView
                }

                Widgets.SearchBox {
                    id: searchBox

                    enabled: MainPlaylistController.count > 1

                    toggleButton.font.pixelSize: VLCStyle.icon_banner

                    popBelow: true

                    displayRegexToggleButton: true
                    displayCaseSensitiveToggleButton: true

                    popup.width: root.juxtopposed ? Math.min(Math.max(popup.implicitWidth, root.contentItem.width * 0.6),
                                                           queuePanel.width - 40 * VLCStyle.scale)
                                                 : Math.max(popup.implicitWidth, root.contentItem?.width * 0.6)
                    popup.topPadding: VLCStyle.margin_xxsmall

                    Navigation.parentItem: root
                    Navigation.leftItem: sort
                    Navigation.downItem: listView

                    Connections {
                        target: contextMenu

                        function onDiscardedFilteredOutItems() {
                            searchBox.retract()
                        }
                    }
                }
            }

            Widgets.CaptionLabel {
                color: theme.fg.secondary
                visible: listView.count !== 0
                text: {
                    if (listView.count <= 0)
                        return "" // Not visible otherwise

                    const duration = root.queueDuration()
                    return qsTr("%1 elements, %2").arg(listView.count).arg(duration.formatLong())
                }
            }
        }

        Item {
            // Spacer
            visible: !root.juxtopposed

            implicitHeight: VLCStyle.margin_xsmall
        }

        RowLayout {
            visible: !root.juxtopposed && listView.count !== 0

            Layout.fillHeight: false
            Layout.leftMargin: VLCStyle.margin_normal
            Layout.rightMargin: Math.max(listView.T.ScrollBar.vertical.width, VLCStyle.margin_normal)

            spacing: VLCStyle.margin_large

            Widgets.IconLabel {
                // play queue cover column
                Layout.preferredWidth: VLCStyle.icon_playlistArt

                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text: VLCIcons.album_cover
                font.pixelSize: VLCStyle.icon_playlistHeader

                color: theme.fg.secondary

                Accessible.role: Accessible.ColumnHeader
                Accessible.name: qsTr("Cover")
                Accessible.ignored: false
            }

            //Use Text here as we're redefining its Accessible.role
            Text {
                Layout.fillWidth: true

                elide: Text.ElideRight
                font.pixelSize: VLCStyle.fontSize_normal
                textFormat: Text.PlainText

                verticalAlignment: Text.AlignVCenter
                text: qsTr("Title")
                color: theme.fg.secondary

                Accessible.role: Accessible.ColumnHeader
                Accessible.name: text
            }

            Widgets.IconLabel {
                Layout.preferredWidth: durationMetric.width

                text: VLCIcons.time
                color: theme.fg.secondary
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: VLCStyle.icon_playlistHeader

                Accessible.role: Accessible.ColumnHeader
                Accessible.name: qsTr("Duration")
                Accessible.ignored: false

                TextMetrics {
                    id: durationMetric

                    font.weight: Font.DemiBold
                    font.pixelSize: VLCStyle.fontSize_normal
                    text: "00:00"
                }
            }
        }

        Item {
            id: legacyQueueHost
            visible: !root.juxtopposed
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        Widgets.ListViewExt {
            id: listView
            parent: root.juxtopposed ? queuePanel.bodyContainer : legacyQueueHost
            anchors.fill: parent
            visible: !root.juxtopposed || root.currentTab === 0
            spacing: root.juxtopposed ? 12 * VLCStyle.scale : 0
            onModelChanged: root.queueSelectionRevision++

            selectionModel: ListSelectionModel {
                model: listView.model
            }

            focus: true

            clip: !fadingEdge.implicitClipping && (height < contentHeight)

            model: proxyModel ?? root.model

            onWidthChanged: {
                if (searchBox.expanded) {
                    if (width < searchBox.textField.width)
                        searchBox.retract()
                }
            }

            property QtSortFilterProxyModel proxyModel

            function adjustFiltering() {
                // Once text changes, we switch to the proxy model.
                // We do not switch back to the source model after that.
                if (!proxyModel) {
                    proxyModel = proxyModelComponent.createObject(listView)
                }

                if (searchBox.regexButtonToggled)
                    proxyModel.setFilterRegularExpression(searchBox.searchPattern)
                else
                    proxyModel.setFilterFixedString(searchBox.searchPattern)
            }

            Component {
                id: proxyModelComponent

                QtSortFilterProxyModel {
                    filterRole: PlaylistListModel.FilterRole

                    sourceModel: root.model

                    filterCaseSensitivity: searchBox.caseSensitiveButtonToggled ? Qt.CaseSensitive
                                                                                : Qt.CaseInsensitive
                }
            }

            fadingEdge.backgroundColor: root.juxtopposed ? Qt.rgba(14 / 255, 14 / 255, 14 / 255, 0.95)
                                      : (root.background && (root.background.color.a >= 1.0)) ? root.background.color
                                                                                              : Qt.alpha(root.background.color, 0.0)

            function mapViewToSource(index) : int {
                if (index < 0 || index >= listView.count)
                    return -1

                const sourceIndex = (listView.model instanceof AbstractProxyModel)
                        ? listView.model.mapToSource(listView.model.index(index, 0)).row
                        : index
                return sourceIndex >= 0 && sourceIndex < root.model.count ? sourceIndex : -1
            }

            function mapSourceToView(index) : int {
                if (index < 0 || index >= root.model.count)
                    return -1

                const viewIndex = (listView.model instanceof AbstractProxyModel)
                        ? listView.model.mapFromSource(root.model.index(index, 0)).row
                        : index
                return viewIndex >= 0 && viewIndex < listView.count ? viewIndex : -1
            }

            function mappedSelectedIndexes() : var {
                if (!listView.selectionModel)
                    return [] // unlikely

                const selectedIndexes = listView.selectionModel.selectedIndexesFlat
                const sourceIndexes = []
                for (let i = 0; i < selectedIndexes.length; ++i) {
                    const sourceIndex = listView.mapViewToSource(selectedIndexes[i])
                    if (sourceIndex >= 0)
                        sourceIndexes.push(sourceIndex)
                }

                // The playlist move/remove APIs require ascending source rows,
                // whereas selection order follows the user's clicks.
                return sourceIndexes.sort((a, b) => a - b)
            }

            // Row drop zones already map their targets to source coordinates.
            // Keep those distinct from the footer and queue-toggle drops, which
            // append to the complete queue even when its tail is filtered out.
            function isSourceDropAcceptable(drop, index) : bool {
                if (index < 0 || index > root.model.count)
                    return false

                if (drop.source === dragItem) {
                    const sourceIndexes = listView.mappedSelectedIndexes()
                    return sourceIndexes.length > 0 && Helpers.itemsMovable(sourceIndexes, index)
                } else if (Helpers.isValidInstanceOf(drop.source, Widgets.DragItem))
                    return true
                else if (drop.hasUrls)
                    return true
                else
                    return false
            }

            function acceptSourceDrop(index, drop) : var {
                if (!listView.isSourceDropAcceptable(drop, index))
                    return Promise.resolve()

                const item = drop.source;

                // NOTE: Move implementation.
                if (dragItem === item) {
                    root.model.moveItemsPre(listView.mappedSelectedIndexes(), index);
                    listView.forceActiveFocus();
                // NOTE: Dropping medialibrary content into the queue.
                } else if (Helpers.isValidInstanceOf(item, Widgets.DragItem)) {
                    return item.getSelectedInputItem().then((inputItems) => {
                            if (!Helpers.isArray(inputItems) || inputItems.length === 0) {
                                console.warn("can't convert items to input items");
                                return
                            }
                            MainPlaylistController.insert(index, inputItems, false)
                        }).then(() => { listView.forceActiveFocus(); })
                // NOTE: Dropping an external item (i.e. filesystem) into the queue.
                } else if (drop.hasUrls) {
                    const urlList = [];

                    for (let url in drop.urls)
                        urlList.push(drop.urls[url]);

                    MainPlaylistController.insert(index, urlList, false);

                    // NOTE This is required otherwise backend may handle the drop as well yielding double addition.
                    drop.accept(Qt.IgnoreAction);
                    listView.forceActiveFocus();
                }

                return Promise.resolve()
            }

            isDropAcceptableFunc: function(drop, index) {
                return listView.isSourceDropAcceptable(drop, root.model.count)
            }

            acceptDropFunc: function(index, drop) {
                return listView.acceptSourceDrop(root.model.count, drop)
            }

            property int shiftIndex: -1

            onShowContextMenu: (globalPos) => {
                contextMenu.popup(-1, globalPos)
            }

            Behavior on contentY {
                id: contentYBehavior

                enabled: false

                // NOTE: Usage of `SmoothedAnimation` is intentional here.
                SmoothedAnimation {
                    duration: VLCStyle.duration_veryLong
                    easing.type: Easing.InOutSine
                }
            }

            Component.onCompleted: {
                // WARNING: Tracking the current item and not the current index is intentional here.
                MainPlaylistController.currentItemChanged.connect(listView, () => {
                    const viewIndex = listView.mapSourceToView(MainPlaylistController.currentIndex)
                    if (viewIndex < 0)
                        return // The playing item can be filtered out.

                    // FIXME: Qt does not provide the `contentY` with `positionViewAtIndex()` for us
                    //        to animate. For that reason, we capture the new `contentY`, adjust
                    //        `contentY` to it is old value then enable the animation and set `contentY`
                    //        to its new value.
                    const oldContentY = listView.contentY
                    listView.positionViewAtIndex(viewIndex, ListView.Contain)
                    const newContentY = listView.contentY
                    if (Math.abs(oldContentY - newContentY) >= Number.EPSILON) {
                        contentYBehavior.enabled = false
                        listView.contentY = oldContentY
                        contentYBehavior.enabled = true
                        listView.contentY = newContentY
                        contentYBehavior.enabled = false
                    }
                })

                searchBox.searchPatternChanged.connect(listView, listView.adjustFiltering)
                searchBox.regexButtonToggledChanged.connect(listView, listView.adjustFiltering)
            }

            Connections {
                target: listView.model

                function onRowsInserted() {
                    root.durationRevision++
                    if (listView.currentIndex === -1)
                        listView.currentIndex = 0
                }

                function onModelReset() {
                    root.durationRevision++
                    root.queueSelectionRevision++
                    if (listView.currentIndex === -1 && listView.count > 0)
                        listView.currentIndex = 0
                }
                function onDataChanged(topLeft, bottomRight, roles) {
                    if ((roles?.length ?? 0) === 0 || roles.includes(PlaylistListModel.DurationRole))
                        root.durationRevision++
                }
                function onRowsRemoved() {
                    root.durationRevision++
                    root.queueSelectionRevision++
                }
                function onRowsMoved() { root.durationRevision++ }
                function onLayoutChanged() { root.durationRevision++ }
            }

            delegate: PlaylistDelegate {
                id: delegate

                width: listView.contentWidth
                juxtopposed: root.juxtopposed
                rightPadding: juxtopposed ? 0 : Math.max(listView.T.ScrollBar.vertical.width, VLCStyle.margin_normal)

                contextMenu: root.contextMenu

                dragItem: root.dragItem

                isDropAcceptable: listView.isSourceDropAcceptable
                acceptDrop: listView.acceptSourceDrop

                onContainsDragChanged: listView.updateItemContainsDrag(this, containsDrag)

                onIsCurrentChanged: {
                    if (isCurrent)
                        listView.fadingEdge.excludeItem = delegate
                    else if (listView.fadingEdge.excludeItem === delegate)
                        listView.fadingEdge.excludeItem = null
                }
            }

            Keys.onDeletePressed: {
                const items = listView.mappedSelectedIndexes()

                if (items.length > 0)
                    root.model.removeItems(items)
            }

            Navigation.parentItem: root
            Navigation.upItem: root.juxtopposed ? queuePanel.sortButton : sort

            onActionAtIndex: (index) => {
                const sourceIndex = listView.mapViewToSource(index)
                if (sourceIndex < 0)
                    return

                MainPlaylistController.goTo(sourceIndex, true)
            }

            Column {
                id: noContentInfoColumn

                anchors.centerIn: parent

                visible: false
                enabled: visible

                opacity: (listView.activeFocus) ? 1.0 : 0.4

                Binding on visible {
                    delayed: true
                    value: (listView.count === 0 && !listView.footerItem.firstItemIndicatorVisible)
                }

                Widgets.IconLabel {
                    id: label

                    anchors.horizontalCenter: parent.horizontalCenter

                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter

                    text: VLCIcons.playlist

                    color: theme.fg.primary

                    font.pixelSize: VLCStyle.dp(48, VLCStyle.scale)
                }

                T.Label {
                    anchors.topMargin: VLCStyle.margin_xlarge

                    anchors.horizontalCenter: parent.horizontalCenter

                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter

                    text: (listView.proxyModel &&
                           listView.model === listView.proxyModel &&
                           listView.count !== root.model.count) ? qsTr("No results")
                                                                : qsTr("No content yet")

                    color: label.color

                    font.pixelSize: VLCStyle.fontSize_xxlarge
                }

                T.Label {
                    anchors.topMargin: VLCStyle.margin_normal

                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter

                    text: qsTr("Drag & Drop some content here!")
                    visible: (listView.proxyModel && listView.model === listView.proxyModel) ? (listView.count === root.model.count)
                                                                                             : true

                    color: label.color

                    font.pixelSize: VLCStyle.fontSize_large
                }
            }
        }
    }

    Keys.priority: Keys.AfterItem
    Keys.forwardTo: !root.juxtopposed || root.currentTab === 0 ? [listView]
                  : root.currentTab === 1 && bookmarksLoader.item ? [bookmarksLoader.item]
                  : root.currentTab === 2 ? [chaptersView] : []
    Keys.onPressed: (event) => {
        if (root.juxtopposed && root.currentTab === 0 && event.matches(StandardKey.Find)) {
            searchBox.expandAndFocus()
            event.accepted = true
        } else {
            root.Navigation.defaultKeyAction(event)
        }
    }

    component PlaylistToggleButton : Widgets.IconToolButton {
        id: playlistBtn

        focus: true

        font.pixelSize: VLCStyle.icon_banner
        text: VLCIcons.playlist
        description: qsTr("Play queue")

        AccessibleCompat.id: "playqueueToggle"

        width: VLCStyle.bannerButton_width
        height: VLCStyle.bannerButton_height
        //highlighted: MainCtx.playlistVisible

        property Item plListView

        DropArea {
            anchors.fill: parent

            onContainsDragChanged: {
                if (containsDrag) {
                    _timer.restart()

                    if (playlistBtn.plListView)
                        MainCtx.setCursor(Qt.DragCopyCursor)
                } else {
                    _timer.stop()

                    if (playlistBtn.plListView)
                        MainCtx.restoreCursor()
                }
            }

            onEntered: (drag) => {
                const plListView = playlistBtn.plListView
                if (plListView) {
                    console.assert(plListView.isDropAcceptableFunc)
                    console.assert(plListView.model)
                    if (plListView.isDropAcceptableFunc(drag, plListView.model.count)) {
                        drag.accept()
                    } else {
                        drag.accepted = false
                    }
                } else {
                    drag.accepted = false
                }
            }

            onDropped: (drop) => {
                const plListView = playlistBtn.plListView
                if (plListView) {
                    console.assert(plListView.acceptDropFunc)
                    plListView.acceptDropFunc(plListView.model.count, drop)
                }
            }

            Timer {
                id: _timer
                interval: VLCStyle.duration_humanMoment

                onTriggered: {
                    MainCtx.playqueuePanel.visible = true
                }
            }
        }
    }
}
