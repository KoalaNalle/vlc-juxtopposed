# Audio-only folder: actual Windows journey

14 October 2026. This records the native application in the existing test profile,
using the built Home Qt plugin `88452491…ed99c6`. The eight JPEG captures are
unchanged bytes from the Windows application; their dimensions and hashes are
in [verification.json](verification.json). Folder-picker captures, private
preferences, library databases and raw host logs are excluded.

Music → Add Folder used the actual Windows folder picker to add a generated
directory containing two real FLAC files. Its name contains Unicode and `#/%`.
After indexing, the new Home folder card opened both actual metadata titles.
Opening a track entered the source-folder music player paused at zero; playback
then pause produced native progress `0.0522445`. Back retained paused audio in
the mini-player. Normal quit/restart restored the Home card, and its route still
opened both tracks.

This is a warm-profile added-root journey. It does not prove fresh-user setup,
all visit/reset/opt-out/error routes, supported-format coverage, final packaging,
or Figma fidelity. The original Home reference and its full-scroll state remain
separate visual gates. The missing-art fallback visible here still needs its
prepared dark-art correction and native acceptance.
