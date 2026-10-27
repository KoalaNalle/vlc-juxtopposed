// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
import QtQuick

QtObject {
    property Item focusItem
    readonly property bool editing: (focusItem instanceof TextInput || focusItem instanceof TextEdit)
                                   && !focusItem.readOnly
}
