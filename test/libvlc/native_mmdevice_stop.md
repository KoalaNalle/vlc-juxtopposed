# Native MMDevice stream teardown

MMDevice queues stream teardown to its existing audio thread. The caller waits
for acknowledgement. This keeps teardown after an active render/timer call and
after any wait on the stream's buffer event. The audio thread also releases the
WASAPI client on the thread that obtains its render service. Pending blocks are
released before deleting the stream owner.

Build the opt-in probe after the configured Windows test Makefile:

~~~sh
make -C "$build/test" -f Makefile -f "$repo/test/mmdevice-native.mk" \
  -j2 MMDEVICE_OUT="$freshOutput" mmdevice-stop-native
~~~

Run the actual executable under `.libs`, using a fresh output under this
checkout's ignored `build-windows` directory:

~~~powershell
python test/libvlc/native_mmdevice_stop.py --package $package --probe $probe --output $freshOutput
~~~

The probe includes the production MMDevice module. It executes its actual
`Stop`, `MMSessionMainloop`, block queue and stop dispatcher with a controlled
backend, real VLC locks/condition variables and Windows events. Eight cases
each stop during a timer callback, playback, a buffer-event wait, or idle
dispatch. They check callback thread identity, no overlap, acknowledgement,
buffer-handle closure and exact queued-block release. The idle case exercises
the dispatcher used by the outer worker; it does not initialize a real audio
device or execute that worker's COM session setup. Wait-entry observation wraps
the Windows wait without replacing its implementation.

For actual device playback and repeated stop/reopen in one process:

~~~powershell
python test/libvlc/native_mmdevice_lifecycle.py --package $package --output $freshOutput
~~~

This uses an owned generated silent WAV, the real MMDevice/WASAPI modules and
one retained LibVLC player. Sixteen cycles require decoded and played audio
statistics, acknowledged stop and a subsequent successful reopen. Eight cycles
also select a deliberately nonexistent endpoint while playing, requiring the
module's default-device fallback. Selection is local to the player; system
device settings are unchanged. A parent bounds the native child process,
including release. It checks module-load/fallback logs and preserves the source
runtime hashes. This does not simulate unplugging a physical device.

The controlled probe alone does not prove device playback. Neither check proves
device hotplug or an intermittent failure on another Windows version. Run
ordinary VLC playback and repeated live-stream stop/reopen checks against the
matching production DLL as well. Keep evidence for the original Windows version
separate from newer-host replays.
