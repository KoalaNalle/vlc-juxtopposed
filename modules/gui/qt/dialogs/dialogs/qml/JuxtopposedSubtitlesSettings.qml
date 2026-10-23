// SPDX-License-Identifier: GPL-2.0-or-later
// Original visual design and artwork: Juxtopposed, CC BY 4.0.
import QtQuick
import QtQuick.Controls.Basic as C
import QtQuick.Layouts
FocusScope {
    id:root;objectName:"juxtopposedSubtitlesSettings"
    required property var preferences
    property var subtitlesPreferences:null
    readonly property var service:subtitlesPreferences
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
    readonly property bool available:!!service
    readonly property bool busy:false
    readonly property bool stacked:width<650*uiScale
    readonly property color accent:appearance?appearance.accent:"#D37918"
    readonly property color accentSelected:appearance?appearance.accentSelected:"#371F06"
    readonly property color accentDim:appearance?appearance.accentDim:"#AB6314"
    readonly property var settingKeys:["enableSubtitles", "preferredLanguage", "textEncoding", "autoLoad", "filenameMatching", "fontFamily", "sizeMode", "fontSize", "bold", "textColor", "textOpacity", "outlineThickness", "outlineColor", "shadow", "shadowColor", "shadowDistance", "background", "backgroundColor", "backgroundOpacity", "verticalPosition", "subtitleDelay", "showFeedback", "feedbackDuration", "showFilename", "filenameDuration", "searchFolders", "ignoreAssStyling", "shadowAngle", "textDirection", "monospaceFont", "visualizationSize"]
    readonly property var ranges:Object.assign({"textOpacity": {"minimum": 0, "maximum": 100, "step": 1, "labels": ["0%", "50%", "100%"], "tickCount": 11}, "backgroundOpacity": {"minimum": 0, "maximum": 100, "step": 1, "labels": ["0%", "50%", "100%"], "tickCount": 11}, "subtitleDelay": {"minimum": -5000, "maximum": 5000, "step": 100, "labels": ["-5 s", "0 s", "+5 s"], "tickCount": 11}, "feedbackDuration": {"minimum": 500, "maximum": 5000, "step": 500, "labels": ["0.5 s", "5 s"], "tickCount": 10}, "filenameDuration": {"minimum": 1000, "maximum": 10000, "step": 1000, "labels": ["1 s", "10 s"], "tickCount": 10}},{fontSize:{minimum:values.sizeMode===1?1:.1,maximum:values.sizeMode===1?512:50},outlineThickness:{minimum:0,maximum:50},shadowDistance:{minimum:0,maximum:1},shadowAngle:{minimum:-360,maximum:360}})
    property bool advancedExpanded:false
    readonly property bool advancedVisible:advancedExpanded||searchText.trim().length>0&&sectionMatches([qsTr("Load subtitles from these folders"),qsTr("Ignore styling in ASS and SSA subtitles"),qsTr("Shadow angle"),qsTr("Text direction"),qsTr("Monospace font"),qsTr("Visualization size")])
    readonly property var focusTargets:[enableSubtitles,preferredLanguage,textEncoding,autoLoad,filenameMatching,fontFamily,sizeMode.relative,sizeMode.pixels,fontSize.control,bold,textColor,textOpacity.control,outlineThickness.control,outlineColor,shadow,shadowColor,shadowDistance.control,background,backgroundColor,backgroundOpacity.control,verticalPosition,subtitleDelay.control,showFeedback,feedbackDuration.control,showFilename,filenameDuration.control,advancedButton,searchFolders,ignoreAssStyling,shadowAngle,textDirection,monospaceFont,visualizationSize]
    implicitHeight:form.implicitHeight
    function tone(value){return appearance?appearance.color(value):value}
    function matches(text){return !searchText.trim().length||text.toLocaleLowerCase().includes(searchText.trim().toLocaleLowerCase())}
    function sectionMatches(names){return names.some(name=>matches(name))}
    function supported(key){return available&&capabilities[key]===true&&key in values}
    function editable(key){return supported(key)&&!busy}
    function write(key,value){return editable(key)&&service.setValue(key,value)}
    function nativeChoices(key){return choices[key]||[]}
    function navigate(item,backwards){if(focusNavigator)focusNavigator(item,backwards)}
    function reveal(item){if(item.activeFocus)focusRevealed(item)}
    function closeChoices(){for(const control of[preferredLanguage,textEncoding,filenameMatching,fontSize,outlineThickness,shadowDistance,background,verticalPosition,textDirection,visualizationSize])if(control.popup.visible)control.popup.close()}
    function focusActivePopup(){closeChoices();return false}
    onVisibleChanged:if(!visible)closeChoices()
    function helpFor(key){
        const group=["textOpacity","backgroundOpacity"].includes(key)?"opacity":["fontFamily","sizeMode","bold","textColor","outlineColor","shadow","shadowColor","shadowDistance","monospaceFont","shadowAngle"].includes(key)?"appearance":key
        const hints={textEncoding:qsTr("Choose a supported text encoding for subtitle files."),autoLoad:qsTr("Look for subtitle files beside each newly opened media file."),filenameMatching:qsTr("Choose this build's matching policy."),showFilename:qsTr("Show the file name when a video starts. The existing On Screen Display master policy must be enabled in Advanced."),filenameDuration:qsTr("Duration of the file name message, in milliseconds.")}
        const timing={restart:qsTr("Restart VLC to apply this default."),nextInput:qsTr("Applies when the next media opens."),nextFeedback:qsTr("Applies to the next shortcut message."),rendererUpdate:qsTr("May update text that is currently being rendered.")}
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
                        source: root.assetBase + "subtitles-settings-1f9fe.svg"
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
                source: root.assetBase + "subtitles-settings-764c9.svg"
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

    component PreferenceInput: C.TextField {
        id: field
        required property string valueKey
        required property string accessibleTitle
        property string kind: "number"
        property string unit: ""
        property int digits: unit === " dB" ? 1 : -1
        property bool userEdited: false
        readonly property var current: root.values[valueKey]
        readonly property var bounds: root.ranges[valueKey]
        anchors.fill: parent
        enabled: root.editable(valueKey)
        text: displayCurrent()
        selectByMouse: true; activeFocusOnTab: true
        font.family: inter.name; font.pixelSize: 14 * root.uiScale
        color: root.tone("#8F8F8F"); selectionColor: root.accentSelected
        leftPadding: 12 * root.uiScale; rightPadding: 18 * root.uiScale
        Accessible.name: accessibleTitle; Accessible.description: root.helpFor(valueKey)
            + (bounds ? " " + qsTr("Supported range: %1 to %2.").arg(bounds.minimum).arg(bounds.maximum) : "")
        function displayCurrent() {
            if (current == null) return ""
            if (typeof current === "number" && digits >= 0 && Math.fround(Number(current.toFixed(digits))) === Math.fround(current)) return current.toFixed(digits) + unit
            return String(current) + unit
        }
        onTextEdited: userEdited = true
        function sync() { text = Qt.binding(displayCurrent); userEdited = false }
        function commit() {
            if (!userEdited) { sync(); return }
            let raw=kind === "text" ? text : text.trim()
            if (unit.length && raw.endsWith(unit)) raw=raw.slice(0,-unit.length).trim()
            const value=kind === "text" ? raw : raw.length && Number.isFinite(Number(raw)) ? Number(raw) : raw
            if (value !== current) {
                root.write(valueKey,value)
            }
            sync()
        }
        onEditingFinished: commit()
        Connections { target: root.service; function onValuesChanged() { field.sync() } }
        onActiveFocusChanged: root.reveal(field)
        Keys.onTabPressed: event => { if (root.focusNavigator) { root.navigate(field,false);event.accepted=true } }
        Keys.onBacktabPressed: event => { if (root.focusNavigator) { root.navigate(field,true);event.accepted=true } }
        background: Rectangle { color: root.tone("#090909");radius:6*root.uiScale;border.width:2*root.uiScale;border.color:field.activeFocus?root.accent:root.tone("#191919") }
    }
    component ColorInput: PreferenceInput {
        id:colorField
        kind:"text";rightPadding:40*root.uiScale
        background:Rectangle{color:root.tone("#090909");radius:6*root.uiScale;border.width:colorField.activeFocus?root.uiScale:0;border.color:root.accent}
        Rectangle{objectName:colorField.objectName+"Swatch";anchors.right:parent.right;anchors.rightMargin:5*root.uiScale;anchors.verticalCenter:parent.verticalCenter;width:22*root.uiScale;height:22*root.uiScale;radius:2*root.uiScale;readonly property string rgb:String(root.values[colorField.valueKey]||"");color:/^#[0-9a-fA-F]{6}$/.test(rgb)?rgb:"transparent";border.width:1.5*root.uiScale;border.color:"#36FFFFFF";Accessible.ignored:true}
    }
    component NumericChoice:Item {
        id:numeric
        required property string valueKey
        required property string accessibleTitle
        property var options:[]
        property alias control:editor
        property alias popup:menu
        property string unit:""
        readonly property int currentIndex:{for(let i=0;i<options.length;++i)if(options[i].value===root.values[valueKey])return i;return -1}
        anchors.fill:parent;activeFocusOnTab:false
        function showMenu(){menu.choiceIndex=currentIndex>=0?currentIndex:0;menu.open()}
        function choose(index){if(index>=0&&index<options.length)root.write(valueKey,options[index].value);menu.close()}
        Keys.onPressed:event=>{if(event.key===Qt.Key_Space||event.key===Qt.Key_Down){showMenu();event.accepted=true}}
        PreferenceInput{
            id:editor;objectName:numeric.objectName+"Input";valueKey:numeric.valueKey;accessibleTitle:numeric.accessibleTitle;unit:numeric.unit
            leftPadding:16*root.uiScale;rightPadding:48*root.uiScale
            background:Rectangle{color:root.tone("#090909");radius:6*root.uiScale;border.width:editor.activeFocus?root.uiScale:0;border.color:root.accent;Rectangle{anchors.fill:parent;anchors.margins:2*root.uiScale;color:root.tone("#191919");radius:4*root.uiScale}}
            Keys.onDownPressed:event=>{numeric.showMenu();event.accepted=true}
            Accessible.description:root.helpFor(valueKey)+qsTr(" Enter a number, or choose a convenience value from the dropdown.")
        }
        C.Button{
            objectName:numeric.objectName+"MenuButton";anchors.right:parent.right;width:48*root.uiScale;height:parent.height;activeFocusOnTab:false
            Accessible.name:qsTr("Choose %1").arg(numeric.accessibleTitle);onClicked:numeric.showMenu();background:Item{}
            contentItem:Item{Image{objectName:numeric.objectName+"OriginalChevron";anchors.centerIn:parent;width:24;height:24;scale:root.uiScale;mirror:true;source:root.assetBase+"subtitles-settings-764c9.svg";Accessible.ignored:true}}
        }
        C.Popup{
            id:menu;objectName:numeric.objectName+"Popup";y:numeric.height+2*root.uiScale;width:numeric.width
            property int choiceIndex:0
            height:Math.min(contentItem.implicitHeight+4*root.uiScale,240*root.uiScale);padding:2*root.uiScale;focus:true
            onOpened:numericList.forceActiveFocus(Qt.PopupFocusReason)
            onClosed:if(root&&root.visible&&editor&&editor.visible&&editor.enabled)editor.forceActiveFocus(Qt.PopupFocusReason)
            contentItem:ListView{
                id:numericList;objectName:numeric.objectName+"Options";focus:true;clip:true;implicitHeight:contentHeight;model:numeric.options;currentIndex:menu.choiceIndex
                delegate:C.ItemDelegate{
                    required property var modelData;required property int index;objectName:numeric.objectName+"Option"+index;width:numeric.width;activeFocusOnTab:false;focusPolicy:Qt.NoFocus
                    highlighted:menu.choiceIndex===index;onClicked:numeric.choose(index)
                    contentItem:SettingLabel{text:modelData.name;verticalAlignment:Text.AlignVCenter}
                    background:Rectangle{color:root.tone(parent.highlighted?"#222222":"#191919")}
                }
                C.ScrollBar.vertical:C.ScrollBar{}
                Keys.onPressed:event=>{
                    if(event.key===Qt.Key_Home)menu.choiceIndex=0
                    else if(event.key===Qt.Key_End)menu.choiceIndex=numeric.options.length-1
                    else if(event.key===Qt.Key_Down)menu.choiceIndex=Math.min(numeric.options.length-1,menu.choiceIndex+1)
                    else if(event.key===Qt.Key_Up)menu.choiceIndex=Math.max(0,menu.choiceIndex-1)
                    else if(event.key===Qt.Key_Return||event.key===Qt.Key_Enter)numeric.choose(menu.choiceIndex)
                    else if(event.key===Qt.Key_Escape)menu.close()
                    else return
                    event.accepted=true
                }
            }
            background:Rectangle{color:root.tone("#191919");border.width:root.uiScale;border.color:root.tone("#404040");radius:4*root.uiScale}
        }
    }
    component UnitButton:SettingButton{
        required property int nativeValue
        readonly property bool selected:root.values.sizeMode===nativeValue
        width:(parent.width-6*root.uiScale)/2;height:32*root.uiScale;y:2*root.uiScale
        enabled:root.editable("sizeMode");Accessible.role:Accessible.RadioButton;Accessible.checkable:true;Accessible.checked:selected;Accessible.name:text;Accessible.description:root.helpFor("sizeMode")
        onClicked:root.write("sizeMode",nativeValue)
        background:Rectangle{radius:4*root.uiScale;color:parent.selected?root.accentSelected:root.tone("#191919");border.width:parent.visualFocus?root.uiScale:0;border.color:root.accent}
        contentItem:SettingLabel{text:parent.text;color:parent.selected?root.accent:root.tone("#6B6B6B");font:parent.font;horizontalAlignment:Text.AlignHCenter;verticalAlignment:Text.AlignVCenter}
    }
    component UnitSelector:Item {
        anchors.fill:parent
        property alias relative:relativeButton
        property alias pixels:pixelButton
        Rectangle{anchors.fill:parent;color:root.tone("#090909");radius:6*root.uiScale}
        UnitButton{id:relativeButton;objectName:"subtitlesSizeModeRelative";x:2*root.uiScale;nativeValue:0;text:qsTr("Relative")}
        UnitButton{id:pixelButton;objectName:"subtitlesSizeModePixels";x:parent.width-width-2*root.uiScale;nativeValue:1;text:qsTr("Pixels")}
    }
    component ValueRange:Item{
        id:range
        required property string valueKey
        required property string accessibleTitle
        property alias control:slider
        readonly property var bounds:root.ranges[valueKey]
        readonly property var current:root.values[valueKey]
        readonly property bool known:typeof current==="number"&&Number.isFinite(current)&&current>=bounds.minimum&&current<=bounds.maximum
        anchors.fill:parent
        C.Slider{
            id:slider;objectName:"subtitles"+range.valueKey
            y:4*root.uiScale;width:parent.width;height:24*root.uiScale
            from:range.bounds.minimum;to:range.bounds.maximum;stepSize:range.bounds.step;padding:0;leftPadding:2*root.uiScale;rightPadding:2*root.uiScale
            value:range.known?range.current:range.bounds.minimum;enabled:root.editable(range.valueKey);activeFocusOnTab:true
            Accessible.name:range.accessibleTitle;Accessible.description:root.helpFor(range.valueKey)+qsTr(" Supported range: %1 to %2.").arg(from).arg(to)
            function sync(){value=range.known?range.current:range.bounds.minimum}
            onMoved:{root.write(range.valueKey,Math.round(value));sync()}
            Connections{target:root.service;function onValuesChanged(){slider.sync()}}
            onActiveFocusChanged:root.reveal(slider)
            Keys.onTabPressed:event=>{if(root.focusNavigator){root.navigate(slider,false);event.accepted=true}}
            Keys.onBacktabPressed:event=>{if(root.focusNavigator){root.navigate(slider,true);event.accepted=true}}
            Keys.onPressed:event=>{if(event.key===Qt.Key_Home||event.key===Qt.Key_End){root.write(range.valueKey,event.key===Qt.Key_Home?from:to);sync();event.accepted=true}}
            C.ToolTip.visible:hovered||visualFocus
            C.ToolTip.text:range.known?String(range.current)+(range.valueKey.includes("Opacity")?"%":" ms"):range.valueKey==="feedbackDuration"&&range.current===-1?qsTr("Native default: text 1 s, widgets 1.2 s"):qsTr("Current value is outside the supported range")
            background:Rectangle{
                y:4*root.uiScale;width:slider.width;height:16*root.uiScale;color:root.tone("#090909");radius:6*root.uiScale
                Rectangle{x:2*root.uiScale;y:2*root.uiScale;width:range.known?(slider.width-4*root.uiScale)*slider.position:0;height:12*root.uiScale;radius:4*root.uiScale;color:root.accentSelected}
                Repeater{model:range.bounds.tickCount;Item{
                    required property int index;x:2*root.uiScale+(slider.width-4*root.uiScale)*index/(range.bounds.tickCount-1);y:5*root.uiScale;width:0;height:6*root.uiScale
                    readonly property bool endpoint:index===0||index===range.bounds.tickCount-1;clip:endpoint
                    Image{objectName:"subtitles"+range.valueKey+"OriginalTick"+index;anchors.centerIn:parent;width:parent.endpoint?32:2;height:parent.endpoint?32:8;scale:root.uiScale;source:root.assetBase+(parent.endpoint?"subtitles-settings-8ba8e.svg":"subtitles-settings-1ab84.svg");Accessible.ignored:true}
                }}
            }
            handle:Rectangle{visible:range.known;x:2*root.uiScale+(slider.width-4*root.uiScale)*slider.position-width/2;y:0;width:8*root.uiScale;height:24*root.uiScale;radius:4*root.uiScale;color:root.accentDim}
        }
        Row{y:38*root.uiScale;width:parent.width
            Repeater{model:range.bounds.labels;SettingLabel{required property string modelData;required property int index;width:range.width/range.bounds.labels.length;text:modelData;font.pixelSize:12*root.uiScale;color:root.tone("#6B6B6B");horizontalAlignment:index===0?Text.AlignLeft:index===range.bounds.labels.length-1?Text.AlignRight:Text.AlignHCenter}}
        }
        SettingLabel{y:59*root.uiScale;visible:!range.known;font.pixelSize:12*root.uiScale;text:range.valueKey==="feedbackDuration"&&range.current===-1?qsTr("Native default"):qsTr("Current value: %1").arg(range.current)}
    }
    Column{
        id:form;width:root.width;spacing:20*root.uiScale
        SettingLabel{objectName:"subtitlesSettingsError";width:parent.width;text:root.service?root.service.lastError:"";visible:text.length>0;color:"#D98779";Accessible.role:Accessible.StaticText;Accessible.name:text}
        SettingLabel{objectName:"subtitlesSettingsRestart";width:parent.width;text:qsTr("Restart VLC to apply changed subtitle defaults.");visible:root.service&&root.service.restartRequired;color:root.tone("#C6C6C6");Accessible.role:Accessible.StaticText;Accessible.name:text}
        Column{width:parent.width
            visible:root.sectionMatches(["Tracks & Languages", "Enable subtitles", "Preferred language", "Text encoding"])
            SectionHeading{text:qsTr("Tracks & Languages")}
            SettingRow{objectName:"subtitlesenableSubtitlesRow";title:qsTr("Enable subtitles");visible:root.matches(title)||root.matches(qsTr("Tracks & Languages"));opacity:root.supported("enableSubtitles")?1:.5
                help:root.helpFor("enableSubtitles");infoName:"subtitlesenableSubtitlesInfo";controlHeight:32*root.uiScale
                Toggle{id:enableSubtitles;objectName:"subtitlesenableSubtitles";valueKey:"enableSubtitles";accessibleTitle:qsTr("Enable subtitles");enabled:root.editable("enableSubtitles")}
            }
            SettingRow{objectName:"subtitlespreferredLanguageRow";title:qsTr("Preferred language");visible:root.matches(title)||root.matches(qsTr("Tracks & Languages"));opacity:root.supported("preferredLanguage")?1:.5
                help:root.helpFor("preferredLanguage");infoName:"subtitlespreferredLanguageInfo";controlHeight:36*root.uiScale
                Choice{id:preferredLanguage;objectName:"subtitlespreferredLanguage";valueKey:"preferredLanguage";accessibleTitle:qsTr("Preferred language");options:root.nativeChoices("preferredLanguage");enabled:root.editable("preferredLanguage")}
            }
            SettingRow{objectName:"subtitlestextEncodingRow";title:qsTr("Text encoding");visible:root.matches(title)||root.matches(qsTr("Tracks & Languages"));opacity:root.supported("textEncoding")?1:.5
                help:root.helpFor("textEncoding");infoName:"subtitlestextEncodingInfo";controlHeight:36*root.uiScale
                Choice{id:textEncoding;objectName:"subtitlestextEncoding";valueKey:"textEncoding";accessibleTitle:qsTr("Text encoding");options:root.nativeChoices("textEncoding");enabled:root.editable("textEncoding")}
            }
        }
        Column{width:parent.width
            visible:root.sectionMatches(["Loading Subtitles", "Load subtitle files automatically", "File name matching"])
            SectionHeading{text:qsTr("Loading Subtitles")}
            SettingRow{objectName:"subtitlesautoLoadRow";title:qsTr("Load subtitle files automatically");visible:root.matches(title)||root.matches(qsTr("Loading Subtitles"));opacity:root.supported("autoLoad")?1:.5
                help:root.helpFor("autoLoad");infoName:"subtitlesautoLoadInfo";controlHeight:32*root.uiScale
                Toggle{id:autoLoad;objectName:"subtitlesautoLoad";valueKey:"autoLoad";accessibleTitle:qsTr("Load subtitle files automatically");enabled:root.editable("autoLoad")}
            }
            SettingRow{objectName:"subtitlesfilenameMatchingRow";title:qsTr("File name matching");visible:root.matches(title)||root.matches(qsTr("Loading Subtitles"));opacity:root.supported("filenameMatching")?1:.5
                help:root.helpFor("filenameMatching");infoName:"subtitlesfilenameMatchingInfo";controlHeight:36*root.uiScale
                Choice{id:filenameMatching;objectName:"subtitlesfilenameMatching";valueKey:"filenameMatching";accessibleTitle:qsTr("File name matching");options:root.nativeChoices("filenameMatching");enabled:root.editable("filenameMatching")}
            }
        }
        Column{width:parent.width
            visible:root.sectionMatches(["Appearance", "Font", "Size values", "Font size", "Bold", "Text color", "Text opacity", "Outline thickness", "Outline color", "Shadow", "Shadow color", "Shadow distance", "Background", "Background color", "Background opacity", "Vertical position"])
            SectionHeading{text:qsTr("Appearance")}
            SettingRow{objectName:"subtitlesfontFamilyRow";title:qsTr("Font");visible:root.matches(title)||root.matches(qsTr("Appearance"));opacity:root.supported("fontFamily")?1:.5
                help:root.helpFor("fontFamily");infoName:"subtitlesfontFamilyInfo";controlHeight:36*root.uiScale
                PreferenceInput{id:fontFamily;objectName:"subtitlesfontFamily";valueKey:"fontFamily";accessibleTitle:qsTr("Font");kind:"text";placeholderText:qsTr("System font")}
            }
            SettingRow{objectName:"subtitlessizeModeRow";title:qsTr("Size values");visible:root.matches(title)||root.matches(qsTr("Appearance"));opacity:root.supported("sizeMode")?1:.5
                help:root.helpFor("sizeMode");infoName:"subtitlessizeModeInfo";controlHeight:36*root.uiScale
                UnitSelector{id:sizeMode;objectName:"subtitlessizeMode"}
            }
            SettingRow{objectName:"subtitlesfontSizeRow";title:qsTr("Font size");visible:root.matches(title)||root.matches(qsTr("Appearance"));opacity:root.supported("fontSize")?1:.5
                help:"";infoName:"subtitlesfontSizeInfo";controlHeight:36*root.uiScale
                NumericChoice{id:fontSize;objectName:"subtitlesfontSize";valueKey:"fontSize";accessibleTitle:qsTr("Font size");options:root.values.sizeMode===1?[{value:12,name:"12 px"},{value:18,name:"18 px"},{value:24,name:"24 px"},{value:36,name:"36 px"}]:[{value:3.125,name:"3.125%"},{value:6.25,name:"6.25%"},{value:12.5,name:"12.5%"}];enabled:root.editable("fontSize")}
            }
            SettingRow{objectName:"subtitlesboldRow";title:qsTr("Bold");visible:root.matches(title)||root.matches(qsTr("Appearance"));opacity:root.supported("bold")?1:.5
                help:"";infoName:"subtitlesboldInfo";controlHeight:32*root.uiScale
                Toggle{id:bold;objectName:"subtitlesbold";valueKey:"bold";accessibleTitle:qsTr("Bold");enabled:root.editable("bold")}
            }
            SettingRow{objectName:"subtitlestextColorRow";title:qsTr("Text color");visible:root.matches(title)||root.matches(qsTr("Appearance"));opacity:root.supported("textColor")?1:.5
                help:"";infoName:"subtitlestextColorInfo";controlHeight:32*root.uiScale
                ColorInput{id:textColor;objectName:"subtitlestextColor";valueKey:"textColor";accessibleTitle:qsTr("Text color")}
            }
            SettingRow{objectName:"subtitlestextOpacityRow";title:qsTr("Text opacity");visible:root.matches(title)||root.matches(qsTr("Appearance"));opacity:root.supported("textOpacity")?1:.5
                help:"";infoName:"subtitlestextOpacityInfo";controlHeight:(textOpacity.known?57:76)*root.uiScale
                ValueRange{id:textOpacity;valueKey:"textOpacity";accessibleTitle:qsTr("Text opacity")}
            }
            SettingRow{objectName:"subtitlesoutlineThicknessRow";title:qsTr("Outline thickness");visible:root.matches(title)||root.matches(qsTr("Appearance"));opacity:root.supported("outlineThickness")?1:.5
                help:"";infoName:"subtitlesoutlineThicknessInfo";controlHeight:36*root.uiScale
                NumericChoice{id:outlineThickness;objectName:"subtitlesoutlineThickness";valueKey:"outlineThickness";accessibleTitle:qsTr("Outline thickness");options:[{value:0,name:qsTr("None (0)")},{value:2,name:qsTr("Thin (2)")},{value:4,name:qsTr("Normal (4)")},{value:6,name:qsTr("Thick (6)")}];enabled:root.editable("outlineThickness")}
            }
            SettingRow{objectName:"subtitlesoutlineColorRow";title:qsTr("Outline color");visible:root.matches(title)||root.matches(qsTr("Appearance"));opacity:root.supported("outlineColor")?1:.5
                help:"";infoName:"subtitlesoutlineColorInfo";controlHeight:32*root.uiScale
                ColorInput{id:outlineColor;objectName:"subtitlesoutlineColor";valueKey:"outlineColor";accessibleTitle:qsTr("Outline color")}
            }
            SettingRow{objectName:"subtitlesshadowRow";title:qsTr("Shadow");visible:root.matches(title)||root.matches(qsTr("Appearance"));opacity:root.supported("shadow")?1:.5
                help:"";infoName:"subtitlesshadowInfo";controlHeight:32*root.uiScale
                Toggle{id:shadow;objectName:"subtitlesshadow";valueKey:"shadow";accessibleTitle:qsTr("Shadow");enabled:root.editable("shadow")}
            }
            SettingRow{objectName:"subtitlesshadowColorRow";title:qsTr("Shadow color");visible:root.matches(title)||root.matches(qsTr("Appearance"));opacity:root.supported("shadowColor")?1:.5
                help:"";infoName:"subtitlesshadowColorInfo";controlHeight:32*root.uiScale
                ColorInput{id:shadowColor;objectName:"subtitlesshadowColor";valueKey:"shadowColor";accessibleTitle:qsTr("Shadow color")}
            }
            SettingRow{objectName:"subtitlesshadowDistanceRow";title:qsTr("Shadow distance");visible:root.matches(title)||root.matches(qsTr("Appearance"));opacity:root.supported("shadowDistance")?1:.5
                help:"";infoName:"subtitlesshadowDistanceInfo";controlHeight:36*root.uiScale
                NumericChoice{id:shadowDistance;objectName:"subtitlesshadowDistance";valueKey:"shadowDistance";accessibleTitle:qsTr("Shadow distance");options:[{value:0,name:"0"},{value:.06,name:"0.06"},{value:.1,name:"0.1"},{value:.5,name:"0.5"},{value:1,name:"1"}];enabled:root.editable("shadowDistance")}
            }
            SettingRow{objectName:"subtitlesbackgroundRow";title:qsTr("Background");visible:root.matches(title)||root.matches(qsTr("Appearance"));opacity:root.supported("background")?1:.5
                help:"";infoName:"subtitlesbackgroundInfo";controlHeight:36*root.uiScale
                Choice{id:background;objectName:"subtitlesbackground";valueKey:"background";accessibleTitle:qsTr("Background");options:root.nativeChoices("background");enabled:root.editable("background")}
            }
            SettingRow{objectName:"subtitlesbackgroundColorRow";title:qsTr("Background color");visible:root.matches(title)||root.matches(qsTr("Appearance"));opacity:root.supported("backgroundColor")?1:.5
                help:"";infoName:"subtitlesbackgroundColorInfo";controlHeight:32*root.uiScale
                ColorInput{id:backgroundColor;objectName:"subtitlesbackgroundColor";valueKey:"backgroundColor";accessibleTitle:qsTr("Background color")}
            }
            SettingRow{objectName:"subtitlesbackgroundOpacityRow";title:qsTr("Background opacity");visible:root.matches(title)||root.matches(qsTr("Appearance"));opacity:root.supported("backgroundOpacity")?1:.5
                help:"";infoName:"subtitlesbackgroundOpacityInfo";controlHeight:(backgroundOpacity.known?57:76)*root.uiScale
                ValueRange{id:backgroundOpacity;valueKey:"backgroundOpacity";accessibleTitle:qsTr("Background opacity")}
            }
            SettingRow{objectName:"subtitlesverticalPositionRow";title:qsTr("Vertical position");visible:root.matches(title)||root.matches(qsTr("Appearance"));opacity:root.supported("verticalPosition")?1:.5
                help:"";infoName:"subtitlesverticalPositionInfo";controlHeight:36*root.uiScale
                Choice{id:verticalPosition;objectName:"subtitlesverticalPosition";valueKey:"verticalPosition";accessibleTitle:qsTr("Vertical position");options:root.nativeChoices("verticalPosition");enabled:root.editable("verticalPosition")}
            }
        }
        Column{width:parent.width
            visible:root.sectionMatches(["Timing", "Subtitle delay"])
            SectionHeading{text:qsTr("Timing")}
            SettingRow{objectName:"subtitlessubtitleDelayRow";title:qsTr("Subtitle delay");visible:root.matches(title)||root.matches(qsTr("Timing"));opacity:root.supported("subtitleDelay")?1:.5
                help:root.helpFor("subtitleDelay");infoName:"subtitlessubtitleDelayInfo";controlHeight:(subtitleDelay.known?57:76)*root.uiScale
                ValueRange{id:subtitleDelay;valueKey:"subtitleDelay";accessibleTitle:qsTr("Subtitle delay")}
            }
        }
        Column{width:parent.width
            visible:root.sectionMatches(["On-Screen Messages", "Show feedback for shortcuts", "Show feedback for", "Show filename when a video starts", "Show filename for"])
            SectionHeading{text:qsTr("On-Screen Messages")}
            SettingRow{objectName:"subtitlesshowFeedbackRow";title:qsTr("Show feedback for shortcuts");visible:root.matches(title)||root.matches(qsTr("On-Screen Messages"));opacity:root.supported("showFeedback")?1:.5
                help:root.helpFor("showFeedback");infoName:"subtitlesshowFeedbackInfo";controlHeight:32*root.uiScale
                Toggle{id:showFeedback;objectName:"subtitlesshowFeedback";valueKey:"showFeedback";accessibleTitle:qsTr("Show feedback for shortcuts");enabled:root.editable("showFeedback")}
            }
            SettingRow{objectName:"subtitlesfeedbackDurationRow";title:qsTr("Show feedback for");visible:root.matches(title)||root.matches(qsTr("On-Screen Messages"));opacity:root.supported("feedbackDuration")?1:.5
                help:"";infoName:"subtitlesfeedbackDurationInfo";controlHeight:(feedbackDuration.known?57:76)*root.uiScale
                ValueRange{id:feedbackDuration;valueKey:"feedbackDuration";accessibleTitle:qsTr("Show feedback for")}
            }
            SettingRow{objectName:"subtitlesshowFilenameRow";title:qsTr("Show filename when a video starts");visible:root.matches(title)||root.matches(qsTr("On-Screen Messages"));opacity:root.supported("showFilename")?1:.5
                help:root.helpFor("showFilename");infoName:"subtitlesshowFilenameInfo";controlHeight:32*root.uiScale
                Toggle{id:showFilename;objectName:"subtitlesshowFilename";valueKey:"showFilename";accessibleTitle:qsTr("Show filename when a video starts");enabled:root.editable("showFilename")}
            }
            SettingRow{objectName:"subtitlesfilenameDurationRow";title:qsTr("Show filename for");visible:root.matches(title)||root.matches(qsTr("On-Screen Messages"));opacity:root.supported("filenameDuration")?1:.5
                help:"";infoName:"subtitlesfilenameDurationInfo";controlHeight:(filenameDuration.known?57:76)*root.uiScale
                ValueRange{id:filenameDuration;valueKey:"filenameDuration";accessibleTitle:qsTr("Show filename for")}
            }
        }
        SettingButton{id:advancedButton;objectName:"subtitlesAdvanced";width:parent.width;height:38*root.uiScale;text:qsTr("Advanced Options");onClicked:root.advancedExpanded=!root.advancedExpanded
            background:Rectangle{color:root.tone("#191919");opacity:.95;radius:2*root.uiScale}
            contentItem:Item{Row{anchors.centerIn:parent;spacing:10*root.uiScale;SettingLabel{text:advancedButton.text;font:advancedButton.font}
                Item{width:24*root.uiScale;height:24*root.uiScale;Image{objectName:"subtitlesAdvancedOriginalChevron";anchors.centerIn:parent;width:24;height:24;scale:root.uiScale;mirror:true;source:root.assetBase+(root.advancedVisible?"subtitles-settings-764c9.svg":"subtitles-settings-97863.svg");Accessible.ignored:true}}}}
        }
        Column{width:parent.width
            visible:root.advancedVisible
            SettingRow{objectName:"subtitlessearchFoldersRow";title:qsTr("Load subtitles from these folders");visible:root.matches(title)||root.matches(qsTr("Advanced Options"));opacity:root.supported("searchFolders")?1:.5
                help:root.helpFor("searchFolders");infoName:"subtitlessearchFoldersInfo";controlHeight:36*root.uiScale
                PreferenceInput{id:searchFolders;objectName:"subtitlessearchFolders";valueKey:"searchFolders";accessibleTitle:qsTr("Load subtitles from these folders");kind:"text"}
            }
            SettingRow{objectName:"subtitlesignoreAssStylingRow";title:qsTr("Ignore styling in ASS and SSA subtitles");visible:root.matches(title)||root.matches(qsTr("Advanced Options"));opacity:root.supported("ignoreAssStyling")?1:.5
                help:root.helpFor("ignoreAssStyling");infoName:"subtitlesignoreAssStylingInfo";controlHeight:32*root.uiScale
                Toggle{id:ignoreAssStyling;objectName:"subtitlesignoreAssStyling";valueKey:"ignoreAssStyling";accessibleTitle:qsTr("Ignore styling in ASS and SSA subtitles");enabled:root.editable("ignoreAssStyling")}
            }
            SettingRow{objectName:"subtitlesshadowAngleRow";title:qsTr("Shadow angle");visible:root.matches(title)||root.matches(qsTr("Advanced Options"));opacity:root.supported("shadowAngle")?1:.5
                help:root.helpFor("shadowAngle");infoName:"subtitlesshadowAngleInfo";controlHeight:36*root.uiScale
                PreferenceInput{id:shadowAngle;objectName:"subtitlesshadowAngle";valueKey:"shadowAngle";accessibleTitle:qsTr("Shadow angle")}
            }
            SettingRow{objectName:"subtitlestextDirectionRow";title:qsTr("Text direction");visible:root.matches(title)||root.matches(qsTr("Advanced Options"));opacity:root.supported("textDirection")?1:.5
                help:root.helpFor("textDirection");infoName:"subtitlestextDirectionInfo";controlHeight:36*root.uiScale
                Choice{id:textDirection;objectName:"subtitlestextDirection";valueKey:"textDirection";accessibleTitle:qsTr("Text direction");options:root.nativeChoices("textDirection");enabled:root.editable("textDirection")}
            }
            SettingRow{objectName:"subtitlesmonospaceFontRow";title:qsTr("Monospace font");visible:root.matches(title)||root.matches(qsTr("Advanced Options"));opacity:root.supported("monospaceFont")?1:.5
                help:root.helpFor("monospaceFont");infoName:"subtitlesmonospaceFontInfo";controlHeight:36*root.uiScale
                PreferenceInput{id:monospaceFont;objectName:"subtitlesmonospaceFont";valueKey:"monospaceFont";accessibleTitle:qsTr("Monospace font");kind:"text";placeholderText:qsTr("System monospace")}
            }
            SettingRow{objectName:"subtitlesvisualizationSizeRow";title:qsTr("Visualization size");visible:root.matches(title)||root.matches(qsTr("Advanced Options"));opacity:root.supported("visualizationSize")?1:.5
                help:root.helpFor("visualizationSize");infoName:"subtitlesvisualizationSizeInfo";controlHeight:36*root.uiScale
                Choice{id:visualizationSize;objectName:"subtitlesvisualizationSize";valueKey:"visualizationSize";accessibleTitle:qsTr("Visualization size");options:root.nativeChoices("visualizationSize");enabled:root.editable("visualizationSize")}
            }
        }
    }
}
