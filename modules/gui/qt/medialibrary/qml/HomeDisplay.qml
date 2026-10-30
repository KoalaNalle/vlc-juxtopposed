/******************************************************************************
 * Copyright (C) 2024 VLC authors and VideoLAN
 *
 * Author: Ash <ashutoshv191@gmail.com>
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
 ******************************************************************************/
import QtQuick

import VLC.MediaLibrary

import VLC.MainInterface
import VLC.Widgets as Widgets
import VLC.Util

Widgets.PageLoader {
    id: root

    pageModel: [{
        name: "base",
        component: MainCtx.juxtopposedControls ? juxtHomeComponent : homeComponent,
        default: true
    }, {
        name: "continueWatching",
        component: MainCtx.juxtopposedControls ? juxtVideoHistoryComponent : continueWatchingComponent
    }, {
        name: "favorites",
        component: favoritesComponent
    }, {
        name: "newVideo",
        component: newVideoComponent
    }, {
        name: "newMusic",
        component: newMusicComponent
    }, {
        name: "listenAgain", component: juxtAudioHistoryComponent
    }, {
        name: "watchAgain", component: juxtWatchedVideoHistoryComponent
    }, {
        name: "frequentFolders", component: juxtFoldersComponent
    }, {
        name: "frequentPlaylists", component: juxtPlaylistsComponent
    }, {
        name: "frequentArtists", component: juxtArtistsComponent
    }, {
        name: "folder", component: juxtFolderComponent
    }]

    property int displayMarginBeginning: 0
    property int displayMarginEnd: 0

    property bool enableBeginningFade: true
    property bool enableEndFade: true

    Accessible.role: Accessible.Client
    Accessible.name: qsTr("Home View")
    AccessibleCompat.id: "homeDisplay"


    function openCollection(kind, entry, reason) {
        if (kind === "folder")
            History.push(["home", "folder"], {parentId: entry.id, title: entry.title}, reason)
        else if (kind === "playlist")
            History.push(["playlists", "list"], {parentId: entry.id, title: entry.title}, reason)
        else
            History.push(["music", "artists", "albums"], {artistId: entry.id}, reason)
    }
    Component {
        id: juxtHomeComponent
        JuxtopposedHomePage {
            focus: true
            onSeeAllButtonClicked: (category, reason) => History.push([...root.pagePrefix, category], reason)
            onCollectionRequested: (kind, entry, reason) => root.openCollection(kind, entry, reason)
        }
    }
    Component { id: juxtVideoHistoryComponent; JuxtopposedHomeHistoryPage { isVideo: true; focus: true } }
    Component { id: juxtWatchedVideoHistoryComponent; JuxtopposedHomeHistoryPage { isVideo: true; completedVideos: true; focus: true } }
    Component { id: juxtAudioHistoryComponent; JuxtopposedHomeHistoryPage { isVideo: false; focus: true } }
    Component {
        id: juxtFoldersComponent
        JuxtopposedHomeCollectionPage {
            category: "frequentFolders"; focus: true
            onCollectionRequested: (kind, entry, reason) => root.openCollection(kind, entry, reason)
        }
    }
    Component {
        id: juxtPlaylistsComponent
        JuxtopposedHomeCollectionPage {
            category: "frequentPlaylists"; focus: true
            onCollectionRequested: (kind, entry, reason) => root.openCollection(kind, entry, reason)
        }
    }
    Component {
        id: juxtArtistsComponent
        JuxtopposedHomeCollectionPage {
            category: "frequentArtists"; focus: true
            onCollectionRequested: (kind, entry, reason) => root.openCollection(kind, entry, reason)
        }
    }
    Component { id: juxtFolderComponent; JuxtopposedHomeFolderPage { focus: true } }

    Component {
        id: homeComponent

        HomePage {
            focus: true

            onSeeAllButtonClicked: (name, reason) => {
                History.push([...root.pagePrefix, name], reason)
            }
        }
    }

    Component {
        id: continueWatchingComponent

        Widgets.PageExt {
            id: continueWatchingPage

            title: qsTr("Continue Watching")

            VideoAll {
                id: continueWatching

                anchors.fill: parent

                focus: true

                model: MLRecentVideoModel {
                    ml: MediaLib

                    sortCriteria: continueWatchingPage.sort.criteria
                    sortOrder: continueWatchingPage.sort.order
                    searchPattern: continueWatchingPage.search.pattern

                    readonly property bool needToNavigateBack: !loading && count === 0
                                                               && continueWatchingPage.search.pattern === ""

                    onNeedToNavigateBackChanged: {
                        if (needToNavigateBack)
                            History.previous(Qt.BacktabFocusReason)
                    }
                }

                sectionProperty: model.sortCriteria === "title" ? "title_first_symbol" : ""

                contextMenu: MLContextMenu {
                    model: continueWatching.model

                    showPlayAsAudioAction: true
                }

                displayMarginBeginning: root.displayMarginBeginning
                displayMarginEnd: root.displayMarginEnd

                enableBeginningFade: root.enableBeginningFade
                enableEndFade: root.enableEndFade
            }
        }
    }

    Component {
        id: favoritesComponent

        Widgets.PageExt {
            id: favoritesPage

            title: qsTr("Favorites")

            MediaView {
                focus: true

                anchors.fill: parent

                model: MLMediaModel {
                    favoriteOnly: true

                    ml: MediaLib

                    sortCriteria: favoritesPage.sort.criteria || "insertion"
                    sortOrder: favoritesPage.sort.order
                    searchPattern: favoritesPage.search.pattern
                }

                displayMarginBeginning: root.displayMarginBeginning
                displayMarginEnd: root.displayMarginEnd

                enableBeginningFade: root.enableBeginningFade
                enableEndFade: root.enableEndFade
            }
        }
    }

    Component {
        id: newVideoComponent

        Widgets.PageExt {
            id: newVideoPage

            title: qsTr("New Videos")

            VideoAll {
                id: newVideo

                anchors.fill: parent

                focus: true

                model: MLVideoModel {
                    ml: MediaLib

                    sortCriteria: newVideoPage.sort.criteria || "insertion"
                    sortOrder: newVideoPage.sort.order
                    searchPattern: newVideoPage.search.pattern
                }

                sectionProperty: model.sortCriteria === "title" ? "title_first_symbol" : ""

                contextMenu: MLContextMenu {
                    model: newVideo.model

                    showPlayAsAudioAction: true
                }

                displayMarginBeginning: root.displayMarginBeginning
                displayMarginEnd: root.displayMarginEnd

                enableBeginningFade: root.enableBeginningFade
                enableEndFade: root.enableEndFade
            }
        }
    }

    Component {
        id: newMusicComponent

        Widgets.PageExt {
            id: newMusicPage

            title: qsTr("New Music")

            MediaView {
                focus: true

                anchors.fill: parent

                model: MLAudioModel {
                    ml: MediaLib

                    sortCriteria: newMusicPage.sort.criteria || "insertion"
                    sortOrder: newMusicPage.sort.order
                    searchPattern: newMusicPage.search.pattern
                }

                displayMarginBeginning: root.displayMarginBeginning
                displayMarginEnd: root.displayMarginEnd

                enableBeginningFade: root.enableBeginningFade
                enableEndFade: root.enableEndFade
            }
        }
    }
}
