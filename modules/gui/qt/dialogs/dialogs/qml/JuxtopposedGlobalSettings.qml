// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
// Original visual design and SVG artwork: Juxtopposed, CC BY 4.0.
import QtQuick
import QtQuick.Controls.Basic as C
import QtQuick.Layouts

C.Dialog {
    id: root
    objectName: "juxtopposedGlobalSettings"
    required property var preferences
    property var appearance: null
    property real uiScale: 1
    property url assetBase: "qrc:/juxtopposed/"
    // Real category bodies are supplied as Components. This shell does not
    // claim unsupported categories or silently fall back to legacy dialogs.
    property var categoryPages: ({})
    property var categorySearchTerms: ({})
    property Item backdropSource: null
    property Item focusReturnItem: null
    property string selectedCategory: "general"
    readonly property alias searchText: searchField.text
    readonly property var categories: [
        {key: "general", title: qsTr("General"), group: qsTr("App"), icon: "global-settings-66cd9.svg", selectedIcon: "global-settings-4dae7.svg", mirror: false},
        {key: "interface", title: qsTr("Interface"), group: qsTr("App"), icon: "global-settings-1bf8a.svg", selectedIcon: "global-settings-f1a4a.svg", mirror: false},
        {key: "privacy", title: qsTr("Privacy"), group: qsTr("App"), icon: "global-settings-7d179.svg", mirror: false},
        {key: "controls", title: qsTr("Controls"), group: qsTr("App"), icon: "global-settings-36bfa.svg", mirror: false},
        {key: "playback", title: qsTr("Playback"), group: qsTr("Media"), icon: "global-settings-ad68b.svg", mirror: false},
        {key: "video", title: qsTr("Video"), group: qsTr("Media"), icon: "global-settings-ec3f6.svg", mirror: true},
        {key: "audio", title: qsTr("Audio"), group: qsTr("Media"), icon: "audio.svg", mirror: false},
        {key: "subtitles", title: qsTr("Subtitles"), group: qsTr("Media"), icon: "subtitles.svg", mirror: false},
        {key: "library", title: qsTr("Library"), group: qsTr("Media"), icon: "global-settings-cf6e1.svg", mirror: true},
        {key: "network", title: qsTr("Network"), group: qsTr("System"), icon: "global-settings-6db55.svg", mirror: true},
        {key: "performance", title: qsTr("Performance"), group: qsTr("System"), icon: "global-settings-66cd9.svg", mirror: false},
        {key: "recording", title: qsTr("Recording"), group: qsTr("System"), icon: "global-settings-08f95.svg", mirror: false},
        {key: "extensions", title: qsTr("Extensions"), group: qsTr("System"), icon: "global-settings-05bc9.svg", mirror: false}
    ]
    readonly property var generalSearchTerms: [qsTr("General"), qsTr("Appearance"), qsTr("Interface language"), qsTr("Theme"), qsTr("Accent color"), qsTr("Startup"), qsTr("Show confirmation and error dialogs"), qsTr("Check for updates automatically"), qsTr("Diagnostics"), qsTr("Developer mode"), qsTr("Save a log"), qsTr("Log file"), qsTr("Log details"), qsTr("Advanced Options")]
    readonly property color accent: appearance ? appearance.accent : "#D37918"
    signal advancedRequested()
    signal categoryRequested(string category)
    signal fallbackFocusRequested()
    modal: true
    focus: true
    anchors.centerIn: parent
    title: qsTr("Global Settings")
    implicitWidth: 1048 * uiScale
    implicitHeight: 865 * uiScale
    width: Math.min(implicitWidth, Math.max(0, parent.width - 32 * uiScale))
    height: Math.min(implicitHeight, Math.max(0, parent.height - 32 * uiScale))
    padding: 0
    closePolicy: C.Popup.CloseOnEscape

    function tone(value) { return appearance ? appearance.color(value) : value }
    function hasPage(key) { return key === "general" || !!categoryPages[key] }
    function categoryMatches(category) {
        const query = searchField.text.trim().toLocaleLowerCase()
        if (!query.length) return true
        if (!hasPage(category.key)) return false
        const words = category.key === "general" ? generalSearchTerms : [category.title].concat(categorySearchTerms[category.key] || [])
        return words.some(word => word.toLocaleLowerCase().includes(query))
    }
    function chooseCategory(key) {
        if (!hasPage(key)) return false
        selectedCategory = key
        pageScroll.contentY = 0
        categoryRequested(key)
        return true
    }
    function showGroupHeading(index) {
        const category = categories[index]
        for (let previous = index - 1; previous >= 0 && categories[previous].group === category.group; --previous)
            if (categoryMatches(categories[previous])) return false
        return true
    }
    function showDialog(returnItem) {
        if (!visible) {
            focusReturnItem = returnItem
            if (preferences) preferences.reload()
            open()
        }
        searchField.forceActiveFocus(Qt.PopupFocusReason)
    }
    function focusItems() {
        const items = [searchField]
        for (let i = 0; i < categoriesRepeater.count; ++i) items.push(categoriesRepeater.itemAt(i).categoryButton)
        items.push(closeButton)
        if (pageLoader.item && "focusTargets" in pageLoader.item) items.push(...pageLoader.item.focusTargets)
        return items.filter(item => item && item.visible && item.enabled)
    }
    function navigateFocus(item, backwards) {
        const items = focusItems()
        if (!items.length) return
        let index = items.indexOf(item)
        if (index < 0) index = backwards ? 0 : -1
        const next = items[(index + (backwards ? items.length - 1 : 1)) % items.length]
        next.forceActiveFocus(backwards ? Qt.BacktabFocusReason : Qt.TabFocusReason)
    }
    function reveal(item) {
        if (!pageScroll.contentItem || !item) return
        const point = item.mapToItem(pageScroll.contentItem, 0, 0)
        if (point.y < pageScroll.contentY) pageScroll.contentY = Math.max(0, point.y - 5 * uiScale)
        else if (point.y + item.height > pageScroll.contentY + pageScroll.height)
            pageScroll.contentY = Math.min(Math.max(0, pageScroll.contentHeight - pageScroll.height), point.y + item.height - pageScroll.height + 5 * uiScale)
    }
    function configurePage() {
        const page = pageLoader.item
        if (!page) return
        const assignments = {preferences: () => root.preferences, appearance: () => root.appearance,
            uiScale: () => root.uiScale, assetBase: () => root.assetBase,
            searchText: () => {
                const category = root.categories.find(category => category.key === root.selectedCategory)
                const query = searchField.text.trim().toLocaleLowerCase()
                return category && query.length && category.title.toLocaleLowerCase().includes(query) ? "" : searchField.text
            }, focusNavigator: () => root.navigateFocus}
        for (const name in assignments) if (name in page) page[name] = Qt.binding(assignments[name])
    }
    onClosed: {
        if (focusReturnItem && focusReturnItem.visible && focusReturnItem.enabled) focusReturnItem.forceActiveFocus(Qt.PopupFocusReason)
        else fallbackFocusRequested()
        focusReturnItem = null
    }
    onCategoryPagesChanged: if (!hasPage(selectedCategory)) selectedCategory = "general"
    FontLoader { id: inter; source: root.assetBase + "InterVariable.ttf" }
    background: JuxtopposedSettingsBackdrop {
        sourceItem: root.backdropSource
        active: root.visible
        color: root.tone("#0E0E0E")
        uiScale: root.uiScale
    }
    C.Overlay.modal: Rectangle { color: "#80000000" }

    header: Item {
        implicitHeight: 50 * root.uiScale
        Rectangle { anchors.fill: parent; color: root.tone("#090909"); topLeftRadius: 14 * root.uiScale; topRightRadius: 14 * root.uiScale }
        C.TextField {
            id: searchField
            objectName: "globalSettingsSearch"
            x: 8 * root.uiScale; y: 8 * root.uiScale
            width: Math.min(200 * root.uiScale, Math.max(0, parent.width - 80 * root.uiScale)); height: 34 * root.uiScale
            activeFocusOnTab: true; selectByMouse: true
            placeholderText: qsTr("Search")
            placeholderTextColor: root.tone("#404040"); color: root.tone("#8F8F8F"); selectionColor: root.appearance ? root.appearance.accentSelected : "#371F06"
            font.family: inter.name; font.pixelSize: 14 * root.uiScale
            leftPadding: 42 * root.uiScale; rightPadding: 8 * root.uiScale
            Accessible.name: qsTr("Search global settings")
            background: Rectangle { color: root.tone("#151515"); radius: 6 * root.uiScale; border.width: searchField.visualFocus ? root.uiScale : 0; border.color: root.accent }
            Image { objectName: "globalSettingsOriginalSearch"; x: 8 * root.uiScale + (24 * root.uiScale - width) / 2; anchors.verticalCenter: parent.verticalCenter; width: 24; height: 24; scale: root.uiScale; source: root.assetBase + "global-settings-80d5a.svg"; Accessible.ignored: true }
            Keys.onTabPressed: { root.navigateFocus(searchField, false) }
            Keys.onBacktabPressed: { root.navigateFocus(searchField, true) }
            onTextEdited: {
                if (!root.categories.some(category => category.key === root.selectedCategory && root.categoryMatches(category))) {
                    const first = root.categories.find(category => root.categoryMatches(category) && root.hasPage(category.key))
                    if (first) root.chooseCategory(first.key)
                }
                pageScroll.contentY = 0
            }
        }
        Text {
            objectName: "globalSettingsTitle"
            x: searchField.x + searchField.width + 32 * root.uiScale
            anchors.verticalCenter: parent.verticalCenter
            width: Math.max(0, closeButton.x - x - 8 * root.uiScale)
            text: root.title; color: root.tone("#969696")
            font.family: inter.name; font.pixelSize: 16 * root.uiScale
            elide: Text.ElideRight; textFormat: Text.PlainText
        }
        C.Button {
            id: closeButton
            objectName: "globalSettingsClose"
            anchors.right: parent.right; anchors.rightMargin: 14 * root.uiScale; anchors.verticalCenter: parent.verticalCenter
            width: 32 * root.uiScale; height: 32 * root.uiScale
            padding: 0; activeFocusOnTab: true
            Accessible.name: qsTr("Close global settings")
            background: Rectangle { color: closeButton.hovered ? root.tone("#191919") : "transparent"; radius: 4 * root.uiScale; border.width: closeButton.visualFocus ? root.uiScale : 0; border.color: root.accent }
            contentItem: Item { Image { objectName: "globalSettingsOriginalClose"; anchors.centerIn: parent; width: 24; height: 24; scale: root.uiScale; mirror: true; source: root.assetBase + "new-playlist-close.svg"; Accessible.ignored: true } }
            Keys.onTabPressed: root.navigateFocus(closeButton, false)
            Keys.onBacktabPressed: root.navigateFocus(closeButton, true)
            onClicked: root.close()
        }
    }
    contentItem: Row {
        C.ScrollView {
            id: sidebarScroll
            objectName: "globalSettingsCategoriesScroll"
            width: Math.min(216 * root.uiScale, Math.max(104 * root.uiScale, root.width * .3))
            height: parent.height
            clip: true
            C.ScrollBar.horizontal.policy: C.ScrollBar.AlwaysOff
            C.ScrollBar.vertical.policy: C.ScrollBar.AsNeeded
            contentWidth: availableWidth
            contentHeight: Math.max(availableHeight, categoriesColumn.implicitHeight + 28 * root.uiScale)
            background: Rectangle { color: root.tone("#0E0E0E"); opacity: .95; bottomLeftRadius: 14 * root.uiScale }
            Rectangle {
                x: 8 * root.uiScale; y: 8 * root.uiScale
                width: sidebarScroll.availableWidth - 16 * root.uiScale
                height: Math.max(sidebarScroll.availableHeight - 16 * root.uiScale, categoriesColumn.implicitHeight + 12 * root.uiScale)
                color: root.tone("#191919"); opacity: .95; radius: 6 * root.uiScale
                Column {
                    id: categoriesColumn
                    anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 6 * root.uiScale
                    spacing: 8 * root.uiScale
                    Repeater {
                        id: categoriesRepeater
                        model: root.categories
                        delegate: Column {
                            id: entry
                            required property var modelData
                            required property int index
                            property alias categoryButton: categoryButton
                            width: categoriesColumn.width
                            visible: root.categoryMatches(modelData)
                            spacing: 8 * root.uiScale
                            Text {
                                visible: root.showGroupHeading(index)
                                width: parent.width; height: 33 * root.uiScale
                                leftPadding: 10 * root.uiScale
                                text: modelData.group; color: root.tone("#6B6B6B")
                                font.family: inter.name; font.pixelSize: 14 * root.uiScale
                                verticalAlignment: Text.AlignVCenter
                            }
                            C.Button {
                                id: categoryButton
                                objectName: "globalCategory" + modelData.key
                                width: parent.width; height: 40 * root.uiScale
                                property bool selected: modelData.key === root.selectedCategory
                                activeFocusOnTab: true
                                enabled: root.hasPage(modelData.key)
                                padding: 4 * root.uiScale
                                Accessible.name: modelData.title
                                Accessible.role: Accessible.PageTab
                                Accessible.selected: selected
                                background: Rectangle { color: categoryButton.selected ? root.tone("#222222") : "transparent"; radius: 2 * root.uiScale; border.width: categoryButton.visualFocus ? root.uiScale : 0; border.color: root.accent }
                                contentItem: Row {
                                    spacing: 8 * root.uiScale
                                    Item {
                                        objectName: "globalCategory" + modelData.key + "IconSlot"
                                        width: 32 * root.uiScale; height: 32 * root.uiScale
                                        Image {
                                            objectName: "globalCategory" + modelData.key + "OriginalIcon"
                                            anchors.centerIn: parent; width: 24; height: 24; scale: root.uiScale
                                            source: root.assetBase + (categoryButton.selected && modelData.selectedIcon ? modelData.selectedIcon : modelData.icon)
                                            mirror: modelData.mirror
                                            Accessible.ignored: true
                                        }
                                    }
                                    Text {
                                        height: parent.height; width: Math.max(0, parent.width - 40 * root.uiScale)
                                        text: modelData.title
                                        color: root.tone(categoryButton.selected ? "#EAEAEA" : "#8F8F8F")
                                        font.family: inter.name; font.pixelSize: 14 * root.uiScale
                                        verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight
                                    }
                                }
                                onClicked: root.chooseCategory(modelData.key)
                                Keys.onTabPressed: root.navigateFocus(categoryButton, false)
                                Keys.onBacktabPressed: root.navigateFocus(categoryButton, true)
                                Keys.onDownPressed: root.navigateFocus(categoryButton, false)
                                Keys.onUpPressed: root.navigateFocus(categoryButton, true)
                                onActiveFocusChanged: if (activeFocus) {
                                    const point = entry.mapToItem(sidebarScroll.contentItem.contentItem, 0, 0)
                                    const flick = sidebarScroll.contentItem
                                    if (point.y < flick.contentY) flick.contentY = Math.max(0, point.y)
                                    else if (point.y + entry.height > flick.contentY + flick.height) flick.contentY = Math.min(flick.contentHeight - flick.height, point.y + entry.height - flick.height)
                                }
                            }
                        }
                    }
                }
            }
        }
        Flickable {
            id: pageScroll
            objectName: "globalSettingsPageScroll"
            width: parent.width - sidebarScroll.width
            height: parent.height
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            contentWidth: width
            contentHeight: Math.max(height, pageColumn.implicitHeight + 120 * root.uiScale)
            C.ScrollBar.vertical: C.ScrollBar { policy: C.ScrollBar.AsNeeded }
            Column {
                id: pageColumn
                x: 24 * root.uiScale; y: 20 * root.uiScale
                width: Math.max(0, pageScroll.width - 48 * root.uiScale)
                spacing: 10 * root.uiScale
                Text {
                    objectName: "globalSettingsError"
                    width: parent.width; visible: text.length > 0
                    text: root.preferences ? root.preferences.lastError : ""
                    color: "#D98779"; wrapMode: Text.WordWrap; textFormat: Text.PlainText
                    font.family: inter.name; font.pixelSize: 14 * root.uiScale
                    Accessible.role: Accessible.StaticText; Accessible.name: text
                }
                Text {
                    objectName: "globalSettingsRestart"
                    width: parent.width; visible: root.preferences && root.preferences.restartRequired
                    text: qsTr("Restart VLC to apply these changes.")
                    color: root.tone("#C6C6C6"); wrapMode: Text.WordWrap
                    font.family: inter.name; font.pixelSize: 14 * root.uiScale
                    Accessible.role: Accessible.StaticText; Accessible.name: text
                }
                Text {
                    objectName: "globalSettingsNoResults"
                    width: parent.width
                    visible: !root.categories.some(category => root.hasPage(category.key) && root.categoryMatches(category))
                    text: qsTr("No matching settings.")
                    color: root.tone("#8F8F8F"); wrapMode: Text.WordWrap
                    font.family: inter.name; font.pixelSize: 14 * root.uiScale
                }
                Item {
                    id: pageLoader
                    objectName: "globalSettingsPageLoader"
                    property var item: null
                    property bool ready: false
                    property Component pageComponent: root.selectedCategory === "general" ? generalPage : root.categoryPages[root.selectedCategory] || null
                    width: parent.width
                    implicitHeight: item ? item.implicitHeight : 0
                    visible: !root.categories.some(category => category.key === root.selectedCategory && !root.categoryMatches(category))
                    function rebuild() {
                        if (!ready) return
                        if (item) item.destroy()
                        item = pageComponent ? pageComponent.createObject(pageLoader, {
                            preferences: root.preferences, appearance: root.appearance,
                            uiScale: root.uiScale, assetBase: root.assetBase,
                            searchText: searchField.text, focusNavigator: root.navigateFocus
                        }) : null
                        if (item) {
                            item.width = Qt.binding(() => pageLoader.width)
                            root.configurePage()
                        }
                    }
                    onPageComponentChanged: rebuild()
                    Component.onCompleted: { ready = true; rebuild() }
                }
            }
        }
    }
    Component {
        id: generalPage
        JuxtopposedGeneralSettings {
            onAdvancedRequested: { root.close(); root.advancedRequested() }
        }
    }
    Connections {
        target: pageLoader.item
        ignoreUnknownSignals: true
        function onFocusRevealed(item) { root.reveal(item) }
    }
}
