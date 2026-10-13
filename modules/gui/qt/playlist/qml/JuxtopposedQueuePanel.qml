/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * Visual design by Juxtopposed: https://www.youtube.com/@juxtopposed
 * Original design: https://www.figma.com/community/file/1691465999134370980
 * Code licensed under GPLv2 or later; artwork under CC BY 4.0.
 *****************************************************************************/
import QtQuick
import QtQuick.Controls as C
import QtQuick.Layouts

// Presentation and action signals only. VLC supplies the queue/bookmark/
// chapter view; standalone previews can instead supply bodyComponent.
C.Control {
    id: root

    property real uiScale: 1
    property url assetBase: "qrc:/juxtopposed/"
    property string summary: ""
    property int activeTab: 0
    property bool sortEnabled: true
    property bool repeatEnabled: true
    property bool shuffleEnabled: true
    property bool removeEnabled: true
    property bool addEnabled: true
    property string addDescription: qsTr("Add files")
    property string removeDescription: qsTr("Remove selected items")
    property bool showSort: true
    property bool showRepeat: true
    property bool showShuffle: true
    property bool showRemove: true
    property bool showAdd: true
    property bool showToolbar: true
    property real toolbarContentHeight: 28
    property bool repeatActive: false
    property bool shuffleActive: false
    property string repeatDescription: repeatActive ? qsTr("Repeat all") : qsTr("Repeat off")
    property string shuffleDescription: shuffleActive ? qsTr("Shuffle on") : qsTr("Shuffle off")
    property color accentColor: "#D37918"
    property Component bodyComponent
    readonly property Item bodyContainer: body
    readonly property Item bodyItem: bodyLoader.item
    readonly property Item sortButton: sortButton
    readonly property Item chaptersButton: tabRepeater.itemAt(2)

    signal tabRequested(int index)
    signal sortRequested()
    signal repeatRequested()
    signal shuffleRequested()
    signal removeRequested()
    signal addRequested()

    implicitWidth: 400 * uiScale
    implicitHeight: 946 * uiScale
    padding: 0
    Accessible.role: Accessible.Pane
    Accessible.name: qsTr("Play Queue")

    FontLoader {
        id: inter
        source: root.assetBase + "InterVariable.ttf"
    }

    background: Rectangle {
        color: Qt.rgba(14 / 255, 14 / 255, 14 / 255, 0.95)
        radius: 10 * root.uiScale
    }

    contentItem: Item {
        Item {
            id: tabStrip
            objectName: "queueTabs"
            anchors.left: parent.left
            anchors.right: parent.right
            height: 50 * root.uiScale

            // Match the enclosing frame's ten-pixel top corners while keeping
            // the tab strip's own two-pixel corners along its lower edge.
            Rectangle {
                anchors.fill: parent
                color: "#090909"
                radius: 10 * root.uiScale
            }
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 40 * root.uiScale
                color: "#090909"
                radius: 2 * root.uiScale
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 6 * root.uiScale
                anchors.rightMargin: 6 * root.uiScale
                spacing: root.uiScale

                Repeater {
                    id: tabRepeater
                    model: [
                        { "label": qsTr("Queue"), "icon": root.activeTab === 0 ? "queue-tab-active.svg" : "queue-tab-inactive.svg" },
                        { "label": qsTr("Bookmarks"), "icon": root.activeTab === 1 ? "queue-bookmarks-active.svg" : "queue-bookmarks.svg" },
                        { "label": qsTr("Chapters"), "icon": root.activeTab === 2 ? "queue-chapters-active.svg" : "queue-chapters.svg" }
                    ]

                    C.ToolButton {
                        id: tabButton
                        required property int index
                        required property var modelData
                        objectName: "queueTab" + index
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                        Layout.preferredHeight: 50 * root.uiScale
                        padding: 0
                        activeFocusOnTab: true
                        checked: root.activeTab === index
                        Accessible.role: Accessible.PageTab
                        Accessible.name: modelData.label
                        Accessible.selected: checked
                        background: Rectangle {
                            color: "transparent"
                            border.width: tabButton.visualFocus ? root.uiScale : 0
                            border.color: root.accentColor
                            radius: 2 * root.uiScale
                        }
                        contentItem: Item {
                            RowLayout {
                                anchors.horizontalCenter: parent.horizontalCenter
                                y: 14 * root.uiScale
                                height: 20 * root.uiScale
                                spacing: 8 * root.uiScale

                                Item {
                                    Layout.preferredWidth: 20 * root.uiScale
                                    Layout.preferredHeight: 20 * root.uiScale
                                    Image {
                                        objectName: "queueTabIcon" + tabButton.index
                                        anchors.centerIn: parent
                                        source: root.assetBase + tabButton.modelData.icon
                                        scale: root.uiScale
                                        Accessible.ignored: true
                                    }
                                }

                                Text {
                                    objectName: "queueTabCaption" + tabButton.index
                                    text: tabButton.modelData.label
                                    font.family: inter.name
                                    font.pixelSize: 12 * root.uiScale
                                    font.weight: Font.Normal
                                    color: tabButton.checked ? "#8F8F8F" : "#6B6B6B"
                                    textFormat: Text.PlainText
                                    Accessible.ignored: true
                                }
                            }
                        }
                        Rectangle {
                            objectName: "queueTabUnderline" + tabButton.index
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            anchors.leftMargin: 8 * root.uiScale
                            anchors.rightMargin: 8 * root.uiScale
                            height: 2 * root.uiScale
                            color: root.accentColor
                            visible: tabButton.checked
                        }
                        onClicked: root.tabRequested(index)
                    }
                }
            }
        }

        Item {
            id: toolbar
            visible: root.showToolbar
            objectName: "queueToolbar"
            anchors.left: parent.left
            anchors.right: parent.right
            y: 51 * root.uiScale
            height: (40 + root.toolbarContentHeight) * root.uiScale

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 28 * root.uiScale
                anchors.rightMargin: 20 * root.uiScale
                anchors.topMargin: 20 * root.uiScale
                anchors.bottomMargin: 20 * root.uiScale
                spacing: 12 * root.uiScale

                Text {
                    objectName: "queueSummary"
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: root.summary
                    font.family: inter.name
                    font.pixelSize: 14 * root.uiScale
                    font.weight: Font.Normal
                    color: "#6B6B6B"
                    textFormat: Text.PlainText
                    elide: Text.ElideRight
                    Accessible.ignored: true
                }

                RowLayout {
                    objectName: "queueActions"
                    Layout.preferredWidth: implicitWidth
                    Layout.preferredHeight: root.toolbarContentHeight * root.uiScale
                    spacing: 12 * root.uiScale

                    QueueIconButton {
                        id: sortButton
                        objectName: "queueSort"
                        visible: root.showSort
                        iconFile: "queue-sort.svg"
                        description: qsTr("Sort queue")
                        enabled: root.sortEnabled
                        onClicked: root.sortRequested()
                    }
                    QueueIconButton {
                        objectName: "queueRepeat"
                        visible: root.showRepeat
                        iconFile: "queue-repeat.svg"
                        description: root.repeatDescription
                        Layout.preferredWidth: 30 * root.uiScale
                        Layout.preferredHeight: 28 * root.uiScale
                        checked: root.repeatActive
                        Accessible.checkable: true
                        Accessible.checked: checked
                        enabled: root.repeatEnabled
                        onClicked: root.repeatRequested()
                    }
                    QueueIconButton {
                        objectName: "queueShuffle"
                        visible: root.showShuffle
                        iconFile: "queue-shuffle.svg"
                        description: root.shuffleDescription
                        Layout.preferredWidth: 30 * root.uiScale
                        Layout.preferredHeight: 28 * root.uiScale
                        checked: root.shuffleActive
                        Accessible.checkable: true
                        Accessible.checked: checked
                        enabled: root.shuffleEnabled
                        onClicked: root.shuffleRequested()
                    }
                    QueueIconButton {
                        objectName: "queueRemove"
                        visible: root.showRemove
                        iconFile: "queue-remove.svg"
                        description: root.removeDescription
                        enabled: root.removeEnabled
                        onClicked: root.removeRequested()
                    }
                    QueueIconButton {
                        objectName: "queueAdd"
                        visible: root.showAdd
                        iconFile: "queue-add.svg"
                        description: root.addDescription
                        enabled: root.addEnabled
                        onClicked: root.addRequested()
                    }
                }
            }
        }

        Item {
            id: body
            objectName: "queueBody"
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.leftMargin: 20 * root.uiScale
            anchors.rightMargin: 20 * root.uiScale
            anchors.topMargin: (root.showToolbar ? 92 + root.toolbarContentHeight : 71) * root.uiScale

            Loader {
                id: bodyLoader
                anchors.fill: parent
                sourceComponent: root.bodyComponent
            }
        }
    }

    component QueueIconButton: C.ToolButton {
        id: button
        required property string iconFile
        required property string description
        Layout.preferredWidth: 24 * root.uiScale
        Layout.preferredHeight: 24 * root.uiScale
        padding: 0
        activeFocusOnTab: true
        Accessible.name: description
        C.ToolTip.visible: hovered || visualFocus
        C.ToolTip.text: description
        C.ToolTip.delay: 500
        background: Rectangle {
            // Active-mode feedback uses native interaction styling; the Figma
            // references only cover these actions in their unchecked state.
            color: button.checked ? Qt.rgba(root.accentColor.r, root.accentColor.g, root.accentColor.b, 0.18)
                                  : "transparent"
            radius: 2 * root.uiScale
            border.width: button.visualFocus || button.checked ? root.uiScale : 0
            border.color: root.accentColor
        }
        contentItem: Item {
            Image {
                objectName: button.objectName + "Icon"
                anchors.centerIn: parent
                source: root.assetBase + button.iconFile
                scale: root.uiScale
                opacity: button.enabled ? 1 : 0.35
                Accessible.ignored: true
            }
        }
    }
}
