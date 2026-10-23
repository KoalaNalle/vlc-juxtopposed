from pathlib import Path
from hashlib import sha256
import json, shutil, subprocess

work = Path(__file__).resolve().parent
repo = work.parent.parent
read = lambda p: json.loads(p.read_text(encoding='utf-8-sig'))
digest = lambda p: sha256(p.read_bytes()).hexdigest()


def write(p, value):
    p.write_text(json.dumps(value, indent=2)+'\n', encoding='utf-8', newline='\n')


prior = read(work/'inputs-native-r3.json')['files']
changed = [p for p, h in prior.items() if digest(repo/p) != h]
assert changed == ['test/libvlc/native_paused_subtitle_delay.md',
                   'test/libvlc/native_paused_subtitle_delay.py'], changed
for batch in ['ass-wasapi', 'formats']:
    summary = read(work/('native-delay-'+batch+'-r3')/'processes.json')
    assert summary['allProcessesNormal']
    assert all(r['sourceInputsUnchanged'] and r['runtimeUnchanged']
               and r['fixturesUnchanged'] for r in summary['records'])
root = work/'candidate-r4'
assert not root.exists()
root.mkdir()
sources = {}
names = ['src/input/decoder.c', 'src/input/es_out.c', 'src/input/es_out.h',
         'src/input/input.c', 'src/player/input.c', 'src/player/player.c',
         'src/player/player.h']
for name in names:
    sources[name] = (repo/name).read_text(encoding='utf-8-sig')


def replace(name, old, new):
    assert sources[name].count(old) == 1, (name, old[:120])
    sources[name] = sources[name].replace(old, new)


replace('src/input/decoder.c', '''    owner->delay = delay;
    vlc_fifo_Unlock( owner->p_fifo );''', '''    owner->delay = delay;
    vlc_fifo_Signal( owner->p_fifo );
    vlc_fifo_Unlock( owner->p_fifo );''')
replace('src/input/es_out.h', '''    ES_OUT_PRIV_SELECT_SOURCE /* arg1=input_source_t *, arg2=es_format_category_e */''',
'''    ES_OUT_PRIV_SELECT_SOURCE, /* arg1=input_source_t *, arg2=es_format_category_e */

    /* Earlier media needed to retain delayed selected subtitle cues on seek. */
    ES_OUT_PRIV_GET_SUBTITLE_PREROLL /* arg1=vlc_tick_t *, res=cannot fail */''')
replace('src/input/es_out.c', '''    case ES_OUT_PRIV_SET_DELAY:
    {''', '''    case ES_OUT_PRIV_GET_SUBTITLE_PREROLL:
    {
        vlc_tick_t *delay = va_arg(args, vlc_tick_t *);
        *delay = 0;
        es_out_id_t *es;
        foreach_es_then_es_slaves(es)
        {
            if (es->fmt.i_cat != SPU_ES || !es->p_dec)
                continue;
            const vlc_tick_t selected_delay = es->delay != VLC_TICK_MAX
                                           ? es->delay : p_sys->i_spu_delay;
            *delay = __MAX(*delay, selected_delay);
        }
        return VLC_SUCCESS;
    }
    case ES_OUT_PRIV_SET_DELAY:
    {''')
replace('src/input/es_out.c', '''        /* If i_date is still invalid (first/all non dated), expect to be in preroll */
        if( i_date == VLC_TICK_INVALID ||
            es->i_pts_level < p_sys->i_preroll_end )''',
'''        vlc_tick_t preroll_end = p_sys->i_preroll_end;
        if (es->fmt.i_cat == SPU_ES)
        {
            const vlc_tick_t delay = es->delay != VLC_TICK_MAX
                                   ? es->delay : p_sys->i_spu_delay;
            /* A delayed cue can remain visible after its media stop date.
             * Compare subtitle packets with their own media-time boundary. */
            if (delay > 0)
                preroll_end = delay >= preroll_end ? 0 : preroll_end - delay;
        }
        /* If i_date is still invalid (first/all non dated), expect to be in preroll */
        if( i_date == VLC_TICK_INVALID ||
            es->i_pts_level < preroll_end )''')
replace('src/input/input.c', '''    i_ret = demux_SetTime( priv->master->p_demux, priv->i_start + val,
                           !fast_seek );''',
'''    const vlc_tick_t requested = priv->i_start + val;
    vlc_tick_t subtitle_preroll = 0;
    if (!fast_seek && !priv->prev_frame.enabled)
        es_out_PrivControl(priv->p_es_out_display,
                           ES_OUT_PRIV_GET_SUBTITLE_PREROLL, &subtitle_preroll);
    const vlc_tick_t demux_target = subtitle_preroll > 0 && requested > 0
                                 ? __MAX(requested - subtitle_preroll, 0)
                                 : requested;
    i_ret = demux_SetTime( priv->master->p_demux, demux_target,
                           !fast_seek );''')
replace('src/input/input.c', '''            double f_pos = (double)(priv->i_start + val) / (double)i_length;''',
'''            double f_pos = (double)demux_target / (double)i_length;''')
old = '''    if( i_ret == VLC_SUCCESS )
    {
        if( priv->i_slave > 0 )
            SlaveSeek( p_input );
        priv->master->b_eof = false;
    }

    return i_ret;
}

static int ControlSetPosition'''
new = '''    if( i_ret == VLC_SUCCESS )
    {
        /* Decode earlier delayed subtitle cues, but retain the requested
         * video/audio display boundary and the source's timestamp origin. */
        if (demux_target != requested)
            es_out_Control(&priv->p_es_out->out, ES_OUT_SET_NEXT_DISPLAY_TIME,
                           priv->master->i_normal_time + requested);
        if( priv->i_slave > 0 )
            SlaveSeek( p_input );
        priv->master->b_eof = false;
    }

    return i_ret;
}

static int ControlSetPosition'''
replace('src/input/input.c', old, new)
replace('src/input/input.c', '''    int i_ret = demux_SetPosition( priv->master->p_demux, val, !fast_seek );''',
'''    if (!fast_seek && i_length > 0)
    {
        vlc_tick_t subtitle_preroll = 0;
        es_out_PrivControl(priv->p_es_out_display,
                           ES_OUT_PRIV_GET_SUBTITLE_PREROLL, &subtitle_preroll);
        if (subtitle_preroll > 0)
            return ControlSetTime(p_input, llround(val * i_length) - priv->i_start,
                                  false);
    }
    int i_ret = demux_SetPosition( priv->master->p_demux, val, !fast_seek );''')

name = 'src/player/input.c'
start = sources[name].index('                /* Refill a new subtitle decoder at the frozen video picture.')
end_marker = '''                }
            }
            break;
        case VLC_INPUT_ES_UNSELECTED:'''
end = sources[name].index(end_marker, start) + len('                }')
original = sources[name][start:end]
assert 'if (newly_selected && ev->fmt->i_cat == SPU_ES' in original
sources[name] = sources[name][:start] + '''                if (newly_selected && ev->fmt->i_cat == SPU_ES)
                    vlc_player_input_RefreshPausedSubtitle(input);''' + sources[name][end:]
helper = '''void
vlc_player_input_RefreshPausedSubtitle(struct vlc_player_input *input)
{
    vlc_player_t *player = input->player;
    if (input->state != VLC_PLAYER_STATE_PAUSED
     || !(input->capabilities & VLC_PLAYER_CAP_SEEK)
     || input->recording || input != player->input)
        return;

    bool have_target = false;
    vlc_tick_t target = 0, clock_time = VLC_TICK_INVALID;
    vlc_mutex_lock(&player->timer.lock);
    const struct vlc_player_timer_source *video = &player->timer.smpte_source;
    if (video->es && !video->seeking
     && video->point.system_date != VLC_TICK_INVALID
     && video->point.ts >= VLC_TICK_0)
    {
        clock_time = video->point.ts;
        target = clock_time - VLC_TICK_0;
        have_target = true;
    }
    else if (player->timer.seek_ts != VLC_TICK_INVALID)
    {
        target = clock_time = player->timer.seek_ts;
        have_target = true;
    }
    vlc_mutex_unlock(&player->timer.lock);
    if (have_target)
    {
        vlc_player_UpdateTimerSeekState(player, clock_time, -1);
        input_SetTime(input->thread, target, false);
    }
}

static void
vlc_player_input_HandleEsEvent'''
replace(name, 'static void\nvlc_player_input_HandleEsEvent', helper)
replace('src/player/player.h', '''void
vlc_player_input_RestoreMlStates''', '''void
vlc_player_input_RefreshPausedSubtitle(struct vlc_player_input *input);

void
vlc_player_input_RestoreMlStates''')
replace('src/player/player.c', '''    vlc_tick_t *cat_delay = &input->cat_delays[cat];''',
'''    vlc_tick_t *cat_delay = &input->cat_delays[cat];
    const vlc_tick_t old_delay = *cat_delay;''')
replace('src/player/player.c', '''        vlc_player_SendEvent(player, on_category_delay_changed, cat, delay);''',
'''        vlc_player_SendEvent(player, on_category_delay_changed, cat, delay);
        if (cat == SPU_ES && old_delay != delay)
            vlc_player_input_RefreshPausedSubtitle(input);''')
replace('src/player/player.c', '''    if (absolute)
        trackpriv->delay = delay;''', '''    const vlc_tick_t old_delay = trackpriv->delay;
    if (absolute)
        trackpriv->delay = delay;''')
replace('src/player/player.c', '''        vlc_player_SendEvent(player, on_track_delay_changed, es_id, delay);''',
'''        vlc_player_SendEvent(player, on_track_delay_changed, es_id, delay);
        if (trackpriv->t.fmt.i_cat == SPU_ES && old_delay != delay)
            vlc_player_input_RefreshPausedSubtitle(input);''')

patches = []
for name, source in sources.items():
    destination = root/name
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(source, encoding='utf-8', newline='\n')
    diff = subprocess.run(['git', 'diff', '--no-index', '--', str(repo/name), str(destination)],
                          cwd=repo, capture_output=True)
    assert diff.returncode == 1, name
    patches.append(diff.stdout)
(work/'candidate-r4.patch').write_bytes(b''.join(patches))
files = set(prior)
files.update((root/name).relative_to(repo).as_posix() for name in names)
files.update('build-windows/beta-subtitle-preferences-20261023/'+p for p in [
    'prepare-candidate-r4.py', 'candidate-r4.mk', 'build-candidate-r4.sh',
    'run-delay-r4.py', 'analyze-masks-r3.py'])
snapshot = work/'source-candidate-r4'
assert not snapshot.exists()
snapshot.mkdir()
inputs = {p: digest(repo/p) for p in sorted(files)}
for name, h in inputs.items():
    destination = snapshot/name
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(repo/name, destination)
    assert digest(destination) == h
write(work/'inputs-candidate-r4.json', dict(files=inputs, sourceCount=len(inputs),
      sourceSnapshot=str(snapshot), productionApplied=False, fullBetaAccepted=False))
write(work/'candidate-r4.json', dict(productionApplied=False, fullBetaAccepted=False,
      files={p: digest(root/p) for p in names}, preimages={p: digest(repo/p) for p in names},
      sourceCount=len(inputs), hypothesis='Retain delayed cues with earlier demux '
      'preroll and delay-aware SPU packet cutoff, wake delay consumers and refill '
      'seekable paused subtitles at the actual held video timestamp.'))
print('Prepared private candidate;', len(inputs), 'actual input copies; production unchanged', flush=True)
