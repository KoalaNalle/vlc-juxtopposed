from pathlib import Path
import ast
work=Path(__file__).resolve().parent
text=(work/'run-native-mkv-edges-r20.py').read_text(encoding='utf-8')
start=text.index('configs=[');end=text.index("source=read(",start)
replacement='''configs=[dict(name='overlapping-cues',media='media-r21/colored-overlap.mkv',language='eng',audio=True,steps=[step('overlap-first',11,1,True),step('long-alone',15,1,True),step('overlap-second',31,0,True),step('long-alone-again',35,0,True),step('long-at-first',5,0,True)])]
packets=read(work/'media-r21/subtitle-packets.json')['packets'];assert len(packets)==3
assert [(float(p['pts_time']),float(p['duration_time'])) for p in packets]==[(0,59.9),(10,2),(30,2)]
'''
text=text[:start]+replacement+text[end:]
text=text.replace("root=work/'native-mkv-edges-r20'", "root=work/'native-mkv-overlap-r21'")
text=text.replace("fixtureProducerSha256=digest(work/'generate-subtitle-seek-edges-r20.sh')", "fixtureProducerSha256=digest(work/'generate-subtitle-overlap-r21.sh')")
ast.parse(text)
target=work/'run-native-mkv-overlap-r21.py';assert not target.exists();target.write_text(text,encoding='utf-8',newline='\n')
