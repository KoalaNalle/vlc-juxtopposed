// SPDX-License-Identifier: GPL-2.0-or-later
// Visual design: Juxtopposed. Uses the existing native layout's ordered IDs.
import QtQuick
import QtQuick.Controls.Basic as C
import QtQuick.Layouts
import VLC.Style

Item {
    id: root
    property real uiScale: 1
    property url assetBase: "qrc:/juxtopposed/"
    property var columns: [[], [], []]
    property var appearance: JuxtopposedPalette
    property var describe: function(id) { return ({name: String(id), kind: "button"}) }
    property var state: function(id) { return ({enabled: false}) }
    property var metadata: ({})
    property bool editing: false
    property bool timelineInline: true
    property Component volumeComponent
    property Item dropViewport: null
    readonly property Item timelineHost: timelineHost
    readonly property real contentWidth: row.implicitWidth
    signal activated(int tile, var item)
    signal tilePressed(int column, int index, int tile, var item, point position)
    signal tileMoved(var item, point position)
    signal tileReleased(var item, point position)
    signal selectionRequested(int column, int index)
    signal removalRequested(int column, int index)
    signal revealRequested(var item)

    function controls() {
        const result = []
        for (let c = 0; c < groups.count; ++c) {
            const group = groups.itemAt(c)
            for (let n = 0; n < group.tiles.count; ++n) {
                if (root.editing) { result.push(group.tiles.itemAt(n)); continue }
                const item = group.tiles.itemAt(n).item
                if (item && item.keyboardControls !== undefined) {
                    for (const child of item.keyboardControls)
                        if (child && child.enabled && child.visible && child.activeFocusOnTab) result.push(child)
                } else if (item && item.enabled && item.visible && item.activeFocusOnTab) result.push(item)
            }
        }
        return result
    }
    function itemFor(tile) {
        for (let c = 0; c < groups.count; ++c) {
            const group = groups.itemAt(c)
            for (let n = 0; n < group.tiles.count; ++n)
                if (Number(group.values[n]) === tile) return group.tiles.itemAt(n).item
        }
        return null
    }
    function itemForKind(kind) {
        for (let c = 0; c < groups.count; ++c) {
            const group = groups.itemAt(c)
            for (let n = 0; n < group.tiles.count; ++n)
                if (root.describe(Number(group.values[n])).kind === kind) return group.tiles.itemAt(n).item
        }
        return null
    }
    // Insertion boundaries are calculated from current geometry at release.
    // Neither drag state nor the caller keeps a delegate through a model reset.
    function destination(item, position) {
        if (dropViewport) {
            const viewportPoint = item.mapToItem(dropViewport, position.x, position.y)
            if (viewportPoint.x < 0 || viewportPoint.x > dropViewport.width || viewportPoint.y < 0 || viewportPoint.y > dropViewport.height) return null
        }
        const point = item.mapToItem(root, position.x, position.y)
        if (point.y < 0 || point.y > height || point.x < 0 || point.x > width) return null
        let chosen = 0
        let distance = Infinity
        for (let c = 0; c < groups.count; ++c) {
            const group = groups.itemAt(c)
            const x = Math.max(group.x, Math.min(point.x, group.x + group.width))
            if (Math.abs(point.x - x) < distance) { chosen = c; distance = Math.abs(point.x - x) }
        }
        const group = groups.itemAt(chosen)
        for (let n = 0; n < group.tiles.count; ++n) {
            const tile = group.tiles.itemAt(n)
            if (point.x < group.x + tile.x + tile.width / 2) return ({column: chosen, index: n})
        }
        return ({column: chosen, index: group.tiles.count})
    }

    implicitHeight: 32 * uiScale
    implicitWidth: row.implicitWidth
    FontLoader { id: inter; source: root.assetBase + "InterVariable.ttf" }
    RowLayout {
        id: row
        anchors.fill: parent
        spacing: 20 * root.uiScale
        Repeater {
            id: groups
            model: 3
            delegate: Item {
                id: group
                required property int index
                readonly property var values: root.columns[index] ?? []
                property alias tiles: tiles
                Layout.fillWidth: index === 1
                Layout.minimumWidth: root.editing ? 60 * root.uiScale : 0
                Layout.preferredHeight: 32 * root.uiScale
                implicitWidth: contents.implicitWidth
                Rectangle {
                    anchors.fill: parent
                    visible: root.editing
                    color: "transparent"
                    border.width: 1
                    border.color: root.appearance.color("#454545")
                    radius: 4 * root.uiScale
                    z: -1
                }
                RowLayout {
                id: contents
                anchors.fill: parent
                spacing: 8 * root.uiScale
                Repeater {
                    id: tiles
                    model: group.values
                    delegate: Loader {
                        id: tile
                        required property int index
                        required property var modelData
                        readonly property int tileId: Number(modelData)
                        readonly property var info: root.describe(tileId)
                        readonly property var status: root.state(tileId)
                        objectName: "layoutTile_" + group.index + "_" + index
                        activeFocusOnTab: root.editing
                        onActiveFocusChanged: if (root.editing && activeFocus) {
                            root.selectionRequested(group.index, tile.index)
                            root.revealRequested(tile)
                        }
                        Accessible.role: root.editing ? Accessible.Button : Accessible.NoRole
                        Accessible.name: info.name
                        Keys.onReturnPressed: if (root.editing) root.selectionRequested(group.index, tile.index)
                        Keys.onEnterPressed: if (root.editing) root.selectionRequested(group.index, tile.index)
                        Keys.onDeletePressed: if (root.editing) root.removalRequested(group.index, tile.index)
                        Layout.preferredWidth: item ? item.implicitWidth : 0
                        Layout.preferredHeight: (["play", "previous", "next"].includes(info.kind) ? 28 : 32) * root.uiScale
                        Layout.alignment: Qt.AlignVCenter
                        sourceComponent: info.kind === "spacer" ? spacer : info.kind === "divider" ? divider
                            : info.kind === "metadata" ? metadataLabel : info.kind === "artwork" ? artwork
                            : info.kind === "volume" ? root.volumeComponent : action

                        Component { id: spacer; Item { implicitWidth: 24 * root.uiScale } }
                        Component { id: divider; Item {
                            implicitWidth: 8 * root.uiScale
                            Rectangle { anchors.centerIn: parent; height: 20 * root.uiScale; width: root.uiScale; color: root.appearance.color("#454545") }
                        } }
                        Component { id: metadataLabel; Item {
                            implicitWidth: Math.min(160 * root.uiScale, Math.max(70 * root.uiScale, metrics.advanceWidth))
                            C.Label {
                            id: label
                            anchors.fill: parent
                            text: root.metadata[tile.info.key] ?? ""
                            textFormat: Text.PlainText
                            elide: Text.ElideRight
                            verticalAlignment: Text.AlignVCenter
                            font.family: inter.name; font.pixelSize: 13 * root.uiScale
                            color: root.appearance.color("#969696")
                            Accessible.name: tile.info.name + ": " + text
                            TextMetrics { id: metrics; text: label.text; font: label.font }
                            HoverHandler { id: metadataHover }
                            C.ToolTip.visible: metadataHover.hovered
                            C.ToolTip.text: text
                            }
                        } }
                        Component { id: artwork; Item {
                            implicitWidth: 32 * root.uiScale
                            Image {
                            anchors.fill: parent
                            source: root.metadata.artwork ?? ""
                            fillMode: Image.PreserveAspectFit
                            Accessible.role: Accessible.Graphic
                            Accessible.name: tile.info.name
                            }
                        } }
                        Component { id: action; C.CheckBox {
                            id: button
                            // Use the existing native key-ownership seam after
                            // pointer/accessibility focus as well as Tab focus.
                            readonly property bool navigationKeyScope: true
                            function activateKey(event) {
                                if (event.modifiers & (Qt.ControlModifier | Qt.AltModifier | Qt.MetaModifier)) {
                                    event.accepted = false
                                    return
                                }
                                event.accepted = true
                                if (!event.isAutoRepeat) root.activated(tile.tileId, button)
                            }
                            implicitWidth: (tile.info.kind === "play" ? 54 : 32) * root.uiScale
                            implicitHeight: 32 * root.uiScale
                            enabled: !root.editing && tile.status.enabled === true
                            activeFocusOnTab: !root.editing
                            checkable: tile.status.checked !== undefined
                            indicator: null
                            spacing: 0
                            // The action owner supplies the result, including
                            // asynchronous failure. Clicking never guesses it.
                            nextCheckState: () => tile.status.checked === true ? Qt.Checked : Qt.Unchecked
                            // Keep the native button's state in sync as well as
                            // its accessible flags. Automatic toggling stays off:
                            // the native action owner confirms the new state.
                            checked: tile.status.checked === true
                            padding: 3 * root.uiScale
                            opacity: enabled || root.editing ? 1 : 0.45
                            Accessible.role: Accessible.Button
                            Accessible.name: tile.status.name ?? tile.info.name
                            Accessible.description: tile.status.note ?? ""
                            Accessible.checkable: tile.status.checked !== undefined
                            Accessible.checked: tile.status.checked === true
                            C.ToolTip.visible: hovered || visualFocus
                            C.ToolTip.text: tile.status.note ?? Accessible.name
                            C.ToolTip.delay: 500
                            background: Rectangle {
                                radius: 6 * root.uiScale
                                color: tile.info.kind === "play" ? (button.down ? root.appearance.color("#B16614")
                                    : tile.status.playing ? root.appearance.color("#6B6B6B") : root.appearance.playButtonAccent)
                                    : root.appearance.color(tile.status.checked ? "#371F06" : button.down ? "#454545" : button.hovered ? "#303030" : "#1B1B1B")
                                border.width: button.visualFocus ? 2 * root.uiScale : 0
                                border.color: root.appearance.color("#D37918")
                            }
                            contentItem: Item {
                                Image {
                                    anchors.centerIn: parent
                                    width: 24 * root.uiScale; height: width
                                    source: tile.status.source ?? ""
                                    sourceSize: Qt.size(width, height)
                                    visible: source.toString().length > 0
                                }
                                Text {
                                    anchors.centerIn: parent
                                    visible: text.length > 0
                                    text: tile.status.glyph ?? ""
                                    font.family: tile.status.fontFamily ?? inter.name
                                    font.pixelSize: 22 * root.uiScale
                                    color: root.appearance.color("#969696")
                                }
                            }
                            onClicked: root.activated(tile.tileId, button)
                            Keys.onReturnPressed: event => button.activateKey(event)
                            Keys.onEnterPressed: event => button.activateKey(event)
                            Keys.onSpacePressed: event => button.activateKey(event)
                            onActiveFocusChanged: if (activeFocus) root.revealRequested(button)
                        } }
                        // Editing intercepts pointer gestures before a real action.
                        MouseArea {
                            anchors.fill: parent
                            enabled: root.editing
                            cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                            onPressed: mouse => {
                                root.tilePressed(group.index, tile.index, tile.tileId, tile, Qt.point(mouse.x, mouse.y))
                                tile.forceActiveFocus(Qt.MouseFocusReason)
                            }
                            onPositionChanged: mouse => { if (pressed) root.tileMoved(tile, Qt.point(mouse.x, mouse.y)) }
                            onReleased: mouse => root.tileReleased(tile, Qt.point(mouse.x, mouse.y))
                            onClicked: root.selectionRequested(group.index, tile.index)
                            onCanceled: root.tileReleased(tile, Qt.point(-100000, -100000))
                        }
                        HoverHandler {id: unavailableHover; enabled: !root.editing && tile.status.enabled !== true}
                        C.ToolTip.visible: unavailableHover.hovered
                        C.ToolTip.text: tile.status.note ?? tile.info.name
                        C.ToolTip.delay: 500
                        Rectangle {
                            anchors.fill: parent
                            visible: root.editing && tile.activeFocus
                            color: "transparent"
                            border.width: 2 * root.uiScale
                            border.color: root.appearance.color("#D37918")
                            radius: 4 * root.uiScale
                        }
                    }
                }
                Item {
                    id: inlineHost
                    Layout.fillWidth: true
                    Layout.minimumWidth: 200 * root.uiScale
                    Layout.preferredHeight: 28 * root.uiScale
                    visible: group.index === 1 && root.timelineInline
                    Component.onCompleted: if (group.index === 1) timelineHost.parent = inlineHost
                }
                }
            }
        }
    }
    Item { id: timelineHost; anchors.fill: parent }
}
