from pathlib import Path
from hashlib import sha256
import ast,json
work=Path(__file__).resolve().parent;repo=work.parent.parent
old=repo/'build-windows/beta-tracks-subtitles-20261023'
source=repo/'src/player/input.c';text=source.read_text(encoding='utf-8')
before='''                trackpriv->t.selected = true;
                trackpriv->selected_by_user = ev->forced;'''
after='''                const bool newly_selected = !trackpriv->t.selected;
                trackpriv->t.selected = true;
                trackpriv->selected_by_user = ev->forced;'''
assert text.count(before)==1;text=text.replace(before,after)
before='''                vlc_player_input_HandleTeletextMenu(input, ev, trackpriv);
            }
            break;
        case VLC_INPUT_ES_UNSELECTED:'''
after='''                vlc_player_input_HandleTeletextMenu(input, ev, trackpriv);
                if (newly_selected && ev->fmt->i_cat == SPU_ES
                 && input->state == VLC_PLAYER_STATE_PAUSED
                 && (input->capabilities & VLC_PLAYER_CAP_SEEK)
                 && !input->recording && input == player->input)
                {
                    bool seeking = true;
                    vlc_tick_t time;
                    if (vlc_player_GetTimerPoint(player, &seeking,
                                                VLC_TICK_INVALID, &time, NULL)
                        == VLC_SUCCESS && time >= VLC_TICK_0)
                    {
                        msg_Dbg(input->thread, "candidate paused subtitle refresh @%"PRId64,
                                time);
                        vlc_player_UpdateTimerSeekState(player, time, -1);
                        input_SetTime(input->thread, time, false);
                    }
                }
            }
            break;
        case VLC_INPUT_ES_UNSELECTED:'''
assert text.count(before)==1;text=text.replace(before,after)
folder=work/'refresh-candidate-r3';assert not folder.exists();folder.mkdir()
(folder/'input.c').write_text(text,encoding='utf-8',newline='\n')
make=(old/'core-trace-r9c.mk').read_text(encoding='utf-8')
for a,b in [('TRACE_CORE','CANDIDATE_CORE'),('TRACE_DECODER','CANDIDATE_INPUT'),('owned-decoder-trace-core-r9c','owned-subtitle-refresh-core-r3'),('input/libvlccore_la-decoder.lo','player/libvlccore_la-input.lo'),('decoder.lo','player-input.lo'),('decoder.d','player-input.d'),('-iquote $(srcdir)/input','-iquote $(srcdir)/player')]:make=make.replace(a,b)
(work/'refresh-candidate-r3.mk').write_text(make,encoding='utf-8',newline='\n')
build=(old/'build-core-trace-r9c.sh').read_text(encoding='utf-8')
for a,b in [('beta-tracks-subtitles-20261023','beta-subtitle-workflows-20261023'),('owned-decoder-trace-core-r9c','owned-subtitle-refresh-core-r3'),('build-core-trace-r9c','build-refresh-core-r3'),('core-trace-r9c.mk','refresh-candidate-r3.mk'),('core-trace-r9c-source/decoder.c','refresh-candidate-r3/input.c'),('core-trace-r9c-bin','refresh-candidate-r3-bin'),('decoder.d','player-input.d'),('TRACE_CORE','CANDIDATE_CORE'),('TRACE_DECODER','CANDIDATE_INPUT'),('Private trace Core','Candidate paused subtitle refresh Core')]:build=build.replace(a,b)
(work/'build-refresh-core-r3.sh').write_text(build,encoding='utf-8',newline='\n')
worker=(work/'native-tracks-r1.py').read_text(encoding='utf-8')
worker=worker.replace("    'original-noaudio':(work/'media-r1/multi-language.mkv',True,False,0),", "    'original-noaudio':(work/'media-r1/multi-language.mkv',True,False,0),\n+    'colored-wasapi':(work/'media-generator-smoke-r35/colored-long.mkv',True,True,0),\n+    'colored-noaudio':(work/'media-generator-smoke-r35/colored-long.mkv',True,False,0),".replace('\n+','\n'))
worker=worker.replace("runtimeRole='Ordinary Core000f / MKVf46 / GUId0ff; replay of previous track investigation after interval repair.'", "runtimeRole='Paired unchanged/candidate Core, MKVf46, GUId0ff; timer-watch observations do not call player APIs in callbacks.'")
worker=worker.replace("sourceManifest='build-windows/beta-caption-layout-20261023/inputs-after-r1.json'", "sourceManifest='build-windows/beta-subtitle-workflows-20261023/inputs-r3.json'")
worker=worker.replace("fixtureProducerSha256=digest(work/('generate-fixtures-r1.sh' if mediaPath.parent.name=='media-r1' else 'generate-timestamp-controls-r8.sh'))", "fixtureProducerSha256=digest(repo/'test/libvlc/generate_paused_subtitle_seek.sh' if mediaPath.parent.name=='media-generator-smoke-r35' else work/('generate-fixtures-r1.sh' if mediaPath.parent.name=='media-r1' else 'generate-timestamp-controls-r8.sh'))")
needle="media=mediaNew(str(mediaPath).encode());assert media"
callbacks='''class TimePoint(C.Structure):
    _fields_=[('position',C.c_double),('rate',C.c_double),('ts',C.c_int64),('length',C.c_int64),('systemDate',C.c_int64)]
assert C.sizeof(TimePoint)==40
Update=C.CFUNCTYPE(None,C.c_void_p,C.POINTER(TimePoint))
Paused=C.CFUNCTYPE(None,C.c_void_p,C.c_int64)
timerEvents=[]
def point_data(value):
    p=value.contents
    return dict(position=p.position,rate=p.rate,ts=p.ts,length=p.length,systemDate=p.systemDate)
@Update
def on_update(unused,value):timerEvents.append(dict(event='update',point=point_data(value)))
@Paused
def on_paused(unused,date):timerEvents.append(dict(event='paused',systemDate=date))
@Update
def on_seek(unused,value):timerEvents.append(dict(event='seek',point=point_data(value) if value else None))
class WatchCallbacks(C.Structure):
    _fields_=[('version',C.c_uint32),('onUpdate',Update),('onPaused',Paused),('onSeek',Update)]
assert C.sizeof(WatchCallbacks)==32
callbacks=WatchCallbacks(0,on_update,on_paused,on_seek)
watch=bind('libvlc_media_player_watch_time',C.c_int,C.c_void_p,C.c_int64,C.POINTER(WatchCallbacks),C.c_void_p)
unwatch=bind('libvlc_media_player_unwatch_time',None,C.c_void_p)
assert watch(player,100_000,C.byref(callbacks),None)==0
'''
worker=worker.replace(needle,callbacks+needle)
worker=worker.replace("entry=dict(step=name,state=state(player),timeUs=getTime(player),vouts=vouts(player),subtitleTracks=tracks())", "entry=dict(step=name,state=state(player),timeUs=getTime(player),vouts=vouts(player),subtitleTracks=tracks(),timerEvents=list(timerEvents))")
worker=worker.replace("width=im.width(),height=im.height(),brightPixels=bright)", "width=im.width(),height=im.height(),brightPixels=bright,rgb=[im.pixelColor(480,270).red(),im.pixelColor(480,270).green(),im.pixelColor(480,270).blue()])")
worker=worker.replace("playerRelease(player);release(instance);result['teardownComplete']=True", "unwatch(player);playerRelease(player);release(instance);result['teardownComplete']=True")
ast.parse(worker);(work/'native-tracks-r3.py').write_text(worker,encoding='utf-8',newline='\n')
record=dict(productionApplied=False,baselinePlayerInputSha256=sha256(source.read_bytes()).hexdigest(),candidatePlayerInputSha256=sha256((folder/'input.c').read_bytes()).hexdigest(),scope='Refresh a newly selected subtitle on paused seekable nonrecording current input using its uninterpolated timer point, leaving normal playing/nonseekable/recording paths untouched. Diagnostic log in candidate only. No ordinary acceptance yet.')
(work/'refresh-candidate-r3.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps(record))
