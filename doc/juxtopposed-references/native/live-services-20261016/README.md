# Live services and publication checkpoint — 16 October 2026

The supplied Yle TV2 HLS address plays in the ordinary Windows x64 app. The
redesigned Network Stream dialog loads the HTTPS playlist, adaptive playback
fetches current segments, moving video renders, and pause/resume works. The
downloaded `.m3u8` is an old snapshot with relative segment URLs, so acceptance
uses the original live address rather than the downloaded local copy:

<https://yletv.akamaized.net/hls/live/622366/yletv2fin/yletv_video1080p50.m3u8>

This accepts the observed streaming journey, not every live-media behavior.
The live elapsed/duration labels are inconsistent for this rolling stream and
remain a beta follow-up. Audio/language/subtitle variants are not accepted by
this video-variant check. Broadcast frames and downloaded commercial artwork
are kept out of the repository.

## Real artwork services

The check uses generated silent media with public Beatles / Abbey Road tags,
including MusicBrainz release `c93e1d4c-89fa-3139-9c65-8542a39f07d1`. It contains
no embedded cover. Requests use the shipped real Lua provider, system certificate
validation, the VideoLAN MusicBrainz mirror, Cover Art Archive and Internet Archive.
No private fixture provider is installed in the ordinary package.

Cold local-only lookup returns the embedded title/artist/album and no artwork,
without HTTP requests. Initial internet lookup reaches the service but times out
at five seconds, preserving the 20-second duration and tags. A diagnostic with
a larger request budget downloads the real 86,911-byte cover in 6.672 seconds.
The library's optional lookup now permits ten seconds; file extraction remains
separate. Cancellation and fallback are still checked by the native suite.
On the final ordinary build, a fresh library import of the silent MP3 downloads
the cover into its initially empty owned portable cache. The actual Albums page
shows Abbey Road, The Beatles and the real cover. The library's saved thumbnail
and cache copy are verified independently; no personal cache supplies this import.

The live test also exposes a shared directory comparator defect: the sorting
adapter passes array-slot addresses as filenames. Consequently Lua provider
priority order varies and may try failing legacy providers before MusicBrainz.
Dereferencing the slots restores the documented filename comparator contract.
An exact extracted-body check fails on the earlier function and passes after
repair; real Windows `vlc_scandir` independently reproduces unordered names before
and the intended `00`–`03` order afterward. The portable regression is registered
in the existing Ubuntu CI job.

MusicBrainz search/release requests and Cover Art Archive now use HTTPS. This
removes avoidable plain-HTTP redirects while retaining system trust validation.
Legacy Google/Last.fm fallback services are not accepted by these checks; a
Last.fm HTTP 403 was observed before repair. Availability remains best effort.
The [Cover Art Archive API](https://musicbrainz.org/doc/Cover_Art_Archive/API) and
[release reference](https://musicbrainz.org/release/c93e1d4c-89fa-3139-9c65-8542a39f07d1/cover-art)
identify the public test data and supported service path.

## Final verification

Exact outcomes, ordinary binaries, source snapshots, service traces and profile
preservation are in [verification.json](verification.json). The final native
metadata/opening/process/subtitle suites pass 18/42/9/37 cases with zero
failures/errors/skips and normal exits. The maintained Library component passes
11 groups. Earlier failures remain recorded; they are not relabeled as passes.

The earlier [opening/folder](../library-opening-20261016/README.md) and
[metadata](../library-metadata-20261016/README.md) checkpoints retain their own
historical source/binary identities. This checkpoint supersedes their current
package and five-second optional-library-timeout status. Their baseline work is
included in the authorized commit/push, with unrelated pre-existing files left
untouched.

Full beta acceptance is still open: Windows startup with saved media-library
OFF, live timeline labels, full Figma/DPI/media/provider coverage, unfinished
categories and clean licensed release packaging remain separate work. This is
a source checkpoint, not a published beta release.

Ignored local work: `build-windows/live-validation-20261016`. The runnable
package remains `build-windows/package`.
