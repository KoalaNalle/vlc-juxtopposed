# Audio-only recording and native destination retry

27 October 2026. No production change follows the recording464/mux464 repair in
source `ac762a8bbb`. This is additional native API evidence on the identified
GUI441/Core265/record464/mux464 tuple, not an ordinary desktop or complete beta pass.

Each generated WAV/FLAC/MP3 input is recorded twice in the same player. Recording
is armed while paused, then stopped after about eight seconds of playback. Arming
does not advance the paused clock; pause after stop holds. All six saved files
decode completely to mono44.1kHzPCM with no FFmpeg decoding errors. WAV stays PCM
WAV, FLAC uses the existing Ogg-FLAC path and MP3 stays MP3. Child exits are normal,
all source/runtime/base/fixture/producer/worker guards pass, and the three ordinary
native-format logs contain no error lines.

| Input / segment | Output codec | Decoded PCM samples | Decoded seconds | Observed API clock span |
| --- | --- | ---: | ---: | ---: |
| wav 1 | pcm_s16le | 357210 | 8.100000 | 7.930766 |
| wav 2 | pcm_s16le | 350595 | 7.950000 | 8.010607 |
| flac 1 | flac | 350208 | 7.941224 | 7.930908 |
| flac 2 | flac | 340992 | 7.732245 | 8.010498 |
| mp3 1 | mp3 | 353664 | 8.019592 | 7.982656 |
| mp3 2 | mp3 | 354816 | 8.045714 | 8.010130 |

These are real decoded counts, not exact recording-boundary acceptance. Packet/
queue timing differs from the observed playback clock by up to
0.278253seconds (the second FLAC recording). The predeclared
0.65-second bound passes. No bit-exact onset/tail or the separate strict88,200sample
conversion regression is claimed by this recording check.

A separate WAV test supplies an owned file where the output folder should be.
The first output fails to open as intended. Playback remains running; after pause,
a valid folder is supplied to the second recording in the same player. That file
decodes successfully. The marker's bytes remain unchanged, and all guards/normal
exits pass. The expected output-open diagnostics are retained, including the
unhelpful Windows message ending in No error. No ordinary error-dialog feedback,
automatic failure-state correction or settings/picker retry is accepted here;
that visible journey remains the next check.

Inputs are generated from the already-owned music fixture, whose hash is checked
unchanged. Media, raw decoded PCM and binaries remain ignored. The compressed
receipt retains commands, exact identities, producer/worker scripts, outcomes and
diagnostics. Every artifact was decompressed and verified against its SHA-256.
Other formats, live capture, unknown timing, full action/state/error combinations,
physical endpoints, accessibility/DPI and clean-package/beta gates remain open.
No dependencies, firewall rules or security/privacy/Defender settings changed.
