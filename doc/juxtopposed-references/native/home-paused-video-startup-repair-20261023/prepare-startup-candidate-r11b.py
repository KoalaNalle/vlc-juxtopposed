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
text = original
changes = [
    ('''/**
 * The decoding main loop''', '''static bool DecoderNeedsPausedPicture(const vlc_input_decoder_t *owner)
{
    /* Buffering can finish before any video packet reaches this decoder.
     * Keep obtaining input for its first picture without making this a
     * user-requested frame step or advancing the paused audio output. */
    return owner->paused && owner->cat == VIDEO_ES && owner->b_first;
}

/**
 * The decoding main loop'''),
    ('if( p_owner->paused && p_owner->frames_countdown == 0 )',
     'if( p_owner->paused && p_owner->frames_countdown == 0\n         && !DecoderNeedsPausedPicture(p_owner) )'),
    ('if (p_owner->cat == VIDEO_ES && p_owner->frames_countdown > 0)',
     'if (p_owner->cat == VIDEO_ES\n                 && (p_owner->frames_countdown > 0\n                  || DecoderNeedsPausedPicture(p_owner)))'),
    ('if (vlc_fifo_IsEmpty(p_owner->p_fifo) && p_owner->frames_countdown > 0)',
     'if (vlc_fifo_IsEmpty(p_owner->p_fifo)\n     && (p_owner->frames_countdown > 0 || DecoderNeedsPausedPicture(p_owner)))'),
]
for before, after in changes:
    assert text.count(before) == 1, before
    text = text.replace(before, after)
out = work / 'startup-candidate-r11b-source'
assert not out.exists()
out.mkdir()
(out / 'decoder-original.c').write_bytes(source.read_bytes())
(out / 'decoder.c').write_text(text, encoding='utf-8', newline='\n')
record = dict(candidateOnly=True, productionSourceUnchanged=digest(source) == inputs['src/input/decoder.c'], originalSha256=digest(source), candidateSha256=digest(out / 'decoder.c'), producerSha256=digest(Path(__file__)), scope='Allow a paused video decoder awaiting its first picture to consume/request input independently of explicit frame-step counts. No layout, audio/subtitle or user step notification changes.')
(out / 'source-record.json').write_text(json.dumps(record, indent=2)+'\n', encoding='utf-8', newline='\n')
for name in ['startup-candidate-r10.mk', 'build-startup-candidate-r10.sh']:
    target = work / name.replace('r10', 'r11b')
    assert not target.exists()
    target.write_text((work / name).read_text(encoding='utf-8').replace('r10', 'r11b'), encoding='utf-8', newline='\n')
print(json.dumps(record, indent=2))
