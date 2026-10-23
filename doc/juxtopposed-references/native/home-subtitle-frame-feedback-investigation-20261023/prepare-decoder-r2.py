from pathlib import Path
from hashlib import sha256
import json, subprocess
work=Path(__file__).resolve().parent;repo=work.parent.parent
source=repo/'src/input/decoder.c';before=source.read_text(encoding='utf-8-sig')
digest=lambda p:sha256(p.read_bytes()).hexdigest()
assert digest(source)=='ce774a56e630a0603aa82387975a184b88c3b2fa3696f8125d3738ffdc2b04bc'
after=before
def replace(old,new,count=1):
    global after
    assert after.count(old)==count,(old[:80],after.count(old),count)
    after=after.replace(old,new)
replace('    int frames_countdown;\n','    int frames_countdown;\n    /* The first countdown picture belongs to an internal paused flush. */\n    bool flush_frame_pending;\n')
replace('static void Decoder_PausedForNextFrame(vlc_input_decoder_t *owner)\n', '''static void Decoder_NotifyNextPicture(vlc_input_decoder_t *owner)
{
    vlc_fifo_Assert(owner->p_fifo);
    if (owner->flush_frame_pending)
    {
        owner->flush_frame_pending = false;
        return;
    }
    decoder_Notify(owner, frame_next_status, 0);
}

static void Decoder_PausedForNextFrame(vlc_input_decoder_t *owner)
''')
replace('    for (int i = 0; i < next_request_count - owner->frames_countdown; ++i)\n        decoder_Notify(owner, frame_next_status, 0);',
        '    for (int i = 0; i < next_request_count - owner->frames_countdown; ++i)\n        Decoder_NotifyNextPicture(owner);')
replace('        p_owner->frames_countdown--;\n        vout_PutPicture(p_vout, p_picture);\n        decoder_Notify(p_owner, frame_next_status, 0);',
        '        p_owner->frames_countdown--;\n        vout_PutPicture(p_vout, p_picture);\n        Decoder_NotifyNextPicture(p_owner);')
replace('''            owner->frames_countdown = 0;
            decoder_Notify(owner, frame_next_status, -EAGAIN);''', '''            bool has_user_request = owner->frames_countdown
                                  > (owner->flush_frame_pending ? 1 : 0);
            owner->frames_countdown = 0;
            owner->flush_frame_pending = false;
            if (has_user_request)
                decoder_Notify(owner, frame_next_status, -EAGAIN);''')
replace('''    /* Flush video/spu decoder when paused: increment frames_countdown in order
     * to display one frame/subtitle */
    if( p_owner->paused && ( cat == VIDEO_ES || cat == SPU_ES )
     && p_owner->frames_countdown == 0 )
        p_owner->frames_countdown = 1;''', '''    /* Refresh one paused picture/subtitle without reporting a user frame step.
     * Existing user requests retain their normal completion notifications. */
    if( p_owner->paused && ( cat == VIDEO_ES || cat == SPU_ES )
     && p_owner->frames_countdown == 0 )
    {
        p_owner->frames_countdown = 1;
        p_owner->flush_frame_pending = cat == VIDEO_ES;
    }''')
replace('    p_owner->frames_countdown = 0;\n', '    p_owner->frames_countdown = 0;\n    p_owner->flush_frame_pending = false;\n',count=3)
# Correct indentation in the early explicit-step EOF return.
replace('        p_owner->frames_countdown = 0;\n    p_owner->flush_frame_pending = false;',
        '        p_owner->frames_countdown = 0;\n        p_owner->flush_frame_pending = false;')
replace('''    if (owner->frames_countdown == -1)
        owner->frames_countdown = 0;''', '''    if (owner->frames_countdown == -1)
    {
        owner->frames_countdown = 0;
        owner->flush_frame_pending = false;
    }''')
replace('        owner->frames_countdown = -1;\n','        owner->frames_countdown = -1;\n        owner->flush_frame_pending = false;\n')
candidate=work/'candidate-r2';assert not candidate.exists();candidate.mkdir()
(candidate/'decoder.c').write_text(after,encoding='utf-8',newline='\n')
diff=subprocess.run(['git','diff','--no-index','--',str(source),str(candidate/'decoder.c')],cwd=repo,capture_output=True)
assert diff.returncode==1
(work/'decoder-r2.patch').write_bytes(diff.stdout)
(work/'candidate-r2.json').write_text(json.dumps(dict(productionApplied=False,sourceSha256=digest(source),candidateSha256=digest(candidate/'decoder.c'),patchSha256=digest(work/'decoder-r2.patch'),fullBetaAccepted=False),indent=2)+'\n',encoding='utf-8',newline='\n')
print((work/'candidate-r2.json').read_text())
