// SPDX-License-Identifier: GPL-2.0-or-later
// Original visual design and SVG artwork: Juxtopposed, CC BY 4.0.
import QtQuick
import QtQuick.Controls.Basic as C
import QtQuick.Layouts
FocusScope {
    id:root;objectName:"juxtopposedVideoSettings"
    required property var preferences
    property var videoPreferences:null
    readonly property var service:videoPreferences
    property var appearance:null
    property real uiScale:1
    property url assetBase:"qrc:/juxtopposed/"
    property string searchText:""
    property var focusNavigator:null
    signal focusRevealed(var item)
    signal advancedRequested()
    readonly property var values:service?service.values:({})
    readonly property var capabilities:service?service.capabilities:({})
    readonly property var choices:service?service.choices:({})
    readonly property var applyModes:service?service.applyModes:({})
    readonly property var supportNotes:service?service.supportNotes:({})
    readonly property int nativeScreensaverMode:service?service.nativeScreensaverMode:-1
    readonly property bool available:!!service
    readonly property bool pickerBusy:!!service&&service.pickerActive
    readonly property bool busy:pickerBusy||confirmation.visible
    readonly property bool stacked:width<650*uiScale
    readonly property color accent:appearance?appearance.accent:"#D37918"
    readonly property color accentSelected:appearance?appearance.accentSelected:"#371F06"
    readonly property color accentDim:appearance?appearance.accentDim:"#AB6314"
    readonly property var settingKeys:["enableVideo", "startFullscreen", "disableScreensaver", "wallpaper", "hue", "brightness", "contrast", "saturation", "gamma", "deinterlace", "deinterlaceMode", "aspectRatio", "crop", "scaleToWindow", "zoom", "position", "snapshotDirectory", "snapshotPrefix", "snapshotFormat", "snapshotSize", "snapshotSequential", "snapshotPreview", "customAspectRatios", "customCropRatios", "monitorPixelAspectRatio", "timeShiftBufferSize"]
    readonly property var ranges:({hue:{minimum:-180,maximum:180},brightness:{minimum:0,maximum:2},contrast:{minimum:0,maximum:2},saturation:{minimum:0,maximum:3},gamma:{minimum:.01,maximum:10}})
    property bool advancedExpanded:false
    readonly property bool advancedVisible:advancedExpanded||searchText.trim().length>0&&sectionMatches([qsTr("Custom aspect ratios"),qsTr("Custom crop ratios"),qsTr("Monitor pixel aspect ratio"),qsTr("Time-shift buffer size")])
    readonly property var focusTargets:confirmation.visible?[cancelReset,confirmReset]:[enableVideo,startFullscreen,disableScreensaver,wallpaper,
        hue.control,brightness.control,contrast.control,saturation.control,gamma.control,resetPicture,
        deinterlace,deinterlaceMode,aspectRatio,crop,scaleToWindow,zoom,position,snapshotDirectory,snapshotBrowse,
        snapshotPrefix,snapshotFormat,snapshotSize,snapshotSequential,snapshotPreview,advancedButton,
        customAspectRatios,customCropRatios,monitorPixelAspectRatio,timeShiftBufferSize]
    implicitHeight:form.implicitHeight
    function tone(value){return appearance?appearance.color(value):value}
    function matches(text){return !searchText.trim().length||text.toLocaleLowerCase().includes(searchText.trim().toLocaleLowerCase())}
    function sectionMatches(names){return names.some(name=>matches(name))}
    function supported(key){return available&&capabilities[key]===true&&(key==="resetPicture"||key in values)}
    function editable(key){return supported(key)&&!busy}
    function write(key,value){return editable(key)&&service.setValue(key,value)}
    function nativeChoices(key){return choices[key]||[]}
    function navigate(item,backwards){if(focusNavigator)focusNavigator(item,backwards)}
    function reveal(item){if(item.activeFocus)focusRevealed(item)}
    function closeChoices(){for(const choice of[deinterlace,deinterlaceMode,aspectRatio,crop,zoom,position,snapshotFormat,snapshotSize,timeShiftBufferSize])if(choice.popup.visible)choice.popup.close()}
    function focusActivePopup(){
        if(confirmation.visible){cancelReset.forceActiveFocus(Qt.PopupFocusReason);return true}
        if(pickerBusy)return true
        closeChoices();return false
    }
    onVisibleChanged:if(!visible){confirmation.close();closeChoices()}
    function helpFor(key){
        const group=["hue","brightness","contrast","saturation","gamma"].includes(key)?"picture":["aspectRatio","crop","scaleToWindow","zoom","position"].includes(key)?"framing":key
        const hints={deinterlace:qsTr("Choose automatic detection, On, or Off."),deinterlaceMode:qsTr("Choose a deinterlacing method for future playback."),
            snapshotDirectory:qsTr("Choose an existing writable local folder, or leave empty for the default."),
            snapshotPrefix:qsTr("Use a file name prefix without path separators or reserved filename characters."),
            snapshotFormat:qsTr("Choose an available native image encoder."),snapshotSequential:qsTr("Number snapshot files in sequence.")}
        const timing={restart:qsTr("Restart VLC to apply this default."),nextInput:qsTr("Applies when the next media opens."),nextSnapshot:qsTr("Applies to the next native snapshot.")}
        return(supportNotes[group]||hints[key]||"")+(timing[applyModes[key]]?" "+timing[applyModes[key]]:"")+(supported(key)?"":" "+qsTr("This setting is unavailable in this build."))
    }
    FontLoader{id:inter;source:root.assetBase+"InterVariable.ttf"}
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
                        source: root.assetBase + "video-settings-1f9fe.svg"
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
                source: root.assetBase + "video-settings-764c9.svg"
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
        readonly property var current: root.values[valueKey]
        readonly property var bounds: root.ranges[valueKey]
        anchors.fill: parent
        enabled: root.editable(valueKey)
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
            let raw=kind === "text" ? text : text.trim()
            if (unit.length && raw.endsWith(unit)) raw=raw.slice(0,-unit.length).trim()
            const value=kind === "text" ? raw : raw.length && Number.isFinite(Number(raw)) ? Number(raw) : raw
            if (value !== current) {
                const accepted=root.write(valueKey,value)
                validationError=accepted||!root.service?"":root.service.lastError
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
    component PictureRange: Item {
        id:range
        required property string valueKey
        required property string accessibleTitle
        required property int tickCount
        property alias control:slider
        readonly property var bounds:root.ranges[valueKey]
        readonly property var current:root.values[valueKey]
        readonly property bool known:typeof current==="number"&&Number.isFinite(current)&&current>=bounds.minimum&&current<=bounds.maximum
        anchors.fill:parent
        C.Slider{
            id:slider;objectName:"video"+range.valueKey
            anchors.verticalCenter:parent.verticalCenter;width:parent.width;height:24*root.uiScale
            from:range.bounds.minimum;to:range.bounds.maximum;stepSize:.01;padding:0;leftPadding:2*root.uiScale;rightPadding:2*root.uiScale
            value:range.known?range.current:range.bounds.minimum;enabled:root.editable(range.valueKey);activeFocusOnTab:true
            Accessible.name:range.accessibleTitle;Accessible.description:root.helpFor(range.valueKey)+qsTr(" Supported range: %1 to %2.").arg(from).arg(to)
            function sync(){value=range.known?range.current:range.bounds.minimum}
            onMoved:{root.write(range.valueKey,Math.round(value*100)/100);sync()}
            Connections{target:root.service;function onValuesChanged(){slider.sync()}}
            onActiveFocusChanged:root.reveal(slider)
            Keys.onTabPressed:event=>{if(root.focusNavigator){root.navigate(slider,false);event.accepted=true}}
            Keys.onBacktabPressed:event=>{if(root.focusNavigator){root.navigate(slider,true);event.accepted=true}}
            C.ToolTip.visible:hovered||visualFocus
            C.ToolTip.text:range.known?String(range.current):qsTr("Current value is outside the native range")
            background:Rectangle{
                y:4*root.uiScale;width:slider.width;height:16*root.uiScale;color:root.tone("#090909");radius:6*root.uiScale
                Rectangle{x:2*root.uiScale;y:2*root.uiScale;width:range.known?(slider.width-4*root.uiScale)*slider.position:0;height:12*root.uiScale;radius:4*root.uiScale;color:root.accentSelected}
                Repeater{model:range.tickCount;Item{
                    required property int index;x:2*root.uiScale+(slider.width-4*root.uiScale)*index/(range.tickCount-1);y:5*root.uiScale;width:0;height:6*root.uiScale
                    readonly property bool endpoint:index===0||index===range.tickCount-1
                    clip:endpoint
                    Image{objectName:"video"+range.valueKey+"OriginalTick"+index;anchors.centerIn:parent;width:parent.endpoint?32:2;height:parent.endpoint?32:8;scale:root.uiScale;source:root.assetBase+(parent.endpoint?"video-settings-8ba8e.svg":"video-settings-1ab84.svg");Accessible.ignored:true}
                }}
            }
            handle:Rectangle{visible:range.known;x:2*root.uiScale+(slider.width-4*root.uiScale)*slider.position-width/2;y:0;width:8*root.uiScale;height:24*root.uiScale;radius:4*root.uiScale;color:root.accentDim}
        }
    }
    Column{
        id:form;width:root.width;spacing:20*root.uiScale
        SettingLabel{objectName:"videoSettingsError";width:parent.width;text:root.service?root.service.lastError:"";visible:text.length>0;color:"#D98779";Accessible.role:Accessible.StaticText;Accessible.name:text}
        SettingLabel{objectName:"videoSettingsRestart";width:parent.width;text:qsTr("Restart VLC to apply these video defaults.");visible:root.service&&root.service.restartRequired;color:root.tone("#C6C6C6");Accessible.role:Accessible.StaticText;Accessible.name:text}
        Column{width:parent.width
            visible:root.sectionMatches([qsTr("Output"), qsTr("Enable video"), qsTr("Start videos in fullscreen"), qsTr("Stop the screensaver during playback"), qsTr("Play video as desktop wallpaper")])
            SectionHeading{text:qsTr("Output")}
            SettingRow{
                objectName:"videoenableVideoRow";title:qsTr("Enable video");visible:root.matches(title)||root.matches(qsTr("Output"))
                opacity:root.supported("enableVideo")?1:.5
                help:root.helpFor("enableVideo");infoName:"videoenableVideoInfo";controlHeight:32*root.uiScale
                Toggle{id:enableVideo;objectName:"videoenableVideo";valueKey:"enableVideo";accessibleTitle:qsTr("Enable video");enabled:root.editable("enableVideo")}
            }
            SettingRow{
                objectName:"videostartFullscreenRow";title:qsTr("Start videos in fullscreen");visible:root.matches(title)||root.matches(qsTr("Output"))
                opacity:root.supported("startFullscreen")?1:.5
                help:root.helpFor("startFullscreen");infoName:"videostartFullscreenInfo";controlHeight:32*root.uiScale
                Toggle{id:startFullscreen;objectName:"videostartFullscreen";valueKey:"startFullscreen";accessibleTitle:qsTr("Start videos in fullscreen");enabled:root.editable("startFullscreen")}
            }
            SettingRow{
                objectName:"videodisableScreensaverRow";title:qsTr("Stop the screensaver during playback");visible:root.matches(title)||root.matches(qsTr("Output"))
                opacity:root.supported("disableScreensaver")?1:.5
                help:root.helpFor("disableScreensaver");infoName:"videodisableScreensaverInfo";controlHeight:32*root.uiScale
                Toggle{id:disableScreensaver;objectName:"videodisableScreensaver";valueKey:"disableScreensaver";accessibleTitle:qsTr("Stop the screensaver during playback");enabled:root.editable("disableScreensaver")}
            }
            SettingRow{
                objectName:"videowallpaperRow";title:qsTr("Play video as desktop wallpaper");visible:root.matches(title)||root.matches(qsTr("Output"))
                opacity:root.supported("wallpaper")?1:.5
                help:root.helpFor("wallpaper");infoName:"videowallpaperInfo";controlHeight:32*root.uiScale
                Toggle{id:wallpaper;objectName:"videowallpaper";valueKey:"wallpaper";accessibleTitle:qsTr("Play video as desktop wallpaper");enabled:root.editable("wallpaper")}
            }
        }
        Column{width:parent.width
            visible:root.sectionMatches([qsTr("Picture"), qsTr("Hue"), qsTr("Brightness"), qsTr("Contrast"), qsTr("Saturation"), qsTr("Gamma"), qsTr("Reset picture")])
            SectionHeading{text:qsTr("Picture")}
            SettingRow{
                objectName:"videohueRow";title:qsTr("Hue");visible:root.matches(title)||root.matches(qsTr("Picture"))
                opacity:root.supported("hue")?1:.5
                help:"";infoName:"videohueInfo";controlHeight:32*root.uiScale
                PictureRange{id:hue;valueKey:"hue";accessibleTitle:qsTr("Hue");tickCount:3}
            }
            SettingRow{
                objectName:"videobrightnessRow";title:qsTr("Brightness");visible:root.matches(title)||root.matches(qsTr("Picture"))
                opacity:root.supported("brightness")?1:.5
                help:"";infoName:"videobrightnessInfo";controlHeight:32*root.uiScale
                PictureRange{id:brightness;valueKey:"brightness";accessibleTitle:qsTr("Brightness");tickCount:3}
            }
            SettingRow{
                objectName:"videocontrastRow";title:qsTr("Contrast");visible:root.matches(title)||root.matches(qsTr("Picture"))
                opacity:root.supported("contrast")?1:.5
                help:"";infoName:"videocontrastInfo";controlHeight:32*root.uiScale
                PictureRange{id:contrast;valueKey:"contrast";accessibleTitle:qsTr("Contrast");tickCount:3}
            }
            SettingRow{
                objectName:"videosaturationRow";title:qsTr("Saturation");visible:root.matches(title)||root.matches(qsTr("Picture"))
                opacity:root.supported("saturation")?1:.5
                help:"";infoName:"videosaturationInfo";controlHeight:32*root.uiScale
                PictureRange{id:saturation;valueKey:"saturation";accessibleTitle:qsTr("Saturation");tickCount:4}
            }
            SettingRow{
                objectName:"videogammaRow";title:qsTr("Gamma");visible:root.matches(title)||root.matches(qsTr("Picture"))
                opacity:root.supported("gamma")?1:.5
                help:"";infoName:"videogammaInfo";controlHeight:32*root.uiScale
                PictureRange{id:gamma;valueKey:"gamma";accessibleTitle:qsTr("Gamma");tickCount:11}
            }
            SettingRow{
                objectName:"videoresetPictureRow";title:qsTr("Reset picture");visible:root.matches(title)||root.matches(qsTr("Picture"))
                opacity:root.supported("resetPicture")?1:.5
                help:"";infoName:"videoresetPictureInfo";controlHeight:32*root.uiScale
                SettingButton{id:resetPicture;objectName:"videoResetPicture";anchors.right:parent.right;width:100*root.uiScale;height:32*root.uiScale;text:qsTr("Reset");font.weight:Font.DemiBold;enabled:root.editable("resetPicture");onClicked:{confirmation.open();cancelReset.forceActiveFocus(Qt.PopupFocusReason)}}
            }
        }
        Column{width:parent.width
            visible:root.sectionMatches([qsTr("Deinterlacing"), qsTr("Deinterlacing"), qsTr("Method")])
            SectionHeading{text:qsTr("Deinterlacing")}
            SettingRow{
                objectName:"videodeinterlaceRow";title:qsTr("Deinterlacing");visible:root.matches(title)||root.matches(qsTr("Deinterlacing"))
                opacity:root.supported("deinterlace")?1:.5
                help:root.helpFor("deinterlace");infoName:"videodeinterlaceInfo";controlHeight:36*root.uiScale
                Choice{id:deinterlace;objectName:"videodeinterlace";valueKey:"deinterlace";accessibleTitle:qsTr("Deinterlacing");options:root.nativeChoices("deinterlace");enabled:root.editable("deinterlace")}
            }
            SettingRow{
                objectName:"videodeinterlaceModeRow";title:qsTr("Method");visible:root.matches(title)||root.matches(qsTr("Deinterlacing"))
                opacity:root.supported("deinterlaceMode")?1:.5
                help:root.helpFor("deinterlaceMode");infoName:"videodeinterlaceModeInfo";controlHeight:36*root.uiScale
                Choice{id:deinterlaceMode;objectName:"videodeinterlaceMode";valueKey:"deinterlaceMode";accessibleTitle:qsTr("Method");options:root.nativeChoices("deinterlaceMode");enabled:root.editable("deinterlaceMode")}
            }
        }
        Column{width:parent.width
            visible:root.sectionMatches([qsTr("Framing"), qsTr("Aspect ratio"), qsTr("Crop"), qsTr("Scale video to the window"), qsTr("Zoom"), qsTr("Position in the window")])
            SectionHeading{text:qsTr("Framing")}
            SettingRow{
                objectName:"videoaspectRatioRow";title:qsTr("Aspect ratio");visible:root.matches(title)||root.matches(qsTr("Framing"))
                opacity:root.supported("aspectRatio")?1:.5
                help:root.helpFor("aspectRatio");infoName:"videoaspectRatioInfo";controlHeight:36*root.uiScale
                Choice{id:aspectRatio;objectName:"videoaspectRatio";valueKey:"aspectRatio";accessibleTitle:qsTr("Aspect ratio");options:root.nativeChoices("aspectRatio");enabled:root.editable("aspectRatio")}
            }
            SettingRow{
                objectName:"videocropRow";title:qsTr("Crop");visible:root.matches(title)||root.matches(qsTr("Framing"))
                opacity:root.supported("crop")?1:.5
                help:root.helpFor("crop");infoName:"videocropInfo";controlHeight:36*root.uiScale
                Choice{id:crop;objectName:"videocrop";valueKey:"crop";accessibleTitle:qsTr("Crop");options:root.nativeChoices("crop");enabled:root.editable("crop")}
            }
            SettingRow{
                objectName:"videoscaleToWindowRow";title:qsTr("Scale video to the window");visible:root.matches(title)||root.matches(qsTr("Framing"))
                opacity:root.supported("scaleToWindow")?1:.5
                help:root.helpFor("scaleToWindow");infoName:"videoscaleToWindowInfo";controlHeight:32*root.uiScale
                Toggle{id:scaleToWindow;objectName:"videoscaleToWindow";valueKey:"scaleToWindow";accessibleTitle:qsTr("Scale video to the window");enabled:root.editable("scaleToWindow")}
            }
            SettingRow{
                objectName:"videozoomRow";title:qsTr("Zoom");visible:root.matches(title)||root.matches(qsTr("Framing"))
                opacity:root.supported("zoom")?1:.5
                help:root.helpFor("zoom");infoName:"videozoomInfo";controlHeight:36*root.uiScale
                Choice{id:zoom;objectName:"videozoom";valueKey:"zoom";accessibleTitle:qsTr("Zoom");options:root.nativeChoices("zoom");enabled:root.editable("zoom")}
            }
            SettingRow{
                objectName:"videopositionRow";title:qsTr("Position in the window");visible:root.matches(title)||root.matches(qsTr("Framing"))
                opacity:root.supported("position")?1:.5
                help:root.helpFor("position");infoName:"videopositionInfo";controlHeight:36*root.uiScale
                Choice{id:position;objectName:"videoposition";valueKey:"position";accessibleTitle:qsTr("Position in the window");options:root.nativeChoices("position");enabled:root.editable("position")}
            }
        }
        Column{width:parent.width
            visible:root.sectionMatches([qsTr("Snapshots"), qsTr("Save to"), qsTr("File name prefix"), qsTr("Format"), qsTr("Size"), qsTr("Number files sequentially"), qsTr("Show a preview after capture")])
            SectionHeading{text:qsTr("Snapshots")}
            SettingRow{
                objectName:"videosnapshotDirectoryRow";title:qsTr("Save to");visible:root.matches(title)||root.matches(qsTr("Snapshots"))
                opacity:root.supported("snapshotDirectory")?1:.5
                help:root.helpFor("snapshotDirectory");infoName:"videosnapshotDirectoryInfo";controlHeight:32*root.uiScale
                RowLayout{anchors.fill:parent;spacing:10*root.uiScale
                    Item{Layout.fillWidth:true;Layout.fillHeight:true;Input{id:snapshotDirectory;objectName:"videosnapshotDirectory";valueKey:"snapshotDirectory";kind:"text";accessibleTitle:qsTr("Save snapshots to")}}
                    SettingButton{id:snapshotBrowse;objectName:"videoSnapshotBrowse";Layout.preferredWidth:100*root.uiScale;Layout.preferredHeight:32*root.uiScale;text:qsTr("Browse");font.weight:Font.DemiBold;enabled:root.editable("snapshotDirectory");onClicked:root.service.chooseSnapshotDirectory()}
                }
            }
            SettingRow{
                objectName:"videosnapshotPrefixRow";title:qsTr("File name prefix");visible:root.matches(title)||root.matches(qsTr("Snapshots"))
                opacity:root.supported("snapshotPrefix")?1:.5
                help:root.helpFor("snapshotPrefix");infoName:"videosnapshotPrefixInfo";controlHeight:36*root.uiScale
                Input{id:snapshotPrefix;objectName:"videosnapshotPrefix";valueKey:"snapshotPrefix";kind:"text";accessibleTitle:qsTr("File name prefix")}
            }
            SettingRow{
                objectName:"videosnapshotFormatRow";title:qsTr("Format");visible:root.matches(title)||root.matches(qsTr("Snapshots"))
                opacity:root.supported("snapshotFormat")?1:.5
                help:root.helpFor("snapshotFormat");infoName:"videosnapshotFormatInfo";controlHeight:36*root.uiScale
                Choice{id:snapshotFormat;objectName:"videosnapshotFormat";valueKey:"snapshotFormat";accessibleTitle:qsTr("Format");options:root.nativeChoices("snapshotFormat");enabled:root.editable("snapshotFormat")}
            }
            SettingRow{
                objectName:"videosnapshotSizeRow";title:qsTr("Size");visible:root.matches(title)||root.matches(qsTr("Snapshots"))
                opacity:root.supported("snapshotSize")?1:.5
                help:root.helpFor("snapshotSize");infoName:"videosnapshotSizeInfo";controlHeight:36*root.uiScale
                Choice{id:snapshotSize;objectName:"videosnapshotSize";valueKey:"snapshotSize";accessibleTitle:qsTr("Size");options:root.nativeChoices("snapshotSize");enabled:root.editable("snapshotSize")}
            }
            SettingRow{
                objectName:"videosnapshotSequentialRow";title:qsTr("Number files sequentially");visible:root.matches(title)||root.matches(qsTr("Snapshots"))
                opacity:root.supported("snapshotSequential")?1:.5
                help:root.helpFor("snapshotSequential");infoName:"videosnapshotSequentialInfo";controlHeight:32*root.uiScale
                Toggle{id:snapshotSequential;objectName:"videosnapshotSequential";valueKey:"snapshotSequential";accessibleTitle:qsTr("Number files sequentially");enabled:root.editable("snapshotSequential")}
            }
            SettingRow{
                objectName:"videosnapshotPreviewRow";title:qsTr("Show a preview after capture");visible:root.matches(title)||root.matches(qsTr("Snapshots"))
                opacity:root.supported("snapshotPreview")?1:.5
                help:root.helpFor("snapshotPreview");infoName:"videosnapshotPreviewInfo";controlHeight:32*root.uiScale
                Toggle{id:snapshotPreview;objectName:"videosnapshotPreview";valueKey:"snapshotPreview";accessibleTitle:qsTr("Show a preview after capture");enabled:root.editable("snapshotPreview")}
            }
        }
        SettingButton{id:advancedButton;objectName:"videoAdvanced";width:parent.width;height:38*root.uiScale;text:qsTr("Advanced Options")
            onClicked:root.advancedExpanded=!root.advancedExpanded
            background:Rectangle{color:root.tone("#191919");opacity:.95;radius:2*root.uiScale}
            contentItem:Item{Row{anchors.centerIn:parent;spacing:10*root.uiScale
                SettingLabel{text:advancedButton.text;font:advancedButton.font}
                Item{width:24*root.uiScale;height:24*root.uiScale
                    Image{objectName:"videoAdvancedOriginalChevron";anchors.centerIn:parent;width:24;height:24;scale:root.uiScale;mirror:true;source:root.assetBase+(root.advancedVisible?"video-settings-764c9.svg":"video-settings-97863.svg");Accessible.ignored:true}
                }
            }}
        }
        Column{width:parent.width
            visible:root.advancedVisible
            SettingRow{
                objectName:"videocustomAspectRatiosRow";title:qsTr("Custom aspect ratios");visible:root.matches(title)||root.matches(qsTr("Advanced Options"))
                opacity:root.supported("customAspectRatios")?1:.5
                help:root.helpFor("customAspectRatios");infoName:"videocustomAspectRatiosInfo";controlHeight:36*root.uiScale
                Input{id:customAspectRatios;objectName:"videocustomAspectRatios";valueKey:"customAspectRatios";kind:"text";accessibleTitle:qsTr("Custom aspect ratios")}
            }
            SettingRow{
                objectName:"videocustomCropRatiosRow";title:qsTr("Custom crop ratios");visible:root.matches(title)||root.matches(qsTr("Advanced Options"))
                opacity:root.supported("customCropRatios")?1:.5
                help:root.helpFor("customCropRatios");infoName:"videocustomCropRatiosInfo";controlHeight:36*root.uiScale
                Input{id:customCropRatios;objectName:"videocustomCropRatios";valueKey:"customCropRatios";kind:"text";accessibleTitle:qsTr("Custom crop ratios")}
            }
            SettingRow{
                objectName:"videomonitorPixelAspectRatioRow";title:qsTr("Monitor pixel aspect ratio");visible:root.matches(title)||root.matches(qsTr("Advanced Options"))
                opacity:root.supported("monitorPixelAspectRatio")?1:.5
                help:root.helpFor("monitorPixelAspectRatio");infoName:"videomonitorPixelAspectRatioInfo";controlHeight:36*root.uiScale
                Input{id:monitorPixelAspectRatio;objectName:"videomonitorPixelAspectRatio";valueKey:"monitorPixelAspectRatio";kind:"text";accessibleTitle:qsTr("Monitor pixel aspect ratio")}
            }
            SettingRow{
                objectName:"videotimeShiftBufferSizeRow";title:qsTr("Time-shift buffer size");visible:root.matches(title)||root.matches(qsTr("Advanced Options"))
                opacity:root.supported("timeShiftBufferSize")?1:.5
                help:root.helpFor("timeShiftBufferSize");infoName:"videotimeShiftBufferSizeInfo";controlHeight:36*root.uiScale
                Choice{id:timeShiftBufferSize;objectName:"videotimeShiftBufferSize";valueKey:"timeShiftBufferSize";accessibleTitle:qsTr("Time-shift buffer size");options:root.nativeChoices("timeShiftBufferSize");enabled:root.editable("timeShiftBufferSize")}
            }
        }
    }
    C.Dialog {
        id: confirmation
        objectName: "videoResetConfirmation"
        parent: C.Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(420 * root.uiScale, parent ? parent.width - 32 * root.uiScale : 420 * root.uiScale)
        modal: true; focus: true
        closePolicy: C.Popup.CloseOnEscape
        title: qsTr("Reset picture settings?")
        padding: 16 * root.uiScale
        font.family: inter.name; font.pixelSize: 14 * root.uiScale
        background: Rectangle { color: root.tone("#191919"); radius: 6 * root.uiScale; border.width: root.uiScale; border.color: root.tone("#404040") }
        header: SettingLabel { text: confirmation.title; padding: 16 * root.uiScale; color: root.tone("#C6C6C6"); font.pixelSize: 16 * root.uiScale; font.weight: Font.Medium }
        contentItem: Column {
            spacing: 16 * root.uiScale
            SettingLabel { width: parent.width; text: qsTr("Restore Hue, Brightness, Contrast, Saturation and Gamma to their defaults. Other video effects and settings are kept.") }
            Row {
                anchors.right: parent.right
                spacing: 10 * root.uiScale
                SettingButton {
                    id: cancelReset; objectName: "videoCancelReset"
                    text: qsTr("Cancel"); enabled: !root.pickerBusy
                    onClicked: confirmation.close()
                    Keys.onTabPressed: { confirmReset.forceActiveFocus(Qt.TabFocusReason) }
                    Keys.onBacktabPressed: { confirmReset.forceActiveFocus(Qt.BacktabFocusReason) }
                }
                SettingButton {
                    enabled: !root.pickerBusy && root.supported("resetPicture")
                    id: confirmReset; objectName: "videoConfirmReset"
                    text: qsTr("Reset")
                    onClicked: { confirmation.close(); if (!root.pickerBusy && root.supported("resetPicture")) root.service.resetPicture() }
                    Keys.onTabPressed: { cancelReset.forceActiveFocus(Qt.TabFocusReason) }
                    Keys.onBacktabPressed: { cancelReset.forceActiveFocus(Qt.BacktabFocusReason) }
                }
            }
        }
        onClosed: if (resetPicture && resetPicture.visible && resetPicture.enabled) resetPicture.forceActiveFocus(Qt.PopupFocusReason)
    }
}
