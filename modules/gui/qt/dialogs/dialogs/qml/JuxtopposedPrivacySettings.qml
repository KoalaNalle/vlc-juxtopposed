// SPDX-License-Identifier: GPL-2.0-or-later
// Original visual design and unchanged SVG artwork: Juxtopposed, CC BY 4.0.
// Password label, state/error feedback and narrow layout are native adaptations.
import QtQuick
import QtQuick.Controls.Basic as C
FocusScope {
    id: root
    objectName: "juxtopposedPrivacySettings"
    required property var preferences
    property var privacyPreferences: null
    readonly property var service: privacyPreferences
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
    property string failedKey: ""
    readonly property var rows: [saveRecentlyPlayedRow,rememberPlaybackPositionRow,historyExclusionRow,clearHistoryRow,internetMetadataRow,submitPlayedTracksRow,lastfmUsernameRow,lastfmPasswordRow,lastfmSubmissionUrlRow]
    readonly property var controls: [saveRecentlyPlayed,rememberPlaybackPosition,historyExclusion,clearHistory,internetMetadata,submitPlayedTracks,lastfmUsername,lastfmPassword,lastfmSubmissionUrl]
    readonly property var focusTargets: rows.reduce((all,row,index)=>all.concat([row.helpButton,controls[index]]),[])
    implicitHeight: form.implicitHeight
    function tone(value) { return appearance ? appearance.color(value) : value }
    function matches(text) { return !searchText.trim().length || text.toLocaleLowerCase().includes(searchText.trim().toLocaleLowerCase()) }
    function sectionMatches(names) { return names.some(name=>matches(name)) }
    function supported(key) { return available && capabilities[key] === true }
    function editable(key) { return supported(key) && !busy }
    function navigate(item,backwards) { if (focusNavigator) focusNavigator(item,backwards) }
    function reveal(item) { if (item.activeFocus) focusRevealed(item) }
    function write(key,value) {
        if (!editable(key)) return false
        const accepted=service.setValue(key,value)
        failedKey=accepted ? "" : key
        return accepted
    }
    function helpFor(key) {
        if (!service) return qsTr("This setting is unavailable in this build.")
        let result=service.supportNotes[key] || ""
        if (service.applyModes[key] === "restart") result += " " + qsTr("Restart VLC to apply this change.")
        return result
    }
    function closeChoices() {
        for (const toggle of [saveRecentlyPlayed,rememberPlaybackPosition,internetMetadata,submitPlayedTracks]) toggle.customPopup.close()
        for (const row of rows) row.helpButton.tipOpen=false
    }
    function focusActivePopup() { closeChoices(); return false }
    onVisibleChanged: if (!visible) closeChoices()
    readonly property alias formItem: form
    readonly property string settingsFontFamily: inter.name
    FontLoader { id: inter; source: root.assetBase + "InterVariable.ttf" }
    component SettingLabel: JuxtopposedSettingsFields.SettingLabel { settings: root }
    component SettingButton: JuxtopposedSettingsFields.SettingButton { settings: root }
    component SettingRow: JuxtopposedSettingsFields.SettingRow { settings: root }
    component Toggle: JuxtopposedSettingsFields.Toggle { settings: root }
    component SectionHeading: JuxtopposedSettingsFields.SectionHeading { settings: root }


    Connections{target:root.service;function onLastErrorChanged(){if(!root.service.lastError.length)root.failedKey=""}}
    component FeedbackTip: JuxtopposedSettingsFields.FeedbackTip { settings: root }
    component PreferenceText: C.TextField {
        id: field
        required property string valueKey
        required property string accessibleTitle
        property bool password: false
        anchors.fill: parent
        enabled: root.editable(valueKey)
        activeFocusOnTab: true
        selectByMouse: true
        echoMode: password ? TextInput.Password : TextInput.Normal
        font.family: inter.name
        font.pixelSize: 14 * root.uiScale
        color: root.tone("#8F8F8F")
        selectionColor: root.accentDim
        selectedTextColor: root.tone("#C6C6C6")
        leftPadding: 12 * root.uiScale; rightPadding: 18 * root.uiScale
        topPadding: 4 * root.uiScale; bottomPadding: 4 * root.uiScale
        Accessible.name: accessibleTitle
        Accessible.description: root.helpFor(valueKey)
        function sync() { text = typeof root.values[valueKey] === "string" ? root.values[valueKey] : "" }
        Component.onCompleted: sync()
        Connections { target: root.service; function onValuesChanged() { if (!field.activeFocus) field.sync() } }
        onEditingFinished: {
            if (text !== root.values[valueKey] && root.editable(valueKey)) {
                if (root.write(valueKey,text)) sync()
            }
        }
        onActiveFocusChanged: root.reveal(field)
        Keys.onEscapePressed: event => { sync(); root.failedKey=""; event.accepted=true }
        Keys.onTabPressed: event => { if (root.focusNavigator) { root.navigate(field,false); event.accepted=true } }
        Keys.onBacktabPressed: event => { if (root.focusNavigator) { root.navigate(field,true); event.accepted=true } }
        background: Rectangle {
            color: root.tone("#090909"); radius: 6 * root.uiScale
            border.width: 2 * root.uiScale
            border.color: field.visualFocus ? root.accent : root.tone("#191919")
        }
        FeedbackTip { control: field; valueKey: field.valueKey }
    }
    Column {
        id: form; width: root.width; spacing: 20 * root.uiScale
        SettingLabel { objectName: "privacyError"; width: parent.width; text: root.service ? root.service.lastError : ""; visible: text.length > 0; color: "#D98779"; Accessible.name: text }
        SettingLabel { objectName: "privacyStatus"; width: parent.width; text: root.service ? root.service.status : ""; visible: text.length > 0; Accessible.name: text }
        SettingLabel { objectName: "privacyRestart"; width: parent.width; text: qsTr("Restart VLC to apply the changed network defaults."); visible: root.service && root.service.restartRequired; Accessible.name: text }
        Column {
            width: parent.width
            visible: root.sectionMatches([qsTr("History"), qsTr("Save recently played items"), qsTr("Remember the playback position"), qsTr("Never remember items matching"), qsTr("Clear history now")])
            SectionHeading { text: qsTr("History") }
            SettingRow {
                id: saveRecentlyPlayedRow; objectName: "privacysaveRecentlyPlayedRow"
                title: qsTr("Save recently played items"); help: root.helpFor("saveRecentlyPlayed"); infoName: "privacysaveRecentlyPlayedInfo"
                visible: root.matches(title) || root.matches(qsTr("History"))
                opacity: root.supported("saveRecentlyPlayed") ? 1 : .5
                controlHeight: 32 * root.uiScale
                Toggle { id: saveRecentlyPlayed; objectName: "privacysaveRecentlyPlayed"; valueKey: "saveRecentlyPlayed"; accessibleTitle: saveRecentlyPlayedRow.title; enabled: root.editable(valueKey) }
            }
            SettingRow {
                id: rememberPlaybackPositionRow; objectName: "privacyrememberPlaybackPositionRow"
                title: qsTr("Remember the playback position"); help: root.helpFor("rememberPlaybackPosition"); infoName: "privacyrememberPlaybackPositionInfo"
                visible: root.matches(title) || root.matches(qsTr("History"))
                opacity: root.supported("rememberPlaybackPosition") ? 1 : .5
                controlHeight: 32 * root.uiScale
                Toggle { id: rememberPlaybackPosition; objectName: "privacyrememberPlaybackPosition"; valueKey: "rememberPlaybackPosition"; accessibleTitle: rememberPlaybackPositionRow.title; enabled: root.editable(valueKey) }
            }
            SettingRow {
                id: historyExclusionRow; objectName: "privacyhistoryExclusionRow"
                title: qsTr("Never remember items matching"); help: root.helpFor("historyExclusion"); infoName: "privacyhistoryExclusionInfo"
                visible: root.matches(title) || root.matches(qsTr("History"))
                opacity: root.supported("historyExclusion") ? 1 : .5
                controlHeight: 36 * root.uiScale
                PreferenceText { id: historyExclusion; objectName: "privacyhistoryExclusion"; valueKey: "historyExclusion"; accessibleTitle: historyExclusionRow.title; password: false }
            }
            SettingRow {
                id: clearHistoryRow; objectName: "privacyclearHistoryRow"
                title: qsTr("Clear history now"); help: root.helpFor("clearHistory"); infoName: "privacyclearHistoryInfo"
                visible: root.matches(title) || root.matches(qsTr("History"))
                opacity: root.supported("clearHistory") ? 1 : .5
                controlHeight: 32 * root.uiScale
                controlWidth: 100 * root.uiScale
                SettingButton {
                    id: clearHistory; objectName: "privacyClearHistory"; anchors.fill: parent
                    text: root.busy ? qsTr("Clearing…") : qsTr("Clear")
                    enabled: root.editable("clearHistory")
                    Accessible.name: clearHistoryRow.title
                    Accessible.description: root.helpFor("clearHistory")
                    onClicked: { root.failedKey=root.service.clearHistory() ? "" : "clearHistory" }
                }
            }
        }
        Column {
            width: parent.width
            visible: root.sectionMatches([qsTr("Network"), qsTr("Fetch titles and cover art from the internet"), qsTr("Submit played tracks to last.fm"), qsTr("Last.fm username"), qsTr("Last.fm password"), qsTr("Last.fm submission URL")])
            SectionHeading { text: qsTr("Network") }
            SettingRow {
                id: internetMetadataRow; objectName: "privacyinternetMetadataRow"
                title: qsTr("Fetch titles and cover art from the internet"); help: root.helpFor("internetMetadata"); infoName: "privacyinternetMetadataInfo"
                visible: root.matches(title) || root.matches(qsTr("Network"))
                opacity: root.supported("internetMetadata") ? 1 : .5
                controlHeight: 32 * root.uiScale
                Toggle { id: internetMetadata; objectName: "privacyinternetMetadata"; valueKey: "internetMetadata"; accessibleTitle: internetMetadataRow.title; enabled: root.editable(valueKey) }
            }
            SettingRow {
                id: submitPlayedTracksRow; objectName: "privacysubmitPlayedTracksRow"
                title: qsTr("Submit played tracks to last.fm"); help: root.helpFor("submitPlayedTracks"); infoName: "privacysubmitPlayedTracksInfo"
                visible: root.matches(title) || root.matches(qsTr("Network"))
                opacity: root.supported("submitPlayedTracks") ? 1 : .5
                controlHeight: 32 * root.uiScale
                Toggle { id: submitPlayedTracks; objectName: "privacysubmitPlayedTracks"; valueKey: "submitPlayedTracks"; accessibleTitle: submitPlayedTracksRow.title; enabled: root.editable(valueKey) }
            }
            SettingRow {
                id: lastfmUsernameRow; objectName: "privacylastfmUsernameRow"
                title: qsTr("Last.fm username"); help: root.helpFor("lastfmUsername"); infoName: "privacylastfmUsernameInfo"
                visible: root.matches(title) || root.matches(qsTr("Network"))
                opacity: root.supported("lastfmUsername") ? 1 : .5
                controlHeight: 36 * root.uiScale
                PreferenceText { id: lastfmUsername; objectName: "privacylastfmUsername"; valueKey: "lastfmUsername"; accessibleTitle: lastfmUsernameRow.title; password: false }
            }
            SettingRow {
                id: lastfmPasswordRow; objectName: "privacylastfmPasswordRow"
                title: qsTr("Last.fm password"); help: root.helpFor("lastfmPassword"); infoName: "privacylastfmPasswordInfo"
                visible: root.matches(title) || root.matches(qsTr("Network"))
                opacity: root.supported("lastfmPassword") ? 1 : .5
                controlHeight: 36 * root.uiScale
                PreferenceText { id: lastfmPassword; objectName: "privacylastfmPassword"; valueKey: "lastfmPassword"; accessibleTitle: lastfmPasswordRow.title; password: true }
            }
            SettingRow {
                id: lastfmSubmissionUrlRow; objectName: "privacylastfmSubmissionUrlRow"
                title: qsTr("Last.fm submission URL"); help: root.helpFor("lastfmSubmissionUrl"); infoName: "privacylastfmSubmissionUrlInfo"
                visible: root.matches(title) || root.matches(qsTr("Network"))
                opacity: root.supported("lastfmSubmissionUrl") ? 1 : .5
                controlHeight: 36 * root.uiScale
                PreferenceText { id: lastfmSubmissionUrl; objectName: "privacylastfmSubmissionUrl"; valueKey: "lastfmSubmissionUrl"; accessibleTitle: lastfmSubmissionUrlRow.title; password: false }
            }
        }
    }
    Connections { target: form; function onImplicitHeightChanged() { Qt.callLater(()=>{ const item=root.focusTargets.find(item=>item && item.activeFocus); if (item) root.reveal(item) }) } }
}
