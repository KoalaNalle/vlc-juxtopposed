// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtQuick.Window
import QtTest
import VLC.MainInterface
import VLC.PlayerPolicyFixture

TestCase {
    id: test
    name: "InterfaceToolbarPolicy"
    Component { id: factory; PlayerToolbarVisibilityFSM {} }
    function make() {
        const value = createTemporaryObject(factory, test)
        verify(value !== null)
        compare(value.started, true)
        return value
    }
    function init() {
        MainCtx.controlAutoHideMode = 0
        MainCtx.controlsHideTimeout = 100000
        MainCtx.hasEmbededVideo = false
        MainCtx.intfMainWindow.visibility = Window.Windowed
        Player.isInteractive = false
    }
    function test_modes_data() {
        const rows = []
        for (let mode = -1; mode <= 2; ++mode)
            for (const video of [false, true])
                for (const fullscreen of [false, true])
                    rows.push({tag: `${mode}-${video}-${fullscreen}`, mode, video, fullscreen})
        return rows
    }
    function test_modes(row) {
        MainCtx.controlAutoHideMode = row.mode
        MainCtx.hasEmbededVideo = row.video
        MainCtx.intfMainWindow.visibility = row.fullscreen ? Window.FullScreen : Window.Windowed
        const fsm = make()
        const shouldHide = row.mode < 0 ? row.video
            : row.mode === 2 || (row.mode === 1 && row.fullscreen)
        compare(fsm.isVisible, true)
        fsm.timeout()
        compare(fsm.isVisible, !shouldHide)
        fsm.mouseMove()
        compare(fsm.isVisible, true)
        fsm.keyboardMove()
        compare(fsm.isVisible, true)
    }
    function test_fullscreenTransitionRevealsWindowed() {
        MainCtx.controlAutoHideMode = 1
        const fsm = make()
        fsm.timeout(); compare(fsm.isVisible, true)
        MainCtx.intfMainWindow.visibility = Window.FullScreen
        fsm.timeout(); compare(fsm.isVisible, false)
        MainCtx.intfMainWindow.visibility = Window.Windowed
        compare(fsm.isVisible, true)
        fsm.timeout(); compare(fsm.isVisible, true)
    }
    function test_nestedLocksStopTimerAndRelease() {
        MainCtx.controlAutoHideMode = 2
        const fsm = make()
        fsm.lock(); fsm.lock(); compare(fsm.lockCount, 2)
        fsm.timeout(); compare(fsm.isVisible, true)
        fsm.unlock(); compare(fsm.lockCount, 1)
        fsm.timeout(); compare(fsm.isVisible, true)
        fsm.unlock(); compare(fsm.lockCount, 0)
        fsm.timeout(); compare(fsm.isVisible, false)
        fsm.mouseMove(); compare(fsm.isVisible, true)
    }
    function test_audioVideoTransitionUsesActualPolicy() {
        MainCtx.controlAutoHideMode = 2
        const fsm = make()
        fsm.timeout(); compare(fsm.isVisible, false)
        MainCtx.hasEmbededVideo = true
        fsm.updateVideoEmbed()
        fsm.mouseMove(); compare(fsm.isVisible, true)
        fsm.timeout(); compare(fsm.isVisible, false)
        MainCtx.hasEmbededVideo = false
        fsm.updateVideoEmbed()
        fsm.mouseMove(); compare(fsm.isVisible, true)
        fsm.timeout(); compare(fsm.isVisible, false)
    }
    function test_legacyInteractiveAndExplicitNever() {
        MainCtx.controlAutoHideMode = -1
        MainCtx.hasEmbededVideo = true
        Player.isInteractive = true
        const fsm = make()
        compare(fsm.isVisible, false)
        fsm.mouseMove(); compare(fsm.isVisible, false)
        fsm.askShow(); compare(fsm.isVisible, true)
        fsm.askShow(); compare(fsm.isVisible, false)
        // The service applies settings at restart; this mutable stub change
        // separately exercises the actual consumer's hidden-policy transition.
        MainCtx.controlAutoHideMode = 0
        compare(fsm.isVisible, true)
        fsm.askShow(); fsm.timeout(); compare(fsm.isVisible, true)
    }
    function test_realTimer_data() {
        return [{tag:"zero", delay:0}, {tag:"nonPreset777", delay:777}, {tag:"maximum10000", delay:10000}]
    }
    function test_realTimer(row) {
        MainCtx.controlAutoHideMode = 2
        MainCtx.controlsHideTimeout = row.delay
        const fsm = make()
        compare(fsm.isVisible, true)
        if (row.delay > 100) {
            wait(100)
            compare(fsm.isVisible, true)
        }
        tryCompare(fsm, "isVisible", false, row.delay + 1500)
    }
}
