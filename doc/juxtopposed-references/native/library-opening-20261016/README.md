# Library opening and subtitle desktop checkpoint

Historical checkpoint: the later [metadata/artwork acceptance](../library-metadata-20261016/README.md)
enables the fourth Library consumer and supersedes the optional-metadata and
current-package status below. Its final metadata/opening/process/subtitle suites
pass 18/42/9/37; complete Library/beta acceptance remains open.

16 October 2026. Local Windows x64 checkpoint based on
`6a258e99a89c745b8ba2e28865c3045b14409db8`. The bounded file-opening/folder slice
and pending subtitle desktop checks pass. The complete Library surface and the
publishable beta remain incomplete. These changes have not been committed or
pushed.

## Result and implementation

The Library page now enables the tested single-instance/file-manager policy,
append/replace policy and Windows hidden-file controls. Legacy custom values
retain their previous behavior. Include-subfolders behavior is also verified.
Optional metadata/artwork fetching remains unavailable; its separate
cancellation, fallback and provider-scope work is unfinished.

Explicit cross-process opening resolves relative filenames in the sender's
working directory, preserves per-file input options, and checks receiver
compatibility before sending. The receiver validates and prepares the request
before changing the queue. Replace inserts the new items before removing the old
queue; append preserves current playback. Bounded delivery errors report whether
the queue may already have changed and never automatically replay the request.

Actual folder tests exposed three additional production defects:

- The external preparser returned a separately deserialized root item, preventing
  the playlist from identifying the submitted folder. Its callback now retains
  the original root identity.
- Optional folder-artwork lookup could delay delivery of already parsed children.
  Container expansion now completes independently of that optional lookup.
- The preparser child did not receive the effective hidden-file preference.
  Each parse request now carries a typed snapshot, including explicit item
  overrides, without changing the caller's options or child-wide state.

The hidden-policy request field is optional when reading older JSON, with its
legacy default retained. The Core, preparser executable and JSON serializer
nevertheless form a matched build tuple and must be staged together. The ordinary
package contains no private test providers, probe modules or diagnostic exports.

## Verification

The [receipt](verification.json) records source and binary identities, result
counts, screenshot identities, restoration and remaining gates.
The [final source/artifact audit](final-audit.json) checks the current production
sources against ordinary-build snapshots, freshly derived private test inputs
against the inputs actually compiled, the accepted executable identities and
staged package tuple, preserved personal files and documentation links.

| Check | Result | Evidence |
| --- | --- | --- |
| Native opening/folder suite | 42 passed, no failures/errors/skips; normal exit | [Final output](opening-final.txt), [receipt](opening-final.json) |
| Same unchanged opening executable against the earlier runtime | 3 failures in 42 cases, all at the external hidden-policy boundary; normal exit | [Before output](opening-before.txt), [receipt](opening-before.json) |
| Actual sender/receiver processes | 9 passed, no failures/errors/skips; normal exit | [Output](process-final.txt), [receipt](process-final.json) |
| Unchanged native subtitle regression | 37 passed, no failures/errors/skips; normal exit | [Output](subtitles-final.txt), [receipt](subtitles-final.json) |
| Extracted production delivery error branches | 16 receiver and 8 sender cases passed | [Receipt](ipc-errors.json) |
| Integrated QML components | Library 11 groups; Subtitles 15 groups; Host 24 checks, 12 directed transitions and 9 busy guards; zero Qt messages | [Library](components-library.json), [Subtitles](components-subtitles.json), [Host](components-host.json) |

The earlier root-identity and optional-artwork failures are retained separately:
[root output](folder-root-before.txt), [artwork output](folder-artwork-before.txt).
The native external-parser fixture explicitly rejects unsupported provider-counter
tests rather than silently passing them. Those metadata tests are outside the
selected opening scope. Reproduction instructions and maintained fixture sources
are in [native_library](../../../../modules/gui/qt/tests/native_library/README.md).

Both jobs of the baseline commit's remote run
[38765572111](https://github.com/KoalaNalle/vlc-juxtopposed/actions/runs/38765572111)
passed. Remote CI has not run for these unpushed local changes. Native Windows
fixtures were run locally; the portable delivery-error driver is registered in
the maintained Ubuntu CI job.

## Actual desktop checks

Seven ordinary app launches used monitor 2 and quit normally. Six separately
saved logs end with the logger's normal stop marker; the first launch was checked
at the time without a separately retained final log.

- Include subfolders restored after restart and opened both the top-level and
  nested silent media: [queue](03-recursive-folder-queue.png).
- Library instance/file-manager, append and hidden settings persisted across
  restart: [restored settings](14-library-restored.png). Explicit replace saved
  its native value: [settings](17-hidden-off-replace-saved.png).
- With hidden files enabled, opening the owned folder added its dot-named,
  Windows-hidden and visible files: [queue](16-hidden-on-fixed.png). Disabling
  hidden files made the next opening add only the visible file; already queued
  entries remained present: [next opening](18-hidden-off-next-folder.png).
- Top subtitle alignment survived restart and rendered embedded
  [English](04-top-english.png) and [Finnish](05-finnish-feedback-immediate.png)
  SRT. The V shortcut selected Finnish; seeking decoded its long cue.
- The saved 2000 ms shortcut overlay was visible at 1295 ms and gone at 2463 ms:
  [before expiry](08-feedback-before-expiry.png),
  [after expiry](09-feedback-after-expiry.png).
- Shortcut feedback disabled persisted across restart. Seeking worked
  [without its overlay](12-no-shortcut-overlay.png), while the independent
  [filename overlay](11-filename-with-feedback-off.png) remained operational.

Subtitle and initial Library persistence checks used the earlier r4 Core with
r2/r5 Qt. The final hidden-file journey used matched r6 components. The final
37-case subtitle regression also used r6; the screenshots do not claim every
journey used the final tuple.

All test media were owned generated fixtures. The original portable profile was
restored, retaining only monitor-2 window geometry. The two checked personal VLC
configuration files remained byte-identical. No development VLC or preparser was
left running. Raw `vlc.exe` and actual sender processes supplied the instance
checks: the development launcher deliberately forces independent instances.

## Remaining beta gates

- Optional metadata/artwork cancellation, fallback and provider-scope acceptance.
- Actual Windows media-library startup OFF: the Windows entry point still injects
  `--media-library`, overriding the saved preference. Native libvlc preservation
  tests do not accept this launcher behavior.
- An earlier isolated parser run displayed a transient Runtime Library window.
  Its parent suite passed, but app-access approval timed out before the window
  could be inspected. No retained diagnostic details or matching Windows error
  event were obtained. Later parent suites and ordinary app exits pass; that
  does not establish every child-process exit as healthy.
- Dark styling of the legacy Custom On/Off menu, broader Figma/DPI/media and
  subtitle format/preset/timeline acceptance, other unfinished beta categories,
  and the clean licensed package and final remote CI/publication.

The twelve archived shadow proposal files were not read, run or reused in this
checkpoint. Their ZIP remains unchanged. Computer Use is paused at the user's
request while the monitors are occupied; the completed desktop evidence above
was obtained before that pause.
