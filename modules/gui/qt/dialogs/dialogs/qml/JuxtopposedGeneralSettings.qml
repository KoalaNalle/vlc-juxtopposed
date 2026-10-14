// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
// Original visual design and SVG artwork: Juxtopposed, CC BY 4.0.
import QtQuick
import QtQuick.Controls.Basic as C
import QtQuick.Layouts

FocusScope {
    id: root
    objectName: "juxtopposedGeneralSettings"
    required property var preferences
    property var appearance: null
    property real uiScale: 1
    property url assetBase: "qrc:/juxtopposed/"
    property string searchText: ""
    // The modal shell supplies its complete focus order. The page can also be
    // embedded by another host without depending on global window objects.
    property var focusNavigator: null
    signal advancedRequested()
    readonly property var values: preferences ? preferences.values : ({})
    readonly property bool available: !!preferences
    readonly property bool stacked: width < 650 * uiScale
    readonly property var accentColors: ["#D37918", "#E15032", "#C49E23", "#19A234", "#18AAD3", "#185DD3", "#8E42FF", "#F052B3", "#AFAFAF"]
    readonly property var accentNames: [qsTr("Orange"), qsTr("Red"), qsTr("Yellow"), qsTr("Green"), qsTr("Cyan"), qsTr("Blue"), qsTr("Purple"), qsTr("Pink"), qsTr("Gray")]
    readonly property color accent: appearance ? appearance.accent : "#D37918"
    readonly property color accentSelected: appearance ? appearance.accentSelected : "#371F06"
    readonly property color accentDim: appearance ? appearance.accentDim : "#AB6314"
    readonly property int accentFocusIndex: Number.isInteger(values.accentIndex) && values.accentIndex >= 0 && values.accentIndex < 9 ? values.accentIndex : 0
    readonly property var focusTargets: [languageChoice, themeDark, themeLight, themeSystem,
        accentRepeater.itemAt(accentFocusIndex), dialogsToggle, updatesToggle,
        developerToggle, loggingToggle, logFileField, browseButton, detailsChoice, advancedButton]
    implicitHeight: form.implicitHeight

    function tone(value) { return appearance ? appearance.color(value) : value }
    function matches(text) { return !searchText.trim().length || text.toLocaleLowerCase().includes(searchText.trim().toLocaleLowerCase()) }
    function sectionMatches(names) { return names.some(name => matches(name)) }
    function navigate(item, backwards) { if (focusNavigator) focusNavigator(item, backwards) }
    function write(key, value) { return available && preferences.setValue(key, value) }
    function reveal(item) { if (item.activeFocus) focusRevealed(item) }
    signal focusRevealed(var item)
    FontLoader { id: inter; source: root.assetBase + "InterVariable.ttf" }

    component SettingLabel: Text {
        color: root.tone("#8F8F8F")
        font.family: inter.name
        font.pixelSize: 14 * root.uiScale
        textFormat: Text.PlainText
        wrapMode: Text.WordWrap
    }
    component SettingButton: C.Button {
        id: button
        implicitHeight: 32 * root.uiScale
        implicitWidth: 100 * root.uiScale
        activeFocusOnTab: true
        font.family: inter.name
        font.pixelSize: 14 * root.uiScale
        padding: 0
        background: Rectangle {
            color: root.tone(button.down ? "#222222" : "#191919")
            radius: 6 * root.uiScale
            border.width: button.visualFocus ? root.uiScale : 0
            border.color: root.accent
        }
        contentItem: SettingLabel {
            text: button.text
            font: button.font
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        Keys.onTabPressed: event => { if (root.focusNavigator) { root.navigate(button, false); event.accepted = true } }
        Keys.onBacktabPressed: event => { if (root.focusNavigator) { root.navigate(button, true); event.accepted = true } }
        onActiveFocusChanged: root.reveal(button)
    }
    component SettingRow: Item {
        id: row
        required property string title
        property string help: ""
        property string infoName: ""
        property real controlWidth: 320 * root.uiScale
        property real controlHeight: 36 * root.uiScale
        default property alias controls: controlHolder.data
        implicitHeight: root.stacked ? rowLabel.implicitHeight + controlHeight + 20 * root.uiScale : Math.max(rowLabel.implicitHeight, controlHeight) + 10 * root.uiScale
        width: parent.width
        Row {
            id: labelGroup
            x: 0
            y: root.stacked ? 5 * root.uiScale : (row.height - height) / 2
            spacing: 10 * root.uiScale
            width: root.stacked ? row.width : Math.max(0, row.width - row.controlWidth - 18 * root.uiScale)
            height: Math.max(rowLabel.implicitHeight, info.visible ? info.height : 0)
            SettingLabel {
                id: rowLabel
                text: row.title
                width: Math.min(implicitWidth, Math.max(0, labelGroup.width - (info.visible ? info.width + labelGroup.spacing : 0)))
                anchors.verticalCenter: parent.verticalCenter
            }
            C.Button {
                id: info
                objectName: row.infoName
                property bool tipOpen: false
                width: 18 * root.uiScale; height: 18 * root.uiScale
                anchors.verticalCenter: parent.verticalCenter
                visible: row.help.length > 0
                activeFocusOnTab: false
                padding: 0
                Accessible.name: qsTr("About %1").arg(row.title)
                background: Item {}
                contentItem: Item {
                    Image {
                        objectName: row.infoName + "Original"
                        anchors.centerIn: parent
                        width: 18; height: 18; scale: root.uiScale
                        source: root.assetBase + "global-settings-1f9fe.svg"
                        Accessible.ignored: true
                    }
                }
                onClicked: tipOpen = !tipOpen
                C.ToolTip.visible: hovered || visualFocus || tipOpen
                C.ToolTip.text: row.help
                C.ToolTip.timeout: 12000
                onVisibleChanged: if (!visible) tipOpen = false
            }
        }
        Item {
            id: controlHolder
            x: root.stacked ? 0 : row.width - width
            y: root.stacked ? labelGroup.y + labelGroup.height + 10 * root.uiScale : (row.height - height) / 2
            width: root.stacked ? row.width : Math.min(row.controlWidth, row.width)
            height: row.controlHeight
        }
    }
    component Choice: C.ComboBox {
        id: choice
        property string valueKey
        property var options: []
        property string accessibleTitle
        anchors.fill: parent
        model: options
        textRole: "name"
        valueRole: "value"
        currentIndex: indexForValue()
        enabled: root.available
        activeFocusOnTab: true
        font.family: inter.name
        font.pixelSize: 14 * root.uiScale
        Accessible.name: accessibleTitle
        Accessible.description: (currentIndex < 0 ? qsTr("The current setting has no matching preset. Choose a value to replace it.") + " " : "") + (parent.parent.help || "")
        displayText: currentIndex >= 0 ? currentText : qsTr("Current default / custom")
        leftPadding: 16 * root.uiScale; rightPadding: 48 * root.uiScale
        topPadding: 4 * root.uiScale; bottomPadding: 4 * root.uiScale
        function indexForValue() {
            const current = root.values[valueKey]
            for (let i = 0; i < options.length; ++i)
                if (options[i].value === current) return i
            return -1
        }
        function sync() { currentIndex = indexForValue() }
        Connections {
            target: root.preferences
            function onValuesChanged() { choice.sync() }
        }
        onOptionsChanged: sync()
        onActivated: index => {
            if (index >= 0 && index < options.length) root.write(valueKey, options[index].value)
            sync()
        }
        onActiveFocusChanged: root.reveal(choice)
        Keys.onTabPressed: event => { if (root.focusNavigator && !popup.visible) { root.navigate(choice, false); event.accepted = true } }
        Keys.onBacktabPressed: event => { if (root.focusNavigator && !popup.visible) { root.navigate(choice, true); event.accepted = true } }
        background: Rectangle {
            color: root.tone("#090909"); radius: 6 * root.uiScale
            border.width: choice.visualFocus ? root.uiScale : 0; border.color: root.accent
            Rectangle { anchors.fill: parent; anchors.margins: 2 * root.uiScale; color: root.tone("#191919"); radius: 4 * root.uiScale }
        }
        contentItem: SettingLabel { text: choice.displayText; font: choice.font; elide: Text.ElideRight; wrapMode: Text.NoWrap; verticalAlignment: Text.AlignVCenter }
        indicator: Item {
            objectName: choice.objectName + "ChevronSlot"
            width: 24 * root.uiScale; height: 24 * root.uiScale
            x: choice.width - width - 16 * root.uiScale; y: (choice.height - height) / 2
            Image {
                objectName: choice.objectName + "OriginalChevron"
                anchors.centerIn: parent
                width: 24; height: 24; scale: root.uiScale
                // The original Figma down-chevron slot mirrors the supplied SVG.
                mirror: true
                source: root.assetBase + "global-settings-764c9.svg"
                Accessible.ignored: true
            }
        }
        delegate: C.ItemDelegate {
            required property var modelData
            required property int index
            width: choice.width
            text: modelData.name
            font: choice.font
            highlighted: choice.highlightedIndex === index
            contentItem: SettingLabel { text: modelData.name; elide: Text.ElideRight; verticalAlignment: Text.AlignVCenter }
            background: Rectangle { color: root.tone(parent.highlighted ? "#222222" : "#191919") }
        }
        popup: C.Popup {
            objectName: choice.objectName + "Popup"
            y: choice.height + 2 * root.uiScale
            width: choice.width
            height: Math.min(contentItem.implicitHeight + 4 * root.uiScale, 240 * root.uiScale)
            padding: 2 * root.uiScale
            contentItem: ListView {
                clip: true
                implicitHeight: contentHeight
                model: choice.popup.visible ? choice.delegateModel : null
                currentIndex: choice.highlightedIndex
                C.ScrollBar.vertical: C.ScrollBar {}
            }
            background: Rectangle { color: root.tone("#191919"); border.width: root.uiScale; border.color: root.tone("#404040"); radius: 4 * root.uiScale }
        }
    }
    component Toggle: SettingButton {
        id: toggle
        required property string valueKey
        required property string accessibleTitle
        readonly property bool selected: root.values[valueKey] === true
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        width: 68 * root.uiScale; height: 32 * root.uiScale
        enabled: root.available
        Accessible.name: accessibleTitle
        Accessible.role: Accessible.CheckBox
        Accessible.checkable: true
        Accessible.checked: selected
        Accessible.description: parent.parent.help
        onClicked: root.write(valueKey, !selected)
        background: Rectangle {
            color: root.tone("#090909"); radius: 6 * root.uiScale
            border.width: toggle.visualFocus ? root.uiScale : 0; border.color: root.accent
        }
        contentItem: Item {
            Rectangle { x: (toggle.selected ? 3 : 32) * root.uiScale; y: 3 * root.uiScale; width: 33 * root.uiScale; height: 26 * root.uiScale; radius: 3 * root.uiScale; color: toggle.selected ? root.accentSelected : root.tone("#191919") }
            Rectangle { x: (toggle.selected ? 39 : 3) * root.uiScale; y: 3 * root.uiScale; width: 26 * root.uiScale; height: 26 * root.uiScale; radius: 3 * root.uiScale; color: toggle.selected ? root.accentDim : root.tone("#404040") }
            SettingLabel { x: (toggle.selected ? 10 : 36) * root.uiScale; anchors.verticalCenter: parent.verticalCenter; text: toggle.selected ? qsTr("ON") : qsTr("OFF"); color: toggle.selected ? root.accent : root.tone("#6B6B6B"); font.pixelSize: 12 * root.uiScale }
        }
    }
    component SectionHeading: SettingLabel {
        width: parent.width
        height: 39 * root.uiScale
        color: root.tone("#C6C6C6")
        font.pixelSize: 16 * root.uiScale
        font.weight: Font.Medium
        verticalAlignment: Text.AlignVCenter
    }
    Column {
        id: form
        width: root.width
        spacing: 20 * root.uiScale
        Column {
            width: parent.width
            visible: root.sectionMatches([qsTr("Appearance"), qsTr("Interface language"), qsTr("Theme"), qsTr("Accent color")])
            SectionHeading { text: qsTr("Appearance") }
            SettingRow {
                visible: root.matches(title) || root.matches(qsTr("Appearance"))
                title: qsTr("Interface language")
                Choice {
                    id: languageChoice
                    objectName: "generalLanguage"
                    valueKey: "language"
                    accessibleTitle: parent.parent.title
                    options: root.preferences ? root.preferences.languages.map(language => ({name: language.name, value: language.code})) : []
                }
            }
            SettingRow {
                visible: root.matches(title) || root.matches(qsTr("Appearance"))
                title: qsTr("Theme")
                Rectangle {
                    anchors.fill: parent
                    color: root.tone("#090909"); radius: 6 * root.uiScale
                    RowLayout {
                        anchors.fill: parent; anchors.margins: 2 * root.uiScale; spacing: 2 * root.uiScale
                        SettingButton {
                            id: themeDark; objectName: "generalThemeDark"
                            text: qsTr("Dark"); readonly property bool selected: root.values.theme === 2
                            Layout.fillWidth: true; Layout.fillHeight: true
                            enabled: root.available
                            Accessible.role: Accessible.RadioButton; Accessible.checkable: true; Accessible.checked: selected
                            onClicked: root.write("theme", 2)
                            Keys.onRightPressed: { root.write("theme", 1); themeLight.forceActiveFocus(Qt.TabFocusReason) }
                            Keys.onLeftPressed: { root.write("theme", 0); themeSystem.forceActiveFocus(Qt.BacktabFocusReason) }
                            background: Rectangle { color: themeDark.selected ? root.accentSelected : root.tone("#191919"); radius: 4 * root.uiScale; border.width: themeDark.visualFocus ? root.uiScale : 0; border.color: root.accent }
                            contentItem: SettingLabel { text: themeDark.text; color: themeDark.selected ? root.accent : root.tone("#6B6B6B"); horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }
                        SettingButton {
                            id: themeLight; objectName: "generalThemeLight"
                            text: qsTr("Light"); readonly property bool selected: root.values.theme === 1
                            Layout.fillWidth: true; Layout.fillHeight: true
                            enabled: root.available
                            Accessible.role: Accessible.RadioButton; Accessible.checkable: true; Accessible.checked: selected
                            onClicked: root.write("theme", 1)
                            Keys.onRightPressed: { root.write("theme", 0); themeSystem.forceActiveFocus(Qt.TabFocusReason) }
                            Keys.onLeftPressed: { root.write("theme", 2); themeDark.forceActiveFocus(Qt.BacktabFocusReason) }
                            background: Rectangle { color: themeLight.selected ? root.accentSelected : root.tone("#191919"); radius: 4 * root.uiScale; border.width: themeLight.visualFocus ? root.uiScale : 0; border.color: root.accent }
                            contentItem: SettingLabel { text: themeLight.text; color: themeLight.selected ? root.accent : root.tone("#6B6B6B"); horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }
                        SettingButton {
                            id: themeSystem; objectName: "generalThemeSystem"
                            text: qsTr("System"); readonly property bool selected: root.values.theme === 0
                            Layout.fillWidth: true; Layout.fillHeight: true
                            enabled: root.available
                            Accessible.role: Accessible.RadioButton; Accessible.checkable: true; Accessible.checked: selected
                            onClicked: root.write("theme", 0)
                            Keys.onRightPressed: { root.write("theme", 2); themeDark.forceActiveFocus(Qt.TabFocusReason) }
                            Keys.onLeftPressed: { root.write("theme", 1); themeLight.forceActiveFocus(Qt.BacktabFocusReason) }
                            background: Rectangle { color: themeSystem.selected ? root.accentSelected : root.tone("#191919"); radius: 4 * root.uiScale; border.width: themeSystem.visualFocus ? root.uiScale : 0; border.color: root.accent }
                            contentItem: SettingLabel { text: themeSystem.text; color: themeSystem.selected ? root.accent : root.tone("#6B6B6B"); horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }
                    }
                }
            }
            SettingRow {
                visible: root.matches(title) || root.matches(qsTr("Appearance"))
                title: qsTr("Accent color")
                Rectangle {
                    anchors.fill: parent; color: root.tone("#090909"); radius: 6 * root.uiScale
                    RowLayout {
                        anchors.fill: parent; anchors.margins: 2 * root.uiScale; spacing: 4 * root.uiScale
                        Repeater {
                            id: accentRepeater
                            model: root.accentColors
                            delegate: SettingButton {
                                id: swatch
                                required property int index
                                required property string modelData
                                readonly property bool selected: root.values.accentIndex === index
                                objectName: "generalAccent" + index
                                Layout.fillWidth: true; Layout.fillHeight: true
                                enabled: root.available
                                Accessible.name: qsTr("%1 accent color").arg(root.accentNames[index])
                                Accessible.role: Accessible.RadioButton; Accessible.checkable: true; Accessible.checked: selected
                                onClicked: root.write("accentIndex", index)
                                Keys.onRightPressed: { const next = (index + 1) % 9; root.write("accentIndex", next); accentRepeater.itemAt(next).forceActiveFocus(Qt.TabFocusReason) }
                                Keys.onLeftPressed: { const next = (index + 8) % 9; root.write("accentIndex", next); accentRepeater.itemAt(next).forceActiveFocus(Qt.BacktabFocusReason) }
                                background: Rectangle { color: swatch.selected ? root.accentSelected : "transparent"; radius: 4 * root.uiScale; border.width: swatch.visualFocus ? root.uiScale : 0; border.color: root.accent }
                                contentItem: Item { Rectangle { anchors.fill: parent; anchors.margins: 6 * root.uiScale; color: swatch.modelData; opacity: swatch.selected ? 1 : .7; radius: root.uiScale } }
                            }
                        }
                    }
                }
            }
        }
        Column {
            width: parent.width
            visible: root.sectionMatches([qsTr("Startup"), qsTr("Show confirmation and error dialogs"), qsTr("Check for updates automatically")])
            SectionHeading { text: qsTr("Startup") }
            SettingRow {
                title: qsTr("Show confirmation and error dialogs")
                help: qsTr("Show optional VLC confirmation and error dialogs. File pickers and required confirmations remain available.")
                infoName: "generalDialogsInfo"
                visible: root.matches(title) || root.matches(qsTr("Startup"))
                controlHeight: 32 * root.uiScale
                Toggle { id: dialogsToggle; objectName: "generalDialogs"; valueKey: "showDialogs"; accessibleTitle: qsTr("Show confirmation and error dialogs") }
            }
            SettingRow {
                title: qsTr("Check for updates automatically")
                help: root.preferences && root.preferences.updatesAvailable ? qsTr("Check for VLC updates automatically.") : qsTr("Automatic update checking is unavailable in this build.")
                infoName: "generalUpdatesInfo"
                visible: root.matches(title) || root.matches(qsTr("Startup"))
                controlHeight: 32 * root.uiScale
                Toggle { id: updatesToggle; objectName: "generalUpdates"; valueKey: "autoUpdates"; accessibleTitle: qsTr("Check for updates automatically"); enabled: root.available && root.preferences.updatesAvailable; Accessible.description: parent.parent.help }
            }
        }
        Column {
            width: parent.width
            visible: root.sectionMatches([qsTr("Diagnostics"), qsTr("Developer mode"), qsTr("Save a log"), qsTr("Log file"), qsTr("Log details")])
            SectionHeading { text: qsTr("Diagnostics") }
            SettingRow {
                title: qsTr("Developer mode"); help: qsTr("Enable detailed Qt interface diagnostics after restarting VLC."); infoName: "generalDeveloperInfo"
                visible: root.matches(title) || root.matches(qsTr("Diagnostics"))
                controlHeight: 32 * root.uiScale
                Toggle { id: developerToggle; objectName: "generalDeveloper"; valueKey: "developerMode"; accessibleTitle: qsTr("Developer mode") }
            }
            SettingRow {
                title: qsTr("Save a log"); help: qsTr("Write VLC messages to a log file after restarting VLC."); infoName: "generalLoggingInfo"
                visible: root.matches(title) || root.matches(qsTr("Diagnostics"))
                controlHeight: 32 * root.uiScale
                Toggle { id: loggingToggle; objectName: "generalLogging"; valueKey: "loggingEnabled"; accessibleTitle: qsTr("Save a log") }
            }
            SettingRow {
                title: qsTr("Log file"); help: qsTr("Choose the file used by VLC file logging after restart."); infoName: "generalLogFileInfo"
                visible: root.matches(title) || root.matches(qsTr("Diagnostics"))
                RowLayout {
                    anchors.fill: parent; spacing: 10 * root.uiScale
                    C.TextField {
                        id: logFileField; objectName: "generalLogFile"
                        Layout.fillWidth: true; Layout.minimumWidth: 0; Layout.fillHeight: true
                        enabled: root.available && root.values.loggingEnabled === true
                        text: root.values.logFile || ""
                        selectByMouse: true; activeFocusOnTab: true
                        color: root.tone("#8F8F8F"); selectionColor: root.accentSelected
                        leftPadding: 12 * root.uiScale; rightPadding: 18 * root.uiScale
                        font.family: inter.name; font.pixelSize: 14 * root.uiScale
                        Accessible.name: qsTr("Log file")
                        onEditingFinished: { if (text !== root.values.logFile) root.write("logFile", text); text = Qt.binding(() => root.values.logFile || "") }
                        onActiveFocusChanged: root.reveal(logFileField)
                        Keys.onTabPressed: event => { if (root.focusNavigator) { root.navigate(logFileField, false); event.accepted = true } }
                        Keys.onBacktabPressed: event => { if (root.focusNavigator) { root.navigate(logFileField, true); event.accepted = true } }
                        background: Rectangle { color: root.tone("#090909"); radius: 6 * root.uiScale; border.width: 2 * root.uiScale; border.color: logFileField.activeFocus ? root.accent : root.tone("#191919") }
                    }
                    SettingButton {
                        id: browseButton; objectName: "generalBrowseLog"
                        Layout.preferredWidth: 100 * root.uiScale; Layout.preferredHeight: 32 * root.uiScale
                        enabled: root.available && root.values.loggingEnabled === true
                        text: qsTr("Browse"); font.weight: Font.DemiBold
                        Accessible.name: qsTr("Choose log file")
                        onClicked: root.preferences.chooseLogFile()
                    }
                }
            }
            SettingRow {
                title: qsTr("Log details"); help: qsTr("Select which messages VLC writes to the log after restarting."); infoName: "generalLogDetailsInfo"
                visible: root.matches(title) || root.matches(qsTr("Diagnostics"))
                Choice {
                    id: detailsChoice; objectName: "generalLogDetails"
                    valueKey: "logDetails"; accessibleTitle: qsTr("Log details")
                    enabled: root.available && root.values.loggingEnabled === true
                    options: [{name: qsTr("Errors only"), value: 0}, {name: qsTr("Errors and warnings"), value: 1}, {name: qsTr("All messages"), value: 2}]
                }
            }
        }
        SettingButton {
            id: advancedButton; objectName: "generalAdvanced"
            visible: root.matches(qsTr("Advanced Options"))
            width: parent.width; height: 38 * root.uiScale
            Accessible.name: qsTr("Advanced Options")
            onClicked: root.advancedRequested()
            background: Rectangle { color: root.tone("#191919"); opacity: .95; radius: 2 * root.uiScale; border.width: advancedButton.visualFocus ? root.uiScale : 0; border.color: root.accent }
            contentItem: Item {
                Row {
                    anchors.centerIn: parent; spacing: 10 * root.uiScale
                    SettingLabel { text: qsTr("Advanced Options"); anchors.verticalCenter: parent.verticalCenter }
                    Item {
                        objectName: "generalAdvancedChevronSlot"
                        width: 24 * root.uiScale; height: 24 * root.uiScale
                        Image { objectName: "generalAdvancedOriginalChevron"; anchors.centerIn: parent; width: 24; height: 24; scale: root.uiScale; mirror: true; source: root.assetBase + "global-settings-97863.svg"; Accessible.ignored: true }
                    }
                }
            }
        }
    }
}
