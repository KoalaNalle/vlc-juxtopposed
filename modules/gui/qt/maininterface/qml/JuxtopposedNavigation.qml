/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * Visual design by Juxtopposed: https://www.youtube.com/@juxtopposed
 * Code licensed under GPLv2 or later; artwork under CC BY 4.0.
 *****************************************************************************/
import QtQuick
import VLC.Style
import VLC.Util
import QtQuick.Controls.Basic as C
import QtQuick.Layouts
import VLC.Widgets as Widgets

C.Pane {
    id: root
    objectName: "juxtopposedNavigation"

    property real uiScale: 1
    property url assetBase: "qrc:/juxtopposed/"
    property string iconFontFamily: ""
    property var routes: []
    property var pinModel: null
    property var pinIcons: ({})
    property string addIcon: "+"
    property int focusedPinIndex: -1
    signal pinRequested(string key, int reason)
    signal pinMenuRequested(string key, point position)
    signal addRequested(point position)
    property string activeRoute: ""
    property bool expanded: false
    property bool libraryBusy: false
    property bool libraryAvailable: true
    property string preferencesIcon: ""
    property color surfaceColor: JuxtopposedPalette.color("#191919")
    property color outerColor: JuxtopposedPalette.color("#0E0E0E")
    property color activeTextColor: JuxtopposedPalette.color("#C6C6C6")
    property color textColor: JuxtopposedPalette.color("#8F8F8F")
    property color accentColor: JuxtopposedPalette.color("#D37918")

    signal routeRequested(var route, int reason)
    signal preferencesRequested()
    signal contentFocusRequested(int reason)

    implicitWidth: (expanded ? 217 : 50) * uiScale
    padding: (expanded ? 8 : 0) * uiScale
    Accessible.name: qsTr("Library navigation")
    Accessible.description: libraryAvailable ? "" : qsTr("The media library is unavailable. You can still open files or use Network.")

    Keys.priority: Keys.AfterItem
    Keys.onUpPressed: moveFocus(-1)
    Keys.onDownPressed: moveFocus(1)
    Keys.onRightPressed: contentFocusRequested(Qt.TabFocusReason)

    function focusPin(index, reason) {
        if (!pinModel || index < 0 || index >= pinModel.count) { pinList.footerItem?.addButton.forceActiveFocus(reason); return }
        pinList.positionViewAtIndex(index, ListView.Contain)
        pinList.forceLayout()
        const item = pinList.itemAtIndex(index)
        if (item) item.forceActiveFocus(reason)
    }
    function focusPinKey(key, reason) {
        for (let i = 0; pinModel && i < pinModel.count; ++i)
            if (pinModel.get(i).key === key) { focusPin(i, reason); return }
        pinList.footerItem?.addButton.forceActiveFocus(reason)
    }
    function moveFocus(direction) {
        const entries = []
        const routes = pinList.headerItem?.routeRepeater
        const addButton = pinList.footerItem?.addButton
        if (!routes || !addButton) return
        for (let i = 0; i < routes.count; ++i) {
            const button = routes.itemAt(i)
            if (button.enabled && button.visible) entries.push({button: button})
        }
        for (let i = 0; pinModel && i < pinModel.count; ++i) entries.push({pin: i})
        entries.push({button: addButton})
        const current = entries.findIndex(entry => entry.button ? entry.button.activeFocus
            : pinList.itemAtIndex(entry.pin)?.activeFocus)
        const target = entries[(current + direction + entries.length) % entries.length]
        const reason = direction > 0 ? Qt.TabFocusReason : Qt.BacktabFocusReason
        if (target.button) {
            if (target.button === addButton) pinList.positionViewAtEnd()
            else pinList.positionViewAtBeginning()
            target.button.forceActiveFocus(reason)
        } else focusPin(target.pin, reason)
    }

    FontLoader {
        id: inter
        source: root.assetBase + "InterVariable.ttf"
    }

    background: Rectangle {
        color: root.expanded ? root.outerColor : root.surfaceColor
    }

    contentItem: C.Pane {
        padding: 0
        leftPadding: (root.expanded ? 16 : 8) * root.uiScale
        rightPadding: leftPadding
        topPadding: (root.expanded ? 12 : 8) * root.uiScale
        bottomPadding: topPadding
        background: Rectangle {
            color: root.surfaceColor
            radius: root.expanded ? 6 * root.uiScale : 0
        }
        contentItem: ColumnLayout {
        spacing: 0

        ListView {
            id: pinList
            objectName: "juxtopposedSidebarPinList"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            reuseItems: true
            spacing: 14 * root.uiScale
            keyNavigationEnabled: false
            C.ScrollBar.vertical: C.ScrollBar {}
            model: root.pinModel
            header: Item {
                property alias routeRepeater: routeRepeater
                width: pinList.width
                height: routeColumn.implicitHeight + 14 * root.uiScale
                Column {
                id: routeColumn
                width: parent.width
                spacing: 14 * root.uiScale
                Repeater {
                    id: routeRepeater
                    model: root.routes

                    delegate: C.ToolButton {
                        id: routeButton
                        required property var modelData
                        required property int index

                        objectName: "juxtopposedNavigation" + modelData.key
                        width: pinList.width
                        height: 32 * root.uiScale
                        padding: 0
                        enabled: modelData.enabled !== false
                        checked: modelData.key === root.activeRoute
                        focusPolicy: Qt.StrongFocus
                        Accessible.name: modelData.text
                        Accessible.description: checked ? qsTr("Current section") : ""
                        C.ToolTip.visible: !root.expanded && (hovered || visualFocus)
                        C.ToolTip.text: modelData.text
                        C.ToolTip.delay: 500

                        onClicked: root.routeRequested(modelData.uri, focusReason)

                        background: Rectangle {
                            color: routeButton.down ? JuxtopposedPalette.color("#404040") : routeButton.hovered ? JuxtopposedPalette.color("#303030") : "transparent"
                            radius: 4 * root.uiScale
                            border.width: routeButton.visualFocus ? 2 * root.uiScale : 0
                            border.color: root.accentColor
                        }

                        contentItem: RowLayout {
                            spacing: 8 * root.uiScale
                            opacity: routeButton.enabled ? 1 : 0.35

                            Item {
                                Layout.preferredWidth: (root.expanded ? (routeButton.modelData.key === "video" ? 32 : 30) : 34) * root.uiScale
                                Layout.fillHeight: true
                                Image {
                                    objectName: "juxtopposedNavigationArt" + routeButton.modelData.key
                                    anchors.centerIn: parent
                                    width: 24; height: 24; scale: root.uiScale
                                    mirror: routeButton.modelData.mirror === true
                                    visible: !!routeButton.modelData.artwork
                                    source: visible ? SVGColorImage.colorize(root.assetBase + (routeButton.checked ? routeButton.modelData.artwork : (routeButton.modelData.inactiveArtwork || routeButton.modelData.artwork)))
                                        .any({"#8F8F8F": routeButton.checked ? root.accentColor : root.textColor,
                                              "#D37918": routeButton.checked ? root.accentColor : root.textColor}).uri() : ""
                                    Accessible.ignored: true
                                }
                                C.Label {
                                    anchors.centerIn: parent
                                    visible: !routeButton.modelData.artwork
                                    text: routeButton.modelData.icon
                                    font.family: root.iconFontFamily
                                    font.pixelSize: 24 * root.uiScale
                                    color: routeButton.checked ? root.accentColor : root.textColor
                                    Accessible.ignored: true
                                }
                            }

                            C.Label {
                                Layout.fillWidth: true
                                visible: root.expanded
                                text: routeButton.modelData.text
                                elide: Text.ElideRight
                                font.family: inter.name
                                font.pixelSize: 14 * root.uiScale
                                color: routeButton.checked ? root.activeTextColor : root.textColor
                                Accessible.ignored: true
                            }
                        }
                    }
                }
                }
            }
            delegate: C.ToolButton {
                id: pinButton
                required property string pinKey
                required property string kind
                required property string title
                required property url artwork
                required property bool available
                required property int index
                objectName: "juxtopposedSidebarPin" + index
                width: pinList.width
                height: 32 * root.uiScale
                padding: 0
                focusPolicy: Qt.StrongFocus
                Accessible.name: title
                Accessible.description: available ? qsTr("Pinned item") : qsTr("Unavailable pinned item. Use the menu to remove it.")
                C.ToolTip.visible: hovered || visualFocus
                C.ToolTip.text: available ? title : qsTr("%1 — unavailable").arg(title)
                C.ToolTip.delay: 500
                onClicked: root.pinRequested(pinKey, focusReason)
                onActiveFocusChanged: if (activeFocus) root.focusedPinIndex = index
                function menu() { root.pinMenuRequested(pinKey, mapToGlobal(0, height)) }
                Keys.onPressed: event => {
                    if (event.key === Qt.Key_Menu || (event.key === Qt.Key_F10 && (event.modifiers & Qt.ShiftModifier))) {
                        menu(); event.accepted = true
                    } else if ((event.modifiers & Qt.ControlModifier) && (event.key === Qt.Key_Up || event.key === Qt.Key_Down)) {
                        const destination = index + (event.key === Qt.Key_Up ? -1 : 1)
                        const key = pinKey
                        if (destination >= 0 && destination < root.pinModel.count && root.pinModel.move(key, destination))
                            Qt.callLater(() => root.focusPinKey(key, Qt.OtherFocusReason))
                        event.accepted = true
                    }
                }
                TapHandler {
                    acceptedButtons: Qt.RightButton
                    onTapped: eventPoint => {
                        pinButton.forceActiveFocus(Qt.MouseFocusReason)
                        root.pinMenuRequested(pinButton.pinKey, pinButton.mapToGlobal(eventPoint.position.x, eventPoint.position.y))
                    }
                }
                background: Rectangle {
                    color: pinButton.down ? JuxtopposedPalette.color("#404040") : pinButton.hovered ? JuxtopposedPalette.color("#303030") : "transparent"
                    radius: 4 * root.uiScale
                    border.width: pinButton.visualFocus ? 2 * root.uiScale : 0
                    border.color: root.accentColor
                }
                contentItem: RowLayout {
                    spacing: 8 * root.uiScale
                    opacity: pinButton.available ? 1 : 0.45
                    Item {
                        Layout.preferredWidth: (root.expanded ? 30 : 34) * root.uiScale
                        Layout.fillHeight: true
                        Widgets.ImageExt {
                            id: pinArt
                            anchors.centerIn: parent
                            width: 24 * root.uiScale; height: width
                            radius: 3 * root.uiScale
                            objectName: "juxtopposedSidebarPinArt" + pinButton.index
                            source: pinButton.artwork.toString().length ? pinButton.artwork
                                    : pinButton.kind === "folder" ? root.assetBase + "extensions-settings-cf6e1.svg" : ""
                            asynchronous: true
                            fillMode: Image.PreserveAspectCrop
                            visible: status === Image.Ready
                        }
                        C.Label {
                            anchors.centerIn: parent
                            visible: !pinArt.visible
                            text: root.pinIcons[pinButton.kind] || ""
                            font.family: root.iconFontFamily
                            font.pixelSize: 24 * root.uiScale
                            color: root.textColor
                            Accessible.ignored: true
                        }
                    }
                    C.Label {
                        Layout.fillWidth: true
                        visible: root.expanded
                        text: pinButton.title
                        elide: Text.ElideRight
                        font.family: inter.name
                        font.pixelSize: 14 * root.uiScale
                        color: root.textColor
                        Accessible.ignored: true
                    }
                }
            }
            footer: Column {
                property alias addButton: addButton
                width: pinList.width
                Item {
                    width: 1
                    height: (root.pinModel?.count > 0 ? 14 : 0) * root.uiScale
                }
                C.ToolButton {
                    id: addButton
                    objectName: "juxtopposedSidebarAdd"
                    width: pinList.width; height: 32 * root.uiScale
                    padding: 0
                    enabled: !root.pinModel?.canRetry
                    focusPolicy: Qt.StrongFocus
                    Accessible.name: qsTr("Add item")
                    C.ToolTip.visible: hovered || visualFocus
                    C.ToolTip.text: qsTr("Add item")
                    C.ToolTip.delay: 500
                    onClicked: root.addRequested(mapToGlobal(0, height))
                    background: Rectangle {
                        color: addButton.hovered ? JuxtopposedPalette.color("#303030") : "transparent"
                        radius: 4 * root.uiScale
                        border.width: addButton.visualFocus ? 2 * root.uiScale : 0
                        border.color: root.accentColor
                    }
                    contentItem: RowLayout {
                        spacing: 8 * root.uiScale
                        Item {
                            Layout.preferredWidth: (root.expanded ? 30 : 34) * root.uiScale
                            Layout.fillHeight: true
                            Image {
                                objectName: "juxtopposedSidebarAddArt"
                                anchors.centerIn: parent
                                width: 24; height: 24; scale: root.uiScale
                                source: SVGColorImage.colorize(root.assetBase + "sidebar-add-bfdda.svg")
                                    .any({"#8F8F8F": root.textColor}).uri()
                                Accessible.ignored: true
                            }
                        }
                        C.Label {
                            Layout.fillWidth: true
                            visible: root.expanded
                            text: qsTr("Add item")
                            elide: Text.ElideRight
                            font.family: inter.name
                            font.pixelSize: 14 * root.uiScale
                            color: root.textColor
                            Accessible.ignored: true
                        }
                    }
                }
                Column {
                    width: parent.width
                    visible: !!root.pinModel?.error
                    C.Label {
                        objectName: "juxtopposedSidebarError"
                        width: parent.width
                        padding: 4 * root.uiScale
                        text: root.expanded ? (root.pinModel?.error || "") : "!"
                        Accessible.name: root.pinModel?.error || ""
                        wrapMode: Text.WordWrap
                        color: root.textColor
                        font.pixelSize: 12 * root.uiScale
                        C.ToolTip.visible: errorHover.hovered
                        C.ToolTip.text: root.pinModel?.error || ""
                        HoverHandler { id: errorHover }
                    }
                    C.ToolButton {
                        objectName: "juxtopposedSidebarRetry"
                        width: parent.width
                        text: qsTr("Retry")
                        visible: !!root.pinModel?.canRetry
                        font.pixelSize: 11 * root.uiScale
                        onClicked: root.pinModel.retry()
                    }
                    C.ToolButton {
                        objectName: "juxtopposedSidebarDismiss"
                        width: parent.width
                        text: qsTr("Dismiss")
                        font.pixelSize: 11 * root.uiScale
                        onClicked: root.pinModel.dismissError()
                    }
                }
            }
            DropArea {
                anchors.fill: parent
                onDropped: drop => {
                    if (drop.hasUrls && root.pinModel?.pinUrls(drop.urls)) drop.acceptProposedAction()
                }
            }
        }


        C.Label {
            objectName: "juxtopposedLibraryUnavailable"
            Layout.fillWidth: true
            visible: root.expanded && !root.libraryAvailable
            text: qsTr("Media library unavailable")
            wrapMode: Text.WordWrap
            font.family: inter.name
            font.pixelSize: 12 * root.uiScale
            color: root.textColor
            horizontalAlignment: Text.AlignHCenter
            padding: 8 * root.uiScale
        }

        C.BusyIndicator {
            objectName: "juxtopposedLibraryScan"
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 28 * root.uiScale
            Layout.preferredHeight: 28 * root.uiScale
            visible: root.libraryBusy
            running: visible
            Accessible.name: qsTr("Scanning media library")
        }

        }
    }
}
