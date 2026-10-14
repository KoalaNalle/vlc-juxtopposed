// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
import QtQuick
import VLC.Widgets as Widgets

Item {
    id: root
    objectName: "globalSettingsBackdropEffect"
    property Item sourceItem: null
    property rect captureRect
    property rect regionInCapture
    property real uiScale: 1
    readonly property bool ready: blur.ready
    readonly property Item capturedSource: capture.sourceItem
    // Six-pass Dual Kawase has per-axis variance (245/2)*(r-.5)^2.
    // Keep its existing integer kernel and choose the texture density so its
    // second moment matches the Figma/CSS 25px Gaussian at every UI scale/DPR.
    // This is the existing project's Gaussian approximation, not a new shader.
    readonly property int kernelRadius: 3
    readonly property real kernelSigma: Math.sqrt(245 / 2) * (kernelRadius - .5)
    readonly property real textureDensity: kernelSigma / (25 * uiScale)
    readonly property size captureSize: Qt.size(Math.max(1, Math.ceil(captureRect.width * textureDensity)),
                                              Math.max(1, Math.ceil(captureRect.height * textureDensity)))

    // Rasterize the underlying vector scene at its native physical pixel
    // density before the existing texture filter performs any downsampling.
    // smooth on ShaderEffectSource affects sampling, not this rasterization.
    readonly property size nativeCaptureSize: Qt.size(Math.max(1, Math.ceil(captureRect.width * blur.eDPR)),
                                                    Math.max(1, Math.ceil(captureRect.height * blur.eDPR)))

    Widgets.RoundedRectangleShadow {
        objectName: "globalSettingsBackdropShadow"
        // CSS shadow 0px 8px 15px 0px rgba(0,0,0,.46): its Gaussian
        // standard deviation is half the CSS shadow blur-radius.
        blurRadius: 7.5 * root.uiScale
        xOffset: 0
        yOffset: 8 * root.uiScale
        radius: 14 * root.uiScale
        color: Qt.rgba(0, 0, 0, .46)
        compensationFactor: 3
    }
    ShaderEffectSource {
        id: capture
        objectName: "globalSettingsBackdropCapture"
        sourceItem: root.sourceItem
        sourceRect: root.captureRect
        textureSize: root.nativeCaptureSize
        visible: false
        hideSource: false
        recursive: false
        live: true
        smooth: true
    }
    Widgets.DualKawaseBlur {
        id: blur
        objectName: "globalSettingsBackdropBlur"
        width: root.captureSize.width / eDPR
        height: root.captureSize.height / eDPR
        source: capture
        radius: root.kernelRadius
        mode: Widgets.DualKawaseBlur.Mode.SixPass
        live: true
        postprocess: false
    }
    ShaderEffectSource {
        id: roundedSource
        objectName: "globalSettingsBackdropRoundedSource"
        sourceItem: blur
        sourceRect: Qt.rect(root.regionInCapture.x * blur.width / Math.max(1, root.captureRect.width),
                            root.regionInCapture.y * blur.height / Math.max(1, root.captureRect.height),
                            root.regionInCapture.width * blur.width / Math.max(1, root.captureRect.width),
                            root.regionInCapture.height * blur.height / Math.max(1, root.captureRect.height))
        textureSize: Qt.size(Math.max(1, Math.ceil(root.width * root.textureDensity)),
                             Math.max(1, Math.ceil(root.height * root.textureDensity)))
        visible: false
        hideSource: true
        recursive: false
        live: true
        smooth: true
        // ImageExt's documented external texture-provider interface.
        readonly property int status: blur.ready ? Image.Ready : Image.Loading
        implicitWidth: textureSize.width
        implicitHeight: textureSize.height
    }
    Widgets.ImageExt {
        objectName: "globalSettingsBackdropRoundedBlur"
        anchors.fill: parent
        textureProviderItem: roundedSource
        loadImages: false
        radius: 14 * root.uiScale
        fillMode: Image.Stretch
        backgroundColor: "transparent"
        visible: root.ready
    }
}
