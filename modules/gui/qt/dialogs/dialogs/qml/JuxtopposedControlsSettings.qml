// SPDX-License-Identifier: GPL-2.0-or-later
// Original visual design and unchanged SVG artwork: Juxtopposed, CC BY 4.0.
import QtQuick
import QtQuick.Controls.Basic as C
FocusScope {
    id: root
    objectName: "juxtopposedControlsSettings"
    required property var preferences
    property var controlsPreferences: null
    readonly property var service: controlsPreferences
    property var appearance: null
    property real uiScale: 1
    property url assetBase: "qrc:/juxtopposed/"
    property string searchText: ""
    property var focusNavigator: null
    signal focusRevealed(var item)
    readonly property var values: service ? service.values : ({})
    readonly property var capabilities: service ? service.capabilities : ({})
    readonly property bool available: !!service
    readonly property bool busy: service ? service.busy : false
    readonly property bool stacked: width < 650 * uiScale
    readonly property color accent: appearance ? appearance.accent : "#D37918"
    readonly property color accentSelected: appearance ? appearance.accentSelected : "#371F06"
    readonly property color accentDim: appearance ? appearance.accentDim : "#AB6314"
    readonly property string settingsFontFamily: inter.name
    readonly property alias formItem: form
    property string failedKey: ""
    readonly property var rows: [shortcutsRow,resetRow,veryShortRow,shortRow,mediumRow,longRow,singleRow,doubleRow,wheelRow,horizontalRow,reverseRow,remoteRow,portRow,linkWheelRow,remoteWheelRow,remoteHorizontalRow,remoteReverseRow]
    readonly property var controls: [editShortcuts,resetShortcuts,veryShort,shortSkip,medium,longSkip,singleClick,doubleClick,scrollWheel,horizontalScroll,reverseScroll,webRemote,remotePort,linkWheelSettings,remoteScrollWheel,remoteHorizontalScroll,remoteReverseScroll]
    readonly property var focusTargets: controls
    readonly property var choices: [singleClick,doubleClick,scrollWheel,horizontalScroll,remoteScrollWheel,remoteHorizontalScroll]
    implicitHeight: form.implicitHeight
    function tone(value) { return appearance ? appearance.color(value) : value }
    function matches(text) { return !searchText.trim().length || text.toLocaleLowerCase().includes(searchText.trim().toLocaleLowerCase()) }
    function sectionMatches(texts) { return texts.some(text => matches(text)) }
    function supported(key) { return available && capabilities[key] === true }
    function editable(key) { return supported(key) && !busy }
    function navigate(item, backwards) { if (focusNavigator) focusNavigator(item, backwards) }
    function reveal(item) { if (item.activeFocus) focusRevealed(item) }
    function helpFor(key) {
        if (!service) return qsTr("This setting is unavailable in this build.")
        const group = key.endsWith("Skip") ? "skip" : key.startsWith("remote") || key === "webRemoteEnabled" || key === "linkWheelSettings" ? "remote" : key === "singleClick" || key === "doubleClick" ? "mouse" : "wheel"
        return service.supportNotes[group] || ""
    }
    function write(key, value) {
        if (!editable(key)) return false
        const accepted = service.setValue(key, value)
        failedKey = accepted ? "" : key
        return accepted
    }
    function action(key, method) {
        if (!editable(key)) return false
        const accepted = service[method]()
        failedKey = accepted ? "" : key
        return accepted
    }
    function choiceName(key, value) {
        if (typeof value !== "number") return qsTr("Unavailable")
        const names = key === "singleClick" || key === "doubleClick"
            ? {0:qsTr("Do nothing"),1:qsTr("Play / Pause"),2:qsTr("Toggle fullscreen")}
            : {"-1":qsTr("Do nothing"),0:qsTr("Volume"),2:qsTr("Seek"),3:qsTr("Seek (reversed)")}
        return names[value] || qsTr("Custom (%1)").arg(value)
    }
    function closeChoices() {
        for (const control of choices) control.popup.close()
        for (const control of [reverseScroll,webRemote,linkWheelSettings,remoteReverseScroll]) control.customPopup.close()
    }
    function focusActivePopup() {
        for (const control of choices) if (control.popup.visible) { control.popup.forceActiveFocus(); return true }
        return false
    }
    onVisibleChanged: { if (!visible) closeChoices(); else if (service) service.reload() }
    onBusyChanged: if (busy) closeChoices()
    FontLoader { id: inter; source: root.assetBase + "InterVariable.ttf" }
    component SettingLabel: JuxtopposedSettingsFields.SettingLabel { settings: root }
    component SettingButton: JuxtopposedSettingsFields.SettingButton { settings: root; font.weight: Font.Medium; opacity: enabled ? 1 : .5 }
    component SettingRow: JuxtopposedSettingsFields.SettingRow { settings: root; showHelpIcon: false; height: implicitHeight }
    component SectionHeading: JuxtopposedSettingsFields.SectionHeading { settings: root }
    component Toggle: JuxtopposedSettingsFields.Toggle { settings: root; enabled: root.editable(valueKey); opacity: enabled ? 1 : .5 }
    component FeedbackTip: JuxtopposedSettingsFields.FeedbackTip { settings: root }
    Connections { target: root.service; function onLastErrorChanged() { if (!root.service.lastError.length) root.failedKey = "" } }
    component NumberField: C.TextField {
        id: field
        required property string valueKey
        required property string accessibleTitle
        property bool seconds: true
        anchors.fill: parent
        activeFocusOnTab: true
        enabled: root.editable(valueKey)
        opacity: enabled ? 1 : .5
        selectByMouse: true
        font.family: inter.name; font.pixelSize: 14 * root.uiScale
        color: root.tone("#8F8F8F"); selectionColor: root.accentDim; selectedTextColor: root.tone("#C6C6C6")
        leftPadding: 12 * root.uiScale; rightPadding: 18 * root.uiScale
        topPadding: 4 * root.uiScale; bottomPadding: 4 * root.uiScale
        Accessible.name: accessibleTitle; Accessible.description: root.helpFor(valueKey)
        function sync() { text = typeof root.values[valueKey] === "number" ? String(root.values[valueKey]) + (seconds ? " s" : "") : "" }
        Component.onCompleted: sync()
        Connections { target: root.service; function onValuesChanged() { if (!field.activeFocus) field.sync() } }
        onEditingFinished: {
            const raw = text.trim()
            const numeric = seconds ? raw.replace(/\s*s$/, "") : raw
            const candidate = /^\d+$/.test(numeric) ? Number(numeric) : text
            if (candidate === root.values[valueKey] && root.failedKey !== valueKey) { sync(); return }
            if (root.editable(valueKey) && root.write(valueKey, candidate)) sync()
        }
        onActiveFocusChanged: root.reveal(field)
        Keys.onEscapePressed: event => { sync(); root.failedKey = ""; event.accepted = true }
        Keys.onTabPressed: event => { if (root.focusNavigator) { root.navigate(field, false); event.accepted = true } }
        Keys.onBacktabPressed: event => { if (root.focusNavigator) { root.navigate(field, true); event.accepted = true } }
        background: Rectangle {
            color: root.tone("#090909"); radius: 6 * root.uiScale; border.width: 2 * root.uiScale
            border.color: field.visualFocus ? root.accent : root.tone("#191919")
        }
        FeedbackTip { control: field; valueKey: field.valueKey }
    }
    component Choice: JuxtopposedSettingsChoice { settings: root; arrowAssetName: "controls-settings-764c9.svg" }
    Column {
        id: form; width: root.width; spacing: 20 * root.uiScale
        SettingLabel { objectName: "controlsError"; width: parent.width; text: root.service ? root.service.lastError : ""; visible: text.length > 0; color: "#D98779"; Accessible.name: text }
        SettingLabel { objectName: "controlsStatus"; width: parent.width; text: root.service ? root.service.status : ""; visible: text.length > 0; Accessible.name: text }
        SettingLabel { objectName: "controlsRestart"; width: parent.width; text: qsTr("Restart VLC to apply changed keyboard, mouse-wheel or web-service settings."); visible: root.service && root.service.restartRequired; Accessible.name: text }
        Column {
            width: parent.width
            visible: root.sectionMatches([qsTr("Keyboard Shortcuts"), qsTr("Defaults"), qsTr("Edit Keyboard Shortcuts"), qsTr("Reset all shortcuts")])
            SectionHeading { text: qsTr("Keyboard Shortcuts") }
            SettingRow {
                id: shortcutsRow; objectName: "controlsShortcutsRow"; title: root.values.shortcutsPreset || qsTr("Defaults"); controlHeight: 32 * root.uiScale
                visible: root.matches(qsTr("Keyboard Shortcuts")) || root.matches(qsTr("Defaults")) || root.matches(qsTr("Edit Keyboard Shortcuts"))
                SettingButton { id: editShortcuts; objectName: "controlsEditShortcuts"; anchors.fill: parent; text: qsTr("Edit Keyboard Shortcuts"); enabled: root.editable("editShortcuts"); onClicked: root.action("editShortcuts", "editShortcuts"); Accessible.description: qsTr("Edit local and global shortcuts, then restart VLC to apply them.") }
            }
            SettingRow {
                id: resetRow; objectName: "controlsResetRow"; title: qsTr("Reset all shortcuts"); controlHeight: 32 * root.uiScale
                visible: root.matches(qsTr("Keyboard Shortcuts")) || root.matches(title)
                SettingButton { id: resetShortcuts; objectName: "controlsResetShortcuts"; anchors.fill: parent; text: qsTr("Reset"); enabled: root.editable("resetShortcuts"); onClicked: root.action("resetShortcuts", "resetShortcuts"); Accessible.description: qsTr("Restore default shortcuts while preserving all other settings.") }
            }
        }
        Column {
            width: parent.width; visible: root.sectionMatches([qsTr("Skip Amounts"),qsTr("Very short skip"),qsTr("Short skip"),qsTr("Medium skip"),qsTr("Long skip")])
            SectionHeading { text: qsTr("Skip Amounts") }
            SettingRow { id: veryShortRow; objectName: "controlsVeryShortRow"; title: qsTr("Very short skip"); minimumRowHeight: 38 * root.uiScale; visible: root.matches(title) || root.matches(qsTr("Skip Amounts")); NumberField { id: veryShort; objectName: "controlsVeryShortSkip"; valueKey: "veryShortSkip"; accessibleTitle: veryShortRow.title } }
            SettingRow { id: shortRow; objectName: "controlsShortRow"; title: qsTr("Short skip"); minimumRowHeight: 38 * root.uiScale; visible: root.matches(title) || root.matches(qsTr("Skip Amounts")); NumberField { id: shortSkip; objectName: "controlsShortSkip"; valueKey: "shortSkip"; accessibleTitle: shortRow.title } }
            SettingRow { id: mediumRow; objectName: "controlsMediumRow"; title: qsTr("Medium skip"); minimumRowHeight: 38 * root.uiScale; visible: root.matches(title) || root.matches(qsTr("Skip Amounts")); NumberField { id: medium; objectName: "controlsMediumSkip"; valueKey: "mediumSkip"; accessibleTitle: mediumRow.title } }
            SettingRow { id: longRow; objectName: "controlsLongRow"; title: qsTr("Long skip"); minimumRowHeight: 38 * root.uiScale; visible: root.matches(title) || root.matches(qsTr("Skip Amounts")); NumberField { id: longSkip; objectName: "controlsLongSkip"; valueKey: "longSkip"; accessibleTitle: longRow.title } }
        }
        Column {
            width: parent.width; visible: root.sectionMatches([qsTr("Mouse"),qsTr("Single click on the video"),qsTr("Double click on the video"),qsTr("Scroll wheel"),qsTr("Horizontal scroll"),qsTr("Reverse scroll direction")])
            SectionHeading { text: qsTr("Mouse") }
            SettingRow { id: singleRow; objectName: "controlsSingleRow"; title: qsTr("Single click on the video"); visible: root.matches(title) || root.matches(qsTr("Mouse")); Choice { id: singleClick; objectName: "controlsSingleClick"; valueKey: "singleClick"; accessibleTitle: singleRow.title } }
            SettingRow { id: doubleRow; objectName: "controlsDoubleRow"; title: qsTr("Double click on the video"); visible: root.matches(title) || root.matches(qsTr("Mouse")); Choice { id: doubleClick; objectName: "controlsDoubleClick"; valueKey: "doubleClick"; accessibleTitle: doubleRow.title } }
            SettingRow { id: wheelRow; objectName: "controlsWheelRow"; title: qsTr("Scroll wheel"); visible: root.matches(title) || root.matches(qsTr("Mouse")); Choice { id: scrollWheel; objectName: "controlsScrollWheel"; valueKey: "scrollWheel"; accessibleTitle: wheelRow.title } }
            SettingRow { id: horizontalRow; objectName: "controlsHorizontalRow"; title: qsTr("Horizontal scroll"); visible: root.matches(title) || root.matches(qsTr("Mouse")); Choice { id: horizontalScroll; objectName: "controlsHorizontalScroll"; valueKey: "horizontalScroll"; accessibleTitle: horizontalRow.title } }
            SettingRow { id: reverseRow; objectName: "controlsReverseRow"; title: qsTr("Reverse scroll direction"); controlHeight: 32 * root.uiScale; visible: root.matches(title) || root.matches(qsTr("Mouse")); Toggle { id: reverseScroll; objectName: "controlsReverseScroll"; valueKey: "reverseScroll"; accessibleTitle: reverseRow.title } }
        }
        Column {
            width: parent.width; visible: root.sectionMatches([qsTr("Remotes"),qsTr("Web remote control"),qsTr("Port"),qsTr("Use the same wheel settings"),qsTr("Scroll wheel"),qsTr("Horizontal scroll"),qsTr("Reverse scroll direction")])
            SectionHeading { text: qsTr("Remotes") }
            SettingLabel {
                objectName: "controlsRemoteServiceState"; width: parent.width
                visible: root.values.webRemoteEnabled === true || root.values.remoteState === "active" || root.values.remoteState === "failed"
                text: root.values.remoteState === "active" ? qsTr("Web remote is running.") : root.values.remoteState === "starting" ? qsTr("Web remote is starting…") : root.values.remoteState === "failed" ? qsTr("Web remote could not start. Check its port and HTTPS settings, then restart VLC.") : root.values.remoteState === "password-required" ? qsTr("Web remote is disabled because a password is required.") : qsTr("Web remote is configured. Restart VLC to start it.")
                color: root.values.remoteState === "failed" ? "#D98779" : root.tone("#8F8F8F"); Accessible.name: text
            }
            SettingRow { id: remoteRow; objectName: "controlsRemoteRow"; title: qsTr("Web remote control"); controlHeight: 32 * root.uiScale; minimumRowHeight: 38 * root.uiScale; visible: root.matches(title) || root.matches(qsTr("Remotes")); Toggle { id: webRemote; objectName: "controlsWebRemote"; valueKey: "webRemoteEnabled"; accessibleTitle: remoteRow.title } }
            SettingRow { id: portRow; objectName: "controlsPortRow"; title: qsTr("Port"); minimumRowHeight: 38 * root.uiScale; visible: root.matches(title) || root.matches(qsTr("Remotes")); NumberField { id: remotePort; objectName: "controlsRemotePort"; valueKey: "remotePort"; accessibleTitle: portRow.title; seconds: false } }
            SettingRow { id: linkWheelRow; objectName: "controlsLinkWheelRow"; title: qsTr("Use the same wheel settings"); controlHeight: 32 * root.uiScale; visible: root.matches(title) || root.matches(qsTr("Remotes")); Toggle { id: linkWheelSettings; objectName: "controlsLinkWheelSettings"; valueKey: "linkWheelSettings"; accessibleTitle: linkWheelRow.title } }
            SettingRow { id: remoteWheelRow; objectName: "controlsRemoteWheelRow"; title: qsTr("Scroll wheel"); visible: root.matches(title) || root.matches(qsTr("Remotes")); Choice { id: remoteScrollWheel; objectName: "controlsRemoteScrollWheel"; valueKey: "remoteScrollWheel"; accessibleTitle: qsTr("Remote scroll wheel") } }
            SettingRow { id: remoteHorizontalRow; objectName: "controlsRemoteHorizontalRow"; title: qsTr("Horizontal scroll"); visible: root.matches(title) || root.matches(qsTr("Remotes")); Choice { id: remoteHorizontalScroll; objectName: "controlsRemoteHorizontalScroll"; valueKey: "remoteHorizontalScroll"; accessibleTitle: qsTr("Remote horizontal scroll") } }
            SettingRow { id: remoteReverseRow; objectName: "controlsRemoteReverseRow"; title: qsTr("Reverse scroll direction"); controlHeight: 32 * root.uiScale; visible: root.matches(title) || root.matches(qsTr("Remotes")); Toggle { id: remoteReverseScroll; objectName: "controlsRemoteReverseScroll"; valueKey: "remoteReverseScroll"; accessibleTitle: qsTr("Remote reverse scroll direction") } }
        }
    }
}
