// SPDX-License-Identifier: GPL-2.0-or-later
// Original visual design and SVG artwork: Juxtopposed, CC BY 4.0.
import QtQuick
import QtQuick.Controls.Basic as C
import QtQuick.Layouts

FocusScope {
    id: root
    objectName: "juxtopposedPlaybackSettings"
    required property var preferences
    // The shell supplies General here; this category uses only its own service.
    property var playbackPreferences: null
    readonly property var service: playbackPreferences
    property var appearance: null
    property real uiScale: 1
    property real viewportHeight: 0
    property url assetBase: "qrc:/juxtopposed/"
    property string searchText: ""
    property var focusNavigator: null
    signal focusRevealed(var item)
    signal advancedRequested()
    property bool advancedExpanded: false
    readonly property bool advancedVisible: advancedExpanded || searchText.trim().length > 0
        && sectionMatches([qsTr("Time-shift buffer folder"), qsTr("Time-shift buffer size")])
    readonly property var values: service ? service.values : ({})
    readonly property var capabilities: service ? service.capabilities : ({})
    readonly property var choices: service ? service.choices : ({})
    readonly property var applyModes: service ? service.applyModes : ({})
    readonly property var supportNotes: service ? service.supportNotes : ({})
    readonly property var settingKeys: ["continuePlayback", "startPlayback", "defaultPlaybackSpeed", "pauseOtherAudio", "preferredVideoQuality", "preferredMenuLanguage", "defaultAudioTrack", "defaultSubtitleTrack", "repeatMode", "shuffle", "stopAfterEachItem", "finishAction", "preciseSeeking", "showThumbnails", "defaultOpticalDrive", "aviRepair", "mkvPreload", "timeShiftFolder", "timeShiftSize"]
    readonly property var unsupportedKeys: settingKeys.filter(key => !supported(key))
    readonly property bool available: !!service
    readonly property bool busy: !!service && service.pickerActive
    readonly property bool stacked: width < 650 * uiScale
    readonly property color accent: appearance ? appearance.accent : "#D37918"
    readonly property color accentSelected: appearance ? appearance.accentSelected : "#371F06"
    readonly property color accentDim: appearance ? appearance.accentDim : "#AB6314"
    readonly property var focusTargets: [continuePlayback, startPlayback, speedRange.control, pauseOtherAudio,
        preferredVideoQuality, preferredMenuLanguage, defaultAudioTrack, defaultSubtitleTrack,
        repeatMode, shuffle, stopAfterEachItem, finishAction, preciseSeeking, showThumbnails,
        defaultOpticalDrive, aviRepair, mkvPreload, advancedButton, timeShiftFolder, timeShiftBrowse, timeShiftSize]
    implicitHeight: form.implicitHeight
    function tone(value) { return appearance ? appearance.color(value) : value }
    function matches(text) { return !searchText.trim().length || text.toLocaleLowerCase().includes(searchText.trim().toLocaleLowerCase()) }
    function sectionMatches(names) { return names.some(name => matches(name)) }
    function navigate(item, backwards) { if (focusNavigator) focusNavigator(item, backwards) }
    property Item _focusRevealTarget: null
    property Item _focusRevealBounds: null
    function reveal(item, boundsItem) {
        if (item.activeFocus) {
            _focusRevealTarget = item
            _focusRevealBounds = boundsItem || item
            focusRevealed(_focusRevealBounds)
        }
    }
    function revealActiveControl() {
        if (_focusRevealTarget && _focusRevealTarget.visible && _focusRevealTarget.activeFocus)
            focusRevealed(_focusRevealBounds || _focusRevealTarget)
    }
    onViewportHeightChanged: Qt.callLater(revealActiveControl)
    onWidthChanged: Qt.callLater(revealActiveControl)
    // An invalid QVariant is intentional for mixed optical-drive defaults:
    // the key remains present and editable so the user can replace that state.
    function supported(key) { return available && capabilities[key] === true && key in values }
    function editable(key) { return supported(key) && !busy }
    function write(key, value) { return editable(key) && service.setValue(key, value) }
    function nativeChoices(key) { return choices[key] || [] }
    function closeChoices() {
        for (const choice of [continuePlayback, preferredVideoQuality, preferredMenuLanguage, repeatMode, finishAction, aviRepair, timeShiftSize])
            if (choice.popup.visible) choice.popup.close()
    }
    function focusActivePopup() { if (busy) return true; closeChoices(); return false }
    onVisibleChanged: if (!visible) closeChoices()
    function helpFor(key) {
        const timing = {restart: qsTr("Restart VLC to apply this change."),
            nextInput: qsTr("Applies when the next media item opens.")}
        const hints = {repeatMode: qsTr("Repeat one item or the whole playlist."),
            shuffle: qsTr("Play the playlist in a random order."),
            aviRepair: qsTr("Choose how VLC handles a damaged AVI index."),
            mkvPreload: qsTr("Look for linked Matroska segments in the same folder. This does not add every video to the playlist.")}
        return (supportNotes[key] || hints[key] || "") + (timing[applyModes[key]] ? " " + timing[applyModes[key]] : "")
            + (supported(key) ? "" : " " + qsTr("This setting is unavailable in this build."))
    }
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
                        source: root.assetBase + "playback-settings-1f9fe.svg"
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
                source: root.assetBase + "playback-settings-764c9.svg"
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
        property string validationError: ""
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        width: 68 * root.uiScale; height: 32 * root.uiScale
        enabled: root.available && !root.busy
        Accessible.name: accessibleTitle
        Accessible.role: Accessible.CheckBox
        Accessible.checkable: true
        Accessible.checked: selected
        Accessible.description: root.helpFor(valueKey) + (validationError.length ? " " + validationError : "")
        onClicked: {
            const accepted = root.write(valueKey, !selected)
            validationError = accepted || !root.service ? "" : root.service.lastError
        }
        function revealValidationError() {
            if (activeFocus && visible) root.reveal(toggle, errorTip.visible ? errorBounds : toggle)
        }
        onValidationErrorChanged: Qt.callLater(revealValidationError)
        onActiveFocusChanged: Qt.callLater(revealValidationError)
        Connections {
            target: root.service
            function onLastErrorChanged() { if (!root.service.lastError.length) toggle.validationError = "" }
        }
        // Scroll the control and its local feedback as one rectangle. The
        // category banner can be outside this viewport after a failed save.
        Item {
            id: errorBounds
            y: errorTip.y
            width: toggle.width
            height: toggle.height - y
            onHeightChanged: Qt.callLater(toggle.revealValidationError)
        }
        C.ToolTip {
            id: errorTip
            objectName: toggle.objectName + "ValidationError"
            parent: toggle
            readonly property real pageX: toggle.mapToItem(root, 0, 0).x
            x: Math.max(-pageX, Math.min(0, root.width - pageX - width))
            y: -height - 4 * root.uiScale
            width: Math.min(320 * root.uiScale, root.width)
            visible: toggle.validationError.length > 0 && toggle.activeFocus && toggle.visible && root.visible
            text: toggle.validationError
            focus: false; closePolicy: C.Popup.NoAutoClose; timeout: 0
            padding: 8 * root.uiScale
            onVisibleChanged: Qt.callLater(toggle.revealValidationError)
            contentItem: SettingLabel {
                text: errorTip.text; color: "#D98779"
                Accessible.role: Accessible.StaticText; Accessible.name: text
            }
            background: Rectangle {
                color: root.tone("#191919"); radius: 4 * root.uiScale
                border.width: root.uiScale; border.color: "#D98779"
            }
        }
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

    component Input: C.TextField {
        id: field
        required property string valueKey
        required property string accessibleTitle
        property bool integral: false
        property string validationError: ""
        anchors.fill: parent
        enabled: root.editable(valueKey)
        text: root.values[valueKey] === undefined || root.values[valueKey] === null ? "" : String(root.values[valueKey])
        placeholderText: valueKey === "defaultOpticalDrive" && root.values[valueKey] == null
            ? qsTr("Different saved drive defaults") : ""
        selectByMouse: true; activeFocusOnTab: true
        font.family: inter.name; font.pixelSize: 14 * root.uiScale
        color: root.tone("#8F8F8F"); selectionColor: root.accentSelected
        leftPadding: 12 * root.uiScale; rightPadding: 18 * root.uiScale
        Accessible.name: accessibleTitle; Accessible.description: root.helpFor(valueKey) + (validationError.length ? " " + validationError : "")
        onTextEdited: validationError = ""
        function revealValidationError() {
            if (!validationError.length || !activeFocus) return
            root.reveal(field)
            Qt.callLater(() => {
                if (errorTip.visible && errorTip.background) root.focusRevealed(errorTip.background)
            })
        }
        onValidationErrorChanged: if (validationError.length) Qt.callLater(revealValidationError)
        Connections {
            target: form
            function onImplicitHeightChanged() {
                if (field.validationError.length) Qt.callLater(field.revealValidationError)
            }
        }
        function sync() { text = Qt.binding(() => root.values[valueKey] === undefined || root.values[valueKey] === null ? "" : String(root.values[valueKey])) }
        onEditingFinished: {
            if (text !== String(root.values[valueKey] ?? "")) {
                const value = integral && /^-?\d+$/.test(text.trim()) ? Number(text.trim()) : text
                const accepted = root.write(valueKey, value)
                validationError = accepted || !root.service ? "" : root.service.lastError
            }
            sync()
        }
        Connections {
            target: root.service
            function onValuesChanged() { field.sync() }
            function onLastErrorChanged() { if (!root.service.lastError.length) field.validationError = "" }
        }
        C.ToolTip {
            id: errorTip
            objectName: field.objectName + "ValidationError"
            parent: field
            x: 0; y: -height - 4 * root.uiScale
            width: Math.min(Math.max(field.width, 200 * root.uiScale), root.width)
            visible: field.validationError.length > 0 && field.activeFocus && field.visible
            text: field.validationError
            focus: false; closePolicy: C.Popup.NoAutoClose; timeout: 0
            padding: 8 * root.uiScale
            onVisibleChanged: if (visible) Qt.callLater(field.revealValidationError)
            contentItem: SettingLabel {
                text: errorTip.text; color: "#D98779"
                Accessible.role: Accessible.StaticText; Accessible.name: text
            }
            background: Rectangle {
                color: root.tone("#191919"); radius: 4 * root.uiScale
                border.width: root.uiScale; border.color: "#D98779"
            }
        }
        onActiveFocusChanged: root.reveal(field)
        Keys.onTabPressed: event => { if (root.focusNavigator) { root.navigate(field, false); event.accepted = true } }
        Keys.onBacktabPressed: event => { if (root.focusNavigator) { root.navigate(field, true); event.accepted = true } }
        background: Rectangle { color: root.tone("#090909"); radius: 6 * root.uiScale; border.width: 2 * root.uiScale; border.color: field.activeFocus ? root.accent : root.tone("#191919") }
    }
    component SpeedRange: Item {
        property alias control: speedSlider
        anchors.fill: parent
        readonly property real current: Number(root.values.defaultPlaybackSpeed)
        readonly property bool known: Number.isFinite(current) && current >= .25 && current <= 2 && Number.isInteger(current * 4)
        C.Slider {
            id: speedSlider
            objectName: "playbackdefaultPlaybackSpeed"
            width: parent.width; height: 24 * root.uiScale
            from: .25; to: 2; stepSize: .25
            padding: 0; leftPadding: 2 * root.uiScale; rightPadding: 2 * root.uiScale
            value: parent.known ? parent.current : 1
            enabled: root.editable("defaultPlaybackSpeed"); activeFocusOnTab: true
            Accessible.name: qsTr("Default playback speed")
            Accessible.description: root.helpFor("defaultPlaybackSpeed") + (parent.known ? "" : " " + qsTr("Current speed has no matching quarter-step preset."))
            function sync() { value = parent.known ? parent.current : 1 }
            onMoved: { root.write("defaultPlaybackSpeed", Math.round(value * 4) / 4); sync() }
            Connections { target: root.service; function onValuesChanged() { speedSlider.sync() } }
            Keys.onTabPressed: event => { if (root.focusNavigator) { root.navigate(speedSlider, false); event.accepted = true } }
            Keys.onBacktabPressed: event => { if (root.focusNavigator) { root.navigate(speedSlider, true); event.accepted = true } }
            onActiveFocusChanged: root.reveal(speedSlider)
            background: Rectangle {
                y: 4 * root.uiScale; width: speedSlider.width; height: 16 * root.uiScale
                color: root.tone("#090909"); radius: 6 * root.uiScale
                border.width: speedSlider.visualFocus ? root.uiScale : 0; border.color: root.accent
                Rectangle { x: 2 * root.uiScale; y: 2 * root.uiScale; width: speedSlider.parent.known ? Math.max(0, speedSlider.width - 4 * root.uiScale) * speedSlider.visualPosition : 0; height: 12 * root.uiScale; radius: 4 * root.uiScale; color: root.accentSelected }
                Repeater {
                    model: 8
                    Item {
                        required property int index
                        x: 2 * root.uiScale + (speedSlider.width - 4 * root.uiScale) * index / 7
                        y: 5 * root.uiScale; width: 0; height: 6 * root.uiScale
                        clip: index === 0 || index === 7
                        Image {
                            objectName: "playbackOriginalSpeedTick" + index
                            anchors.centerIn: parent
                            width: index === 0 || index === 7 ? 32 : 2
                            height: index === 0 || index === 7 ? 32 : 8
                            scale: root.uiScale
                            source: root.assetBase + (index === 0 || index === 7 ? "playback-settings-8ba8e.svg" : "playback-settings-1ab84.svg")
                            Accessible.ignored: true
                        }
                    }
                }
            }
            handle: Rectangle {
                visible: speedSlider.parent.known
                x: 2 * root.uiScale + (speedSlider.width - 4 * root.uiScale) * speedSlider.visualPosition - width / 2
                y: 0; width: 8 * root.uiScale; height: 24 * root.uiScale
                radius: 4 * root.uiScale; color: root.accentDim
            }
        }
        Item {
            y: 28 * root.uiScale; width: parent.width
            Repeater {
                model: ["0.25", "0.5", "0.75", "1", "1.25", "1.5", "1.75", "2"]
                SettingLabel { required property string modelData; required property int index
                    width: 26 * root.uiScale
                    x: (parent.width - width) * index / 7
                    text: modelData; font.pixelSize: 12 * root.uiScale; color: root.tone("#6B6B6B")
                    horizontalAlignment: index === 0 ? Text.AlignLeft : index === 7 ? Text.AlignRight : Text.AlignHCenter
                }
            }
        }
        SettingLabel { y: 46 * root.uiScale; visible: !parent.known; text: qsTr("Current speed: %1").arg(root.values.defaultPlaybackSpeed); font.pixelSize: 12 * root.uiScale }
    }
    Column {
        id: form; width: root.width; spacing: 20 * root.uiScale
        onImplicitHeightChanged: Qt.callLater(root.revealActiveControl)
        SettingLabel { objectName: "playbackSettingsError"; width: parent.width
            text: root.service ? root.service.lastError : ""; visible: text.length > 0; color: "#D98779"; Accessible.role: Accessible.StaticText; Accessible.name: text }
        SettingLabel { objectName: "playbackSettingsRestart"; width: parent.width
            text: qsTr("Restart VLC to apply these changes."); visible: root.service && root.service.restartRequired
            color: root.tone("#C6C6C6"); Accessible.role: Accessible.StaticText; Accessible.name: text }
        Column {
            width: parent.width
            visible: root.sectionMatches([qsTr("Startup"), qsTr("Continue where you left off"), qsTr("Start playing as soon as a file opens"), qsTr("Default playback speed"), qsTr("Pause when another app uses audio")])
            SectionHeading { text: qsTr("Startup") }
            SettingRow {
                objectName: "playbackcontinuePlaybackRow"
                title: qsTr("Continue where you left off")
                visible: root.matches(title) || root.matches(qsTr("Startup"))
                opacity: root.supported("continuePlayback") ? 1 : .5
                help: root.helpFor("continuePlayback"); infoName: "playbackcontinuePlaybackInfo"
                controlHeight: 36 * root.uiScale
                Choice { id: continuePlayback; objectName: "playbackcontinuePlayback"; valueKey: "continuePlayback"; accessibleTitle: qsTr("Continue where you left off"); options: root.nativeChoices("continuePlayback"); enabled: root.editable("continuePlayback") }
            }
            SettingRow {
                objectName: "playbackstartPlaybackRow"
                title: qsTr("Start playing as soon as a file opens")
                visible: root.matches(title) || root.matches(qsTr("Startup"))
                opacity: root.supported("startPlayback") ? 1 : .5
                help: root.helpFor("startPlayback"); infoName: "playbackstartPlaybackInfo"
                controlHeight: 32 * root.uiScale
                Toggle { id: startPlayback; objectName: "playbackstartPlayback"; valueKey: "startPlayback"; accessibleTitle: qsTr("Start playing as soon as a file opens"); enabled: root.editable("startPlayback") }
            }
            SettingRow {
                objectName: "playbackdefaultPlaybackSpeedRow"
                title: qsTr("Default playback speed")
                visible: root.matches(title) || root.matches(qsTr("Startup"))
                opacity: root.supported("defaultPlaybackSpeed") ? 1 : .5
                help: ""; infoName: "playbackdefaultPlaybackSpeedInfo"
                controlHeight: 48 * root.uiScale
                SpeedRange { id: speedRange }
            }
            SettingRow {
                objectName: "playbackpauseOtherAudioRow"
                title: qsTr("Pause when another app uses audio")
                visible: root.matches(title) || root.matches(qsTr("Startup"))
                opacity: root.supported("pauseOtherAudio") ? 1 : .5
                help: root.helpFor("pauseOtherAudio"); infoName: "playbackpauseOtherAudioInfo"
                controlHeight: 32 * root.uiScale
                Toggle { id: pauseOtherAudio; objectName: "playbackpauseOtherAudio"; valueKey: "pauseOtherAudio"; accessibleTitle: qsTr("Pause when another app uses audio"); enabled: root.editable("pauseOtherAudio") }
            }
        }
        Column {
            width: parent.width
            visible: root.sectionMatches([qsTr("Tracks"), qsTr("Preferred video quality"), qsTr("Preferred menu language"), qsTr("Default audio track"), qsTr("Default subtitle track")])
            SectionHeading { text: qsTr("Tracks") }
            SettingRow {
                objectName: "playbackpreferredVideoQualityRow"
                title: qsTr("Preferred video quality")
                visible: root.matches(title) || root.matches(qsTr("Tracks"))
                opacity: root.supported("preferredVideoQuality") ? 1 : .5
                help: root.helpFor("preferredVideoQuality"); infoName: "playbackpreferredVideoQualityInfo"
                controlHeight: 36 * root.uiScale
                Choice { id: preferredVideoQuality; objectName: "playbackpreferredVideoQuality"; valueKey: "preferredVideoQuality"; accessibleTitle: qsTr("Preferred video quality"); options: root.nativeChoices("preferredVideoQuality"); enabled: root.editable("preferredVideoQuality") }
            }
            SettingRow {
                objectName: "playbackpreferredMenuLanguageRow"
                title: qsTr("Preferred menu language")
                visible: root.matches(title) || root.matches(qsTr("Tracks"))
                opacity: root.supported("preferredMenuLanguage") ? 1 : .5
                help: root.helpFor("preferredMenuLanguage"); infoName: "playbackpreferredMenuLanguageInfo"
                controlHeight: 36 * root.uiScale
                Choice { id: preferredMenuLanguage; objectName: "playbackpreferredMenuLanguage"; valueKey: "preferredMenuLanguage"; accessibleTitle: qsTr("Preferred menu language"); options: root.nativeChoices("preferredMenuLanguage"); enabled: root.editable("preferredMenuLanguage") }
            }
            SettingRow {
                objectName: "playbackdefaultAudioTrackRow"
                title: qsTr("Default audio track")
                visible: root.matches(title) || root.matches(qsTr("Tracks"))
                opacity: root.supported("defaultAudioTrack") ? 1 : .5
                help: root.helpFor("defaultAudioTrack"); infoName: "playbackdefaultAudioTrackInfo"
                controlHeight: 36 * root.uiScale
                Input { id: defaultAudioTrack; objectName: "playbackdefaultAudioTrack"; valueKey: "defaultAudioTrack"; accessibleTitle: qsTr("Default audio track"); integral: true }
            }
            SettingRow {
                objectName: "playbackdefaultSubtitleTrackRow"
                title: qsTr("Default subtitle track")
                visible: root.matches(title) || root.matches(qsTr("Tracks"))
                opacity: root.supported("defaultSubtitleTrack") ? 1 : .5
                help: root.helpFor("defaultSubtitleTrack"); infoName: "playbackdefaultSubtitleTrackInfo"
                controlHeight: 36 * root.uiScale
                Input { id: defaultSubtitleTrack; objectName: "playbackdefaultSubtitleTrack"; valueKey: "defaultSubtitleTrack"; accessibleTitle: qsTr("Default subtitle track"); integral: true }
            }
        }
        Column {
            width: parent.width
            visible: root.sectionMatches([qsTr("Order"), qsTr("Repeat"), qsTr("Shuffle"), qsTr("Stop after each item"), qsTr("When playback finishes")])
            SectionHeading { text: qsTr("Order") }
            SettingRow {
                objectName: "playbackrepeatModeRow"
                title: qsTr("Repeat")
                visible: root.matches(title) || root.matches(qsTr("Order"))
                opacity: root.supported("repeatMode") ? 1 : .5
                help: root.helpFor("repeatMode"); infoName: "playbackrepeatModeInfo"
                controlHeight: 36 * root.uiScale
                Choice { id: repeatMode; objectName: "playbackrepeatMode"; valueKey: "repeatMode"; accessibleTitle: qsTr("Repeat"); options: root.nativeChoices("repeatMode"); enabled: root.editable("repeatMode") }
            }
            SettingRow {
                objectName: "playbackshuffleRow"
                title: qsTr("Shuffle")
                visible: root.matches(title) || root.matches(qsTr("Order"))
                opacity: root.supported("shuffle") ? 1 : .5
                help: root.helpFor("shuffle"); infoName: "playbackshuffleInfo"
                controlHeight: 32 * root.uiScale
                Toggle { id: shuffle; objectName: "playbackshuffle"; valueKey: "shuffle"; accessibleTitle: qsTr("Shuffle"); enabled: root.editable("shuffle") }
            }
            SettingRow {
                objectName: "playbackstopAfterEachItemRow"
                title: qsTr("Stop after each item")
                visible: root.matches(title) || root.matches(qsTr("Order"))
                opacity: root.supported("stopAfterEachItem") ? 1 : .5
                help: root.helpFor("stopAfterEachItem"); infoName: "playbackstopAfterEachItemInfo"
                controlHeight: 32 * root.uiScale
                Toggle { id: stopAfterEachItem; objectName: "playbackstopAfterEachItem"; valueKey: "stopAfterEachItem"; accessibleTitle: qsTr("Stop after each item"); enabled: root.editable("stopAfterEachItem") }
            }
            SettingRow {
                objectName: "playbackfinishActionRow"
                title: qsTr("When playback finishes")
                visible: root.matches(title) || root.matches(qsTr("Order"))
                opacity: root.supported("finishAction") ? 1 : .5
                help: root.helpFor("finishAction"); infoName: "playbackfinishActionInfo"
                controlHeight: 36 * root.uiScale
                Choice { id: finishAction; objectName: "playbackfinishAction"; valueKey: "finishAction"; accessibleTitle: qsTr("When playback finishes"); options: root.nativeChoices("finishAction"); enabled: root.editable("finishAction") }
            }
        }
        Column {
            width: parent.width
            visible: root.sectionMatches([qsTr("Seeking"), qsTr("Seek precisely"), qsTr("Show thumbnail while scrubbing")])
            SectionHeading { text: qsTr("Seeking") }
            SettingRow {
                objectName: "playbackpreciseSeekingRow"
                title: qsTr("Seek precisely")
                visible: root.matches(title) || root.matches(qsTr("Seeking"))
                opacity: root.supported("preciseSeeking") ? 1 : .5
                help: root.helpFor("preciseSeeking"); infoName: "playbackpreciseSeekingInfo"
                controlHeight: 32 * root.uiScale
                Toggle { id: preciseSeeking; objectName: "playbackpreciseSeeking"; valueKey: "preciseSeeking"; accessibleTitle: qsTr("Seek precisely"); enabled: root.editable("preciseSeeking") }
            }
            SettingRow {
                objectName: "playbackshowThumbnailsRow"
                title: qsTr("Show thumbnail while scrubbing")
                visible: root.matches(title) || root.matches(qsTr("Seeking"))
                opacity: root.supported("showThumbnails") ? 1 : .5
                help: root.helpFor("showThumbnails"); infoName: "playbackshowThumbnailsInfo"
                controlHeight: 32 * root.uiScale
                Toggle { id: showThumbnails; objectName: "playbackshowThumbnails"; valueKey: "showThumbnails"; accessibleTitle: qsTr("Show thumbnail while scrubbing"); enabled: root.editable("showThumbnails") }
            }
        }
        Column {
            width: parent.width
            visible: root.sectionMatches([qsTr("Files & Discs"), qsTr("Default optical drive"), qsTr("Repair damaged AVI files"), qsTr("Load matching .mkv files from the same folder")])
            SectionHeading { text: qsTr("Files & Discs") }
            SettingRow {
                objectName: "playbackdefaultOpticalDriveRow"
                title: qsTr("Default optical drive")
                visible: root.matches(title) || root.matches(qsTr("Files & Discs"))
                opacity: root.supported("defaultOpticalDrive") ? 1 : .5
                help: root.helpFor("defaultOpticalDrive"); infoName: "playbackdefaultOpticalDriveInfo"
                controlHeight: 36 * root.uiScale
                Input { id: defaultOpticalDrive; objectName: "playbackdefaultOpticalDrive"; valueKey: "defaultOpticalDrive"; accessibleTitle: qsTr("Default optical drive"); integral: false }
            }
            SettingRow {
                objectName: "playbackaviRepairRow"
                title: qsTr("Repair damaged AVI files")
                visible: root.matches(title) || root.matches(qsTr("Files & Discs"))
                opacity: root.supported("aviRepair") ? 1 : .5
                help: root.helpFor("aviRepair"); infoName: "playbackaviRepairInfo"
                controlHeight: 36 * root.uiScale
                Choice { id: aviRepair; objectName: "playbackaviRepair"; valueKey: "aviRepair"; accessibleTitle: qsTr("Repair damaged AVI files"); options: root.nativeChoices("aviRepair"); enabled: root.editable("aviRepair") }
            }
            SettingRow {
                objectName: "playbackmkvPreloadRow"
                title: qsTr("Load matching .mkv files from the same folder")
                visible: root.matches(title) || root.matches(qsTr("Files & Discs"))
                opacity: root.supported("mkvPreload") ? 1 : .5
                help: root.helpFor("mkvPreload"); infoName: "playbackmkvPreloadInfo"
                controlHeight: 32 * root.uiScale
                Toggle { id: mkvPreload; objectName: "playbackmkvPreload"; valueKey: "mkvPreload"; accessibleTitle: qsTr("Load matching .mkv files from the same folder"); enabled: root.editable("mkvPreload") }
            }
        }
        SettingButton {
            id: advancedButton; objectName: "playbackAdvanced"
            width: parent.width; height: 38 * root.uiScale
            text: qsTr("Advanced Options")
            Accessible.description: root.advancedVisible ? qsTr("Time-shift options expanded") : qsTr("Show time-shift options")
            onClicked: root.advancedExpanded = !root.advancedExpanded
            background: Rectangle { color: root.tone("#191919"); opacity: .95; radius: 2 * root.uiScale; border.width: advancedButton.visualFocus ? root.uiScale : 0; border.color: root.accent }
            contentItem: Item {
              Row {
                anchors.centerIn: parent; spacing: 10 * root.uiScale
                SettingLabel { text: advancedButton.text; font: advancedButton.font; height: advancedButton.height; verticalAlignment: Text.AlignVCenter }
                Item { width: 24 * root.uiScale; height: 24 * root.uiScale; y: 7 * root.uiScale
                    Image { objectName: "playbackAdvancedOriginalChevron"; anchors.centerIn: parent; width: 24; height: 24; scale: root.uiScale; mirror: true; source: root.assetBase + (root.advancedVisible ? "playback-settings-764c9.svg" : "playback-settings-97863.svg"); Accessible.ignored: true }
                }
              }
            }
        }
        Column {
            width: parent.width
            visible: root.advancedVisible
            SettingRow {
                objectName: "playbacktimeShiftFolderRow"
                title: qsTr("Time-shift buffer folder")
                visible: root.matches(title) || root.matches(qsTr("Advanced Options"))
                opacity: root.supported("timeShiftFolder") ? 1 : .5
                help: root.helpFor("timeShiftFolder"); infoName: "playbacktimeShiftFolderInfo"
                controlHeight: 32 * root.uiScale
                RowLayout {
                    anchors.fill: parent; spacing: 10 * root.uiScale
                    Input { id: timeShiftFolder; objectName: "playbacktimeShiftFolder"; valueKey: "timeShiftFolder"; accessibleTitle: qsTr("Time-shift buffer folder")
                        anchors.fill: undefined; Layout.fillWidth: true; Layout.minimumWidth: 0; Layout.fillHeight: true }
                    SettingButton { id: timeShiftBrowse; objectName: "playbacktimeShiftBrowse"
                        Layout.preferredWidth: 100 * root.uiScale; Layout.preferredHeight: 32 * root.uiScale
                        text: qsTr("Browse"); font.weight: Font.DemiBold; enabled: root.editable("timeShiftFolder")
                        Accessible.name: qsTr("Choose time-shift folder"); onClicked: root.service.chooseTimeShiftFolder() }
                }
            }
            SettingRow {
                objectName: "playbacktimeShiftSizeRow"
                title: qsTr("Time-shift buffer size")
                visible: root.matches(title) || root.matches(qsTr("Advanced Options"))
                opacity: root.supported("timeShiftSize") ? 1 : .5
                help: root.helpFor("timeShiftSize"); infoName: "playbacktimeShiftSizeInfo"
                controlHeight: 36 * root.uiScale
                Choice { id: timeShiftSize; objectName: "playbacktimeShiftSize"; valueKey: "timeShiftSize"; accessibleTitle: qsTr("Time-shift buffer size"); options: [{name: qsTr("Default (50 MiB per file)"), value: -1}, {name: "1 MiB", value: 1048576}, {name: "16 MiB", value: 16777216}, {name: "50 MiB", value: 52428800}, {name: "100 MiB", value: 104857600}, {name: "512 MiB", value: 536870912}]; enabled: root.editable("timeShiftSize") }
            }
        }
    }
}
