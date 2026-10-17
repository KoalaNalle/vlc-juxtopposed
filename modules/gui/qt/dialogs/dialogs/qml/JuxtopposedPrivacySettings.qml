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
        enabled: !root.busy
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
        property alias helpButton:info
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
                activeFocusOnTab: true
                Accessible.description:row.help
                padding: 0
                Accessible.name: qsTr("About %1").arg(row.title)
                background: Item {}
                contentItem: Item {
                    Image {
                        objectName: row.infoName + "Original"
                        anchors.centerIn: parent
                        width: 18; height: 18; scale: root.uiScale
                        source: root.assetBase + "privacy-settings-1f9fe.svg"
                        Accessible.ignored: true
                    }
                }
                onClicked: tipOpen = !tipOpen
                onActiveFocusChanged:root.reveal(info)
                Keys.onTabPressed:event=>{root.navigate(info,false);event.accepted=true}
                Keys.onBacktabPressed:event=>{root.navigate(info,true);event.accepted=true}
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
    component Toggle: SettingButton {
        id: toggle
        FeedbackTip{control:toggle;valueKey:toggle.valueKey}
        required property string valueKey
        required property string accessibleTitle
        readonly property bool custom:typeof root.values[valueKey]!=="boolean"
        readonly property bool selected: root.values[valueKey] === true
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        width: 68 * root.uiScale; height: 32 * root.uiScale
        enabled: root.available && !root.busy
        Accessible.name: accessibleTitle
        Accessible.role: custom ? Accessible.Button : Accessible.CheckBox
        Accessible.checkable: !custom
        Accessible.checked: selected
        Accessible.description: (custom?qsTr("Custom saved value. Choose On or Off to replace it.")+" ":"")+root.helpFor(valueKey)+(root.failedKey===valueKey&&root.service?" "+root.service.lastError:"")
        property alias customPopup:customMenu
        onClicked:if(custom)customMenu.open();else root.write(valueKey,!selected)
        Keys.onLeftPressed:root.write(valueKey,false)
        Keys.onRightPressed:root.write(valueKey,true)
        C.Menu {
            id:customMenu;objectName:toggle.objectName+"CustomMenu";parent:toggle;focus:true
            y:toggle.height+2*root.uiScale;width:Math.max(120*root.uiScale,toggle.width)
            font.family:inter.name;font.pixelSize:14*root.uiScale
            C.MenuItem{objectName:toggle.objectName+"CustomOn";text:qsTr("On");onTriggered:root.write(toggle.valueKey,true)}
            C.MenuItem{objectName:toggle.objectName+"CustomOff";text:qsTr("Off");onTriggered:root.write(toggle.valueKey,false)}
        }
        background: Rectangle {
            color: root.tone("#090909"); radius: 6 * root.uiScale
            border.width: toggle.visualFocus ? root.uiScale : 0; border.color: root.accent
        }
        contentItem: Item {
            Rectangle { visible:!toggle.custom; x: (toggle.selected ? 3 : 32) * root.uiScale; y: 3 * root.uiScale; width: 33 * root.uiScale; height: 26 * root.uiScale; radius: 3 * root.uiScale; color: toggle.selected ? root.accentSelected : root.tone("#191919") }
            Rectangle { visible:!toggle.custom; x: (toggle.selected ? 39 : 3) * root.uiScale; y: 3 * root.uiScale; width: 26 * root.uiScale; height: 26 * root.uiScale; radius: 3 * root.uiScale; color: toggle.selected ? root.accentDim : root.tone("#404040") }
            SettingLabel { width:toggle.custom?parent.width:26*root.uiScale;wrapMode:Text.NoWrap;elide:Text.ElideRight;horizontalAlignment:toggle.custom?Text.AlignHCenter:Text.AlignLeft; x: toggle.custom?0:(toggle.selected ? 10 : 36) * root.uiScale; anchors.verticalCenter: parent.verticalCenter; text:toggle.custom?qsTr("Custom"):toggle.selected ? qsTr("ON") : qsTr("OFF"); color: toggle.selected ? root.accent : root.tone("#6B6B6B"); font.pixelSize: 12 * root.uiScale }
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


    Connections{target:root.service;function onLastErrorChanged(){if(!root.service.lastError.length)root.failedKey=""}}
    component FeedbackTip:C.ToolTip {
        id:tip
        required property Item control
        required property string valueKey
        objectName:control.objectName+"ValidationError";parent:control
        y:-height-4*root.uiScale;width:Math.min(Math.max(control.width,200*root.uiScale),root.width)
        visible:root.failedKey===valueKey&&root.service&&root.service.lastError.length>0&&control.activeFocus&&control.visible
        text:root.service?root.service.lastError:"";focus:false;closePolicy:C.Popup.NoAutoClose;timeout:0;padding:8*root.uiScale
        function reveal(){if(!visible)return;root.reveal(control);Qt.callLater(()=>{if(!visible)return;const p=control.mapToItem(root,0,0);x=Math.max(-p.x,Math.min(0,root.width-p.x-width));if(background)root.focusRevealed(background)})}
        onVisibleChanged:if(visible)Qt.callLater(reveal)
        Connections{target:form;function onImplicitHeightChanged(){if(tip.visible)Qt.callLater(tip.reveal)}}
        contentItem:SettingLabel{text:tip.text;color:"#D98779";Accessible.role:Accessible.StaticText;Accessible.name:text}
        background:Rectangle{color:root.tone("#191919");radius:4*root.uiScale;border.width:root.uiScale;border.color:"#D98779"}
    }
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
