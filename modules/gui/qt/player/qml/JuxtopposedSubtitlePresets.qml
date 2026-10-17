// Copyright (C) 2026 VLC authors and VideoLAN. GPLv2 or later.
// Original visual design: Juxtopposed. The unshown naming form is provisional.
pragma ComponentBehavior: Bound
import QtQuick
import VLC.Style
import QtQml.Models
import QtQuick.Controls.Basic as C
import QtQuick.Layouts

ColumnLayout {
    id: root

    required property var controller
    property real uiScale: 1
    property string fontFamily: "Inter"
    property string iconFontFamily: fontFamily
    property string addIcon: "+"
    property string moreIcon: "..."
    property string checkIcon: "✓"
    property string downIcon: "v"
    property bool naming: false
    property alias selector: selector
    property alias nameEditor: nameEditor
    property alias saveButton: saveButton
    property alias cancelButton: cancelButton
    property alias advancedButton: advancedButton
    readonly property var keyboardControls: naming
        ? [selector, nameEditor, saveButton, cancelButton, advancedButton]
        : [selector, advancedButton]

    signal revealRequested(var item)
    signal advancedRequested
    signal moveFocusRequested(var current, int direction)

    function startNaming() {
        if (!controller.canSave)
            return;
        controller.clearError();
        naming = true;
        nameEditor.text = "";
        Qt.callLater(() => nameEditor.forceActiveFocus(Qt.PopupFocusReason));
    }
    function cancelNaming() {
        naming = false;
        nameEditor.text = "";
        controller.clearError();
        selector.forceActiveFocus(Qt.PopupFocusReason);
    }
    function saveCurrent() {
        if (controller.save(nameEditor.text)) {
            naming = false;
            nameEditor.text = "";
            selector.forceActiveFocus(Qt.PopupFocusReason);
        } else {
            Qt.callLater(() => root.revealRequested(presetStatus));
        }
    }

    spacing: 8 * uiScale

    C.Button {
        id: selector

        Accessible.name: qsTr("Subtitle preset")
        Layout.fillWidth: true
        Layout.preferredHeight: 36 * root.uiScale
        enabled: root.controller.canApply || root.controller.canSave
        font.family: root.fontFamily
        font.pixelSize: 14 * root.uiScale
        objectName: "subtitlePresetSelector"
        text: root.controller.selectedName

        background: Rectangle {
            border.color: selector.visualFocus ? JuxtopposedPalette.color("#D37918") : JuxtopposedPalette.color("#252525")
            border.width: root.uiScale
            color: selector.down ? JuxtopposedPalette.color("#303030") : JuxtopposedPalette.color("#161616")
            radius: 4 * root.uiScale
        }
        contentItem: RowLayout {
            spacing: 8 * root.uiScale
            C.Label {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                color: selector.enabled ? JuxtopposedPalette.color("#969696") : JuxtopposedPalette.color("#606060")
                elide: Text.ElideRight
                font: selector.font
                horizontalAlignment: Text.AlignHCenter
                text: selector.text
                verticalAlignment: Text.AlignVCenter
            }
            C.Label {
                Accessible.ignored: true
                color: JuxtopposedPalette.color("#969696")
                font.family: root.iconFontFamily
                font.pixelSize: 16 * root.uiScale
                text: root.downIcon
            }
        }
        onClicked: presets.open()
        Keys.onUpPressed: root.moveFocusRequested(selector, -1)
        Keys.onDownPressed: root.moveFocusRequested(selector, 1)

        C.Menu {
            id: presets

            closePolicy: C.Popup.CloseOnEscape | C.Popup.CloseOnPressOutside
            padding: 4 * root.uiScale
            width: selector.width
            y: selector.height

            background: Rectangle {
                color: JuxtopposedPalette.color("#191919")
                radius: 6 * root.uiScale
                border.color: JuxtopposedPalette.color("#252525")
                border.width: root.uiScale
            }
            C.MenuItem {
                id: newPreset

                Accessible.name: qsTr("New Custom Preset")
                enabled: root.controller.canSave
                font.family: root.fontFamily
                font.pixelSize: 14 * root.uiScale
                height: 40 * root.uiScale
                objectName: "newSubtitlePreset"
                text: qsTr("New Custom Preset")

                contentItem: RowLayout {
                    spacing: 10 * root.uiScale
                    C.Label {
                        Accessible.ignored: true
                        color: JuxtopposedPalette.color("#969696")
                        font.family: root.iconFontFamily
                        font.pixelSize: 16 * root.uiScale
                        text: root.addIcon
                    }
                    C.Label {
                        Layout.fillWidth: true
                        color: newPreset.enabled ? JuxtopposedPalette.color("#969696") : JuxtopposedPalette.color("#606060")
                        font: newPreset.font
                        text: newPreset.text
                        verticalAlignment: Text.AlignVCenter
                    }
                }
                background: Rectangle {
                    color: newPreset.highlighted ? JuxtopposedPalette.color("#242424") : "transparent"
                    radius: 4 * root.uiScale
                }
                onTriggered: {
                    presets.close();
                    root.startNaming();
                }
            }
            Instantiator {
                model: root.controller.entries
                delegate: C.MenuItem {
                    id: entry

                    required property var modelData

                    Accessible.name: modelData.name
                    checkable: true
                    checked: modelData.selected
                    enabled: root.controller.canApply
                    font.family: root.fontFamily
                    font.pixelSize: 14 * root.uiScale
                    height: 40 * root.uiScale
                    objectName: "subtitlePresetEntry_" + modelData.id
                    padding: 10 * root.uiScale
                    text: modelData.name

                    indicator: null
                    background: Rectangle {
                        color: entry.highlighted || entry.checked ? JuxtopposedPalette.color("#242424") : "transparent"
                        radius: 4 * root.uiScale
                    }
                    contentItem: RowLayout {
                        spacing: 10 * root.uiScale
                        C.Label {
                            Accessible.ignored: true
                            color: JuxtopposedPalette.color("#969696")
                            font.family: root.iconFontFamily
                            font.pixelSize: 16 * root.uiScale
                            text: root.moreIcon
                        }
                        C.Label {
                            Layout.fillWidth: true
                            Layout.minimumWidth: 0
                            color: entry.enabled ? JuxtopposedPalette.color("#969696") : JuxtopposedPalette.color("#606060")
                            elide: Text.ElideRight
                            font: entry.font
                            text: entry.text
                        }
                        C.Label {
                            Accessible.ignored: true
                            color: JuxtopposedPalette.color("#D37918")
                            font.family: root.iconFontFamily
                            font.pixelSize: 16 * root.uiScale
                            text: entry.checked ? root.checkIcon : ""
                        }
                    }
                    onTriggered: {
                        // Applying replaces the entries and can destroy this item.
                        const presetId = modelData.id;
                        presets.close();
                        root.controller.apply(presetId);
                    }
                }
                onObjectAdded: (index, object) => presets.insertItem(index + 1, object)
                onObjectRemoved: (index, object) => presets.removeItem(object)
            }
        }
    }
    ColumnLayout {
        Layout.fillWidth: true
        spacing: 8 * root.uiScale
        visible: root.naming

        C.TextField {
            id: nameEditor

            Accessible.name: qsTr("Subtitle preset name")
            Layout.fillWidth: true
            Layout.preferredHeight: 36 * root.uiScale
            color: JuxtopposedPalette.color("#969696")
            font.family: root.fontFamily
            font.pixelSize: 14 * root.uiScale
            maximumLength: 120
            objectName: "subtitlePresetName"
            padding: 10 * root.uiScale
            topPadding: 4 * root.uiScale
            bottomPadding: 4 * root.uiScale
            placeholderText: qsTr("Preset name")
            selectByMouse: true

            background: Rectangle {
                border.color: nameEditor.activeFocus ? JuxtopposedPalette.color("#D37918") : JuxtopposedPalette.color("#252525")
                border.width: root.uiScale
                color: JuxtopposedPalette.color("#090909")
                radius: 4 * root.uiScale
            }
            onAccepted: {
                if (saveButton.enabled)
                    root.saveCurrent();
            }
            Keys.onEscapePressed: event => { root.cancelNaming(); event.accepted = true; }
            Keys.onUpPressed: root.moveFocusRequested(nameEditor, -1)
            Keys.onDownPressed: root.moveFocusRequested(nameEditor, 1)
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 8 * root.uiScale
            uniformCellSizes: true

            PresetButton {
                id: cancelButton

                objectName: "subtitlePresetCancel"
                text: qsTr("Cancel")
                onClicked: root.cancelNaming()
            }
            PresetButton {
                id: saveButton

                enabled: root.controller.canSave && nameEditor.text.trim().length > 0
                objectName: "subtitlePresetSave"
                text: qsTr("Save Preset")
                onClicked: root.saveCurrent()
            }
        }
    }
    C.Label {
        id: presetStatus
        Accessible.name: text
        Layout.fillWidth: true
        color: root.controller.error.length ? JuxtopposedPalette.color("#E39B60") : JuxtopposedPalette.color("#969696")
        font.family: root.fontFamily
        font.pixelSize: 12 * root.uiScale
        objectName: "subtitlePresetStatus"
        text: root.controller.error.length ? root.controller.error : root.controller.statusText
        visible: text.length > 0
        wrapMode: Text.WordWrap
    }
    PresetButton {
        id: advancedButton

        objectName: "subtitlePresetAdvanced"
        text: qsTr("Open Advanced Settings")
        onClicked: root.advancedRequested()
    }

    component PresetButton: C.Button {
        id: button

        Accessible.name: text
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        Layout.preferredHeight: 36 * root.uiScale
        font.family: root.fontFamily
        font.pixelSize: 14 * root.uiScale
        padding: 8 * root.uiScale
        background: Rectangle {
            border.color: button.visualFocus ? JuxtopposedPalette.color("#D37918") : JuxtopposedPalette.color("#252525")
            border.width: root.uiScale
            color: button.down ? JuxtopposedPalette.color("#303030") : JuxtopposedPalette.color("#161616")
            radius: 4 * root.uiScale
        }
        contentItem: C.Label {
            color: button.enabled ? JuxtopposedPalette.color("#969696") : JuxtopposedPalette.color("#606060")
            elide: Text.ElideRight
            font: button.font
            horizontalAlignment: Text.AlignHCenter
            text: button.text
            verticalAlignment: Text.AlignVCenter
        }
        Keys.onUpPressed: root.moveFocusRequested(button, -1)
        Keys.onDownPressed: root.moveFocusRequested(button, 1)
    }
}
