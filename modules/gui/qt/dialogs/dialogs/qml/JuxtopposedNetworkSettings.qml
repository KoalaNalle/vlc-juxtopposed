// SPDX-License-Identifier: GPL-2.0-or-later
// Original design and unchanged SVG artwork: Juxtopposed, CC BY 4.0.
import QtQuick
import QtQuick.Controls.Basic as C
FocusScope {
    id: root
    objectName: "juxtopposedNetworkSettings"
    required property var preferences
    property var networkPreferences: null
    readonly property var service: networkPreferences
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
    readonly property bool busy: false
    readonly property bool stacked: width < 650 * uiScale
    readonly property color accent: appearance ? appearance.accent : "#D37918"
    readonly property color accentSelected: appearance ? appearance.accentSelected : "#371F06"
    readonly property color accentDim: appearance ? appearance.accentDim : "#AB6314"
    readonly property string settingsFontFamily: inter.name
    readonly property alias formItem: form
    property string failedKey: ""
    readonly property var rows: [bufferingRow,localRow,discRow,streamsRow,captureRow,proxyRow,passwordRow,trustRow,cipherRow]
    readonly property var focusTargets: [buffering,bufferingRow.helpButton,localFiles,discs,networkStreams,liveCapture,httpProxy,proxyRow.helpButton,proxyPassword,passwordClear,passwordRow.helpButton,systemTrust,trustRow.helpButton,cipherPreference,cipherRow.helpButton,advanced]
    implicitHeight: form.implicitHeight
    function tone(value) { return appearance ? appearance.color(value) : value }
    function matches(text) { return !searchText.trim().length || text.toLocaleLowerCase().includes(searchText.trim().toLocaleLowerCase()) }
    function sectionMatches(texts) { return texts.some(text => matches(text)) }
    function supported(key) { return available && capabilities[key] === true }
    function editable(key) { return supported(key) }
    function navigate(item, backwards) { if (focusNavigator) focusNavigator(item, backwards) }
    function reveal(item) { if (item.activeFocus) focusRevealed(item) }
    function helpFor(key) {
        if (!service) return qsTr("This setting is unavailable in this build.")
        const group = key.startsWith("httpProxy") ? "proxy" : key === "systemTrust" || key === "cipherPreference" ? "tls" : "buffering"
        return service.supportNotes[group] || ""
    }
    function write(key, value) {
        if (!editable(key)) return false
        const accepted = service.setValue(key, value)
        failedKey = accepted ? "" : key
        return accepted
    }
    function choiceName(key, value) {
        if (key === "cipherPreference" && value === "NORMAL") return qsTr("Balanced")
        if (service) for (const entry of service.choices[key] || []) if (entry.value === value) return entry.name
        return value === undefined ? qsTr("Unavailable") : qsTr("Custom (%1)").arg(value)
    }
    function closeChoices() {
        buffering.popup.close(); cipherPreference.popup.close(); systemTrust.customPopup.close()
        for (const row of [bufferingRow,proxyRow,passwordRow,trustRow,cipherRow]) row.helpButton.tipOpen = false
    }
    function focusActivePopup() {
        for (const choice of [buffering,cipherPreference]) if (choice.popup.visible) { choice.popup.forceActiveFocus(); return true }
        return false
    }
    onVisibleChanged: {
        if (!visible) { closeChoices(); proxyPassword.clearDraft() }
        else if (service) service.reload()
    }
    FontLoader { id: inter; source: root.assetBase + "InterVariable.ttf" }
    component SettingLabel: JuxtopposedSettingsFields.SettingLabel { settings: root }
    component SettingButton: JuxtopposedSettingsFields.SettingButton { settings: root }
    component SettingRow: JuxtopposedSettingsFields.SettingRow { settings: root; infoAssetName: "network-settings-1f9fe.svg"; height: implicitHeight }
    component SectionHeading: JuxtopposedSettingsFields.SectionHeading { settings: root }
    component Choice: JuxtopposedSettingsChoice { settings: root; arrowAssetName: "network-settings-764c9.svg" }
    component FeedbackTip: JuxtopposedSettingsFields.FeedbackTip { settings: root }
    Connections {
        target: root.service
        function onLastErrorChanged() { if (!root.service.lastError.length) root.failedKey = "" }
        function onAdvancedOptionsRequested() { root.closeChoices(); root.advancedRequested() }
        function onValuesChanged() { if (!root.service.capabilities.httpProxyPassword) proxyPassword.clearDraft() }
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
        color: root.tone("#8F8F8F"); placeholderTextColor: root.tone("#8F8F8F"); selectionColor: root.accentDim; selectedTextColor: root.tone("#C6C6C6")
        leftPadding: 12 * root.uiScale; rightPadding: 18 * root.uiScale; topPadding: 4 * root.uiScale; bottomPadding: 4 * root.uiScale
        Accessible.name: accessibleTitle; Accessible.description: root.helpFor(valueKey)
        placeholderText: valueKey === "httpProxy" ? root.values.proxyAddressCustom ? qsTr("Custom saved proxy") : qsTr("System proxy") : ""
        function sync() {
            text = milliseconds ? root.values.buffering !== 0 ? "" : typeof root.values[valueKey] === "number" ? String(root.values[valueKey]) + " ms" : ""
                : root.values[valueKey] || ""
            dirty = false
        }
        Component.onCompleted: sync()
        onTextEdited: dirty = true
        Connections { target: root.service; function onValuesChanged() { if (!field.activeFocus) field.sync() } }
        onEditingFinished: {
            if (!dirty || !enabled) return
            const raw = text.trim(); const numeric = raw.replace(/\s*ms$/, "")
            const candidate = milliseconds ? /^\d+$/.test(numeric) ? Number(numeric) : text : text
            if (root.write(valueKey, candidate)) sync()
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
        SettingLabel { objectName: "networkError"; width: parent.width; text: root.service ? root.service.lastError : ""; visible: text.length > 0; color: "#D98779"; Accessible.name: text }
        SettingLabel { objectName: "networkStatus"; width: parent.width; text: root.service ? root.service.status : ""; visible: text.length > 0; Accessible.name: text }
        Column {
            width: parent.width; visible: root.sectionMatches([qsTr("Buffering"),qsTr("Local files"),qsTr("Discs"),qsTr("Network streams"),qsTr("Live capture")])
            SectionHeading { text: qsTr("Buffering") }
            SettingRow { id: bufferingRow; objectName: "networkBufferingRow"; title: qsTr("Buffering"); help: root.helpFor("buffering"); infoName: "networkBufferingInfo"; visible: root.matches(title); Choice { id: buffering; objectName: "networkBuffering"; valueKey: "buffering"; accessibleTitle: bufferingRow.title } }
            SettingRow { id: localRow; objectName: "networkLocalRow"; title: qsTr("Local files"); minimumRowHeight: 38 * root.uiScale; opacity: root.supported("localFiles") ? 1 : .3; visible: root.matches(title) || root.matches(qsTr("Buffering")); Field { id: localFiles; objectName: "networkLocalFiles"; valueKey: "localFiles"; accessibleTitle: localRow.title; milliseconds: true } }
            SettingRow { id: discRow; objectName: "networkDiscRow"; title: qsTr("Discs"); minimumRowHeight: 38 * root.uiScale; opacity: root.supported("discs") ? 1 : .3; visible: root.matches(title) || root.matches(qsTr("Buffering")); Field { id: discs; objectName: "networkDiscs"; valueKey: "discs"; accessibleTitle: discRow.title; milliseconds: true } }
            SettingRow { id: streamsRow; objectName: "networkStreamsRow"; title: qsTr("Network streams"); minimumRowHeight: 38 * root.uiScale; opacity: root.supported("networkStreams") ? 1 : .3; visible: root.matches(title) || root.matches(qsTr("Buffering")); Field { id: networkStreams; objectName: "networkStreams"; valueKey: "networkStreams"; accessibleTitle: streamsRow.title; milliseconds: true } }
            SettingRow { id: captureRow; objectName: "networkCaptureRow"; title: qsTr("Live capture"); minimumRowHeight: 38 * root.uiScale; opacity: root.supported("liveCapture") ? 1 : .3; visible: root.matches(title) || root.matches(qsTr("Buffering")); Field { id: liveCapture; objectName: "networkLiveCapture"; valueKey: "liveCapture"; accessibleTitle: captureRow.title; milliseconds: true } }
        }
        Column {
            width: parent.width; visible: root.sectionMatches([qsTr("Proxy"),qsTr("HTTP proxy"),qsTr("HTTP proxy password")])
            SectionHeading { text: qsTr("Proxy") }
            SettingRow { id: proxyRow; objectName: "networkProxyRow"; title: qsTr("HTTP proxy"); minimumRowHeight: 38 * root.uiScale; help: root.helpFor("httpProxy"); infoName: "networkProxyInfo"; visible: root.matches(title) || root.matches(qsTr("Proxy")); Field { id: httpProxy; objectName: "networkHttpProxy"; valueKey: "httpProxy"; accessibleTitle: proxyRow.title } }
            SettingRow {
                id: passwordRow; objectName: "networkPasswordRow"; title: qsTr("HTTP proxy password"); minimumRowHeight: 38 * root.uiScale; help: root.helpFor("httpProxyPassword"); infoName: "networkPasswordInfo"; visible: root.matches(title) || root.matches(qsTr("Proxy"))
                C.TextField {
                    id: proxyPassword; objectName: "networkProxyPassword"
                    property bool dirty: false
                    anchors.fill: parent; enabled: root.editable("httpProxyPassword"); activeFocusOnTab: true
                    echoMode: TextInput.Password; passwordMaskDelay: 0; selectByMouse: true
                    font.family: inter.name; font.pixelSize: 14 * root.uiScale; color: root.tone("#8F8F8F")
                    leftPadding: 12 * root.uiScale; rightPadding: passwordClear.visible ? 70 * root.uiScale : 18 * root.uiScale
                    selectionColor: root.accentDim; selectedTextColor: root.tone("#C6C6C6")
                    placeholderText: root.values.proxyPasswordConfigured ? qsTr("Saved password") : ""
                    placeholderTextColor: root.tone("#8F8F8F")
                    Accessible.name: passwordRow.title; Accessible.description: root.helpFor("httpProxyPassword")
                    function clearDraft() { text = ""; dirty = false }
                    onTextEdited: dirty = true
                    onEditingFinished: { if (dirty && enabled && root.write("httpProxyPassword",text)) clearDraft() }
                    onActiveFocusChanged: root.reveal(proxyPassword)
                    Keys.onEscapePressed: event => { clearDraft(); root.failedKey = ""; event.accepted = true }
                    Keys.onTabPressed: event => { if (root.focusNavigator) { root.navigate(proxyPassword,false); event.accepted = true } }
                    Keys.onBacktabPressed: event => { if (root.focusNavigator) { root.navigate(proxyPassword,true); event.accepted = true } }
                    background: Rectangle { color: root.tone("#090909"); radius: 6 * root.uiScale; border.width: 2 * root.uiScale; border.color: proxyPassword.visualFocus ? root.accent : root.tone("#191919") }
                    SettingButton { id: passwordClear; objectName: "networkClearPassword"; anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; anchors.rightMargin: 4 * root.uiScale; width: 60 * root.uiScale; height: 28 * root.uiScale; visible: root.values.proxyPasswordConfigured === true; enabled: root.editable("httpProxyPassword"); text: qsTr("Clear"); Accessible.name: qsTr("Clear saved proxy password"); onClicked: { if (root.write("httpProxyPassword","")) proxyPassword.clearDraft() } }
                    FeedbackTip { control: proxyPassword; valueKey: "httpProxyPassword" }
                }
            }
        }
        Column {
            width: parent.width; visible: root.sectionMatches([qsTr("Security"),qsTr("Verify certificates against the system trust store"),qsTr("Cipher preference")])
            SectionHeading { text: qsTr("Security") }
            SettingRow { id: trustRow; objectName: "networkTrustRow"; title: qsTr("Verify certificates against the system trust store"); controlHeight: 32 * root.uiScale; help: root.helpFor("systemTrust"); infoName: "networkTrustInfo"; visible: root.matches(title) || root.matches(qsTr("Security")); JuxtopposedSettingsFields.Toggle { id: systemTrust; objectName: "networkSystemTrust"; settings: root; valueKey: "systemTrust"; accessibleTitle: trustRow.title; enabled: root.editable(valueKey) } }
            SettingRow { id: cipherRow; objectName: "networkCipherRow"; title: qsTr("Cipher preference"); help: root.helpFor("cipherPreference"); infoName: "networkCipherInfo"; visible: root.matches(title) || root.matches(qsTr("Security")); Choice { id: cipherPreference; objectName: "networkCipherPreference"; valueKey: "cipherPreference"; accessibleTitle: cipherRow.title } }
        }
        SettingButton {
            id: advanced; objectName: "networkAdvanced"; width: parent.width; height: 38 * root.uiScale
            visible: root.matches(qsTr("Advanced Options")); enabled: root.editable("advanced")
            Accessible.name: qsTr("Advanced Options")
            onClicked: { const accepted = root.service.openAdvanced(); root.failedKey = accepted ? "" : "advanced" }
            background: Rectangle { color: root.tone("#191919"); opacity: .95; radius: 2 * root.uiScale; border.width: advanced.visualFocus ? root.uiScale : 0; border.color: root.accent }
            contentItem: Item { Row { spacing: 10 * root.uiScale; anchors.centerIn: parent; SettingLabel { text: qsTr("Advanced Options"); anchors.verticalCenter: parent.verticalCenter } Item { width: 24 * root.uiScale; height: width; Image { objectName: "networkAdvancedOriginalChevron"; anchors.centerIn: parent; width: 24; height: 24; scale: root.uiScale; mirror: true; source: root.assetBase + "network-settings-97863.svg"; Accessible.ignored: true } } } }
        }
    }
}
