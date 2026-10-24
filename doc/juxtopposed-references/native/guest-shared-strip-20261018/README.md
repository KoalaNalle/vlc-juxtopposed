# Shared Home/player title strip — 18 October 2026 guest

Home and Player now use the original shared 40px title strip, its original logo,
menu, navigation, source, separator and four window-control SVGs. The actual native
menu retains the navigation toggle. Player navigation reuses the existing typed
sidebar/pins routes and returns to Home/Recent without stopping paused media.
The media-name option follows VLC's native name, including its filename fallback.

A live source-switch test reproduced a stale title: PlayerController notified QML
before storing the new name. Storing the name before emitting the property signal
fixes the retained strip; the actual FLAC/video transitions now match the native
window caption. Traditional controls remain on their original path.

## Accepted scoped checks

All **13 Windows 10 native processes / 2,012 checks** pass normally, with no forced
cleanup and zero Qt warnings. Per-process counts and exact identities are in
[results.json](results.json) and [native.json](native.json). They cover actual
resource-byte identity, 18/24px SVG roots, scaled 40px height and 6/51px slots,
centered labels, native WM_NCHITTEST results for minimize/maximize/close, actual
maximize/restore/fullscreen/minimize/Close, real keyboard menu interaction,
paused Home/source/drawer navigation, live title changes and the seven existing
sidebar/history processes. Interface scales 1/1.5/2 are application settings;
they do not establish operating-system monitor DPI coverage.

Five ordinary playback cases pass normally: redesigned, traditional, Library,
local HLS and a bounded read-only E:/Downloads sample. All Qt warnings are zero.
The offered sample's size/time is unchanged; its name and frames stay private.
The ordinary GUI contains no private journey/HTTP-test/title-asset or temporary
diagnostic marker. The prior runtime and generated media are preserved.

Twelve component geometry cases cover Home/player at wide/narrow widths and
1/1.5/2 interface scale, with supplied controller seams. They are separate from
native window acceptance. A deliberate 44px negative fails the original 40px
criterion. Ordinary 1,935/private six/component thirteen frozen inputs match.
Retained 458 Core/eight pin-store/four thread-test inputs and the five original
untracked files match; the earlier 34 thread cases were not rerun here.

## Original references and reproduction

Cached full Home6261:15078 and Player247:445 design contexts/screenshots accompany
this proof under references/. No new Figma MCP reads were spent. The nine original
SVG slots are byte-identical to the downloaded exports; six common Home/player
exports are identical. Menu art retains an 18px root in its original 24px frame;
other roots remain 24px. Original paths were not rewritten. Effective wrapper
scales, positions and native resource identities are checked. The two existing
identical folder/close files are reused. Original design/font/VLC licenses remain.

The full cached/native captures were visually compared for the title strip.
Different media/content and window sizes are intentional test inputs. These
captures do not accept the whole Home/player composition or certify a full
independent fidelity score. Runtime artwork is SVG, never a reference screenshot.

All source, builds, profiles, caches, downloads and output remain on E:. Use
E:/Codex/migration-20261016/scripts/Common.ps1 and VLC-Juxtopposed-Travel. Builds use
JOBS=1/make -j1 under the shared build lock. Only one heavy job ran at a time.
The immutable staged runtime replaces only its new GUI hardlink. Reproduction
scripts preserve exact E: paths; do not substitute a different drive. The runtime
is E:/Modding/vlc-juxtopposed/build-windows/beta-shared-strip-20261018/ordinary-runtime-r1.
Its exact Core/GUI/ML hashes are in [runtime.json](runtime.json).

## Rejected attempts and remaining work

Private R1 rejected duplicate QML visibility (exit2, no native run). R2 exposed
raw metadata-title versus native fallback semantics. R3 passed five redesigned
runs but its traditional readiness helper incorrectly required redesigned objects.
R4 corrected that helper and reproduced the real retained-name notification bug;
[the negative](negative-retained-title-r4.json) remains rejected. R5 supplies the
accepted native/ordinary source. Early component-harness quoting/metaobject/enum/
font-lifetime failures were test-only; one dynamic PySide metaobject attempt ended
in an access violation. A real inherited 2px button-padding offset was corrected.
No Windows machine crash or new detector payload was received in this work.

Full beta remains incomplete. Native system drag/double-click, Windows 11 Snap,
OS DPI/monitor variants, full Light/System/accent/accessibility/keyboard states,
short-height drawer behavior, CJK font fallback, transparent backdrop fidelity,
and unshown restore/fullscreen artwork variants remain open. The drawer is a
functional adaptation using the original Home pane, not an accepted original
expanded-player reference. Remaining Home/Recent states, independent whole-screen
fidelity and a clean matched licensed package still require work. Videos/Music
original icons await the pending two-read budget decision; no extra read was made.
No push, publication or designer outreach occurred. The original security-trip
cause remains unknown; no new automatic approval rejection was received.
