from pathlib import Path
from hashlib import sha256
import json
work=Path(__file__).resolve().parent;repo=work.parent.parent
source=repo/'test/libvlc/native_paused_subtitle_tracks.py';text=source.read_text(encoding='utf-8-sig')
needle="parser.add_argument('--runtime',type=Path,required=True)"
assert text.count(needle)==1
text=text.replace(needle,"parser.add_argument('--external-file',default='external.srt')\nparser.add_argument('--external-track-count',type=int,default=1)\n"+needle)
needle="externalPath=fixtures/'external.srt'";assert text.count(needle)==1;text=text.replace(needle,'externalPath=fixtures/args.external_file')
needle="wait(lambda:len(tracks())==3,3)";assert text.count(needle)==1;text=text.replace(needle,'wait(lambda:len(tracks())==len(available)+args.external_track_count,3)')
target=work/'native-tracks-extra-r2.py';assert not target.exists();target.write_text(text,encoding='utf-8',newline='\n')
(work/'extra-probe-r2.json').write_text(json.dumps(dict(sourceSha256=sha256(source.read_bytes()).hexdigest(),probeSha256=sha256(target.read_bytes()).hexdigest(),scope='Additional candidate source-registration tests for ASS and a two-track Matroska subtitle file. Maintained automatic/preserve probe unchanged while R1 matrix runs.'),indent=2)+'\n',encoding='utf-8',newline='\n')
