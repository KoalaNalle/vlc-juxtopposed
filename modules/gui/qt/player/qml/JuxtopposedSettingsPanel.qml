// Copyright (C) 2026 VLC authors and VideoLAN. GPLv2 or later.
// Visual design: Juxtopposed; original artwork: CC BY 4.0.
import QtQuick
import VLC.Style
import QtQuick.Controls.Basic as C
import QtQuick.Layouts

C.Pane {
    id: root

    property url assetBase: "qrc:/juxtopposed/"
    property Component audioContent
    readonly property var contentComponents: [playbackContent, videoContent, audioContent, subtitlesContent]
    property int currentTab: 0
    property string iconFontFamily: ""
    property alias loadedPage: pageLoader.item
    property Component playbackContent
    property Component subtitlesContent
    property var tabIcons: ["", "", "", ""]
    property real uiScale: 1
    property Component videoContent

    signal closeRequested
    signal tabRequested(int index)

    function focusContent(reason) {
        if (pageLoader.item?.setCurrentItemFocus)
            pageLoader.item.setCurrentItemFocus(reason);
        else
            pageLoader.item?.forceActiveFocus(reason);
    }
    function selectTab(index, reason) {
        if (index < 0 || index >= 4)
            return;
        if (currentTab !== index)
            tabRequested(index);
        currentTab = index;
        Qt.callLater(focusContent, reason);
    }

    Accessible.name: qsTr("Player settings")
    implicitHeight: 600 * uiScale
    implicitWidth: 400 * uiScale
    padding: 0

    background: Rectangle {
        color: JuxtopposedPalette.color("#111111")
        radius: 8 * root.uiScale
    }

    Keys.onEscapePressed: closeRequested()

    FontLoader {
        id: inter

        source: root.assetBase + "InterVariable.ttf"
    }
    Row {
        id: tabs

        height: 52 * root.uiScale
        width: parent.width

        Repeater {
            id: tabButtons

            model: [qsTr("Playback"), qsTr("Video"), qsTr("Audio"), qsTr("Subtitles")]

            C.ToolButton {
                id: button

                required property int index
                required property string modelData

                Accessible.name: modelData
                C.ToolTip.text: modelData
                C.ToolTip.visible: hovered || visualFocus
                checked: root.currentTab === index
                height: tabs.height
                objectName: "playerSettingsTab" + index
                padding: 0
                width: tabs.width / 4

                background: Rectangle {
                    border.color: JuxtopposedPalette.color("#D37918")
                    border.width: button.visualFocus ? root.uiScale : 0
                    color: button.hovered ? JuxtopposedPalette.color("#202020") : "transparent"

                    Rectangle {
                        anchors.bottom: parent.bottom
                        color: button.checked ? JuxtopposedPalette.color("#D37918") : "transparent"
                        height: 2 * root.uiScale
                        width: parent.width
                    }
                }
                contentItem: Item {
                    Row {
                        anchors.centerIn: parent
                        spacing: 4 * root.uiScale

                        C.Label {
                            Accessible.ignored: true
                            color: button.checked ? JuxtopposedPalette.color("#D37918") : JuxtopposedPalette.color("#6B6B6B")
                            font.family: root.iconFontFamily
                            font.pixelSize: 20 * root.uiScale
                            horizontalAlignment: Text.AlignHCenter
                            text: root.tabIcons[button.index] ?? ""
                            width: 20 * root.uiScale
                        }
                        C.Label {
                            color: button.checked ? JuxtopposedPalette.color("#969696") : JuxtopposedPalette.color("#6B6B6B")
                            elide: Text.ElideRight
                            font.family: inter.name
                            font.pixelSize: 14 * root.uiScale
                            horizontalAlignment: Text.AlignHCenter
                            text: button.modelData
                            visible: root.width >= 360 * root.uiScale
                            width: Math.min(implicitWidth, Math.max(0, button.width - 28 * root.uiScale))
                        }
                    }
                }

                Keys.onDownPressed: root.focusContent(Qt.TabFocusReason)
                Keys.onEnterPressed: clicked()
                Keys.onLeftPressed: tabButtons.itemAt((index + 3) % 4).forceActiveFocus(Qt.BacktabFocusReason)
                Keys.onReturnPressed: clicked()
                Keys.onRightPressed: tabButtons.itemAt((index + 1) % 4).forceActiveFocus(Qt.TabFocusReason)
                onClicked: root.selectTab(index, button.focusReason)
            }
        }
    }
    Loader {
        id: pageLoader

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.margins: 20 * root.uiScale
        anchors.right: parent.right
        anchors.top: tabs.bottom
        focus: true
        sourceComponent: root.contentComponents[root.currentTab] ?? null
    }
}
