/*****************************************************************************
 * Copyright (C) 2020 VLC authors and VideoLAN
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
import QtQuick.Window


import VLC.MainInterface
import VLC.Style


Window {
    id: root
    visible: _componentCompleted && MainCtx.playqueuePanel.visible

    transientParent: MainCtx.intfMainWindow

    property alias playlistView: playlistView

    width: playlistView.juxtopposed ? VLCStyle.dp(400, VLCStyle.scale) : 350
    minimumWidth: playlistView.minimumWidth

    title: qsTr("Play Queue")
    color: theme.bg.primary

    property bool _componentCompleted: false

    onVisibleChanged: {
        if (visible) {
            console.assert(transientParent)
            height = transientParent.height
            minimumHeight = transientParent.minimumHeight
            MainCtx.placeDetachedQueue(this, transientParent)
            // Windows may report zero frame margins while the native window
            // is hidden. Recheck after showing it so its title bar also fits.
            Qt.callLater(function() {
                if (root.visible)
                    MainCtx.placeDetachedQueue(root, root.transientParent)
            })
        }
    }

    onClosing: {
        MainCtx.playqueuePanel.visible = false
    }

    Component.onCompleted: {
        if (MainCtx.createWindowWithoutRedirectionSurface) { // Win32
            const ret = MainCtx.createWindowWithoutRedirectionSurface(this)
            if (!ret)
                console.debug("MainCtx::createWindowWithoutRedirectionSurface(): returned false for window", this)
        }

        _componentCompleted = true
    }

    PlaylistPane {
        id: playlistView

        useAcrylic: false
        focus: true
        anchors.fill: parent

        colorContext.palette: VLCStyle.palette
    }
}
