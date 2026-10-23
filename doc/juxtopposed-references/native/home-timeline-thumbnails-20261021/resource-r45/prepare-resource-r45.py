from pathlib import Path
from hashlib import sha256
import json,shutil
repo=Path('C:/Modding/vlc-juxtopposed');work=Path(__file__).resolve().parent
prior=json.loads((repo/'doc/juxtopposed-references/native/home-timeline-thumbnails-20261021/visible-r38/checkpoint.json').read_text())
paths=set(prior['sourceBindings'])
paths.update((work/n).relative_to(repo).as_posix() for n in ['prepare-resource-r45.py','run-resource-r45.py','resource_metrics_r45.py','make-resource-fixture-r45.sh','resource-fixtures-r45/motion-1080p50-silent.mp4','resource-fixtures-r45/media.json'])
paths.update(['test/libvlc/native_http_runtime.py','modules/control/cli/cli.c','modules/control/cli/player.c','modules/control/cli/playlist.c'])
manifest={p:sha256((repo/p).read_bytes()).hexdigest() for p in sorted(paths)}
assert all(manifest[p]==h for p,h in prior['sourceBindings'].items())
assert not (work/'source-resource-r45').exists()
media=json.loads((work/'resource-fixtures-r45/media.json').read_text())
video,audio=media['streams']
assert video['width']==1920 and video['height']==1080 and video['avg_frame_rate']=='50/1' and video['nb_frames']=='6000'
assert audio['codec_type']=='audio' and audio['sample_rate']=='48000'
for path in manifest:
    target=work/'source-resource-r45'/path;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(repo/path,target)
(work/'inputs-resource-r45.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
print('Frozen ordinary resource comparison:',len(manifest),'inputs; 120s generated 1080p50/silent stereo')
