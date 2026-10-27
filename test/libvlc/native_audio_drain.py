#!/usr/bin/env python3
"""Check a Windows package's full PCM export, resampler drain and real waveform.

Use the mono S16 WAV from generate_slice_fixtures.sh. The output directory must
be fresh. This exercises actual LibVLC/plugins; it does not construct a GUI.
"""
import argparse
import ctypes as C
import json
import math
import os
from pathlib import Path
import struct
import time
import wave

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--package', type=Path, required=True)
parser.add_argument('--media', type=Path, required=True)
parser.add_argument('--output-dir', type=Path, required=True)
args = parser.parse_args()
package, media_path, output = args.package.resolve(), args.media.resolve(), args.output_dir.resolve()
with wave.open(str(media_path)) as source:
    assert source.getnchannels() == 1 and source.getsampwidth() == 2
    source_rate, source_count = source.getframerate(), source.getnframes()
    source_pcm = source.readframes(source_count)
source_duration = source_count / source_rate
output.mkdir(parents=True, exist_ok=False)
os.environ['VLC_PLUGIN_PATH'] = str(package / 'plugins')
os.environ['VLC_LIBEXEC_PATH'] = str(package / 'vlc-preparser.exe')
os.environ['VLC_USERDATA_PATH'] = str(output / 'userdata')
dll_directory = os.add_dll_directory(str(package))
vlc = C.CDLL(str(package / 'libvlc.dll'))


def bind(name, restype, *argtypes):
    fn = getattr(vlc, name)
    fn.restype, fn.argtypes = restype, argtypes
    return fn


new = bind('libvlc_new', C.c_void_p, C.c_int, C.POINTER(C.c_char_p))
release = bind('libvlc_release', None, C.c_void_p)
media_new = bind('libvlc_media_new_path', C.c_void_p, C.c_char_p)
media_release = bind('libvlc_media_release', None, C.c_void_p)
add_option = bind('libvlc_media_add_option', None, C.c_void_p, C.c_char_p)
player_new = bind('libvlc_media_player_new', C.c_void_p, C.c_void_p, C.c_void_p, C.c_void_p)
player_release = bind('libvlc_media_player_release', None, C.c_void_p)
set_media = bind('libvlc_media_player_set_media', None, C.c_void_p, C.c_void_p)
play = bind('libvlc_media_player_play', C.c_int, C.c_void_p)
state = bind('libvlc_media_player_get_state', C.c_int, C.c_void_p)
options = [b'--ignore-config', b'--no-media-library', b'--no-video', b'--no-spu',
           b'--no-sub-autodetect-file', b'--no-sout-video', b'--no-sout-spu',
           b'--verbose=2', b'--file-logging', ('--logfile=' + str(output / 'native.log')).encode()]
instance = new(len(options), (C.c_char_p * len(options))(*options))
assert instance


def export(rate, mux):
    destination = output / f'audio-{rate}.{ "wav" if mux == "wav" else "pcm" }'
    escaped = destination.as_posix().replace("'", "\\'")
    chain = f":sout=#transcode{{acodec=s16l,channels=1,samplerate={rate}}}:std{{access=file{{no-overwrite}},mux={mux},dst='{escaped}'}}"
    player = player_new(instance, None, None)
    assert player
    started = time.monotonic()
    try:
        media = media_new(str(media_path).encode())
        assert media
        add_option(media, chain.encode())
        add_option(media, b':no-sout-keep')
        set_media(player, media)
        media_release(media)
        assert play(player) == 0
        started_playback = False
        while time.monotonic() - started < 45:
            current = state(player)
            started_playback |= current in (1, 2)
            if current == 6:
                raise AssertionError('Native export failed; inspect native.log')
            # The player can still report its initial Stopped state while the
            # start request is queued. Never release it during that transition.
            if current == 4 and (started_playback or
                (destination.exists() and destination.stat().st_size > 0)):
                break
            time.sleep(.01)
        else:
            raise AssertionError('Native export exceeded 45 seconds')
    finally:
        # Deletion waits for the output chain to close before inspecting bytes.
        player_release(player)
    if mux == 'wav':
        with wave.open(str(destination)) as exported:
            assert exported.getnchannels() == 1 and exported.getsampwidth() == 2
            assert exported.getframerate() == rate
            pcm = exported.readframes(exported.getnframes())
    else:
        pcm = destination.read_bytes()
    count, expected = len(pcm) // 2, round(source_duration * rate)
    assert count == expected, {'rate': rate, 'mux': mux, 'samples': count, 'expected': expected}
    record = {'rate': rate, 'mux': mux, 'samples': count, 'duration': count / rate,
              'elapsedSeconds': time.monotonic() - started}
    print('Passed: complete native audio export', json.dumps(record))
    return pcm, record


try:
    raw, resampled = export(8000, 'raw')
    identical, original_rate = export(source_rate, 'raw')
    assert identical == source_pcm, 'Source-rate PCM differs, including its final packet'
    _, playable = export(8000, 'wav')
finally:
    release(instance)

samples = [sample[0] / 32768. for sample in struct.iter_unpack('<h', raw)]
bucket_size = math.ceil(len(samples) / 300)
peaks = [max(abs(v) for v in samples[i:i + bucket_size]) for i in range(0, len(samples), bucket_size)]
# Compare with the generated fixture's independent time-varying amplitude.
error = sum(abs(peak - .4 * (.6 + .4 * math.sin(2 * math.pi * .2 * ((i + .5) * bucket_size / 8000))))
            for i, peak in enumerate(peaks)) / len(peaks)
assert len(peaks) == 300 and error < .015, {'points': len(peaks), 'amplitudeError': error}
result = {'exports': [resampled, original_rate, playable], 'sourcePcmIdentical': True,
          'waveformMeanAmplitudeError': error, 'peaks': peaks}
(output / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
print('Passed: 300 waveform peaks follow the source amplitude and span its complete duration.')
