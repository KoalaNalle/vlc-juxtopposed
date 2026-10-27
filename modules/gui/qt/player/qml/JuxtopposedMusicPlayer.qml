/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * Visual design: Juxtopposed, https://www.youtube.com/@juxtopposed
 * Code: GPLv2 or later. Original artwork: CC BY 4.0.
 *****************************************************************************/
import QtQuick
import VLC.Style
import QtQuick.Controls.Basic as C
import QtQuick.Layouts
import QtQml.Models
import VLC.Widgets as Widgets
import VLC.MainInterface as MainInterface
import VLC.MediaLibrary as MediaLibrary
import VLC.Util

FocusScope {
    id: root
    // Mouse-focused source rows and buttons own their navigation/activation keys.
    readonly property bool navigationKeyScope: true
    property real uiScale: 1
    property url assetBase: "qrc:/juxtopposed/"
    property url artwork
    property url fallbackArtwork
    property string title: ""
    property string artist: ""
    property var sourceModel
    property int itemCount: 0
    property int currentIndex: -1
    property bool folderAvailable: false
    property bool displayFolder: true
    property bool sidebarVisible: true
    readonly property bool folderMode: folderAvailable && displayFolder
    property string folderName: ""
    property var folderBreadcrumbs: []
    property bool canGoBack: false
    property bool canGoForward: false
    property bool gridMode: false
    property int selectedRow: -1
    property bool busy: false
    property string error: ""
    property string searchPattern: ""
    property bool sortAscending: true
    property string iconFontFamily: ""
    property string backIcon: ""
    property string moreIcon: ""
    property string addIcon: ""
    property string searchIcon: ""
    property string preferencesIcon: ""
    property var navigationRoutes: []
    property Component navigationComponent: defaultNavigation
    property bool libraryAvailable: true
    property bool libraryBusy: false
    property bool lyricsAvailable: false
    property bool lyricsEnabled: false
    readonly property bool editingOrMenuActive: searchField.activeFocus || sourceMenu.visible || ancestorMenu.visible
    readonly property bool artworkReady: coverImage.status === Image.Ready
    readonly property real railWidth: 50 * uiScale
    readonly property bool compact: width < 1280 * uiScale
    readonly property real sourcePanelWidth: Math.min((compact ? 250 : 377) * uiScale,
                                                     Math.max(180 * uiScale, width * .36))
    readonly property Item artworkTextureProvider: coverImage.status === Image.Ready
                                                   ? coverImage.sourceTextureProviderItem
                                                   : coverFallback.sourceTextureProviderItem
    readonly property real artworkAspectRatio: artworkTextureProvider.implicitWidth > 0
                                               && artworkTextureProvider.implicitHeight > 0
                                               ? artworkTextureProvider.implicitWidth / artworkTextureProvider.implicitHeight : 1
    readonly property real coverSize: coverFrame.width
    signal activationRequested(int row)
    signal addRequested()
    signal backRequested()
    signal folderBackRequested()
    signal folderForwardRequested()
    signal folderBrowseRequested(url uri)
    signal sourceFolderRequested()
    signal folderOpenRequested()
    signal routeRequested(var route, int reason)
    signal preferencesRequested()
    signal searchRequested(string pattern)
    signal sortRequested()
    signal refreshRequested()
    signal controlsFocusRequested(int reason)
    signal lyricsRequested()

    Accessible.name: qsTr("Music player")
    property bool _completed: false
    Component.onCompleted: {
        _completed = true
        setCurrentItemFocus(Qt.OtherFocusReason)
    }
    onSidebarVisibleChanged: {
        if (_completed)
            setCurrentItemFocus(Qt.OtherFocusReason)
    }
    FontLoader { id: inter; source: root.assetBase + "InterVariable.ttf" }
    function setCurrentItemFocus(reason) {
        if (!sidebarVisible) {
            controlsFocusRequested(reason)
            return
        }
        if (sourceView.item?.count > 0) {
            selectRow(root.currentIndex >= 0 ? root.currentIndex : Math.max(0, selectedRow), reason)
        } else addButton.forceActiveFocus(reason)
    }
    function selectRow(row, reason) {
        const view = sourceView.item
        if (!view || view.count <= 0) return
        selectedRow = Math.max(0, Math.min(view.count - 1, row))
        view.currentIndex = selectedRow
        view.positionViewAtIndex(selectedRow, gridMode ? GridView.Contain : ListView.Contain)
        view.forceActiveFocus(reason)
        view.currentItem?.forceActiveFocus(reason)
    }
    function moveSelection(offset) {
        if (sourceView.item.currentIndex + offset < 0)
            addButton.forceActiveFocus(Qt.BacktabFocusReason)
        else
            selectRow(sourceView.item.currentIndex + offset, Qt.TabFocusReason)
    }
    onCurrentIndexChanged: {
        if (currentIndex >= 0 && sourceView.item) {
            selectedRow = currentIndex
            sourceView.item.currentIndex = currentIndex
            sourceView.item.positionViewAtIndex(currentIndex, gridMode ? GridView.Contain : ListView.Contain)
        }
    }
    onFolderModeChanged: {
        if (!folderMode && searchField)
            searchField.visible = false
    }
    Keys.onEscapePressed: backRequested()

    Loader {
        id: navigation
        visible: root.sidebarVisible
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: root.railWidth
        focus: true
        sourceComponent: root.navigationComponent
    }
    Component {
        id: defaultNavigation
        MainInterface.JuxtopposedNavigation {
            uiScale: root.uiScale
            assetBase: root.assetBase
            iconFontFamily: root.iconFontFamily
            activeRoute: "music"
            routes: root.navigationRoutes
            preferencesIcon: root.preferencesIcon
            libraryAvailable: root.libraryAvailable
            libraryBusy: root.libraryBusy
            onRouteRequested: (route, reason) => root.routeRequested(route, reason)
            onPreferencesRequested: root.preferencesRequested()
            onContentFocusRequested: reason => root.setCurrentItemFocus(reason)
        }
    }

    Rectangle {
        id: sourcePanel
        objectName: "musicSourcePanel"
        visible: root.sidebarVisible
        anchors.left: navigation.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: root.sourcePanelWidth
        color: JuxtopposedPalette.color("#EE111111")
        radius: 6 * root.uiScale
        readonly property bool compactHeader: width < 300 * root.uiScale

        component HeaderButton: C.ToolButton {
            id: button
            property string description
            property string artworkName
            property bool mirrorArtwork: false
            property real artworkSize: 24
            width: 24 * root.uiScale
            height: width
            padding: 0
            focusPolicy: Qt.StrongFocus
            Accessible.name: description
            C.ToolTip.visible: hovered || visualFocus
            C.ToolTip.text: description
            C.ToolTip.delay: 500
            Keys.onReturnPressed: clicked()
            Keys.onEnterPressed: clicked()
            background: Rectangle {
                color: button.down ? JuxtopposedPalette.color("#404040") : button.hovered ? JuxtopposedPalette.color("#303030") : "transparent"
                radius: 4 * root.uiScale
                border.width: button.visualFocus ? 2 * root.uiScale : 0
                border.color: JuxtopposedPalette.color("#D37918")
            }
            contentItem: Item {
                opacity: button.enabled ? 1 : 0.3
                Image {
                    anchors.centerIn: parent
                    width: button.artworkSize * root.uiScale
                    height: width
                    mirror: button.mirrorArtwork
                    source: SVGColorImage.colorize(root.assetBase + button.artworkName)
                        .any({"#8F8F8F": JuxtopposedPalette.color("#8F8F8F"),
                              "#969696": JuxtopposedPalette.color("#8F8F8F")}).uri()
                    Accessible.ignored: true
                }
            }
        }
        component SourceMenu: C.Menu {
            palette.window: JuxtopposedPalette.color("#161616")
            palette.windowText: JuxtopposedPalette.color("#969696")
            palette.base: JuxtopposedPalette.color("#161616")
            palette.text: JuxtopposedPalette.color("#969696")
            palette.button: JuxtopposedPalette.color("#202020")
            palette.buttonText: JuxtopposedPalette.color("#969696")
            palette.highlight: JuxtopposedPalette.color("#303030")
            palette.highlightedText: JuxtopposedPalette.color("#D37918")
            palette.light: JuxtopposedPalette.color("#303030")
            palette.midlight: JuxtopposedPalette.color("#404040")
        }
        RowLayout {
            id: heading
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 20 * root.uiScale
            height: 24 * root.uiScale
            spacing: 10 * root.uiScale
            Row {
                spacing: 12 * root.uiScale
                HeaderButton {
                    objectName: "musicBack"
                    description: qsTr("Previous source folder")
                    artworkName: "music-source-back-785d7.svg"
                    mirrorArtwork: true
                    enabled: root.folderMode && root.canGoBack
                    onClicked: root.folderBackRequested()
                }
                HeaderButton {
                    objectName: "musicForward"
                    description: qsTr("Next source folder")
                    artworkName: "music-source-forward-a7541.svg"
                    mirrorArtwork: true
                    enabled: root.folderMode && root.canGoForward
                    onClicked: root.folderForwardRequested()
                }
            }
            RowLayout {
                id: crumbs
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                spacing: 6 * root.uiScale
                readonly property int visibleCount: sourcePanel.width < 300 * root.uiScale ? 1 : 2
                readonly property var entries: root.folderMode && root.folderBreadcrumbs.length
                    ? root.folderBreadcrumbs.slice(-visibleCount)
                    : [{text: root.folderMode ? root.folderName : qsTr("Play Queue"), uri: ""}]
                HeaderButton {
                    objectName: "musicAncestors"
                    visible: !sourcePanel.compactHeader && root.folderMode && root.folderBreadcrumbs.length > crumbs.entries.length
                    description: qsTr("Ancestor folders")
                    artworkName: "titlebar-menu.svg"
                    artworkSize: 18
                    mirrorArtwork: true
                    onClicked: ancestorMenu.popup()
                }
                Repeater {
                    model: crumbs.entries
                    RowLayout {
                        required property var modelData
                        required property int index
                        Layout.fillWidth: true
                        Layout.minimumWidth: index < crumbs.entries.length - 1 ? Math.min(implicitWidth, 64 * root.uiScale, crumbs.width * .4) : 0
                        Layout.maximumWidth: index < crumbs.entries.length - 1 ? Math.min(64 * root.uiScale, crumbs.width * .4) : Infinity
                        spacing: 6 * root.uiScale
                        C.Label {
                            visible: index > 0 || (!sourcePanel.compactHeader && root.folderBreadcrumbs.length > crumbs.entries.length)
                            text: "›"
                            color: JuxtopposedPalette.color("#8F8F8F")
                            font.pixelSize: 16 * root.uiScale
                            Accessible.ignored: true
                        }
                        C.ToolButton {
                            objectName: index === crumbs.entries.length - 1 ? "musicSourceName" : "musicParentFolder"
                            Layout.fillWidth: true
                            Layout.minimumWidth: 0
                            Layout.preferredHeight: 24 * root.uiScale
                            padding: 0
                            Accessible.name: root.folderMode ? qsTr("Source folder: %1").arg(modelData.text) : qsTr("Play Queue")
                            Accessible.description: index === crumbs.entries.length - 1 ? qsTr("Source options") : qsTr("Browse folder")
                            C.ToolTip.visible: hovered || visualFocus
                            C.ToolTip.text: modelData.text
                            C.ToolTip.delay: 500
                            Keys.onReturnPressed: clicked()
                            Keys.onEnterPressed: clicked()
                            onClicked: {
                                if (index === crumbs.entries.length - 1) sourceMenu.popup()
                                else if (root.folderMode && modelData.uri.toString().length)
                                    root.folderBrowseRequested(modelData.uri)
                                else sourceMenu.popup()
                            }
                            background: Rectangle {
                                color: parent.hovered ? JuxtopposedPalette.color("#303030") : "transparent"
                                border.width: parent.visualFocus ? 2 * root.uiScale : 0
                                border.color: JuxtopposedPalette.color("#D37918")
                                radius: 4 * root.uiScale
                            }
                            contentItem: C.Label {
                                text: modelData.text
                                textFormat: Text.PlainText
                                elide: Text.ElideRight
                                font.family: inter.name
                                font.pixelSize: 16 * root.uiScale
                                color: JuxtopposedPalette.color("#969696")
                            }
                        }
                    }
                }
            }
            Row {
                spacing: 12 * root.uiScale
                HeaderButton {
                    objectName: "musicSourceMenu"
                    visible: !sourcePanel.compactHeader
                    description: qsTr("Source options")
                    artworkName: "titlebar-menu.svg"
                    artworkSize: 18
                    mirrorArtwork: true
                    onClicked: sourceMenu.popup()
                }
                HeaderButton {
                    objectName: "musicExternalFolder"
                    description: qsTr("Open source folder in file manager")
                    artworkName: "music-source-external-6dd1e.svg"
                    mirrorArtwork: true
                    enabled: root.folderMode
                    onClicked: root.folderOpenRequested()
                }
            }
        }
        SourceMenu {
            id: ancestorMenu
            Instantiator {
                model: root.folderBreadcrumbs.slice(0, -crumbs.entries.length)
                delegate: C.MenuItem {
                    required property var modelData
                    text: modelData.text
                    onTriggered: {
                        const uri = modelData.uri
                        ancestorMenu.close()
                        root.folderBrowseRequested(uri)
                    }
                }
                onObjectAdded: (index, object) => ancestorMenu.insertItem(index, object)
                onObjectRemoved: (index, object) => ancestorMenu.removeItem(object)
            }
        }
        SourceMenu {
            id: sourceMenu
            C.MenuItem {
                text: qsTr("Source folder")
                enabled: root.folderAvailable
                checkable: true
                checked: root.folderMode
                onTriggered: root.displayFolder = true
            }
            C.MenuItem {
                text: qsTr("Play Queue")
                checkable: true
                checked: !root.folderMode
                onTriggered: root.displayFolder = false
            }
            C.MenuSeparator {}
            C.MenuItem {
                text: qsTr("Open source folder")
                enabled: root.folderAvailable
                onTriggered: root.folderOpenRequested()
            }
            C.MenuItem {
                text: qsTr("Current track folder")
                enabled: root.folderAvailable
                onTriggered: { root.displayFolder = true; root.sourceFolderRequested() }
            }
            SourceMenu {
                id: sourceAncestors
                title: qsTr("Ancestor folders")
                enabled: root.folderMode && root.folderBreadcrumbs.length > 1
                Instantiator {
                    model: root.folderBreadcrumbs.slice(0, -1)
                    delegate: C.MenuItem {
                        required property var modelData
                        text: modelData.text
                        onTriggered: {
                            const uri = modelData.uri
                            sourceAncestors.close()
                            sourceMenu.close()
                            root.folderBrowseRequested(uri)
                        }
                    }
                    onObjectAdded: (index, object) => sourceAncestors.insertItem(index, object)
                    onObjectRemoved: (index, object) => sourceAncestors.removeItem(object)
                }
            }
            C.MenuItem {
                text: qsTr("Refresh folder")
                enabled: root.folderAvailable
                onTriggered: root.refreshRequested()
            }
            C.MenuItem {
                text: qsTr("Show lyrics")
                enabled: root.lyricsAvailable
                checked: root.lyricsEnabled
                checkable: true
                onTriggered: root.lyricsRequested()
            }
            C.MenuItem {
                text: qsTr("Back to library")
                onTriggered: root.backRequested()
            }
        }
        RowLayout {
            id: summary
            anchors.left: heading.left
            anchors.right: heading.right
            anchors.top: heading.bottom
            anchors.topMargin: 20 * root.uiScale
            height: 24 * root.uiScale
            spacing: 12 * root.uiScale
            C.Label {
                Layout.fillWidth: true
                text: root.itemCount === 1 ? qsTr("1 item") : qsTr("%1 items").arg(root.itemCount)
                color: JuxtopposedPalette.color("#6B6B6B")
                font.family: inter.name
                font.pixelSize: 14 * root.uiScale
            }
            HeaderButton {
                objectName: "musicFolderSort"
                visible: root.folderMode
                description: root.sortAscending ? qsTr("Sort descending") : qsTr("Sort ascending")
                artworkName: "queue-sort.svg"
                onClicked: root.sortRequested()
            }
            HeaderButton {
                objectName: "musicViewToggle"
                description: root.gridMode ? qsTr("Switch to list view") : qsTr("Switch to cover view")
                artworkName: "music-source-view-fcd8d.svg"
                onClicked: {
                    root.gridMode = !root.gridMode
                    Qt.callLater(() => root.selectRow(Math.max(0, root.selectedRow), Qt.ShortcutFocusReason))
                }
            }
            HeaderButton {
                objectName: "musicSearchToggle"
                visible: root.folderMode
                description: qsTr("Search folder")
                artworkName: "controls-settings-80d5a.svg"
                onClicked: {
                    searchField.visible = !searchField.visible
                    if (searchField.visible) searchField.forceActiveFocus(Qt.ShortcutFocusReason)
                    else { root.searchRequested(""); root.setCurrentItemFocus(Qt.ShortcutFocusReason) }
                }
            }
        }
        Rectangle {
            id: separator
            anchors.left: heading.left
            anchors.right: heading.right
            anchors.top: summary.bottom
            anchors.topMargin: 17 * root.uiScale
            height: root.uiScale
            color: JuxtopposedPalette.color("#252525")
        }
        C.TextField {
            id: searchField
            objectName: "musicSearch"
            anchors.top: separator.bottom
            anchors.left: heading.left
            anchors.right: heading.right
            height: visible ? 34 * root.uiScale : 0
            visible: false
            text: root.searchPattern
            placeholderText: qsTr("Search source folder")
            selectByMouse: true
            Accessible.name: qsTr("Search source folder")
            font.family: inter.name
            font.pixelSize: 14 * root.uiScale
            color: JuxtopposedPalette.color("#969696")
            selectionColor: JuxtopposedPalette.color("#D37918")
            onTextEdited: root.searchRequested(text)
            Keys.onEscapePressed: { visible = false; root.searchRequested(""); root.setCurrentItemFocus(Qt.ShortcutFocusReason) }
            Keys.onReturnPressed: root.setCurrentItemFocus(Qt.ShortcutFocusReason)
            background: Rectangle { color: JuxtopposedPalette.color("#202020"); radius: 4 * root.uiScale }
        }
        C.ToolButton {
            id: addButton
            objectName: "musicAdd"
            anchors.top: searchField.bottom
            anchors.topMargin: 12 * root.uiScale
            anchors.left: heading.left
            anchors.right: heading.right
            height: 44 * root.uiScale
            padding: 0
            Accessible.name: qsTr("Add audio files to play queue")
            Keys.onReturnPressed: clicked()
            Keys.onDownPressed: root.setCurrentItemFocus(Qt.TabFocusReason)
            onClicked: root.addRequested()
            background: Rectangle {
                color: parent.hovered ? JuxtopposedPalette.color("#202020") : "transparent"
                radius: 4 * root.uiScale
                border.width: parent.visualFocus ? 2 * root.uiScale : 0
                border.color: JuxtopposedPalette.color("#D37918")
            }
            contentItem: Row {
                spacing: 12 * root.uiScale
                C.Label {
                    width: 32 * root.uiScale
                    height: parent.height
                    text: root.addIcon
                    font.family: root.iconFontFamily
                    font.pixelSize: 24 * root.uiScale
                    color: JuxtopposedPalette.color("#969696")
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                }
                C.Label {
                    height: parent.height
                    text: qsTr("Add")
                    font.family: inter.name
                    font.pixelSize: 14 * root.uiScale
                    color: JuxtopposedPalette.color("#969696")
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
        C.Label {
            id: statusLabel
            objectName: "musicSourceStatus"
            anchors.top: addButton.bottom
            anchors.left: heading.left
            anchors.right: heading.right
            height: visible ? contentHeight + 8 * root.uiScale : 0
            visible: root.error.length > 0 || (root.busy && root.folderMode) || root.itemCount === 0
            text: root.error || (root.busy && root.folderMode ? qsTr("Reading audio metadata…")
                : root.searchPattern.length > 0 ? qsTr("No matching audio files") : qsTr("No audio files"))
            textFormat: Text.PlainText
            wrapMode: Text.WordWrap
            font.family: inter.name
            font.pixelSize: 12 * root.uiScale
            color: JuxtopposedPalette.color("#969696")
        }
        Loader {
            id: sourceView
            anchors.top: statusLabel.bottom
            anchors.left: heading.left
            anchors.right: heading.right
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 12 * root.uiScale
            sourceComponent: root.gridMode ? coversComponent : listComponent
        }
        Component {
            id: listComponent
            ListView {
                id: tracks
                objectName: "musicSourceList"
                clip: true
                model: root.sourceModel
                currentIndex: root.selectedRow
                boundsBehavior: Flickable.StopAtBounds
                C.ScrollBar.vertical: C.ScrollBar {}
                Keys.onDownPressed: root.moveSelection(1)
                Keys.onUpPressed: root.moveSelection(-1)
                Keys.onRightPressed: root.controlsFocusRequested(Qt.TabFocusReason)
                delegate: Widgets.JuxtopposedMediaRow {
                    required property var model
                    required property int index
                    objectName: "musicSourceRow" + index
                    width: tracks.width
                    uiScale: root.uiScale
                    assetBase: root.assetBase
                    title: model.artist ? model.title + " - " + model.artist : model.title
                    artwork: model.artwork
                    fallbackArtwork: root.fallbackArtwork
                    current: index === root.currentIndex
                    selected: ListView.isCurrentItem
                    onSelectionRequested: root.selectRow(index, Qt.MouseFocusReason)
                    onActivationRequested: root.activationRequested(index)
                    Keys.onDownPressed: root.moveSelection(1)
                    Keys.onUpPressed: root.moveSelection(-1)
                    Keys.onRightPressed: root.controlsFocusRequested(Qt.TabFocusReason)
                }
            }
        }
        Component {
            id: coversComponent
            GridView {
                id: covers
                objectName: "musicSourceGrid"
                clip: true
                model: root.sourceModel
                currentIndex: root.selectedRow
                readonly property int columns: Math.max(1, Math.floor(width / (150 * root.uiScale)))
                cellWidth: width / columns
                cellHeight: cellWidth + 70 * root.uiScale
                boundsBehavior: Flickable.StopAtBounds
                C.ScrollBar.vertical: C.ScrollBar {}
                Keys.onDownPressed: root.moveSelection(columns)
                Keys.onUpPressed: root.moveSelection(-columns)
                Keys.onLeftPressed: root.moveSelection(-1)
                Keys.onRightPressed: root.moveSelection(1)
                delegate: MediaLibrary.JuxtopposedHomeCard {
                    required property var model
                    required property int index
                    objectName: "musicSourceCard" + index
                    width: covers.cellWidth - 12 * root.uiScale
                    height: covers.cellHeight - 12 * root.uiScale
                    uiScale: root.uiScale
                    fontFamily: inter.name
                    kind: "audio"
                    entry: ({title: model.title, main_artist: model.artist, artwork: model.artwork})
                    fallbackArtwork: root.fallbackArtwork
                    textColor: JuxtopposedPalette.color(index === root.currentIndex ? "#D37918" : "#969696")
                    Accessible.selected: GridView.isCurrentItem
                    onClicked: root.selectRow(index, Qt.MouseFocusReason)
                    onDoubleClicked: root.activationRequested(index)
                    Keys.onReturnPressed: root.activationRequested(index)
                    Keys.onEnterPressed: root.activationRequested(index)
                    Keys.onSpacePressed: root.activationRequested(index)
                    Keys.onDownPressed: root.moveSelection(covers.columns)
                    Keys.onUpPressed: root.moveSelection(-covers.columns)
                    Keys.onLeftPressed: root.moveSelection(-1)
                    Keys.onRightPressed: root.moveSelection(1)
                }
            }
        }
    }
    Item {
        id: artworkPane
        anchors.left: root.sidebarVisible ? sourcePanel.right : parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        readonly property real padding: Math.min(74 * root.uiScale, height * .07)
        readonly property real labelGap: Math.min(74 * root.uiScale, height * .07)
        readonly property real desiredSize: Math.min(root.width * .386,
            height - padding * 2 - labelGap - trackLabel.height,
            width - 48 * root.uiScale)
        Item {
            id: coverFrame
            objectName: "musicCoverFrame"
            width: Math.max(0, artworkPane.desiredSize)
            height: width
            // The canonical cover is near the whole window's centre, rather than
            // the free pane's centre. Clamp only when the source panel would collide.
            x: Math.max(24 * root.uiScale, root.width / 2 - artworkPane.x - width / 2)
            y: Math.max(12 * root.uiScale, (artworkPane.height - height - artworkPane.labelGap - trackLabel.height) / 2)
            Widgets.ImageExt {
                id: coverFallback
                anchors.fill: parent
                source: root.fallbackArtwork
                fillMode: Image.PreserveAspectCrop
                radius: 20 * root.uiScale
                visible: coverImage.status !== Image.Ready
                sourceSize: Qt.size(width * 2, height * 2)
                Accessible.ignored: true
            }
            Widgets.ImageExt {
                id: coverImage
                objectName: "musicCover"
                anchors.fill: parent
                source: root.artwork
                cache: false
                fillMode: Image.PreserveAspectCrop
                radius: 20 * root.uiScale
                sourceSize: Qt.size(width * 2, height * 2)
                Accessible.role: Accessible.Graphic
                Accessible.name: root.title.length ? qsTr("Cover for %1").arg(root.title) : qsTr("Music cover")
            }
        }
        C.Label {
            id: trackLabel
            objectName: "musicTrackLabel"
            x: coverFrame.x - Math.max(0, width - coverFrame.width) / 2
            y: coverFrame.y + coverFrame.height + artworkPane.labelGap
            width: Math.min(artworkPane.width - 32 * root.uiScale, coverFrame.width + 120 * root.uiScale)
            text: root.artist.length > 0 ? root.title + " - " + root.artist : root.title
            textFormat: Text.PlainText
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignHCenter
            font.family: inter.name
            font.pixelSize: (root.compact ? 20 : 24) * root.uiScale
            color: JuxtopposedPalette.color("#969696")
            Accessible.role: Accessible.StaticText
        }
    }
}
