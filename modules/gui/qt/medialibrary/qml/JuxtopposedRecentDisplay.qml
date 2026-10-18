/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * Code licensed under GPLv2 or later.
 *****************************************************************************/
import QtQuick
import VLC.Widgets as Widgets

Widgets.PageLoader {
    id: root
    Accessible.name: qsTr("Recent activity")
    pageModel: [{name: "media", default: true, component: mediaPage},
                {name: "playlists", component: playlistPage}]
    Component { id: mediaPage; JuxtopposedRecentPage {} }
    Component { id: playlistPage; JuxtopposedRecentPage { showPlaylists: true } }
    property int displayMarginBeginning: 0
    property int displayMarginEnd: 0
    property bool enableBeginningFade: true
    property bool enableEndFade: true
    onCurrentItemChanged: {
        if (!currentItem) return
        currentItem.displayMarginBeginning = Qt.binding(() => root.displayMarginBeginning)
        currentItem.displayMarginEnd = Qt.binding(() => root.displayMarginEnd)
        currentItem.enableBeginningFade = Qt.binding(() => root.enableBeginningFade)
        currentItem.enableEndFade = Qt.binding(() => root.enableEndFade)
    }
}
