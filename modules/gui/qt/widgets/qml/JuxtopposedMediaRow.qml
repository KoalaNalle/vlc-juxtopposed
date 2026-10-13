/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * Visual design by Juxtopposed: https://www.youtube.com/@juxtopposed
 * Code licensed under GPLv2 or later; artwork under CC BY 4.0.
 *****************************************************************************/
import QtQuick
import QtQuick.Controls.Basic as C

C.Control {
    id: root
    property real uiScale: 1
    property url assetBase: "qrc:/juxtopposed/"
    property string title: ""
    property string durationText: ""
    property url artwork
    property url fallbackArtwork
    property real progress: 0
    property bool current: false
    property bool selected: false
    property bool showDuration: false
    signal selectionRequested(int modifiers)
    signal activationRequested(bool keyboard)
    signal dragRequested(bool active, int modifiers)
    signal contextRequested(point position)

    implicitHeight: 44 * uiScale
    implicitWidth: 380 * uiScale
    padding: 0
    focusPolicy: Qt.StrongFocus
    hoverEnabled: true
    Accessible.role: Accessible.Cell
    Accessible.name: title
    Accessible.selected: selected
    Accessible.description: durationText
    Accessible.onPressAction: activationRequested(false)
    Keys.onMenuPressed: contextRequested(mapToGlobal(0, height))
    Keys.onReturnPressed: activationRequested(true)
    Keys.onEnterPressed: activationRequested(true)
    Keys.onSpacePressed: activationRequested(true)

    Item {
        anchors.fill: parent
        z: 1
        TapHandler {
            id: tap
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            gesturePolicy: TapHandler.ReleaseWithinBounds
            onTapped: (eventPoint, button) => {
                root.forceActiveFocus(Qt.MouseFocusReason)
                if (button === Qt.RightButton)
                    root.contextRequested(eventPoint.globalPosition)
                else
                    root.selectionRequested(point.modifiers)
            }
            onDoubleTapped: (eventPoint, button) => {
                if (button === Qt.LeftButton)
                    root.activationRequested(false)
            }
        }
        DragHandler {
            target: null
            acceptedDevices: PointerDevice.AllDevices & ~PointerDevice.TouchScreen
            grabPermissions: PointerHandler.CanTakeOverFromHandlersOfDifferentType | PointerHandler.ApprovesTakeOverByAnything
            onActiveChanged: root.dragRequested(active, centroid.modifiers)
        }
    }

    FontLoader { id: inter; source: root.assetBase + "InterVariable.ttf" }
    background: Rectangle {
        color: root.selected ? "#292929" : root.hovered ? "#202020" : "transparent"
        radius: 4 * root.uiScale
        border.width: root.visualFocus ? 2 * root.uiScale : 0
        border.color: "#D37918"
    }
    contentItem: Item {
        Image {
            id: thumbnail
            objectName: "libraryRowArtwork"
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            width: 32 * root.uiScale
            height: width
            source: root.artwork
            sourceSize: Qt.size(width, height)
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            Accessible.ignored: true
            Image {
                anchors.fill: parent
                visible: !thumbnail.source.toString().length || thumbnail.status === Image.Error
                source: root.fallbackArtwork
                fillMode: Image.PreserveAspectCrop
                Accessible.ignored: true
            }
            Rectangle {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                width: parent.width * Math.min(1, Math.max(0, root.progress))
                height: 2 * root.uiScale
                color: "#D37918"
                visible: root.progress > 0
            }
        }
        C.Label {
            objectName: "libraryRowTitle"
            anchors.left: thumbnail.right
            anchors.leftMargin: 12 * root.uiScale
            anchors.right: root.showDuration ? duration.left : parent.right
            anchors.rightMargin: 16 * root.uiScale
            anchors.verticalCenter: parent.verticalCenter
            text: root.title
            elide: Text.ElideRight
            font.family: inter.name
            font.pixelSize: 14 * root.uiScale
            color: root.current ? "#D37918" : "#969696"
            Accessible.ignored: true
        }
        C.Label {
            id: duration
            anchors.right: parent.right
            anchors.rightMargin: 8 * root.uiScale
            anchors.verticalCenter: parent.verticalCenter
            visible: root.showDuration
            text: root.durationText
            color: "#6B6B6B"
            font.family: inter.name
            font.pixelSize: 14 * root.uiScale
            Accessible.ignored: true
        }
    }
}
