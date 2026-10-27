// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
// Original visual design by Juxtopposed: https://www.youtube.com/@juxtopposed
import QtQuick
import VLC.Style
import QtQuick.Controls.Basic as C

FocusScope {
    id: root
    objectName: "juxtopposedSubtitleCueTimeline"
    readonly property bool subtitleTimelineKeyScope: true
    property var controller
    property int focusedCueIndex: -1
    readonly property url focusedCueSource: controller ? controller.source : ""
    readonly property string focusedCueTrackId: controller ? controller.trackId : ""
    onControllerChanged: focusedCueIndex = -1
    onFocusedCueSourceChanged: focusedCueIndex = -1
    onFocusedCueTrackIdChanged: focusedCueIndex = -1
    onActiveFocusChanged: if (!activeFocus) focusedCueIndex = -1
    property real uiScale: 1
    property url assetBase: "qrc:/juxtopposed/"
    property url previousRangeIcon: assetBase + "timeline-chevron-left.svg"
    property url nextRangeIcon: assetBase + "timeline-chevron-right.svg"
    property url zoomOutIcon: assetBase + "timeline-minus.svg"
    property url zoomInIcon: assetBase + "timeline-plus.svg"
    property bool showStatus: true
    // All supplied/native values are microsecond VLC ticks. The initial
    // viewport shows thirty seconds around the real playback position.
    property real windowSpanTicks: 30000000
    property bool followPlayback: true
    property real manualWindowStartTicks: 0
    property real positionTicks: controller ? controller.positionTicks : 0
    property real lengthTicks: controller ? controller.lengthTicks : 0
    property real windowStartTicks: Math.max(0, Math.min(Math.max(0, lengthTicks - windowSpanTicks),
        followPlayback ? positionTicks - windowSpanTicks / 2 : manualWindowStartTicks))
    property real windowEndTicks: Math.max(windowStartTicks + 1, Math.min(lengthTicks, windowStartTicks + windowSpanTicks))
    readonly property var viewData: {
        if (!controller || !controller.available) return {segments: [], tooDense: false}
        const revision = controller.revision
        return controller.view(windowStartTicks, windowEndTicks)
    }
    readonly property bool timelineReady: !!controller && controller.available && lengthTicks > 0 && !viewData.tooDense
    readonly property bool hasCues: timelineReady && viewData.segments.length > 0
    readonly property bool canNavigate: !!controller && controller.available && lengthTicks > 0 && !timingDrag.pressed
    property bool draggingTiming: false
    property real dragPreviewTicks: 0
    signal requestLockUnlockAutoHide(bool lock)
    signal closeRequested()
    readonly property string statusText: viewData.tooDense ? qsTr("Too many cues in this range. Use a shorter timeline range.")
        : controller && controller.available && lengthTicks <= 0 ? qsTr("Waiting for the video duration.")
        : controller ? controller.status : qsTr("Subtitle timeline is unavailable.")
    implicitWidth: 600 * uiScale
    implicitHeight: (showStatus ? 58 : 28) * uiScale
    Accessible.role: Accessible.Grouping
    Accessible.name: qsTr("Subtitle source cue timeline")
    Accessible.description: statusText
    FontLoader { id: inter; source: root.assetBase + "InterVariable.ttf" }
    Keys.onLeftPressed: event => { root.panRange(-1); event.accepted = true }
    Keys.onRightPressed: event => { root.panRange(1); event.accepted = true }
    Keys.onEscapePressed: event => { timingDrag.cancelShift(); root.closeRequested(); event.accepted = true }
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Plus || event.key === Qt.Key_Equal) {
            root.zoomRange(0.5); event.accepted = true
        } else if (event.key === Qt.Key_Minus) {
            root.zoomRange(2); event.accepted = true
        } else if (event.key === Qt.Key_R && (event.modifiers & Qt.ControlModifier)) {
            if (root.controller && !root.controller.loading) root.controller.reload()
            event.accepted = true
        }
    }

    function relative(ticks) {
        return Math.max(0, Math.min(1, (ticks - windowStartTicks) / (windowEndTicks - windowStartTicks)))
    }
    function panRange(direction) {
        if (!canNavigate) return
        manualWindowStartTicks = windowStartTicks + direction * windowSpanTicks / 4
        followPlayback = false
    }
    function zoomRange(factor) {
        if (!canNavigate) return
        const centre = (windowStartTicks + windowEndTicks) / 2
        const span = Math.max(1000000, Math.min(Math.max(1000000, lengthTicks), windowSpanTicks * factor))
        windowSpanTicks = span
        if (!followPlayback) manualWindowStartTicks = centre - span / 2
    }
    function followPosition() {
        followPlayback = true
    }
    Rectangle {
        id: strip
        objectName: "subtitleCueStrip"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 28 * root.uiScale
        color: JuxtopposedPalette.color("#090909")
        radius: 6 * root.uiScale
        clip: true
        Rectangle {
            objectName: "subtitleCueElapsed"
            x: 2 * root.uiScale
            y: 2 * root.uiScale
            height: strip.height - 4 * root.uiScale
            width: Math.max(0, root.relative(root.positionTicks) * strip.width - 2 * root.uiScale)
            color: JuxtopposedPalette.color("#371F06")
            radius: 4 * root.uiScale
            visible: root.timelineReady
            Accessible.ignored: true
        }
        Repeater {
            id: cueRepeater
            model: root.hasCues ? root.viewData.segments : []
            delegate: Rectangle {
                id: cue
                required property var modelData
                required property int index
                objectName: "subtitleCue" + index
                x: root.relative(modelData.startTicks + root.dragPreviewTicks) * strip.width
                y: 2 * root.uiScale
                width: Math.max(root.uiScale, (root.relative(modelData.endTicks + root.dragPreviewTicks) - root.relative(modelData.startTicks + root.dragPreviewTicks)) * strip.width)
                height: strip.height - 4 * root.uiScale
                color: JuxtopposedPalette.color("#24FFFFFF")
                radius: root.uiScale
                activeFocusOnTab: !!modelData.seekable
                onActiveFocusChanged: if (activeFocus) root.focusedCueIndex = modelData.cueIndex
                Component.onCompleted: Qt.callLater(() => {
                    // Native seek completion can replace this viewport's
                    // delegates. Keep keyboard ownership inside the timeline.
                    if (root && cue && root.activeFocus && root.focusedCueIndex === cue.modelData.cueIndex)
                        cue.forceActiveFocus(Qt.OtherFocusReason)
                })
                border.width: activeFocus ? root.uiScale : 0
                border.color: JuxtopposedPalette.color("#D37918")
                Accessible.role: modelData.seekable ? Accessible.Button : Accessible.StaticText
                Accessible.name: modelData.text
                Accessible.description: modelData.seekable ? qsTr("Seek to this source cue's start.") : qsTr("This cue is unavailable for seeking.")
                Accessible.onPressAction: cue.activate()
                function activate() {
                    if (root.controller && modelData.seekable)
                        return root.controller.seekCue(modelData.cueIndex, modelData.revision)
                    return false
                }
                Keys.onReturnPressed: event => { cue.activate(); event.accepted = true }
                Keys.onEnterPressed: event => { cue.activate(); event.accepted = true }
                Keys.onSpacePressed: event => { cue.activate(); event.accepted = true }
                C.ToolTip {
                    id: cueTip
                    visible: hover.hovered
                    text: cue.modelData.text
                    delay: 400
                    contentItem: Text { text: cueTip.text; textFormat: Text.PlainText; font.family: inter.name; font.pixelSize: 12 * root.uiScale; color: JuxtopposedPalette.color("#BEBEBE"); wrapMode: Text.WordWrap; width: Math.min(implicitWidth, 320 * root.uiScale) }
                    background: Rectangle { color: JuxtopposedPalette.color("#171717"); radius: 3 * root.uiScale; border.width: root.uiScale; border.color: JuxtopposedPalette.color("#303030") }
                }
                HoverHandler { id: hover }
            }
        }
        Rectangle {
            objectName: "subtitleCuePlayhead"
            x: Math.max(0, Math.min(strip.width - width, root.relative(root.positionTicks) * strip.width - width / 2))
            y: 2 * root.uiScale
            width: 8 * root.uiScale
            height: strip.height - 4 * root.uiScale
            radius: width / 2
            color: JuxtopposedPalette.color("#D37918")
            visible: root.timelineReady
            Accessible.ignored: true
        }
        // A drag previews one whole-source shift and submits only on release.
        // Cancellation, a source change or an old revision cannot commit it.
        MouseArea {
            id: timingDrag
            objectName: "subtitleTimingDrag"
            x: 44 * root.uiScale
            width: Math.max(0, strip.width - 88 * root.uiScale)
            height: strip.height
            acceptedButtons: Qt.LeftButton
            enabled: root.timelineReady
            property real pressedX: 0
            property real initialDelay: 0
            property int initialRevision: -1
            property real initialSpan: 0
            property real initialWidth: 1
            property bool previousFollow: true
            property bool lastGestureWasDrag: false
            property bool interactionLocked: false
            function unlock() {
                if (interactionLocked) {
                    interactionLocked = false
                    root.requestLockUnlockAutoHide(false)
                }
            }
            function cancelShift() {
                if (interactionLocked) root.followPlayback = previousFollow
                root.draggingTiming = false
                root.dragPreviewTicks = 0
                lastGestureWasDrag = true
                initialRevision = -1
                unlock()
            }
            onPressed: mouse => {
                pressedX = mouse.x
                initialDelay = root.controller.delayTicks
                initialRevision = root.controller.revision
                initialSpan = root.windowEndTicks - root.windowStartTicks
                initialWidth = Math.max(1, strip.width)
                previousFollow = root.followPlayback
                root.manualWindowStartTicks = root.windowStartTicks
                root.followPlayback = false
                lastGestureWasDrag = false
                root.draggingTiming = false
                root.dragPreviewTicks = 0
                interactionLocked = true
                root.requestLockUnlockAutoHide(true)
            }
            onPositionChanged: mouse => {
                if (!pressed) return
                const delta = mouse.x - pressedX
                if (Math.abs(delta) >= 8 * root.uiScale) root.draggingTiming = true
                if (root.draggingTiming)
                    root.dragPreviewTicks = Math.round(delta * initialSpan / initialWidth)
            }
            onReleased: mouse => {
                lastGestureWasDrag = root.draggingTiming
                const requested = initialDelay + root.dragPreviewTicks
                if (root.draggingTiming && Number.isFinite(requested) && initialRevision >= 0)
                    root.controller.setDelayTicks(Math.round(requested), initialRevision)
                root.draggingTiming = false
                root.dragPreviewTicks = 0
                root.followPlayback = previousFollow
                unlock()
            }
            onCanceled: cancelShift()
            onEnabledChanged: if (!enabled) cancelShift()
            Component.onDestruction: unlock()
            onClicked: mouse => {
                if (lastGestureWasDrag) return
                const tick = root.windowStartTicks + (mouse.x + x) / strip.width * (root.windowEndTicks - root.windowStartTicks)
                // Later overlapping cues are painted above earlier ones.
                for (let i = cueRepeater.count - 1; i >= 0; --i) {
                    const cue = cueRepeater.itemAt(i)
                    if (cue && cue.modelData.startTicks <= tick && cue.modelData.endTicks > tick) {
                        cue.forceActiveFocus(Qt.MouseFocusReason)
                        cue.activate()
                        return
                    }
                }
            }
        }
        RangeButton {
            objectName: "subtitlePreviousRange"
            x: 2 * root.uiScale
            source: root.previousRangeIcon
            mirrorIcon: true
            description: qsTr("Show earlier subtitle times")
            width: 18 * root.uiScale
            enabled: root.canNavigate && root.windowStartTicks > 0
            onClicked: root.panRange(-1)
        }
        RangeButton {
            objectName: "subtitleZoomOut"
            x: 22 * root.uiScale
            source: root.zoomOutIcon
            description: qsTr("Zoom out subtitle timeline")
            width: 22 * root.uiScale
            enabled: root.canNavigate && root.windowSpanTicks < Math.max(1000000, root.lengthTicks)
            onClicked: root.zoomRange(2)
        }
        RangeButton {
            objectName: "subtitleZoomIn"
            anchors.right: nextRange.left
            anchors.rightMargin: 2 * root.uiScale
            source: root.zoomInIcon
            description: qsTr("Zoom in subtitle timeline")
            width: 22 * root.uiScale
            enabled: root.canNavigate && root.windowSpanTicks > 1000000
            onClicked: root.zoomRange(0.5)
        }
        RangeButton {
            id: nextRange
            objectName: "subtitleNextRange"
            anchors.right: parent.right
            source: root.nextRangeIcon
            description: qsTr("Show later subtitle times")
            width: 18 * root.uiScale
            enabled: root.canNavigate && root.windowEndTicks < root.lengthTicks
            onClicked: root.panRange(1)
        }
        C.Label {
            anchors.centerIn: parent
            width: Math.max(0, parent.width - 96 * root.uiScale)
            visible: !root.timelineReady && !root.controller?.loading
            text: root.statusText
            textFormat: Text.PlainText
            font.family: inter.name
            font.pixelSize: 11 * root.uiScale
            color: JuxtopposedPalette.color("#8F8F8F")
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
            Accessible.ignored: true
        }
        C.BusyIndicator {
            objectName: "subtitleCueLoading"
            anchors.centerIn: parent
            width: strip.height; height: width
            running: !!root.controller && root.controller.loading
            visible: running
        }
    }
    component RangeButton: C.ToolButton {
        required property url source
        required property string description
        property bool mirrorIcon: false
        y: 2 * root.uiScale
        height: 24 * root.uiScale
        padding: 0
        activeFocusOnTab: true
        onActiveFocusChanged: if (activeFocus) root.focusedCueIndex = -1
        Keys.onReturnPressed: event => { if (enabled) clicked(); event.accepted = true }
        Keys.onEnterPressed: event => { if (enabled) clicked(); event.accepted = true }
        Accessible.name: description
        C.ToolTip.visible: hovered || visualFocus
        C.ToolTip.text: description
        C.ToolTip.delay: 400
        contentItem: Image { source: parent.source; mirror: parent.mirrorIcon; fillMode: Image.PreserveAspectFit; opacity: parent.enabled ? 1 : 0.35 }
        background: Rectangle { color: JuxtopposedPalette.color("#090909"); radius: root.uiScale; border.width: parent.visualFocus ? root.uiScale : 0; border.color: JuxtopposedPalette.color("#D37918") }
    }
    C.Label {
        objectName: "subtitleCueStatus"
        anchors.left: parent.left
        anchors.right: reload.left
        anchors.rightMargin: 8 * root.uiScale
        anchors.top: strip.bottom
        anchors.topMargin: 5 * root.uiScale
        visible: root.showStatus
        text: root.statusText
        textFormat: Text.PlainText
        font.family: inter.name
        font.pixelSize: 11 * root.uiScale
        color: JuxtopposedPalette.color("#686868")
        elide: Text.ElideRight
        Accessible.ignored: true
    }
    C.ToolButton {
        id: reload
        objectName: "subtitleCueReload"
        anchors.right: parent.right
        anchors.top: strip.bottom
        anchors.topMargin: 3 * root.uiScale
        width: 70 * root.uiScale
        height: 24 * root.uiScale
        visible: root.showStatus
        enabled: !!root.controller && !root.controller.loading && root.controller.source.toString().length > 0
        onActiveFocusChanged: if (activeFocus) root.focusedCueIndex = -1
        Keys.onReturnPressed: event => { if (enabled) clicked(); event.accepted = true }
        Keys.onEnterPressed: event => { if (enabled) clicked(); event.accepted = true }
        text: qsTr("Reload")
        font.family: inter.name
        font.pixelSize: 11 * root.uiScale
        Accessible.name: qsTr("Reload subtitle source cue timings")
        Accessible.description: qsTr("Read current cue times after file edits. Reopen the subtitle to refresh playback text.")
        onClicked: root.controller.reload()
        contentItem: Text { text: reload.text; font: reload.font; color: reload.enabled ? JuxtopposedPalette.color("#BEBEBE") : JuxtopposedPalette.color("#686868"); horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
        background: Rectangle { color: reload.down ? JuxtopposedPalette.color("#303030") : reload.hovered ? JuxtopposedPalette.color("#252525") : JuxtopposedPalette.color("#171717"); radius: 3 * root.uiScale; border.width: reload.activeFocus ? root.uiScale : 0; border.color: JuxtopposedPalette.color("#D37918") }
    }
}
