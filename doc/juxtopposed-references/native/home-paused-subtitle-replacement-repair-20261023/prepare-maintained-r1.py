from pathlib import Path
import ast
work=Path(__file__).resolve().parent;repo=work.parent.parent
old=repo/'build-windows/beta-subtitle-frame-conservation-20261023'
generator=(old/'generate-marker-r1.sh').read_text(encoding='utf-8')
start=generator.index('repo=');end=generator.index('ffmpeg -hide_banner')
generator=generator[:start]+'''if [ "$#" -ne 2 ]; then
    printf "Usage: %s COLORED_LONG_MKV NEW_OUTPUT_DIRECTORY\\n" "$0" >&2
    exit 2
fi
source="$1"
out="$2"
test -f "$source"
test ! -e "$out"
mkdir "$out"
'''+generator[end:]
generator+='''
cat > "$out/external.srt" <<'EOF'
1
00:00:00,000 --> 00:00:19,999
EXTERNAL FIRST CUE

2
00:00:20,000 --> 00:00:39,999
EXTERNAL MIDDLE CUE

3
00:00:40,000 --> 00:00:59,900
EXTERNAL LAST CUE
EOF
ffmpeg -hide_banner -loglevel error -nostdin -copyts -i "$out/frame-marker.mkv" \\
    -map 0 -c:v copy -c:a copy -c:s ass -avoid_negative_ts disabled "$out/frame-marker-ass.mkv"
ffmpeg -hide_banner -loglevel error -nostdin -copyts -i "$out/frame-marker.mkv" \\
    -map 0 -c:v copy -c:a copy -c:s mov_text -avoid_negative_ts disabled "$out/frame-marker.mp4"
for name in frame-marker-ass.mkv frame-marker.mp4; do
    ffprobe -v error -show_streams -show_format -of json "$out/$name" > "$out/$name-probe.json"
    sha256sum "$out/$name"
done
'''
target=repo/'test/libvlc/generate_paused_subtitle_tracks.sh';assert not target.exists();target.write_text(generator,encoding='utf-8',newline='\n')
text=(old/'native-tracks-r1.py').read_text(encoding='utf-8')
text=text.replace('"""Native investigation, not a passing subtitle/startup regression."""','"""Opt-in Windows LibVLC 4 paused subtitle replacement/frame regression."""')
start=text.index('parser=argparse.ArgumentParser()');end=text.index('\ndef write(name,v):')
text=text[:start]+'''parser=argparse.ArgumentParser()
parser.add_argument('--fixture-root',type=Path,required=True)
parser.add_argument('--media',choices=['frame-marker.mkv','frame-marker-ass.mkv','frame-marker.mp4'],default='frame-marker.mkv')
parser.add_argument('--audio',choices=['wasapi','none','dummy'],default='wasapi')
parser.add_argument('--runtime',type=Path,required=True)
parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args()
repo=Path(__file__).resolve().parents[2]
fixtures=args.fixture_root.resolve();out=args.output.resolve();runtime=args.runtime.resolve()
assert out.is_dir() and not any(out.iterdir())
mediaPath=fixtures/args.media;externalPath=fixtures/'external.srt'
assert mediaPath.is_file() and externalPath.is_file()
audio=True if args.audio=='wasapi' else 'dummy' if args.audio=='dummy' else False
spu=True;threads=0;label=args.media+'-'+args.audio
'''+text[end:]
text=text.replace('args.case','label')
start=text.index('result=dict(');end=text.index('\ndef observe(',start)
text=text[:start]+'''result=dict(label=label,passed=False,fullBetaAccepted=False,
    scope='Native paused selected-subtitle replacement, caption masks, actual frame-marker conservation, sampled pause and resume. Cold captions, automatic external selection, physical audibility, desktop and full beta require separate checks.',
    coreSha256=digest(runtime/'libvlccore.dll'),mkvSha256=digest(runtime/'plugins/demux/libmkv_plugin.dll'),
    options=options,producerSha256=digest(Path(__file__)),fixtureProducerSha256=digest(repo/'test/libvlc/generate_paused_subtitle_tracks.sh'),
    mediaSha256=digest(mediaPath),externalSha256=digest(externalPath),observations=[],teardownComplete=False)
'''+text[end:]
text=text.replace("if entry.get('snapshotAvailable') and mediaPath.name=='frame-marker.mkv':", "if entry.get('snapshotAvailable'):")
needle="            entry.update(snapshot=picture.name"
assert text.count(needle)==1
text=text.replace(needle,"            mask=bytes(1 if pixels[i]>160 and pixels[i+1]>160 and pixels[i+2]>160 else 0 for i in range(0,len(pixels),4))\n            entry['captionMaskSha256']=sha256(mask).hexdigest()\n"+needle)
text=text.replace("(work/'media-r1/manual-external.srt').as_uri()", "externalPath.as_uri()")
needle="        for cycle,language in enumerate(['eng','fin','eng','fin']):"
text=text.replace(needle,"        select(player,2,finnish['id'].encode());time.sleep(.5);observe('same-track-selected-paused')\n"+needle)
text=text.replace("write('investigation.json'", "write('result.json'")
start=text.index("write('result.json',result)\nassert result['stopReached']",text.index("lines=(out/'native.log')"))
text=text[:start]+'''rows={r['step']:r for r in result['observations']}
normal=['normal-pause-finnish','normal-pause-english']
later=['finnish-selected-paused','same-track-selected-paused','repeat-0-eng','repeat-1-fin','repeat-2-eng','repeat-3-fin','external-explicit-selected-paused']
selected=normal+later
english=rows['played-paused'].get('captionMaskSha256');finnish=rows['normal-pause-finnish'].get('captionMaskSha256')
external=rows['external-explicit-selected-paused'].get('captionMaskSha256')
languages={n:('eng' if n in ['normal-pause-english','repeat-0-eng','repeat-2-eng'] else 'fin') for n in selected[:-1]}
checks=dict(
    playPauseReady=result['resumeReached'] and result['playedPauseReached'],
    pausedPictures=all(r['state']==3 and r.get('snapshotAvailable') for r in result['observations']),
    sampledClocksFrozen=all(all(s['state']==3 for s in r['pausedSamples']) and len({s['timeUs'] for s in r['pausedSamples']})==1 for r in result['observations']),
    captionsPresent=all(rows[n].get('brightPixels',0)>1000 for n in selected),
    selectedLanguages=all([t['language'] for t in rows[n]['subtitleTracks'] if t['selected']]==[lang] for n,lang in languages.items()),
    englishRestored=bool(english) and all(rows[n].get('captionMaskSha256')==english for n in ['normal-pause-english','seek-first-paused','repeat-0-eng','repeat-2-eng']),
    finnishDistinct=bool(finnish) and finnish!=english and all(rows[n].get('captionMaskSha256')==finnish for n in ['finnish-selected-paused','same-track-selected-paused','repeat-1-fin','repeat-3-fin']),
    externalDistinct=bool(external) and external not in [english,finnish] and rows['external-after-resume'].get('captionMaskSha256')==external,
    normalFramesConserved=all(rows[n].get('frameMarker')==rows['played-paused'].get('frameMarker') for n in normal),
    seekFramesConserved=all(rows[n].get('frameMarker')==rows['seek-first-paused'].get('frameMarker') for n in later),
    explicitExternalSelected=result['externalAddReturn']==0 and result['externalTrackAdded'] and result['externalExplicitSelected'],
    resumeAdvanced=result['externalResumeReached'] and rows['external-after-resume'].get('frameMarker',-1)>rows['external-explicit-selected-paused'].get('frameMarker',9999),
    audioBackend=args.audio!='wasapi' or result['actualWasapiSelected'],
    teardown=result['stopReached'] and result['teardownComplete'])
if args.audio=='none':
    checks['reportedPausedTimeConserved']=all(rows[n]['timeUs']==rows['played-paused']['timeUs'] for n in normal) and all(rows[n]['timeUs']==rows['seek-first-paused']['timeUs'] for n in later)
result.update(checks=checks,passed=all(checks.values()),
    automaticExternalSelectionObserved=any(t['selected'] and t['id'] not in [v['id'] for v in rows['seek-first-paused']['subtitleTracks']] for t in rows['external-added-selected-paused']['subtitleTracks']),
    notQualified=['Cold initial caption delivery','Automatic external-track selection','WASAPI absolute paused get_time / exact media PTS','Desktop and physical audibility','Whole subtitle workflow and full beta'])
write('result.json',result)
assert result['stopReached'] and result['teardownComplete']
print(label,'normal terminal; selected replacement passed',result['passed'],flush=True)
'''
ast.parse(text)
target=repo/'test/libvlc/native_paused_subtitle_tracks.py';assert not target.exists();target.write_text(text,encoding='utf-8',newline='\n')
make=repo/'test/Makefile.am';raw=make.read_bytes();assert b'native_paused_subtitle_tracks.py' not in raw
make.write_bytes(raw+b'\nEXTRA_DIST += libvlc/native_paused_subtitle_tracks.py \\\n    libvlc/generate_paused_subtitle_tracks.sh libvlc/native_paused_subtitle_tracks.md\n')
print('Prepared portable native frame/subtitle regression and fixture generator')
