# Native desktop FLAC conversion — 14 October 2026

The C8 development package completed one conversion through the actual Windows
source and destination pickers. Its created FLAC independently decoded to all
1,323,000 expected mono PCM frames, matching every source byte and the final 64
samples. Completed-state modal shortcuts and normal application exit were also
observed. This is a scoped desktop result for the shared-style conversion
adaptation, not final-beta or original Figma-form acceptance.

Root recorded the actual app interaction in `results/desktop-observations.json`.
The reviewer inspected those observations, the four exact captures, build/
runtime identities and saved independent native decoder evidence without
repeating desktop actions or launching native tests.

| Observed step | Result and evidence |
| --- | --- |
| Open conversion | Actual shared modal, initial empty inputs: `captures/01-native-conversion-idle.jpg`. |
| Choose files | Actual native source/destination pickers selected the generated waveform and a new FLAC path: `captures/02-native-conversion-ready.jpg`. Private picker views are excluded. |
| Convert | Native operation reported complete: `captures/03-native-conversion-complete.jpg`; exact content checked separately below. |
| Completed-state keys | Up/Down preserved primary volume scalar 1. Space activated Done and closed the modal; the selected primary fixture remained paused at progress 0. These dynamic observations are recorded by Root, not inferred from a screenshot alone. |
| Outside-modal key | The next Space intentionally resumed that same primary fixture. Root later intentionally paused it near 56 seconds; `captures/04-native-modal-closed-shortcuts-restored.jpg` shows that later state, not the immediate paused-at-zero state after Done. |
| Quit | Normal quit was requested; Root verified exit code 0 rather than treating a stale process listing as a live application. The selected GUI log excerpt contains `QApp exec() finished` and logger shutdown. |

The selected primary fixture was `subtitle-style.mp4`, paused at progress 0 and
volume scalar 1 during the picker/conversion observations. This run does not
establish active primary playback throughout encoding. Encoding/Cancelling
keys, complete queue/drop behavior, restart/all lifecycle cases, other media/
profiles and Figma fidelity remain pending.

## Independent content check

The frozen checker has SHA-256
`2d0123c82aa9a89d34f78e3d674f1ab73b62bf5428181dc280544a004d318821`.
It imports the unchanged current LibVLC4 ABI bindings from
`test/libvlc/native_flac_drain.py`, SHA-256
`3ff0fc044cf2fdce8d20cc864698c043912b353f21660d2f807947b1423ac609`.
It owns a separate player/instance and fresh userdata, never modifies either
input, negotiates mono native 16-bit PCM at 44100 Hz, and retains callbacks
through native release. PCM is bounded at 4 MiB, decoding at 40 seconds and the
owned worker including cleanup at 55 seconds.

`results/output-verification.json` records exit 0, EOF stopping reason 1,
stopped state 4, one observed audio-drain callback and 2,646,000 decoded bytes.
Original and decoded PCM both have SHA-256
`404bb49451768e577fe6c841019877c4785e0c9ebca062d165207841305c7d45`.
Peak, RMS and nonzero counts agree; source and converted file hashes remained
unchanged. A second read-only review compared the saved PCM directly with the
original generated WAV and verified all evidence hashes. No native replay was
performed by that review.

The converted FLAC STREAMINFO reports 44100 Hz, two channels and 16 bits; its
sample-total field is zero (unknown). The complete-output claim comes from
actual native decoding, not that header. PCM equality here covers the explicitly
negotiated mono output against this mono source; independent stereo-channel
content or other channel/rate conversions are outside this check.

The decoder log selects the real `flac` module under the exclusive package
plugin root. The resolved package DLL identity is recorded; this is not a live
process memory-map claim. Native pickers/controls and primary-state observations
are separate from this decoder's content/ownership gate.

## Source, build and runtime mapping

`source-runtime-manifest.json` records the exact C8 Qt DLL, core/encoder/decoder,
selected frozen feature/controller sources, checker/bindings, input/output and
evidence identities. Root observed builder `92956` complete with exit 0, copied
the built GUI and staged that exact C8 DLL. The GUI build log contains real CXX
for the window handler/MainCtx/provider, conversion-dialog cache generation/
compilation and final linking. The resource log records registration generation.
Warnings and the actual build script are retained with sanitized path labels.

A whole-Qt source-at-build freeze was not captured. Selected reviewed source
identities and the build-owner/staging record are provided; this bundle does
not reconstruct provenance for every input to the DLL. Current source hashes
alone are not compilation evidence. The package was a development copy, and
the staging record's earlier pending status remains historical context.

`evidence-index.json` pairs original ignored-file hashes with publishable copies.
JSON/text checkout/user/build-root prefixes are sanitized. The four approved
2560×1392 JPEGs are copied byte for byte and retain visible generated-fixture/
project paths. No private native-picker captures, private media metadata,
original or converted media, PCM, userdata, executable or DLL is published.
The full local GUI log is retained only by path/hash; this bundle includes a
small interface/encoder/normal-exit excerpt without media metadata.

## Reproduction

Use Windows x64 Python 3.11 or later and the matching existing C8 development
package, original generated 30-second mono16/44100 WAV and desktop-created
FLAC. The inputs' hashes must match this run if comparing its identities.
From the checkout root, all arguments below resolve to absolute Windows paths;
choose a fresh output directory:

```powershell
$conversionCheckout = (Resolve-Path .).Path
python "$conversionCheckout\doc\juxtopposed-references\native\conversion-desktop-20261014\checker\native_desktop_flac.py" `
  --package "$conversionCheckout\build-windows\slice-conversion-modal-package-20261014" `
  --source "$conversionCheckout\build-windows\duration-fixtures-20261014\waveform-audio.wav" `
  --converted "$conversionCheckout\build-windows\conversion-modal-desktop-20261014\converted-native.flac" `
  --bindings "$conversionCheckout\test\libvlc\native_flac_drain.py" `
  --output-dir "$conversionCheckout\build-windows\conversion-desktop-output-FRESH"
```

This repeats only the independent content check. For desktop acceptance, use
the app's native Browse actions with the generated fixture and a new output
path, observe primary state/volume, complete the conversion and verify the
completed-state keys/Done/outside-modal restoration separately. A changed
runtime/source/output or a new run creates new evidence. The retained build
script is historical evidence, not an automatically executed setup script.
