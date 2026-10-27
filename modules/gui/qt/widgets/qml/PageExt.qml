/*****************************************************************************
 * Copyright (C) 2025 VLC authors and VideoLAN
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
import QtQuick.Layouts
import QtQuick.Templates as T

import VLC.MainInterface
import VLC.Style
import VLC.Menus
import VLC.Util

T.Page {
    id: root

    //path of the current page loader
    property var pagePrefix: []

    //note that T.Page introduce an intermediate Item, so to access our delegate,
    //we need to use contentChildren[0] rather than contentItem
    readonly property Item _firstChild: contentChildren[0] ?? null

    //indicates whether the subview support grid/list mode
    property bool hasGridListMode: _firstChild?.hasGridListMode ?? false

    property bool isSearchable: _firstChild?.isSearchable ?? false

    // Pages with a wrapper around their view can expose the actual model here.
    property var libraryModel: _firstChild?.model ?? null
    property int libraryItemCount: libraryModel?.count ?? _firstChild?.count ?? -1
    property var libraryBreadcrumbs: null
    property bool libraryHeaderEnabled: true

    property var sortModel: _firstChild?.sortModel ?? null

    //property is *not* readOnly, a PageLoader may define a localMenuDelegate common for its subviews (music, video)
    property SortMenu sortMenu: null

    property int displayMarginBeginning: 0
    property int displayMarginEnd: 0

    property bool enableBeginningFade: true
    property bool enableEndFade: true

    readonly property SearchCtx search: _searchCtx
    readonly property SortCtx sort: _sortCtx

    // Leaf properties are restored by PageLoader before the view is completed,
    // so filtering precedes restoration of its row/selection index.
    property alias historySearchPattern: _searchCtx.pattern
    property var _searchHistoryEntry: null
    property bool _searchHistoryReady: false

    Component.onCompleted: {
        _searchHistoryEntry = History.viewProp
        _searchHistoryReady = true
    }

    onHistorySearchPatternChanged: {
        // An outgoing page can still emit changes while the next entry loads,
        // including another entry with the same route. Never update that entry.
        if (_searchHistoryReady && _searchHistoryEntry === History.viewProp
                && History.exactMatch(History.viewPath, root.pagePrefix))
            _searchHistoryEntry.historySearchPattern = historySearchPattern
    }

    // Some views use an explicit native fallback while the saved key is empty.
    // Reflect that model's criterion rather than guessing a default for all views.
    property string libraryDefaultSortCriteria: _firstChild?.libraryDefaultSortCriteria ?? ""
    readonly property string librarySortCriteria: sort.criteria || libraryModel?.sortCriteria || libraryDefaultSortCriteria


    SearchCtx {
        id: _searchCtx
        available: root.isSearchable
    }

    SortCtx {
        id: _sortCtx

        readonly property string _sortCriteriaKey: ["sortCriteria", ...root.pagePrefix].join("/")
        readonly property string _sortOrderKey:  ["sortOrder", ...root.pagePrefix].join("/")

        available: Helpers.isArray(root.sortModel) && (root.sortModel).length > 0
        model: root.sortModel

        //save & restore setting of the page
        Component.onCompleted: {
            const criteria = MainCtx.settingValue(_sortCriteriaKey, undefined)
            if (criteria !== undefined)
                _sortCtx.criteria = criteria

            const order = MainCtx.settingValue(_sortOrderKey, undefined)
            if (order !== undefined)
                _sortCtx.order = parseInt(order)
        }

        Component.onDestruction: {
            MainCtx.setSettingValue(_sortCriteriaKey, _sortCtx.criteria)
            MainCtx.setSettingValue(_sortOrderKey, _sortCtx.order)
        }
    }

    Binding {
        target: MainCtx
        property: "hasGridListMode";
        value: root.hasGridListMode
    }

    on_FirstChildChanged: {
        if (!_firstChild)
            return
        _firstChild.Navigation.parentItem = root
        _firstChild.Navigation.upItem = root.header
    }

    function positionContentAtBeginning() {
        _firstChild?.positionContentAtBeginning?.()
    }

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding
                             + (implicitHeaderHeight > 0 ? implicitHeaderHeight + spacing : 0)
                             + (implicitFooterHeight > 0 ? implicitFooterHeight + spacing : 0))

    function focusContent() {
        if (typeof root.setCurrentItemFocus === "function")
            root.setCurrentItemFocus(Qt.TabFocusReason)
        else if (typeof root._firstChild?.setCurrentItemFocus === "function")
            root._firstChild.setCurrentItemFocus(Qt.TabFocusReason)
        else
            root._firstChild?.forceActiveFocus(Qt.TabFocusReason)
    }

    header: Loader {
        active: root.libraryHeaderEnabled
        enabled: active
        visible: active
        height: item?.implicitHeight ?? 0
        sourceComponent: MainCtx.juxtopposedControls ? juxtopposedHeader : traditionalHeader
        Navigation.parentItem: root
        Navigation.downAction: root.focusContent
        onActiveFocusChanged: {
            if (activeFocus && MainCtx.juxtopposedControls)
                item?.setCurrentItemFocus(Qt.TabFocusReason)
        }
    }

    Component {
        id: juxtopposedHeader
        JuxtopposedLibraryHeader {
            id: libraryHeader
            uiScale: VLCStyle.dp(1, VLCStyle.scale)
            iconFontFamily: VLCIcons.fontFamily
            backIcon: VLCIcons.back
            viewIcon: MainCtx.gridView ? VLCIcons.list : VLCIcons.grid
            searchIcon: VLCIcons.search
            moreIcon: VLCIcons.more
            canGoBack: !History.previousEmpty
            canGoForward: !History.nextEmpty
            sortAvailable: root.sort.available
            searchAvailable: root.search.available
            viewAvailable: root.hasGridListMode
            searchPattern: root.search.pattern
            itemCount: Math.max(0, root.libraryItemCount)
            countAvailable: root.libraryItemCount >= 0
            Navigation.parentItem: root.header
            Keys.onPressed: event => Navigation.defaultKeyAction(event)
            breadcrumbs: {
                if (root.libraryBreadcrumbs)
                    return root.libraryBreadcrumbs
                const section = root.pagePrefix[0]
                const labels = {video: qsTr("Videos"), music: qsTr("Music"), playlists: qsTr("Playlists"),
                                network: qsTr("Network"), home: qsTr("Home"), recent: qsTr("Recent")}
                const first = labels[section] ?? root.title
                const result = [{text: first, uri: section ? [section] : root.pagePrefix}]
                if (root.title && root.title !== first)
                    result.push({text: root.title, uri: root.pagePrefix})
                return result
            }
            onBackRequested: History.previous(Qt.TabFocusReason)
            onForwardRequested: History.next(Qt.TabFocusReason)
            onBreadcrumbRequested: uri => {
                if (!History.exactMatch(History.viewPath, uri))
                    History.push(uri, Qt.TabFocusReason)
            }
            onViewRequested: MainCtx.gridView = !MainCtx.gridView
            onSearchRequested: pattern => root.search.pattern = pattern
            onContentFocusRequested: root.focusContent()
            onSortRequested: position => {
                const model = root.sort.model.map(entry => ({text: entry.text,
                    checked: entry.criteria === root.librarySortCriteria,
                    order: entry.criteria === root.librarySortCriteria ? root.sort.order : undefined}))
                const menu = root.sortMenu ?? librarySortMenu
                menu.popup(position, false, model)
            }
            onMoreRequested: position => libraryMenu.popup(position)
            Connections {
                target: root.search
                function onAskShow() { libraryHeader.showSearch() }
            }
            Connections {
                target: root.sortMenu
                function onSelected(index) { root.selectLibrarySort(index) }
            }
        }
    }

    SortMenu {
        id: librarySortMenu
        ctx: MainCtx
        onSelected: index => root.selectLibrarySort(index)
    }
    QmlGlobalMenu { id: libraryMenu; ctx: MainCtx }
    function selectLibrarySort(index) {
        if (!MainCtx.juxtopposedControls || index < 0 || index >= root.sort.model.length)
            return
        const criteria = root.sort.model[index].criteria
        if (root.librarySortCriteria === criteria)
            root.sort.order = root.sort.order === Qt.AscendingOrder ? Qt.DescendingOrder : Qt.AscendingOrder
        else {
            root.sort.order = Qt.AscendingOrder
        }
        root.sort.criteria = criteria
    }

    Component {
        id: traditionalHeader
        DefaultPageHeader {
            text: root.title

            leftPadding: VLCStyle.dynamicAppMargins(width) + VLCStyle.layout_left_margin + root.leftPadding
            rightPadding: VLCStyle.dynamicAppMargins(width) + VLCStyle.layout_right_margin + root.rightPadding

            sortMenu: root.sortMenu

            search: _searchCtx
            sort: _sortCtx

            Navigation.parentItem: root
            Navigation.downAction: root.focusContent
        }
    }

    component DefaultPageHeader: T.ToolBar {
        id: defaultPageHeader

        // Properties
        property alias text: label.text

        property alias sortMenu: gridSortFilter.sortMenu

        property alias search: gridSortFilter.search
        property alias sort: gridSortFilter.sort

        position: T.ToolBar.Header

        topPadding: VLCStyle.layoutTitle_top_padding
        bottomPadding: VLCStyle.layoutTitle_bottom_padding

        // FIXME: `GridItem`'s background extends beyond its
        //        bounding rect, violating the hypothetical
        //        clip test (see 7e6b23db).
        bottomInset: (MainCtx.hasGridListMode && MainCtx.gridView) ? VLCStyle.gridItemSelectedBorder : undefined

        // Settings

        implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                                implicitContentWidth + leftPadding + rightPadding)
        implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                                 implicitContentHeight + topPadding + bottomPadding)

        Navigation.navigable: gridSortFilter.enabled

        // Children

        readonly property ColorContext colorContext: ColorContext {
            id: theme
            colorSet: ColorContext.View
        }

        background: Rectangle {
            color: theme.bg.primary
        }

        RowLayout {
            id: row

            anchors.fill: parent

            SubtitleLabel {
                id: label

                Layout.fillWidth: true

                color: theme.fg.primary
            }

            GridSortFilterControls {
                id: gridSortFilter
                focus: true
                sort: _sortCtx
                search: _searchCtx
                Navigation.parentItem: defaultPageHeader
            }
        }
    }
}
