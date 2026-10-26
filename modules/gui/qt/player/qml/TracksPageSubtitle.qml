/*****************************************************************************
 * Copyright (C) 2022 VLC authors and VideoLAN
 *
 * Authors: Benjamin Arnaud <bunjee@omega.gg>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * ( at your option ) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

import QtQuick
import QtQuick.Layouts


import VLC.MainInterface
import VLC.Style
import VLC.Player
import VLC.Util
import VLC.Widgets as Widgets

TracksPage {
    id: root

    // A pair of real-time marks belongs to one continuous media timeline.
    // Cancellation must not apply a zero per-track delay or change inheritance.
    Connections {
        target: Player
        function onInputChanged() { root.cancelPendingMarks() }
        function onRateChanged() { root.cancelPendingMarks() }
        function onSeekStarted() { root.cancelPendingMarks() }
        function onPlayingStateChanged() { root.cancelPendingMarks() }
    }

    function cancelPendingMarks() {
        delayEstimatorPrimary.cancel()
        delayEstimatorSecondary.cancel()
    }

    // Functions

    function textFromValueA(value, locale) {
        return qsTr("%1 ms").arg(Number(value).toLocaleString(locale, 'f', 0))
    }

    function valueFromTextA(text, locale) {
        return Number.fromLocaleString(locale, text.substring(0, text.length - 3))
    }

    function textFromValueB(value, locale) {
        return qsTr("%1 fps").arg(Number(value / 1000).toLocaleString(locale, 'f', 3))
    }

    function valueFromTextB(text, locale) {
        return Number.fromLocaleString(locale, text.substring(0, text.length - 4)) * 1000
    }

    // Children

    ColumnLayout {
        anchors.left: parent.left
        anchors.right: parent.right

        spacing: VLCStyle.margin_xsmall

        Widgets.SubtitleLabel {
            Layout.fillWidth: true

            text: qsTr("Subtitle synchronization")

            color: root.colorContext.fg.primary
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.topMargin: VLCStyle.margin_large

            spacing: VLCStyle.margin_xxxsmall

            Accessible.role: Accessible.Grouping
            Accessible.name: qsTr("Primary subtitle delay")

            DelayEstimator {
                id: delayEstimatorPrimary

                onDelayChanged: {
                    // Marks measure wall time; native track offsets use media time.
                    Player.addSubtitleDelay(delayEstimatorPrimary.delay.scale(Player.rate))
                }
            }

            RowLayout {
                Layout.fillWidth: true

                spacing: VLCStyle.margin_xsmall

                Widgets.MenuCaption {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter

                    text: qsTr("Primary subtitle delay")

                    color: root.colorContext.fg.primary
                }

                Widgets.SpinBoxExt {
                    id: spinBoxA

                    Accessible.name: qsTr("Primary subtitle delay")

                    property bool update: false

                    Layout.preferredWidth: VLCStyle.dp(128, VLCStyle.scale)

                    stepSize: 50
                    from: -10000
                    to: 10000

                    textFromValue: root.textFromValueA
                    valueFromText: root.valueFromTextA

                    Navigation.parentItem: root
                    Navigation.rightItem: resetA

                    Component.onCompleted: {
                        spinBoxA.value = Player.subtitleDelayMS

                        spinBoxA.update = true
                    }

                    onValueChanged: {
                        if (update === false)
                            return

                        Player.subtitleDelayMS = spinBoxA.value
                    }

                    Connections {
                        target: Player

                        function onSubtitleDelayChanged() {
                            spinBoxA.update = false

                            spinBoxA.value = Player.subtitleDelayMS

                            spinBoxA.update = true
                        }

                    }
                }

                Widgets.TextToolButton {
                    id: resetA

                    text: qsTr("Reset")

                    focus: true

                    Navigation.parentItem: root
                    Navigation.leftItem: spinBoxA
                    Navigation.downItem: resetB

                    onClicked: {
                        Player.subtitleDelayMS = 0
                        delayEstimatorPrimary.reset()
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true

                Layout.alignment: Qt.AlignRight

                spacing: VLCStyle.margin_small

                Widgets.TrackDelayButton {
                    id: voiceHeardA
                    Navigation.parentItem: root

                    iconTxt: VLCIcons.check
                    text: qsTr("Voice Heard")
                    selected: delayEstimatorPrimary.isHeardTimeMarked

                    onClicked: {
                        delayEstimatorPrimary.markHeardTime()
                        if (!delayEstimatorPrimary.isSpottedTimeMarked && delayEstimatorPrimary.isHeardTimeMarked)
                            textSeenA.animate()
                    }
                }

                Widgets.TrackDelayButton {
                    id: textSeenA
                    Navigation.parentItem: root

                    iconTxt: VLCIcons.check
                    text: qsTr("Text Seen")
                    selected: delayEstimatorPrimary.isSpottedTimeMarked

                    onClicked: {
                        delayEstimatorPrimary.markSpottedTime()
                        if (!delayEstimatorPrimary.isHeardTimeMarked && delayEstimatorPrimary.isSpottedTimeMarked)
                            voiceHeardA.animate()
                    }
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true

            spacing: VLCStyle.margin_xxxsmall

            Accessible.role: Accessible.Grouping
            Accessible.name: qsTr("Secondary subtitle delay")

            DelayEstimator {
                id: delayEstimatorSecondary

                onDelayChanged: {
                    Player.addSecondarySubtitleDelay(delayEstimatorSecondary.delay.scale(Player.rate))
                }
            }

            RowLayout {
                Layout.fillWidth: true

                spacing: VLCStyle.margin_xsmall

                Widgets.MenuCaption {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter

                    text: qsTr("Secondary subtitle delay")

                    color: root.colorContext.fg.primary
                }

                Widgets.SpinBoxExt {
                    id: spinBoxB

                    Accessible.name: qsTr("Secondary subtitle delay")

                    property bool update: false

                    Layout.preferredWidth: VLCStyle.dp(128, VLCStyle.scale)

                    stepSize: 50
                    from: -10000
                    to: 10000

                    textFromValue: root.textFromValueA
                    valueFromText: root.valueFromTextA

                    Navigation.parentItem: root
                    Navigation.rightItem: resetB

                    Component.onCompleted: {
                        value = Player.secondarySubtitleDelayMS

                        update = true
                    }

                    onValueChanged: {
                        if (update === false)
                            return

                        Player.secondarySubtitleDelayMS = spinBoxB.value
                    }

                    Connections {
                        target: Player

                        function onSecondarySubtitleDelayChanged() {
                            spinBoxB.update = false

                            spinBoxB.value = Player.secondarySubtitleDelayMS

                            spinBoxB.update = true
                        }
                    }
                }

                Widgets.TextToolButton {
                    id: resetB

                    text: qsTr("Reset")

                    Navigation.parentItem: root
                    Navigation.leftItem: spinBoxB
                    Navigation.upItem: resetA
                    Navigation.downItem: resetC

                    onClicked: {
                        Player.secondarySubtitleDelayMS = 0
                        delayEstimatorSecondary.reset()
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true

                Layout.alignment: Qt.AlignRight

                spacing: VLCStyle.margin_xsmall

                Widgets.TrackDelayButton {
                    id: voiceHeardB
                    Navigation.parentItem: root

                    text: qsTr("Voice Heard")
                    iconTxt: VLCIcons.check
                    selected: delayEstimatorSecondary.isHeardTimeMarked

                    onClicked: {
                        delayEstimatorSecondary.markHeardTime()
                        if (!delayEstimatorSecondary.isSpottedTimeMarked && delayEstimatorSecondary.isHeardTimeMarked)
                            textSeenB.animate()
                    }
                }

                Widgets.TrackDelayButton {
                    id: textSeenB
                    Navigation.parentItem: root

                    text: qsTr("Text Seen")
                    iconTxt: VLCIcons.check
                    selected: delayEstimatorSecondary.isSpottedTimeMarked

                    onClicked: {
                        delayEstimatorSecondary.markSpottedTime()
                        if (!delayEstimatorSecondary.isHeardTimeMarked && delayEstimatorSecondary.isSpottedTimeMarked)
                            voiceHeardB.animate()
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true

            spacing: VLCStyle.margin_xsmall

            Accessible.role: Accessible.Grouping
            Accessible.name: qsTr("Subtitle frame rate")

            Widgets.MenuCaption {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter

                text: qsTr("Subtitle frame rate")

                color: root.colorContext.fg.primary
            }

            FocusScope {
                id: spinBoxC

                objectName: "subtitleFrameRate"
                Accessible.role: Accessible.SpinBox
                Accessible.name: qsTr("Subtitle frame rate")
                Accessible.description: qsTr("Frame rate of external subtitle files. Set to 0 to restore normal timing.")
                activeFocusOnTab: true

                property bool update: false
                // Expose real FPS to accessibility; the existing visual spin
                // control uses integer thousandths only as its implementation.
                property real value: 0
                readonly property real minimumValue: 0
                readonly property real maximumValue: Math.max(100, value)
                readonly property real stepSize: 0.1

                Layout.preferredWidth: VLCStyle.dp(128, VLCStyle.scale)
                implicitHeight: fpsEditor.implicitHeight
                implicitWidth: fpsEditor.implicitWidth

                Navigation.parentItem: root
                Navigation.rightItem: resetC

                onActiveFocusChanged: if (activeFocus) fpsEditor.forceActiveFocus()

                Widgets.SpinBoxExt {
                    id: fpsEditor

                    anchors.fill: parent
                    Accessible.ignored: true
                    from: 0
                    to: Math.max(100000, Math.ceil(spinBoxC.value * 1000))
                    stepSize: 100
                    value: Math.round(spinBoxC.value * 1000)
                    textFromValue: root.textFromValueB
                    valueFromText: root.valueFromTextB
                    Navigation.parentItem: spinBoxC
                    onValueModified: spinBoxC.value = value / 1000
                }

                Component.onCompleted: {
                    value = Player.subtitleFPS

                    update = true
                }

                onValueChanged: {
                    if (update === false)
                        return

                    Player.subtitleFPS = value
                }

                Connections {
                    target: Player

                    function onSubtitleFPSChanged() {
                        spinBoxC.update = false

                        spinBoxC.value = Player.subtitleFPS

                        spinBoxC.update = true
                    }
                }
            }

            Widgets.TextToolButton {
                id: resetC

                text: qsTr("Reset")

                onClicked: Player.subtitleFPS = 0

                Navigation.parentItem: root
                Navigation.leftItem: spinBoxC
                Navigation.upItem: resetB
            }
        }
    }
}
