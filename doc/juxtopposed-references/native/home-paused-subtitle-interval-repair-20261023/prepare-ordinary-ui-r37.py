from pathlib import Path
import ast
work=Path(__file__).resolve().parent
text=(work/'run-ordinary-r14.py').read_text(encoding='utf-8')
text=text.replace("manifest=work/'inputs-r14.json';inputs=read(manifest)", "manifest=work/'inputs-r35c.json';inputs=read(manifest)['files']")
text=text.replace('len(inputs)>=2120','len(inputs)==2210')
text=text.replace("out=work/'ordinary-ui-r14'", "out=work/'ordinary-ui-r37'")
text=text.replace("base=repo/'build-windows/beta-queue-workflow-20261022/candidate-runtime-r23'", "base=work/'ordinary-ui-r14/runtime'")
text=text.replace("'plugins/audio_output/libwasapi_plugin.dll']", "'plugins/audio_output/libwasapi_plugin.dll','plugins/demux/libmkv_plugin.dll']")
text=text.replace('4422fbb02fb22857c257340d486a0310c923526d6d276127258566d3ac2671de','000f1159b7d190d83edaba83739b97b869669ca2eec2ba97bc35d6c51f9bb9cb')
start=text.index("qt=runtime/'plugins/gui/libqt_plugin.dll'")
end=text.index('identities=',start)
text=text[:start]+"mkv=runtime/'plugins/demux/libmkv_plugin.dll';mkv.unlink();shutil.copyfile(work/'ordinary-mkv-r35c-bin/libmkv_plugin.dll',mkv)\n"+text[end:]
text=text.replace("str(media/'multi-language.mkv'),str(media/'plain.mkv')", "str(work/'media-generator-smoke-r35/colored-overlap.mkv'),str(media/'multi-language.mkv')")
text=text.replace("sourceManifest='build-windows/beta-tracks-subtitles-20261023/inputs-r14.json'", "sourceManifest='build-windows/beta-tracks-subtitles-20261023/inputs-r35c.json'")
text=text.replace("str(media.as_uri())", "str(media.as_uri())")
point="mediaHashes={p.name:digest(p) for p in media.iterdir() if p.is_file()}"
assert text.count(point)==1
text=text.replace(point,point+"\nextraMedia=work/'media-generator-smoke-r35/colored-overlap.mkv';extraMediaHash=digest(extraMedia)")
text=text.replace("mediaSha256=mediaHashes,", "mediaSha256=mediaHashes,overlapMediaSha256=extraMediaHash,")
text=text.replace("mediaUnchanged=all(digest(media/p)==h for p,h in mediaHashes.items())", "mediaUnchanged=all(digest(media/p)==h for p,h in mediaHashes.items()) and digest(extraMedia)==extraMediaHash")
text=text.replace("fixtureProducerSha256=digest(work/'generate-fixtures-r1.sh')", "fixtureProducerSha256=digest(repo/'test/libvlc/generate_paused_subtitle_seek.sh')")
ast.parse(text)
target=work/'run-ordinary-ui-r37.py';assert not target.exists()
target.write_text(text,encoding='utf-8',newline='\n')
