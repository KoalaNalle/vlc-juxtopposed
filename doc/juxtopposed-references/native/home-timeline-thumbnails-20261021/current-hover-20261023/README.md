# Current ordinary preview hover, 23 October 2026

The current ordinary build keeps the actual timeline preview visible through an
untouched paused hold of 345.984 seconds. The main video stays paused at41
seconds and the card shows its41-second image at the reviewed start/middle/end.
Built-in named Qt hover logging has no intervening false transition and the
window-focus log has no deactivation during the hold. Pixels are reviewed at
those points; this is not continuous screen recording.

An intentional click on the title strip moves the pointer away and closes the
card, with a matching true-to-false hover transition. Returning to the timeline
restores the picture card with a matching false-to-true transition. The timeline
click also performs its ordinary seek; no special pointer helper is used.
Playback resumes and advances while the preview remains visible, then the app
quits normally with no checked GUI diagnostics. No private observer or GUI/Core
hooks are linked, and no functional hover change was required.

Tuple: Core0ef9dde2 / GUId0ff14c6 / MKVf46d8d15 / WASAPI4e4e. The
2,373 actual frozen input copies, runtime and fixture verify.
This refreshes the controlled hover result after the subtitle repairs.

The original historical disappearance remains unconfirmed: there was no cursor/
focus trace then, and the user later said they might have moved the mouse.
Both loss of hover and loss of window activity are explicit production preview
eligibility conditions. These observations establish expected behavior on this
run; they do not prove what happened during the earlier event. Retain that
uncertainty rather than inventing a timeout fix. Broader thumbnail visual,
accessibility/DPI, media/source-switch and full-beta gates remain separate.
No install, commit/push, release/upload or shutdown occurred.

- [Reviewed observations and scope](review.json)
- [Ordinary process/log verification](verification.json)
- [Source manifest](inputs-current-hover-r7.json)
- [Artifact hashes](checkpoint.json)
