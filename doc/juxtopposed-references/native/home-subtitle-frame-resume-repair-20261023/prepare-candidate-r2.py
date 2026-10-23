from pathlib import Path
from hashlib import sha256
import json,shutil,subprocess
work=Path(__file__).resolve().parent;repo=work.parent.parent
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
before=read(work/'inputs-r1.json')['files'];assert all(digest(repo/p)==h for p,h in before.items())
root=work/'candidate-r2';assert not root.exists();root.mkdir()
decoder=repo/'build-windows/beta-subtitle-frame-feedback-20261023/candidate-r2/decoder.c'
assert digest(decoder)=='2d9429992e23abc653a974c9a1eb559535320fd015df8654c60a948f4bd64940'
shutil.copyfile(decoder,root/'decoder.c')
original=repo/'src/input/es_out.c';assert digest(original)=='00fc7a6c326809655462427e0980893afb1502206f6c49f5f5819191bc083ae0'
source=original.read_text(encoding='utf-8-sig')
old='    /* Flush every ES except the video one */'
assert source.count(old)==1;source=source.replace(old,'    /* Retain the held video and queued subtitles when frame stepping ends. */')
old='''            if( p_es != p_next_frame_es )
                vlc_input_decoder_Flush( p_es->p_dec );'''
new='''            /* Only frame-step resume supplies a retained video. Its subtitle
             * cues were already demuxed; an actual seek supplies NULL and must
             * still flush them. Audio keeps the existing resynchronization. */
            if( p_es != p_next_frame_es
             && !(p_next_frame_es && p_es->fmt.i_cat == SPU_ES) )
                vlc_input_decoder_Flush( p_es->p_dec );'''
assert source.count(old)==1;source=source.replace(old,new)
old='''    /* Drop all ESes except the video one in case of next-frame */
    if( p_sys->p_next_frame_es != NULL && p_sys->p_next_frame_es != es )'''
new='''    /* Keep subtitle packets alongside the stepped video, so current and
     * upcoming cues remain available. Other outputs keep the existing policy. */
    if( p_sys->p_next_frame_es != NULL && p_sys->p_next_frame_es != es
     && es->fmt.i_cat != SPU_ES )'''
assert source.count(old)==1;source=source.replace(old,new)
(root/'es_out.c').write_text(source,encoding='utf-8',newline='\n')
diff=subprocess.run(['git','diff','--no-index','--',str(original),str(root/'es_out.c')],cwd=repo,capture_output=True)
assert diff.returncode==1;(work/'es-out-r2.patch').write_bytes(diff.stdout)
parent=(repo/'build-windows/beta-subtitle-frame-feedback-20261023/run-private-r2.py').read_text(encoding='utf-8')
parent=parent.replace("manifest=work/('inputs-candidate-r2.json')", "manifest=work/'inputs-candidate-r2.json'")
parent=parent.replace("abi=work/'layout-r1.json'", "abi=repo/'build-windows/beta-subtitle-frame-feedback-20261023/layout-r1.json'")
first=parent.index("assert (work/'build-layout-r1-exit.txt')");last=parent.index('media=repo/',first)
parent=parent[:first]+'''abi=repo/'build-windows/beta-subtitle-frame-feedback-20261023/layout-r1.json'
provenance=read(abi.parent/'layout-provenance-r1.json')
assert provenance['headerSha256']==digest(repo/'include/vlc/libvlc_media_player.h') and provenance['sourceSha256']==digest(repo/'test/libvlc/native_player_callback_layout.c')
'''+parent[last:]
target=work/'run-candidate-r2.py';assert not target.exists();target.write_text(parent,encoding='utf-8',newline='\n')
files=set(before)|{(root/p).relative_to(repo).as_posix() for p in ['decoder.c','es_out.c']}
files.update(p.relative_to(repo).as_posix() for p in work.iterdir() if p.is_file() and p.suffix in ['.py','.sh','.mk'])
hashes={p:digest(repo/p) for p in sorted(files)};snapshot=work/'source-candidate-r2';assert not snapshot.exists();snapshot.mkdir()
for p,h in hashes.items():
    d=snapshot/p;d.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(repo/p,d);assert digest(d)==h
(work/'inputs-candidate-r2.json').write_text(json.dumps(dict(files=hashes,sourceCount=len(hashes),sourceSnapshot=str(snapshot),productionApplied=False,fullBetaAccepted=False),indent=2)+'\n',encoding='utf-8',newline='\n')
(work/'candidate-r2.json').write_text(json.dumps(dict(productionApplied=False,files={p:digest(root/p) for p in ['decoder.c','es_out.c']},sourceInputCount=len(hashes),scope='Preserve subtitle packets and queued cues across frame-step/resume, alongside the private silent-flush feedback repair. Actual seek and audio resynchronization routes retain their old policy.',fullBetaAccepted=False),indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps(read(work/'candidate-r2.json')))
