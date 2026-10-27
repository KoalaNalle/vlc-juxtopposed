// SPDX-License-Identifier: GPL-2.0-or-later
// Original visual design and unchanged SVG artwork: Juxtopposed, CC BY 4.0.
import QtQuick
import QtQuick.Controls.Basic as C
FocusScope {
    id:root
    objectName:"juxtopposedLibrarySettings"
    required property var preferences
    property var libraryPreferences:null
    readonly property var service:libraryPreferences
    property var appearance:null
    property real uiScale:1
    property url assetBase:"qrc:/juxtopposed/"
    property string searchText:""
    property var focusNavigator:null
    signal focusRevealed(var item)
    signal advancedRequested()
    readonly property var values:service?service.values:({})
    readonly property var choices:service?service.choices:({})
    readonly property var capabilities:service?service.capabilities:({})
    readonly property bool available:!!service
    readonly property bool busy:false
    readonly property bool stacked:width<650*uiScale
    property string failedKey:""
    readonly property color accent:appearance?appearance.accent:"#D37918"
    readonly property color accentSelected:appearance?appearance.accentSelected:"#371F06"
    readonly property color accentDim:appearance?appearance.accentDim:"#AB6314"
    readonly property var focusTargets:[singleInstanceRow.helpButton,singleInstance,fileManagerSingleInstanceRow.helpButton,fileManagerSingleInstance,enqueueInSingleInstanceRow.helpButton,enqueueInSingleInstance,folderRecursionRow.helpButton,folderRecursion,showHiddenFilesRow.helpButton,showHiddenFiles,metadataSourceRow.helpButton,metadataSource,useMediaLibraryRow.helpButton,useMediaLibrary,advancedButton]
    implicitHeight:form.implicitHeight
    function tone(value){return appearance?appearance.color(value):value}
    function matches(text){return !searchText.trim().length||text.toLocaleLowerCase().includes(searchText.trim().toLocaleLowerCase())}
    function sectionMatches(names){return names.some(name=>matches(name))}
    function supported(key){return available&&capabilities[key]===true&&key in values}
    function editable(key){return supported(key)}
    function nativeChoices(key){return choices[key]||[]}
    function navigate(item,backwards){if(focusNavigator)focusNavigator(item,backwards)}
    function reveal(item){if(item.activeFocus)focusRevealed(item)}
    function write(key,value){if(!editable(key))return false;const accepted=service.setValue(key,value);failedKey=accepted?"":key;return accepted}
    function helpFor(key){
        if(!service)return qsTr("This setting is unavailable in this build.")
        const timing={restart:qsTr("Restart VLC to apply this change."),nextExternalOpen:qsTr("Applies to subsequently opened files in the receiving instance."),nextFolderOpen:qsTr("Applies when the next folder opens.")}
        let result=(service.supportNotes[key]||"")+" "+(timing[service.applyModes[key]]||"")
        if(key==="singleInstance"&&service.legacyFileManagerOnly)result+=" "+qsTr("The saved legacy policy currently applies only to file-manager launches; it has not been replaced.")
        if(key==="useMediaLibrary")result+=" "+(service.libraryAvailable?qsTr("The media library is currently active in this session."):qsTr("The media library is currently inactive in this session."))
        if (!supported(key)) {
            const needsSingleInstance = key === "fileManagerSingleInstance" || key === "enqueueInSingleInstance"
            result += " " + (needsSingleInstance && values.singleInstance !== true
                ? qsTr("Turn on ‘Only allow one VLC window’ to change this setting.")
                : qsTr("This setting is unavailable in this build."))
        }
        return result
    }
    function closeChoices(){folderRecursion.popup.close();metadataSource.popup.close();for(const toggle of[singleInstance,fileManagerSingleInstance,enqueueInSingleInstance,showHiddenFiles,useMediaLibrary])toggle.customPopup.close();for(const row of[singleInstanceRow,fileManagerSingleInstanceRow,enqueueInSingleInstanceRow,folderRecursionRow,showHiddenFilesRow,metadataSourceRow,useMediaLibraryRow])row.helpButton.tipOpen=false}
    function focusActivePopup(){closeChoices();return false}
    onVisibleChanged:if(!visible)closeChoices()
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
                        source: root.assetBase + "audio-settings-1f9fe.svg"
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
    component Choice: C.ComboBox {
        id: choice
        FeedbackTip{control:choice;valueKey:choice.valueKey}
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
        Accessible.description: (currentIndex < 0 ? qsTr("The current setting has no matching preset. Choose a value to replace it.") + " " : "") + root.helpFor(valueKey)+(root.failedKey===valueKey&&root.service?" "+root.service.lastError:"")
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
        Accessible.description: (custom?qsTr("Custom saved opening behavior. Choose On or Off to replace it.")+" ":"")+root.helpFor(valueKey)+(root.failedKey===valueKey&&root.service?" "+root.service.lastError:"")
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
    Column {
        id:form;width:root.width;spacing:20*root.uiScale
        SettingLabel{objectName:"libraryError";width:parent.width;text:root.service?root.service.lastError:"";visible:text.length>0;color:"#D98779";Accessible.name:text}
        SettingLabel{objectName:"libraryRestart";width:parent.width;text:qsTr("Restart VLC to apply the changed startup defaults.");visible:root.service&&root.service.restartRequired;Accessible.name:text}
        Column {width:parent.width;visible:root.sectionMatches([qsTr("Opening Files"), qsTr("Only allow one VLC window"), qsTr("Apply to files opened from the file manager"), qsTr("Add to the playlist instead of replacing it"), qsTr("When opening a folder"), qsTr("Show hidden files")])
            SectionHeading{text:qsTr("Opening Files")}
            SettingRow {
                id:singleInstanceRow;objectName:"librarysingleInstanceRow";title:qsTr("Only allow one VLC window");help:root.helpFor("singleInstance");infoName:"librarysingleInstanceInfo"
                visible:root.matches(title)||root.matches(qsTr("Opening Files"))
                opacity:root.supported("singleInstance")?1:.5
                controlHeight:32*root.uiScale
                Toggle {id:singleInstance;objectName:"librarysingleInstance";valueKey:"singleInstance";accessibleTitle:qsTr("Only allow one VLC window");enabled:root.editable(valueKey)}
            }
            SettingRow {
                id:fileManagerSingleInstanceRow;objectName:"libraryfileManagerSingleInstanceRow";title:qsTr("Apply to files opened from the file manager");help:root.helpFor("fileManagerSingleInstance");infoName:"libraryfileManagerSingleInstanceInfo"
                visible:root.matches(title)||root.matches(qsTr("Opening Files"))
                opacity:root.values.singleInstance!==true?.3:root.supported("fileManagerSingleInstance")?1:.5
                controlHeight:32*root.uiScale
                Toggle {id:fileManagerSingleInstance;objectName:"libraryfileManagerSingleInstance";valueKey:"fileManagerSingleInstance";accessibleTitle:qsTr("Apply to files opened from the file manager");enabled:root.editable(valueKey)}
            }
            SettingRow {
                id:enqueueInSingleInstanceRow;objectName:"libraryenqueueInSingleInstanceRow";title:qsTr("Add to the playlist instead of replacing it");help:root.helpFor("enqueueInSingleInstance");infoName:"libraryenqueueInSingleInstanceInfo"
                visible:root.matches(title)||root.matches(qsTr("Opening Files"))
                opacity:root.values.singleInstance!==true?.3:root.supported("enqueueInSingleInstance")?1:.5
                controlHeight:32*root.uiScale
                Toggle {id:enqueueInSingleInstance;objectName:"libraryenqueueInSingleInstance";valueKey:"enqueueInSingleInstance";accessibleTitle:qsTr("Add to the playlist instead of replacing it");enabled:root.editable(valueKey)}
            }
            SettingRow {
                id:folderRecursionRow;objectName:"libraryfolderRecursionRow";title:qsTr("When opening a folder");help:root.helpFor("folderRecursion");infoName:"libraryfolderRecursionInfo"
                visible:root.matches(title)||root.matches(qsTr("Opening Files"))
                opacity:root.supported("folderRecursion")?1:.5
                controlHeight:36*root.uiScale
                Choice {id:folderRecursion;objectName:"libraryfolderRecursion";valueKey:"folderRecursion";accessibleTitle:qsTr("When opening a folder");enabled:root.editable(valueKey);options:root.nativeChoices(valueKey)}
            }
            SettingRow {
                id:showHiddenFilesRow;objectName:"libraryshowHiddenFilesRow";title:qsTr("Show hidden files");help:root.helpFor("showHiddenFiles");infoName:"libraryshowHiddenFilesInfo"
                visible:root.matches(title)||root.matches(qsTr("Opening Files"))
                opacity:root.supported("showHiddenFiles")?1:.5
                controlHeight:32*root.uiScale
                Toggle {id:showHiddenFiles;objectName:"libraryshowHiddenFiles";valueKey:"showHiddenFiles";accessibleTitle:qsTr("Show hidden files");enabled:root.editable(valueKey)}
            }
        }
        Column {width:parent.width;visible:root.sectionMatches([qsTr("Metadata & Artwork"), qsTr("Fetch titles and artwork"), qsTr("Use the media library")])
            SectionHeading{text:qsTr("Metadata & Artwork")}
            SettingRow {
                id:metadataSourceRow;objectName:"librarymetadataSourceRow";title:qsTr("Fetch titles and artwork");help:root.helpFor("metadataSource");infoName:"librarymetadataSourceInfo"
                visible:root.matches(title)||root.matches(qsTr("Metadata & Artwork"))
                opacity:root.supported("metadataSource")?1:.5
                controlHeight:36*root.uiScale
                Choice {id:metadataSource;objectName:"librarymetadataSource";valueKey:"metadataSource";accessibleTitle:qsTr("Fetch titles and artwork");enabled:root.editable(valueKey);options:root.nativeChoices(valueKey)}
            }
            SettingRow {
                id:useMediaLibraryRow;objectName:"libraryuseMediaLibraryRow";title:qsTr("Use the media library");help:root.helpFor("useMediaLibrary");infoName:"libraryuseMediaLibraryInfo"
                visible:root.matches(title)||root.matches(qsTr("Metadata & Artwork"))
                opacity:root.supported("useMediaLibrary")?1:.5
                controlHeight:32*root.uiScale
                Toggle {id:useMediaLibrary;objectName:"libraryuseMediaLibrary";valueKey:"useMediaLibrary";accessibleTitle:qsTr("Use the media library");enabled:root.editable(valueKey)}
            }
        }
        SettingButton {
            id:advancedButton;objectName:"libraryAdvanced";width:parent.width;height:38*root.uiScale;text:qsTr("Advanced Options")
            onClicked:root.advancedRequested()
            background:Rectangle{color:root.tone("#191919");opacity:.95;radius:2*root.uiScale;border.width:advancedButton.visualFocus?root.uiScale:0;border.color:root.accent}
            contentItem:Item {
                Row{anchors.centerIn:parent;anchors.alignWhenCentered:false;spacing:10*root.uiScale
                    SettingLabel{text:advancedButton.text;anchors.verticalCenter:parent.verticalCenter}
                    Item{width:24*root.uiScale;height:24*root.uiScale;Image{objectName:"libraryAdvancedOriginalChevron";anchors.centerIn:parent;width:24;height:24;scale:root.uiScale;mirror:true;source:root.assetBase+"audio-settings-97863.svg";Accessible.ignored:true}}
                }
            }
        }
    }
    Connections {target:form;function onImplicitHeightChanged(){Qt.callLater(()=>{const focused=root.focusTargets.find(item=>item&&item.activeFocus);if(focused)root.reveal(focused)})}}
}
