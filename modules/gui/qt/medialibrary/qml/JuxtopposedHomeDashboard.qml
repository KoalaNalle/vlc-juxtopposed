/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * Visual design by Juxtopposed: https://www.youtube.com/@juxtopposed
 * Code licensed under GPLv2 or later.
 *****************************************************************************/
import QtQuick
import QtQuick.Controls.Basic as C
import QtQuick.Layouts

Flickable {
    id: root
    property real uiScale: 1
    property url assetBase: "qrc:/juxtopposed/"
    property string iconFontFamily: ""
    property string arrowIcon: ""
    property string videoIcon: ""
    property string audioIcon: ""
    property string folderIcon: ""
    property string playlistIcon: ""
    property string artistIcon: ""
    property url folderArtwork: ""
    property url artistArtwork: ""
    property var videoEntries: null
    property var audioEntries: null
    property var folderEntries: []
    property var playlistEntries: []
    property var artistEntries: []
    property bool gridMode: true
    property bool historyLoading: false
    property bool activityLoading: false
    property bool libraryBusy: false
    property string errorText: ""
    property string searchPattern: ""
    property bool mediaEmpty: false
    property color surfaceColor: "#0F0F0F"
    property color textColor: "#969696"
    property color accentColor: "#D37918"
    property Item focusedEntry: null
    signal mediaRequested(var entry, bool resume, int reason)
    signal folderRequested(var entry, int reason)
    signal playlistRequested(var entry, int reason)
    signal artistRequested(var entry, int reason)
    signal seeAllRequested(string category, int reason)
    signal addFolderRequested()
    signal retryRequested()

    clip: true
    contentWidth: width
    contentHeight: sections.implicitHeight + 88 * uiScale
    boundsBehavior: Flickable.StopAtBounds
    flickableDirection: Flickable.VerticalFlick
    C.ScrollBar.vertical: C.ScrollBar {}
    Accessible.name: qsTr("Home media library")
    Rectangle { parent: root; anchors.fill: parent; z: -1; color: root.surfaceColor }
    onWidthChanged: Qt.callLater(revealFocused)
    onHeightChanged: Qt.callLater(revealFocused)
    onContentHeightChanged: Qt.callLater(revealFocused)
    onUiScaleChanged: Qt.callLater(revealFocused)
    onGridModeChanged: Qt.callLater(revealFocused)
    function revealFocused() {
        if (focusedEntry?.activeFocus && focusedEntry.visible)
            reveal(focusedEntry)
    }

    function setCurrentItemFocus(reason) {
        continueSection.setCurrentItemFocus(reason)
    }
    function positionContentAtBeginning() { contentY = 0 }
    function reveal(item) {
        const p = item.mapToItem(sections, 0, 0)
        const y = p.y + sections.y
        if (y < contentY)
            contentY = Math.max(0, y)
        else if (y + item.height > contentY + height)
            contentY = Math.min(Math.max(0, contentHeight - height), y + item.height - height)
    }
    function activate(kind, entry, reason) {
        if (kind === "video" || kind === "audio")
            mediaRequested(entry, kind === "video" && (entry?.progress || 0) > 0, reason)
        else if (kind === "folder")
            folderRequested(entry, reason)
        else if (kind === "playlist")
            playlistRequested(entry, reason)
        else
            artistRequested(entry, reason)
    }

    FontLoader { id: inter; source: root.assetBase + "InterVariable.ttf" }

    component Section: Column {
        id: section
        required property string title
        required property string category
        required property string kind
        property var entries: null
        property string fallbackIcon: ""
        property url fallbackArtwork: ""
        property string emptyText: ""
        property bool loading: false
        property int maximumShown: kind === "video" ? 4 : 4
        readonly property real cardWidth: (kind === "video" ? 320 : 200) * root.uiScale
        readonly property int fittingCount: root.gridMode ? Math.max(1, Math.floor((width + 20 * root.uiScale) / (cardWidth + 20 * root.uiScale))) : 3
        readonly property int shownCount: Math.min(maximumShown, fittingCount)
        readonly property bool hasRows: rows.count > 0
        width: parent.width
        spacing: (root.gridMode ? 18 : 14) * root.uiScale

        function setCurrentItemFocus(reason) {
            const card = rows.itemAt(0)
            if (card?.visible)
                card.forceActiveFocus(reason)
            else
                heading.forceActiveFocus(reason)
        }

        C.ToolButton {
            id: heading
            objectName: "homeSection" + section.category
            padding: 0
            width: root.gridMode ? implicitWidth : parent.width
            implicitHeight: 28 * root.uiScale
            focusPolicy: Qt.StrongFocus
            Accessible.name: section.title
            Accessible.description: qsTr("See all")
            onClicked: root.seeAllRequested(section.category, focusReason)
            onActiveFocusChanged: if (activeFocus) { root.focusedEntry = this; root.reveal(this) }
            background: Rectangle {
                color: heading.hovered ? "#242424" : "transparent"
                radius: 4 * root.uiScale
                border.width: heading.visualFocus ? 2 * root.uiScale : 0
                border.color: root.accentColor
            }
            contentItem: Item {
                implicitWidth: headingText.implicitWidth + 10 * root.uiScale + headingArrow.implicitWidth
                implicitHeight: Math.max(headingText.implicitHeight, headingArrow.implicitHeight)
                C.Label {
                    id: headingText
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    text: section.title
                    font.family: inter.name
                    font.pixelSize: 20 * root.uiScale
                    color: root.textColor
                }
                C.Label {
                    id: headingArrow
                    x: root.gridMode ? headingText.implicitWidth + 10 * root.uiScale : parent.width - width
                    text: root.arrowIcon
                    font.family: root.iconFontFamily
                    font.pixelSize: 18 * root.uiScale
                    color: root.textColor
                    anchors.verticalCenter: parent.verticalCenter
                    Accessible.ignored: true
                }
            }
        }
        Flow {
            width: parent.width
            spacing: (root.gridMode ? 20 : 0) * root.uiScale
            Repeater {
                id: rows
                // Activity lists can contain the complete visit history.
                // The dashboard creates only its preview; See All virtualizes
                // the full collection. Native media models already have limits.
                model: section.entries && typeof section.entries.slice === "function"
                       ? section.entries.slice(0, section.maximumShown) : section.entries
                delegate: JuxtopposedHomeCard {
                    required property var model
                    required property int index
                    readonly property var row: model.modelData ?? model
                    objectName: "home" + section.kind + index
                    entry: row
                    uiScale: root.uiScale
                    fontFamily: inter.name
                    iconFontFamily: root.iconFontFamily
                    fallbackIcon: section.fallbackIcon
                    fallbackArtwork: section.fallbackArtwork
                    gridMode: root.gridMode
                    kind: section.kind
                    visible: index < section.shownCount
                    width: root.gridMode ? Math.min(implicitWidth, parent.width) : parent.width
                    height: implicitHeight
                    surfaceColor: root.surfaceColor
                    textColor: root.textColor
                    accentColor: root.accentColor
                    onClicked: root.activate(kind, row, focusReason)
                    onActiveFocusChanged: if (activeFocus) { root.focusedEntry = this; root.reveal(this) }
                }
            }
        }
        C.Label {
            objectName: "homeEmpty" + section.category
            visible: !section.hasRows
            width: parent.width
            text: section.loading ? qsTr("Loading…") : root.searchPattern ? qsTr("No matching items") : section.emptyText
            wrapMode: Text.WordWrap
            font.family: inter.name
            font.pixelSize: 14 * root.uiScale
            color: "#626262"
        }
    }

    Column {
        id: sections
        x: 24 * root.uiScale
        y: 40 * root.uiScale
        width: Math.max(0, root.width - 48 * root.uiScale)
        spacing: (root.gridMode ? 72 : 28) * root.uiScale

        Column {
            width: parent.width
            spacing: 12 * root.uiScale
            visible: root.mediaEmpty || root.libraryBusy || root.errorText.length > 0
            C.Label {
                width: parent.width
                text: root.errorText || (root.libraryBusy ? qsTr("Scanning your media library…") : qsTr("Add a folder to start your media library."))
                wrapMode: Text.WordWrap
                font.family: inter.name
                font.pixelSize: 16 * root.uiScale
                color: root.textColor
            }
            Row {
                spacing: 12 * root.uiScale
                C.Button { objectName: "homeAddFolder"; text: qsTr("Add Folder"); onClicked: root.addFolderRequested() }
                C.Button { objectName: "homeRetry"; visible: root.errorText.length > 0; text: qsTr("Retry"); onClicked: root.retryRequested() }
            }
        }
        Section {
            id: continueSection
            title: qsTr("Continue Watching")
            category: "continueWatching"
            kind: "video"
            entries: root.videoEntries
            fallbackIcon: root.videoIcon
            loading: root.historyLoading
            emptyText: qsTr("Videos you watch will appear here.")
        }
        Section {
            title: qsTr("Listen Again")
            category: "listenAgain"
            kind: "audio"
            entries: root.audioEntries
            fallbackIcon: root.audioIcon
            maximumShown: 3
            loading: root.historyLoading
            emptyText: qsTr("Music you listen to will appear here.")
        }
        Section {
            title: qsTr("Frequently Visited Folders")
            category: "frequentFolders"
            kind: "folder"
            entries: root.folderEntries
            fallbackIcon: root.folderIcon
            fallbackArtwork: root.folderArtwork
            loading: root.activityLoading
            emptyText: qsTr("Folders you visit will appear here.")
        }
        Section {
            title: qsTr("Frequently Visited Playlists")
            category: "frequentPlaylists"
            kind: "playlist"
            entries: root.playlistEntries
            fallbackIcon: root.playlistIcon
            loading: root.activityLoading
            emptyText: qsTr("Playlists you visit will appear here.")
        }
        Section {
            title: qsTr("Frequently Listened Artists")
            category: "frequentArtists"
            kind: "artist"
            entries: root.artistEntries
            fallbackIcon: root.artistIcon
            fallbackArtwork: root.artistArtwork
            loading: root.activityLoading
            emptyText: qsTr("Artists you listen to will appear here.")
        }
    }
}
