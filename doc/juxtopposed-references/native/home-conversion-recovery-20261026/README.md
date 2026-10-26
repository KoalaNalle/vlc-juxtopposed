# Ordinary conversion recovery and shutdown - 26 October 2026

Regular Core `05ec8a7a` with GUI `8ef1ade1` completes this scoped ordinary
conversion checkpoint. The beta session continues; design, DPI and package
acceptance remain open. The form is a documented practical adaptation.

Conversion cancellation preserves a concurrently playing primary video and
removes temporary output. A corrupt owned audio fixture gives a useful error;
renaming a selected source away gives a readable-file error. Restoring that
source permits retry. Both ordinary outputs fully decode to exactly 88,200
samples, byte-identical to the source including its tail, with normal EOS.

The earlier GUI `ed1dc5f8` clipped a second terminal error in a 602 x 421
window. The repaired form reveals the result again after layout reflow while
Convert/Done owns focus. Form-field focus retains its own scrolling behavior.
The actual short-window message is visible after layout settles, and retry
succeeds. Five offscreen production-QML geometry/scale cases with repeated
errors pass both versions: they extend coverage but did not reproduce the
desktop failure and must not be described as its failing regression.

A generated one-hour source keeps the actual conversion worker active long
enough to verify header Close, cleanup, reopening and reuse. A second active
job is followed by normal Alt+F4 app shutdown. Both paths leave neither a
published partial output nor a temporary conversion directory. The ordinary
process exits zero, with unchanged source, fixtures, 602 staged modules and
binary/cache guards. Nine dvdread diagnostics are specifically retained from
probing the deliberately corrupt 67-byte source; this is not a claim of
error-free valid-media logging.

[checkpoint.json](checkpoint.json) binds observations and retained artifacts.
Gzip preserves source/report/capture bytes. Only owned generated media was used.
Earlier strict FLAC, destination race, accented/percent path, paused-primary and
header movement checks remain in the
[preceding receipt](../home-startup-rate-conversion-20261026/README.md).
Continue from [STATUS](../../../juxtopposed/STATUS.md).
