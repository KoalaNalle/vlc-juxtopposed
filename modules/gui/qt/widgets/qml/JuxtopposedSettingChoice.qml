// Copyright (C) 2026 VLC authors and VideoLAN. GPLv2 or later.
// Visual design: Juxtopposed; original artwork: CC BY 4.0.
import QtQuick
import VLC.Style
import QtQuick.Controls.Basic as C
import QtQuick.Layouts
import QtQml.Models

C.Control {
    id: root

    property url assetBase: "qrc:/juxtopposed/"
    property bool available: true
    property var choices: null
    property alias selector: combo
    property string title: ""
    property real uiScale: 1

    function setCurrentItemFocus(reason) {
        combo.forceActiveFocus(reason);
    }
    function updateSelection() {
        let selected = -1;
        for (let i = 0; i < selection.count; ++i)
            if (selection.objectAt(i)?.checked) {
                selected = i;
                break;
            }
        combo.currentIndex = selected;
    }

    enabled: available && selection.count > 0
    implicitHeight: 44 * uiScale
    implicitWidth: 360 * uiScale
    padding: 0

    contentItem: RowLayout {
        spacing: 12 * root.uiScale

        C.Label {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            Layout.preferredWidth: root.width * 0.38
            color: JuxtopposedPalette.color("#969696")
            elide: Text.ElideRight
            font.family: inter.name
            font.pixelSize: 14 * root.uiScale
            text: root.title
        }
        C.ComboBox {
            id: combo

            Accessible.name: root.title
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            Layout.preferredHeight: 32 * root.uiScale
            Layout.preferredWidth: root.width * 0.62
            currentIndex: -1
            displayText: currentIndex >= 0 ? currentText : enabled ? qsTr("Custom") : qsTr("Unavailable")
            font.family: inter.name
            font.pixelSize: 14 * root.uiScale
            model: root.choices
            objectName: "settingChoiceSelector"
            padding: 10 * root.uiScale
            rightPadding: 24 * root.uiScale
            textRole: "display"

            background: Rectangle {
                border.color: combo.visualFocus ? JuxtopposedPalette.color("#D37918") : JuxtopposedPalette.color("#252525")
                border.width: root.uiScale
                color: JuxtopposedPalette.color("#161616")
                radius: 4 * root.uiScale
            }
            contentItem: C.Label {
                color: JuxtopposedPalette.color("#969696")
                elide: Text.ElideRight
                font: combo.font
                text: combo.displayText
                verticalAlignment: Text.AlignVCenter
            }
            indicator: C.Label {
                Accessible.ignored: true
                anchors.right: parent.right
                anchors.rightMargin: 8 * root.uiScale
                anchors.verticalCenter: parent.verticalCenter
                color: JuxtopposedPalette.color("#969696")
                font: combo.font
                text: "⌄"
            }

            onActivated: index => root.choices.toggleIndex(index)
        }
    }

    FontLoader {
        id: inter

        source: root.assetBase + "InterVariable.ttf"
    }
    Instantiator {
        id: selection

        model: root.choices

        delegate: QtObject {
            required property bool checked

            onCheckedChanged: Qt.callLater(root.updateSelection)
        }

        onObjectAdded: Qt.callLater(root.updateSelection)
        onObjectRemoved: Qt.callLater(root.updateSelection)
    }
}
