from pathlib import Path
from hashlib import sha256
import json,shutil,subprocess
work=Path(__file__).resolve().parent;repo=work.parent.parent
prior=repo/'build-windows/beta-subtitle-replacement-repair-20261023'
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
last=read(prior/'final-verification-r6.json');assert last['passed']
old=read(prior/'inputs-ordinary-r2.json')['files']
expected=set(last['onlyPostFreezeChanges'])|{'test/libvlc/native_paused_subtitle_tracks.py','test/libvlc/native_paused_subtitle_tracks.md'}
assert {p for p,h in old.items() if digest(repo/p)!=h}==expected
assert all(digest(repo/p)==h for p,h in read(prior/'application-r3.json')['files'].items())
proposal=work/'candidate-r1';assert not proposal.exists();proposal.mkdir()
for name in ['input.c','es_out.c','es_out.h']:
    shutil.copyfile(repo/'src/input'/name,proposal/name)
p=proposal/'es_out.h';s=p.read_text(encoding='utf-8-sig')
old_enum='    ES_OUT_PRIV_SET_VBI_TRANSPARENCY                /* arg1=bool res=can fail */\n'
assert s.count(old_enum)==1
s=s.replace(old_enum,'    ES_OUT_PRIV_SET_VBI_TRANSPARENCY,               /* arg1=bool res=can fail */\n\n    /* Select the preferred track from an explicitly added source. */\n    ES_OUT_PRIV_SELECT_SOURCE /* arg1=input_source_t *, arg2=es_format_category_e */\n')
p.write_text(s,encoding='utf-8',newline='\n')
p=proposal/'input.c';s=p.read_text(encoding='utf-8-sig')
needle='    TAB_APPEND( priv->i_slave, priv->slave, p_source );\n\n    return VLC_SUCCESS;'
assert s.count(needle)==1
s=s.replace(needle,'''    TAB_APPEND( priv->i_slave, priv->slave, p_source );

    /* A runtime load-and-select request is an explicit choice, rather than
     * another candidate for language/ID-based automatic selection. */
    if( i_type == SLAVE_TYPE_SPU && b_forced && b_set_time )
        es_out_PrivControl( priv->p_es_out_display, ES_OUT_PRIV_SELECT_SOURCE,
                            p_source, SPU_ES );

    return VLC_SUCCESS;''')
p.write_text(s,encoding='utf-8',newline='\n')
p=proposal/'es_out.c';s=p.read_text(encoding='utf-8-sig')
needle='    case ES_OUT_PRIV_SET_ES_CAT_IDS:\n'
assert s.count(needle)==1
s=s.replace(needle,'''    case ES_OUT_PRIV_SELECT_SOURCE:
    {
        input_source_t *requested = va_arg( args, input_source_t * );
        enum es_format_category_e cat = va_arg( args, enum es_format_category_e );
        es_out_es_props_t *props = GetPropsByCat( p_sys, cat );
        if( !requested || !props || !p_sys->b_active
         || p_sys->i_mode != ES_OUT_MODE_AUTO )
            return VLC_EGENERIC;

        /* Keep language/priority ordering inside a multi-track external file,
         * but do not let the previously selected source veto this request. */
        es_out_id_t *best = NULL, *es;
        int best_language = -1;
        foreach_es_then_es_slaves( es )
        {
            if( es->id.source != requested || es->fmt.i_cat != cat
             || es->p_pgrm != p_sys->p_pgrm || es->b_terminated )
                continue;
            int language = props->ppsz_language
                ? LanguageArrayIndex( props->ppsz_language,
                                      es->psz_language_code ) : -1;
            if( !best || (language >= 0
                && (best_language < 0 || language < best_language))
             || (language == best_language
                && es->fmt.i_priority > best->fmt.i_priority) )
            {
                best = es;
                best_language = language;
            }
        }
        if( !best )
            return VLC_EGENERIC;

        EsOutSelect( p_sys, best, true );
        return EsIsSelected( best ) ? VLC_SUCCESS : VLC_EGENERIC;
    }
'''+needle)
p.write_text(s,encoding='utf-8',newline='\n')
files=set(old)
files.update(p.relative_to(repo).as_posix() for p in prior.iterdir() if p.suffix in ['.py','.sh','.mk'])
files.update(p.relative_to(repo).as_posix() for p in work.rglob('*') if p.is_file() and p.suffix in ['.py','.sh','.mk','.c','.h'])
hashes={p:digest(repo/p) for p in sorted(files)}
snapshot=work/'source-r1';assert not snapshot.exists();snapshot.mkdir()
for p,h in hashes.items():
    d=snapshot/p;d.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(repo/p,d);assert digest(d)==h
write(work/'inputs-r1.json',dict(files=hashes,sourceCount=len(hashes),sourceSnapshot=str(snapshot),changedSinceParent=sorted(expected),productionApplied=False,fullBetaAccepted=False))
write(work/'candidate-r1.json',dict(files={p.name:digest(p) for p in proposal.iterdir()},productionApplied=False,scope='Runtime explicit subtitle source selection after successful source initialization. Automatic discovery and load-only requests retain their existing rules.',remaining='Verify single/multiple-track source registration timing, selection policy, repeated additions, invalid paths and subsequent explicit choices.'))
subprocess.run(['git','diff','--check'],cwd=repo,check=True)
print(json.dumps(dict(sourceCount=len(hashes),productionApplied=False,candidateFiles=list(read(work/'candidate-r1.json')['files']))))
