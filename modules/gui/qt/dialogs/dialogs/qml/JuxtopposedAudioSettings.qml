// SPDX-License-Identifier: GPL-2.0-or-later
// Original visual design and SVG artwork: Juxtopposed, CC BY 4.0.
import QtQuick
import QtQuick.Controls.Basic as C
import QtQuick.Layouts
FocusScope {
    id: root
    objectName: "juxtopposedAudioSettings"
    required property var preferences
    property var audioPreferences: null
    readonly property var service: audioPreferences
    property var appearance: null
    property real uiScale: 1
    property url assetBase: "qrc:/juxtopposed/"
    property string searchText: ""
    property var focusNavigator: null
    property real viewportHeight: 0
    signal focusRevealed(var item)
    signal advancedRequested()
    readonly property bool busy: false // The frozen Audio API has no picker/job.
    readonly property bool available: !!service
    readonly property var values: service ? service.values : ({})
    readonly property var capabilities: service ? service.capabilities : ({})
    readonly property var options: service ? service.options : ({})
    readonly property var devices: service ? service.devices : []
    readonly property var ranges: service ? service.ranges : ({})
    readonly property var frequencies: service ? service.equalizerFrequencies : []
    readonly property var settingKeys: ["audioEnabled", "outputDevice", "speakerSetup", "maximumVolume", "rememberVolume", "startupVolume", "volumeStep", "normalizeLoudness", "normalizationTarget", "nightMode", "replayGainMode", "replayGainPreamp", "equalizerEnabled", "equalizerPreset", "equalizerTwoPass", "equalizerPreamp", "equalizerBands", "spatialAudio", "spatialAudioMode", "stereoWidening", "compressor", "pitchShift", "chorusFlanger", "karaoke", "preferredLanguage", "audioDelayMs", "visualizer", "passthrough", "detectDolby", "resamplingQuality", "mediaRole", "visualizationSize"]
    readonly property bool stacked: width < 650 * uiScale
    readonly property color accent: appearance ? appearance.accent : "#D37918"
    readonly property color accentSelected: appearance ? appearance.accentSelected : "#371F06"
    readonly property color accentDim: appearance ? appearance.accentDim : "#AB6314"
    property bool equalizerExpanded: false
    readonly property bool equalizerVisible: equalizerExpanded || searchText.trim().length > 0
        && sectionMatches([qsTr("Preamp"),qsTr("Equalizer bands"),qsTr("Equalizer preamp")])
    property bool advancedExpanded: false
    readonly property bool advancedVisible: advancedExpanded || searchText.trim().length > 0 && sectionMatches([qsTr("Pass compressed audio to the receiver"),qsTr("Detect Dolby Surround"),qsTr("Resampling quality"),qsTr("Media role"),qsTr("Visualization size")])
    readonly property var focusTargets: [audioEnabled, outputDevice, speakerSetup, maximumVolume.control,
        rememberVolume, fixedVolume, startupVolume, volumeStep, normalizeLoudness, normalizationTarget,
        nightMode, replayGainMode, replayGainPreamp, equalizerEnabled, equalizerPreset, eqOpen]
        .concat(equalizerTargets()).concat([equalizerTwoPass, spatialAudio, spatialAudioMode, stereoWidening,
        compressor, pitchShift, chorusFlanger, karaoke, preferredLanguage, audioDelayMs, visualizer,
        advancedButton, passthrough, detectDolby, resamplingQuality, mediaRole, visualizationSize])
    implicitHeight: form.implicitHeight
    function tone(value) { return appearance ? appearance.color(value) : value }
    function matches(text) { return !searchText.trim().length || text.toLocaleLowerCase().includes(searchText.trim().toLocaleLowerCase()) }
    function sectionMatches(names) { return names.some(name => matches(name)) }
    function supported(key) { return available && capabilities[key] === true && key in values }
    function editable(key) { return supported(key) && !busy }
    function nativeChoices(key) { return options[key] || [] }
    function write(key, value) { return editable(key) && service.setValue(key, value) }
    property list<var> bandPayload: []
    function writeBands(bands) { bandPayload = bands; return editable("equalizerBands") && service.setValue("equalizerBands",bandPayload) }
    function navigate(item, backwards) { if (focusNavigator) focusNavigator(item, backwards) }
    function reveal(item) {
        if (!item.activeFocus) return
        if (equalizerVisible) {
            const point = item.mapToItem(eqScroll.contentItem,0,0)
            if (point.y >= 0 && point.y < eqScroll.contentHeight) {
                if (point.x < eqScroll.contentX) eqScroll.contentX = Math.max(0,point.x)
                else if (point.x + item.width > eqScroll.contentX + eqScroll.width)
                    eqScroll.contentX = Math.min(eqScroll.contentWidth-eqScroll.width,point.x+item.width-eqScroll.width)
            }
        }
        focusRevealed(item)
    }
    function equalizerTargets() {
        const items = []
        for (let i=0; i<eqRepeater.count; ++i) {
            const channel=eqRepeater.itemAt(i)
            if (channel) items.push(channel.slider,channel.input)
        }
        return items
    }
    function closeChoices() {
        for (const choice of [outputDevice,speakerSetup,replayGainMode,equalizerPreset,spatialAudioMode,preferredLanguage,visualizer,detectDolby,resamplingQuality,mediaRole,visualizationSize])
            if (choice.popup.visible) choice.popup.close()
    }
    function focusActivePopup() { closeChoices(); return false }
    onVisibleChanged: if (!visible) closeChoices()
    function helpFor(key) {
        const hints = {audioEnabled: qsTr("Include audio when playing media."),outputDevice: qsTr("Choose an available output device. A disconnected saved device stays unselected until you choose another."),speakerSetup: qsTr("Choose the preferred mix. Available choices and the source format determine which layouts can be used."),maximumVolume: qsTr("The slider spans 100–300%. Saved values from 60–99% are preserved and shown below the scale."),rememberVolume: qsTr("Remember the last volume, or select a fixed startup volume. Editing the fixed number alone does not change this policy."),startupVolume: qsTr("Fixed MMDevice startup volume, from 0% to 125%. You can prepare the number while Remember is selected."),volumeStep: qsTr("Volume change in percentage points, from 100/256 to 100. It is independent of maximum volume."),normalizeLoudness: qsTr("Enable the native loudness normalization effect."),normalizationTarget: qsTr("Positive target level. You can prepare the value before enabling normalization."),nightMode: qsTr("A separate Night mode is not available. The Compressor setting remains independent."),replayGainMode: qsTr("Use saved track or album replay gain, or turn it off."),replayGainPreamp: qsTr("Replay gain adjustment in dB, from -20 to 20."),equalizerEnabled: qsTr("Enable these saved equalizer defaults. Current playback equalizer controls are separate."),equalizerPreset: qsTr("Choose an actual native preset. Custom gains stay unselected; choosing a preset replaces all ten bands and preamp."),equalizerPreamp: qsTr("Preamp gain in dB, from -20 to 20. Editing a gain changes the saved preset to Custom."),equalizerBands: qsTr("Ten band gains in dB, from -20 to 20. Frequencies come from VLC. Custom edits clear the named preset."),equalizerTwoPass: qsTr("Apply a second equalizer pass. This can be prepared while the equalizer is off."),spatialAudio: qsTr("A separate Spatial audio switch is unavailable. Speaker setup may offer Binaural when supported."),spatialAudioMode: qsTr("A separate Spatial audio mode is unavailable."),stereoWidening: qsTr("Enable stereo widening for compatible sources."),compressor: qsTr("Enable the compressor effect."),pitchShift: qsTr("Pitch shift in semitones, from -12 to 12. Zero turns this effect off."),chorusFlanger: qsTr("Enable the native chorus/flanger effect."),karaoke: qsTr("Reduce the center vocal in compatible stereo sources. Results depend on the recording."),preferredLanguage: qsTr("Preferred audio language. Automatic uses an empty preference. Existing ordered language lists are retained until you choose a preset."),audioDelayMs: qsTr("Audio delay in milliseconds, using a signed whole number."),visualizer: qsTr("Choose an available visualization."),passthrough: qsTr("Pass supported compressed formats to a compatible receiver. An unknown saved state remains unselected: Left chooses Off, Right chooses On."),detectDolby: qsTr("Choose the Dolby Surround metadata policy. This is separate from compressed-audio passthrough."),resamplingQuality: qsTr("Choose a real SoX resampling policy. An unmanaged resampler remains under its current owner."),mediaRole: qsTr("Available only for an output that consumes this role. Windows MMDevice does not use it."),visualizationSize: qsTr("Adaptive Match window sizing is unavailable. Existing fixed visualization dimensions are preserved.")}
        return (hints[key] || "") + (supported(key)
            ? " " + (key === "volumeStep" ? qsTr("Applies to volume steps immediately.")
                : key === "audioDelayMs" ? qsTr("Applies when the next media item opens.")
                : qsTr("Restart VLC to apply changed audio defaults."))
            : " " + qsTr("This setting is unavailable in this build."))
    }
    function frequencyLabel(value) { return value >= 1000 ? (value/1000) + " KHz" : value + " Hz" }
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
                        source: root.assetBase + "audio-settings-1f9fe.svg"
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
                source: root.assetBase + "audio-settings-764c9.svg"
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
        readonly property bool known: typeof root.values[valueKey] === "boolean"
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
        Keys.onLeftPressed: root.write(valueKey, false)
        Keys.onRightPressed: root.write(valueKey, true)
        background: Rectangle {
            color: root.tone("#090909"); radius: 6 * root.uiScale
            border.width: toggle.visualFocus ? root.uiScale : 0; border.color: root.accent
        }
        MouseArea {
            anchors.fill: parent; enabled: !toggle.known && toggle.enabled
            onClicked: mouse => root.write(toggle.valueKey, mouse.x >= width / 2)
        }
        contentItem: Item {
            Rectangle { x: (toggle.selected ? 3 : 32) * root.uiScale; y: 3 * root.uiScale; width: 33 * root.uiScale; height: 26 * root.uiScale; radius: 3 * root.uiScale; color: toggle.selected ? root.accentSelected : root.tone("#191919") }
            Rectangle { x: (toggle.selected ? 39 : 3) * root.uiScale; y: 3 * root.uiScale; width: 26 * root.uiScale; height: 26 * root.uiScale; radius: 3 * root.uiScale; color: toggle.selected ? root.accentDim : root.tone("#404040") }
            SettingLabel { x: (toggle.selected ? 10 : 36) * root.uiScale; anchors.verticalCenter: parent.verticalCenter; text: !toggle.known ? "?" : toggle.selected ? qsTr("ON") : qsTr("OFF"); color: toggle.selected ? root.accent : root.tone("#6B6B6B"); font.pixelSize: 12 * root.uiScale }
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
        property string kind: "number"
        property string unit: ""
        property int digits: unit === " dB" ? 1 : -1
        property bool userEdited: false
        property string validationError: ""
        property int bandIndex: -2
        readonly property var current: bandIndex >= 0 ? (root.values.equalizerBands || [])[bandIndex] : root.values[valueKey]
        readonly property var bounds: root.ranges[valueKey]
        anchors.fill: parent
        enabled: root.editable(valueKey) && (bandIndex < 0 || (root.values.equalizerBands || []).length === 10)
        text: displayCurrent()
        selectByMouse: true; activeFocusOnTab: true
        font.family: inter.name; font.pixelSize: 14 * root.uiScale
        color: root.tone("#8F8F8F"); selectionColor: root.accentSelected
        leftPadding: 12 * root.uiScale; rightPadding: 18 * root.uiScale
        Accessible.name: accessibleTitle; Accessible.description: root.helpFor(valueKey) + (validationError.length ? " " + validationError : "")
            + (bounds ? " " + qsTr("Supported range: %1 to %2.").arg(bounds.minimum).arg(bounds.maximum) : "")
        function displayCurrent() {
            if (current == null) return ""
            if (typeof current === "number" && digits >= 0 && Math.fround(Number(current.toFixed(digits))) === Math.fround(current)) return current.toFixed(digits) + unit
            // Bound display noise only; current and commit parsing stay exact.
            if (typeof current === "number" && Number.isFinite(current) && !Number.isInteger(current)) return String(Number(current.toPrecision(6))) + unit
            return String(current) + unit
        }
        onTextEdited: { userEdited = true; validationError = "" }
        function revealValidationError() {
            if (!validationError.length || !activeFocus) return
            root.reveal(field)
            Qt.callLater(() => {
                const position=field.mapToItem(root,0,0)
                errorTip.x=Math.max(-position.x,Math.min(0,root.width-position.x-errorTip.width))
                if (errorTip.visible && errorTip.background) root.focusRevealed(errorTip.background)
            })
        }
        onValidationErrorChanged: if (validationError.length) Qt.callLater(revealValidationError)
        Connections {
            target: form
            function onImplicitHeightChanged() {
                Qt.callLater(() => {
                    if (!field.activeFocus) return
                    if (field.validationError.length) field.revealValidationError()
                    else root.reveal(field)
                })
            }
        }
        function sync() { text = Qt.binding(displayCurrent); userEdited = false }
        function commit() {
            if (!userEdited) { sync(); return }
            let raw=text.trim()
            if (unit.length && raw.endsWith(unit)) raw=raw.slice(0,-unit.length).trim()
            const value=kind === "text" ? raw : raw.length && Number.isFinite(Number(raw)) ? Number(raw) : raw
            if (value !== current) {
                if (bandIndex >= 0) {
                    const bands=(root.values.equalizerBands || []).slice()
                    if (bands.length === 10) { bands[bandIndex]=value; const accepted=root.writeBands(bands); validationError=accepted||!root.service?"":root.service.lastError }
                } else {const accepted=root.write(valueKey,value);validationError=accepted||!root.service?"":root.service.lastError}
            }
            sync()
        }
        onEditingFinished: commit()
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
        Keys.onTabPressed: event => { if (root.focusNavigator) { root.navigate(field,false);event.accepted=true } }
        Keys.onBacktabPressed: event => { if (root.focusNavigator) { root.navigate(field,true);event.accepted=true } }
        background: Rectangle { color: root.tone("#090909");radius:6*root.uiScale;border.width:2*root.uiScale;border.color:field.activeFocus?root.accent:root.tone("#191919") }
    }
    component VolumeRange: Item {
        id: range
        property alias control: slider
        anchors.fill: parent
        readonly property real current: Number(root.values.maximumVolume)
        readonly property bool known: Number.isFinite(current) && current>=100 && current<=300
        C.Slider {
            id: slider; objectName: "audiomaximumVolume"
            y:4*root.uiScale;width: parent.width; height:24*root.uiScale
            from:100;to:300;stepSize:1;padding:0;leftPadding:2*root.uiScale;rightPadding:2*root.uiScale
            value:range.known?range.current:100; enabled:root.editable("maximumVolume");activeFocusOnTab:true
            Accessible.name:qsTr("Maximum volume");Accessible.description:root.helpFor("maximumVolume")
            function sync(){value=range.known?range.current:100}
            onMoved:{root.write("maximumVolume",Math.round(value));sync()}
            Connections{target:root.service;function onValuesChanged(){slider.sync()}}
            onActiveFocusChanged:root.reveal(slider)
            Keys.onTabPressed:event=>{if(root.focusNavigator){root.navigate(slider,false);event.accepted=true}}
            Keys.onBacktabPressed:event=>{if(root.focusNavigator){root.navigate(slider,true);event.accepted=true}}
            background:Rectangle{
                y:4*root.uiScale;width:slider.width;height:16*root.uiScale;color:root.tone("#090909");radius:6*root.uiScale
                Rectangle{x:2*root.uiScale;y:2*root.uiScale;width:range.known?(slider.width-4*root.uiScale)*slider.position:0;height:12*root.uiScale;radius:4*root.uiScale;color:root.accentSelected}
                Repeater{model:9;Item{required property int index;x:2*root.uiScale+(slider.width-4*root.uiScale)*index/8;y:5*root.uiScale;width:0;height:6*root.uiScale;clip:index===0||index===8
                    Image{objectName:"audioMaximumOriginalTick"+index;anchors.centerIn:parent;width:index===0||index===8?32:2;height:index===0||index===8?32:8;scale:root.uiScale;source:root.assetBase+(index===0||index===8?"audio-settings-8ba8e.svg":"audio-settings-1ab84.svg");Accessible.ignored:true}
                }}
            }
            handle:Rectangle{objectName:"audioMaximumThumb";visible:range.known;x:2*root.uiScale+(slider.width-4*root.uiScale)*slider.position-width/2;y:0;width:8*root.uiScale;height:24*root.uiScale;radius:4*root.uiScale;color:root.accentDim}
        }
        Row{y:38*root.uiScale;width:parent.width
            SettingLabel{width:parent.width/3;text:"100%";font.pixelSize:12*root.uiScale;color:root.tone("#6B6B6B")}
            SettingLabel{width:parent.width/3;text:"200%";horizontalAlignment:Text.AlignHCenter;font.pixelSize:12*root.uiScale;color:root.tone("#6B6B6B")}
            SettingLabel{width:parent.width/3;text:"300%";horizontalAlignment:Text.AlignRight;font.pixelSize:12*root.uiScale;color:root.tone("#6B6B6B")}
        }
        SettingLabel{y:61*root.uiScale;visible:!range.known;text:qsTr("Current value: %1%").arg(root.values.maximumVolume);font.pixelSize:12*root.uiScale}
    }
    Column {
        id:form;width:root.width;spacing:20*root.uiScale
        SettingLabel{objectName:"audioSettingsError";width:parent.width;text:root.service?root.service.lastError:"";visible:text.length>0;color:"#D98779";Accessible.role:Accessible.StaticText;Accessible.name:text}
        SettingLabel{objectName:"audioSettingsRestart";width:parent.width;text:qsTr("Restart VLC to apply changed audio defaults.");visible:root.service&&root.service.restartRequired;color:root.tone("#C6C6C6");Accessible.role:Accessible.StaticText;Accessible.name:text}
        Column {
            width:parent.width
            visible:root.sectionMatches([qsTr("Output"), qsTr("Enable audio"), qsTr("Output device"), qsTr("Speaker setup")])
            SectionHeading{text:qsTr("Output")}
            SettingRow {
                objectName:"audioaudioEnabledRow";title:qsTr("Enable audio")
                visible:root.matches(title)||root.matches(qsTr("Output"))
                opacity:root.supported("audioEnabled")?1:.5
                help:root.helpFor("audioEnabled");infoName:"audioaudioEnabledInfo";controlHeight:32*root.uiScale
                Toggle{id:audioEnabled;objectName:"audioaudioEnabled";valueKey:"audioEnabled";accessibleTitle:qsTr("Enable audio");enabled:root.editable("audioEnabled")}
            }
            SettingRow {
                objectName:"audiooutputDeviceRow";title:qsTr("Output device")
                visible:root.matches(title)||root.matches(qsTr("Output"))
                opacity:root.supported("outputDevice")?1:.5
                help:root.helpFor("outputDevice");infoName:"audiooutputDeviceInfo";controlHeight:36*root.uiScale
                Choice{id:outputDevice;objectName:"audiooutputDevice";valueKey:"outputDevice";accessibleTitle:qsTr("Output device");options:root.devices;enabled:root.editable("outputDevice");displayText:!root.supported(valueKey)?qsTr("Unavailable"):currentIndex>=0?currentText:valueKey==="equalizerPreset"?qsTr("Custom"):qsTr("Current default / custom")}
            }
            SettingRow {
                objectName:"audiospeakerSetupRow";title:qsTr("Speaker setup")
                visible:root.matches(title)||root.matches(qsTr("Output"))
                opacity:root.supported("speakerSetup")?1:.5
                help:root.helpFor("speakerSetup");infoName:"audiospeakerSetupInfo";controlHeight:36*root.uiScale
                Choice{id:speakerSetup;objectName:"audiospeakerSetup";valueKey:"speakerSetup";accessibleTitle:qsTr("Speaker setup");options:root.nativeChoices("speakerSetup");enabled:root.editable("speakerSetup");displayText:!root.supported(valueKey)?qsTr("Unavailable"):currentIndex>=0?currentText:valueKey==="equalizerPreset"?qsTr("Custom"):qsTr("Current default / custom")}
            }
        }
        Column {
            width:parent.width
            visible:root.sectionMatches([qsTr("Volume"), qsTr("Maximum volume"), qsTr("Startup volume"), qsTr("Volume step")])
            SectionHeading{text:qsTr("Volume")}
            SettingRow {
                objectName:"audiomaximumVolumeRow";title:qsTr("Maximum volume")
                visible:root.matches(title)||root.matches(qsTr("Volume"))
                opacity:root.supported("maximumVolume")?1:.5
                help:"";infoName:"audiomaximumVolumeInfo";controlHeight:(Number(root.values.maximumVolume)>=100&&Number(root.values.maximumVolume)<=300?57:76)*root.uiScale
                VolumeRange{id:maximumVolume}
            }
            SettingRow {
                objectName:"audiorememberVolumeRow";title:qsTr("Startup volume")
                visible:root.matches(title)||root.matches(qsTr("Volume"))
                opacity:root.supported("rememberVolume")?1:.5
                help:root.helpFor("rememberVolume");infoName:"audiorememberVolumeInfo";controlHeight:78*root.uiScale
                Rectangle{
                    anchors.fill:parent;color:root.tone("#090909");radius:6*root.uiScale
                    SettingButton{id:rememberVolume;objectName:"audiorememberVolume";x:2*root.uiScale;y:2*root.uiScale;width:parent.width-4*root.uiScale;height:36*root.uiScale
                        text:qsTr("Remember last volume");enabled:root.editable("rememberVolume");onClicked:root.write("rememberVolume",true)
                        Accessible.role:Accessible.RadioButton;Accessible.checkable:true;Accessible.checked:root.values.rememberVolume===true
                        background:Rectangle{color:root.values.rememberVolume===true?root.accentSelected:root.tone("#191919");radius:4*root.uiScale}
                        contentItem:SettingLabel{text:rememberVolume.text;color:root.values.rememberVolume===true?root.accent:root.tone("#6B6B6B");leftPadding:12*root.uiScale;verticalAlignment:Text.AlignVCenter}
                    }
                    RowLayout{x:2*root.uiScale;y:40*root.uiScale;width:parent.width-4*root.uiScale;height:36*root.uiScale;spacing:2*root.uiScale
                        SettingButton{id:fixedVolume;objectName:"audioFixedVolume";Layout.preferredWidth:128*root.uiScale;Layout.fillHeight:true;text:qsTr("Always starts at:");enabled:root.editable("rememberVolume");onClicked:root.write("rememberVolume",false)
                            Accessible.role:Accessible.RadioButton;Accessible.checkable:true;Accessible.checked:root.values.rememberVolume===false
                            background:Rectangle{color:root.values.rememberVolume===false?root.accentSelected:root.tone("#191919");radius:4*root.uiScale}
                        }
                        Item{Layout.fillWidth:true;Layout.fillHeight:true
                            Input{id:startupVolume;objectName:"audiostartupVolume";valueKey:"startupVolume";unit:"%";accessibleTitle:qsTr("Fixed startup volume")}
                        }
                    }
                }
            }
            SettingRow {
                objectName:"audiovolumeStepRow";title:qsTr("Volume step")
                visible:root.matches(title)||root.matches(qsTr("Volume"))
                opacity:root.supported("volumeStep")?1:.5
                help:root.helpFor("volumeStep");infoName:"audiovolumeStepInfo";controlHeight:36*root.uiScale
                Input{id:volumeStep;objectName:"audiovolumeStep";valueKey:"volumeStep";accessibleTitle:qsTr("Volume step");}
            }
        }
        Column {
            width:parent.width
            visible:root.sectionMatches([qsTr("Loudness"), qsTr("Normalize loudness"), qsTr("Target level"), qsTr("Night mode"), qsTr("Replay gain"), qsTr("Replay gain preamp")])
            SectionHeading{text:qsTr("Loudness")}
            SettingRow {
                objectName:"audionormalizeLoudnessRow";title:qsTr("Normalize loudness")
                visible:root.matches(title)||root.matches(qsTr("Loudness"))
                opacity:root.supported("normalizeLoudness")?1:.5
                help:root.helpFor("normalizeLoudness");infoName:"audionormalizeLoudnessInfo";controlHeight:32*root.uiScale
                Toggle{id:normalizeLoudness;objectName:"audionormalizeLoudness";valueKey:"normalizeLoudness";accessibleTitle:qsTr("Normalize loudness");enabled:root.editable("normalizeLoudness")}
            }
            SettingRow {
                objectName:"audionormalizationTargetRow";title:qsTr("Target level")
                visible:root.matches(title)||root.matches(qsTr("Loudness"))
                opacity:root.supported("normalizationTarget")?1:.5
                help:root.helpFor("normalizationTarget");infoName:"audionormalizationTargetInfo";controlHeight:36*root.uiScale
                Input{id:normalizationTarget;objectName:"audionormalizationTarget";valueKey:"normalizationTarget";accessibleTitle:qsTr("Target level");}
            }
            SettingRow {
                objectName:"audionightModeRow";title:qsTr("Night mode")
                visible:root.matches(title)||root.matches(qsTr("Loudness"))
                opacity:root.supported("nightMode")?1:.5
                help:root.helpFor("nightMode");infoName:"audionightModeInfo";controlHeight:32*root.uiScale
                Toggle{id:nightMode;objectName:"audionightMode";valueKey:"nightMode";accessibleTitle:qsTr("Night mode");enabled:root.editable("nightMode")}
            }
            SettingRow {
                objectName:"audioreplayGainModeRow";title:qsTr("Replay gain")
                visible:root.matches(title)||root.matches(qsTr("Loudness"))
                opacity:root.supported("replayGainMode")?1:.5
                help:root.helpFor("replayGainMode");infoName:"audioreplayGainModeInfo";controlHeight:36*root.uiScale
                Choice{id:replayGainMode;objectName:"audioreplayGainMode";valueKey:"replayGainMode";accessibleTitle:qsTr("Replay gain");options:root.nativeChoices("replayGainMode");enabled:root.editable("replayGainMode");displayText:!root.supported(valueKey)?qsTr("Unavailable"):currentIndex>=0?currentText:valueKey==="equalizerPreset"?qsTr("Custom"):qsTr("Current default / custom")}
            }
            SettingRow {
                objectName:"audioreplayGainPreampRow";title:qsTr("Replay gain preamp")
                visible:root.matches(title)||root.matches(qsTr("Loudness"))
                opacity:root.supported("replayGainPreamp")?1:.5
                help:root.helpFor("replayGainPreamp");infoName:"audioreplayGainPreampInfo";controlHeight:36*root.uiScale
                Input{id:replayGainPreamp;objectName:"audioreplayGainPreamp";valueKey:"replayGainPreamp";accessibleTitle:qsTr("Replay gain preamp");}
            }
        }
        Column {
            width:parent.width
            visible:root.sectionMatches([qsTr("Effects"), qsTr("Equalizer"), qsTr("Equalizer preset"), qsTr("Two-pass"), qsTr("Spatial audio"), qsTr("Spatial audio mode"), qsTr("Stereo widening"), qsTr("Compressor"), qsTr("Pitch shift"), qsTr("Chorus/flanger"), qsTr("Karaoke"), qsTr("Preamp"), qsTr("Equalizer bands"), qsTr("Equalizer preamp")])
            SectionHeading{text:qsTr("Effects")}
            SettingRow {
                objectName:"audioequalizerEnabledRow";title:qsTr("Equalizer")
                visible:root.matches(title)||root.matches(qsTr("Effects"))
                opacity:root.supported("equalizerEnabled")?1:.5
                help:root.helpFor("equalizerEnabled");infoName:"audioequalizerEnabledInfo";controlHeight:32*root.uiScale
                Toggle{id:equalizerEnabled;objectName:"audioequalizerEnabled";valueKey:"equalizerEnabled";accessibleTitle:qsTr("Equalizer");enabled:root.editable("equalizerEnabled")}
            }
            SettingRow {
                objectName:"audioequalizerPresetRow";title:qsTr("Equalizer preset")
                visible:root.matches(title)||root.matches(qsTr("Effects"))
                opacity:root.supported("equalizerPreset")?1:.5
                help:root.helpFor("equalizerPreset");infoName:"audioequalizerPresetInfo";controlHeight:36*root.uiScale
                Choice{id:equalizerPreset;objectName:"audioequalizerPreset";valueKey:"equalizerPreset";accessibleTitle:qsTr("Equalizer preset");options:root.nativeChoices("equalizerPreset");enabled:root.editable("equalizerPreset");displayText:!root.supported(valueKey)?qsTr("Unavailable"):currentIndex>=0?currentText:valueKey==="equalizerPreset"?qsTr("Custom"):qsTr("Current default / custom")}
            }
            Rectangle {
                objectName:"audioEqualizerPanel"
                width:parent.width; height:root.equalizerVisible?eqScroll.height+74*root.uiScale:36*root.uiScale
                color:root.tone(root.equalizerVisible?"#191919":"#090909");radius:2*root.uiScale
                SettingButton {
                    id:eqOpen;objectName:"audioOpenEqualizer";anchors.horizontalCenter:parent.horizontalCenter;y:6*root.uiScale
                    width:160*root.uiScale;height:24*root.uiScale
                    text:root.equalizerVisible?qsTr("Close Equalizer"):qsTr("Open Equalizer")
                    onClicked:root.equalizerExpanded=!root.equalizerExpanded
                    background:Item{}
                    contentItem:Row{spacing:10*root.uiScale
                        SettingLabel{text:eqOpen.text;font:eqOpen.font}
                        Item{width:24*root.uiScale;height:24*root.uiScale
                            Image{objectName:"audioEqualizerOriginalChevron";anchors.centerIn:parent;width:24;height:24;scale:root.uiScale;mirror:true;source:root.assetBase+"audio-settings-764c9.svg";Accessible.ignored:true}
                        }
                    }
                }
                Flickable {
                    id:eqScroll;objectName:"audioEqualizerScroll"
                    readonly property real barHeight: Math.min(320*root.uiScale,Math.max(80*root.uiScale,root.viewportHeight>0?root.viewportHeight-20*root.uiScale:320*root.uiScale))
                    y:54*root.uiScale;width:parent.width;height:barHeight+95*root.uiScale;contentWidth:Math.max(width,740*root.uiScale);contentHeight:height
                    visible:root.equalizerVisible;clip:true;boundsBehavior:Flickable.StopAtBounds
                    C.ScrollBar.horizontal:C.ScrollBar{}
                    Row {
                        anchors.horizontalCenter:parent.horizontalCenter;spacing:8*root.uiScale
                        Repeater {
                            id:eqRepeater;model:11
                            Item {
                                id:channel;required property int index
                                property alias slider:gainSlider;property alias input:gainInput
                                readonly property string key:index===0?"equalizerPreamp":"equalizerBands"
                                readonly property var gain:index===0?root.values.equalizerPreamp:(root.values.equalizerBands||[])[index-1]
                                readonly property bool known:typeof gain==="number"&&Number.isFinite(gain)&&gain>=-20&&gain<=20
                                width:60*root.uiScale;height:eqScroll.height
                                C.Slider{
                                    id:gainSlider;objectName:"audioEqGain"+channel.index
                                    x:18*root.uiScale;width:24*root.uiScale;height:eqScroll.barHeight
                                    orientation:Qt.Vertical;from:-20;to:20;stepSize:.1;padding:0;topPadding:2*root.uiScale;bottomPadding:2*root.uiScale
                                    value:channel.known?channel.gain:0
                                    enabled:root.editable(channel.key)&&(channel.index===0||(root.values.equalizerBands||[]).length===10);activeFocusOnTab:true
                                    Accessible.name:channel.index===0?qsTr("Equalizer preamp"):qsTr("Equalizer %1").arg(root.frequencyLabel(root.frequencies[channel.index-1]))
                                    Accessible.description:root.helpFor(channel.key)
                                    function sync(){value=channel.known?channel.gain:0}
                                    onMoved:{
                                        const value=Math.round(gainSlider.value*10)/10
                                        if(channel.index===0)root.write("equalizerPreamp",value)
                                        else{const bands=(root.values.equalizerBands||[]).slice();if(bands.length===10){bands[channel.index-1]=value;root.writeBands(bands)}}
                                        sync()
                                    }
                                    Connections{target:root.service;function onValuesChanged(){gainSlider.sync()}}
                                    onActiveFocusChanged:root.reveal(gainSlider)
                                    Keys.onTabPressed:event=>{if(root.focusNavigator){root.navigate(gainSlider,false);event.accepted=true}}
                                    Keys.onBacktabPressed:event=>{if(root.focusNavigator){root.navigate(gainSlider,true);event.accepted=true}}
                                    background:Rectangle{
                                        x:4*root.uiScale;width:16*root.uiScale;height:gainSlider.height;color:root.tone("#090909");radius:6*root.uiScale
                                        Rectangle{x:2*root.uiScale;width:12*root.uiScale;height:channel.known?(parent.height-4*root.uiScale)*gainSlider.position:0;y:parent.height-2*root.uiScale-height;color:root.accentSelected;radius:4*root.uiScale}
                                        Repeater{model:channel.index===0?2:3;Item{
                                            required property int index
                                            x:5*root.uiScale;y:2*root.uiScale+(gainSlider.height-4*root.uiScale)*index/(channel.index===0?1:2)
                                            width:6*root.uiScale;height:0
                                            readonly property bool endpoint:index===0||index===(channel.index===0?1:2)
                                            clip:endpoint
                                            Image{objectName:"audioEq"+channel.index+"OriginalTick"+index;anchors.centerIn:parent;width:parent.endpoint?32:2;height:parent.endpoint?32:8;rotation:-90;scale:root.uiScale;source:root.assetBase+(parent.endpoint?"audio-settings-8ba8e.svg":"audio-settings-1ab84.svg");Accessible.ignored:true}
                                        }}
                                    }
                                    handle:Rectangle{visible:channel.known;x:0;y:2*root.uiScale+(gainSlider.height-4*root.uiScale)*(1-gainSlider.position)-height/2;width:24*root.uiScale;height:8*root.uiScale;radius:4*root.uiScale;color:root.accentDim}
                                }
                                SettingLabel{objectName:"audioEqFrequency"+channel.index;y:eqScroll.barHeight+33*root.uiScale;width:parent.width;height:17*root.uiScale;text:channel.index===0?qsTr("Preamp"):root.frequencyLabel(root.frequencies[channel.index-1]);horizontalAlignment:Text.AlignHCenter;color:root.tone("#6B6B6B")}
                                Item{x:0;y:eqScroll.barHeight+63*root.uiScale;width:60*root.uiScale;height:32*root.uiScale
                                    Input{id:gainInput;objectName:"audioEqInput"+channel.index;valueKey:channel.key;bandIndex:channel.index-1;unit:" dB";accessibleTitle:gainSlider.Accessible.name;font.pixelSize:12*root.uiScale;leftPadding:2*root.uiScale;rightPadding:2*root.uiScale;horizontalAlignment:Text.AlignHCenter
                                        background:Rectangle{color:root.tone("#090909");radius:6*root.uiScale;border.width:gainInput.visualFocus?root.uiScale:0;border.color:root.accent}
                                    }
                                }
                            }
                        }
                    }
                }
            }
            SettingRow {
                objectName:"audioequalizerTwoPassRow";title:qsTr("Two-pass")
                visible:root.matches(title)||root.matches(qsTr("Effects"))
                opacity:root.supported("equalizerTwoPass")?1:.5
                help:root.helpFor("equalizerTwoPass");infoName:"audioequalizerTwoPassInfo";controlHeight:32*root.uiScale
                Toggle{id:equalizerTwoPass;objectName:"audioequalizerTwoPass";valueKey:"equalizerTwoPass";accessibleTitle:qsTr("Two-pass");enabled:root.editable("equalizerTwoPass")}
            }
            SettingRow {
                objectName:"audiospatialAudioRow";title:qsTr("Spatial audio")
                visible:root.matches(title)||root.matches(qsTr("Effects"))
                opacity:root.supported("spatialAudio")?1:.5
                help:root.helpFor("spatialAudio");infoName:"audiospatialAudioInfo";controlHeight:32*root.uiScale
                Toggle{id:spatialAudio;objectName:"audiospatialAudio";valueKey:"spatialAudio";accessibleTitle:qsTr("Spatial audio");enabled:root.editable("spatialAudio")}
            }
            SettingRow {
                objectName:"audiospatialAudioModeRow";title:qsTr("Spatial audio mode")
                visible:root.matches(title)||root.matches(qsTr("Effects"))
                opacity:root.supported("spatialAudioMode")?1:.5
                help:root.helpFor("spatialAudioMode");infoName:"audiospatialAudioModeInfo";controlHeight:36*root.uiScale
                Choice{id:spatialAudioMode;objectName:"audiospatialAudioMode";valueKey:"spatialAudioMode";accessibleTitle:qsTr("Spatial audio mode");options:root.nativeChoices("spatialAudioMode");enabled:root.editable("spatialAudioMode");displayText:!root.supported(valueKey)?qsTr("Unavailable"):currentIndex>=0?currentText:valueKey==="equalizerPreset"?qsTr("Custom"):qsTr("Current default / custom")}
            }
            SettingRow {
                objectName:"audiostereoWideningRow";title:qsTr("Stereo widening")
                visible:root.matches(title)||root.matches(qsTr("Effects"))
                opacity:root.supported("stereoWidening")?1:.5
                help:root.helpFor("stereoWidening");infoName:"audiostereoWideningInfo";controlHeight:32*root.uiScale
                Toggle{id:stereoWidening;objectName:"audiostereoWidening";valueKey:"stereoWidening";accessibleTitle:qsTr("Stereo widening");enabled:root.editable("stereoWidening")}
            }
            SettingRow {
                objectName:"audiocompressorRow";title:qsTr("Compressor")
                visible:root.matches(title)||root.matches(qsTr("Effects"))
                opacity:root.supported("compressor")?1:.5
                help:root.helpFor("compressor");infoName:"audiocompressorInfo";controlHeight:32*root.uiScale
                Toggle{id:compressor;objectName:"audiocompressor";valueKey:"compressor";accessibleTitle:qsTr("Compressor");enabled:root.editable("compressor")}
            }
            SettingRow {
                objectName:"audiopitchShiftRow";title:qsTr("Pitch shift")
                visible:root.matches(title)||root.matches(qsTr("Effects"))
                opacity:root.supported("pitchShift")?1:.5
                help:root.helpFor("pitchShift");infoName:"audiopitchShiftInfo";controlHeight:36*root.uiScale
                Input{id:pitchShift;objectName:"audiopitchShift";valueKey:"pitchShift";accessibleTitle:qsTr("Pitch shift");}
            }
            SettingRow {
                objectName:"audiochorusFlangerRow";title:qsTr("Chorus/flanger")
                visible:root.matches(title)||root.matches(qsTr("Effects"))
                opacity:root.supported("chorusFlanger")?1:.5
                help:root.helpFor("chorusFlanger");infoName:"audiochorusFlangerInfo";controlHeight:32*root.uiScale
                Toggle{id:chorusFlanger;objectName:"audiochorusFlanger";valueKey:"chorusFlanger";accessibleTitle:qsTr("Chorus/flanger");enabled:root.editable("chorusFlanger")}
            }
            SettingRow {
                objectName:"audiokaraokeRow";title:qsTr("Karaoke")
                visible:root.matches(title)||root.matches(qsTr("Effects"))
                opacity:root.supported("karaoke")?1:.5
                help:root.helpFor("karaoke");infoName:"audiokaraokeInfo";controlHeight:32*root.uiScale
                Toggle{id:karaoke;objectName:"audiokaraoke";valueKey:"karaoke";accessibleTitle:qsTr("Karaoke");enabled:root.editable("karaoke")}
            }
        }
        Column {
            width:parent.width
            visible:root.sectionMatches([qsTr("Tracks"), qsTr("Preferred audio language"), qsTr("Audio delay")])
            SectionHeading{text:qsTr("Tracks")}
            SettingRow {
                objectName:"audiopreferredLanguageRow";title:qsTr("Preferred audio language")
                visible:root.matches(title)||root.matches(qsTr("Tracks"))
                opacity:root.supported("preferredLanguage")?1:.5
                help:root.helpFor("preferredLanguage");infoName:"audiopreferredLanguageInfo";controlHeight:36*root.uiScale
                Choice{id:preferredLanguage;objectName:"audiopreferredLanguage";valueKey:"preferredLanguage";accessibleTitle:qsTr("Preferred audio language");options:root.nativeChoices("preferredLanguage");enabled:root.editable("preferredLanguage");displayText:!root.supported(valueKey)?qsTr("Unavailable"):currentIndex>=0?currentText:valueKey==="equalizerPreset"?qsTr("Custom"):qsTr("Current default / custom")}
            }
            SettingRow {
                objectName:"audioaudioDelayMsRow";title:qsTr("Audio delay")
                visible:root.matches(title)||root.matches(qsTr("Tracks"))
                opacity:root.supported("audioDelayMs")?1:.5
                help:root.helpFor("audioDelayMs");infoName:"audioaudioDelayMsInfo";controlHeight:36*root.uiScale
                Input{id:audioDelayMs;objectName:"audioaudioDelayMs";valueKey:"audioDelayMs";accessibleTitle:qsTr("Audio delay");unit:qsTr(" ms");}
            }
        }
        Column {
            width:parent.width
            visible:root.sectionMatches([qsTr("Visualization"), qsTr("Visualizer")])
            SectionHeading{text:qsTr("Visualization")}
            SettingRow {
                objectName:"audiovisualizerRow";title:qsTr("Visualizer")
                visible:root.matches(title)||root.matches(qsTr("Visualization"))
                opacity:root.supported("visualizer")?1:.5
                help:root.helpFor("visualizer");infoName:"audiovisualizerInfo";controlHeight:36*root.uiScale
                Choice{id:visualizer;objectName:"audiovisualizer";valueKey:"visualizer";accessibleTitle:qsTr("Visualizer");options:root.nativeChoices("visualizer");enabled:root.editable("visualizer");displayText:!root.supported(valueKey)?qsTr("Unavailable"):currentIndex>=0?currentText:valueKey==="equalizerPreset"?qsTr("Custom"):qsTr("Current default / custom")}
            }
        }
        SettingButton {
            id:advancedButton;objectName:"audioAdvanced";width:parent.width;height:38*root.uiScale;text:qsTr("Advanced Options")
            onClicked:root.advancedExpanded=!root.advancedExpanded
            background:Rectangle{color:root.tone("#191919");opacity:.95;radius:2*root.uiScale}
            contentItem:Item{Row{anchors.centerIn:parent;spacing:10*root.uiScale
                SettingLabel{text:advancedButton.text;font:advancedButton.font}
                Item{width:24*root.uiScale;height:24*root.uiScale
                    Image{objectName:"audioAdvancedOriginalChevron";anchors.centerIn:parent;width:24;height:24;scale:root.uiScale;mirror:true;source:root.assetBase+(root.advancedVisible?"audio-settings-764c9.svg":"audio-settings-97863.svg");Accessible.ignored:true}
                }
            }}
        }
        Column {
            width:parent.width
            visible:root.advancedVisible
            SettingRow {
                objectName:"audiopassthroughRow";title:qsTr("Pass compressed audio to the receiver")
                visible:root.matches(title)||root.matches(qsTr("Advanced Options"))
                opacity:root.supported("passthrough")?1:.5
                help:root.helpFor("passthrough");infoName:"audiopassthroughInfo";controlHeight:32*root.uiScale
                Toggle{id:passthrough;objectName:"audiopassthrough";valueKey:"passthrough";accessibleTitle:qsTr("Pass compressed audio to the receiver");enabled:root.editable("passthrough")}
            }
            SettingRow {
                objectName:"audiodetectDolbyRow";title:qsTr("Detect Dolby Surround")
                visible:root.matches(title)||root.matches(qsTr("Advanced Options"))
                opacity:root.supported("detectDolby")?1:.5
                help:root.helpFor("detectDolby");infoName:"audiodetectDolbyInfo";controlHeight:36*root.uiScale
                Choice{id:detectDolby;objectName:"audiodetectDolby";valueKey:"detectDolby";accessibleTitle:qsTr("Detect Dolby Surround");options:root.nativeChoices("detectDolby");enabled:root.editable("detectDolby");displayText:!root.supported(valueKey)?qsTr("Unavailable"):currentIndex>=0?currentText:valueKey==="equalizerPreset"?qsTr("Custom"):qsTr("Current default / custom")}
            }
            SettingRow {
                objectName:"audioresamplingQualityRow";title:qsTr("Resampling quality")
                visible:root.matches(title)||root.matches(qsTr("Advanced Options"))
                opacity:root.supported("resamplingQuality")?1:.5
                help:root.helpFor("resamplingQuality");infoName:"audioresamplingQualityInfo";controlHeight:36*root.uiScale
                Choice{id:resamplingQuality;objectName:"audioresamplingQuality";valueKey:"resamplingQuality";accessibleTitle:qsTr("Resampling quality");options:root.nativeChoices("resamplingQuality");enabled:root.editable("resamplingQuality");displayText:!root.supported(valueKey)?qsTr("Unavailable"):currentIndex>=0?currentText:valueKey==="equalizerPreset"?qsTr("Custom"):qsTr("Current default / custom")}
            }
            SettingRow {
                objectName:"audiomediaRoleRow";title:qsTr("Media role")
                visible:root.matches(title)||root.matches(qsTr("Advanced Options"))
                opacity:root.supported("mediaRole")?1:.5
                help:root.helpFor("mediaRole");infoName:"audiomediaRoleInfo";controlHeight:36*root.uiScale
                Choice{id:mediaRole;objectName:"audiomediaRole";valueKey:"mediaRole";accessibleTitle:qsTr("Media role");options:root.nativeChoices("mediaRole");enabled:root.editable("mediaRole");displayText:!root.supported(valueKey)?qsTr("Unavailable"):currentIndex>=0?currentText:valueKey==="equalizerPreset"?qsTr("Custom"):qsTr("Current default / custom")}
            }
            SettingRow {
                objectName:"audiovisualizationSizeRow";title:qsTr("Visualization size")
                visible:root.matches(title)||root.matches(qsTr("Advanced Options"))
                opacity:root.supported("visualizationSize")?1:.5
                help:root.helpFor("visualizationSize");infoName:"audiovisualizationSizeInfo";controlHeight:36*root.uiScale
                Choice{id:visualizationSize;objectName:"audiovisualizationSize";valueKey:"visualizationSize";accessibleTitle:qsTr("Visualization size");options:root.nativeChoices("visualizationSize");enabled:root.editable("visualizationSize");displayText:!root.supported(valueKey)?qsTr("Unavailable"):currentIndex>=0?currentText:valueKey==="equalizerPreset"?qsTr("Custom"):qsTr("Current default / custom")}
            }
        }
    }
}
