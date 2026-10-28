# Duration-repair real-writer regression preparation

Preparation only. No compiler,VLC,cache generator or decoder is launched by
the preparation or adaptation scripts. Do not execute until root authorizes
the runtime lane and the new plugin's guarded build has succeeded.

`prepare-regression.py --freeze-after-qualified-build` normalizes the approved
four-row source identity array,verifies f2a90481 mux+unchanged helper/test/recipe,
requires the future plugin-only build's successful terminal receipt and actual
compile/link/preservation guards,then pins its exact DLL SHA256. It writes a
new one-use regression configuration. It refuses to freeze before that DLL
exists or to replace a previous configuration. R1 inputs/receipts stay unchanged.

Independent actual runtime cases after review:

- `run-short.py --execute-frozen-smoke --case mp4`: default presentation only,
  require source-exact60 decoded H264 frames and116736 PCM samples,matching
  audio packet counts/payloads,and no discarded video packets. No edit-list
  override and no matched-control relaxation.
- The same producer with `--case mpegts`: source-exact60 frames and115200 PCM
  samples,including exact audio packet count/payloads.
- `--case short-matroska`: source-exact3frames and5760 FLAC samples with explicit
 1500ms mux-cache for a120ms source; retained logs must show forced header on
  deletion. Each case has a separate fresh output directory.
- `run-long.py --case original` and separately `--case reversed`:4-track real
  Matroska writes with actual opposite A/V deletion orders verified from Core's
  existing ES pointer registration/deletion logs. Then run `analyze-long.py`
  separately for each case in WSL with an outer bounded process group.
  Acceptance requires626 exact FLAC packets,2880528 source-exact PCM samples,
  all1500 expected video markers,both exact SRT exports/timings,and audio/video
  packet timelines within2ms. A runner exit alone is not acceptance.

All runtimes use strict hardlink-only full staging and replacement DLLs,Core5,
603modules,explicit avformat mux selection,normal exit and no checked diagnostics.
Sources,fixtures,full original/staged runtime inventories,producers,configs,
build/unit receipts and human configs must remain unchanged. Tool identities
are checked before/after long analysis and before short inspection.

Conservative per-lane peak budgets:205520896 bytes for a long run and100663296
bytes for a short run. Each runtime checks live C:headroom against20% reserve.
The native duration build has its separate659978947-byte budget and must not
overlap another native writer. No runtime or native build is authorized by this
file. All old MP4 source-exact failure/default-cropping receipts remain frozen.
