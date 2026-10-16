/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * Visual design: Juxtopposed, https://www.youtube.com/@juxtopposed
 * Code: GPLv2 or later. Original artwork: CC BY 4.0.
 *****************************************************************************/
import QtQuick
import VLC.Style
import QtQuick.Controls.Basic as C
import QtQuick.Layouts
import VLC.Widgets as Widgets
import VLC.MainInterface as MainInterface

FocusScope {
    id: root
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
    readonly property bool folderMode: folderAvailable && displayFolder
    property string folderName: ""
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
    property bool libraryAvailable: true
    property bool libraryBusy: false
    property bool lyricsAvailable: false
    property bool lyricsEnabled: false
    readonly property bool editingOrMenuActive: searchField.activeFocus || sourceMenu.visible
    readonly property bool artworkReady: coverImage.status === Image.Ready
    readonly property real railWidth: 46 * uiScale
    readonly property bool compact: width < 1280 * uiScale
    readonly property real sourcePanelWidth: Math.min((compact ? 250 : 380) * uiScale,
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
    signal folderOpenRequested()
    signal routeRequested(var route, int reason)
    signal preferencesRequested()
    signal searchRequested(string pattern)
    signal sortRequested()
    signal refreshRequested()
    signal controlsFocusRequested(int reason)
    signal lyricsRequested()

    Accessible.name: qsTr("Music player")
    Component.onCompleted: setCurrentItemFocus(Qt.OtherFocusReason)
    FontLoader { id: inter; source: root.assetBase + "InterVariable.ttf" }
    function setCurrentItemFocus(reason) {
        if (tracks.count > 0) {
            tracks.currentIndex = root.currentIndex >= 0 ? root.currentIndex : 0
            tracks.forceActiveFocus(reason)
            tracks.currentItem?.forceActiveFocus(reason)
        } else addButton.forceActiveFocus(reason)
    }
    onCurrentIndexChanged: {
        if (currentIndex >= 0) {
            tracks.currentIndex = currentIndex
            tracks.positionViewAtIndex(currentIndex, ListView.Contain)
        }
    }
    onFolderModeChanged: {
        if (!folderMode && searchField)
            searchField.visible = false
    }
    Keys.onEscapePressed: backRequested()

    MainInterface.JuxtopposedNavigation {
        id: navigation
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: root.railWidth
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

    Rectangle {
        id: sourcePanel
        objectName: "musicSourcePanel"
        anchors.left: navigation.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: root.sourcePanelWidth
        color: JuxtopposedPalette.color("#EE111111")
        radius: 6 * root.uiScale

        component HeaderButton: C.ToolButton {
            id: button
            property string description
            property string glyph
            width: 30 * root.uiScale
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
            contentItem: C.Label {
                text: button.glyph
                font.family: root.iconFontFamily
                font.pixelSize: 24 * root.uiScale
                color: button.enabled ? JuxtopposedPalette.color("#969696") : JuxtopposedPalette.color("#454545")
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                Accessible.ignored: true
            }
        }
        RowLayout {
            id: heading
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 16 * root.uiScale
            height: 30 * root.uiScale
            spacing: 4 * root.uiScale
            HeaderButton {
                objectName: "musicBack"
                description: qsTr("Back to library")
                glyph: root.backIcon
                onClicked: root.backRequested()
            }
            C.ToolButton {
                objectName: "musicSourceName"
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                padding: 4 * root.uiScale
                Accessible.name: root.folderMode ? qsTr("Source folder: %1").arg(root.folderName) : qsTr("Play Queue")
                Keys.onReturnPressed: clicked()
                onClicked: sourceMenu.popup()
                background: Rectangle {
                    color: parent.hovered ? JuxtopposedPalette.color("#303030") : "transparent"
                    border.width: parent.visualFocus ? 2 * root.uiScale : 0
                    border.color: JuxtopposedPalette.color("#D37918")
                    radius: 4 * root.uiScale
                }
                contentItem: C.Label {
                    text: root.folderMode ? root.folderName : qsTr("Play Queue")
                    textFormat: Text.PlainText
                    elide: Text.ElideRight
                    font.family: inter.name
                    font.pixelSize: 16 * root.uiScale
                    color: JuxtopposedPalette.color("#969696")
                }
            }
            HeaderButton {
                objectName: "musicSourceMenu"
                description: qsTr("Source options")
                glyph: root.moreIcon
                onClicked: sourceMenu.popup()
            }
        }
        C.Menu {
            id: sourceMenu
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
        }
        RowLayout {
            id: summary
            anchors.left: heading.left
            anchors.right: heading.right
            anchors.top: heading.bottom
            anchors.topMargin: 8 * root.uiScale
            height: 30 * root.uiScale
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
                contentItem: Image {
                    source: root.assetBase + "queue-sort.svg"
                    width: 24 * root.uiScale
                    height: width
                    fillMode: Image.PreserveAspectFit
                    Accessible.ignored: true
                }
                onClicked: root.sortRequested()
            }
            HeaderButton {
                objectName: "musicSearchToggle"
                visible: root.folderMode
                description: qsTr("Search folder")
                glyph: root.searchIcon
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
            anchors.topMargin: 8 * root.uiScale
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
        ListView {
            id: tracks
            objectName: "musicSourceList"
            anchors.top: statusLabel.bottom
            anchors.left: heading.left
            anchors.right: heading.right
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 12 * root.uiScale
            clip: true
            model: root.sourceModel
            currentIndex: root.currentIndex
            boundsBehavior: Flickable.StopAtBounds
            C.ScrollBar.vertical: C.ScrollBar {}
            Keys.onDownPressed: { incrementCurrentIndex(); currentItem?.forceActiveFocus(Qt.TabFocusReason) }
            Keys.onUpPressed: {
                if (currentIndex > 0) { decrementCurrentIndex(); currentItem?.forceActiveFocus(Qt.BacktabFocusReason) }
                else addButton.forceActiveFocus(Qt.BacktabFocusReason)
            }
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
                onSelectionRequested: { tracks.currentIndex = index; forceActiveFocus(Qt.MouseFocusReason) }
                onActivationRequested: root.activationRequested(index)
                Keys.onDownPressed: { tracks.incrementCurrentIndex(); tracks.currentItem?.forceActiveFocus(Qt.TabFocusReason) }
                Keys.onUpPressed: {
                    if (tracks.currentIndex > 0) { tracks.decrementCurrentIndex(); tracks.currentItem?.forceActiveFocus(Qt.BacktabFocusReason) }
                    else addButton.forceActiveFocus(Qt.BacktabFocusReason)
                }
                Keys.onRightPressed: root.controlsFocusRequested(Qt.TabFocusReason)
            }
        }
    }
    Item {
        id: artworkPane
        anchors.left: sourcePanel.right
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
