// SPDX-License-Identifier: GPL-2.0-or-later
// Original design and unchanged SVG artwork: Juxtopposed, CC BY 4.0.
import QtQuick
import QtQuick.Controls.Basic as C
FocusScope {
    id: root
    objectName: "juxtopposedPerformanceSettings"
    required property var preferences
    property var performancePreferences: null
    readonly property var service: performancePreferences
    property var appearance: null
    property real uiScale: 1
    property url assetBase: "qrc:/juxtopposed/"
    property string searchText: ""
    property var focusNavigator: null
    signal focusRevealed(var item)
    readonly property var values: service ? service.values : ({})
    readonly property var capabilities: service ? service.capabilities : ({})
    readonly property bool available: !!service
    readonly property bool busy: !!service && service.busy
    readonly property bool stacked: width < 650 * uiScale
    readonly property color accent: appearance ? appearance.accent : "#D37918"
    readonly property color accentSelected: appearance ? appearance.accentSelected : "#371F06"
    readonly property color accentDim: appearance ? appearance.accentDim : "#AB6314"
    readonly property string settingsFontFamily: inter.name
    readonly property alias formItem: form
    property string failedKey: ""
    property string appliedTiming: ""
    property bool advancedExpanded: true
    readonly property bool developerVisible: advancedExpanded || searchText.trim().length > 0
    readonly property var rows: [hardwareRow,postprocessingRow,loopRow,threadsRow,dropRow,realtimeRow,priorityRow,rescanRow,encoderRow,decoderRow,syncRow,jitterRow,averageRow]
    readonly property var choices: [hardwareDecoder,postprocessing,loopFilter,encoder,decoder,clockSync]
    readonly property var focusTargets: [hardwareDecoder,hardwareRow.helpButton,postprocessing,postprocessingRow.helpButton,
        loopFilter,loopRow.helpButton,decodingThreads,threadsRow.helpButton,dropLateFrames,realTimePriority,
        processPriority,priorityRow.helpButton,rescan,advanced,encoder,encoderRow.helpButton,
        decoder,decoderRow.helpButton,clockSync,syncRow.helpButton,clockJitter,jitterRow.helpButton,
        clockAverage,averageRow.helpButton]
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
        const group = key === "processPriority" || key === "realTimePriority" ? "priority"
            : key.startsWith("clock") ? "clock" : key
        const override = service.applyModes[key] === "commandLineOverride"
            ? " " + service.supportNotes.commandLineOverride : ""
        return (service.supportNotes[group] || "") + override
    }
    function timing(mode) {
        switch (mode) {
        case "restart": return qsTr("Saved. Restart VLC to apply this change.")
        case "nextInput": return qsTr("Saved. Applies to the next media opened.")
        case "nextJob": return qsTr("Saved. Applies to the next automatic conversion.")
        case "commandLineOverride": return qsTr("Saved. Restart without the command-line override to apply this default.")
        case "live": return qsTr("Applied and saved.")
        default: return ""
        }
    }
    function write(key, value) {
        if (!editable(key)) return false
        const accepted = service.setValue(key, value)
        failedKey = accepted ? "" : key
        appliedTiming = accepted ? timing(service.applyModes[key]) : ""
        return accepted
    }
    function choiceName(key, value) {
        if (key === "loopFilter" && value === 0) return qsTr("Never")
        if (service) for (const entry of service.choices[key] || []) if (entry.value === value) return entry.name
        return value === undefined ? qsTr("Unavailable") : value === "custom" ? qsTr("Custom") : qsTr("Custom (%1)").arg(value)
    }
    function closeChoices() {
        for (const choice of choices) choice.popup.close()
        dropLateFrames.customPopup.close(); realTimePriority.customPopup.close()
        for (const row of rows) row.helpButton.tipOpen = false
    }
    function focusActivePopup() {
        for (const choice of choices) if (choice.popup.visible) { choice.popup.forceActiveFocus(); return true }
        return false
    }
    onVisibleChanged: {
        if (!visible) closeChoices()
        else if (service) { service.reload(); appliedTiming = "" }
    }
    onBusyChanged: if (busy) closeChoices()
    FontLoader { id: inter; source: root.assetBase + "InterVariable.ttf" }
    component SettingLabel: JuxtopposedSettingsFields.SettingLabel { settings: root }
    component SettingButton: JuxtopposedSettingsFields.SettingButton { settings: root }
    component SettingRow: JuxtopposedSettingsFields.SettingRow { settings: root; infoAssetName: "performance-settings-1f9fe.svg"; height: implicitHeight }
    component SectionHeading: JuxtopposedSettingsFields.SectionHeading { settings: root }
    component Choice: JuxtopposedSettingsChoice { settings: root; arrowAssetName: "performance-settings-764c9.svg" }
    component FeedbackTip: JuxtopposedSettingsFields.FeedbackTip { settings: root }
    Connections {
        target: root.service
        function onLastErrorChanged() { if (!root.service.lastError.length) root.failedKey = "" }
    }
    component Field: C.TextField {
        id: field
        required property string valueKey
        required property string accessibleTitle
        property bool milliseconds: false
        property bool dirty: false
        anchors.fill: parent
        enabled: root.editable(valueKey)
        activeFocusOnTab: true; selectByMouse: true
        font.family: inter.name; font.pixelSize: 14 * root.uiScale
        color: root.tone("#8F8F8F"); selectionColor: root.accentDim; selectedTextColor: root.tone("#C6C6C6")
        leftPadding: 12 * root.uiScale; rightPadding: 18 * root.uiScale
        topPadding: 4 * root.uiScale; bottomPadding: 4 * root.uiScale
        Accessible.name: accessibleTitle; Accessible.description: root.helpFor(valueKey)
        function sync() {
            text = typeof root.values[valueKey] === "number" ? String(root.values[valueKey]) + (milliseconds ? " ms" : "") : ""
            dirty = false
        }
        Component.onCompleted: sync()
        onTextEdited: dirty = true
        Connections { target: root.service; function onValuesChanged() { if (!field.activeFocus) field.sync() } }
        onEditingFinished: {
            if (!dirty || !enabled) return
            const raw = milliseconds ? text.trim().replace(/\s*ms$/, "") : text.trim()
            if (root.write(valueKey, /^-?\d+$/.test(raw) ? Number(raw) : text)) sync()
        }
        onActiveFocusChanged: root.reveal(field)
        Keys.onEscapePressed: event => { sync(); root.failedKey = ""; event.accepted = true }
        Keys.onTabPressed: event => { if (root.focusNavigator) { root.navigate(field,false); event.accepted = true } }
        Keys.onBacktabPressed: event => { if (root.focusNavigator) { root.navigate(field,true); event.accepted = true } }
        background: Rectangle { color: root.tone("#090909"); radius: 6 * root.uiScale; border.width: 2 * root.uiScale; border.color: field.visualFocus ? root.accent : root.tone("#191919") }
        FeedbackTip { control: field; valueKey: field.valueKey }
    }
    Column {
        id: form; width: root.width; spacing: 20 * root.uiScale
        SettingLabel { objectName: "performanceError"; width: parent.width; text: root.service ? root.service.lastError : ""; visible: text.length > 0; color: "#D98779"; Accessible.name: text }
        SettingLabel { objectName: "performanceStatus"; width: parent.width; text: root.service && root.service.status.length ? root.service.status : root.appliedTiming; visible: text.length > 0; Accessible.name: text }
        Column {
            width: parent.width
            visible: root.sectionMatches([qsTr("Decoding"),hardwareRow.title,postprocessingRow.title,loopRow.title,threadsRow.title])
            SectionHeading { text: qsTr("Decoding") }
            SettingRow { id: hardwareRow; objectName: "performanceHardwareRow"; title: qsTr("Hardware-accelerated decoding"); help: root.helpFor("hardwareDecoder"); infoName: "performanceHardwareInfo"; visible: root.matches(title) || root.matches(qsTr("Decoding")); Choice { id: hardwareDecoder; objectName: "performanceHardwareDecoder"; valueKey: "hardwareDecoder"; accessibleTitle: hardwareRow.title } }
            SettingRow { id: postprocessingRow; objectName: "performancePostprocessingRow"; title: qsTr("Video post-processing"); help: root.helpFor("postprocessing"); infoName: "performancePostprocessingInfo"; visible: root.matches(title) || root.matches(qsTr("Decoding")); Choice { id: postprocessing; objectName: "performancePostprocessing"; valueKey: "postprocessing"; accessibleTitle: postprocessingRow.title } }
            SettingRow { id: loopRow; objectName: "performanceLoopRow"; title: qsTr("Skip the H.264 loop filter"); help: root.helpFor("loopFilter"); infoName: "performanceLoopInfo"; visible: root.matches(title) || root.matches(qsTr("Decoding")); Choice { id: loopFilter; objectName: "performanceLoopFilter"; valueKey: "loopFilter"; accessibleTitle: loopRow.title } }
            SettingRow { id: threadsRow; objectName: "performanceThreadsRow"; title: qsTr("Decoding threads"); help: root.helpFor("decodingThreads"); infoName: "performanceThreadsInfo"; visible: root.matches(title) || root.matches(qsTr("Decoding")); Field { id: decodingThreads; objectName: "performanceDecodingThreads"; valueKey: "decodingThreads"; accessibleTitle: threadsRow.title } }
        }
        Column {
            width: parent.width
            visible: root.sectionMatches([qsTr("Smooth Playback"),dropRow.title,realtimeRow.title,priorityRow.title,rescanRow.title])
            SectionHeading { text: qsTr("Smooth Playback") }
            SettingRow { id: dropRow; objectName: "performanceDropRow"; title: qsTr("Drop frames when the system can’t keep up"); controlHeight: 32 * root.uiScale; visible: root.matches(title) || root.matches(qsTr("Smooth Playback")); JuxtopposedSettingsFields.Toggle { id: dropLateFrames; objectName: "performanceDropLateFrames"; settings: root; valueKey: "dropLateFrames"; accessibleTitle: dropRow.title; enabled: root.editable(valueKey) } }
            SettingRow { id: realtimeRow; objectName: "performanceRealtimeRow"; title: qsTr("Give VLC real-time priority"); controlHeight: 32 * root.uiScale; visible: root.matches(title) || root.matches(qsTr("Smooth Playback")); JuxtopposedSettingsFields.Toggle { id: realTimePriority; objectName: "performanceRealTimePriority"; settings: root; valueKey: "realTimePriority"; accessibleTitle: realtimeRow.title; enabled: root.editable(valueKey) } }
            SettingRow { id: priorityRow; objectName: "performancePriorityRow"; title: qsTr("Process priority"); help: root.helpFor("processPriority"); infoName: "performancePriorityInfo"; visible: root.matches(title) || root.matches(qsTr("Smooth Playback")); Field { id: processPriority; objectName: "performanceProcessPriority"; valueKey: "processPriority"; accessibleTitle: priorityRow.title } }
            SettingRow {
                id: rescanRow; objectName: "performanceRescanRow"; title: qsTr("Rescan plugins")
                visible: root.matches(title) || root.matches(qsTr("Smooth Playback"))
                SettingButton { id: rescan; objectName: "performanceRescan"; anchors.fill: parent; text: root.busy ? qsTr("Rescanning…") : qsTr("Rescan"); enabled: root.editable("rescan"); Accessible.name: rescanRow.title; Accessible.description: root.helpFor("rescan"); onClicked: { root.appliedTiming = ""; root.failedKey = root.service.rescanPlugins() ? "" : "rescan" } }
            }
        }
        SettingButton {
            id: advanced; objectName: "performanceAdvanced"; width: parent.width; height: 38 * root.uiScale
            text: qsTr("Advanced Options"); enabled: root.editable("advanced")
            visible: root.matches(text) || developerSection.visible
            Accessible.name: text; Accessible.description: root.developerVisible ? qsTr("Developer settings expanded") : qsTr("Developer settings collapsed")
            Accessible.role: Accessible.CheckBox; Accessible.checkable: true; Accessible.checked: root.developerVisible
            onClicked: { root.closeChoices(); root.advancedExpanded = !root.advancedExpanded }
            Image { objectName: "performanceAdvancedOriginalChevron"; width: 24; height: 24; scale: root.uiScale; x: (parent.width + advanced.contentItem.implicitWidth) / 2 + 8 * root.uiScale; anchors.verticalCenter: parent.verticalCenter; source: root.assetBase + "performance-settings-764c9.svg"; rotation: root.developerVisible ? 0 : -90; Accessible.ignored: true }
        }
        Column {
            id: developerSection; objectName: "performanceDeveloperSection"; width: parent.width
            visible: root.developerVisible && root.sectionMatches([qsTr("Developer"),encoderRow.title,decoderRow.title,syncRow.title,jitterRow.title,averageRow.title])
            SectionHeading { text: qsTr("Developer") }
            SettingRow { id: encoderRow; objectName: "performanceEncoderRow"; title: qsTr("Encoder"); help: root.helpFor("encoder"); infoName: "performanceEncoderInfo"; visible: root.matches(title) || root.matches(qsTr("Developer")); Choice { id: encoder; objectName: "performanceEncoder"; valueKey: "encoder"; accessibleTitle: encoderRow.title } }
            SettingRow { id: decoderRow; objectName: "performanceDecoderRow"; title: qsTr("Decoder"); help: root.helpFor("decoder"); infoName: "performanceDecoderInfo"; visible: root.matches(title) || root.matches(qsTr("Developer")); Choice { id: decoder; objectName: "performanceDecoder"; valueKey: "decoder"; accessibleTitle: decoderRow.title } }
            SettingRow { id: syncRow; objectName: "performanceSyncRow"; title: qsTr("Clock synchronisation"); help: root.helpFor("clockSync"); infoName: "performanceSyncInfo"; visible: root.matches(title) || root.matches(qsTr("Developer")); Choice { id: clockSync; objectName: "performanceClockSync"; valueKey: "clockSync"; accessibleTitle: syncRow.title } }
            SettingRow { id: jitterRow; objectName: "performanceJitterRow"; title: qsTr("Clock jitter"); help: root.helpFor("clockJitter"); infoName: "performanceJitterInfo"; visible: root.matches(title) || root.matches(qsTr("Developer")); Field { id: clockJitter; objectName: "performanceClockJitter"; valueKey: "clockJitter"; accessibleTitle: jitterRow.title; milliseconds: true } }
            SettingRow { id: averageRow; objectName: "performanceAverageRow"; title: qsTr("Clock reference averaging"); help: root.helpFor("clockAverage"); infoName: "performanceAverageInfo"; visible: root.matches(title) || root.matches(qsTr("Developer")); Field { id: clockAverage; objectName: "performanceClockAverage"; valueKey: "clockAverage"; accessibleTitle: averageRow.title } }
        }
    }
}
