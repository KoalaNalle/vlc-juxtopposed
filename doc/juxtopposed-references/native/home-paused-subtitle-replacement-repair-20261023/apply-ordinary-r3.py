from pathlib import Path
from hashlib import sha256
import json
work=Path(__file__).resolve().parent;repo=work.parent.parent
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
before=read(work/'native-before-r1/processes.json');after=read(work/'native-candidate-r2/processes.json')
assert len(before['records'])==6 and len(after['records'])==6 and before['allProcessesNormal'] and after['allProcessesNormal']
assert all(not r['passed'] for r in before['records'])
assert all(r['passed'] and r['runtimeUnchanged'] and r['sourceInputsUnchanged'] and r['fixtureUnchanged'] for r in after['records'])
baseline=read(work/'inputs-tests-r2.json')['baselineProductionSha256']
assert all(sha256((repo/p).read_bytes()).hexdigest()==h for p,h in baseline.items())
sources={
    'src/player/input.c':'candidate-preroll-r2/player/input.c',
    'src/input/decoder.c':'candidate-preroll-r2/input/decoder.c',
    'src/input/decoder.h':'candidate-preroll-r2/input/decoder.h',
    'src/input/es_out.c':'candidate-preroll-r2/input/es_out.c',
}
hashes={}
for rel,candidate in sources.items():
    text=(work/candidate).read_text(encoding='utf-8')
    if rel=='src/player/input.c':
        log='''                        msg_Dbg(input->thread,
                                "candidate subtitle frame refresh clock=%"PRId64" target=%"PRId64,
                                clock_time, target);
'''
        assert text.count(log)==1;text=text.replace(log,'')
        needle='''                if (newly_selected && ev->fmt->i_cat == SPU_ES'''
        assert text.count(needle)==1;text=text.replace(needle,'''                /* Refill a new subtitle decoder at the frozen video picture.
                 * Timer points and demux dates use different time origins. */
'''+needle)
    elif rel=='src/input/decoder.c':
        needle='''    const bool exact_paused_target = p_owner->paused'''
        assert text.count(needle)==1;text=text.replace(needle,'''    /* Reordered video may be output before the packet-level preroll boundary.
     * A paused precise seek must wait for its actual picture timestamp.
     * Previous-frame decoding retains its independent preroll window. */
'''+needle)
    elif rel=='src/input/decoder.h':
        needle='void vlc_input_decoder_SetPausedPrerollTarget(vlc_input_decoder_t *, vlc_tick_t);'
        assert text.count(needle)==1;text=text.replace(needle,'/* Exact picture boundary for paused precise seeking. */\n'+needle)
    elif rel=='src/input/es_out.c':
        needle='''        p_sys->i_preroll_end = i_date;
        es_out_id_t *es;'''
        assert text.count(needle)==1;text=text.replace(needle,'''        p_sys->i_preroll_end = i_date;
        /* Retain the exact target after packet-level buffering/preroll ends. */
        es_out_id_t *es;''')
    path=repo/rel;path.write_text(text,encoding='utf-8',newline='\n');hashes[rel]=sha256(path.read_bytes()).hexdigest()
(work/'application-r3.json').write_text(json.dumps(dict(productionApplied=True,files=hashes,candidateCoreSha256='578d86a221eebb945106573d706be9a7f9ed23c818ed57c981e4f7008c3b19ae',candidateOnlyLogRemoved=True,ordinaryBuildAccepted=False,fullBetaAccepted=False),indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps(dict(productionApplied=True,files=list(hashes))))
