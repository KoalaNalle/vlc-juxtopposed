// SPDX-License-Identifier: GPL-2.0-or-later
// Shared settings geometry/typography from the original Juxtopposed design.
// Original artwork remains unchanged in the calling page's asset namespace.
import QtQuick
import QtQuick.Controls.Basic as C
Item {
    component SettingLabel: Text {
        id: sharedLabel
        required property var settings
        color: sharedLabel.settings.tone("#8F8F8F")
        font.family: sharedLabel.settings.settingsFontFamily
        font.pixelSize: 14 * sharedLabel.settings.uiScale
        textFormat: Text.PlainText
        wrapMode: Text.WordWrap
    }
    component SettingButton: C.Button {
        id: button
        required property var settings
        enabled: !button.settings.busy
        implicitHeight: 32 * button.settings.uiScale
        implicitWidth: 100 * button.settings.uiScale
        activeFocusOnTab: true
        font.family: button.settings.settingsFontFamily
        font.pixelSize: 14 * button.settings.uiScale
        padding: 0
        background: Rectangle {
            color: button.settings.tone(button.down ? "#222222" : "#191919")
            radius: 6 * button.settings.uiScale
            border.width: button.visualFocus ? button.settings.uiScale : 0
            border.color: button.settings.accent
        }
        contentItem: SettingLabel {
            settings: button.settings
            text: button.text
            font: button.font
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        Keys.onTabPressed: event => { if (button.settings.focusNavigator) { button.settings.navigate(button, false); event.accepted = true } }
        Keys.onBacktabPressed: event => { if (button.settings.focusNavigator) { button.settings.navigate(button, true); event.accepted = true } }
        onActiveFocusChanged: button.settings.reveal(button)
    }
    component SettingRow: Item {
        id: row
        required property var settings
        property bool showHelpIcon: true
        property string infoAssetName: "privacy-settings-1f9fe.svg"
        property real minimumRowHeight: 0
        required property string title
        property string help: ""
        property string infoName: ""
        property alias helpButton:info
        property real controlWidth: 320 * row.settings.uiScale
        property real controlHeight: 36 * row.settings.uiScale
        default property alias controls: controlHolder.data
        implicitHeight: row.settings.stacked ? rowLabel.implicitHeight + controlHeight + 20 * row.settings.uiScale : Math.max(minimumRowHeight, rowLabel.implicitHeight, controlHeight) + 10 * row.settings.uiScale
        width: parent.width
        Row {
            id: labelGroup
            x: 0
            y: row.settings.stacked ? 5 * row.settings.uiScale : (row.height - height) / 2
            spacing: 10 * row.settings.uiScale
            width: row.settings.stacked ? row.width : Math.max(0, row.width - row.controlWidth - 18 * row.settings.uiScale)
            height: Math.max(rowLabel.implicitHeight, info.visible ? info.height : 0)
            SettingLabel {
                id: rowLabel
                settings: row.settings
                text: row.title
                width: Math.min(implicitWidth, Math.max(0, labelGroup.width - (info.visible ? info.width + labelGroup.spacing : 0)))
                anchors.verticalCenter: parent.verticalCenter
            }
            C.Button {
                id: info
                objectName: row.infoName
                property bool tipOpen: false
                width: 18 * row.settings.uiScale; height: 18 * row.settings.uiScale
                anchors.verticalCenter: parent.verticalCenter
                visible: row.showHelpIcon && row.help.length > 0
                activeFocusOnTab: true
                Accessible.description:row.help
                padding: 0
                Accessible.name: qsTr("About %1").arg(row.title)
                background: Item {}
                contentItem: Item {
                    Image {
                        objectName: row.infoName + "Original"
                        anchors.centerIn: parent
                        width: 18; height: 18; scale: row.settings.uiScale
                        source: row.settings.assetBase + row.infoAssetName
                        Accessible.ignored: true
                    }
                }
                onClicked: tipOpen = !tipOpen
                onActiveFocusChanged:row.settings.reveal(info)
                Keys.onTabPressed:event=>{row.settings.navigate(info,false);event.accepted=true}
                Keys.onBacktabPressed:event=>{row.settings.navigate(info,true);event.accepted=true}
                C.ToolTip.visible: hovered || visualFocus || tipOpen
                C.ToolTip.text: row.help
                C.ToolTip.timeout: 12000
                onVisibleChanged: if (!visible) tipOpen = false
            }
        }
        Item {
            id: controlHolder
            x: row.settings.stacked ? 0 : row.width - width
            y: row.settings.stacked ? labelGroup.y + labelGroup.height + 10 * row.settings.uiScale : (row.height - height) / 2
            width: row.settings.stacked ? row.width : Math.min(row.controlWidth, row.width)
            height: row.controlHeight
        }
    }
    component SectionHeading: SettingLabel {
        id: heading
        width: parent.width
        height: 39 * heading.settings.uiScale
        color: heading.settings.tone("#C6C6C6")
        font.pixelSize: 16 * heading.settings.uiScale
        font.weight: Font.Medium
        verticalAlignment: Text.AlignVCenter
    }
    component Toggle: SettingButton {
        id: toggle
        FeedbackTip{settings:toggle.settings;control:toggle;valueKey:toggle.valueKey}
        required property string valueKey
        required property string accessibleTitle
        readonly property bool custom:typeof toggle.settings.values[valueKey]!=="boolean"
        readonly property bool selected: toggle.settings.values[valueKey] === true
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        width: 68 * toggle.settings.uiScale; height: 32 * toggle.settings.uiScale
        enabled: toggle.settings.available && !toggle.settings.busy
        Accessible.name: accessibleTitle
        Accessible.role: custom ? Accessible.Button : Accessible.CheckBox
        Accessible.checkable: !custom
        Accessible.checked: selected
        Accessible.description: (custom?qsTr("Custom saved value. Choose On or Off to replace it.")+" ":"")+toggle.settings.helpFor(valueKey)+(toggle.settings.failedKey===valueKey&&toggle.settings.service?" "+toggle.settings.service.lastError:"")
        property alias customPopup:customMenu
        onClicked:if(custom)customMenu.open();else toggle.settings.write(valueKey,!selected)
        Keys.onLeftPressed:toggle.settings.write(valueKey,false)
        Keys.onRightPressed:toggle.settings.write(valueKey,true)
        C.Menu {
            id:customMenu;objectName:toggle.objectName+"CustomMenu";parent:toggle;focus:true
            y:toggle.height+2*toggle.settings.uiScale;width:Math.max(120*toggle.settings.uiScale,toggle.width)
            font.family:toggle.settings.settingsFontFamily;font.pixelSize:14*toggle.settings.uiScale
            C.MenuItem{objectName:toggle.objectName+"CustomOn";text:qsTr("On");onTriggered:toggle.settings.write(toggle.valueKey,true)}
            C.MenuItem{objectName:toggle.objectName+"CustomOff";text:qsTr("Off");onTriggered:toggle.settings.write(toggle.valueKey,false)}
        }
        background: Rectangle {
            color: toggle.settings.tone("#090909"); radius: 6 * toggle.settings.uiScale
            border.width: toggle.visualFocus ? toggle.settings.uiScale : 0; border.color: toggle.settings.accent
        }
        contentItem: Item {
            Rectangle { visible:!toggle.custom; x: (toggle.selected ? 3 : 32) * toggle.settings.uiScale; y: 3 * toggle.settings.uiScale; width: 33 * toggle.settings.uiScale; height: 26 * toggle.settings.uiScale; radius: 3 * toggle.settings.uiScale; color: toggle.selected ? toggle.settings.accentSelected : toggle.settings.tone("#191919") }
            Rectangle { visible:!toggle.custom; x: (toggle.selected ? 39 : 3) * toggle.settings.uiScale; y: 3 * toggle.settings.uiScale; width: 26 * toggle.settings.uiScale; height: 26 * toggle.settings.uiScale; radius: 3 * toggle.settings.uiScale; color: toggle.selected ? toggle.settings.accentDim : toggle.settings.tone("#404040") }
            SettingLabel { settings:toggle.settings; width:toggle.custom?parent.width:26*toggle.settings.uiScale;wrapMode:Text.NoWrap;elide:Text.ElideRight;horizontalAlignment:toggle.custom?Text.AlignHCenter:Text.AlignLeft; x: toggle.custom?0:(toggle.selected ? 10 : 36) * toggle.settings.uiScale; anchors.verticalCenter: parent.verticalCenter; text:toggle.custom?qsTr("Custom"):toggle.selected ? qsTr("ON") : qsTr("OFF"); color: toggle.selected ? toggle.settings.accent : toggle.settings.tone("#6B6B6B"); font.pixelSize: 12 * toggle.settings.uiScale }
        }
    }
    component FeedbackTip:C.ToolTip {
        id:tip
        required property var settings
        required property Item control
        required property string valueKey
        objectName:control.objectName+"ValidationError";parent:control
        y:-height-4*tip.settings.uiScale;width:Math.min(Math.max(control.width,200*tip.settings.uiScale),tip.settings.width)
        visible:tip.settings.failedKey===valueKey&&tip.settings.service&&tip.settings.service.lastError.length>0&&control.activeFocus&&control.visible
        text:tip.settings.service?tip.settings.service.lastError:"";focus:false;closePolicy:C.Popup.NoAutoClose;timeout:0;padding:8*tip.settings.uiScale
        function reveal(){if(!visible)return;tip.settings.reveal(control);Qt.callLater(()=>{if(!visible)return;const p=control.mapToItem(root,0,0);x=Math.max(-p.x,Math.min(0,tip.settings.width-p.x-width));if(background)tip.settings.focusRevealed(background)})}
        onVisibleChanged:if(visible)Qt.callLater(reveal)
        Connections{target:tip.settings.formItem;function onImplicitHeightChanged(){if(tip.visible)Qt.callLater(tip.reveal)}}
        contentItem:SettingLabel{settings:tip.settings;text:tip.text;color:"#D98779";Accessible.role:Accessible.StaticText;Accessible.name:text}
        background:Rectangle{color:tip.settings.tone("#191919");radius:4*tip.settings.uiScale;border.width:tip.settings.uiScale;border.color:"#D98779"}
    }
}
