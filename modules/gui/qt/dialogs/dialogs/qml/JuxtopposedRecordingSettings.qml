// SPDX-License-Identifier: GPL-2.0-or-later
// Original design and unchanged SVG artwork: Juxtopposed, CC BY 4.0.
import QtQuick
import QtQuick.Controls.Basic as C
FocusScope {
    id: root
    objectName: "juxtopposedRecordingSettings"
    required property var preferences
    property var recordingPreferences: null
    readonly property var service: recordingPreferences
    property var appearance: null
    property real uiScale: 1
    property url assetBase: "qrc:/juxtopposed/"
    property string searchText: ""
    property var focusNavigator: null
    signal focusRevealed(var item)
    signal advancedRequested()
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
    readonly property var rows: [folderRow,nativeRow,discoveryRow,qualityRow,localRow,audioRow,videoRow,subtitlesRow,presetRow,tuningRow]
    readonly property var choices: [castQuality,encodingPreset,tuning]
    readonly property var toggles: [nativeRecording,discoverDevices,playLocally,includeAudio,includeVideo,includeSubtitles]
    readonly property var focusTargets: [recordingDirectory,browse,folderRow.helpButton,nativeRecording,
        discoverDevices,castQuality,qualityRow.helpButton,playLocally,includeAudio,includeVideo,includeSubtitles,
        encodingPreset,presetRow.helpButton,tuning,tuningRow.helpButton,advanced]
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
        const group = key.startsWith("include") ? "streamTracks" : key
        const override = service.applyModes[key] === "commandLineOverride" ? " " + service.supportNotes.commandLineOverride : ""
        return (service.supportNotes[group] || "") + override
    }
    function timing(mode) {
        switch (mode) {
        case "nextInput": return qsTr("Saved. Applies to the next media opened.")
        case "nextJob": return qsTr("Saved. Applies to the next stream or cast using this default.")
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
        if (service) for (const entry of service.choices[key] || []) if (entry.value === value) return entry.name
        return value === undefined ? qsTr("Unavailable") : qsTr("Custom (%1)").arg(value)
    }
    function closeChoices() {
        for (const choice of choices) choice.popup.close()
        for (const toggle of toggles) toggle.customPopup.close()
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
    component SettingRow: JuxtopposedSettingsFields.SettingRow { settings: root; infoAssetName: "recording-settings-1f9fe.svg"; height: implicitHeight }
    component SectionHeading: JuxtopposedSettingsFields.SectionHeading { settings: root }
    component Choice: JuxtopposedSettingsChoice { settings: root; arrowAssetName: "recording-settings-764c9.svg" }
    component Toggle: JuxtopposedSettingsFields.Toggle {
        settings: root; enabled: root.editable(valueKey)
        C.ToolTip.visible: enabled && !root.failedKey.length && (hovered || visualFocus)
        C.ToolTip.text: root.helpFor(valueKey)
        C.ToolTip.delay: 500
        C.ToolTip.timeout: 12000
    }
    Connections {
        target: root.service
        function onLastErrorChanged() { if (!root.service.lastError.length) root.failedKey = "" }
        function onAdvancedOptionsRequested() { root.closeChoices(); root.advancedRequested() }
    }
    Column {
        id: form; width: root.width; spacing: 20 * root.uiScale
        SettingLabel { objectName: "recordingError"; width: parent.width; text: root.service ? root.service.lastError : ""; visible: text.length > 0; color: "#D98779"; Accessible.name: text }
        SettingLabel { objectName: "recordingStatus"; width: parent.width; text: root.appliedTiming; visible: text.length > 0; Accessible.name: text }
        Column {
            width: parent.width
            visible: root.sectionMatches([qsTr("Recording"),folderRow.title,nativeRow.title])
            SectionHeading { text: qsTr("Recording") }
            SettingRow {
                id: folderRow; objectName: "recordingFolderRow"; title: qsTr("Save recordings to")
                help: root.helpFor("recordingDirectory"); infoName: "recordingFolderInfo"
                visible: root.matches(title) || root.matches(qsTr("Recording"))
                C.TextField {
                    id: recordingDirectory; objectName: "recordingDirectory"
                    property bool dirty: false
                    anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom
                    width: Math.max(0, parent.width - browse.width - 10 * root.uiScale)
                    enabled: root.editable("recordingDirectory"); activeFocusOnTab: true; selectByMouse: true
                    font.family: inter.name; font.pixelSize: 14 * root.uiScale
                    color: root.tone("#8F8F8F"); placeholderTextColor: root.tone("#8F8F8F")
                    selectionColor: root.accentDim; selectedTextColor: root.tone("#C6C6C6")
                    leftPadding: 12 * root.uiScale; rightPadding: 12 * root.uiScale
                    placeholderText: root.values.defaultRecordingDirectory || ""
                    Accessible.name: folderRow.title; Accessible.description: root.helpFor("recordingDirectory")
                    function sync() { text = root.values.recordingDirectory || ""; dirty = false }
                    Component.onCompleted: sync()
                    onTextEdited: dirty = true
                    Connections { target: root.service; function onValuesChanged() { if (!recordingDirectory.activeFocus) recordingDirectory.sync() } }
                    onEditingFinished: if (dirty && enabled && root.write("recordingDirectory", text)) sync()
                    onActiveFocusChanged: root.reveal(recordingDirectory)
                    Keys.onEscapePressed: event => { sync(); root.failedKey = ""; event.accepted = true }
                    Keys.onTabPressed: event => { if (root.focusNavigator) { root.navigate(recordingDirectory,false); event.accepted = true } }
                    Keys.onBacktabPressed: event => { if (root.focusNavigator) { root.navigate(recordingDirectory,true); event.accepted = true } }
                    background: Rectangle { color: root.tone("#090909"); radius: 6 * root.uiScale; border.width: 2 * root.uiScale; border.color: recordingDirectory.visualFocus ? root.accent : root.tone("#191919") }
                    JuxtopposedSettingsFields.FeedbackTip { settings: root; control: recordingDirectory; valueKey: "recordingDirectory" }
                }
                SettingButton {
                    id: browse; objectName: "recordingBrowse"; anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
                    width: 100 * root.uiScale; height: 32 * root.uiScale; text: qsTr("Browse"); font.weight: Font.DemiBold; enabled: root.editable("browse")
                    Accessible.name: qsTr("Browse for recording folder")
                    onClicked: { root.appliedTiming = ""; root.failedKey = root.service.chooseRecordingDirectory() ? "" : "recordingDirectory" }
                }
            }
            SettingRow { id: nativeRow; objectName: "recordingNativeRow"; title: qsTr("Record without re-encoding when possible"); controlHeight: 32 * root.uiScale; visible: root.matches(title) || root.matches(qsTr("Recording")); Toggle { id: nativeRecording; objectName: "recordingNative"; valueKey: "nativeRecording"; accessibleTitle: nativeRow.title } }
        }
        Column {
            width: parent.width
            visible: root.sectionMatches([qsTr("Casting"),discoveryRow.title,qualityRow.title])
            SectionHeading { text: qsTr("Casting") }
            SettingRow { id: discoveryRow; objectName: "recordingDiscoveryRow"; title: qsTr("Discover cast devices"); controlHeight: 32 * root.uiScale; visible: root.matches(title) || root.matches(qsTr("Casting")); Toggle { id: discoverDevices; objectName: "recordingDiscoverDevices"; valueKey: "discoverDevices"; accessibleTitle: discoveryRow.title } }
            SettingRow { id: qualityRow; objectName: "recordingQualityRow"; title: qsTr("Cast quality"); help: root.helpFor("castQuality"); infoName: "recordingQualityInfo"; visible: root.matches(title) || root.matches(qsTr("Casting")); Choice { id: castQuality; objectName: "recordingCastQuality"; valueKey: "castQuality"; accessibleTitle: qualityRow.title } }
        }
        Column {
            width: parent.width
            visible: root.sectionMatches([qsTr("Streaming"),localRow.title,audioRow.title,videoRow.title,subtitlesRow.title,presetRow.title,tuningRow.title])
            SectionHeading { text: qsTr("Streaming") }
            SettingRow { id: localRow; objectName: "recordingLocalRow"; title: qsTr("Play locally while streaming"); controlHeight: 32 * root.uiScale; visible: root.matches(title) || root.matches(qsTr("Streaming")); Toggle { id: playLocally; objectName: "recordingPlayLocally"; valueKey: "playLocally"; accessibleTitle: localRow.title } }
            SettingRow { id: audioRow; objectName: "recordingAudioRow"; title: qsTr("Include audio"); controlHeight: 32 * root.uiScale; visible: root.matches(title) || root.matches(qsTr("Streaming")); Toggle { id: includeAudio; objectName: "recordingIncludeAudio"; valueKey: "includeAudio"; accessibleTitle: audioRow.title } }
            SettingRow { id: videoRow; objectName: "recordingVideoRow"; title: qsTr("Include video"); controlHeight: 32 * root.uiScale; visible: root.matches(title) || root.matches(qsTr("Streaming")); Toggle { id: includeVideo; objectName: "recordingIncludeVideo"; valueKey: "includeVideo"; accessibleTitle: videoRow.title } }
            SettingRow { id: subtitlesRow; objectName: "recordingSubtitlesRow"; title: qsTr("include subtitles"); controlHeight: 32 * root.uiScale; visible: root.matches(title) || root.matches(qsTr("Streaming")); Toggle { id: includeSubtitles; objectName: "recordingIncludeSubtitles"; valueKey: "includeSubtitles"; accessibleTitle: subtitlesRow.title } }
            SettingRow { id: presetRow; objectName: "recordingPresetRow"; title: qsTr("Encoding preset"); help: root.helpFor("encodingPreset"); infoName: "recordingPresetInfo"; visible: root.matches(title) || root.matches(qsTr("Streaming")); Choice { id: encodingPreset; objectName: "recordingEncodingPreset"; valueKey: "encodingPreset"; accessibleTitle: presetRow.title } }
            SettingRow { id: tuningRow; objectName: "recordingTuningRow"; title: qsTr("Tuning"); help: root.helpFor("tuning"); infoName: "recordingTuningInfo"; visible: root.matches(title) || root.matches(qsTr("Streaming")); Choice { id: tuning; objectName: "recordingTuning"; valueKey: "tuning"; accessibleTitle: tuningRow.title } }
        }
        SettingButton {
            id: advanced; objectName: "recordingAdvanced"; width: parent.width; height: 38 * root.uiScale
            text: qsTr("Advanced Options"); enabled: root.editable("advanced"); visible: root.matches(text)
            Accessible.name: text
            onClicked: root.service.openAdvanced()
            background: Rectangle { color: root.tone(advanced.down ? "#222222" : "#191919"); opacity: .95; radius: 2 * root.uiScale; border.width: advanced.visualFocus ? root.uiScale : 0; border.color: root.accent }
            contentItem: Item {
                Row {
                    anchors.centerIn: parent; spacing: 10 * root.uiScale
                    SettingLabel { text: advanced.text; font: advanced.font; height: 24 * root.uiScale; verticalAlignment: Text.AlignVCenter }
                    Item {
                        width: 24 * root.uiScale; height: width
                        Image { objectName: "recordingAdvancedOriginalChevron"; anchors.centerIn: parent; width: 24; height: 24; scale: root.uiScale; source: root.assetBase + "recording-settings-97863.svg"; mirror: true; Accessible.ignored: true }
                    }
                }
            }
        }
    }
}
