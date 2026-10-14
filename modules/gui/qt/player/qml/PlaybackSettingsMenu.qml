// Copyright (C) 2026 VLC authors and VideoLAN. GPLv2 or later.
// Visual design: Juxtopposed; original artwork: CC BY 4.0.
import QtQuick
import QtQuick.Window
import QtQuick.Templates as T
import QtQuick.Controls.Basic as C
import QtQuick.Layouts
import VLC.MainInterface
import VLC.Style
import VLC.Player
import VLC.Dialogs
import VLC.Widgets as Widgets
import VLC.Util

T.Popup {
    id: root

    readonly property ColorContext colorContext: ColorContext {
        id: theme

        colorSet: ColorContext.Window
    }
    property real maximumHeight: MainCtx.intfMainWindow.height - VLCStyle.margin_large * 2
    readonly property real uiScale: VLCStyle.dp(1, VLCStyle.scale)

    function focusSetting(current, direction) {
        const controls = panel.loadedPage?.keyboardControls ?? [];
        const start = controls.indexOf(current);
        for (let n = 1; n <= controls.length; ++n) {
            const item = controls[(start + n * direction + controls.length) % controls.length];
            if (item?.enabled && item.visible) {
                item.forceActiveFocus(direction > 0 ? Qt.TabFocusReason : Qt.BacktabFocusReason);
                return;
            }
        }
    }
    function showSettings() {
        Player.equalizer.refresh();
        open();
    }

    closePolicy: T.Popup.CloseOnEscape | T.Popup.CloseOnPressOutside
    focus: true
    height: Math.min(600 * uiScale, Math.max(1, maximumHeight))
    margins: MainCtx.windowExtendedMargin
    padding: 0
    width: Math.min(400 * uiScale, parent.width)

    background: Rectangle {
        color: "#111111"
        radius: 8 * root.uiScale
    }
    contentItem: JuxtopposedSettingsPanel {
        id: panel

        iconFontFamily: VLCIcons.fontFamily
        tabIcons: [VLCIcons.play, VLCIcons.tv, VLCIcons.visualization, VLCIcons.audiosub]
        uiScale: root.uiScale

        audioContent: Component {
            SettingsPage {
                keyboardControls: {
                    const result = [stereo.selector, mix.selector, audioDelay.editor, eqEnabled, preset, preamp.slider];
                    for (let i = 0; i < bandControls.count; ++i)
                        result.push(bandControls.itemAt(i).slider);
                    return result.concat([twoPass, audioEffects]);
                }

                ChoiceSetting {
                    id: stereo

                    available: Player.isStarted
                    choices: Player.audioStereoMode
                    title: qsTr("Stereo mode")
                }
                ChoiceSetting {
                    id: mix

                    available: Player.isStarted
                    choices: Player.audioMixMode
                    title: qsTr("Audio mix")
                }
                SectionLabel {
                    text: qsTr("Synchronization")
                }
                Widgets.JuxtopposedSettingSlider {
                    id: audioDelay

                    Layout.fillWidth: true
                    decimals: 3
                    enabled: Player.isStarted && Player.audioTracks.count > 0
                    from: -10
                    sliderVisible: false
                    title: qsTr("Audio track delay")
                    to: 10
                    uiScale: root.uiScale
                    unit: " s"
                    value: Player.audioDelayMS / 1000

                    onMoveFocusRequested: direction => root.focusSetting(audioDelay.editor, direction)
                    onValueRequested: value => Player.audioDelayMS = Math.round(value * 1000)
                }
                SectionLabel {
                    text: qsTr("Equalizer")
                }
                ToggleSetting {
                    id: eqEnabled

                    checked: Player.equalizer.enabled
                    enabled: Player.isStarted && Player.audioTracks.count > 0 && Player.equalizer.available
                    text: qsTr("Enable equalizer")

                    onToggled: Player.equalizer.enabled = checked
                }
                C.ComboBox {
                    id: preset

                    Accessible.name: qsTr("Equalizer preset")
                    Layout.fillWidth: true
                    Layout.preferredHeight: 32 * root.uiScale
                    currentIndex: Player.equalizer.presetIndex
                    enabled: eqEnabled.enabled
                    font.family: inter.name
                    font.pixelSize: 14 * root.uiScale
                    model: Player.equalizer.presets

                    background: Rectangle {
                        border.color: preset.visualFocus ? "#D37918" : "#252525"
                        border.width: root.uiScale
                        color: "#161616"
                        radius: 4 * root.uiScale
                    }
                    contentItem: C.Label {
                        color: "#969696"
                        elide: Text.ElideRight
                        font: preset.font
                        text: preset.displayText
                        verticalAlignment: Text.AlignVCenter
                    }

                    onActivated: index => Player.equalizer.setPreset(index)
                }
                Widgets.JuxtopposedSettingSlider {
                    id: preamp

                    Layout.fillWidth: true
                    enabled: eqEnabled.enabled
                    from: -20
                    stepSize: 0.1
                    title: qsTr("Preamp")
                    to: 20
                    uiScale: root.uiScale
                    unit: " dB"
                    value: Player.equalizer.preamp

                    onMoveFocusRequested: direction => root.focusSetting(preamp.slider, direction)
                    onValueRequested: value => Player.equalizer.preamp = value
                }
                Repeater {
                    id: bandControls

                    model: Player.equalizer.frequencies

                    Widgets.JuxtopposedSettingSlider {
                        required property int index
                        required property real modelData

                        Layout.fillWidth: true
                        enabled: eqEnabled.enabled
                        from: -20
                        stepSize: 0.1
                        title: modelData >= 1000 ? qsTr("%1 kHz").arg(modelData / 1000) : qsTr("%1 Hz").arg(modelData)
                        to: 20
                        uiScale: root.uiScale
                        unit: " dB"
                        value: Player.equalizer.bands[index] ?? 0

                        onMoveFocusRequested: direction => root.focusSetting(slider, direction)
                        onValueRequested: value => Player.equalizer.setBand(index, value)
                    }
                }
                ToggleSetting {
                    id: twoPass

                    checked: Player.equalizer.twoPass
                    enabled: eqEnabled.enabled
                    text: qsTr("Two pass")

                    onToggled: Player.equalizer.twoPass = checked
                }
                ActionButton {
                    id: audioEffects

                    text: qsTr("More audio effects")

                    onClicked: {
                        root.close();
                        DialogsProvider.extendedDialog();
                    }
                }
            }
        }
        playbackContent: Component {
            SettingsPage {
                keyboardControls: [speed.slider, speed.editor]

                Widgets.JuxtopposedSettingSlider {
                    id: speed

                    Layout.fillWidth: true
                    decimals: 2
                    enabled: Player.isStarted && Player.ratechangable
                    from: 0.25
                    sliderMaximum: 2
                    stepSize: 0.01
                    ticks: [0.25, 0.5, 0.75, 1, 1.25, 1.5, 1.75, 2]
                    title: qsTr("Speed")
                    to: 4
                    uiScale: root.uiScale
                    unit: "x"
                    value: Player.rate

                    onMoveFocusRequested: direction => root.focusSetting(speed.slider, direction)
                    onValueRequested: value => Player.rate = value
                }
            }
        }
        subtitlesContent: Component {
            SettingsPage {
                keyboardControls: [subtitleDelay.editor, secondaryDelay.editor, subtitleFPS.editor, sync]

                SectionLabel {
                    text: qsTr("Synchronization")
                }
                Widgets.JuxtopposedSettingSlider {
                    id: subtitleDelay

                    Layout.fillWidth: true
                    decimals: 3
                    enabled: Player.isStarted && Player.subtitleTracks.count > 0
                    from: -10
                    sliderVisible: false
                    title: qsTr("Subtitle track delay")
                    to: 10
                    uiScale: root.uiScale
                    unit: " s"
                    value: Player.subtitleDelayMS / 1000

                    onMoveFocusRequested: direction => root.focusSetting(subtitleDelay.editor, direction)
                    onValueRequested: value => Player.subtitleDelayMS = Math.round(value * 1000)
                }
                Widgets.JuxtopposedSettingSlider {
                    id: secondaryDelay

                    Layout.fillWidth: true
                    decimals: 3
                    enabled: Player.isStarted && Player.subtitleTracks.count > 0
                    from: -10
                    sliderVisible: false
                    title: qsTr("Secondary subtitle delay")
                    to: 10
                    uiScale: root.uiScale
                    unit: " s"
                    value: Player.secondarySubtitleDelayMS / 1000

                    onMoveFocusRequested: direction => root.focusSetting(secondaryDelay.editor, direction)
                    onValueRequested: value => Player.secondarySubtitleDelayMS = Math.round(value * 1000)
                }
                Widgets.JuxtopposedSettingSlider {
                    id: subtitleFPS

                    Layout.fillWidth: true
                    decimals: 3
                    enabled: Player.isStarted && Player.subtitleTracks.count > 0
                    from: 0
                    sliderVisible: false
                    title: qsTr("Subtitle frame rate")
                    to: 100
                    uiScale: root.uiScale
                    unit: " fps"
                    value: Player.subtitleFPS

                    onMoveFocusRequested: direction => root.focusSetting(subtitleFPS.editor, direction)
                    onValueRequested: value => Player.subtitleFPS = value
                }
                ActionButton {
                    id: sync

                    enabled: Player.isStarted
                    text: qsTr("Synchronization tools")

                    onClicked: {
                        root.close();
                        DialogsProvider.synchroDialog();
                    }
                }
            }
        }
        videoContent: Component {
            SettingsPage {
                keyboardControls: [zoom.selector, aspect.selector, crop.selector, deinterlace.selector, mode.selector, effects]

                SectionLabel {
                    text: qsTr("Defaults")
                }
                ChoiceSetting {
                    id: zoom

                    available: Player.hasVideoOutput
                    choices: Player.zoom
                    title: qsTr("Zoom")
                }
                ChoiceSetting {
                    id: aspect

                    available: Player.hasVideoOutput
                    choices: Player.aspectRatio
                    title: qsTr("Aspect ratio")
                }
                ChoiceSetting {
                    id: crop

                    available: Player.hasVideoOutput
                    choices: Player.crop
                    title: qsTr("Crop")
                }
                ChoiceSetting {
                    id: deinterlace

                    available: Player.hasVideoOutput
                    choices: Player.deinterlace
                    title: qsTr("Deinterlace")
                }
                ChoiceSetting {
                    id: mode

                    available: Player.hasVideoOutput
                    choices: Player.deinterlaceMode
                    title: qsTr("Deinterlace mode")
                }
                ActionButton {
                    id: effects

                    text: qsTr("More video effects")

                    onClicked: {
                        root.close();
                        DialogsProvider.extendedDialog();
                    }
                }
            }
        }

        onCloseRequested: root.close()
        onTabRequested: index => {
            if (index === 2)
                Player.equalizer.refresh();
        }
    }

    onOpened: panel.focusContent(Qt.PopupFocusReason)

    FontLoader {
        id: inter

        source: "qrc:/juxtopposed/InterVariable.ttf"
    }

    component ActionButton: C.Button {
        id: actionButton

        Layout.fillWidth: true
        Layout.preferredHeight: 32 * root.uiScale
        font.family: inter.name
        font.pixelSize: 14 * root.uiScale

        background: Rectangle {
            border.color: actionButton.visualFocus ? "#D37918" : "#252525"
            border.width: root.uiScale
            color: actionButton.down ? "#303030" : "#161616"
            radius: 4 * root.uiScale
        }
        contentItem: C.Label {
            color: "#969696"
            font: actionButton.font
            horizontalAlignment: Text.AlignHCenter
            text: actionButton.text
            verticalAlignment: Text.AlignVCenter
        }
    }
    component ChoiceSetting: Widgets.JuxtopposedSettingChoice {
        Layout.fillWidth: true
        uiScale: root.uiScale
    }
    component SectionLabel: C.Label {
        Layout.fillWidth: true
        Layout.topMargin: 8 * root.uiScale
        color: "#969696"
        font.family: inter.name
        font.pixelSize: 16 * root.uiScale
    }
    component SettingsPage: FocusScope {
        id: page

        readonly property ColorContext colorContext: ColorContext {
            colorSet: theme.colorSet
            palette: theme.palette
        }
        default property alias content: column.data
        property var keyboardControls: []

        function setCurrentItemFocus(reason) {
            for (const item of keyboardControls)
                if (item.enabled && item.visible) {
                    item.forceActiveFocus(reason);
                    return;
                }
            page.forceActiveFocus(reason);
        }

        Connections {
            function onActiveFocusItemChanged() {
                const item = page.Window.window?.activeFocusItem;
                if (!item || !root.visible)
                    return;
                let ancestor = item;
                while (ancestor) {
                    if (ancestor === column) {
                        const top = item.mapToItem(flick.contentItem, 0, 0).y;
                        if (top < flick.contentY)
                            flick.contentY = Math.max(0, top);
                        else if (top + item.height > flick.contentY + flick.height)
                            flick.contentY = Math.min(Math.max(0, flick.contentHeight - flick.height), top + item.height - flick.height);
                        return;
                    }
                    ancestor = ancestor.parent;
                }
            }

            target: page.Window.window
        }
        Flickable {
            id: flick

            anchors.fill: parent
            boundsBehavior: Flickable.StopAtBounds
            clip: true
            contentHeight: column.implicitHeight
            contentWidth: width

            C.ScrollBar.vertical: C.ScrollBar {
            }

            ColumnLayout {
                id: column

                spacing: 12 * root.uiScale
                width: flick.width
            }
        }
    }
    component ToggleSetting: C.Switch {
        id: toggle

        Accessible.name: text
        Layout.fillWidth: true
        Layout.preferredHeight: 32 * root.uiScale
        font.family: inter.name
        font.pixelSize: 14 * root.uiScale
        leftPadding: 0
        rightPadding: 68 * root.uiScale

        contentItem: C.Label {
            color: "#969696"
            font: toggle.font
            text: toggle.text
            verticalAlignment: Text.AlignVCenter
        }
        indicator: Rectangle {
            border.color: toggle.visualFocus ? "#D37918" : "#252525"
            border.width: root.uiScale
            color: "#090909"
            height: 24 * root.uiScale
            radius: 4 * root.uiScale
            width: 56 * root.uiScale
            x: toggle.width - width
            y: (toggle.height - height) / 2

            Rectangle {
                color: toggle.checked ? "#D37918" : "#404040"
                height: parent.height - 2 * root.uiScale
                radius: 3 * root.uiScale
                width: parent.width / 2 - root.uiScale
                x: toggle.checked ? parent.width / 2 : root.uiScale
                y: root.uiScale
            }
            C.Label {
                color: toggle.checked ? "#D37918" : "#6B6B6B"
                font.family: inter.name
                font.pixelSize: 10 * root.uiScale
                height: parent.height
                horizontalAlignment: Text.AlignHCenter
                text: toggle.checked ? qsTr("ON") : qsTr("OFF")
                verticalAlignment: Text.AlignVCenter
                width: parent.width / 2
                x: toggle.checked ? 0 : parent.width / 2
            }
        }
    }
}
