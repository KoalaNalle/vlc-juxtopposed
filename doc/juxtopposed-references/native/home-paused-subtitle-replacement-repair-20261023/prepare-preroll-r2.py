from pathlib import Path
from hashlib import sha256
import ast,json
work=Path(__file__).resolve().parent;repo=work.parent.parent
folder=work/'candidate-preroll-r2';assert not folder.exists();(folder/'input').mkdir(parents=True);(folder/'player').mkdir()
paths=['src/input/decoder.c','src/input/decoder.h','src/input/es_out.c','src/player/input.c']
baseline={p:sha256((repo/p).read_bytes()).hexdigest() for p in paths}
decoder=(repo/'src/input/decoder.c').read_text(encoding='utf-8')
needle='    vlc_tick_t i_preroll_end;'
assert decoder.count(needle)==1;decoder=decoder.replace(needle,needle+'\n    vlc_tick_t i_paused_preroll_target;')
needle='''    p_owner->i_preroll_end = PREROLL_NONE;
    p_owner->p_resource = cfg->resource;'''
assert decoder.count(needle)==1;decoder=decoder.replace(needle,'''    p_owner->i_preroll_end = PREROLL_NONE;
    p_owner->i_paused_preroll_target = VLC_TICK_INVALID;
    p_owner->p_resource = cfg->resource;''')
needle='''    enum es_format_category_e cat = p_owner->cat;

    /* Empty the fifo */'''
assert decoder.count(needle)==1;decoder=decoder.replace(needle,'''    enum es_format_category_e cat = p_owner->cat;
    p_owner->i_paused_preroll_target = VLC_TICK_INVALID;

    /* Empty the fifo */''')
needle='''    bool prerolled = p_owner->i_preroll_end != PREROLL_NONE;
    if( prerolled && p_owner->i_preroll_end > p_picture->date )'''
replacement='''    bool prerolled = p_owner->i_preroll_end != PREROLL_NONE;
    const bool exact_paused_target = p_owner->paused
        && p_owner->frames_countdown >= 0
        && p_owner->i_paused_preroll_target != VLC_TICK_INVALID;
    if (exact_paused_target
        ? p_picture->date < p_owner->i_paused_preroll_target
        : prerolled && p_owner->i_preroll_end > p_picture->date)'''
assert decoder.count(needle)==1;decoder=decoder.replace(needle,replacement)
needle='''    p_owner->i_preroll_end = PREROLL_NONE;

    if( unlikely(prerolled) )'''
assert decoder.count(needle)==2
# Restrict replacement to the video function, leaving audio unchanged.
start=decoder.index('static int ModuleThread_PlayVideo(');end=decoder.index('static void ModuleThread_QueueVideo',start)
video=decoder[start:end];assert video.count(needle)==1
video=video.replace(needle,'''    p_owner->i_preroll_end = PREROLL_NONE;
    p_owner->i_paused_preroll_target = VLC_TICK_INVALID;

    if( unlikely(prerolled) )''')
decoder=decoder[:start]+video+decoder[end:]
needle='void vlc_input_decoder_Flush( vlc_input_decoder_t *p_owner )'
method='''void vlc_input_decoder_SetPausedPrerollTarget(vlc_input_decoder_t *owner,
                                               vlc_tick_t target)
{
    assert(owner->cat == VIDEO_ES);
    vlc_fifo_Lock(owner->p_fifo);
    owner->i_paused_preroll_target = target;
    vlc_fifo_Unlock(owner->p_fifo);
}

'''
assert decoder.count(needle)==1;decoder=decoder.replace(needle,method+needle)
header=(repo/'src/input/decoder.h').read_text(encoding='utf-8')
needle='void vlc_input_decoder_ChangePause( vlc_input_decoder_t *, bool b_paused, vlc_tick_t i_date );'
assert header.count(needle)==1;header=header.replace(needle,needle+'\nvoid vlc_input_decoder_SetPausedPrerollTarget(vlc_input_decoder_t *, vlc_tick_t);')
esout=(repo/'src/input/es_out.c').read_text(encoding='utf-8')
needle='''        p_sys->i_preroll_end = i_date;

        return VLC_SUCCESS;'''
assert esout.count(needle)==1
esout=esout.replace(needle,'''        p_sys->i_preroll_end = i_date;
        es_out_id_t *es;
        foreach_es_then_es_slaves(es)
        {
            if (es->p_dec && es->fmt.i_cat == VIDEO_ES)
                vlc_input_decoder_SetPausedPrerollTarget(es->p_dec, i_date);
        }

        return VLC_SUCCESS;''')
(folder/'input/decoder.c').write_text(decoder,encoding='utf-8',newline='\n')
(folder/'input/decoder.h').write_text(header,encoding='utf-8',newline='\n')
(folder/'input/es_out.c').write_text(esout,encoding='utf-8',newline='\n')
(folder/'player/input.c').write_bytes((repo/'build-windows/beta-subtitle-frame-conservation-20261023/candidate-r1/input.c').read_bytes())
parent=(work/'run-native-r1.py').read_text(encoding='utf-8')
parent=parent.replace("choices=['before','candidate','ordinary']", "choices=['candidate']")
parent=parent.replace("manifest=work/('inputs-ordinary-r2.json' if args.phase=='ordinary' else 'inputs-tests-r1.json')", "manifest=work/'inputs-tests-r2.json'")
parent=parent.replace("root=work/('native-'+args.phase+'-r1')", "root=work/'native-candidate-r2'")
parent=parent.replace("repo/'build-windows/beta-subtitle-frame-conservation-20261023/candidate-r1-bin/libvlccore.dll' if args.phase=='candidate' else work/'ordinary-core-r2/libvlccore.dll'", "work/'candidate-preroll-r2-bin/libvlccore.dll'")
ast.parse(parent);(work/'run-native-r2.py').write_text(parent,encoding='utf-8',newline='\n')
prior=json.loads((work/'inputs-tests-r1.json').read_text(encoding='utf-8-sig'))['files']
assert all(sha256((repo/p).read_bytes()).hexdigest()==h for p,h in prior.items())
files=set(prior);files.update(p.relative_to(repo).as_posix() for p in work.iterdir() if p.suffix in ['.py','.sh','.mk'])
files.update(p.relative_to(repo).as_posix() for p in folder.rglob('*') if p.is_file())
hashes={p:sha256((repo/p).read_bytes()).hexdigest() for p in sorted(files)}
snapshot=work/'source-tests-r2';snapshot.mkdir()
import shutil
for p,h in hashes.items():
    target=snapshot/p;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(repo/p,target);assert sha256(target.read_bytes()).hexdigest()==h
(work/'inputs-tests-r2.json').write_text(json.dumps(dict(files=hashes,sourceCount=len(hashes),sourceSnapshot=str(snapshot),productionApplied=False,baselineProductionSha256=baseline,fullBetaAccepted=False),indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps(dict(productionApplied=False,sourceCount=len(hashes),baselineProductionSha256=baseline)))
