from pathlib import Path
from hashlib import sha256
import ast,json,shutil
work=Path(__file__).resolve().parent
repo=work.parent.parent
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
for name in ['native-mkv-headers-r30','native-subtitle-cancel-r31b','native-seek-policy-r33']:
    record=read(work/name/'comparison.json')
    assert record['passed'] and record['allProcessesNormal']
    assert all(r['sourceInputsUnchanged'] and r['runtimeUnchanged'] and r['producerUnchanged'] and r['fixturesUnchanged'] for r in record['records'])
record=read(work/'mkv-headers-r29-source/source-record.json')
assert all(digest(repo/p)==h for p,h in record['files'].items())
assert read(work/'headers-build-audit-r32.json')['passed']
private=work/'mkv-headers-r29-source'
baseline=read(work/'mkv-spu-seek-r18b-source/source-record.json')['files']
changed=[]
for name,h in record['privateSources']['after'].items():
    path=repo/'modules/demux/mkv'/name
    assert path.exists() and digest(path)==baseline['modules/demux/mkv/'+name]
    assert digest(private/'after'/name)==h
    if digest(path)!=h:changed.append(name)
assert set(changed)=={'matroska_segment.cpp','matroska_segment.hpp','matroska_segment_seeker.cpp','matroska_segment_seeker.hpp'}
for name in changed:shutil.copyfile(private/'after'/name,repo/'modules/demux/mkv'/name)

worker=(work/'native-mkv-interval-r25.py').read_text(encoding='utf-8')
worker=worker.replace('"""Native investigation, not a passing subtitle/startup regression."""',
    '"""Opt-in Windows LibVLC 4 paused subtitle-seeking regression probe."""')
worker=worker.replace("parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()",
    "parser.add_argument('--output',type=Path,required=True);parser.add_argument('--fixture-root',type=Path,required=True);args=parser.parse_args()")
worker=worker.replace("repo=Path('C:/Modding/vlc-juxtopposed');work=Path(__file__).resolve().parent;out=args.output;runtime=args.runtime",
    "work=args.fixture_root.resolve();out=args.output.resolve();runtime=args.runtime.resolve();assert out.is_dir()")
worker=worker.replace('candidateOnly=True,repairAccepted=False,configuration=cfg', 'fullBeta=False,configuration=cfg')
assert "C:/Modding" not in worker and '--fixture-root' in worker
ast.parse(worker)
test=repo/'test/libvlc/native_paused_subtitle_seek.py';assert not test.exists()
test.write_text(worker,encoding='utf-8',newline='\n')

generator=(work/'generate-subtitle-seek-edges-r20.sh').read_text(encoding='utf-8')
generator=generator.replace('work=/mnt/c/Modding/vlc-juxtopposed/build-windows/beta-tracks-subtitles-20261023\nout="$work/media-r20"',
    'if [ "$#" -ne 1 ]; then\n    printf "Usage: %s OUTPUT_DIRECTORY\\n" "$0" >&2\n    exit 2\nfi\nout="$1"')
generator+='''
cat > "$out/overlap-en.srt" <<'EOF'
1
00:00:00,000 --> 00:00:59,900
LONG OVERLAPPING CUE

2
00:00:10,000 --> 00:00:12,000
SHORT OVERLAY CUE

3
00:00:30,000 --> 00:00:32,000
SECOND SHORT OVERLAY
EOF
ffmpeg -hide_banner -loglevel error -nostdin -copyts -i "$out/colored-long.mkv" -i "$out/overlap-en.srt" \\
    -map 0:v -map 0:a -map 1:s -c copy -metadata:s:s:0 language=eng -disposition:s:0 default \\
    -cluster_time_limit 1000 -avoid_negative_ts disabled "$out/colored-overlap.mkv"
ffmpeg -hide_banner -loglevel error -nostdin -copyts -i "$out/colored-overlap.mkv" \\
    -map 0 -c:v copy -c:a copy -c:s ass -metadata:s:s:0 language=eng \\
    -cluster_time_limit 1000 -avoid_negative_ts disabled "$out/ass-overlap.mkv"
for name in colored-overlap ass-overlap; do
    ffprobe -v error -show_streams -show_format -of json "$out/$name.mkv" > "$out/$name-probe.json"
    ffprobe -v error -select_streams s -show_packets -show_entries packet=pts_time,duration_time,pos -of json "$out/$name.mkv" > "$out/$name-subtitle-packets.json"
    sha256sum "$out/$name.mkv"
done
'''
target=repo/'test/libvlc/generate_paused_subtitle_seek.sh';assert not target.exists()
target.write_text(generator,encoding='utf-8',newline='\n')
make=repo/'test/Makefile.am'
original=digest(make)
with make.open('a',encoding='utf-8',newline='\n') as f:
    f.write('\nEXTRA_DIST += libvlc/native_paused_subtitle_seek.py \\\n    libvlc/generate_paused_subtitle_seek.sh libvlc/native_paused_subtitle_seek.md\n')
output=dict(localSourceApplied=True,ordinaryBuildAccepted=False,fullBeta=False,productionFiles={n:digest(repo/'modules/demux/mkv'/n) for n in changed},
    maintainedProbeSha256=digest(test),generatorSha256=digest(target),testMakefileBeforeSha256=original,testMakefileAfterSha256=digest(make),
    scope='Verified private source promoted locally after positive caption, performance, Stop and policy checks. Standard ordinary build and visible app checks remain required. No commit, push or release.')
(work/'source-application-r34.json').write_text(json.dumps(output,indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps(output,indent=2))
