from pathlib import Path
import ast
work=Path(__file__).resolve().parent
parent=(work/'run-native-subtitle-ordinary-r36.py').read_text(encoding='utf-8')
parent=parent.replace("inputs-r35.json","inputs-r35b.json").replace("source-r35'","source-r35b'")
parent=parent.replace("source['sourceCount']==2205", "source['sourceCount']==2208")
parent=parent.replace('build-ordinary-mkv-r35-exit', 'build-ordinary-mkv-r35b-exit')
parent=parent.replace('ordinary-mkv-r35-bin', 'ordinary-mkv-r35b-bin')
parent=parent.replace("root=work/'native-subtitle-ordinary-r36'", "root=work/'native-subtitle-ordinary-r36b'")
for name in ['long','delayed','gaps']:
    parent=parent.replace('media-r20/colored-'+name+'.mkv','media-generator-smoke-r35/colored-'+name+'.mkv')
parent=parent.replace('media-r21/colored-overlap.mkv','media-generator-smoke-r35/colored-overlap.mkv')
parent=parent.replace('media-r26/ass-overlap.mkv','media-generator-smoke-r35/ass-overlap.mkv')
parent=parent.replace("fixtureProducerSha256=digest(work/'generate-subtitle-cost-r26.sh')", "fixtureProducerSha256=digest(repo/'test/libvlc/generate_paused_subtitle_seek.sh'),largerFixtureProducerSha256=digest(work/'generate-subtitle-cost-r26.sh')")
ast.parse(parent)
target=work/'run-native-subtitle-ordinary-r36b.py';assert not target.exists()
target.write_text(parent,encoding='utf-8',newline='\n')
