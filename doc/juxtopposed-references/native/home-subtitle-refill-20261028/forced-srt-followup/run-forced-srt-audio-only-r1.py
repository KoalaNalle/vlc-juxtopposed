from pathlib import Path
from hashlib import file_digest
import json
import os
import re
import shutil
import subprocess
import sys
import time

root = Path(__file__).resolve().parent
repo = root.parents[1]
sys.path.insert(0, str(repo / 'test/libvlc'))
import native_http_runtime as helper

def digest(path):
    with Path(path).open('rb') as stream:
        return file_digest(stream, 'sha256').hexdigest()

previous_run = root / 'forced_srt_matroska_clean_candidate'
previous_file = previous_run / 'verification.json'
previous = json.loads(previous_file.read_text())
assert previous['status'] == 'terminal' and previous['normalExit']
assert all(previous[k] for k in ('sourceUnchanged', 'fixtureUnchanged', 'producerUnchanged',
                                'baselineUnchanged', 'runtimeUnchanged'))
base = previous_run / 'runtime'
identities = previous['runtimeSha256']
required = {
    'libvlccore.dll': 'f8fc980bbe03da57908f79045f266710dd27a16c6ee66b6c45d60cd1f9b93598',
    'plugins/codec/liblibass_plugin.dll': 'b622871c205345373343a463f193271b12cf875853e7c913c707acadb16e9b40',
    'plugins/demux/libsubtitle_plugin.dll': '1e8abb4593f82b5badf9dc9d103f655d4a0f30334c28f89e24b102b301139ea8',
    'plugins/codec/libavcodec_plugin.dll': '545f715418c4d20659dce5d6c81ee33b292b21c87243fafa62036b9ffd739e1f',
}
assert all(identities[n] == h for n, h in required.items())
assert all(digest(base / n) == h for n, h in identities.items())
fixture = root / 'media-record-flac/frame-marker.mkv'
fixture_sha = digest(fixture)
assert fixture_sha == 'ff9c28f52e759fcfcfd657dda5af5d3115b29d0255363b208dbbc08c9ae16a21'
assert digest(previous_run / 'output.mkv') == previous['outputSha256']
source_names = ['modules/demux/avformat/mux.c', 'src/libvlc-module.c',
                'src/input/es_out.c', 'src/input/decoder.c',
                'src/stream_output/stream_output.c',
                'modules/packetizer/flac.c', 'modules/packetizer/flac.h',
                'test/libvlc/native_http_runtime.py']
source = {n: digest(repo / n) for n in source_names}
assert source['modules/demux/avformat/mux.c'] == previous['sourceSha256']
producer = digest(__file__)
previous_sha = digest(previous_file)
human = {str(Path(os.environ['APPDATA']) / 'vlc' / n): None
         for n in ('vlcrc', 'vlc-qt-interface.ini')}
human = {n: digest(n) if Path(n).is_file() else None for n in human}
out = root / 'forced_srt_matroska_audio_only_r1'
out.mkdir(exist_ok=False)
runtime = out / 'runtime'
stage = helper.stage_runtime(base, runtime)
original_environment = helper.environment
def isolated_environment(runtime, output):
    env = original_environment(runtime, output)
    env.pop('VLC_PLUGIN_PATH', None)
    return env
helper.environment = isolated_environment
helper.regenerate_cache(runtime, out / 'cache')
staged_identities = {n: digest(runtime / n) for n in identities}
assert all(staged_identities[n] == h for n, h in identities.items() if not n.endswith('plugins.dat'))
output = out / 'output.mkv'
command = [str(runtime / 'vlc.exe'), '--ignore-config', '--intf=dummy',
    '--no-one-instance', '--no-media-library', '--no-save-recentplay',
    '--no-metadata-network-access', '--no-ml-network-discovery', '--no-auto-preparse',
    '--extraintf=', '--control=', '--no-crashdump', '--play-and-exit',
    '--file-logging', '--verbose=2', '--log-verbose=2', '--logfile=' + str(out / 'vlc.log'),
    '--sout-all', '--no-sout-video', '--no-sout-spu', '--sout-audio',
    '--sout=#standard{access=file{no-overwrite},mux=avformat{mux=matroska},dst=' + output.as_posix() + '}',
    str(fixture)]
record = dict(status='running', started=time.time(), command=command, stage=stage,
    scope='One headless audio-only forced Matroska pass using the exact preserved Core5/libass2/text1/avcodec2 comparison tuple; no rebuild or GUI.',
    previousVerificationSha256=previous_sha, producerSha256=producer,
    sourceSnapshotSha256=source, sourceScope='Current supporting sources guarded against concurrent edits; cached runtime identities remain authoritative.',
    runtimeSha256=staged_identities, baselineRuntimeSha256=identities,
    fixtureSha256=fixture_sha, humanBeforeSha256=human)
def save():
    (out / 'verification.json').write_text(json.dumps(record, indent=2) + '\n')
save()
record.update(helper.run_owned(command, runtime, out, timeout=85))
log = (out / 'vlc.log').read_text(errors='replace')
stderr = (out / 'stderr.txt').read_text(errors='replace')
record.update(status='terminal', ended=time.time(),
    sourceUnchanged=all(digest(repo/n) == h for n,h in source.items()),
    fixtureUnchanged=digest(fixture) == fixture_sha,
    producerUnchanged=digest(__file__) == producer,
    previousVerificationUnchanged=digest(previous_file) == previous_sha,
    baselineUnchanged=all(digest(base/n) == h for n,h in identities.items()),
    runtimeUnchanged=all(digest(runtime/n) == h for n,h in staged_identities.items()),
    humanConfigUnchanged=all((digest(n) if Path(n).is_file() else None) == h for n,h in human.items()),
    moduleCounts=re.findall(r'plug-ins loaded: (\d+) modules', log),
    selectedLibavformat='using sout mux module "avcodec"' in log,
    diagnostics=[line for line in (log+'\n'+stderr).splitlines()
                 if re.search(r'\berror:|Assertion failed|Binding loop|ReferenceError|TypeError', line)])
if output.exists():
    record.update(outputSha256=digest(output), outputBytes=output.stat().st_size)
save()
print(json.dumps({k:v for k,v in record.items() if k not in ('command', 'runtimeSha256',
    'baselineRuntimeSha256', 'sourceSnapshotSha256', 'humanBeforeSha256')}, indent=2), flush=True)
assert record['normalExit'] and all(v for k,v in record.items() if k.endswith('Unchanged'))
assert record['moduleCounts'] == ['603'] and record['selectedLibavformat'] and not record['diagnostics']

analysis_code = r'''
from pathlib import Path
from hashlib import sha256
import json, subprocess
root=Path('/mnt/c/Modding/vlc-juxtopposed/build-windows/beta-fps-20261028')
paths={'source':root/'media-record-flac/frame-marker.mkv',
       'priorAv':root/'forced_srt_matroska_clean_candidate/output.mkv',
       'audioOnly':root/'forced_srt_matroska_audio_only_r1/output.mkv'}
data={};pcms={};reports={}
def run(args):
    result=subprocess.run(args,capture_output=True,check=True)
    assert not result.stderr,result.stderr.decode(errors='replace')
    return result.stdout
for key,path in paths.items():
    inventory=json.loads(run(['ffprobe','-v','error','-show_streams','-of','json',str(path)]))['streams']
    packets=json.loads(run(['ffprobe','-v','error','-select_streams','a:0','-show_packets',
        '-show_data_hash','sha256','-of','json',str(path)]))['packets']
    frames=json.loads(run(['ffprobe','-v','error','-select_streams','a:0','-show_frames',
        '-show_entries','frame=pts_time,nb_samples','-of','json',str(path)]))['frames']
    pcm=run(['ffmpeg','-v','error','-threads','1','-i',str(path),'-map','0:a:0','-f','s16le','-'])
    data[key]=packets;pcms[key]=pcm
    reports[key]=dict(path=str(path.relative_to(root)),fileSha256=sha256(path.read_bytes()).hexdigest(),
        streamInventory=[dict(index=s['index'],type=s['codec_type'],codec=s['codec_name']) for s in inventory],
        audioPackets=len(packets),decodedFrames=len(frames),decodedSamples=sum(int(f['nb_samples']) for f in frames),
        pcmSamples=len(pcm)//2,pcmSha256=sha256(pcm).hexdigest(),lastPacket=packets[-1],lastDecodedFrame=frames[-1])
source_hashes=[p['data_hash'] for p in data['source']]
for key in ('priorAv','audioOnly'):
    hashes=[p['data_hash'] for p in data[key]]
    reports[key].update(packetPayloadsExactSource=hashes==source_hashes,
        packetPayloadsExactPrefix=hashes==source_hashes[:len(hashes)],
        exactSourcePcm=pcms[key]==pcms['source'],pcmExactPrefix=pcms[key]==pcms['source'][:len(pcms[key])],
        missingSourcePackets=[p for p in data['source'] if p['data_hash'] not in hashes])
audio_only_inventory=reports['audioOnly']['streamInventory']
result=dict(scope='Actual output inventory, compressed packet identity and complete decoded PCM comparison; no duration tolerance.',
    reports=reports,onlyOneAudioStream=len(audio_only_inventory)==1 and audio_only_inventory[0]['type']=='audio',
    audioOnlyPreservesCompleteSource=reports['audioOnly']['packetPayloadsExactSource'] and reports['audioOnly']['exactSourcePcm'],
    sourceSamples=reports['source']['decodedSamples'],audioOnlySamples=reports['audioOnly']['decodedSamples'],
    priorAvSamples=reports['priorAv']['decodedSamples'])
print(json.dumps(result,indent=2))
'''
analysis = subprocess.run(['wsl', '-d', 'VLC-Juxtopposed-Home-20261018', '--', 'python3', '-c', analysis_code],
                          capture_output=True, check=True)
assert not analysis.stderr, analysis.stderr.decode(errors='replace')
comparison = json.loads(analysis.stdout)
comparison.update(verificationSha256=digest(out/'verification.json'), producerSha256=producer)
(out/'analysis.json').write_text(json.dumps(comparison, indent=2)+'\n')
print(json.dumps(comparison, indent=2))
assert comparison['onlyOneAudioStream'], 'The requested audio-only test did not actually isolate audio.'
assert comparison['sourceSamples'] == 2880528
