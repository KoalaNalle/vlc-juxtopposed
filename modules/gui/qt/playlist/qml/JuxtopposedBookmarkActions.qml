// SPDX-License-Identifier: GPL-2.0-or-later
// Loaded only while the optional MediaLibrary QML module is available.
import VLC.MainInterface
import VLC.MediaLibrary
import VLC.Player

MLBookmarkModel {
    player: Player
    ml: MediaLib
}
