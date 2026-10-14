// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
// Functional adaptation of the shared Juxtopposed modal styling. The inspected
// Figma references did not contain an original conversion form/profile editor.
import QtQuick
import VLC.Style
import QtQuick.Controls.Basic as C
import QtQuick.Layouts
import VLC.Dialogs

C.Dialog {
    id: root
    objectName: "juxtopposedConversionDialog"
    property var controller
    property real uiScale: 1
    property url assetBase: "qrc:/juxtopposed/"
    property Item focusReturnItem
    property Item revealTarget
    property bool closeAfterCancel: false
    readonly property bool busy: !!controller && controller.busy
    readonly property bool completed: !!controller && controller.state === ConversionController.Succeeded
    readonly property bool cancelling: !!controller && controller.state === ConversionController.Cancelling
    readonly property bool hasFiles: !!controller && controller.sourceUrl.toString().length > 0
                                    && controller.destinationUrl.toString().length > 0
    signal sourceRequested()
    signal destinationRequested()
    signal allProfilesRequested()
    signal fallbackFocusRequested()

    title: qsTr("Convert")
    modal: true
    focus: true
    anchors.centerIn: parent
    implicitWidth: 713 * uiScale
    implicitHeight: 560 * uiScale
    width: Math.min(implicitWidth, Math.max(0, parent.width - 32 * uiScale))
    height: Math.min(implicitHeight, Math.max(0, parent.height - 32 * uiScale))
    padding: 0
    closePolicy: C.Popup.NoAutoClose

    function showDialog(returnItem) {
        if (!visible) {
            focusReturnItem = returnItem
            closeAfterCancel = false
            if (controller && !controller.busy) controller.reset()
            open()
        }
        if (busy) cancelButton.forceActiveFocus(Qt.PopupFocusReason)
        else sourceButton.forceActiveFocus(Qt.PopupFocusReason)
    }
    function submit() {
        if (!controller || busy || !hasFiles) return
        // start() owns validation and supplies the actual error for invalid
        // selections. A selected existing destination must never be overwritten.
        controller.start()
        if (busy) cancelButton.forceActiveFocus(Qt.OtherFocusReason)
    }
    function requestClose() {
        if (busy) {
            closeAfterCancel = true
            controller.cancel()
        } else reject()
    }
    function cancelJob() {
        if (busy) controller.cancel()
        else requestClose()
    }
    function displayPath(url) {
        const value = url ? url.toString() : ""
        try { return decodeURIComponent(value).replace(/^file:\/\/\//, "") }
        catch (error) { return value }
    }
    function timeText(ms) {
        const seconds = Math.max(0, Math.floor(ms / 1000))
        return Math.floor(seconds / 60) + ":" + (seconds % 60).toString().padStart(2, "0")
    }
    function phaseText() {
        if (!controller) return ""
        switch (controller.state) {
        case ConversionController.Starting: return qsTr("Starting conversion…")
        case ConversionController.Encoding: return qsTr("Converting audio…")
        case ConversionController.Validating: return qsTr("Checking the converted audio…")
        case ConversionController.Publishing: return qsTr("Saving the converted file…")
        case ConversionController.Cancelling: return qsTr("Cancelling conversion…")
        case ConversionController.Succeeded: return qsTr("Conversion complete")
        case ConversionController.Failed: return qsTr("Conversion failed")
        case ConversionController.Cancelled: return qsTr("Conversion cancelled")
        default: return qsTr("Choose an audio file and a new output filename.")
        }
    }
    function reveal(item) {
        const flick = formScroll.contentItem
        if (!flick || flick.contentY === undefined || !flick.contentItem) return
        const point = item.mapToItem(flick.contentItem, 0, 0)
        // Leave space around the focus ring at the fixed header/footer edges.
        const margin = 4 * root.uiScale
        const viewportHeight = Math.max(0, Math.min(flick.height, root.footer.mapToItem(flick, 0, 0).y))
        if (point.y < flick.contentY + margin) flick.contentY = Math.max(0, point.y - margin)
        else if (point.y + item.height + margin > flick.contentY + viewportHeight)
            flick.contentY = Math.min(Math.max(0, flick.contentHeight - viewportHeight),
                                     point.y + item.height + margin - viewportHeight)
    }
    function focusFormItem(item) {
        revealTarget = item
        Qt.callLater(() => { if (root.visible && item.activeFocus) root.reveal(item) })
    }
    onClosed: {
        if (focusReturnItem && focusReturnItem.visible && focusReturnItem.enabled)
            focusReturnItem.forceActiveFocus(Qt.PopupFocusReason)
        else fallbackFocusRequested()
        focusReturnItem = null
    }
    Connections {
        target: root.controller
        function onChanged() {
            if (!root.visible) return
            if (!root.busy && root.closeAfterCancel) {
                root.closeAfterCancel = false
                root.reject()
            } else if (!root.busy && root.controller.state !== ConversionController.Idle) {
                Qt.callLater(() => {
                    if (!root.visible || root.busy) return
                    root.reveal(statusArea)
                    convertButton.forceActiveFocus(Qt.OtherFocusReason)
                })
            }
        }
    }
    Connections {
        target: formScroll.contentItem
        function onContentHeightChanged() {
            // Wrapping at a new scale can finish after the first focus event.
            // Reveal the same focused control again using the settled layout.
            Qt.callLater(() => {
                if (root.visible && root.revealTarget && root.revealTarget.activeFocus)
                    root.reveal(root.revealTarget)
            })
        }
    }
    Shortcut { sequence: "Escape"; enabled: root.visible; onActivated: root.requestClose() }
    Shortcut { sequence: "Ctrl+Return"; enabled: root.visible && !root.busy && !root.completed && root.hasFiles; onActivated: root.submit() }
    FontLoader { id: inter; source: root.assetBase + "InterVariable.ttf" }
    background: Rectangle { radius: 14 * root.uiScale; color: Qt.rgba(25 / 255, 25 / 255, 25 / 255, .95) }
    C.Overlay.modal: Rectangle { color: JuxtopposedPalette.color("#80000000") }

    component FormLabel: Text {
        color: JuxtopposedPalette.color("#C6C6C6")
        font.family: inter.name
        font.pixelSize: 14 * root.uiScale
        textFormat: Text.PlainText
        wrapMode: Text.WordWrap
    }
    component FormButton: C.Button {
        id: button
        property bool primary: false
        implicitWidth: 100 * root.uiScale
        implicitHeight: 32 * root.uiScale
        activeFocusOnTab: true
        opacity: enabled ? 1 : .5
        font.family: inter.name
        font.pixelSize: 14 * root.uiScale
        font.weight: Font.DemiBold
        Keys.onReturnPressed: clicked()
        Keys.onEnterPressed: clicked()
        contentItem: Text {
            text: button.text
            font: button.font
            textFormat: Text.PlainText
            color: button.primary ? JuxtopposedPalette.color("#D37918") : JuxtopposedPalette.color("#8F8F8F")
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        background: Rectangle {
            radius: 6 * root.uiScale
            color: button.primary ? (button.down ? JuxtopposedPalette.color("#4D2C09") : JuxtopposedPalette.color("#371F06"))
                : (button.hovered ? JuxtopposedPalette.color("#303030") : JuxtopposedPalette.color("#1B1B1B"))
            border.width: button.visualFocus ? root.uiScale : 0
            border.color: JuxtopposedPalette.color("#D37918")
        }
    }
    component FileField: C.TextField {
        id: field
        readOnly: true
        selectByMouse: true
        activeFocusOnTab: true
        enabled: !root.busy
        color: JuxtopposedPalette.color("#C6C6C6")
        placeholderTextColor: JuxtopposedPalette.color("#686868")
        selectionColor: JuxtopposedPalette.color("#6B431C")
        font.family: inter.name
        font.pixelSize: 13 * root.uiScale
        leftPadding: 10 * root.uiScale
        rightPadding: 10 * root.uiScale
        onActiveFocusChanged: if (activeFocus) root.focusFormItem(field)
        background: Rectangle {
            color: JuxtopposedPalette.color("#090909")
            radius: 6 * root.uiScale
            border.width: field.activeFocus ? root.uiScale : 0
            border.color: JuxtopposedPalette.color("#D37918")
        }
    }
    header: Rectangle {
        implicitHeight: 50 * root.uiScale
        color: JuxtopposedPalette.color("#090909")
        radius: 14 * root.uiScale
        Rectangle { anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; height: parent.height / 2; color: parent.color }
        FormLabel { anchors.left: parent.left; anchors.leftMargin: 20 * root.uiScale; anchors.verticalCenter: parent.verticalCenter; text: root.title; color: JuxtopposedPalette.color("#8F8F8F") }
        C.ToolButton {
            id: closeButton
            objectName: "conversionClose"
            anchors.right: parent.right
            anchors.rightMargin: 9 * root.uiScale
            anchors.verticalCenter: parent.verticalCenter
            width: 32 * root.uiScale; height: width
            activeFocusOnTab: true
            Accessible.name: root.busy ? qsTr("Cancel conversion and close when finished") : qsTr("Close conversion dialog")
            KeyNavigation.tab: root.busy ? cancelButton : sourceField
            KeyNavigation.backtab: convertButton.enabled ? convertButton : cancelButton
            onClicked: root.requestClose()
            background: Rectangle { radius: 4 * root.uiScale; color: closeButton.hovered ? JuxtopposedPalette.color("#303030") : "transparent"; border.width: closeButton.visualFocus ? root.uiScale : 0; border.color: JuxtopposedPalette.color("#D37918") }
            contentItem: Item {
                Image { anchors.centerIn: parent; width: 24; height: 24; scale: root.uiScale; rotation: 180; transform: Scale { origin.x: 12; origin.y: 12; yScale: -1 } source: root.assetBase + "new-playlist-close.svg"; Accessible.ignored: true }
            }
        }
    }
    contentItem: C.ScrollView {
        id: formScroll
        objectName: "conversionScroll"
        clip: true
        contentWidth: availableWidth
        C.ScrollBar.horizontal.policy: C.ScrollBar.AlwaysOff
        ColumnLayout {
            width: formScroll.availableWidth
            spacing: 12 * root.uiScale
            Item { Layout.preferredHeight: 8 * root.uiScale }
            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 20 * root.uiScale; Layout.rightMargin: 20 * root.uiScale
                spacing: 12 * root.uiScale
                FormLabel { text: qsTr("Source") }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10 * root.uiScale
                    FileField {
                        id: sourceField
                        objectName: "conversionSource"
                        Layout.fillWidth: true
                        Layout.preferredHeight: 32 * root.uiScale
                        text: root.controller ? root.displayPath(root.controller.sourceUrl) : ""
                        placeholderText: qsTr("Choose a local audio file")
                        Accessible.name: qsTr("Source audio file")
                        KeyNavigation.priority: KeyNavigation.BeforeItem
                        KeyNavigation.tab: sourceButton
                        KeyNavigation.backtab: closeButton
                    }
                    FormButton {
                        id: sourceButton
                        objectName: "conversionSourceBrowse"
                        text: qsTr("Browse")
                        enabled: !!root.controller && !root.busy
                        Accessible.name: qsTr("Choose source audio file")
                        KeyNavigation.tab: destinationField
                        KeyNavigation.backtab: sourceField
                        onActiveFocusChanged: if (activeFocus) root.focusFormItem(sourceButton)
                        onClicked: root.sourceRequested()
                    }
                }
                FormLabel { text: qsTr("Profile"); Layout.topMargin: 6 * root.uiScale }
                FormLabel {
                    objectName: "conversionProfile"
                    text: root.controller ? root.controller.profileName : ""
                    Layout.fillWidth: true
                    font.weight: Font.DemiBold
                }
                FormLabel {
                    text: qsTr("44.1 kHz · Stereo · Up to 512 MiB and one hour. Your original file is kept.")
                    Layout.fillWidth: true
                    font.pixelSize: 12 * root.uiScale
                    color: JuxtopposedPalette.color("#686868")
                }
                FormLabel { text: qsTr("Destination"); Layout.topMargin: 6 * root.uiScale }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10 * root.uiScale
                    FileField {
                        id: destinationField
                        objectName: "conversionDestination"
                        Layout.fillWidth: true
                        Layout.preferredHeight: 32 * root.uiScale
                        text: root.controller ? root.displayPath(root.controller.destinationUrl) : ""
                        placeholderText: qsTr("Choose a new .flac filename")
                        Accessible.name: qsTr("Destination FLAC file")
                        KeyNavigation.priority: KeyNavigation.BeforeItem
                        KeyNavigation.tab: destinationButton
                        KeyNavigation.backtab: sourceButton
                    }
                    FormButton {
                        id: destinationButton
                        objectName: "conversionDestinationBrowse"
                        text: qsTr("Browse")
                        enabled: !!root.controller && !root.busy
                        Accessible.name: qsTr("Choose destination FLAC filename")
                        KeyNavigation.tab: allProfilesButton
                        KeyNavigation.backtab: destinationField
                        onActiveFocusChanged: if (activeFocus) root.focusFormItem(destinationButton)
                        onClicked: root.destinationRequested()
                    }
                }
                FormButton {
                    id: allProfilesButton
                    objectName: "conversionAllProfiles"
                    Layout.fillWidth: true
                    Layout.topMargin: 4 * root.uiScale
                    text: qsTr("All formats and profiles…")
                    enabled: !root.busy
                    KeyNavigation.tab: root.completed ? outputField : cancelButton
                    KeyNavigation.backtab: destinationButton
                    onActiveFocusChanged: if (activeFocus) root.focusFormItem(allProfilesButton)
                    onClicked: { root.reject(); root.allProfilesRequested() }
                }
                ColumnLayout {
                    id: statusArea
                    objectName: "conversionStatusArea"
                    Layout.fillWidth: true
                    Layout.topMargin: 6 * root.uiScale
                    spacing: 8 * root.uiScale
                    FormLabel {
                        objectName: "conversionStatus"
                        Layout.fillWidth: true
                        text: root.phaseText()
                        color: root.completed ? JuxtopposedPalette.color("#D37918") : JuxtopposedPalette.color("#8F8F8F")
                        Accessible.role: Accessible.StaticText
                        Accessible.name: text
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        visible: root.busy
                        C.BusyIndicator {
                            objectName: "conversionBusy"
                            Layout.preferredWidth: 24 * root.uiScale
                            Layout.preferredHeight: width
                            running: visible
                            visible: root.controller && (root.controller.state !== ConversionController.Encoding || root.controller.progress < 0)
                        }
                        C.ProgressBar {
                            id: progressBar
                            objectName: "conversionProgress"
                            Layout.fillWidth: true
                            Layout.preferredHeight: 6 * root.uiScale
                            visible: root.controller && root.controller.state === ConversionController.Encoding && root.controller.progress >= 0
                            value: root.controller ? Math.max(0, Math.min(1, root.controller.progress)) : 0
                            background: Rectangle { radius: height / 2; color: JuxtopposedPalette.color("#090909") }
                            contentItem: Item {
                                implicitHeight: 6 * root.uiScale
                                Rectangle { width: progressBar.visualPosition * parent.width; height: parent.height; radius: height / 2; color: JuxtopposedPalette.color("#D37918") }
                            }
                            Accessible.name: qsTr("Audio conversion progress")
                        }
                        FormLabel {
                            objectName: "conversionTiming"
                            visible: progressBar.visible
                            text: root.controller ? Math.floor(root.controller.progress * 100) + "% · "
                                + root.timeText(root.controller.sourceTimeMs) + " / " + root.timeText(root.controller.sourceDurationMs) : ""
                            font.pixelSize: 12 * root.uiScale
                            color: JuxtopposedPalette.color("#8F8F8F")
                        }
                    }
                    FormLabel {
                        objectName: "conversionError"
                        Layout.fillWidth: true
                        text: root.controller ? root.controller.error : ""
                        visible: text.length > 0
                        color: JuxtopposedPalette.color("#D98779")
                        font.pixelSize: 12 * root.uiScale
                        Accessible.role: Accessible.StaticText
                        Accessible.name: text
                    }
                    FileField {
                        id: outputField
                        objectName: "conversionOutput"
                        Layout.fillWidth: true
                        Layout.preferredHeight: 32 * root.uiScale
                        visible: root.completed
                        text: root.controller ? root.displayPath(root.controller.outputUrl) : ""
                        Accessible.name: qsTr("Saved converted audio file")
                        KeyNavigation.tab: cancelButton
                        KeyNavigation.backtab: allProfilesButton
                    }
                }
            }
            Item { Layout.preferredHeight: 8 * root.uiScale }
        }
    }
    footer: Item {
        implicitHeight: 72 * root.uiScale
        FormButton {
            id: cancelButton
            objectName: "conversionCancel"
            anchors.left: parent.left; anchors.leftMargin: 20 * root.uiScale
            anchors.bottom: parent.bottom; anchors.bottomMargin: 20 * root.uiScale
            text: root.cancelling ? qsTr("Cancelling…") : root.busy ? qsTr("Cancel") : qsTr("Close")
            enabled: !root.cancelling
            KeyNavigation.tab: convertButton.enabled ? convertButton : closeButton
            KeyNavigation.backtab: root.busy ? closeButton : root.completed ? outputField : allProfilesButton
            onClicked: root.cancelJob()
        }
        FormButton {
            id: convertButton
            objectName: "conversionConvert"
            anchors.right: parent.right; anchors.rightMargin: 20 * root.uiScale
            anchors.bottom: parent.bottom; anchors.bottomMargin: 20 * root.uiScale
            primary: true
            text: root.completed ? qsTr("Done") : qsTr("Convert")
            enabled: !!root.controller && !root.busy && (root.completed || root.hasFiles)
            KeyNavigation.tab: closeButton
            KeyNavigation.backtab: cancelButton
            onClicked: root.completed ? root.requestClose() : root.submit()
        }
    }
}
