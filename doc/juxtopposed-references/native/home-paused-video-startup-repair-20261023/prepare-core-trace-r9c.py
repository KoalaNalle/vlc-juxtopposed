from pathlib import Path
from hashlib import sha256
import json
repo=Path('C:/Modding/vlc-juxtopposed');work=Path(__file__).resolve().parent
digest=lambda p:sha256(p.read_bytes()).hexdigest()
inputs=json.loads((work/'inputs-r4.json').read_text(encoding='utf-8-sig'))
source=repo/'src/input/decoder.c';assert digest(source)==inputs['src/input/decoder.c']
out=work/'core-trace-r9c-source';assert not out.exists();out.mkdir()
original=source.read_text(encoding='utf-8');text=original;changes=[]
def function(signature,transform):
    global text
    assert text.count(signature)==1,signature
    start=text.index(signature);opening=text.index('{',start);depth=1;end=opening+1
    while depth:
        depth+=(text[end]=='{')-(text[end]=='}');end+=1
    before=text[start:end];after=transform(before);assert after!=before,signature
    text=text[:start]+after+text[end:];changes.append(signature)
def replace(body,old,new):
    assert body.count(old)==1,(old,body.count(old));return body.replace(old,new)
flags=' cat=%d id=%s paused=%d outpaused=%d first=%d waiting=%d has=%d idle=%d count=%zu next=%d", p_owner->cat, p_owner->psz_id ? p_owner->psz_id : "?", p_owner->paused, p_owner->output_paused, p_owner->b_first, p_owner->b_waiting, p_owner->b_has_data, p_owner->b_idle, vlc_fifo_GetCount(p_owner->p_fifo), p_owner->frames_countdown'
def trace(label):return '    msg_Dbg(&p_owner->dec, "OWNED_DECODER_TRACE '+label+flags+');\n'
def wait_body(body):
    body=replace(body,'    while( !p_owner->b_has_data )',trace('WAIT_ENTER')+'    while( !p_owner->b_has_data )')
    body=replace(body,'            msg_Err( &p_owner->dec, "buffer deadlock prevented" );',trace('WAIT_IDLE_EMPTY')+'            msg_Err( &p_owner->dec, "buffer deadlock prevented" );')
    body=replace(body,'    vlc_fifo_Unlock(p_owner->p_fifo);',trace('WAIT_EXIT')+'    vlc_fifo_Unlock(p_owner->p_fifo);')
    return body
function('void vlc_input_decoder_Wait(',wait_body)
def pause_body(body):
    body=replace(body,'    p_owner->paused = b_paused;',trace('CHANGE_PAUSE_BEFORE')+'    msg_Dbg(&p_owner->dec, "OWNED_DECODER_TRACE PAUSE_REQUEST paused=%d date=%"PRId64, b_paused, (int64_t)i_date);\n    p_owner->paused = b_paused;')
    return replace(body,'    vlc_fifo_Signal( p_owner->p_fifo );',trace('CHANGE_PAUSE_AFTER')+'    vlc_fifo_Signal( p_owner->p_fifo );')
function('void vlc_input_decoder_ChangePause(',pause_body)
def unblock_body(body):
    body=replace(body,'    vlc_fifo_Assert(p_owner->p_fifo);','    vlc_fifo_Assert(p_owner->p_fifo);\n    const bool owned_trace_wait = p_owner->b_waiting || !p_owner->out_started;\n    if (owned_trace_wait) {\n'+trace('UNBLOCK_ENTER')+'    msg_Dbg(&p_owner->dec, "OWNED_DECODER_TRACE UNBLOCK_DATE date=%"PRId64, (int64_t)date);\n    }')
    return replace(body,'    return VLC_SUCCESS;','    if (owned_trace_wait) {\n'+trace('UNBLOCK_EXIT')+'    }\n    return VLC_SUCCESS;')
function('static int DecoderWaitUnblock(',unblock_body)
def video_body(body):
    return replace(body,'    assert( p_owner->cat == VIDEO_ES );','    assert( p_owner->cat == VIDEO_ES );\n    if (p_owner->b_first || p_owner->b_waiting || (p_owner->output_paused && p_owner->frames_countdown > 0)) {\n'+trace('VIDEO_PICTURE')+'    msg_Dbg(p_dec, "OWNED_DECODER_TRACE VIDEO_DATE date=%"PRId64" preroll=%"PRId64, (int64_t)p_picture->date, (int64_t)p_owner->i_preroll_end);\n    }')
function('static int ModuleThread_PlayVideo(',video_body)
def spu_body(body):
    return replace(body,'    vlc_fifo_Lock(p_owner->p_fifo);','    vlc_fifo_Lock(p_owner->p_fifo);\n    msg_Dbg(p_dec, "OWNED_DECODER_TRACE SPU_CUE id=%s start=%"PRId64" stop=%"PRId64" preroll=%"PRId64, p_owner->psz_id ? p_owner->psz_id : "?", (int64_t)p_spu->i_start, (int64_t)p_spu->i_stop, (int64_t)p_owner->i_preroll_end);')
function('static void ModuleThread_QueueSpu(',spu_body)
def enqueue_body(body):
    return replace(body,'    vlc_fifo_QueueUnlocked( p_owner->p_fifo, frame );','    if (p_owner->cat == SPU_ES || (p_owner->cat == VIDEO_ES && p_owner->b_first))\n        msg_Dbg(&p_owner->dec, "OWNED_DECODER_TRACE ENQUEUE cat=%d id=%s pts=%"PRId64" dts=%"PRId64" flags=%x paused=%d count=%zu", p_owner->cat, p_owner->psz_id ? p_owner->psz_id : "?", (int64_t)frame->i_pts, (int64_t)frame->i_dts, frame->i_flags, p_owner->paused, vlc_fifo_GetCount(p_owner->p_fifo));\n    vlc_fifo_QueueUnlocked( p_owner->p_fifo, frame );')
function('void vlc_input_decoder_DecodeWithStatus(',enqueue_body)
(out/'decoder-original.c').write_bytes(source.read_bytes());(out/'decoder.c').write_text(text,encoding='utf-8',newline='\n')
record=dict(diagnosticOnly=True,productionSourceUnchanged=digest(source)==inputs['src/input/decoder.c'],originalSha256=digest(source),traceSha256=digest(out/'decoder.c'),producerSha256=digest(Path(__file__)),instrumentedFunctions=changes,scope='Logging statements and one local observation flag only in a separate decoder translation unit; production source, object and ordinary Core remain untouched. This is diagnostic evidence, not a repair or acceptance build.')
(out/'source-record.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps(record,indent=2))
