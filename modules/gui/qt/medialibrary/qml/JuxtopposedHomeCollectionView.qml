/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * Visual design by Juxtopposed: https://www.youtube.com/@juxtopposed
 * Code licensed under GPLv2 or later.
 *****************************************************************************/
import QtQuick
import VLC.Style
import QtQuick.Controls.Basic as C

FocusScope {
    id: root
    property real uiScale: 1
    property url assetBase: "qrc:/juxtopposed/"
    property string iconFontFamily: ""
    property string fallbackIcon: ""
    property string audioFallbackIcon: ""
    property url fallbackArtwork: ""
    property string kind: "folder"
    property bool showLocation: false
    property var entries: null
    property bool gridMode: true
    property bool loading: false
    property string emptyText: qsTr("No items yet")
    property string errorText: ""
    property color accentColor: JuxtopposedPalette.color("#D37918")
    property color surfaceColor: JuxtopposedPalette.color("#0F0F0F")
    readonly property int count: view.count
    signal activated(var entry, string kind, int reason)
    signal retryRequested()
    activeFocusOnTab: true
    Rectangle { anchors.fill: parent; z: -1; color: root.surfaceColor }
    function setCurrentItemFocus(reason) {
        if (view.count > 0) {
            if (view.currentIndex < 0) view.currentIndex = 0
            view.currentItem?.forceActiveFocus(reason)
        } else
            view.forceActiveFocus(reason)
    }
    function positionContentAtBeginning() { view.positionViewAtBeginning() }
    FontLoader { id: inter; source: root.assetBase + "InterVariable.ttf" }
    GridView {
        id: view
        anchors.fill: parent
        leftMargin: 24 * root.uiScale
        rightMargin: 24 * root.uiScale
        topMargin: 32 * root.uiScale
        bottomMargin: 32 * root.uiScale
        model: root.entries
        reuseItems: true
        focus: true
        clip: true
        readonly property real availableWidth: Math.max(1, width - leftMargin - rightMargin)
        readonly property bool wideCards: root.kind === "video" || root.kind === "media"
        readonly property real standardCellWidth: (wideCards ? 340 : 220) * root.uiScale
        readonly property int columns: root.gridMode ? Math.max(1, Math.floor(availableWidth / standardCellWidth)) : 1
        cellWidth: root.gridMode ? Math.min(standardCellWidth, availableWidth) : availableWidth
        cellHeight: root.gridMode ? (wideCards ? 260 : 288) * root.uiScale : 44 * root.uiScale
        keyNavigationEnabled: true
        highlightFollowsCurrentItem: true
        C.ScrollBar.vertical: C.ScrollBar {}
        onCurrentIndexChanged: {
            if (activeFocus)
                currentItem?.forceActiveFocus(Qt.TabFocusReason)
        }
        delegate: JuxtopposedHomeCard {
            required property var model
            required property int index
            readonly property var row: model.modelData ?? model
            objectName: "homeCollectionItem" + index
            entry: row
            detailText: root.showLocation ? String(row?.url || "") : ""
            kind: root.kind === "media" ? (row?.isVideo ? "video" : "audio") : root.kind
            uiScale: root.uiScale
            fontFamily: inter.name
            iconFontFamily: root.iconFontFamily
            fallbackIcon: root.kind === "media" && audio ? root.audioFallbackIcon : root.fallbackIcon
            fallbackArtwork: root.fallbackArtwork
            surfaceColor: root.surfaceColor
            gridMode: root.gridMode
            width: root.gridMode ? Math.min(implicitWidth, view.cellWidth - 20 * root.uiScale) : view.cellWidth
            height: implicitHeight
            accentColor: root.accentColor
            Keys.forwardTo: [view]
            onActiveFocusChanged: if (activeFocus) view.currentIndex = index
            onClicked: root.activated(row, kind, focusReason)
        }
    }
    Column {
        anchors.centerIn: parent
        width: Math.max(0, parent.width - 48 * root.uiScale)
        spacing: 12 * root.uiScale
        visible: view.count === 0 || root.errorText.length > 0
        C.Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: root.errorText || (root.loading ? qsTr("Loading…") : root.emptyText)
            wrapMode: Text.WordWrap
            font.family: inter.name
            font.pixelSize: 16 * root.uiScale
            color: JuxtopposedPalette.color("#969696")
        }
        C.Button {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: root.errorText.length > 0
            text: qsTr("Retry")
            onClicked: root.retryRequested()
        }
    }
}
