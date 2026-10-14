// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
import QtQuick
import QtQuick.Window

// The source must be the main view, whose visual tree excludes Popup.Overlay.
// Never supply the window contentItem or an ancestor containing this backdrop.
Item {
    id: root
    objectName: "globalSettingsBackdrop"
    property Item sourceItem: null
    property bool active: false
    property color color: "#0E0E0E"
    property real uiScale: 1
    readonly property real radius: 14 * uiScale
    readonly property real blurRadius: 25 * uiScale
    readonly property real fillOpacity: .95
    readonly property bool effectsAvailable: GraphicsInfo.shaderType === GraphicsInfo.RhiShader
    readonly property bool sourceIsSafe: _sourceSafe
    readonly property bool blurReady: effectLoader.item ? effectLoader.item.ready : false
    readonly property Item capturedSource: effectLoader.item ? effectLoader.item.capturedSource : null
    readonly property rect captureRect: _captureRect
    readonly property rect regionInCapture: _regionInCapture
    property bool _sourceSafe: false
    property rect _captureRect: Qt.rect(0, 0, 0, 0)
    property rect _regionInCapture: Qt.rect(0, 0, 0, 0)

    function updateCaptureGeometry() {
        let safe = active && sourceItem && root.Window.window
                && sourceItem.Window.window === root.Window.window
                && sourceItem !== root.Window.window.contentItem
        // Reject recursive visual capture even if a future caller changes the
        // source or uses an embedded popup instead of the window overlay.
        for (let ancestor = root; safe && ancestor; ancestor = ancestor.parent)
            if (ancestor === sourceItem) safe = false
        _sourceSafe = !!safe
        if (!safe || width <= 0 || height <= 0 || sourceItem.width <= 0 || sourceItem.height <= 0) {
            _captureRect = Qt.rect(0, 0, 0, 0)
            _regionInCapture = Qt.rect(0, 0, 0, 0)
            return
        }
        const position = mapToItem(sourceItem, 0, 0)
        // Include the blur footprint; clamp only at the actual view boundary.
        const margin = Math.ceil(3 * blurRadius)
        const left = Math.max(0, Math.floor(position.x - margin))
        const top = Math.max(0, Math.floor(position.y - margin))
        const right = Math.min(sourceItem.width, Math.ceil(position.x + width + margin))
        const bottom = Math.min(sourceItem.height, Math.ceil(position.y + height + margin))
        const rectangle = Qt.rect(left, top, Math.max(0, right - left), Math.max(0, bottom - top))
        const region = Qt.rect(position.x - left, position.y - top, width, height)
        if (_captureRect !== rectangle) _captureRect = rectangle
        if (_regionInCapture !== region) _regionInCapture = region
    }
    onSourceItemChanged: updateCaptureGeometry()
    onActiveChanged: updateCaptureGeometry()
    onWidthChanged: updateCaptureGeometry()
    onHeightChanged: updateCaptureGeometry()
    onUiScaleChanged: updateCaptureGeometry()
    onParentChanged: updateCaptureGeometry()
    Window.onWindowChanged: updateCaptureGeometry()
    Component.onCompleted: updateCaptureGeometry()
    Connections {
        target: root.sourceItem
        function onWidthChanged() { root.updateCaptureGeometry() }
        function onHeightChanged() { root.updateCaptureGeometry() }
    }
    Connections {
        target: root.Window.window
        enabled: root.active && root.sourceItem !== null
        // mapToItem does not install bindings for intermediate transforms.
        // Re-evaluate on rendered frames so scrolling, moving and resizing
        // cannot leave the captured region at its previous position.
        function onAfterAnimating() { root.updateCaptureGeometry() }
    }
    Loader {
        id: effectLoader
        objectName: "globalSettingsBackdropEffectLoader"
        anchors.fill: parent
        active: root.active && root.sourceIsSafe && root.effectsAvailable
                && root.captureRect.width > 0 && root.captureRect.height > 0
        // Delay loading the existing RHI-only types on software renderers.
        source: active ? "JuxtopposedSettingsBackdropEffect.qml" : ""
        onLoaded: {
            item.sourceItem = Qt.binding(() => root.sourceItem)
            item.captureRect = Qt.binding(() => root.captureRect)
            item.regionInCapture = Qt.binding(() => root.regionInCapture)
            item.uiScale = Qt.binding(() => root.uiScale)
        }
    }
    Rectangle {
        objectName: "globalSettingsBackdropTint"
        anchors.fill: parent
        color: root.color
        opacity: root.fillOpacity
        radius: root.radius
    }
}
