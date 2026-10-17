// SPDX-License-Identifier: GPL-2.0-or-later
// Original design and unchanged SVG artwork: Juxtopposed, CC BY 4.0.
import QtQuick
import QtQuick.Controls.Basic as C
FocusScope {
    id: root
    objectName: "juxtopposedExtensionsSettings"
    required property var preferences
    property var extensionsPreferences: null
    readonly property var service: extensionsPreferences
    readonly property var addons: service ? service.addons : null
    property var appearance: null
    property real uiScale: 1
    property url assetBase: "qrc:/juxtopposed/"
    property string searchText: ""
    property var focusNavigator: null
    signal focusRevealed(var item)
    signal advancedRequested()
    readonly property var values: service ? service.values : ({})
    readonly property bool available: !!service
    readonly property bool busy: !!service && service.busy
    readonly property bool working: !!addons && addons.busy
    readonly property bool stacked: width < 650 * uiScale
    readonly property color accent: appearance ? appearance.accent : "#D37918"
    readonly property color accentSelected: appearance ? appearance.accentSelected : "#371F06"
    readonly property color accentDim: appearance ? appearance.accentDim : "#AB6314"
    readonly property string settingsFontFamily: inter.name
    readonly property alias formItem: form
    property string failedKey: ""
    property string appliedTiming: ""
    readonly property var focusTargets: [list, localSearch, findMore, add, remove, startup, retry, advanced]
    implicitHeight: form.implicitHeight
    function tone(value) { return appearance ? appearance.color(value) : value }
    function helpFor(key) { return service ? (service.supportNotes[key] || "") : qsTr("This setting is unavailable in this build.") }
    function matches(value) { return !searchText.trim().length || value.toLocaleLowerCase().includes(searchText.trim().toLocaleLowerCase()) }
    function navigate(item, backwards) { if (focusNavigator) focusNavigator(item, backwards) }
    function reveal(item) { if (item.activeFocus) focusRevealed(item) }
    function write(key, value) {
        if (!service || busy) return false
        const accepted = service.setValue(key, value)
        failedKey = accepted ? "" : key
        appliedTiming = accepted ? qsTr("Saved. Applies on the next VLC start.") : ""
        return accepted
    }
    function runAction(action) {
        failedKey = ""; appliedTiming = ""
        return service && !busy ? action() : false
    }
    function closeChoices() { startup.customPopup.close() }
    function focusActivePopup() { return false }
    function chooseRow(index) {
        if (!addons || index < 0 || index >= list.count) return
        const item = addons.getDataAt(index)
        if (item.selectionKey) { addons.selectedKey = item.selectionKey; list.currentIndex = index; list.positionViewAtIndex(index,ListView.Contain) }
    }
    onVisibleChanged: {
        if (!visible) closeChoices()
        else if (service) { service.reload(); appliedTiming = ""; if (addons) addons.showAll() }
    }
    Component.onCompleted: if (service) { service.reload(); if (addons) addons.showAll() }
    FontLoader { id: inter; source: root.assetBase + "InterVariable.ttf" }
    component SettingLabel: JuxtopposedSettingsFields.SettingLabel { settings: root }
    component SettingButton: JuxtopposedSettingsFields.SettingButton { settings: root; font.weight: Font.DemiBold }
    Connections {
        target: root.service
        function onAdvancedOptionsRequested() { root.closeChoices(); root.advancedRequested() }
    }
    Column {
        id: form; width: root.width; spacing: 20 * root.uiScale
        Column {
            width: parent.width; spacing: 8 * root.uiScale
            visible: (error.text.length || status.text.length || retry.visible) > 0
            SettingLabel { id: error; objectName: "extensionsError"; width: parent.width; visible: text.length > 0; text: root.service ? root.service.lastError : ""; color: "#D98779"; Accessible.name: text }
            SettingLabel { id: status; objectName: "extensionsStatus"; width: parent.width; visible: text.length > 0; text: root.appliedTiming || (root.addons ? root.addons.statusText : ""); Accessible.name: text }
            SettingButton { id: retry; objectName: "extensionsRetryCatalog"; width: Math.max(180 * root.uiScale,implicitWidth); text: qsTr("Retry saving catalog"); visible: !!root.addons && root.addons.catalogSaveNeeded; enabled: !root.working && !root.busy; onClicked: root.runAction(() => root.service.retryCatalogSave()) }
        }
        Column {
            width: parent.width; spacing: 0
            visible: root.matches(qsTr("Add-ons & Scripts")) || root.matches(qsTr("Add-ons")) || root.matches(qsTr("Search")) || root.matches(qsTr("Find more extensions")) || root.matches(qsTr("Add")) || root.matches(qsTr("Remove")) || root.matches(qsTr("Load add-ons on startup"))
            Item {
                width: parent.width; height: 39 * root.uiScale
                SettingLabel { anchors.verticalCenter: parent.verticalCenter; text: qsTr("Add-ons"); font.pixelSize: 16 * root.uiScale; font.weight: Font.Medium; color: root.tone("#C6C6C6") }
            }
            Rectangle {
                objectName: "extensionsListFrame"; width: parent.width; height: 194 * root.uiScale
                color: root.tone("#090909"); radius: 6 * root.uiScale; border.width: 2 * root.uiScale
                border.color: list.activeFocus ? root.accent : root.tone("#191919")
                C.ScrollView {
                    anchors.fill: parent; anchors.margins: 2 * root.uiScale; clip: true
                    leftPadding: 10 * root.uiScale; rightPadding: 16 * root.uiScale; topPadding: 6 * root.uiScale; bottomPadding: 6 * root.uiScale
                    ListView {
                        id: list; objectName: "extensionsList"; model: root.addons
                        activeFocusOnTab: true; boundsBehavior: Flickable.StopAtBounds
                        Accessible.name: qsTr("Add-ons"); Accessible.role: Accessible.List
                        onActiveFocusChanged: root.reveal(list)
                        Keys.onDownPressed: event => { const index = root.addons ? root.addons.selectionIndex() : -1; root.chooseRow(index >= 0 ? Math.min(count-1,index+1) : 0); event.accepted = true }
                        Keys.onUpPressed: event => { const index = root.addons ? root.addons.selectionIndex() : -1; root.chooseRow(index >= 0 ? Math.max(0,index-1) : count-1); event.accepted = true }
                        Keys.onPressed: event => {
                            if (event.key === Qt.Key_Home) { root.chooseRow(0); event.accepted = true }
                            else if (event.key === Qt.Key_End) { root.chooseRow(count-1); event.accepted = true }
                            else event.accepted = false
                        }
                        Keys.onTabPressed: event => { if (root.focusNavigator) { root.navigate(list,false); event.accepted = true } }
                        Keys.onBacktabPressed: event => { if (root.focusNavigator) { root.navigate(list,true); event.accepted = true } }
                        delegate: C.ItemDelegate {
                            required property int index
                            required property string name
                            required property string author
                            required property string stateLabel
                            required property string selectionKey
                            required property bool manageable
                            width: ListView.view.width; height: 44 * root.uiScale
                            objectName: "extensionRow" + index
                            readonly property bool selected: selectionKey.length > 0 && !!root.addons && root.addons.selectedKey === selectionKey
                            Accessible.name: name + ", " + stateLabel; Accessible.selected: selected
                            Accessible.description: manageable ? author : qsTr("Installed manually. VLC cannot remove this add-on automatically.")
                            onClicked: { root.addons.selectedKey = selectionKey; list.currentIndex = index; list.forceActiveFocus() }
                            background: Rectangle { color: selected ? root.accentSelected : parent.hovered ? root.tone("#191919") : "transparent"; radius: 2 * root.uiScale }
                            contentItem: Row {
                                spacing: 10 * root.uiScale
                                SettingLabel { width: Math.max(0,parent.width - statusLabelItem.implicitWidth - parent.spacing); text: name; color: root.tone("#C6C6C6"); elide: Text.ElideRight; wrapMode: Text.NoWrap }
                                SettingLabel { id: statusLabelItem; text: stateLabel; wrapMode: Text.NoWrap }
                            }
                        }
                    }
                }
            }
            Item {
                width: parent.width; height: controls.implicitHeight + 15 * root.uiScale
                Item {
                    id: controls; width: parent.width; y: 5 * root.uiScale
                    implicitHeight: root.stacked ? localSearch.height + 10 * root.uiScale + actions.implicitHeight : 38 * root.uiScale
                    C.TextField {
                        id: localSearch; objectName: "extensionsSearch"; width: root.stacked ? parent.width : 210 * root.uiScale; height: 36 * root.uiScale
                        anchors.verticalCenter: root.stacked ? undefined : parent.verticalCenter
                        text: root.addons ? root.addons.searchPattern : ""
                        placeholderText: qsTr("Search"); selectByMouse: true; activeFocusOnTab: true
                        font.family: inter.name; font.pixelSize: 14 * root.uiScale
                        color: root.tone("#8F8F8F"); placeholderTextColor: root.tone("#404040")
                        selectionColor: root.accentDim; selectedTextColor: root.tone("#C6C6C6")
                        leftPadding: 12 * root.uiScale; rightPadding: 18 * root.uiScale
                        Accessible.name: qsTr("Search add-ons")
                        onTextEdited: if (root.addons) root.addons.searchPattern = text
                        onActiveFocusChanged: root.reveal(localSearch)
                        Keys.onEscapePressed: event => { if (text.length) { clear(); if (root.addons) root.addons.searchPattern = ""; event.accepted = true } else event.accepted = false }
                        Keys.onTabPressed: event => { if (root.focusNavigator) { root.navigate(localSearch,false); event.accepted = true } }
                        Keys.onBacktabPressed: event => { if (root.focusNavigator) { root.navigate(localSearch,true); event.accepted = true } }
                        background: Rectangle { color: root.tone("#090909"); radius: 6 * root.uiScale; border.width: 2 * root.uiScale; border.color: localSearch.visualFocus ? root.accent : root.tone("#191919") }
                    }
                    Flow {
                        id: actions; width: root.stacked ? parent.width : findMore.width + add.width + remove.width + 2 * spacing
                        anchors.right: parent.right; y: root.stacked ? localSearch.height + 10 * root.uiScale : (parent.implicitHeight-height)/2
                        spacing: 10 * root.uiScale
                        SettingButton { id: findMore; objectName: "extensionsFindMore"; width: implicitWidth; implicitWidth: label.implicitWidth + 40 * root.uiScale; text: qsTr("Find more extensions"); enabled: !!root.addons && root.addons.available && !root.working && !root.busy
                            contentItem: SettingLabel { id: label; text: findMore.text; font.weight: Font.DemiBold; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                            onClicked: root.runAction(() => root.service.findMore())
                        }
                        SettingButton { id: add; objectName: "extensionsAdd"; text: qsTr("Add"); enabled: !!root.service && root.service.capabilities.add === true && !root.working && !root.busy; onClicked: root.runAction(() => root.service.add())
                            C.ToolTip.visible: hovered || visualFocus
                            C.ToolTip.text: root.addons && root.addons.canInstall ? qsTr("Install the selected add-on.") : qsTr("Choose a local VLC package (.vlp).")
                            C.ToolTip.delay: 500
                        }
                        SettingButton { id: remove; objectName: "extensionsRemove"; text: qsTr("Remove"); enabled: !!root.addons && root.addons.canRemove && !root.busy; onClicked: root.runAction(() => root.service.remove()) }
                    }
                }
            }
            Item {
                width: parent.width; height: 42 * root.uiScale
                SettingLabel { anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter; width: Math.max(0,parent.width-startup.width-12*root.uiScale); text: qsTr("Load add-ons on startup") }
                JuxtopposedSettingsFields.Toggle {
                    id: startup; objectName: "extensionsLoadOnStartup"; settings: root; valueKey: "loadOnStartup"; accessibleTitle: qsTr("Load add-ons on startup")
                    anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
                    enabled: !!root.service && root.service.capabilities.loadOnStartup === true && !root.busy
                    C.ToolTip.visible: hovered || visualFocus
                    C.ToolTip.text: root.service ? root.service.supportNotes.loadOnStartup : qsTr("Unavailable")
                    C.ToolTip.delay: 500; C.ToolTip.timeout: 12000
                }
            }
        }
        SettingButton {
            id: advanced; objectName: "extensionsAdvanced"; width: parent.width; implicitHeight: 38 * root.uiScale
            visible: root.matches(qsTr("Advanced Options")); enabled: !!root.service && root.service.capabilities.advanced === true && !root.busy
            font.weight: Font.Normal; onClicked: root.runAction(() => root.service.openAdvanced())
            background: Rectangle { radius: 2 * root.uiScale; color: root.tone("#191919"); opacity: 0.95; border.width: advanced.visualFocus ? 2 * root.uiScale : 0; border.color: root.accent }
            contentItem: Item {
                Row { anchors.centerIn: parent; spacing: 10 * root.uiScale
                    SettingLabel { anchors.verticalCenter: parent.verticalCenter; text: qsTr("Advanced Options") }
                    Image { objectName: "extensionsAdvancedOriginalArrow"; width: 24; height: 24; scale: root.uiScale; mirror: true; source: root.assetBase + "extensions-settings-97863.svg"; Accessible.ignored: true }
                }
            }
        }
    }
}
