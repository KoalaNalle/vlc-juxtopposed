# Process-priority consumer — Windows 10 guest

Saved legacy high priority now applies after saved settings and the full
command line are available. Windows no longer treats a flag-like string value
as an early scheduling request. Default/no-high behavior preserves an embedding
host's inherited class. The OS2 early path is unchanged.

New process-only APIs expose real Windows readback, separate real-time from
the five base classes, and reject a failed/downgraded request while attempting
to restore the prior class. They do not enable a token privilege. Modern fields
use explicit inheritance by default. New CLI fields precede legacy CLI fields,
which precede saved policy; an explicit legacy no-high leaves the class alone.
Full native integers are checked before conversion to a class.

The identical quiet probe executable runs against the old and new Core. It
never opens media and restores its initial class immediately after observing
initialization, before teardown/I/O. Only those owned child processes change
class; the test runner, app, WSL and unrelated processes are unchanged.

| Check | Result |
| --- | --- |
| Older consumer, seven cases | Targeted negatives: saved high stays Normal, and a flag-like string incorrectly raises Above Normal; other cases pass |
| New consumer, 21 cases | All pass: inheritance, saved/CLI legacy, five base classes, CLI precedence, invalid/overflow values and actual real-time outcomes |
| Real-time, saved and CLI | This account did not obtain the exact class; error 50 is reported and the prior Normal class is preserved |
| Ordinary new-Core runtime | Five normal exits: redesigned/traditional playback, Library-on startup, local HLS and a bounded Downloads D3D11VA sample |

Core is
0a2ad359f2e2d279f0efb78acd2ae10ff7484cfd288ca08f184f7358ff282e64.
The ordinary GUI remains encoder-checkpoint
753feb6236a3e6457260ee98dfd45e93da1244f6251b5a59e371bcceddc1346f,
with generator6c50e5/transcodeb1a422. There are no private fixtures in that
ordinary runtime. Generated playback has no QML, write, visual-callback or PCR
diagnostics. The private Downloads source/capture stay outside Git and its stat
is unchanged.

The first priority candidate's class and precedence cases passed, but its
integer range silently clamped invalid/overflow requests into Inherit. That
failed matrix remains under priority-after-r1. The final option has explicit
enum choices and validates the complete integer; both bad values now report
Windows error 87 without changing an inherited Below Normal class.

[source-inputs.json](source-inputs.json) records sixteen unchanged scoped
inputs on base639b9374bc. Autotools resolves the explicit standard probe target
and source/include/test Meson syntax parses. Native Meson compilation, a
rebuilt production GUI and a complete final-source package are not claimed.
[manifest.json](manifest.json) records raw/public receipt and probe identities.
[Reproduction](../../../../test/libvlc/native_process_priority.md) documents
the API, precedence and maintained runner.

The Performance model/page still needs checked persistence and native
interaction tests. A successfully granted real-time path is unverified on this
account, and elevated scheduling under media load is not tested. Remaining
Performance fields, Recording/Extensions, full later-tuple regression,
themes/DPI, Windows 11 and clean final beta packaging remain open. The earlier
encoder/rescan runtimes and five unrelated untracked files are preserved.
Nothing is pushed or published.
