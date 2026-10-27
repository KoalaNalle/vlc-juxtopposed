# Shortcut editor visibility and restart - 27 October 2026

The unfiltered keyboard-shortcut editor was blank on ordinary GUI293. A native
row-geometry test reproduces it (14 pass, 1 fail). Removing premature uniform
row heights restores the real rows on GUI301; the native suite now passes15/0/0.

Ordinary checks cover duplicate-key cancellation, retained draft and retry after
an owned save failure, saved mapping reopen, real play/pause before/after restart,
movable native popup, Reset, and restored Space on a fresh traditional player.
Reset changes only shortcut options. All four ordinary processes exit normally
with every guard intact. The deliberate save-failure run retains its expected
configuration diagnostic and the strict runner's resulting assertion failure;
the other three ordinary runs have zero error diagnostics.

See the [checkpoint](checkpoint.json) for exact identities, captures, failures
and limitations. Traditional Simple Preferences Hotkeys navigation, full action/
accessibility/DPI coverage and final-package acceptance remain separate. Continue
from [STATUS](../../../juxtopposed/STATUS.md).
