// SPDX-License-Identifier: GPL-2.0-or-later
// Shared native Qt settings dropdown; original arrow bytes are supplied by each page.
import QtQuick
import QtQuick.Controls.Basic as C
C.ComboBox {
        required property var settings
        required property string arrowAssetName
        id: choice
        required property string valueKey
        required property string accessibleTitle
        readonly property var entries: choice.settings.service ? choice.settings.service.choices[valueKey] || [] : []
        anchors.fill: parent
        model: entries; textRole: "name"; valueRole: "value"
        enabled: choice.settings.editable(valueKey); activeFocusOnTab: true
        opacity: enabled ? 1 : .5
        padding: 2 * choice.settings.uiScale
        font.family: choice.settings.settingsFontFamily; font.pixelSize: 14 * choice.settings.uiScale
        currentIndex: { for (let i = 0; i < entries.length; ++i) if (entries[i].value === choice.settings.values[valueKey]) return i; return -1 }
        displayText: choice.settings.choiceName(valueKey, choice.settings.values[valueKey])
        Accessible.name: accessibleTitle; Accessible.description: choice.settings.helpFor(valueKey)
        onActivated: index => choice.settings.write(valueKey, entries[index].value)
        onActiveFocusChanged: choice.settings.reveal(choice)
        onVisibleChanged: if (!visible) popup.close()
        Keys.onTabPressed: event => { if (choice.settings.focusNavigator) { choice.settings.navigate(choice, false); event.accepted = true } }
        Keys.onBacktabPressed: event => { if (choice.settings.focusNavigator) { choice.settings.navigate(choice, true); event.accepted = true } }
        indicator: Item {}
        background: Rectangle { color: choice.settings.tone("#090909"); radius: 6 * choice.settings.uiScale; border.width: choice.visualFocus ? choice.settings.uiScale : 0; border.color: choice.settings.accent }
        contentItem: Rectangle {
            radius: 4 * choice.settings.uiScale; color: choice.settings.tone("#191919")
            JuxtopposedSettingsFields.SettingLabel {
                settings: choice.settings
                x: 14 * choice.settings.uiScale; anchors.verticalCenter: parent.verticalCenter
                width: Math.max(0, parent.width - 62 * choice.settings.uiScale)
                text: choice.displayText; elide: Text.ElideRight; wrapMode: Text.NoWrap
            }
            Item {
                width: 24 * choice.settings.uiScale; height: width; anchors.right: parent.right; anchors.rightMargin: 14 * choice.settings.uiScale; anchors.verticalCenter: parent.verticalCenter
                Image { objectName: choice.objectName + "OriginalArrow"; anchors.centerIn: parent; width: 24; height: 24; scale: choice.settings.uiScale; source: choice.settings.assetBase + choice.arrowAssetName; Accessible.ignored: true }
            }
        }
        delegate: C.ItemDelegate {
            required property var modelData
            required property int index
            width: choice.width - 4 * choice.settings.uiScale; height: 32 * choice.settings.uiScale
            text: choice.settings.choiceName(choice.valueKey, modelData.value)
            font: choice.font; highlighted: choice.highlightedIndex === index
            contentItem: JuxtopposedSettingsFields.SettingLabel { settings: choice.settings; text: parent.text; verticalAlignment: Text.AlignVCenter; leftPadding: 12 * choice.settings.uiScale }
            background: Rectangle { color: parent.highlighted ? choice.settings.accentSelected : choice.settings.tone("#191919"); radius: 4 * choice.settings.uiScale }
        }
        popup: C.Popup {
            y: choice.height + 2 * choice.settings.uiScale; width: choice.width; padding: 2 * choice.settings.uiScale
            implicitHeight: Math.min(contentItem.implicitHeight + padding * 2, 220 * choice.settings.uiScale)
            contentItem: ListView { clip: true; implicitHeight: contentHeight; model: choice.popup.visible ? choice.delegateModel : null; currentIndex: choice.highlightedIndex; C.ScrollIndicator.vertical: C.ScrollIndicator {} }
            background: Rectangle { color: choice.settings.tone("#191919"); radius: 6 * choice.settings.uiScale; border.width: choice.settings.uiScale; border.color: choice.settings.tone("#404040") }
        }
        JuxtopposedSettingsFields.FeedbackTip { settings: choice.settings; control: choice; valueKey: choice.valueKey }
    }
