from pathlib import Path
from hashlib import sha256
import json

repo = Path('C:/Modding/vlc-juxtopposed')
work = Path(__file__).resolve().parent
digest = lambda p: sha256(p.read_bytes()).hexdigest()
inputs = json.loads((work / 'inputs-r4.json').read_text(encoding='utf-8-sig'))
source = repo / 'src/input/decoder.c'
assert digest(source) == inputs['src/input/decoder.c']
original = source.read_text(encoding='utf-8')
before = '''    p_owner->paused = b_paused;
    p_owner->pause_date = i_date;
    p_owner->frames_countdown = 0;
'''
after = '''    p_owner->paused = b_paused;
    p_owner->pause_date = i_date;
    /* Buffering can finish before this video decoder receives its first
     * packet. Obtain its first picture while paused, using the existing
     * next-frame input request when more packets are needed. */
    p_owner->frames_countdown = b_paused && p_owner->cat == VIDEO_ES
                             && p_owner->b_first ? 1 : 0;
'''
assert original.count(before) == 1
out = work / 'startup-candidate-r10-source'
assert not out.exists()
out.mkdir()
(out / 'decoder-original.c').write_bytes(source.read_bytes())
(out / 'decoder.c').write_text(original.replace(before, after), encoding='utf-8', newline='\n')
record = dict(candidateOnly=True, productionSourceUnchanged=digest(source) == inputs['src/input/decoder.c'], originalSha256=digest(source), candidateSha256=digest(out / 'decoder.c'), producerSha256=digest(Path(__file__)), scope='ChangePause permits one first video picture using existing next-frame data requests. No subtitle/audio policy or production source changed.')
(out / 'source-record.json').write_text(json.dumps(record, indent=2)+'\n', encoding='utf-8', newline='\n')
print(json.dumps(record, indent=2))
