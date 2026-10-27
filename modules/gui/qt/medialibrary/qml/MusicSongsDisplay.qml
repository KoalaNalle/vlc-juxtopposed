/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * Visual design by Juxtopposed: https://www.youtube.com/@juxtopposed
 * Code licensed under GPLv2 or later.
 *****************************************************************************/
import QtQuick
import VLC.MainInterface
import VLC.Widgets as Widgets

Widgets.PageLoader {
    id: root
    property int displayMarginBeginning: 0
    property int displayMarginEnd: 0
    property bool enableBeginningFade: true
    property bool enableEndFade: true

    pageModel: [{
        name: "folders", default: MainCtx.juxtopposedControls,
        url: "qrc:///qt/qml/VLC/MediaLibrary/MusicFoldersDisplay.qml"
    }, {
        name: "all", default: !MainCtx.juxtopposedControls,
        url: "qrc:///qt/qml/VLC/MediaLibrary/MusicTracksDisplay.qml"
    }, {
        name: "folder",
        url: "qrc:///qt/qml/VLC/MediaLibrary/MusicTracksDisplay.qml"
    }]

    onCurrentItemChanged: {
        if (!currentItem) return
        for (const name of ["displayMarginBeginning", "displayMarginEnd", "enableBeginningFade", "enableEndFade"])
            if (currentItem[name] !== undefined)
                currentItem[name] = Qt.binding(() => root[name])
    }
}
