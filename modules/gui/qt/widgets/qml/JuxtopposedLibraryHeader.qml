/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * Visual design by Juxtopposed: https://www.youtube.com/@juxtopposed
 * Code licensed under GPLv2 or later; artwork under CC BY 4.0.
 *****************************************************************************/
import QtQuick
import VLC.Style
import QtQuick.Controls.Basic as C
import QtQuick.Layouts

C.Pane {
    id: root
    property real uiScale: 1
    property url assetBase: "qrc:/juxtopposed/"
    property string iconFontFamily: ""
    property string backIcon: ""
    property string viewIcon: ""
    property string searchIcon: ""
    property string moreIcon: ""
    property var breadcrumbs: []
    property int itemCount: 0
    property bool countAvailable: false
    property bool canGoBack: false
    property bool canGoForward: false
    property bool sortAvailable: false
    property bool viewAvailable: false
    property bool searchAvailable: false
    property string searchPattern: ""
    property bool searchExpanded: false
    readonly property bool compact: width < 760 * uiScale
    property color surfaceColor: JuxtopposedPalette.color("#111111")
    property color textColor: JuxtopposedPalette.color("#969696")
    property color accentColor: JuxtopposedPalette.color("#D37918")

    signal backRequested()
    signal forwardRequested()
    signal breadcrumbRequested(var uri)
    signal sortRequested(point position)
    signal viewRequested()
    signal searchRequested(string pattern)
    signal moreRequested(point position)
    signal contentFocusRequested()

    implicitHeight: 72 * uiScale
    padding: 0
    Accessible.name: qsTr("Library header")
    Keys.priority: Keys.AfterItem
    Keys.onDownPressed: contentFocusRequested()
    function showSearch() {
        if (!searchAvailable)
            return
        searchExpanded = true
        searchField.forceActiveFocus(Qt.ShortcutFocusReason)
    }
    function setCurrentItemFocus(reason) {
        if (searchExpanded && searchField.visible)
            searchField.forceActiveFocus(reason)
        else if (root.canGoBack)
            backButton.forceActiveFocus(reason)
        else if (root.breadcrumbs.length > 0)
            breadcrumbButtons.itemAt(0).button.forceActiveFocus(reason)
        else
            moreButton.forceActiveFocus(reason)
    }

    FontLoader { id: inter; source: root.assetBase + "InterVariable.ttf" }
    background: Rectangle { color: root.surfaceColor }

    component HeaderButton: C.ToolButton {
        id: button
        required property string description
        property url source
        property string glyph: ""
        property bool mirrorGlyph: false
        Layout.preferredWidth: 32 * root.uiScale
        Layout.preferredHeight: 32 * root.uiScale
        padding: 0
        focusPolicy: Qt.StrongFocus
        Keys.onReturnPressed: clicked()
        Keys.onEnterPressed: clicked()
        Accessible.name: description
        C.ToolTip.visible: hovered || visualFocus
        C.ToolTip.text: description
        C.ToolTip.delay: 500
        background: Rectangle {
            radius: 4 * root.uiScale
            color: button.down ? JuxtopposedPalette.color("#404040") : button.hovered ? JuxtopposedPalette.color("#303030") : "transparent"
            border.width: button.visualFocus ? 2 * root.uiScale : 0
            border.color: root.accentColor
        }
        contentItem: Item {
            opacity: button.enabled ? 1 : 0.35
            Image {
                anchors.centerIn: parent
                visible: button.source.toString().length > 0
                source: button.source
                sourceSize: Qt.size(24 * root.uiScale, 24 * root.uiScale)
                width: 24 * root.uiScale
                height: width
                Accessible.ignored: true
            }
            C.Label {
                id: glyphLabel
                anchors.centerIn: parent
                visible: !button.source.toString().length
                text: button.glyph
                color: root.textColor
                font.family: root.iconFontFamily
                font.pixelSize: 24 * root.uiScale
                transform: Scale { xScale: button.mirrorGlyph ? -1 : 1; origin.x: glyphLabel.width / 2 }
                Accessible.ignored: true
            }
        }
    }

    RowLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 20 * root.uiScale
        anchors.rightMargin: 20 * root.uiScale
        anchors.verticalCenter: parent.verticalCenter
        spacing: 8 * root.uiScale

        HeaderButton {
            id: backButton
            objectName: "libraryBack"
            description: qsTr("Back")
            glyph: root.backIcon
            enabled: root.canGoBack
            visible: !root.compact || !root.searchExpanded
            onClicked: root.backRequested()
        }
        HeaderButton {
            objectName: "libraryForward"
            description: qsTr("Forward")
            glyph: root.backIcon
            mirrorGlyph: true
            enabled: root.canGoForward
            visible: !root.compact || !root.searchExpanded
            onClicked: root.forwardRequested()
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            visible: !root.compact || !root.searchExpanded
            spacing: 4 * root.uiScale
            Repeater {
                id: breadcrumbButtons
                model: root.breadcrumbs
                delegate: RowLayout {
                    required property var modelData
                    required property int index
                    property alias button: breadcrumbButton
                    Layout.fillWidth: index === root.breadcrumbs.length - 1
                    Layout.minimumWidth: 0
                    spacing: 4 * root.uiScale
                    C.Label {
                        visible: index > 0
                        text: "›"
                        font.family: inter.name
                        font.pixelSize: 16 * root.uiScale
                        color: JuxtopposedPalette.color("#6B6B6B")
                        Accessible.ignored: true
                    }
                    C.ToolButton {
                        id: breadcrumbButton
                        objectName: "libraryBreadcrumb" + index
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        padding: 0
                        focusPolicy: Qt.StrongFocus
                        Keys.onReturnPressed: clicked()
                        Keys.onEnterPressed: clicked()
                        Accessible.name: modelData.text
                        onClicked: root.breadcrumbRequested(modelData.uri)
                        background: Rectangle {
                            color: parent.hovered ? JuxtopposedPalette.color("#303030") : "transparent"
                            border.width: parent.visualFocus ? 2 * root.uiScale : 0
                            border.color: root.accentColor
                            radius: 4 * root.uiScale
                        }
                        contentItem: C.Label {
                            text: modelData.text
                            elide: Text.ElideRight
                            font.family: inter.name
                            font.pixelSize: 16 * root.uiScale
                            color: root.textColor
                        }
                    }
                }
            }
        }

        C.Label {
            visible: root.countAvailable && (!root.compact || !root.searchExpanded)
            text: root.itemCount === 1 ? qsTr("1 item") : qsTr("%1 items").arg(root.itemCount)
            color: JuxtopposedPalette.color("#6B6B6B")
            font.family: inter.name
            font.pixelSize: 14 * root.uiScale
        }
        HeaderButton {
            objectName: "librarySort"
            visible: root.sortAvailable && (!root.compact || !root.searchExpanded)
            description: qsTr("Sort")
            source: root.assetBase + "queue-sort.svg"
            onClicked: root.sortRequested(mapToGlobal(0, height))
        }
        HeaderButton {
            objectName: "libraryView"
            visible: root.viewAvailable && (!root.compact || !root.searchExpanded)
            description: qsTr("Change view")
            glyph: root.viewIcon
            onClicked: root.viewRequested()
        }
        C.TextField {
            id: searchField
            objectName: "librarySearch"
            Layout.fillWidth: root.compact
            Layout.preferredWidth: root.compact ? 160 * root.uiScale : 320 * root.uiScale
            Layout.minimumWidth: 80 * root.uiScale
            Layout.preferredHeight: 32 * root.uiScale
            visible: root.searchAvailable && (!root.compact || root.searchExpanded)
            placeholderText: qsTr("Search")
            Accessible.name: qsTr("Search library")
            text: root.searchPattern
            font.family: inter.name
            font.pixelSize: 14 * root.uiScale
            color: root.textColor
            selectionColor: root.accentColor
            selectByMouse: true
            leftPadding: 38 * root.uiScale
            rightPadding: 28 * root.uiScale
            onTextEdited: root.searchRequested(text)
            Keys.onEscapePressed: {
                root.searchExpanded = false
                root.contentFocusRequested()
            }
            Keys.onReturnPressed: root.contentFocusRequested()
            background: Rectangle { color: JuxtopposedPalette.color("#161616"); radius: 4 * root.uiScale }
            C.Label {
                anchors.left: parent.left
                anchors.leftMargin: 8 * root.uiScale
                anchors.verticalCenter: parent.verticalCenter
                text: root.searchIcon
                color: JuxtopposedPalette.color("#6B6B6B")
                font.family: root.iconFontFamily
                font.pixelSize: 24 * root.uiScale
                Accessible.ignored: true
            }
            C.ToolButton {
                objectName: "librarySearchClear"
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                width: 28 * root.uiScale
                height: parent.height
                text: "×"
                font.family: inter.name
                font.pixelSize: 18 * root.uiScale
                visible: searchField.text.length > 0
                Accessible.name: qsTr("Clear search")
                background: Rectangle {
                    color: "transparent"
                    border.width: parent.visualFocus ? root.uiScale : 0
                    border.color: root.accentColor
                    radius: 4 * root.uiScale
                }
                contentItem: C.Label {
                    text: "×"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: root.textColor
                    font.family: inter.name
                    font.pixelSize: 18 * root.uiScale
                }
                onClicked: {
                    root.searchRequested("")
                    searchField.forceActiveFocus(Qt.MouseFocusReason)
                }
            }
        }
        HeaderButton {
            objectName: "librarySearchToggle"
            visible: root.searchAvailable && root.compact
            description: qsTr("Search")
            glyph: root.searchIcon
            onClicked: {
                if (root.searchExpanded) {
                    root.searchExpanded = false
                    root.contentFocusRequested()
                } else
                    root.showSearch()
            }
        }
        HeaderButton {
            id: moreButton
            objectName: "libraryMore"
            description: qsTr("Menu")
            glyph: root.moreIcon
            onClicked: root.moreRequested(mapToGlobal(0, height))
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 20 * root.uiScale
        anchors.rightMargin: 20 * root.uiScale
        height: root.uiScale
        color: JuxtopposedPalette.color("#252525")
    }
}
