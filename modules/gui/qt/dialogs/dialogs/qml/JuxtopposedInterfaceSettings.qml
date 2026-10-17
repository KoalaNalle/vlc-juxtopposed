// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
// Original visual design and SVG artwork: Juxtopposed, CC BY 4.0.
import QtQuick
import QtQuick.Controls.Basic as C
import QtQuick.Layouts

FocusScope {
    id: root
    objectName: "juxtopposedInterfaceSettings"
    required property var preferences
    // The immutable generic shell supplies its General service as preferences.
    // A category Component can explicitly select the separate Interface service.
    property var interfacePreferences: null
    readonly property var service: interfacePreferences
    property var appearance: null
    property real uiScale: 1
    property url assetBase: "qrc:/juxtopposed/"
    property string searchText: ""
    property var focusNavigator: null
    signal focusRevealed(var item)
    signal advancedRequested()
    readonly property var values: service ? service.values : ({})
    readonly property var capabilities: service && "capabilities" in service
        ? service.capabilities : ({})
    readonly property var settingKeys: ["customSkin", "skinFile", "embeddedVideo", "resizeVideo",
        "rememberGeometry", "alwaysOnTop", "windowDecorations", "titleFileName", "fullscreenScreen",
        "compactMode", "controlAutoHide", "controlHideDelay", "fullscreenControls", "systemTray",
        "startMinimized", "trackPopup", "popupDuration", "playlistTree"]
    readonly property var unsupportedKeys: settingKeys.filter(key => !supported(key))
    readonly property bool available: !!service
    readonly property bool busy: !!service && "pickerActive" in service && service.pickerActive
    readonly property bool stacked: width < 650 * uiScale
    readonly property color accent: appearance ? appearance.accent : "#D37918"
    readonly property color accentSelected: appearance ? appearance.accentSelected : "#371F06"
    readonly property color accentDim: appearance ? appearance.accentDim : "#AB6314"
    readonly property var screenOptions: [{name: qsTr("Current screen"), value: -1}].concat(
        service && "screens" in service ? service.screens.map(screen =>
            ({name: screen.name, value: screen.index})) : [])
    readonly property var focusTargets: [customSkin, skinFile, skinBrowse, embeddedVideo,
        resizeVideo, rememberGeometry, alwaysOnTop, windowDecorations, titleFileName,
        fullscreenScreen, compactMode, controlAutoHide, controlHideDelay.control,
        fullscreenControls, systemTray, startMinimized, trackPopup, popupDuration.control,
        playlistTree, advancedButton]
    implicitHeight: form.implicitHeight
    function tone(value) { return appearance ? appearance.color(value) : value }
    function matches(text) { return !searchText.trim().length || text.toLocaleLowerCase().includes(searchText.trim().toLocaleLowerCase()) }
    function sectionMatches(names) { return names.some(name => matches(name)) }
    function navigate(item, backwards) { if (focusNavigator) focusNavigator(item, backwards) }
    function reveal(item) { if (item.activeFocus) focusRevealed(item) }
    function supported(key) { return available && capabilities[key] === true && values[key] !== undefined }
    function editable(key) { return supported(key) && !busy }
    function closeChoices() {
        for (const choice of [resizeVideo, fullscreenScreen, controlAutoHide, trackPopup])
            if (choice && choice.popup.visible) choice.popup.close()
    }
    function focusActivePopup() {
        if (busy) return true
        closeChoices()
        return false
    }
    onVisibleChanged: if (!visible) closeChoices()
    function helpFor(key) {
        const hints = {
            customSkin: qsTr("Use a VLC skin after restarting. Choose a skin file first."),
            skinFile: qsTr("Choose the skin to use after restarting VLC. You can choose a file before turning on custom skins."),
            embeddedVideo: qsTr("Play video inside the main VLC window after restarting."),
            resizeVideo: qsTr("Resize the main window to fit the video, or keep its current size."),
            rememberGeometry: qsTr("Restore the main window size and position next time VLC starts."),
            alwaysOnTop: qsTr("Keep the VLC window above other windows after restarting."),
            windowDecorations: qsTr("Show your system title bar and window borders after restarting."),
            titleFileName: qsTr("Show the current media name in the window title after restarting."),
            fullscreenScreen: qsTr("Choose the screen for fullscreen. A saved choice is kept if that screen is disconnected."),
            compactMode: qsTr("Start VLC in compact view after restarting."),
            controlAutoHide: qsTr("Choose when playback controls hide."),
            controlHideDelay: qsTr("Choose how long playback controls stay visible before hiding."),
            fullscreenControls: qsTr("Show playback controls in fullscreen."),
            systemTray: qsTr("Show a VLC icon in the system tray after restarting. Turn off minimized startup before hiding the icon."),
            startMinimized: qsTr("Start with VLC in the system tray after restarting. The tray icon must be enabled."),
            trackPopup: qsTr("Show a notification when the current track changes. The tray icon must be enabled."),
            popupDuration: qsTr("Choose how long notifications should stay visible. Your system may use its own timing."),
            playlistTree: qsTr("Show the playlist as a tree of folders and items.")
        }
        return (hints[key] || "") + (supported(key) ? "" : " " + qsTr("This setting is unavailable in this build."))
    }
    function write(key, value) { return editable(key) && service.setValue(key, value) }
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
        Accessible.name: button.text
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
        enabled: root.available && !root.busy
        activeFocusOnTab: true
        font.family: inter.name
        font.pixelSize: 14 * root.uiScale
        Accessible.name: accessibleTitle
        Accessible.description: (currentIndex < 0 ? qsTr("The current setting has no matching preset. Choose a value to replace it.") + " " : "") + root.helpFor(valueKey)
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
            target: root.service
            function onValuesChanged() { choice.sync() }
        }
        onOptionsChanged: Qt.callLater(sync)
        onCountChanged: Qt.callLater(sync)
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
        enabled: root.available && !root.busy
        Accessible.name: accessibleTitle
        Accessible.role: Accessible.CheckBox
        Accessible.checkable: true
        Accessible.checked: selected
        Accessible.description: root.helpFor(valueKey)
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

    component DurationRange: Item {
        id: range
        required property string valueKey
        required property string accessibleTitle
        property alias control: slider
        readonly property bool known: Number.isFinite(root.values[valueKey]) && root.values[valueKey] >= 0 && root.values[valueKey] <= 10000
        anchors.fill: parent
        C.Slider {
            id: slider
            objectName: "interface" + range.valueKey
            y: 0
            width: parent.width; height: 24 * root.uiScale
            from: 0; to: 10; stepSize: .5
            padding: 0; leftPadding: 2 * root.uiScale; rightPadding: 2 * root.uiScale
            value: range.known ? root.values[range.valueKey] / 1000 : 0
            enabled: root.editable(range.valueKey)
            activeFocusOnTab: true
            Accessible.name: range.accessibleTitle
            Accessible.description: root.helpFor(range.valueKey) + (range.known ? "" : " " + qsTr("The current value is outside the 0 to 10 second scale."))
            function sync() { value = range.known ? root.values[range.valueKey] / 1000 : 0 }
            onMoved: { root.write(range.valueKey, Math.round(value * 1000)); sync() }
            Connections { target: root.service; function onValuesChanged() { slider.sync() } }
            Keys.onTabPressed: event => { if (root.focusNavigator) { root.navigate(slider, false); event.accepted = true } }
            Keys.onBacktabPressed: event => { if (root.focusNavigator) { root.navigate(slider, true); event.accepted = true } }
            onActiveFocusChanged: root.reveal(slider)
            background: Rectangle {
                y: 4 * root.uiScale
                width: slider.width; height: 16 * root.uiScale
                color: root.tone("#090909"); radius: 6 * root.uiScale
                border.width: slider.visualFocus ? root.uiScale : 0; border.color: root.accent
                Rectangle { x: 2 * root.uiScale; y: 2 * root.uiScale; width: Math.max(0, slider.width - 4 * root.uiScale) * slider.visualPosition; height: 12 * root.uiScale; radius: 4 * root.uiScale; color: root.accentSelected }
                Repeater {
                    model: 21
                    Item {
                        required property int index
                        x: 2 * root.uiScale + (slider.width - 4 * root.uiScale) * index / 20
                        y: 5 * root.uiScale
                        width: 0; height: 6 * root.uiScale
                        clip: index === 0 || index === 20
                        Image {
                            objectName: "interface" + range.valueKey + "OriginalTick" + index
                            anchors.centerIn: parent
                            width: index === 0 || index === 20 ? 32 : 2
                            height: index === 0 || index === 20 ? 32 : 8
                            scale: root.uiScale
                            source: root.assetBase + (index === 0 || index === 20 ? "interface-settings-8ba8e.svg" : "interface-settings-1ab84.svg")
                            Accessible.ignored: true
                        }
                    }
                }
            }
            handle: Rectangle {
                x: 2 * root.uiScale + (slider.width - 4 * root.uiScale) * slider.visualPosition - width / 2
                y: 0; width: 8 * root.uiScale; height: 24 * root.uiScale
                radius: 4 * root.uiScale; color: root.accentDim
            }
        }
        Row {
            y: 28 * root.uiScale; width: parent.width
            SettingLabel { width: parent.width / 3; text: "0 s"; font.pixelSize: 12 * root.uiScale; color: root.tone("#6B6B6B") }
            SettingLabel { width: parent.width / 3; text: "5 s"; horizontalAlignment: Text.AlignHCenter; font.pixelSize: 12 * root.uiScale; color: root.tone("#6B6B6B") }
            SettingLabel { width: parent.width / 3; text: "10s"; horizontalAlignment: Text.AlignRight; font.pixelSize: 12 * root.uiScale; color: root.tone("#6B6B6B") }
        }
        SettingLabel {
            y: 46 * root.uiScale; width: parent.width
            visible: !range.known
            text: root.values[range.valueKey] === undefined ? qsTr("Unavailable") : qsTr("Current value: %1 ms").arg(root.values[range.valueKey])
            font.pixelSize: 12 * root.uiScale
        }
    }
    Column {
        id: form
        width: root.width
        spacing: 20 * root.uiScale
        SettingLabel {
            objectName: "interfaceSettingsError"
            width: parent.width
            text: root.service ? root.service.lastError : ""
            visible: text.length > 0
            color: "#D98779"
            Accessible.role: Accessible.StaticText; Accessible.name: text
        }
        SettingLabel {
            objectName: "interfaceSettingsRestart"
            width: parent.width
            text: qsTr("Restart VLC to apply these changes.")
            visible: root.service && root.service.restartRequired
            color: root.tone("#C6C6C6")
            Accessible.role: Accessible.StaticText; Accessible.name: text
        }
        SettingLabel {
            objectName: "interfaceUnsupportedMessage"
            width: parent.width
            visible: root.unsupportedKeys.length > 0
            text: qsTr("%1 settings are unavailable in this build.").arg(root.unsupportedKeys.length)
            Accessible.role: Accessible.StaticText
            Accessible.name: text
        }
        Column {
            width: parent.width
            visible: root.sectionMatches([qsTr("Skin"), qsTr("Use a custom skin"), qsTr("Skin file")])
            SectionHeading { text: qsTr("Skin") }
            SettingRow {
                title: qsTr("Use a custom skin")
                visible: root.matches(title) || root.matches(qsTr("Skin"))
                opacity: root.supported("customSkin") ? 1 : .5
                help: root.helpFor("customSkin"); infoName: "interfacecustomSkinInfo"
                controlHeight: 32 * root.uiScale
                Toggle { id: customSkin; objectName: "interfacecustomSkin"; valueKey: "customSkin"; accessibleTitle: qsTr("Use a custom skin"); enabled: root.editable("customSkin") }
            }
            SettingRow {
                title: qsTr("Skin file")
                visible: root.matches(title) || root.matches(qsTr("Skin"))
                opacity: root.supported("skinFile") ? 1 : .5
                help: root.helpFor("skinFile"); infoName: "interfaceskinFileInfo"
                controlHeight: 36 * root.uiScale
                RowLayout {
                    anchors.fill: parent; spacing: 10 * root.uiScale
                    C.TextField {
                        id: skinFile; objectName: "interfaceskinFile"
                        Layout.fillWidth: true; Layout.minimumWidth: 0; Layout.fillHeight: true
                        enabled: root.editable("skinFile")
                        text: root.values.skinFile || ""; selectByMouse: true; activeFocusOnTab: true
                        font.family: inter.name; font.pixelSize: 14 * root.uiScale
                        color: root.tone("#8F8F8F"); selectionColor: root.accentSelected
                        leftPadding: 12 * root.uiScale; rightPadding: 18 * root.uiScale
                        Accessible.name: qsTr("Skin file"); Accessible.description: root.helpFor("skinFile")
                        onEditingFinished: { if (text !== root.values.skinFile) root.write("skinFile", text); text = Qt.binding(() => root.values.skinFile || "") }
                        onActiveFocusChanged: root.reveal(skinFile)
                        Keys.onTabPressed: event => { if (root.focusNavigator) { root.navigate(skinFile, false); event.accepted = true } }
                        Keys.onBacktabPressed: event => { if (root.focusNavigator) { root.navigate(skinFile, true); event.accepted = true } }
                        background: Rectangle { color: root.tone("#090909"); radius: 6 * root.uiScale; border.width: 2 * root.uiScale; border.color: skinFile.activeFocus ? root.accent : root.tone("#191919") }
                    }
                    SettingButton {
                        id: skinBrowse; objectName: "interfaceskinBrowse"
                        Layout.preferredWidth: 100 * root.uiScale; Layout.preferredHeight: 32 * root.uiScale
                        text: qsTr("Browse"); font.weight: Font.DemiBold
                        enabled: root.editable("skinFile") && "chooseSkinFile" in root.service
                        Accessible.name: qsTr("Choose skin file")
                        onClicked: root.service.chooseSkinFile()
                    }
                }
            }
        }
        Column {
            width: parent.width
            visible: root.sectionMatches([qsTr("Window"), qsTr("Play video inside main window"), qsTr("Resize window to fit video"), qsTr("Remember window size and position"), qsTr("Keep window above other windows"), qsTr("Show window decorations"), qsTr("Show the file name in the title bar"), qsTr("Open fullscreen on")])
            SectionHeading { text: qsTr("Window") }
            SettingRow {
                title: qsTr("Play video inside main window")
                visible: root.matches(title) || root.matches(qsTr("Window"))
                opacity: root.supported("embeddedVideo") ? 1 : .5
                help: root.helpFor("embeddedVideo"); infoName: "interfaceembeddedVideoInfo"
                controlHeight: 32 * root.uiScale
                Toggle { id: embeddedVideo; objectName: "interfaceembeddedVideo"; valueKey: "embeddedVideo"; accessibleTitle: qsTr("Play video inside main window"); enabled: root.editable("embeddedVideo") }
            }
            SettingRow {
                title: qsTr("Resize window to fit video")
                visible: root.matches(title) || root.matches(qsTr("Window"))
                opacity: root.supported("resizeVideo") ? 1 : .5
                help: root.helpFor("resizeVideo"); infoName: "interfaceresizeVideoInfo"
                controlHeight: 36 * root.uiScale
                Choice { id: resizeVideo; objectName: "interfaceresizeVideo"; valueKey: "resizeVideo"; accessibleTitle: qsTr("Resize window to fit video"); options: [{name: qsTr("Never"), value: false}, {name: qsTr("Always"), value: true}]; enabled: root.editable("resizeVideo") && root.values.embeddedVideo === true }
            }
            SettingRow {
                title: qsTr("Remember window size and position")
                visible: root.matches(title) || root.matches(qsTr("Window"))
                opacity: root.supported("rememberGeometry") ? 1 : .5
                controlHeight: 32 * root.uiScale
                Toggle { id: rememberGeometry; objectName: "interfacerememberGeometry"; valueKey: "rememberGeometry"; accessibleTitle: qsTr("Remember window size and position"); enabled: root.editable("rememberGeometry") }
            }
            SettingRow {
                title: qsTr("Keep window above other windows")
                visible: root.matches(title) || root.matches(qsTr("Window"))
                opacity: root.supported("alwaysOnTop") ? 1 : .5
                help: root.helpFor("alwaysOnTop"); infoName: "interfacealwaysOnTopInfo"
                controlHeight: 32 * root.uiScale
                Toggle { id: alwaysOnTop; objectName: "interfacealwaysOnTop"; valueKey: "alwaysOnTop"; accessibleTitle: qsTr("Keep window above other windows"); enabled: root.editable("alwaysOnTop") }
            }
            SettingRow {
                title: qsTr("Show window decorations")
                visible: root.matches(title) || root.matches(qsTr("Window"))
                opacity: root.supported("windowDecorations") ? 1 : .5
                help: root.helpFor("windowDecorations"); infoName: "interfacewindowDecorationsInfo"
                controlHeight: 32 * root.uiScale
                Toggle { id: windowDecorations; objectName: "interfacewindowDecorations"; valueKey: "windowDecorations"; accessibleTitle: qsTr("Show window decorations"); enabled: root.editable("windowDecorations") }
            }
            SettingRow {
                title: qsTr("Show the file name in the title bar")
                visible: root.matches(title) || root.matches(qsTr("Window"))
                opacity: root.supported("titleFileName") ? 1 : .5
                help: root.helpFor("titleFileName"); infoName: "interfacetitleFileNameInfo"
                controlHeight: 32 * root.uiScale
                Toggle { id: titleFileName; objectName: "interfacetitleFileName"; valueKey: "titleFileName"; accessibleTitle: qsTr("Show the file name in the title bar"); enabled: root.editable("titleFileName") }
            }
            SettingRow {
                title: qsTr("Open fullscreen on")
                visible: root.matches(title) || root.matches(qsTr("Window"))
                opacity: root.supported("fullscreenScreen") ? 1 : .5
                help: root.helpFor("fullscreenScreen"); infoName: "interfacefullscreenScreenInfo"
                controlHeight: 36 * root.uiScale
                Choice { id: fullscreenScreen; objectName: "interfacefullscreenScreen"; valueKey: "fullscreenScreen"; accessibleTitle: qsTr("Open fullscreen on"); options: root.screenOptions; enabled: root.editable("fullscreenScreen") }
            }
        }
        Column {
            width: parent.width
            visible: root.sectionMatches([qsTr("Playback Controls"), qsTr("Start in compact mode"), qsTr("Auto-hide the controls"), qsTr("Hide after"), qsTr("Show controls in fullscreen")])
            SectionHeading { text: qsTr("Playback Controls") }
            SettingRow {
                title: qsTr("Start in compact mode")
                visible: root.matches(title) || root.matches(qsTr("Playback Controls"))
                opacity: root.supported("compactMode") ? 1 : .5
                help: root.helpFor("compactMode"); infoName: "interfacecompactModeInfo"
                controlHeight: 32 * root.uiScale
                Toggle { id: compactMode; objectName: "interfacecompactMode"; valueKey: "compactMode"; accessibleTitle: qsTr("Start in compact mode"); enabled: root.editable("compactMode") }
            }
            SettingRow {
                title: qsTr("Auto-hide the controls")
                visible: root.matches(title) || root.matches(qsTr("Playback Controls"))
                opacity: root.supported("controlAutoHide") ? 1 : .5
                help: root.helpFor("controlAutoHide"); infoName: "interfacecontrolAutoHideInfo"
                controlHeight: 36 * root.uiScale
                Choice { id: controlAutoHide; objectName: "interfacecontrolAutoHide"; valueKey: "controlAutoHide"; accessibleTitle: qsTr("Auto-hide the controls"); options: [{name: qsTr("Never"), value: 0}, {name: qsTr("In fullscreen only"), value: 1}, {name: qsTr("Always"), value: 2}]; enabled: root.editable("controlAutoHide") }
            }
            SettingRow {
                title: qsTr("Hide after")
                visible: root.matches(title) || root.matches(qsTr("Playback Controls"))
                opacity: root.supported("controlHideDelay") ? 1 : .5
                controlHeight: (Number.isFinite(root.values.controlHideDelay) && root.values.controlHideDelay >= 0 && root.values.controlHideDelay <= 10000 ? 46 : 66) * root.uiScale
                DurationRange { id: controlHideDelay; valueKey: "controlHideDelay"; accessibleTitle: qsTr("Hide after"); control.enabled: root.editable("controlHideDelay") }
            }
            SettingRow {
                title: qsTr("Show controls in fullscreen")
                visible: root.matches(title) || root.matches(qsTr("Playback Controls"))
                opacity: root.supported("fullscreenControls") ? 1 : .5
                help: root.helpFor("fullscreenControls"); infoName: "interfacefullscreenControlsInfo"
                controlHeight: 32 * root.uiScale
                Toggle { id: fullscreenControls; objectName: "interfacefullscreenControls"; valueKey: "fullscreenControls"; accessibleTitle: qsTr("Show controls in fullscreen"); enabled: root.editable("fullscreenControls") }
            }
        }
        Column {
            width: parent.width
            visible: root.sectionMatches([qsTr("Notifications and System Tray"), qsTr("Show a system tray icon"), qsTr("Start minimized to the tray"), qsTr("Show a popup when the track changes"), qsTr("Popup duration")])
            SectionHeading { text: qsTr("Notifications and System Tray") }
            SettingRow {
                title: qsTr("Show a system tray icon")
                visible: root.matches(title) || root.matches(qsTr("Notifications and System Tray"))
                opacity: root.supported("systemTray") ? 1 : .5
                help: root.helpFor("systemTray"); infoName: "interfacesystemTrayInfo"
                controlHeight: 32 * root.uiScale
                Toggle { id: systemTray; objectName: "interfacesystemTray"; valueKey: "systemTray"; accessibleTitle: qsTr("Show a system tray icon"); enabled: root.editable("systemTray") }
            }
            SettingRow {
                title: qsTr("Start minimized to the tray")
                visible: root.matches(title) || root.matches(qsTr("Notifications and System Tray"))
                opacity: root.supported("startMinimized") ? 1 : .5
                help: root.helpFor("startMinimized"); infoName: "interfacestartMinimizedInfo"
                controlHeight: 32 * root.uiScale
                Toggle { id: startMinimized; objectName: "interfacestartMinimized"; valueKey: "startMinimized"; accessibleTitle: qsTr("Start minimized to the tray"); enabled: root.editable("startMinimized") && root.values.systemTray === true }
            }
            SettingRow {
                title: qsTr("Show a popup when the track changes")
                visible: root.matches(title) || root.matches(qsTr("Notifications and System Tray"))
                opacity: root.supported("trackPopup") ? 1 : .5
                help: root.helpFor("trackPopup"); infoName: "interfacetrackPopupInfo"
                controlHeight: 36 * root.uiScale
                Choice { id: trackPopup; objectName: "interfacetrackPopup"; valueKey: "trackPopup"; accessibleTitle: qsTr("Show a popup when the track changes"); options: [{name: qsTr("Never"), value: 0}, {name: qsTr("When minimized"), value: 1}, {name: qsTr("Always"), value: 2}]; enabled: root.editable("trackPopup") && root.values.systemTray === true }
            }
            SettingRow {
                title: qsTr("Popup duration")
                visible: root.matches(title) || root.matches(qsTr("Notifications and System Tray"))
                opacity: root.supported("popupDuration") ? 1 : .5
                controlHeight: (Number.isFinite(root.values.popupDuration) && root.values.popupDuration >= 0 && root.values.popupDuration <= 10000 ? 46 : 66) * root.uiScale
                DurationRange { id: popupDuration; valueKey: "popupDuration"; accessibleTitle: qsTr("Popup duration"); control.enabled: root.editable("popupDuration") && root.values.systemTray === true }
            }
        }
        Column {
            width: parent.width
            visible: root.sectionMatches([qsTr("Playlist"), qsTr("Show the playlist as a tree")])
            SectionHeading { text: qsTr("Playlist") }
            SettingRow {
                title: qsTr("Show the playlist as a tree")
                visible: root.matches(title) || root.matches(qsTr("Playlist"))
                opacity: root.supported("playlistTree") ? 1 : .5
                help: root.helpFor("playlistTree"); infoName: "interfaceplaylistTreeInfo"
                controlHeight: 32 * root.uiScale
                Toggle { id: playlistTree; objectName: "interfaceplaylistTree"; valueKey: "playlistTree"; accessibleTitle: qsTr("Show the playlist as a tree"); enabled: root.editable("playlistTree") }
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
