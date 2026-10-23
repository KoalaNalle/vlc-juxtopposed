from pathlib import Path
from hashlib import sha256
import json,shutil
work=Path(__file__).resolve().parent;repo=work.parent.parent
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
old=read(work/'inputs-r2.json')['files'];assert all(digest(repo/p)==h for p,h in old.items())
failed=read(work/'native-extra-r4/candidate-multi-language.mks/probe/result.json')
assert not failed['passed'] and failed['checks']['automaticExternalSelected'] and not failed['checks']['externalAdditionFrameConserved']
dest=work/'candidate-r3';assert not dest.exists();shutil.copytree(work/'candidate-r2',dest)
shutil.copyfile(repo/'src/input/es_out_source.c',dest/'es_out_source.c')
p=dest/'input.c';s=p.read_text(encoding='utf-8-sig')
needle='    if( b_forced )\n        p_source->autoselected = true;'
assert s.count(needle)==1
s=s.replace(needle,'    /* Demux opening may already publish timing controls. */\n    p_source->b_slave_sub = i_type == SLAVE_TYPE_SPU;\n\n'+needle)
needle='    else\n        p_source->b_slave_sub = true;\n\n    TAB_APPEND( priv->i_slave, priv->slave, p_source );'
assert s.count(needle)==1;s=s.replace(needle,'\n    TAB_APPEND( priv->i_slave, priv->slave, p_source );')
p.write_text(s,encoding='utf-8',newline='\n')
p=dest/'es_out_source.c';s=p.read_text(encoding='utf-8-sig')
needle='    return sys->parent_out->out.cbs->control(&sys->parent_out->out, sys->in, query, args);'
assert s.count(needle)==1
s=s.replace(needle,'''    /* Subtitle slaves follow the main media clock. A container demuxer can
     * publish its own preroll/PCR even during opening or subtitle seeking;
     * those controls must not reset the main video picture or its clock. */
    if( sys->in->b_slave_sub )
    {
        switch( query )
        {
            case ES_OUT_SET_PCR:
            case ES_OUT_SET_GROUP_PCR:
            case ES_OUT_RESET_PCR:
            case ES_OUT_SET_NEXT_DISPLAY_TIME:
                return VLC_SUCCESS;
        }
    }
'''+needle)
p.write_text(s,encoding='utf-8',newline='\n')
mk=(work/'candidate-r1.mk').read_text(encoding='utf-8-sig')
mk=mk.replace('input/libvlccore_la-input.lo input/libvlccore_la-es_out.lo','input/libvlccore_la-input.lo input/libvlccore_la-es_out.lo input/libvlccore_la-es_out_source.lo').replace(')),2)', ')),3)').replace('exactly two','exactly three')
needle='$(EXTERNAL_CORE_OUT)/libvlccore.la:'
rule='''$(EXTERNAL_CORE_OUT)/es_out_source.lo: $(EXTERNAL_SOURCE)/es_out_source.c $(EXTERNAL_SOURCE)/es_out.h | $(EXTERNAL_CORE_OUT)
\t$(LIBTOOL) --tag=CC --mode=compile $(CC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(libvlccore_la_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS) -iquote $(srcdir)/input -I$(srcdir) -MD -MF $(EXTERNAL_CORE_OUT)/es_out_source.d -c -o "$@" "$<"
'''
assert mk.count(needle)==1;mk=mk.replace(needle,rule+needle)
mk=mk.replace('$(EXTERNAL_CORE_OUT)/input.lo $(EXTERNAL_CORE_OUT)/es_out.lo','$(EXTERNAL_CORE_OUT)/input.lo $(EXTERNAL_CORE_OUT)/es_out.lo $(EXTERNAL_CORE_OUT)/es_out_source.lo')
(work/'candidate-r3.mk').write_text(mk,encoding='utf-8',newline='\n')
build=(work/'build-candidate-r2b.sh').read_text(encoding='utf-8-sig').replace('candidate-r2','candidate-r3').replace('core-r2','core-r3').replace('candidate-r1.mk','candidate-r3.mk')
build=build.replace('"$out/es_out.d"','"$out/es_out.d" "$out/es_out_source.d"')
(work/'build-candidate-r3.sh').write_text(build,encoding='utf-8',newline='\n')
runner=(work/'run-native-r2.py').read_text(encoding='utf-8-sig').replace('inputs-r2.json','inputs-r3.json').replace("native-'+args.phase+'-r2","native-'+args.phase+'-r3").replace('candidate-r2-bin','candidate-r3-bin')
(work/'run-native-r3.py').write_text(runner,encoding='utf-8',newline='\n')
files=set(old);files.update(p.relative_to(repo).as_posix() for p in work.iterdir() if p.is_file() and p.suffix in ['.py','.sh','.mk'])
files.update(p.relative_to(repo).as_posix() for p in dest.iterdir() if p.is_file())
hashes={p:digest(repo/p) for p in sorted(files)};snapshot=work/'source-r3';assert not snapshot.exists();snapshot.mkdir()
for p,h in hashes.items():
    d=snapshot/p;d.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(repo/p,d);assert digest(d)==h
(work/'inputs-r3.json').write_text(json.dumps(dict(files=hashes,sourceCount=len(hashes),sourceSnapshot=str(snapshot),productionApplied=False,fullBetaAccepted=False),indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps(dict(sourceCount=len(hashes),productionApplied=False)))
