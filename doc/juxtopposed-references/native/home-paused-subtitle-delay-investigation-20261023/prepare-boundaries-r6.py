from pathlib import Path
from hashlib import sha256
import ast, json, shutil

work = Path(__file__).resolve().parent
repo = work.parent.parent
read = lambda p: json.loads(p.read_text(encoding='utf-8-sig'))
digest = lambda p: sha256(p.read_bytes()).hexdigest()
regressions = read(work/'regressions-candidate-r5/processes.json')
assert regressions['status'] == 'terminal' and regressions['passed']
assert len(regressions['records']) == 24 and regressions['allProcessesNormal']
before = read(work/'inputs-candidate-r4.json')['files']
assert all(digest(repo/p) == h for p, h in before.items())
backup = work/'worker-preimage-r6'
assert not backup.exists()
backup.mkdir()
worker = repo/'test/libvlc/native_paused_subtitle_delay.py'
guide = worker.with_suffix('.md')
shutil.copyfile(worker, backup/worker.name)
shutil.copyfile(guide, backup/guide.name)
source = worker.read_text(encoding='utf-8-sig')


def change(old, new):
    global source
    assert source.count(old) == 1, old[:100]
    source = source.replace(old, new)


change("parser.add_argument('--delay-ms', type=int, choices=[-1200, 0, 1200], default=1200)",
       "parser.add_argument('--delay-ms', type=int, choices=[-1200, 0, 1200], default=1200)\n"
       "parser.add_argument('--seek-mode', choices=['time', 'position'], default='time')\n"
       "parser.add_argument('--boundary-seconds', type=float, choices=[19.0, 20.5], default=20.5)")
change("vouts = bind('libvlc_media_player_has_vout', C.c_uint, C.c_void_p)",
       "set_position = bind('libvlc_media_player_set_position', C.c_int, C.c_void_p, C.c_double, C.c_bool)\n"
       "get_length = bind('libvlc_media_player_get_length', C.c_int64, C.c_void_p)\n"
       "vouts = bind('libvlc_media_player_has_vout', C.c_uint, C.c_void_p)")
change("              media=args.media, audio=args.audio, defaultDelayMs=args.delay_ms,",
       "              media=args.media, audio=args.audio, defaultDelayMs=args.delay_ms,\n"
       "              seekMode=args.seek_mode, boundarySeconds=args.boundary_seconds,")
change("    assert seek(player, round(seconds * 1_000_000), False) == 0", '''    if args.seek_mode == 'time':
        assert seek(player, round(seconds * 1_000_000), False) == 0
    else:
        length = get_length(player)
        assert length > 0
        assert set_position(player, seconds * 1_000_000 / length, False) == 0''')
change("    seek_paused(20.5, 'boundary-seek')",
       "    seek_paused(args.boundary_seconds, 'boundary-seek')")
change("    boundary_reference = 'reference-first' if args.delay_ms > 0 else 'reference-middle'",
       "    boundary_reference = ('reference-first' if args.boundary_seconds - args.delay_ms / 1000 < 20\n"
       "                          else 'reference-middle')\n"
       "    playing_reference = 'reference-first' if args.delay_ms > 0 else 'reference-middle'")
change("playingControlCaption=matches('after-playing-boundary', boundary_reference))",
       "playingControlCaption=matches('after-playing-boundary', playing_reference))")
change("            'reference-first' if name == 'live-positive' else 'reference-middle')",
       "            'reference-first' if args.boundary_seconds - rows[name]['delayUs'] / 1_000_000 < 20\n"
       "            else 'reference-middle')")
ast.parse(source)
worker.write_text(source, encoding='utf-8', newline='\n')
guide.write_text(guide.read_text(encoding='utf-8-sig')+'''

`--seek-mode position` uses LibVLC's precise normalized-position entry point,
matching the progress-bar route; time seeking remains the default. An optional
19-second boundary checks a -1.2-second delay bringing the middle cue forward.
Expected paused cue masks are derived from the requested boundary minus each
live delay. The playing control always crosses 20.2 seconds and derives its
expectation separately. Encoded frame markers still qualify actual paused
conservation independently of the requested timestamp.
''', encoding='utf-8', newline='\n')
parent = (work/'run-delay-r4.py').read_text(encoding='utf-8-sig')


def parent_change(old, new):
    global parent
    assert parent.count(old) == 1, old[:100]
    parent = parent.replace(old, new)


parent_change("choices=['ass-wasapi', 'formats']", "choices=['boundaries']")
parent_change("manifest = work/'inputs-candidate-r4.json'", "manifest = work/'inputs-boundary-r6.json'")
start = parent.index("cases = [('frame-marker-ass.mkv'")
end = parent.index("root = work/", start)
parent = parent[:start]+'''cases = [('frame-marker-ass.mkv', 'wasapi', 1200, 20.5),
         ('frame-marker.mp4', 'wasapi', 1200, 20.5),
         ('frame-marker-ass.mkv', 'wasapi', -1200, 19.0)]
'''+parent[end:]
parent_change("root = work/('native-delay-'+args.variant+'-'+args.batch+'-r4')",
              "root = work/('native-boundary-'+args.variant+'-r6')")
parent_change("for media_name, audio, delay in cases:",
              "for media_name, audio, delay, boundary in cases:")
parent_change("name = media_name+'-'+audio+'-'+str(delay)",
              "name = media_name+'-'+audio+'-'+str(delay)+'-'+str(boundary)")
parent_change("'--audio', audio, '--delay-ms', str(delay), '--output', str(probe)]",
              "'--audio', audio, '--delay-ms', str(delay), '--output', str(probe),\n"
              "                   '--seek-mode', 'position', '--boundary-seconds', str(boundary)]")
ast.parse(parent)
target = work/'run-boundaries-r6.py'
assert not target.exists()
target.write_text(parent, encoding='utf-8', newline='\n')
names = set(before)
names.update((work/p).relative_to(repo).as_posix() for p in [
    'run-regressions-r5.py', 'compare-frames-r4.py', 'prepare-boundaries-r6.py', 'run-boundaries-r6.py'])
snapshot = work/'source-boundary-r6'
assert not snapshot.exists()
snapshot.mkdir()
inputs = {p: digest(repo/p) for p in sorted(names)}
for name, h in inputs.items():
    destination = snapshot/name
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(repo/name, destination)
    assert digest(destination) == h
(work/'inputs-boundary-r6.json').write_text(json.dumps(dict(files=inputs,
    sourceCount=len(inputs), sourceSnapshot=str(snapshot), productionApplied=False,
    fullBetaAccepted=False), indent=2)+'\n', encoding='utf-8', newline='\n')
print('Prepared position/negative-boundary extension;', len(inputs), 'actual input copies', flush=True)
