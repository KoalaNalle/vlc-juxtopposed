# Main-package subtitle desktop checkpoint

15 October 2026. This is bounded desktop acceptance of the subtitle repair,
not final beta acceptance.

The ordinary main package was updated with Core `31f252f3`, Qt `e4e34380`
and FreeType `d102b097`. The prior package/profile remains backed up locally.
The independently rebuilt renderer had already passed the unchanged
[37-case native suite](../subtitle-shadow-repair-20261015/verification.json).

Computer Use exercised two ordinary Windows app launches on monitor 2:

- Open the authored Subtitles settings page.
- Disable Shadow and enter red text color through actual keyboard input.
- Quit normally, restart, and open a generated ten-minute video with matching SRT.
- Observe automatic external subtitle loading and visible red text without shadow.
- Reopen settings and observe the saved red color and Shadow OFF values.
- Continue stable playback through 1 minute 39 seconds, then quit normally.

Both shutdowns completed with the logger stopped. The original package profile
was restored afterward, retaining monitor-2 window geometry. The tested profile
is preserved locally. The two checked existing personal VLC configuration files
retained their pre-test hashes.

[Rendered subtitle](01-red-shadow-off.png) and
[settings after restart](02-settings-after-restart.png) show the actual app.
The first capture is at 19 seconds; the second is during the later settings check.

Native GUI vertical-position, feedback duration/visibility and embedded-language
selection remain pending. Broader formats, preset journeys, final Figma checks
and final package acceptance also remain open. The Library source additions are
newer than this tested main package and do not inherit its native-build result.
