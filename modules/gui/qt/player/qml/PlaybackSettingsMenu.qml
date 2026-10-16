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
    property bool subtitleTimelineVisible: false
    signal subtitleTimelineRequested(bool enabled)

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
        color: JuxtopposedPalette.color("#111111")
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
                        border.color: preset.visualFocus ? JuxtopposedPalette.color("#D37918") : JuxtopposedPalette.color("#252525")
                        border.width: root.uiScale
                        color: JuxtopposedPalette.color("#161616")
                        radius: 4 * root.uiScale
                    }
                    contentItem: C.Label {
                        color: JuxtopposedPalette.color("#969696")
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
                keyboardControls: [subtitleTimelineToggle, subtitleDelay.editor, secondaryDelay.editor, subtitleFPS.editor, duration.slider, embedded, subtitleFont.editor, subtitleColor.editor, outlineColor.editor, subtitleOffset.editor, relativeSize, pixelSize, fontSize.editor, outlineSize.editor, backgroundOpacity.editor, sync].concat(subtitlePresets.keyboardControls)

                SectionLabel {
                    text: qsTr("Synchronization")
                }
                JuxtopposedSubtitleTimelineToggle {
                    id: subtitleTimelineToggle
                    Layout.fillWidth: true
                    checked: root.subtitleTimelineVisible
                    enabled: Player.isStarted && Player.videoTracks.count > 0
                    uiScale: root.uiScale
                    onClicked: root.subtitleTimelineRequested(checked)
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
                Widgets.JuxtopposedSettingSlider {
                    id: duration

                    Layout.fillWidth: true
                    decimals: 3
                    enabled: Player.subtitleAppearance.durationAvailable
                    from: 0
                    helpText: Player.subtitleAppearance.durationDescription
                    title: qsTr("Subtitle duration factor")
                    to: 20
                    uiScale: root.uiScale
                    unit: Player.subtitleAppearance.durationUnit
                    value: Player.subtitleAppearance.durationFactor

                    onMoveFocusRequested: direction => root.focusSetting(duration.slider, direction)
                    onValueRequested: value => Player.subtitleAppearance.durationFactor = value
                }
                SectionLabel {
                    text: qsTr("Appearance")
                }
                SectionLabel {
                    Layout.fillWidth: true
                    text: Player.subtitleAppearance.statusText
                    visible: !Player.subtitleAppearance.available
                    wrapMode: Text.WordWrap
                }
                ToggleSetting {
                    id: embedded

                    checked: Player.subtitleAppearance.embeddedFormat
                    enabled: Player.subtitleAppearance.available
                    text: qsTr("Embedded sub format")

                    onClicked: Player.subtitleAppearance.embeddedFormat = checked
                }
                TextSetting {
                    id: subtitleFont

                    enabled: Player.subtitleAppearance.available
                    title: qsTr("Subtitle font")
                    value: Player.subtitleAppearance.fontFamily

                    onMoveFocusRequested: direction => root.focusSetting(subtitleFont.editor, direction)
                    onValueRequested: value => Player.subtitleAppearance.fontFamily = value
                }
                TextSetting {
                    id: subtitleColor

                    colorField: true
                    enabled: Player.subtitleAppearance.available
                    title: qsTr("Text color")
                    value: Player.subtitleAppearance.textColor

                    onMoveFocusRequested: direction => root.focusSetting(subtitleColor.editor, direction)
                    onValueRequested: value => Player.subtitleAppearance.textColor = value
                }
                TextSetting {
                    id: outlineColor

                    colorField: true
                    enabled: Player.subtitleAppearance.available
                    title: qsTr("Outline color")
                    value: Player.subtitleAppearance.outlineColor

                    onMoveFocusRequested: direction => root.focusSetting(outlineColor.editor, direction)
                    onValueRequested: value => Player.subtitleAppearance.outlineColor = value
                }
                Widgets.JuxtopposedSettingSlider {
                    id: subtitleOffset

                    Layout.fillWidth: true
                    decimals: 0
                    enabled: Player.subtitleAppearance.available
                    from: -2000
                    sliderVisible: false
                    title: qsTr("Subtitle offset")
                    to: 2000
                    uiScale: root.uiScale
                    unit: " px"
                    value: Player.subtitleAppearance.offset

                    onMoveFocusRequested: direction => root.focusSetting(subtitleOffset.editor, direction)
                    onValueRequested: value => Player.subtitleAppearance.offset = Math.round(value)
                }
                RowLayout {
                    Layout.fillWidth: true
                    enabled: Player.subtitleAppearance.available
                    opacity: enabled ? 1 : 0.35

                    SectionLabel {
                        Layout.fillWidth: true
                        text: qsTr("Size values")
                    }
                    RowLayout {
                        Layout.minimumWidth: 0
                        Layout.preferredWidth: 190 * root.uiScale
                        spacing: 2 * root.uiScale
                        uniformCellSizes: true

                        FlipButton {
                            id: relativeSize

                            checked: Player.subtitleAppearance.sizeMode === 0
                            text: qsTr("Relative")

                            onClicked: Player.subtitleAppearance.sizeMode = 0
                        }
                        FlipButton {
                            id: pixelSize

                            checked: Player.subtitleAppearance.sizeMode === 1
                            text: qsTr("Pixels")

                            onClicked: Player.subtitleAppearance.sizeMode = 1
                        }
                    }
                }
                Widgets.JuxtopposedSettingSlider {
                    id: fontSize

                    Layout.fillWidth: true
                    decimals: 2
                    enabled: Player.subtitleAppearance.available
                    from: Player.subtitleAppearance.fontSizeMinimum
                    sliderVisible: false
                    title: qsTr("Font size")
                    to: Player.subtitleAppearance.fontSizeMaximum
                    uiScale: root.uiScale
                    unit: Player.subtitleAppearance.sizeMode === 1 ? " px" : "%"
                    value: Player.subtitleAppearance.fontSize

                    onMoveFocusRequested: direction => root.focusSetting(fontSize.editor, direction)
                    onValueRequested: value => Player.subtitleAppearance.fontSize = value
                }
                Widgets.JuxtopposedSettingSlider {
                    id: outlineSize

                    Layout.fillWidth: true
                    decimals: 1
                    enabled: Player.subtitleAppearance.available
                    from: 0
                    sliderVisible: false
                    title: qsTr("Outline thickness")
                    to: 50
                    uiScale: root.uiScale
                    unit: Player.subtitleAppearance.sizeMode === 1 ? " px" : "%"
                    value: Player.subtitleAppearance.outlineSize

                    onMoveFocusRequested: direction => root.focusSetting(outlineSize.editor, direction)
                    onValueRequested: value => Player.subtitleAppearance.outlineSize = value
                }
                Widgets.JuxtopposedSettingSlider {
                    id: backgroundOpacity

                    Layout.fillWidth: true
                    decimals: 1
                    enabled: Player.subtitleAppearance.available
                    from: 0
                    sliderVisible: false
                    title: qsTr("Background opacity")
                    to: 100
                    uiScale: root.uiScale
                    unit: "%"
                    value: Player.subtitleAppearance.backgroundOpacity

                    onMoveFocusRequested: direction => root.focusSetting(backgroundOpacity.editor, direction)
                    onValueRequested: value => Player.subtitleAppearance.backgroundOpacity = value
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
                JuxtopposedSubtitlePresets {
                    id: subtitlePresets

                    Layout.fillWidth: true
                    controller: MainCtx.subtitlePresets
                    iconFontFamily: VLCIcons.fontFamily
                    addIcon: VLCIcons.add
                    moreIcon: VLCIcons.ellipsis
                    checkIcon: VLCIcons.check
                    downIcon: VLCIcons.chevron_down
                    fontFamily: inter.name
                    uiScale: root.uiScale

                    onMoveFocusRequested: (current, direction) => root.focusSetting(current, direction)
                    onAdvancedRequested: {
                        root.close();
                        DialogsProvider.prefsDialog();
                    }
                }
            }
        }
        videoContent: Component {
            SettingsPage {
                keyboardControls: [flip.noneButton, flip.horizontalButton, flip.verticalButton, rotation.editor, zoom.selector, aspect.selector, crop.selector, deinterlace.selector, mode.selector, effects]

                SectionLabel {
                    text: qsTr("Defaults")
                }
                FlipChoice {
                    id: flip

                    enabled: Player.videoTransform.available
                    value: Player.videoTransform.flip

                    onValueRequested: value => Player.videoTransform.flip = value
                }
                Widgets.JuxtopposedSettingSlider {
                    id: rotation

                    Layout.fillWidth: true
                    decimals: Math.abs(value - Math.round(value)) > 0.001 ? 1 : 0
                    enabled: Player.videoTransform.available
                    from: 0
                    sliderVisible: false
                    title: qsTr("Rotate")
                    to: 360
                    uiScale: root.uiScale
                    unit: qsTr(" deg")
                    value: Player.videoTransform.rotation

                    onValueRequested: value => Player.videoTransform.rotation = value
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
            else if (index === 3)
                Player.subtitleAppearance.refresh();
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
            border.color: actionButton.visualFocus ? JuxtopposedPalette.color("#D37918") : JuxtopposedPalette.color("#252525")
            border.width: root.uiScale
            color: actionButton.down ? JuxtopposedPalette.color("#303030") : JuxtopposedPalette.color("#161616")
            radius: 4 * root.uiScale
        }
        contentItem: C.Label {
            color: JuxtopposedPalette.color("#969696")
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
    component FlipButton: C.Button {
        id: button

        Layout.fillWidth: true
        Layout.minimumWidth: 0
        Layout.preferredHeight: 40 * root.uiScale
        Layout.preferredWidth: 1
        font.family: inter.name
        font.pixelSize: 14 * root.uiScale
        padding: 4 * root.uiScale

        background: Rectangle {
            border.color: button.visualFocus ? JuxtopposedPalette.color("#D37918") : JuxtopposedPalette.color("#090909")
            border.width: root.uiScale
            color: button.checked ? JuxtopposedPalette.color("#331D05") : button.down ? JuxtopposedPalette.color("#303030") : JuxtopposedPalette.color("#161616")
            radius: 4 * root.uiScale
        }
        contentItem: C.Label {
            color: button.checked ? JuxtopposedPalette.color("#D37918") : JuxtopposedPalette.color("#969696")
            elide: Text.ElideRight
            font: button.font
            horizontalAlignment: Text.AlignHCenter
            text: button.text
            verticalAlignment: Text.AlignVCenter
        }
    }
    component FlipChoice: ColumnLayout {
        id: choice

        property alias horizontalButton: horizontal
        property alias noneButton: none
        property int value: 0
        property alias verticalButton: vertical

        signal valueRequested(int value)

        Layout.fillWidth: true
        opacity: enabled ? 1 : 0.35
        spacing: 12 * root.uiScale

        SectionLabel {
            text: qsTr("Flip")
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 2 * root.uiScale
            uniformCellSizes: true

            FlipButton {
                id: none

                Accessible.name: qsTr("Flip none")
                checked: choice.value === 0
                text: qsTr("None")

                onClicked: choice.valueRequested(0)
            }
            FlipButton {
                id: horizontal

                Accessible.name: qsTr("Flip horizontal")
                checked: choice.value === 1
                text: qsTr("Horizontal")

                onClicked: choice.valueRequested(1)
            }
            FlipButton {
                id: vertical

                Accessible.name: qsTr("Flip vertical")
                checked: choice.value === 2
                text: qsTr("Vertical")

                onClicked: choice.valueRequested(2)
            }
        }
    }
    component SectionLabel: C.Label {
        Layout.fillWidth: true
        Layout.topMargin: 8 * root.uiScale
        color: JuxtopposedPalette.color("#969696")
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
    component TextSetting: C.Control {
        id: textSetting

        property bool colorField: false
        property alias editor: field
        property string title: ""
        property string value: ""

        signal moveFocusRequested(int direction)
        signal valueRequested(string value)

        Layout.fillWidth: true
        implicitHeight: 40 * root.uiScale
        implicitWidth: 360 * root.uiScale
        opacity: enabled ? 1 : 0.35
        padding: 0

        contentItem: RowLayout {
            spacing: 12 * root.uiScale

            C.Label {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                color: JuxtopposedPalette.color("#969696")
                elide: Text.ElideRight
                font.family: inter.name
                font.pixelSize: 14 * root.uiScale
                text: textSetting.title
            }
            C.TextField {
                id: field

                property bool dirty: false

                Accessible.name: textSetting.title
                Layout.preferredHeight: 32 * root.uiScale
                Layout.preferredWidth: Math.min(144 * root.uiScale, textSetting.width * 0.48)
                bottomPadding: 4 * root.uiScale
                color: JuxtopposedPalette.color("#969696")
                font.family: inter.name
                font.pixelSize: 14 * root.uiScale
                maximumLength: textSetting.colorField ? 7 : 256
                objectName: "settingTextEditor"
                padding: 10 * root.uiScale
                rightPadding: (textSetting.colorField ? 34 : 10) * root.uiScale
                selectByMouse: true
                topPadding: 4 * root.uiScale

                background: Rectangle {
                    border.color: field.activeFocus ? JuxtopposedPalette.color("#D37918") : JuxtopposedPalette.color("#252525")
                    border.width: root.uiScale
                    color: JuxtopposedPalette.color("#090909")
                    radius: 4 * root.uiScale
                }

                Keys.onDownPressed: textSetting.moveFocusRequested(1)
                Keys.onUpPressed: textSetting.moveFocusRequested(-1)
                onEditingFinished: {
                    if (dirty) {
                        const input = text.trim();
                        if (input && (!textSetting.colorField || /^#[0-9a-fA-F]{6}$/.test(input)) && input !== textSetting.value)
                            textSetting.valueRequested(input);
                        text = textSetting.value;
                    }
                    dirty = false;
                }
                onTextEdited: dirty = true

                Rectangle {
                    anchors.right: parent.right
                    anchors.rightMargin: 8 * root.uiScale
                    anchors.verticalCenter: parent.verticalCenter
                    border.color: JuxtopposedPalette.color("#404040")
                    border.width: root.uiScale
                    color: textSetting.colorField ? textSetting.value : "transparent"
                    height: width
                    radius: 2 * root.uiScale
                    visible: textSetting.colorField
                    width: 20 * root.uiScale
                }
                Binding {
                    property: "text"
                    restoreMode: Binding.RestoreNone
                    target: field
                    value: textSetting.value
                    when: !field.activeFocus || !field.dirty
                }
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
            color: JuxtopposedPalette.color("#969696")
            font: toggle.font
            text: toggle.text
            verticalAlignment: Text.AlignVCenter
        }
        indicator: Rectangle {
            border.color: toggle.visualFocus ? JuxtopposedPalette.color("#D37918") : JuxtopposedPalette.color("#252525")
            border.width: root.uiScale
            color: JuxtopposedPalette.color("#090909")
            height: 24 * root.uiScale
            radius: 4 * root.uiScale
            width: 56 * root.uiScale
            x: toggle.width - width
            y: (toggle.height - height) / 2

            Rectangle {
                color: toggle.checked ? JuxtopposedPalette.color("#D37918") : JuxtopposedPalette.color("#404040")
                height: parent.height - 2 * root.uiScale
                radius: 3 * root.uiScale
                width: parent.width / 2 - root.uiScale
                x: toggle.checked ? parent.width / 2 : root.uiScale
                y: root.uiScale
            }
            C.Label {
                color: toggle.checked ? JuxtopposedPalette.color("#D37918") : JuxtopposedPalette.color("#6B6B6B")
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
