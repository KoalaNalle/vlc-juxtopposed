// SPDX-License-Identifier: GPL-2.0-or-later
// Original two-page onboarding: Juxtopposed, CC BY 4.0, 1902:3670 / 1902:6770.
import QtQuick
import QtQuick.Window
import QtQuick.Controls.Basic as C
import VLC.Style

JuxtopposedMovableDialog {
    id: root
    objectName: "juxtopposedSetupDialog"
    property var preferences: null
    property var activeShortcuts: ({})
    property var interfaceShortcuts: ({})
    property var previewNativePalette: null
    property var previewViewColors: null
    property var previewWindowColors: null
    property real uiScale: 1
    property url assetBase: "qrc:/juxtopposed/"
    property int draftTheme: 2
    property int draftAccent: 0
    property int pageIndex: 0
    property bool saving: false
    property Item focusReturnItem: null
    readonly property bool busy: saving
    readonly property bool available: !!preferences && typeof preferences.finishSetup === "function"
    readonly property bool compact: width < 1089 * uiScale
    readonly property bool tourTwoColumns: width >= 900 * uiScale
    readonly property real footerHeight: (compact ? 64 : 104) * uiScale
    readonly property string saveError: available ? (preferences.lastError || "") : qsTr("Preferences are unavailable.")
    signal fallbackFocusRequested()
    modal: true
    focus: true
    padding: 0
    header: null
    footer: null
    implicitWidth: 1090 * uiScale
    implicitHeight: 759 * uiScale
    width: Math.min(implicitWidth, Math.max(0, parent.width - 32 * uiScale))
    height: Math.min(implicitHeight, Math.max(0, parent.height - 32 * uiScale))
    closePolicy: saving ? C.Popup.NoAutoClose : C.Popup.CloseOnEscape
    title: pageIndex === 0 ? qsTr("Select your preferred theme and color.") : qsTr("Learn the basic keyboard shortcuts.")

    QtObject {
        id: previewAppearance
        property var nativeView: root.previewViewColors
        property var nativeWindow: root.previewWindowColors
        readonly property bool dark: root.draftTheme === 2 || (root.draftTheme === 0
            && (root.previewNativePalette ? root.previewNativePalette.isDark : true))
        readonly property int accentIndex: root.draftAccent
        readonly property color accent: JuxtopposedPalette.accentFor(accentIndex, dark, nativeView)
        readonly property color accentSelected: JuxtopposedPalette.selectedAccentFor(accentIndex, dark, accent)
        readonly property color accentDim: JuxtopposedPalette.dimAccentFor(accentIndex, dark, accent)
        readonly property color playButtonAccent: accentIndex === 0 ? "#D37918" : accent
        function color(value) { return JuxtopposedPalette.color(value, previewAppearance) }
        function icon(name) { return JuxtopposedPalette.icon(name, previewAppearance) }
    }
    function tone(value) { return previewAppearance.color(value) }
    function accentText() {
        const c = previewAppearance.accent
        function linear(v) { return v <= 0.04045 ? v / 12.92 : Math.pow((v + 0.055) / 1.055, 2.4) }
        const luminance = 0.2126 * linear(c.r) + 0.7152 * linear(c.g) + 0.0722 * linear(c.b)
        return (luminance + 0.05) / 0.05 >= 1.05 / (luminance + 0.05) ? "#0F0F0F" : "#FFFFFF"
    }
    function showDialog(returnItem) {
        if (visible) return
        focusReturnItem = returnItem
        if (available) preferences.reload()
        const values = preferences?.values || {}
        draftTheme = [0, 1, 2].includes(values.theme) ? values.theme : 2
        draftAccent = values.accentIndex >= 0 && values.accentIndex <= 8 ? values.accentIndex : 0
        pageIndex = 0
        body.contentY = 0
        open()
        Qt.callLater(() => focusTheme([1, 2, 0].indexOf(draftTheme)))
    }
    function finish() {
        if (!available || saving) return
        saving = true
        let saved = false
        try { saved = preferences.finishSetup(draftTheme, draftAccent) }
        finally { saving = false }
        if (saved) accept()
        else nextButton.forceActiveFocus(Qt.TabFocusReason)
    }
    function reveal(item) {
        if (!item || !item.activeFocus || !visible) return
        let owner = item
        while (owner && owner !== body.contentItem) owner = owner.parent
        if (!owner) return
        const point = item.mapToItem(body.contentItem, 0, 0)
        if (point.y < body.contentY) body.contentY = Math.max(0, point.y)
        else if (point.y + item.height > body.contentY + body.height)
            body.contentY = Math.min(Math.max(0, body.contentHeight - body.height), point.y + item.height - body.height)
    }
    function focusTheme(index) {
        const button = themes.itemAt((index + 3) % 3)
        if (button) button.forceActiveFocus(Qt.TabFocusReason)
    }
    function focusAccent(index) {
        const button = (compact ? compactAccents : desktopAccents).itemAt((index + 9) % 9)
        if (button) button.forceActiveFocus(Qt.TabFocusReason)
    }
    function bindingFor(action) {
        return action[0] === "@" ? (interfaceShortcuts[action.slice(1)] || "") : (activeShortcuts[action] || "")
    }
    function bindingTokens(action) {
        const names = {Left: "←", Right: "→", Up: "↑", Down: "↓", Shift: "SHIFT", Ctrl: "CTRL", Alt: "ALT", Escape: "Esc"}
        const binding = bindingFor(action).split("\t").find(value => value.length) || ""
        const parts = binding ? binding.split("+") : [qsTr("Unassigned")]
        if (binding.endsWith("+")) {
            while (parts.length && !parts[parts.length - 1]) parts.pop()
            parts.push("+")
        }
        return parts.map(token => ({label: names[token] || (token.length === 1 ? token.toUpperCase() : token), separator: false}))
    }
    function keyTokens(actions) {
        const tokens = []
        for (let i = 0; i < actions.length; ++i) {
            if (i) tokens.push({label: "/", separator: true})
            tokens.push(...bindingTokens(actions[i]))
        }
        return tokens
    }
    function keyWidth(token, maxWidth) {
        if (token.separator) return 6 * uiScale
        const pad = /[←→↑↓<>,.]/.test(token.label) ? 30 : token.label.length === 1 || token.label === "Space" ? 46 : 32
        return Math.min(maxWidth, Math.max(35 * uiScale, keyMetrics.advanceWidth(token.label) + pad * uiScale))
    }
    function keyGroups(actions, maxWidth) {
        const groups = actions.map(action => {
            const tokens = bindingTokens(action)
            const width = tokens.reduce((sum, token) => sum + keyWidth(token, maxWidth), 0)
                + Math.max(0, tokens.length - 1) * 10 * uiScale
            return {tokens: tokens, width: Math.min(maxWidth, width)}
        })
        // Keep complete chords together. Show a slash only when both fit on one line.
        const inline = groups.reduce((sum, group) => sum + group.width, 0)
            + Math.max(0, groups.length - 1) * 26 * uiScale <= maxWidth
        if (!inline) return groups
        const result = []
        for (let i = 0; i < groups.length; ++i) {
            if (i) result.push({tokens: [{label: "/", separator: true}], width: 6 * uiScale})
            result.push(groups[i])
        }
        return result
    }
    function rowY(index) {
        const first = tourTwoColumns ? Math.floor(index / 6) * 6 : 0
        let y = Math.max(154 * uiScale, tourHeading.y + tourHeading.height + 32 * uiScale)
        for (let i = first; i < index; ++i) y += tour.itemAt(i)?.height || 82 * uiScale
        return y
    }
    onPageIndexChanged: body.contentY = 0
    onCompactChanged: if (visible && pageIndex === 0) Qt.callLater(() => focusAccent(draftAccent))
    onClosed: {
        if (focusReturnItem && focusReturnItem.visible && focusReturnItem.enabled)
            focusReturnItem.forceActiveFocus(Qt.TabFocusReason)
        else fallbackFocusRequested()
        focusReturnItem = null
    }
    FontLoader { id: inter; source: root.assetBase + "InterVariable.ttf" }
    FontMetrics { id: keyMetrics; font.family: inter.name; font.pixelSize: 18 * root.uiScale }
    background: Rectangle { radius: 14 * root.uiScale; color: root.tone("#0F0F0F") }
    C.Overlay.modal: Rectangle { color: "#80000000" }

    component Label: C.Label {
        font.family: inter.name
        font.pixelSize: 16 * root.uiScale
        color: root.tone("#8F8F8F")
        textFormat: Text.PlainText
        wrapMode: Text.WordWrap
    }
    component AccentChoice: C.Button {
        id: choice
        required property int index
        objectName: "setupAccent" + index
        width: 64 * root.uiScale; height: width
        padding: 0
        checkable: true; autoExclusive: true
        checked: root.draftAccent === index
        enabled: root.available && !root.saving
        Accessible.role: Accessible.RadioButton
        Accessible.name: [qsTr("Orange"),qsTr("Red"),qsTr("Yellow"),qsTr("Green"),qsTr("Cyan"),qsTr("Blue"),qsTr("Purple"),qsTr("Pink"),qsTr("Gray")][index]
        onClicked: root.draftAccent = index
        onActiveFocusChanged: if (activeFocus) root.reveal(choice)
        Keys.onRightPressed: root.focusAccent(index + 1)
        Keys.onLeftPressed: root.focusAccent(index - 1)
        background: Rectangle {
            color: "transparent"; radius: width / 2
            border.width: choice.checked || choice.visualFocus ? 4 * root.uiScale : 0
            border.color: choice.checked ? Qt.darker(JuxtopposedPalette.accentColors[choice.index], 2.8) : root.tone("#8F8F8F")
        }
        contentItem: Item {
            Rectangle { anchors.centerIn: parent; width: 40 * root.uiScale; height: width; radius: width / 2; color: JuxtopposedPalette.accentColors[choice.index]; opacity: choice.checked || choice.hovered ? 1 : 0.4 }
        }
    }
    component ActionButton: C.Button {
        id: button
        property bool primary: false
        width: 100 * root.uiScale; height: 32 * root.uiScale
        padding: 0
        font.family: inter.name; font.pixelSize: 16 * root.uiScale
        background: Rectangle {
            radius: 6 * root.uiScale
            color: button.primary ? previewAppearance.accent : root.tone(button.down ? "#292929" : "#191919")
            border.width: button.visualFocus ? 2 * root.uiScale : 0
            border.color: button.primary ? root.tone("#C6C6C6") : previewAppearance.accent
            opacity: button.enabled ? 1 : 0.5
        }
        contentItem: Label {
            text: button.text; font: button.font
            color: button.primary ? root.accentText() : root.tone("#8F8F8F")
            horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
            opacity: button.enabled ? 1 : 0.5
        }
    }
    contentItem: FocusScope {
        id: contentScope
        Flickable {
            id: body
            objectName: "setupScroll"
            anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
            height: Math.max(0, footer.y - (errorLabel.visible ? errorLabel.height + 16 * root.uiScale : 0))
            contentWidth: width
            contentHeight: root.pageIndex === 0 ? themePage.height : shortcutPage.height
            boundsBehavior: Flickable.StopAtBounds
            clip: true
            C.ScrollBar.vertical: C.ScrollBar {}
            Item {
                id: themePage
                width: body.width
                height: root.compact ? accentFlow.y + accentFlow.height + 24 * root.uiScale : 635 * root.uiScale
                visible: root.pageIndex === 0
                JuxtopposedSetupPreview {
                    id: preview
                    objectName: "setupPreview"
                    x: 17 * root.uiScale
                    y: root.compact ? themeRow.y + themeRow.height + 24 * root.uiScale : 19 * root.uiScale
                    width: Math.max(0, parent.width - 34 * root.uiScale); height: implicitHeight
                    appearance: previewAppearance; assetBase: root.assetBase
                }
                Label {
                    id: themeHeading
                    x: 24 * root.uiScale; width: parent.width - 48 * root.uiScale
                    y: (root.compact ? 24 : 173) * root.uiScale
                    height: Math.max(42 * root.uiScale, implicitHeight)
                    text: root.title; font.pixelSize: 24 * root.uiScale
                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                }
                Row {
                    id: themeRow
                    anchors.horizontalCenter: parent.horizontalCenter
                    y: root.compact ? themeHeading.y + themeHeading.height + 24 * root.uiScale : 286 * root.uiScale
                    spacing: (root.compact ? 16 : 40) * root.uiScale
                    height: themes.itemAt(0)?.height || 157 * root.uiScale
                    Repeater {
                        id: themes
                        model: [{label: qsTr("Light"), value: 1}, {label: qsTr("Dark"), value: 2}, {label: qsTr("System"), value: 0}]
                        delegate: C.Button {
                            id: choice
                            required property int index
                            required property var modelData
                            objectName: "setupTheme" + modelData.value
                            width: root.compact ? Math.min(116 * root.uiScale, Math.max(0, themePage.width - 64 * root.uiScale) / 3) : 116 * root.uiScale
                            height: width + 41 * root.uiScale
                            padding: 0
                            checkable: true; autoExclusive: true; checked: root.draftTheme === modelData.value
                            enabled: root.available && !root.saving
                            Accessible.role: Accessible.RadioButton; Accessible.name: modelData.label
                            onClicked: root.draftTheme = modelData.value
                            onActiveFocusChanged: if (activeFocus) root.reveal(choice)
                            Keys.onRightPressed: root.focusTheme(index + 1)
                            Keys.onLeftPressed: root.focusTheme(index - 1)
                            background: Item {}
                            contentItem: Item {
                                Rectangle {
                                    width: choice.width; height: width; color: "transparent"; radius: 20 * root.uiScale
                                    border.width: choice.checked ? 4 * root.uiScale : choice.visualFocus ? 2 * root.uiScale : 0
                                    border.color: previewAppearance.accent
                                    Rectangle {
                                        x: 8 * root.uiScale; y: x
                                        width: Math.max(0, parent.width - 16 * root.uiScale); height: width
                                        radius: 12 * root.uiScale
                                        color: choice.modelData.value === 1 ? "#C6C6C6" : "#191919"
                                        Image {
                                            anchors.centerIn: parent
                                            width: 100; height: 100
                                            scale: parent.width / 100
                                            visible: choice.modelData.value === 0
                                            source: root.assetBase + "setup-system-theme.svg"
                                            Accessible.ignored: true
                                        }
                                    }
                                }
                                Label {
                                    y: choice.width + 22 * root.uiScale
                                    width: parent.width; height: 19 * root.uiScale
                                    text: choice.modelData.label; horizontalAlignment: Text.AlignHCenter
                                }
                            }
                        }
                    }
                }
                Flow {
                    id: accentFlow
                    x: 16 * root.uiScale; y: preview.y + preview.height + 24 * root.uiScale
                    width: parent.width - 32 * root.uiScale
                    spacing: 16 * root.uiScale
                    visible: root.compact
                    Repeater { id: compactAccents; model: 9; delegate: AccentChoice {} }
                }
            }
            Item {
                id: shortcutPage
                width: body.width
                visible: root.pageIndex === 1
                height: {
                    let bottom = 154 * root.uiScale
                    for (let i = 0; i < tour.count; ++i) { const row = tour.itemAt(i); if (row) bottom = Math.max(bottom, row.y + row.height - 16 * root.uiScale) }
                    return bottom
                }
                Label {
                    id: tourHeading
                    x: 24 * root.uiScale; y: 60 * root.uiScale; width: parent.width - 48 * root.uiScale
                    height: Math.max(42 * root.uiScale, implicitHeight)
                    text: root.title; font.pixelSize: 24 * root.uiScale; horizontalAlignment: Text.AlignHCenter
                }
                Repeater {
                    id: tour
                    model: [
                        {label: qsTr("Play / Pause"), actions: ["key-play-pause"]},
                        {label: qsTr("Seek Backward / Forward"), actions: ["key-jump-extrashort", "key-jump+extrashort"]},
                        {label: qsTr("Previous / Next Item"), actions: ["key-prev", "key-next"]},
                        {label: qsTr("Previous / Next Frame"), actions: ["key-frame-previous", "key-frame-next"]},
                        {label: qsTr("Cycle Subtitle Track"), actions: ["key-subtitle-track"]},
                        {label: qsTr("Cycle Audio Track"), actions: ["key-audio-track"]},
                        {label: qsTr("Toggle Fullscreen"), actions: ["key-toggle-fullscreen"]},
                        {label: qsTr("Increase / Decrease Volume"), actions: ["key-vol-up", "key-vol-down"]},
                        {label: qsTr("Show / Hide Queue"), actions: ["@queue"]},
                        {label: qsTr("Show / Hide File Browser"), actions: ["@browser"]},
                        {label: qsTr("Take a Snapshot"), actions: ["key-snapshot"]},
                        {label: qsTr("Record"), actions: ["key-record"]}]
                    delegate: Item {
                        id: shortcutRow
                        required property int index
                        required property var modelData
                        objectName: "setupShortcut" + index
                        x: root.tourTwoColumns ? (62 * root.uiScale + Math.floor(index / 6) * (width + 150 * root.uiScale)) : 24 * root.uiScale
                        y: root.rowY(index)
                        width: root.tourTwoColumns ? Math.max(0, shortcutPage.width - 304 * root.uiScale) / 2 : shortcutPage.width - 48 * root.uiScale
                        height: Math.max(42 * root.uiScale, shortcutLabel.height, keyFlow.height) + 40 * root.uiScale
                        Accessible.role: Accessible.StaticText
                        Accessible.name: modelData.label + ": " + modelData.actions.map(action => root.bindingFor(action) || qsTr("Unassigned")).join(" / ")
                        Label {
                            id: shortcutLabel
                            width: Math.max(0, parent.width - keyFlow.width - 20 * root.uiScale)
                            height: Math.max(28 * root.uiScale, implicitHeight)
                            y: Math.max(0, (42 * root.uiScale - height) / 2)
                            text: shortcutRow.modelData.label; Accessible.ignored: true
                        }
                        Flow {
                            id: keyFlow
                            anchors.right: parent.right
                            width: Math.min(153 * root.uiScale, parent.width * 0.42)
                            spacing: 10 * root.uiScale
                            Repeater {
                                model: root.keyGroups(shortcutRow.modelData.actions, keyFlow.width)
                                delegate: Flow {
                                    id: bindingGroup
                                    required property var modelData
                                    objectName: "setupBindingGroup" + shortcutRow.index
                                    width: modelData.width
                                    spacing: 10 * root.uiScale
                                    Repeater {
                                        model: bindingGroup.modelData.tokens
                                        delegate: Item {
                                            required property var modelData
                                            width: root.keyWidth(modelData, bindingGroup.width)
                                            height: 42 * root.uiScale
                                            Rectangle { anchors.fill: parent; visible: !parent.modelData.separator; radius: 12 * root.uiScale; color: root.tone("#191919") }
                                            Rectangle { width: parent.width; height: 36 * root.uiScale; visible: !parent.modelData.separator; radius: 12 * root.uiScale; color: root.tone("#0F0F0F"); border.width: 3 * root.uiScale; border.color: root.tone("#292929") }
                                            Label { anchors.centerIn: parent; width: parent.modelData.separator ? parent.width : Math.max(0, parent.width - 12 * root.uiScale); height: 28 * root.uiScale; text: parent.modelData.label; font.pixelSize: 18 * root.uiScale; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; wrapMode: Text.NoWrap; elide: Text.ElideRight; Accessible.ignored: true }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        Label {
            id: errorLabel
            objectName: "setupSaveError"
            x: 24 * root.uiScale; width: parent.width - 48 * root.uiScale
            height: Math.min(80 * root.uiScale, implicitHeight)
            y: footer.y - height - 8 * root.uiScale
            visible: text.length > 0
            text: root.saveError; font.pixelSize: 14 * root.uiScale; clip: true
            Accessible.role: Accessible.AlertMessage
        }
        Item {
            id: footer
            y: Math.max(0, parent.height - root.footerHeight)
            width: parent.width; height: root.footerHeight
            Row {
                x: 41 * root.uiScale; spacing: 16 * root.uiScale
                visible: !root.compact && root.pageIndex === 0
                Repeater { id: desktopAccents; model: 9; delegate: AccentChoice {} }
            }
            Label {
                x: (root.compact ? 24 : 62) * root.uiScale; y: 16 * root.uiScale
                width: Math.max(0, backButton.x - x - 16 * root.uiScale)
                text: qsTr("Find the full list in the menu.")
                visible: root.pageIndex === 1 && !root.compact
            }
            ActionButton {
                id: backButton; objectName: "setupBack"
                x: nextButton.x - 108 * root.uiScale; y: 16 * root.uiScale
                text: qsTr("Back"); enabled: root.pageIndex > 0 && !root.saving
                onClicked: root.pageIndex = 0
            }
            ActionButton {
                id: nextButton; objectName: root.pageIndex === 0 ? "setupNext" : "setupFinish"
                x: parent.width - (root.compact ? 116 : 162) * root.uiScale; y: 16 * root.uiScale
                primary: true; text: root.pageIndex === 0 ? qsTr("Next") : qsTr("Finish")
                enabled: root.available && !root.saving
                onClicked: { if (root.pageIndex === 0) root.pageIndex = 1; else root.finish() }
            }
        }
        JuxtopposedDialogDragArea { anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; height: 19 * root.uiScale; popup: root; z: 2 }
    }
}
