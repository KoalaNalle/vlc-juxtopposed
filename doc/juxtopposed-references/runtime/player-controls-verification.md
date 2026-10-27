# Native player controls and library return

14 October 2026, Windows x64, Qt 6.8.3 / Direct3D11 / DirectComposition. The tested Qt plugin is `2afa0383d817d90987977b33291fb5b0740e6c956021198b281cbd833c804015`, built at the preceding folder/editor checkpoint. The separately prepared Home/music/preset ports are absent from this binary.

## Desktop checks

The initial pinned-control diagnostic displays both bars and the actual generated video. Paused seeking updates red to blue; Back retains the six-second position in Home's mini player; Switch Player followed by resume/pause advances to eight seconds. This diagnostic uses `--qt-pin-controls --mouse-hide-timeout=600000` and does not establish ordinary floating behavior.

A subsequent launch uses `--no-qt-pin-controls --start-paused`, with the ordinary hide timeout. The real controls [hide away from the bars](player-controls/hidden.jpg), return after keyboard navigation, and return when the pointer moves to the footer. The [seek bar](player-controls/seek.jpg) changes the held picture from red to blue at about six seconds. Actual accessibility reports a Play button and position `0.3859275053304904`.

[Back](player-controls/back.jpg) returns to the actual indexed Home, with the mini player's position `0.3844444444444444` and `00:06 / 00:18`. Switch Player opens the same held video. A separate click on Play, 1.2-second wait and click on Pause produces [eight seconds](player-controls/resumed.jpg), position `0.4622222222222222`, with the actual Play button restored. The intervening exploratory seek to seventeen seconds is excluded from this resume measurement.

The fullscreen button enters the actual 2560×1440 surface. Keyboard navigation [restores its controls](player-controls/fullscreen.jpg); the fullscreen button returns to the ordinary window. Both pinned and ordinary sessions exit normally, with their PIDs absent and logger shutdown complete before replacement/build staging.

The earlier bar-less images captured ordinary hidden states. These checks establish actual rendering and input restoration; no compositor-order repair or temporary logging patch was needed. Native accessibility text can lag the screenshot immediately after navigation, so settled state is checked separately.

## Limits

The library's userdata is isolated under `build-windows/folder-picker-desktop-final`; portable VLC configuration and Qt settings remain shared. An overlapping diagnostic launch initially showed the library unavailable; it was closed before a sequential launch successfully reused the indexed test library. This does not prove concurrent library-profile support.

The [verification record](player-controls/verification.json) records the ordinary variant. Narrow/native DPI layouts, settings and queue/chapter/bookmark interactions, original design fidelity, hardware/codec variants and the new Home/music/preset integrations remain open. No whole-app score or complete milestone is claimed. Continue all milestones 0–8.
