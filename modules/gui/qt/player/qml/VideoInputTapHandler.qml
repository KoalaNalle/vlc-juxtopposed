// SPDX-License-Identifier: GPL-2.0-or-later
// Relay mini-player pointer gestures to the same native video policy.
import QtQuick
TapHandler {
    id: tap
    required property var videoSurfaceProvider
    property real pixelRatio: 1
    property bool dragging: false
    property bool nativeGesture: false
    signal touchTapped()
    gesturePolicy: TapHandler.WithinBounds
    function moveNativePointer() {
        if (videoSurfaceProvider)
            videoSurfaceProvider.onMouseMoved(point.position.x * pixelRatio, point.position.y * pixelRatio)
    }
    function cancelNativePointer() {
        if (nativeGesture && videoSurfaceProvider) videoSurfaceProvider.onMouseCancelled()
        nativeGesture = false
    }
    onPressedChanged: {
        if (pressed) {
            nativeGesture = !!videoSurfaceProvider && point.device.type !== PointerDevice.TouchScreen
            if (nativeGesture) { moveNativePointer(); videoSurfaceProvider.onMousePressed(0) }
        } else if (nativeGesture && videoSurfaceProvider) {
            if (!enabled || dragging) cancelNativePointer()
            else {
                moveNativePointer()
                videoSurfaceProvider.onMouseReleased(0)
                nativeGesture = false
            }
        }
    }
    onCanceled: cancelNativePointer()
    onEnabledChanged: if (!enabled) cancelNativePointer()
    onDraggingChanged: if (dragging) cancelNativePointer()
    onDoubleTapped: (eventPoint, button) => {
        if (eventPoint.device.type !== PointerDevice.TouchScreen && videoSurfaceProvider) {
            videoSurfaceProvider.onMouseDoubleClick(0)
            videoSurfaceProvider.onMouseReleased(0)
        }
    }
    onTapped: (eventPoint, button) => {
        if (eventPoint.device.type === PointerDevice.TouchScreen) touchTapped()
    }
}
