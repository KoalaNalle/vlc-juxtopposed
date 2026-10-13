/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
import QtQuick
import QtQuick.Window
import QtQuick.Templates as T
import QtQuick.Controls.Basic as C
import QtQuick.Layouts
import QtQml.Models

import VLC.MainInterface
import VLC.Style
import VLC.Player
import VLC.Dialogs
import VLC.Widgets as Widgets
import VLC.Util

// Transitional native settings popup. Reuse VLC's working controls until the
// Juxtopposed settings sidebar can be implemented from its full Figma context.
T.Popup {
    id: root

    property real maximumHeight: MainCtx.intfMainWindow.height - VLCStyle.margin_large * 2

    readonly property ColorContext colorContext: ColorContext {
        id: theme
        colorSet: ColorContext.Window
    }

    width: Math.min(VLCStyle.dp(448, VLCStyle.scale), parent.width)
    implicitHeight: contentScope.implicitHeight + topPadding + bottomPadding
    height: Math.min(implicitHeight, Math.max(1, maximumHeight))
    padding: VLCStyle.margin_normal
    margins: MainCtx.windowExtendedMargin
    focus: true
    closePolicy: T.Popup.CloseOnEscape | T.Popup.CloseOnPressOutside

    function showSettings() {
        open()
    }

    function focusSetting(current, direction) {
        const controls = [speed.slider, aspect.selector, crop.selector,
                          stereo.selector, mix.selector, effects,
                          synchronization, closeButton]
        const currentIndex = controls.indexOf(current)
        for (let offset = 1; offset <= controls.length; ++offset) {
            const index = (currentIndex + offset * direction + controls.length) % controls.length
            const item = controls[index]
            if (item.enabled && item.visible) {
                item.forceActiveFocus(direction > 0 ? Qt.TabFocusReason : Qt.BacktabFocusReason)
                return
            }
        }
    }

    function ensureVisible(item) {
        const top = item.mapToItem(flick.contentItem, 0, 0).y
        const bottom = top + item.height
        if (top < flick.contentY)
            flick.contentY = Math.max(0, top)
        else if (bottom > flick.contentY + flick.height)
            flick.contentY = Math.min(Math.max(0, flick.contentHeight - flick.height),
                                     bottom - flick.height)
    }

    function ensureFocusedItemVisible() {
        if (!root.visible || !contentScope.Window.window)
            return
        const item = contentScope.Window.window.activeFocusItem
        let ancestor = item
        while (ancestor) {
            if (ancestor === flick.contentItem) {
                ensureVisible(item)
                return
            }
            ancestor = ancestor.parent
        }
    }

    onOpened: {
        flick.contentY = 0
        if (speed.enabled)
            speed.slider.forceActiveFocus(Qt.PopupFocusReason)
        else
            focusSetting(speed.slider, 1)
    }

    Connections {
        target: contentScope.Window.window
        function onActiveFocusItemChanged() {
            root.ensureFocusedItemVisible()
        }
    }

    background: Rectangle {
        color: theme.bg.primary
        radius: VLCStyle.dp(8, VLCStyle.scale)
        border.color: theme.border
    }

    contentItem: FocusScope {
        id: contentScope
        implicitHeight: settings.implicitHeight
        focus: true

        // Popup content is reparented by Qt, so carry the palette explicitly.
        readonly property ColorContext colorContext: ColorContext {
            colorSet: theme.colorSet
            palette: theme.palette
        }

        Accessible.role: Accessible.Pane
        Accessible.name: qsTr("Playback settings")
        AccessibleCompat.id: "playbackSettingsMenu"
        Navigation.cancelAction: function() { root.close() }
        Keys.priority: Keys.AfterItem
        Keys.onEscapePressed: (event) => {
            root.close()
            event.accepted = true
        }

        Flickable {
            id: flick
            anchors.fill: parent
            contentWidth: width
            contentHeight: settings.implicitHeight
            flickableDirection: Flickable.VerticalFlick
            boundsBehavior: Flickable.StopAtBounds
            clip: true
            onHeightChanged: Qt.callLater(root.ensureFocusedItemVisible)

            C.ScrollBar.vertical: Widgets.ScrollBarExt {
                id: scrollBar
            }

            ColumnLayout {
                id: settings
                width: flick.width - (scrollBar.size < 1 ? scrollBar.width + VLCStyle.margin_xsmall : 0)
                spacing: VLCStyle.margin_small

                PlaybackSpeed {
                    id: speed
                    Layout.fillWidth: true
                    enabled: Player.isStarted && Player.ratechangable
                    colorContext.palette: theme.palette
                    Navigation.parentItem: contentScope
                    Navigation.downAction: function() { root.focusSetting(speed.slider, 1) }
                    Navigation.upAction: function() { root.focusSetting(speed.slider, -1) }
                }

                ChoiceSetting {
                    id: aspect
                    title: qsTr("Aspect ratio")
                    choices: Player.aspectRatio
                    available: Player.hasVideoOutput
                    accessibleId: "playbackSettingsAspectRatio"
                }

                ChoiceSetting {
                    id: crop
                    title: qsTr("Crop")
                    choices: Player.crop
                    available: Player.hasVideoOutput
                    accessibleId: "playbackSettingsCrop"
                }

                ChoiceSetting {
                    id: stereo
                    title: qsTr("Stereo mode")
                    choices: Player.audioStereoMode
                    available: Player.isStarted
                    accessibleId: "playbackSettingsStereoMode"
                }

                ChoiceSetting {
                    id: mix
                    title: qsTr("Audio mix")
                    choices: Player.audioMixMode
                    available: Player.isStarted
                    accessibleId: "playbackSettingsAudioMix"
                }

                Widgets.ButtonExt {
                    id: effects
                    Layout.fillWidth: true
                    text: qsTr("Adjustments and effects")
                    activeFocusOnTab: true
                    AccessibleCompat.id: "playbackSettingsEffects"
                    Navigation.parentItem: contentScope
                    Navigation.downAction: function() { root.focusSetting(effects, 1) }
                    Navigation.upAction: function() { root.focusSetting(effects, -1) }
                    onClicked: {
                        root.close()
                        DialogsProvider.extendedDialog()
                    }
                }

                Widgets.ButtonExt {
                    id: synchronization
                    Layout.fillWidth: true
                    text: qsTr("Track synchronization")
                    enabled: Player.isStarted
                    activeFocusOnTab: true
                    AccessibleCompat.id: "playbackSettingsSynchronization"
                    Navigation.parentItem: contentScope
                    Navigation.downAction: function() { root.focusSetting(synchronization, 1) }
                    Navigation.upAction: function() { root.focusSetting(synchronization, -1) }
                    onClicked: {
                        root.close()
                        DialogsProvider.synchroDialog()
                    }
                }

                Widgets.ButtonExt {
                    id: closeButton
                    Layout.fillWidth: true
                    text: qsTr("Close")
                    activeFocusOnTab: true
                    AccessibleCompat.id: "playbackSettingsClose"
                    Navigation.parentItem: contentScope
                    Navigation.downAction: function() { root.focusSetting(closeButton, 1) }
                    Navigation.upAction: function() { root.focusSetting(closeButton, -1) }
                    onClicked: root.close()
                }
            }
        }
    }

    component ChoiceSetting: RowLayout {
        id: field
        required property string title
        required property var choices
        required property string accessibleId
        property bool available: true
        property alias selector: combo

        Layout.fillWidth: true
        spacing: VLCStyle.margin_small
        enabled: available && selection.count > 0

        function updateSelection() {
            let selected = -1
            for (let index = 0; index < selection.count; ++index) {
                const item = selection.objectAt(index)
                if (item && item.checked) {
                    selected = index
                    break
                }
            }
            combo.currentIndex = selected
        }

        Widgets.CaptionLabel {
            Layout.fillWidth: true
            Layout.preferredWidth: field.width / 3
            text: field.title
            color: theme.fg.primary
            wrapMode: Text.Wrap
        }

        Widgets.ComboBoxExt {
            id: combo
            Layout.fillWidth: true
            Layout.preferredWidth: field.width * 2 / 3
            Layout.minimumWidth: 0
            model: field.choices
            textRole: "display"
            currentIndex: -1
            displayText: currentIndex >= 0 ? currentText
                         : enabled ? qsTr("Custom") : qsTr("Unavailable")
            activeFocusOnTab: true
            Accessible.name: field.title
            AccessibleCompat.id: field.accessibleId
            Navigation.parentItem: contentScope
            Navigation.downAction: function() { root.focusSetting(combo, 1) }
            Navigation.upAction: function() { root.focusSetting(combo, -1) }

            // Only user activation writes to the engine. Model changes merely
            // update the displayed selection, including hotkey changes.
            onActivated: (index) => field.choices.toggleIndex(index)
        }

        Instantiator {
            id: selection
            model: field.choices
            delegate: QtObject {
                required property bool checked
                onCheckedChanged: Qt.callLater(field.updateSelection)
            }
            onObjectAdded: Qt.callLater(field.updateSelection)
            onObjectRemoved: Qt.callLater(field.updateSelection)
        }
    }
}
