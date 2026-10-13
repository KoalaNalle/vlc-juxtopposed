/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * Visual design by Juxtopposed: https://www.youtube.com/@juxtopposed
 * Code licensed under GPLv2 or later; artwork under CC BY 4.0.
 *****************************************************************************/
import QtQuick
import QtQuick.Controls.Basic as C

C.Pane {
    id: root
    property real uiScale: 1
    property url assetBase: "qrc:/juxtopposed/"
    property string iconFontFamily: ""
    property string addIcon: "+"
    property var routes: []
    property bool addAvailable: true
    signal addRequested()
    signal routeRequested(var uri, int reason)

    padding: 0
    implicitWidth: 380 * uiScale
    implicitHeight: (routes.length + 1) * 44 * uiScale
    Accessible.name: qsTr("Music categories")
    background: Rectangle { color: "#111111" }
    FontLoader { id: inter; source: root.assetBase + "InterVariable.ttf" }

    function setCurrentItemFocus(reason) {
        if (addButton.enabled)
            addButton.forceActiveFocus(reason)
        else
            categoryButtons.itemAt(0)?.forceActiveFocus(reason)
    }
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
        width: parent.width
        height: 44 * root.uiScale
        padding: 0
        focusPolicy: Qt.StrongFocus
        Accessible.name: label
        Keys.onReturnPressed: clicked()
        Keys.onEnterPressed: clicked()
        onActiveFocusChanged: if (activeFocus) root.ensureVisible(button)
        background: Rectangle {
            radius: 4 * root.uiScale
            color: button.down ? "#303030" : button.hovered ? "#202020" : "transparent"
            border.width: button.visualFocus ? 2 * root.uiScale : 0
            border.color: "#D37918"
        }
        contentItem: Item {
            opacity: button.enabled ? 1 : 0.35
            C.Label {
                id: icon
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                width: 32 * root.uiScale
                text: button.glyph
                horizontalAlignment: Text.AlignHCenter
                font.family: root.iconFontFamily
                font.pixelSize: 24 * root.uiScale
                color: "#969696"
                Accessible.ignored: true
            }
            C.Label {
                anchors.left: icon.right
                anchors.leftMargin: 12 * root.uiScale
                anchors.right: parent.right
                anchors.rightMargin: 12 * root.uiScale
                anchors.verticalCenter: parent.verticalCenter
                text: button.label
                font.family: inter.name
                font.pixelSize: 14 * root.uiScale
                color: "#969696"
                elide: Text.ElideRight
                Accessible.ignored: true
            }
        }
    }
    Flickable {
        id: categoryFlickable
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
                enabled: root.addAvailable
                onClicked: root.addRequested()
                Keys.onDownPressed: categoryButtons.itemAt(0)?.forceActiveFocus(Qt.TabFocusReason)
            }
            Repeater {
                id: categoryButtons
                model: root.routes
                delegate: CategoryButton {
                    required property var modelData
                    required property int index
                    objectName: "musicCategory" + modelData.key
                    label: modelData.text
                    glyph: modelData.icon
                    enabled: modelData.enabled ?? true
                    onClicked: root.routeRequested(modelData.uri, focusReason)
                    Keys.onUpPressed: {
                        if (index > 0)
                            categoryButtons.itemAt(index - 1).forceActiveFocus(Qt.BacktabFocusReason)
                        else if (addButton.enabled)
                            addButton.forceActiveFocus(Qt.BacktabFocusReason)
                    }
                    Keys.onDownPressed: categoryButtons.itemAt(index + 1)?.forceActiveFocus(Qt.TabFocusReason)
                }
            }
        }
    }
}
