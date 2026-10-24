// SPDX-License-Identifier: GPL-2.0-or-later
// Functional profile-name/recovery form. The cached design confirms the menu
// entries, but supplies no original New Profile or interrupted-switch form.
import QtQuick
import QtQuick.Controls.Basic as C
import QtQuick.Layouts
import VLC.Style

JuxtopposedMovableDialog {
    id: root
    objectName: "juxtopposedPlayerProfilesDialog"
    property var controller: null
    property real uiScale: 1
    property url assetBase: "qrc:/juxtopposed/"
    property bool createMode: true
    property Item focusReturnItem: null
    readonly property bool busy: !!controller && controller.busy
    readonly property bool recovering: !!controller && controller.needsRecovery
    readonly property bool canSave: !!controller && controller.available && !busy
        && (!createMode || (!recovering && profileName.text.trim().length > 0))
    signal fallbackFocusRequested()
    title: createMode ? qsTr("New Profile") : qsTr("Player Profile")
    modal: true
    focus: true
    padding: 0
    header: null
    footer: null
    implicitWidth: 520 * uiScale
    implicitHeight: (createMode ? 340 : 330) * uiScale
    width: Math.min(implicitWidth, Math.max(0, parent.width - 32 * uiScale))
    height: Math.min(implicitHeight, Math.max(0, parent.height - 32 * uiScale))
    closePolicy: busy ? C.Popup.NoAutoClose : C.Popup.CloseOnEscape

    function showDialog(returnItem, create) {
        createMode = create === true
        if (!visible) { focusReturnItem = returnItem; profileName.text = ""; open() }
        Qt.callLater(() => {
            (createMode ? profileName : primaryButton).forceActiveFocus(Qt.PopupFocusReason)
            root.revealError()
        })
    }
    function save() {
        if (!canSave) return
        const accepted = createMode ? controller.createProfile(profileName.text)
            : recovering ? controller.retrySwitch() : controller.retrySave()
        if (accepted) accept()
    }
    function reveal(item) {
        const flick = scroll.contentItem
        if (!flick || flick.contentY === undefined || !flick.contentItem) return
        const point = item.mapToItem(flick.contentItem, 0, 0)
        if (point.y < flick.contentY) flick.contentY = Math.max(0, point.y)
        else if (point.y + item.height > flick.contentY + flick.height)
            flick.contentY = Math.min(Math.max(0, flick.contentHeight - flick.height), point.y + item.height - flick.height)
    }
    function revealError() {
        Qt.callLater(() => {
            if (root.visible && profileError.visible) root.reveal(profileError)
        })
    }
    onHeightChanged: if (visible) {
        if (profileError.visible) root.revealError()
        else if (profileName.activeFocus) Qt.callLater(() => root.reveal(profileName))
    }
    onWidthChanged: if (visible) {
        if (profileError.visible) root.revealError()
        else if (profileName.activeFocus) Qt.callLater(() => root.reveal(profileName))
    }
    onClosed: {
        if (focusReturnItem && focusReturnItem.visible && focusReturnItem.enabled)
            focusReturnItem.forceActiveFocus(Qt.PopupFocusReason)
        else fallbackFocusRequested()
        focusReturnItem = null
    }
    Shortcut { sequence: "Ctrl+Return"; enabled: root.visible && root.canSave; onActivated: root.save() }
    FontLoader { id: inter; source: root.assetBase + "InterVariable.ttf" }
    background: Rectangle {radius: 14 * root.uiScale; color: JuxtopposedPalette.color("#191919")}
    C.Overlay.modal: Rectangle {color: JuxtopposedPalette.color("#80000000")}
    component Label: C.Label {
        font.family: inter.name
        font.pixelSize: 14 * root.uiScale
        color: JuxtopposedPalette.color("#C6C6C6")
        textFormat: Text.PlainText
        wrapMode: Text.WordWrap
    }
    component Button: C.Button {
        id: button
        property bool primary: false
        implicitHeight: 34 * root.uiScale
        implicitWidth: Math.max(100 * root.uiScale, contentItem.implicitWidth + 24 * root.uiScale)
        leftPadding: 12 * root.uiScale; rightPadding: 12 * root.uiScale
        activeFocusOnTab: true
        opacity: enabled ? 1 : 0.45
        contentItem: Label {text: button.text; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter;
            color: JuxtopposedPalette.color(button.primary ? "#D37918" : "#C6C6C6")}
        background: Rectangle {radius: 6 * root.uiScale;
            color: JuxtopposedPalette.color(button.primary ? "#371F06" : button.hovered ? "#404040" : "#303030");
            border.width: button.visualFocus ? 2 * root.uiScale : 0; border.color: JuxtopposedPalette.color("#FFFFFF")}
    }
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24 * root.uiScale
        spacing: 14 * root.uiScale
        Item {
            objectName: "playerProfileHeader"
            Layout.fillWidth: true
            implicitHeight: 28 * root.uiScale
            JuxtopposedDialogDragArea {anchors.fill: parent; popup: root}
            Label {text: root.title; font.pixelSize: 20 * root.uiScale; font.weight: Font.DemiBold; anchors.verticalCenter: parent.verticalCenter}
        }
        C.ScrollView {
            id: scroll
            objectName: "playerProfileScroll"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: availableWidth
            ColumnLayout {
                width: scroll.availableWidth
                spacing: 12 * root.uiScale
                Label {Layout.fillWidth: true; text: root.createMode
                    ? qsTr("Start from the current playback, audio, video, subtitle defaults and player layout.")
                    : qsTr("Current profile: %1").arg(root.controller ? root.controller.selectedName : "")}
                C.TextField {
                    id: profileName
                    objectName: "playerProfileName"
                    Layout.fillWidth: true
                    visible: root.createMode
                    enabled: root.canSave || (!root.busy && !root.recovering && !!root.controller && root.controller.available)
                    placeholderText: qsTr("Profile name")
                    placeholderTextColor: JuxtopposedPalette.color("#8F8F8F")
                    maximumLength: 128
                    activeFocusOnTab: true
                    Accessible.name: qsTr("Profile name")
                    font.family: inter.name; font.pixelSize: 14 * root.uiScale
                    color: JuxtopposedPalette.color("#C6C6C6")
                    selectedTextColor: JuxtopposedPalette.color("#FFFFFF")
                    selectionColor: JuxtopposedPalette.color("#371F06")
                    background: Rectangle {radius: 5 * root.uiScale; color: JuxtopposedPalette.color("#303030");
                        border.width: profileName.activeFocus ? 2 * root.uiScale : 1; border.color: JuxtopposedPalette.color(profileName.activeFocus ? "#D37918" : "#404040")}
                    onAccepted: root.save()
                    onActiveFocusChanged: if (activeFocus) Qt.callLater(() => root.reveal(profileName))
                }
                Label {Layout.fillWidth: true; text: qsTr("Appearance, keyboard shortcuts, privacy, network access and library locations stay shared.")}
                Label {Layout.fillWidth: true; text: qsTr("Some defaults apply after restarting VLC. Your current media keeps its selected tracks and position.")}
                Label {
                    id: profileError
                    objectName: "playerProfileError"
                    Layout.fillWidth: true
                    visible: text.length > 0
                    text: root.controller ? root.controller.lastError : qsTr("Player profiles are unavailable.")
                    color: JuxtopposedPalette.color("#ED9B45")
                    Accessible.role: Accessible.AlertMessage
                    Accessible.name: text
                    onTextChanged: root.revealError()
                    onHeightChanged: if (visible) root.revealError()
                }
            }
        }
        Flow {
            Layout.fillWidth: true
            Layout.preferredHeight: childrenRect.height
            spacing: 8 * root.uiScale
            Button {objectName: "playerProfileCancel"; text: qsTr("Cancel"); enabled: !root.busy; onClicked: root.reject()}
            Button {objectName: "playerProfileRestore"; visible: root.recovering && !root.createMode; text: qsTr("Restore previous"); enabled: root.canSave;
                onClicked: if (root.controller.restorePreviousProfile()) root.accept()}
            Button {id: primaryButton; objectName: "playerProfileSave"; text: root.createMode ? qsTr("Create") : root.recovering ? qsTr("Finish switch") : qsTr("Retry save");
                primary: true; enabled: root.canSave; onClicked: root.save()}
        }
    }
}
