/*****************************************************************************
 * Copyright (C) 2019 VLC authors and VideoLAN
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
import QtQml.Models
import VLC.MainInterface
import VLC.MediaLibrary

import VLC.Style
import VLC.Widgets as Widgets

Widgets.PageExt {
    id: root

    title: MainCtx.juxtopposedControls ? qsTr("Songs") : qsTr("Tracks")

    hasGridListMode: MainCtx.juxtopposedControls
    isSearchable: tracklistdisplay_id.isSearchable

    property alias model: tracklistdisplay_id.model
    property alias selectionModel: tracklistdisplay_id.selectionModel
    libraryModel: model

    property var sortModel: [
        { text: qsTr("Title"),    criteria: "title"},
        { text: qsTr("Album"),    criteria: "album_title" },
        { text: qsTr("Artist"),   criteria: "main_artist" },
        { text: qsTr("Duration"), criteria: "duration" },
        { text: qsTr("Track"),    criteria: "track_number" },
        { text: qsTr("Disc"),     criteria: "disc_number" }
    ]

    function setCurrentItemFocus(reason) {
        if (compactSongs.visible)
            compactSongs.setCurrentItemFocus(reason)
        else
            tracklistdisplay_id.setCurrentItemFocus(reason)
    }

    function positionContentAtBeginning() {
        if (compactSongs.visible)
            compactSongs.positionViewAtBeginningAnimated()
        else
            tracklistdisplay_id.positionViewAtBeginningAnimated()
    }

    FocusScope {

        anchors.fill: parent

        focus: true

        // Aliases

        property alias leftPadding: tracklistdisplay_id.leftPadding
        property alias rightPadding: tracklistdisplay_id.rightPadding

        MusicTrackListDisplay {
            id: tracklistdisplay_id

            anchors.fill: parent

            visible: model.count > 0 && !compactSongs.visible
            focus: visible

            searchPattern: root.search.pattern
            sortOrder: root.sort.order
            sortCriteria: root.sort.criteria

            Navigation.parentItem: root
            Navigation.upItem: root.header

            Navigation.cancelAction: function() {
                if (tracklistdisplay_id.currentIndex <= 0)
                    root.Navigation.defaultNavigationCancel()
                else
                    tracklistdisplay_id.currentIndex = 0;
            }


            displayMarginBeginning: root.displayMarginBeginning
            displayMarginEnd: root.displayMarginEnd

            fadingEdge.enableEndFade: root.enableEndFade
            fadingEdge.enableBeginningFade: root.enableBeginningFade
        }

        JuxtopposedVideoGrid {
            id: compactSongs
            anchors.fill: parent
            visible: MainCtx.juxtopposedControls && MainCtx.gridView && model.count > 0
            focus: visible
            model: tracklistdisplay_id.model
            selectionModel: tracklistdisplay_id.selectionModel
            dragItem: tracklistdisplay_id.dragItem
            contextMenu: MLContextMenu { model: compactSongs.model }
            fallbackArtwork: VLCStyle.noArtAlbumCover
            displayMarginBeginning: root.displayMarginBeginning
            displayMarginEnd: root.displayMarginEnd
            Navigation.parentItem: root
            Navigation.upItem: root.header
            onActionAtIndex: {
                model.addAndPlay(selectionModel.selectedIndexes)
                MainCtx.playerView = true
            }
            onItemDoubleClicked: item => {
                MediaLib.addAndPlay(item.id)
                MainCtx.playerView = true
            }
            onFocusInitializationRequested: {
                if (!selectionModel.hasSelection && model.count > 0)
                    selectionModel.select(model.index(0, 0), ItemSelectionModel.ClearAndSelect)
            }
        }

        Binding {
            target: tracklistdisplay_id.dragItem
            property: "view"
            when: compactSongs.visible
            value: compactSongs
        }

        Widgets.EmptyLabelButton {
            anchors.centerIn: parent
            visible: !tracklistdisplay_id.model.loading && (tracklistdisplay_id.model.count <= 0)
            focus: visible
            text: qsTr("No tracks found\nPlease try adding sources")
            Navigation.parentItem: root
            cover: VLCStyle.noArtAlbumCover
        }
    }
}
