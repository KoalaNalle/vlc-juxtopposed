// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
import QtQuick
import VLC.Player
import VLC.Style

JuxtopposedSubtitleCueTimeline {
    id: root
    controller: Player.subtitleTimeline
    showStatus: false
    uiScale: VLCStyle.dp(100, VLCStyle.scale) / 100
    Binding {
        target: Player.subtitleTimeline
        property: "enabled"
        value: root.visible
        restoreMode: Binding.RestoreBindingOrValue
    }
}
