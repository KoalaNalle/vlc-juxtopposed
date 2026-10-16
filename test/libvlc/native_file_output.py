#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Check complete stereo-to-mono PCM export through an owned Windows package.

The package must already have a portable directory. Uses the ordinary launcher,
real transcode/resampler/raw mux/file modules and an independently defined WAV.
It does not run a GUI or accept waveform presentation or other codecs.
"""
import argparse
import ctypes
import hashlib
import json
import os
from pathlib import Path
import subprocess
import struct
import time
import wave

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--package', type=Path, required=True)
parser.add_argument('--output-dir', type=Path, required=True)
parser.add_argument('--require-clean-visual-callbacks', action='store_true',
                    help='Also require no callbacks targeting a missing visual variable')
parser.add_argument('--configured-visualizer', action='store_true',
                    help='Exercise export with a configured spectrum visualization and dummy display')
parser.add_argument('--require-clean-pcr', action='store_true',
                    help='Also require no transcode input/output PCR mismatch')
args = parser.parse_args()
package = args.package.resolve()
output = args.output_dir.resolve()
assert (package / 'portable').is_dir(), 'Use a separate owned portable runtime'
output.mkdir(parents=True, exist_ok=False)
(output / 'temp').mkdir()
source = output / 'silence.wav'
destination = output / 'audio.pcm'
with wave.open(str(source), 'wb') as stream:
    stream.setnchannels(2)
    stream.setsampwidth(2)
    stream.setframerate(44100)
    stream.writeframes(b'\0' * (44100 * 2 * 2 * 2))

config = output / 'vlcrc'
config.write_text('metadata-network-access=0\nml-network-discovery=0\n', encoding='utf-8')
escaped = destination.as_posix().replace("'", "\\'")
sout = ("--sout=#transcode{acodec=s16l,channels=1,samplerate=8000}:"
        f"std{{access=file{{no-overwrite}},mux=raw,dst='{escaped}'}}")
log = output / 'vlc.log'
command = [str(package / 'vlc.exe'), '--intf=dummy', '--no-one-instance',
           '--no-media-library', '--no-crashdump', '--no-metadata-network-access',
           '--no-ml-network-discovery', '--no-video', '--no-spu',
           '--no-sub-autodetect-file', '--no-sout-video', '--no-sout-spu',
           '--no-sout-keep', '--play-and-exit', '--file-logging', '--verbose=2',
           '--log-verbose=2', '--logfile=' + str(log), '--config=' + str(config),
           sout, str(source)]
if args.configured_visualizer:
    command[1:1] = ['--audio-visual=visual', '--effect-list=spectrum', '--vout=vdummy']
environment = os.environ | dict(VLC_PLUGIN_PATH=str(package / 'plugins'),
                               VLC_LIBEXEC_PATH=str(package / 'vlc-preparser.exe'),
                               VLC_USERDATA_PATH=str(output / 'userdata'),
                               TEMP=str(output / 'temp'), TMP=str(output / 'temp'))
(output / 'launch.json').write_text(json.dumps(command, indent=2) + '\n', encoding='utf-8')
started = time.monotonic()
timed_out = False
forced_cleanup = False
with (output / 'stdout.txt').open('wb') as stdout, (output / 'stderr.txt').open('wb') as stderr:
    # Children inherit this request for noninteractive OS fault reporting.
    # In-process assertion dialogs or other hangs still need a bounded wait.
    kernel32 = ctypes.WinDLL('kernel32', use_last_error=True)
    kernel32.SetErrorMode.argtypes = [ctypes.c_uint]
    kernel32.SetErrorMode.restype = ctypes.c_uint
    previous_mode = kernel32.SetErrorMode(0x0001 | 0x0002 | 0x8000)
    try:
        process = subprocess.Popen(command, cwd=output, env=environment, stdout=stdout,
                                   stderr=stderr, creationflags=subprocess.CREATE_NO_WINDOW)
    finally:
        kernel32.SetErrorMode(previous_mode)
    (output / 'pid.txt').write_text(str(process.pid), encoding='utf-8')
    try:
        code = process.wait(timeout=30)
    except subprocess.TimeoutExpired:
        timed_out = True
        process.kill()  # Only this runner's own child; never accepted as a pass.
        code = process.wait(timeout=5)
        forced_cleanup = True

text = log.read_text(encoding='utf-8', errors='replace') if log.exists() else ''
pcm = destination.read_bytes() if destination.exists() and code == 0 else b''
# SoXR's S16 quantization dithers digital silence by one least-significant bit.
# Keep the sample count exact; only the amplitude comparison allows that dither.
peak = max((abs(sample[0]) for sample in struct.iter_unpack('<h', pcm)), default=0) if len(pcm) % 2 == 0 else None
identities = {}
for relative in ('vlc.exe', 'libvlc.dll', 'libvlccore.dll',
                 'plugins/demux/libwav_plugin.dll',
                 'plugins/audio_filter/libsoxr_plugin.dll',
                 'plugins/access_output/libaccess_output_file_plugin.dll',
                 'plugins/stream_out/libstream_out_transcode_plugin.dll'):
    identities[relative] = hashlib.sha256((package / relative).read_bytes()).hexdigest()
report = dict(exitCode=code, normalExit=code == 0 and not forced_cleanup, timedOut=timed_out,
              terminationConfirmed=process.poll() is not None, forcedCleanupUsed=forced_cleanup,
              elapsedSeconds=round(time.monotonic() - started, 3),
              outputSamples=len(pcm) // 2, expectedSamples=16000,
              completePcm=len(pcm) == 32000 and peak is not None and peak <= 1,
              silencePeakLsb=peak,
              fileWriteErrors=sum('file error: cannot write:' in line for line in text.splitlines()),
              missingVisualCallbackErrors=sum('nonexistent variable \'visual\'' in line for line in text.splitlines()),
              pcrMatchErrors=sum('Failed to match transcode input' in line for line in text.splitlines()),
              runtimeSha256=identities)
report['cleanVisualCallbacksRequired'] = args.require_clean_visual_callbacks
report['configuredVisualizer'] = args.configured_visualizer
report['cleanPcrRequired'] = args.require_clean_pcr
report['passed'] = (report['normalExit'] and report['terminationConfirmed']
                    and not timed_out and report['completePcm'] and report['fileWriteErrors'] == 0
                    and (not args.require_clean_visual_callbacks or report['missingVisualCallbackErrors'] == 0)
                    and (not args.require_clean_pcr or report['pcrMatchErrors'] == 0))
(output / 'verification.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
print(json.dumps(report), flush=True)
raise SystemExit(0 if report['passed'] else 1)
