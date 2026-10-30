#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Bounded-runner worker for real FFmpeg output error callbacks.

Arguments: isolated runtime, generated media, fresh case directory, mode.
Modes are success, header-error, writer-error and cancel. writer-error requires
the parent to close its stdout pipe after reading a prefix. This worker never
changes the input media or the user's VLC configuration. A surrounding runner
must bound the process and verify the log, callback counts and output contents.
"""
import ctypes as C
import json
import os
from pathlib import Path
import sys
import threading
import time


def main():
    runtime, media, out = map(Path, sys.argv[1:4])
    mode = sys.argv[4]
    assert mode in ('success', 'header-error', 'writer-error', 'cancel')
    assert runtime.is_absolute() and media.is_absolute() and out.is_absolute()
    assert out.is_dir() and not (out / 'callbacks.json').exists()
    dll_directory = os.add_dll_directory(str(runtime))
    vlc = C.CDLL(str(runtime / 'libvlc.dll'))
    pointer = C.c_void_p

    def bind(name, result, *arguments):
        function = getattr(vlc, name)
        function.restype, function.argtypes = result, arguments
        return function

    new = bind('libvlc_new', pointer, C.c_int, C.POINTER(C.c_char_p))
    release = bind('libvlc_release', None, pointer)
    media_new = bind('libvlc_media_new_path', pointer, C.c_char_p)
    media_release = bind('libvlc_media_release', None, pointer)
    player_new = bind('libvlc_media_player_new', pointer, pointer, pointer, pointer)
    player_release = bind('libvlc_media_player_release', None, pointer)
    set_media = bind('libvlc_media_player_set_media', None, pointer, pointer)
    play = bind('libvlc_media_player_play', C.c_int, pointer)
    state = bind('libvlc_media_player_get_state', C.c_int, pointer)
    get_time = bind('libvlc_media_player_get_time', C.c_int64, pointer)
    stop = bind('libvlc_media_player_stop_async', C.c_int, pointer)
    error_type = C.CFUNCTYPE(None, pointer, C.c_char_p, C.c_char_p)
    register_error = bind('libvlc_dialog_set_error_callback', None,
                          pointer, error_type, pointer)
    native_states = []
    active_seen, terminal_seen = threading.Event(), threading.Event()
    state_type = C.CFUNCTYPE(None, pointer, C.c_int)

    @state_type
    def on_state(_opaque, value):
        native_states.append(dict(state=value, monotonic=time.monotonic()))
        if value in (1, 2, 3, 5):
            active_seen.set()
        elif value in (4, 6) and active_seen.is_set():
            terminal_seen.set()

    class PlayerCallbacks(C.Structure):
        _fields_ = [('version', C.c_uint32)] + [
            (name, state_type if name == 'on_state_changed' else pointer)
            for name in ['on_media_changed', 'on_media_stopping', 'on_state_changed', 'on_buffering_changed', 'on_rate_changed', 'on_capabilities_changed', 'on_position_changed', 'on_length_changed', 'on_track_list_changed', 'on_track_selection_changed', 'on_program_list_changed', 'on_program_selection_changed', 'on_titles_changed', 'on_title_selection_changed', 'on_chapter_selection_changed', 'on_recording_changed', 'on_screenshot_taken', 'on_media_parsed', 'on_media_meta_changed', 'on_media_subitems_changed', 'on_media_attachments_added', 'on_next_frame_status', 'on_prev_frame_status', 'on_vout_changed', 'on_cork_changed', 'on_audio_volume_changed', 'on_audio_mute_changed', 'on_audio_device_changed']]

    callbacks = PlayerCallbacks()
    callbacks.on_state_changed = on_state
    events, states = [], []
    events_lock = threading.Lock()

    @error_type
    def on_error(_opaque, title, text):
        with events_lock:
            events.append(dict(title=(title or b'').decode('utf-8', 'replace'),
                               text=(text or b'').decode('utf-8', 'replace'),
                               monotonic=time.monotonic()))

    output = out / 'output.mkv'
    mux = 'mp4' if mode == 'header-error' else 'matroska'
    sout = ('#standard{access=fd,mux=avformat{mux=matroska},dst=1}'
            if mode == 'writer-error' else
            '#standard{access=file{no-overwrite},mux=avformat{mux=' + mux +
            '},dst=' + output.as_posix() + '}')
    options = ['--ignore-config', '--no-media-library', '--no-save-recentplay',
               '--no-metadata-network-access', '--no-ml-network-discovery',
               '--no-auto-preparse', '--extraintf=', '--control=',
               '--vout=dummy', '--aout=dummy', '--no-hw-dec',
               '--avcodec-threads=1', '--file-logging', '--verbose=2',
               '--log-verbose=2', '--logfile=' + str(out / 'vlc.log'),
               '--sout-all', '--sout=' + sout]
    encoded = [option.encode('utf-8') for option in options]
    instance = player = None
    report = dict(mode=mode, completed=False, cancellationRequested=False)
    try:
        instance = new(len(encoded), (C.c_char_p * len(encoded))(*encoded))
        assert instance, 'libvlc_new failed'
        register_error(instance, on_error, None)
        player = player_new(instance, C.byref(callbacks), None)
        assert player
        item = media_new(str(media).encode('utf-8'))
        assert item
        set_media(player, item)
        media_release(item)
        active_seen.clear()
        terminal_seen.clear()
        native_states.clear()
        assert play(player) == 0
        deadline = time.monotonic() + 70
        while time.monotonic() < deadline:
            current = state(player)
            if not states or states[-1]['state'] != current:
                states.append(dict(state=current, mediaTimeMs=get_time(player)))
            if mode == 'cancel' and not report['cancellationRequested'] and get_time(player) >= 500:
                report['cancellationRequested'] = True
                report['cancelMediaTimeMs'] = get_time(player)
                assert stop(player) == 0
            if terminal_seen.is_set() and current in (4, 6):
                report['terminalState'] = current
                report['completed'] = True
                break
            time.sleep(.01)
        assert report['completed'], 'No terminal player state before deadline'
    finally:
        if player:
            player_release(player)
        if instance:
            release(instance)
        report.update(events=events, states=states, nativeStateEvents=native_states, released=True)
        (out / 'callbacks.json').write_text(json.dumps(report, indent=2) + '\n',
                                          encoding='utf-8')
        dll_directory.close()


if __name__ == '__main__':
    main()
