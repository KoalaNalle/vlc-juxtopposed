// SPDX-License-Identifier: GPL-2.0-or-later
// Original visual design and unmodified artwork: Juxtopposed, CC BY 4.0.
import QtQuick
import QtQuick.Controls.Basic as C
import QtQuick.Window
import VLC.MainInterface
import VLC.Menus as Menus
import VLC.Style
import VLC.Util
import VLC.Widgets as Widgets

FocusScope {
    id: root
    objectName: "juxtopposedTitlebar"
    readonly property real uiScale: VLCStyle.dp(1, VLCStyle.scale)
    property bool playerContext: false
    property bool useAcrylic: true
    property bool showWindowControls: MainCtx.clientSideDecoration && MainCtx.intfMainWindow.visibility !== Window.FullScreen
    property bool navigationVisible: false
    property string sourceTitle: qsTr("Home")
    property string mediaTitle: ""
    property Item _hoveredHelpButton: null
    readonly property string displayedTitle: mediaTitle || qsTr("VLC Media Player")
    readonly property alias menuButton: more
    readonly property alias navigationButton: navigation
    signal homeRequested()
    signal sourceRequested()
    signal navigationRequested()
    signal lockAutoHide(bool locked)
    signal dismissRequested()
    implicitHeight: 40 * uiScale
    Accessible.role: Accessible.ToolBar
    Accessible.name: qsTr("Title bar")
    FontLoader { id: inter; source: "qrc:/juxtopposed/InterVariable.ttf" }
    Widgets.AcrylicBackground {
        anchors.fill: parent
        enabled: root.useAcrylic
        tintColor: JuxtopposedPalette.color(root.playerContext ? "#1B1B1B" : "#191919")
        alternativeColor: tintColor
        opacity: root.playerContext ? 1 : 0.95
    }
    Widgets.CSDTitlebarTapNDrapHandler {
        anchors.fill: parent
        enabled: MainCtx.clientSideDecoration
    }
    function nativeButton(type) {
        for (const button of MainCtx.csdButtonModel.windowCSDButtons)
            if (button.type === type) return button
        return null
    }
    function updateWindowRects() {
        minimize.updateNativeRect(); maximize.updateNativeRect(); close.updateNativeRect()
    }
    onWidthChanged: Qt.callLater(updateWindowRects)
    onHeightChanged: Qt.callLater(updateWindowRects)
    onUiScaleChanged: Qt.callLater(updateWindowRects)
    onVisibleChanged: if (visible) Qt.callLater(updateWindowRects)
    Connections {
        target: MainCtx.intfMainWindow
        function onVisibilityChanged() { Qt.callLater(root.updateWindowRects) }
        function onWidthChanged() { Qt.callLater(root.updateWindowRects) }
        function onHeightChanged() { Qt.callLater(root.updateWindowRects) }
    }
    Connections {
        target: root
        function onDismissRequested() { menu.close() }
    }
    component StripButton: C.Button {
        id: button
        required property string key
        required property string description
        property string caption: ""
        property int artworkSize: 24
        property int artworkFrameSize: 24
        property var windowButton: null
        readonly property bool externalHover: windowButton?.showHovered ?? false
        objectName: "juxtopposedStrip" + key
        implicitWidth: 28 * root.uiScale
        implicitHeight: 28 * root.uiScale
        width: implicitWidth
        height: implicitHeight
        padding: 0
        leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
        hoverEnabled: true
        activeFocusOnTab: true
        Accessible.name: description
        // Keep title-strip help inside this window even at a monitor edge.
        // A native shared popup can otherwise open above/on the next monitor.
        C.ToolTip {
            objectName: "juxtopposedStripTooltip" + button.key
            parent: button
            popupType: C.Popup.Item
            x: (button.width - implicitWidth) / 2
            y: button.height + 4 * root.uiScale
            margins: 6 * root.uiScale
            visible: root.visible && (button.hovered
                || (button.visualFocus && !root._hoveredHelpButton?.hovered))
            text: button.description
            delay: 500
        }
        Navigation.parentItem: root
        onHoveredChanged: {
            if (hovered) root._hoveredHelpButton = button
            else if (root._hoveredHelpButton === button) root._hoveredHelpButton = null
            root.lockAutoHide(hovered)
        }
        onWidthChanged: Qt.callLater(updateNativeRect)
        onHeightChanged: Qt.callLater(updateNativeRect)
        onVisibleChanged: Qt.callLater(updateNativeRect)
        Component.onCompleted: Qt.callLater(updateNativeRect)
        function updateNativeRect() {
            // A hidden library/player instance must not overwrite the visible one's CSD rectangles.
            if (!windowButton || !root.visible) return
            const point = mapToItem(null, 0, 0)
            windowButton.rect = visible ? Qt.rect(point.x, point.y, width, height) : Qt.rect(0, 0, 0, 0)
        }
        background: Rectangle {
            radius: 2 * root.uiScale
            color: button.down || (button.windowButton?.externalPressed && button.externalHover)
                ? JuxtopposedPalette.color("#404040")
                : button.hovered || button.externalHover ? JuxtopposedPalette.color("#303030") : "transparent"
            border.width: button.visualFocus ? 2 * root.uiScale : 0
            border.color: JuxtopposedPalette.color("#D37918")
        }
        contentItem: Item {
            Item {
                id: iconFrame
                width: button.artworkFrameSize * root.uiScale; height: 24 * root.uiScale
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: button.caption ? undefined : parent.horizontalCenter
                Image {
                    objectName: "juxtopposedStripArt" + button.key
                    anchors.centerIn: parent
                    width: button.artworkSize; height: button.artworkSize; scale: root.uiScale
                    mirror: button.key !== "logo"
                    source: button.key === "logo" ? "qrc:/juxtopposed/titlebar-logo.svg"
                        : SVGColorImage.colorize("qrc:/juxtopposed/titlebar-" + button.key + ".svg")
                            .any({"#8F8F8F": JuxtopposedPalette.color("#8F8F8F")}).uri()
                    opacity: button.enabled ? 1 : 0.4
                    Accessible.ignored: true
                }
            }
            C.Label {
                anchors.left: iconFrame.right
                anchors.leftMargin: 4 * root.uiScale
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                visible: button.caption.length > 0
                text: button.caption
                font.family: inter.name
                font.pixelSize: 14 * root.uiScale
                color: JuxtopposedPalette.color("#8F8F8F")
                elide: Text.ElideRight
                Accessible.ignored: true
            }
        }
    }
    Row {
        id: left
        anchors.left: parent.left; anchors.leftMargin: 6 * root.uiScale
        anchors.verticalCenter: parent.verticalCenter
        spacing: 7 * root.uiScale
        StripButton {
            id: logo; key: "logo"; width: 38 * root.uiScale; description: qsTr("Home")
            focus: true
            onClicked: root.homeRequested()
            Navigation.rightItem: more
        }
        Row {
            spacing: 0
            Row {
                spacing: root.playerContext ? 10 * root.uiScale : 0
                StripButton {
                    id: more; key: "menu"; artworkSize: 18; description: qsTr("Menu")
                    checked: menu.shown
                    onClicked: menu.popup(mapToGlobal(0, height))
                    Navigation.leftItem: logo
                    Navigation.rightItem: navigation.visible ? navigation : minimize.visible ? minimize : fullscreen
                }
                StripButton {
                    id: navigation; key: "navigation"; description: qsTr("Show navigation menu")
                    visible: root.playerContext
                    checked: root.navigationVisible
                    onClicked: root.navigationRequested()
                    Navigation.leftItem: more
                    Navigation.rightItem: source.visible ? source : minimize.visible ? minimize : fullscreen
                }
            }
            Item {
                visible: source.visible
                width: 24 * root.uiScale; height: 28 * root.uiScale
                Image {
                    objectName: "juxtopposedStripArtseparator"
                    anchors.centerIn: parent; width: 24; height: 24; scale: root.uiScale
                    mirror: true
                    source: "qrc:/juxtopposed/titlebar-separator.svg"
                    Accessible.ignored: true
                }
            }
            StripButton {
                id: source; key: "sourceFolder"; description: qsTr("Return to %1").arg(root.sourceTitle)
                visible: root.playerContext && root.width >= 520 * root.uiScale
                artworkFrameSize: 28
                width: Math.min(150 * root.uiScale, 36 * root.uiScale + sourceLabel.implicitWidth)
                caption: root.sourceTitle
                onClicked: root.sourceRequested()
                Navigation.leftItem: navigation
                Navigation.rightItem: minimize.visible ? minimize : fullscreen
                C.Label { id: sourceLabel; visible: false; text: root.sourceTitle; font.family: inter.name; font.pixelSize: 14 * root.uiScale }
            }
        }
    }

    C.Label {
        id: title
        objectName: "juxtopposedStripTitle"
        anchors.centerIn: parent
        readonly property real availableWidth: Math.max(0, 2 * Math.min(root.width / 2 - (left.x + left.width) - 12 * root.uiScale, right.x - root.width / 2 - 12 * root.uiScale))
        width: Math.min(implicitWidth, availableWidth)
        visible: width >= 60 * root.uiScale
        text: root.displayedTitle
        font.family: inter.name; font.pixelSize: 14 * root.uiScale
        font.weight: Font.Normal
        color: JuxtopposedPalette.color("#8F8F8F")
        elide: Text.ElideRight
        horizontalAlignment: Text.AlignHCenter
    }
    Row {
        id: right
        anchors.right: parent.right; anchors.rightMargin: 6 * root.uiScale
        anchors.verticalCenter: parent.verticalCenter
        StripButton {
            id: minimize; key: "minimize"; description: qsTr("Minimize")
            width: 36 * root.uiScale
            visible: root.showWindowControls
            windowButton: root.nativeButton(CSDButton.Minimize)
            onClicked: windowButton?.click()
            Navigation.leftItem: source.visible ? source : navigation.visible ? navigation : more
            Navigation.rightItem: maximize
        }
        StripButton {
            id: maximize; key: "maximize"; description: MainCtx.intfMainWindow.visibility === Window.Maximized ? qsTr("Restore") : qsTr("Maximize")
            width: 36 * root.uiScale
            visible: root.showWindowControls
            windowButton: root.nativeButton(CSDButton.MaximizeRestore)
            onClicked: windowButton?.click()
            Navigation.leftItem: minimize
            Navigation.rightItem: fullscreen
        }
        StripButton {
            id: fullscreen; key: "fullscreen"; description: MainCtx.intfMainWindow.visibility === Window.FullScreen ? qsTr("Leave fullscreen") : qsTr("Fullscreen")
            width: 36 * root.uiScale
            onClicked: MainCtx.toggleInterfaceFullScreen()
            Navigation.leftItem: maximize.visible ? maximize : navigation.visible ? navigation : more
            Navigation.rightItem: close.visible ? close : logo
        }
        StripButton {
            id: close; key: "close"; description: qsTr("Close")
            width: 36 * root.uiScale
            visible: root.showWindowControls
            windowButton: root.nativeButton(CSDButton.Close)
            onClicked: windowButton?.click()
            Navigation.leftItem: fullscreen
        }
    }
    Menus.QmlGlobalMenu {
        id: menu
        ctx: MainCtx
        navigationAvailable: true
        navigationVisible: root.navigationVisible
        onNavigationRequested: root.navigationRequested()
        onAboutToShow: root.lockAutoHide(true)
        onAboutToHide: root.lockAutoHide(false)
    }
    Keys.priority: Keys.AfterItem
    Keys.onPressed: (event) => root.Navigation.defaultKeyAction(event)
}
