// Copyright (C) 2026 VLC authors and VideoLAN. GPLv2 or later.
// Visual design: Juxtopposed; original artwork: CC BY 4.0.
import QtQuick
import QtQuick.Controls.Basic as C
import QtQuick.Layouts

C.Control {
    id: root

    property url assetBase: "qrc:/juxtopposed/"
    property int decimals: 1
    property alias editor: editor
    property real from: 0
    property alias slider: slider
    property real sliderMaximum: to
    property bool sliderVisible: true
    property real stepSize: 0.1
    property var ticks: []
    property string title: ""
    property real to: 1
    property real uiScale: 1
    property string unit: ""
    property real value: 0

    signal moveFocusRequested(int direction)
    signal valueRequested(real value)

    function commitText() {
        if (!editor.dirty) {
            editor.text = formatValue(root.value);
            return;
        }
        const number = Number(editor.text.trim().replace(root.unit, ""));
        if (editor.text.trim().length > 0 && Number.isFinite(number) && number >= from && number <= to) {
            if (number !== root.value)
                root.valueRequested(number);
            editor.text = formatValue(number);
        } else
            editor.text = formatValue(root.value);
    }
    function formatValue(value) {
        return Number(value).toFixed(decimals) + unit;
    }
    function setCurrentItemFocus(reason) {
        if (sliderVisible)
            slider.forceActiveFocus(reason);
        else
            editor.forceActiveFocus(reason);
    }

    Accessible.name: title
    implicitHeight: sliderVisible ? (ticks.length ? 100 : 80) * uiScale : 40 * uiScale
    implicitWidth: 360 * uiScale
    opacity: enabled ? 1 : 0.35
    padding: 0

    contentItem: Item {
        RowLayout {
            id: valueRow

            anchors.left: parent.left
            anchors.right: parent.right
            height: 32 * root.uiScale
            spacing: 12 * root.uiScale

            C.Label {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                color: "#969696"
                elide: Text.ElideRight
                font.family: inter.name
                font.pixelSize: 14 * root.uiScale
                text: root.title
            }
            C.TextField {
                id: editor

                property bool dirty: false

                Accessible.name: root.title
                Layout.preferredHeight: 32 * root.uiScale
                Layout.preferredWidth: Math.min(144 * root.uiScale, root.width * 0.48)
                bottomPadding: 4 * root.uiScale
                color: "#969696"
                font.family: inter.name
                font.pixelSize: 14 * root.uiScale
                objectName: "settingValueEditor"
                padding: 10 * root.uiScale
                selectByMouse: true
                topPadding: 4 * root.uiScale

                background: Rectangle {
                    border.color: editor.activeFocus ? "#D37918" : "#252525"
                    border.width: root.uiScale
                    color: "#090909"
                    radius: 4 * root.uiScale
                }

                Keys.onDownPressed: root.moveFocusRequested(1)
                Keys.onUpPressed: root.moveFocusRequested(-1)
                onEditingFinished: {
                    root.commitText();
                    dirty = false;
                }
                onTextEdited: dirty = true

                Binding {
                    property: "text"
                    restoreMode: Binding.RestoreNone
                    target: editor
                    value: root.formatValue(root.value)
                    when: !editor.activeFocus || !editor.dirty
                }
            }
        }
        C.Slider {
            id: slider

            Accessible.name: root.title
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: valueRow.bottom
            anchors.topMargin: 12 * root.uiScale
            from: root.from
            height: 28 * root.uiScale
            objectName: "settingValueSlider"
            padding: 0
            stepSize: root.stepSize
            to: Math.max(root.sliderMaximum, root.value)
            visible: root.sliderVisible
            wheelEnabled: true

            background: Rectangle {
                border.color: slider.visualFocus ? "#D37918" : "#252525"
                border.width: root.uiScale
                color: "#050505"
                height: 14 * root.uiScale
                radius: 5 * root.uiScale
                width: slider.availableWidth
                x: slider.leftPadding
                y: (slider.height - height) / 2

                Rectangle {
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.margins: root.uiScale
                    anchors.top: parent.top
                    color: "#49290C"
                    radius: 4 * root.uiScale
                    width: Math.max(0, (parent.width - 2 * root.uiScale) * slider.visualPosition)
                }
                Repeater {
                    model: root.ticks

                    Rectangle {
                        required property real modelData

                        anchors.verticalCenter: parent.verticalCenter
                        color: "#303030"
                        height: 6 * root.uiScale
                        width: root.uiScale
                        x: Math.max(0, Math.min(parent.width - width, (modelData - slider.from) / (slider.to - slider.from) * parent.width))
                    }
                }
            }
            handle: Rectangle {
                color: "#D37918"
                height: 24 * root.uiScale
                radius: 4 * root.uiScale
                width: 8 * root.uiScale
                x: slider.visualPosition * (slider.availableWidth - width)
                y: (slider.height - height) / 2
            }

            Keys.onDownPressed: root.moveFocusRequested(1)
            Keys.onUpPressed: root.moveFocusRequested(-1)
            onMoved: root.valueRequested(value)

            Binding {
                property: "value"
                restoreMode: Binding.RestoreNone
                target: slider
                value: root.value
                when: !slider.pressed
            }
        }
        Item {
            anchors.top: slider.bottom
            height: 16 * root.uiScale
            visible: root.sliderVisible && root.ticks.length > 0
            width: parent.width

            Repeater {
                model: root.ticks

                C.Label {
                    required property real modelData

                    color: "#6B6B6B"
                    font.family: inter.name
                    font.pixelSize: 10 * root.uiScale
                    horizontalAlignment: Text.AlignHCenter
                    text: modelData
                    x: Math.min(parent.width - width, Math.max(0, (modelData - slider.from) / (slider.to - slider.from) * parent.width - width / 2))
                }
            }
        }
    }

    FontLoader {
        id: inter

        source: root.assetBase + "InterVariable.ttf"
    }
}
