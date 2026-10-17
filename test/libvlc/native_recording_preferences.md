# Native Windows Recording preferences model

This checks the typed model, not a completed Recording page. Consumer gates
default false. MainCtx/QML mounting and actual recording, renderer and stream
jobs remain separate gates.

Read the configured Qt Makefile, followed by
modules/gui/qt/tests/native_library/recording.mk. Set PROPOSAL_OUT to a fresh
absolute owned directory and run recording-preferences-native with -j1. The
ordinary Autotools target is test_qt_recording_preferences.exe. Meson exposes an
isolated integration executable without registering an unconfigured run.

On Windows, use the travel kit's E-only Common.ps1 process environment:

~~~powershell
python test/libvlc/native_recording_preferences.py `
  --package <owned-portable-runtime> `
  --core <matching-libvlccore.dll> `
  --cache-generator <matching-vlc-cache-gen.exe> `
  --test-exe <fresh-.libs/test_qt_recording_preferences.exe> `
  --output-dir <new-owned-output-directory>
~~~

The driver stages immutable binaries on the same volume, detaches only its
replacement links, regenerates cache and requires thirteen exact ordered native
Qt passes, normal exit, no skipped cases or forced cleanup, unchanged source
Core/cache, and no temporary-directory cleanup warning.

Coverage includes opening/reloading without writes, disabled gates, strict
types/native choices, Unicode directory persistence, output-track and x264
defaults, platform-default cast quality, custom-value preservation, real Windows
config/INI sharing rejection and retry, independent discovery persistence and
accepted-OFF notification, command-line precedence, headless picker rejection
and the Advanced action signal. Consumer flags enable only this controller
mapping; they do not prove downstream media or GUI behavior.

Automatic cast quality maps to VLC's actual platform default rather than an
invented adaptive bitrate mode. Discover devices is an interface preference;
its later consumer must gate scans and stop active discovery after successful
save while preserving an active cast. Stream wizard local output must consume
the saved default and honor its explicit choice. These bridges are pending.

The native input-record-native option prefers original-stream recording; its
fallback can also copy/remux. Turning it off does not force an encode. No forced
re-encoding mode is added by this model. The original checkbox's wording is a
separate product clarification before accepting that UI behavior.

See the [checked model proof](../../doc/juxtopposed-references/native/guest-recording-model-20261017/README.md).
