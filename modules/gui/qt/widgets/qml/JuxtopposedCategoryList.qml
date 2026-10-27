/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * Visual design by Juxtopposed: https://www.youtube.com/@juxtopposed
 * Code licensed under GPLv2 or later; artwork under CC BY 4.0.
 *****************************************************************************/
import QtQuick
import VLC.Style
import QtQuick.Controls.Basic as C

C.Pane {
    id: root
    property real uiScale: 1
    property url assetBase: "qrc:/juxtopposed/"
    property string iconFontFamily: ""
    property string addIcon: "+"
    property string addIconSource: "music-add-97a9d.svg"
    property string fallbackIconSource: ""
    property bool fallbackIconMirrored: false
    property var routes: []
    property bool addAvailable: true
    property bool allSongsVisible: false
    property string accessibleName: qsTr("Music categories")
    property string emptyText: ""
    signal addRequested()
    signal routeRequested(var uri, int reason)
    signal entryRequested(var entry, int reason)
    signal allSongsRequested(int reason)

    padding: 0
    implicitWidth: 380 * uiScale
    implicitHeight: (categoryButtons.count + 1 + (allSongsVisible ? 1 : 0)) * 42 * uiScale
    Accessible.name: accessibleName
    background: null
    FontLoader { id: inter; source: root.assetBase + "InterVariable.ttf" }

    function setCurrentItemFocus(reason) {
        if (addButton.enabled)
            addButton.forceActiveFocus(reason)
        else if (categoryButtons.count > 0)
            categoryButtons.itemAt(0)?.forceActiveFocus(reason)
        else if (allSongsVisible)
            allSongsButton.forceActiveFocus(reason)
        else
            categoryFlickable.forceActiveFocus(reason)
    }
    function positionContentAtBeginning() { categoryFlickable.contentY = 0 }
    function ensureVisible(button) {
        const top = button.y
        if (top < categoryFlickable.contentY)
            categoryFlickable.contentY = top
        else if (top + button.height > categoryFlickable.contentY + categoryFlickable.height)
            categoryFlickable.contentY = top + button.height - categoryFlickable.height
    }
    component CategoryButton: C.ToolButton {
        id: button
        required property string label
        required property string glyph
        property string iconSource: ""
        property bool iconMirrored: false
        width: parent.width
        height: 42 * root.uiScale
        padding: 0
        focusPolicy: Qt.StrongFocus
        Accessible.name: label
        Keys.onReturnPressed: clicked()
        Keys.onEnterPressed: clicked()
        onActiveFocusChanged: if (activeFocus) root.ensureVisible(button)
        background: Rectangle {
            radius: 4 * root.uiScale
            color: button.down ? JuxtopposedPalette.color("#303030") : button.hovered ? JuxtopposedPalette.color("#202020") : "transparent"
            border.width: button.visualFocus ? 2 * root.uiScale : 0
            border.color: JuxtopposedPalette.color("#D37918")
        }
        contentItem: Item {
            opacity: button.enabled ? 1 : 0.35
            Item {
                id: icon
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                width: 32 * root.uiScale
                height: 32 * root.uiScale
                Image {
                    objectName: "musicCategoryArtwork"
                    anchors.centerIn: parent
                    width: 24 * root.uiScale
                    height: 24 * root.uiScale
                    source: button.iconSource ? root.assetBase + button.iconSource : ""
                    mirror: button.iconMirrored
                    visible: button.iconSource.length > 0
                    fillMode: Image.PreserveAspectFit
                    Accessible.ignored: true
                }
                C.Label {
                    anchors.centerIn: parent
                    visible: !button.iconSource
                    text: button.glyph
                    font.family: root.iconFontFamily
                    font.pixelSize: 24 * root.uiScale
                    color: JuxtopposedPalette.color("#969696")
                    Accessible.ignored: true
                }
            }
            C.Label {
                anchors.left: icon.right
                anchors.leftMargin: 10 * root.uiScale
                anchors.right: parent.right
                anchors.rightMargin: 12 * root.uiScale
                anchors.verticalCenter: parent.verticalCenter
                text: button.label
                font.family: inter.name
                font.pixelSize: 14 * root.uiScale
                color: JuxtopposedPalette.color(button.hovered || button.visualFocus ? "#969696" : "#6B6B6B")
                elide: Text.ElideRight
                Accessible.ignored: true
            }
        }
    }
    Flickable {
        id: categoryFlickable
        objectName: "musicCategoryViewport"
        anchors.fill: parent
        clip: true
        contentWidth: width
        contentHeight: categories.height
        boundsBehavior: Flickable.StopAtBounds
        C.ScrollBar.vertical: C.ScrollBar {}
        Column {
            id: categories
            width: parent.width
            CategoryButton {
                id: addButton
                objectName: "musicCategoryAdd"
                label: qsTr("Add")
                glyph: root.addIcon
                iconSource: root.addIconSource
                enabled: root.addAvailable
                onClicked: root.addRequested()
                Keys.onDownPressed: {
                    if (categoryButtons.count > 0)
                        categoryButtons.itemAt(0)?.forceActiveFocus(Qt.TabFocusReason)
                    else if (allSongsButton.visible)
                        allSongsButton.forceActiveFocus(Qt.TabFocusReason)
                }
            }
            Repeater {
                id: categoryButtons
                model: root.routes
                delegate: CategoryButton {
                    required property var model
                    required property int index
                    readonly property var entry: model.modelData ?? model
                    objectName: "musicCategory" + (entry.key ?? index)
                    label: entry.text ?? entry.title ?? ""
                    glyph: entry.icon ?? ""
                    iconSource: entry.iconSource ?? root.fallbackIconSource
                    iconMirrored: entry.iconMirrored ?? root.fallbackIconMirrored
                    enabled: entry.enabled ?? true
                    onClicked: {
                        if (entry.uri)
                            root.routeRequested(entry.uri, focusReason)
                        root.entryRequested(entry, focusReason)
                    }
                    Keys.onUpPressed: {
                        if (index > 0)
                            categoryButtons.itemAt(index - 1).forceActiveFocus(Qt.BacktabFocusReason)
                        else if (addButton.enabled)
                            addButton.forceActiveFocus(Qt.BacktabFocusReason)
                    }
                    Keys.onDownPressed: {
                        if (index + 1 < categoryButtons.count)
                            categoryButtons.itemAt(index + 1)?.forceActiveFocus(Qt.TabFocusReason)
                        else if (allSongsButton.visible)
                            allSongsButton.forceActiveFocus(Qt.TabFocusReason)
                    }
                }
            }
            CategoryButton {
                id: allSongsButton
                objectName: "musicAllSongs"
                visible: root.allSongsVisible
                label: qsTr("All songs")
                glyph: ""
                iconSource: "music-songs-018cc.svg"
                iconMirrored: true
                onClicked: root.allSongsRequested(focusReason)
                Keys.onUpPressed: {
                    if (categoryButtons.count > 0)
                        categoryButtons.itemAt(categoryButtons.count - 1)?.forceActiveFocus(Qt.BacktabFocusReason)
                    else if (addButton.enabled)
                        addButton.forceActiveFocus(Qt.BacktabFocusReason)
                }
            }
            C.Label {
                width: parent.width
                padding: 8 * root.uiScale
                visible: categoryButtons.count === 0 && root.emptyText.length > 0
                text: root.emptyText
                wrapMode: Text.WordWrap
                font.family: inter.name
                font.pixelSize: 14 * root.uiScale
                color: JuxtopposedPalette.color("#969696")
            }
        }
    }
}
