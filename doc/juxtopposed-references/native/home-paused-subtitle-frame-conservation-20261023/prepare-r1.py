from pathlib import Path
from hashlib import sha256
import ast,json
work=Path(__file__).resolve().parent;repo=work.parent.parent
old=repo/'build-windows/beta-subtitle-workflows-20261023'
source=old/'refresh-candidate-r4/input.c';text=source.read_text(encoding='utf-8')
before='''                    const vlc_tick_t time =
                        vlc_player_input_GetTime(input, true, vlc_tick_now());
                    if (time >= VLC_TICK_0)
                    {
                        msg_Dbg(input->thread, "candidate paused subtitle refresh @%"PRId64,
                                time);
                        vlc_player_UpdateTimerSeekState(player, time, -1);
                        input_SetTime(input->thread, time, false);
                    }'''
after='''                    bool have_target = false;
                    vlc_tick_t target = 0, clock_time = VLC_TICK_INVALID;
                    vlc_mutex_lock(&player->timer.lock);
                    const struct vlc_player_timer_source *video =
                        &player->timer.smpte_source;
                    if (video->es && !video->seeking
                     && video->point.system_date != VLC_TICK_INVALID
                     && video->point.ts >= VLC_TICK_0)
                    {
                        /* The timer point includes the VLC_TICK_0 origin.
                         * Demux seek dates are relative and add that origin. */
                        clock_time = video->point.ts;
                        target = clock_time - VLC_TICK_0;
                        have_target = true;
                    }
                    else if (player->timer.seek_ts != VLC_TICK_INVALID)
                    {
                        /* No new picture yet: retain the pending user target. */
                        target = clock_time = player->timer.seek_ts;
                        have_target = true;
                    }
                    vlc_mutex_unlock(&player->timer.lock);
                    if (have_target)
                    {
                        msg_Dbg(input->thread,
                                "candidate subtitle frame refresh clock=%"PRId64" target=%"PRId64,
                                clock_time, target);
                        vlc_player_UpdateTimerSeekState(player, clock_time, -1);
                        input_SetTime(input->thread, target, false);
                    }'''
assert text.count(before)==1
folder=work/'candidate-r1';assert not folder.exists();folder.mkdir()
(folder/'input.c').write_text(text.replace(before,after),encoding='utf-8',newline='\n')
make=(old/'refresh-candidate-r4.mk').read_text(encoding='utf-8').replace('owned-subtitle-refresh-core-r4','owned-subtitle-frame-core-r1')
(work/'candidate-r1.mk').write_text(make,encoding='utf-8',newline='\n')
build=(old/'build-refresh-core-r4.sh').read_text(encoding='utf-8')
for a,b in [('beta-subtitle-workflows-20261023','beta-subtitle-frame-conservation-20261023'),('owned-subtitle-refresh-core-r4','owned-subtitle-frame-core-r1'),('build-refresh-core-r4','build-core-r1'),('refresh-candidate-r4.mk','candidate-r1.mk'),('refresh-candidate-r4/input.c','candidate-r1/input.c'),('refresh-candidate-r4-bin','candidate-r1-bin')]:build=build.replace(a,b)
(work/'build-core-r1.sh').write_text(build,encoding='utf-8',newline='\n')
worker=(old/'native-tracks-r4.py').read_text(encoding='utf-8')
worker=worker.replace("repo=Path('C:/Modding/vlc-juxtopposed');", "repo=Path('C:/Modding/vlc-juxtopposed');newWork=repo/'build-windows/beta-subtitle-frame-conservation-20261023';")
worker=worker.replace("    'colored-noaudio':(work/'media-generator-smoke-r35/colored-long.mkv',True,False,0),", "    'colored-noaudio':(work/'media-generator-smoke-r35/colored-long.mkv',True,False,0),\n+    'marker-wasapi':(newWork/'media-r1/frame-marker.mkv',True,True,0),\n+    'marker-noaudio':(newWork/'media-r1/frame-marker.mkv',True,False,0),".replace('\n+','\n'))
worker=worker.replace("sourceManifest='build-windows/beta-subtitle-workflows-20261023/inputs-r4.json'", "sourceManifest='build-windows/beta-subtitle-frame-conservation-20261023/inputs-r1.json'")
worker=worker.replace("fixtureProducerSha256=digest(repo/'test/libvlc/generate_paused_subtitle_seek.sh' if mediaPath.parent.name=='media-generator-smoke-r35' else", "fixtureProducerSha256=digest(newWork/'generate-marker-r1.sh' if mediaPath.name=='frame-marker.mkv' else repo/'test/libvlc/generate_paused_subtitle_seek.sh' if mediaPath.parent.name=='media-generator-smoke-r35' else")
needle="    result['observations'].append(entry);write('investigation.json',result);"
extra='''    if entry.get('snapshotAvailable') and mediaPath.name=='frame-marker.mkv':
        entry['frameMarker']=sum((1<<bit) if im.pixelColor(32+64*bit,20).red()>128 else 0 for bit in range(11))
    entry['pausedSamples']=[dict(state=state(player),timeUs=getTime(player))]
    for unused in range(2):
        time.sleep(.1);entry['pausedSamples'].append(dict(state=state(player),timeUs=getTime(player)))
'''
assert worker.count(needle)==1;worker=worker.replace(needle,extra+needle)
needle="        result['externalAddReturn']=slave"
extra='''        for cycle,language in enumerate(['eng','fin','eng','fin']):
            chosen=next(t for t in available if t['language']==language)
            select(player,2,chosen['id'].encode());wait(lambda:any(t['id']==chosen['id'] and t['selected'] for t in tracks()),3);time.sleep(.7);observe('repeat-'+str(cycle)+'-'+language)
'''
assert worker.count(needle)==1;worker=worker.replace(needle,extra+needle)
ast.parse(worker);(work/'native-tracks-r1.py').write_text(worker,encoding='utf-8',newline='\n')
record=dict(productionApplied=False,baselinePlayerInputSha256=sha256((repo/'src/player/input.c').read_bytes()).hexdigest(),candidatePlayerInputSha256=sha256((folder/'input.c').read_bytes()).hexdigest(),scope='Use current video/SMPTE point, remove tick-origin before demux seek, retain pending user seek if no current video point. Test actual binary frame markers and repeated selection; no acceptance yet.')
(work/'candidate-r1.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps(record))
