/*****************************************************************************
 * Copyright (C) 2022 VLC authors and VideoLAN
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
import VLC.Util

/**
 * player toolbars visibility state machine
 *
 * @startuml
 * state Visible {
 *    state NormalVisible {
 *        state AudioMode {
 *        }
 *       state VideoMode {
 *       } 
 *    }
 *    state Locked {
 *    }
 * }
 * state Hidden {
 *    state NormalHidden {
 *    }
 * }
 * @enduml
 *
 */

FSM {
    id: fsm
 
    //incoming signals
    signal askShow()
    signal mouseMove()
    signal keyboardMove()
    signal lock()
    signal unlock()
    signal forceUnlock()
    signal timeout()
    signal updateVideoEmbed()
 
    //exposed internal states
    property alias isVisible: fsmVisible.active

    property int lockCount: 0 // Track the number of locks
    readonly property int autoHideMode: MainCtx.controlAutoHideMode
    readonly property bool fullscreen: MainCtx.intfMainWindow.visibility === Window.FullScreen
    // This setting affects the bottom controls, not the title/exit controls.
    // Locks, hover and navigation may reveal toolbars without overriding it.
    readonly property bool controlsAllowed: !fullscreen || MainCtx.showFullscreenControls
    property bool controlsForcedVisible: false
    readonly property bool controlsVisible: controlsAllowed && (isVisible || controlsForcedVisible)
    // -1 preserves the established video-only legacy state machine. New
    // modes apply equally to music/video, without changing the docking layout.
    readonly property bool automaticallyHide: autoHideMode < 0 ? MainCtx.hasEmbededVideo
        : autoHideMode === 2 || (autoHideMode === 1 && fullscreen)
    readonly property bool interactionMayReveal: autoHideMode >= 0 || !Player.isInteractive
    property int timeoutDuration: MainCtx.controlsHideTimeout
    onAutomaticallyHideChanged: updateVideoEmbed()

    initialState: ((autoHideMode < 0 && Player.isInteractive && MainCtx.hasEmbededVideo))
                  ? fsmHidden : fsmVisible
 
    signalMap: ({
        askShow: fsm.askShow,
        mouseMove: fsm.mouseMove,
        keyboardMove: fsm.keyboardMove,
        lock: fsm.lock,
        timeout: fsm.timeout,
        unlock: fsm.unlock,
        updateVideoEmbed: fsm.updateVideoEmbed,
    })
 
    FSMState {
        id: fsmVisible

        initialState: fsmNormalVisible
 
        FSMState {
            id: fsmNormalVisible

            initialState: fsm.automaticallyHide ? fsmVideoMode : fsmAudioMode
 
            transitions: ({
                lock: {
                    action: () => { fsm.lockCount++; },
                    target: fsmLocked
                }
            })
 
            FSMState {
                id: fsmAudioMode
 
                transitions: ({
                    updateVideoEmbed: {
                        guard: () => fsm.automaticallyHide,
                        target: fsmVideoMode
                    }
                })
            }
 
            FSMState {
                id: fsmVideoMode

                function enter() {
                    timeout.interval = fsm.timeoutDuration;

                    if (fsm.interactionMayReveal)
                        timeout.restart();
                }

                function exit() {
                  timeout.stop();
                }
 
                transitions: ({
                    updateVideoEmbed: {
                        guard: () => !fsm.automaticallyHide,
                        target: fsmAudioMode
                    },
                    askShow: {
                        guard: () => fsm.automaticallyHide,
                        target: fsmHidden,
                    },
                    timeout: {
                        target: fsmHidden,
                    },
                    mouseMove: {
                        guard: () => fsm.interactionMayReveal,
                        action: () => { 
                            fsm.timeoutDuration = MainCtx.controlsHideTimeout;
                        },
                        target: fsmVideoMode
                    },
                    keyboardMove: {
                        guard: () => fsm.interactionMayReveal,
                        action: () => { 
                            fsm.timeoutDuration = MainCtx.controlsHideTimeout * 2
                        },
                        target: fsmVideoMode
                    }
                })
            }
        }
 
        FSMState {
            id: fsmLocked
 
            transitions: ({
                lock: {
                    action: () => { fsm.lockCount++; }
                },
                unlock: [{
                    action: () => { fsm.lockCount--; },
                    guard: () => fsm.lockCount === 1,
                    target: fsmNormalVisible,
                }, {
                    guard: () => fsm.lockCount > 1,
                    action: () => { fsm.lockCount--; }
                }],

            })
        }
    }
 
    FSMState {
        id: fsmHidden

        function enter() {
            fsm.lockCount = 0;
            fsm.forceUnlock();
        }

        transitions: ({
            updateVideoEmbed: {
                guard: () => !fsm.automaticallyHide,
                target: fsmAudioMode
            },
            mouseMove: {
                guard: () => fsm.interactionMayReveal,
                action: () => {
                    fsm.timeoutDuration = MainCtx.controlsHideTimeout
                },
                target: fsmVisible
            },
            keyboardMove: {
                guard: () => fsm.interactionMayReveal,
                action: () => {
                    fsm.timeoutDuration = MainCtx.controlsHideTimeout * 2
                },
                target: fsmVisible
            },
            askShow: {
                target: fsmVisible
            },
        })
    }
 
    Timer {
        id: timeout
        interval: fsm.timeoutDuration
        repeat: false
        onTriggered: {
            fsm.timeout();
        }
    }
}
