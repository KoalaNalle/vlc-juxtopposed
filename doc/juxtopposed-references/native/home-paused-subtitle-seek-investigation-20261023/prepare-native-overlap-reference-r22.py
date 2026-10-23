from pathlib import Path
import ast
work=Path(__file__).resolve().parent
text=(work/'native-mkv-edges-r20.py').read_text(encoding='utf-8')
start=text.index('    for index,step in enumerate(');end=text.index('\nfinally:',start)
text=text[:start]+'''    pause(player,0)
    result['resumeAdvancedFromRequestedTarget']=wait(lambda:state(player)==2 and getTime(player)>15_200_000,25)
    pause(player,1);result['repaused']=wait(lambda:state(player)==3,4);time.sleep(.3)
    observe('normal-playback-after-short-cue-ended',dict(name='long-alone',target=15,color=1,subtitle=True))
'''+text[end:]
ast.parse(text)
target=work/'native-overlap-reference-r22.py';assert not target.exists();target.write_text(text,encoding='utf-8',newline='\n')
parent=(work/'run-native-mkv-overlap-r21.py').read_text(encoding='utf-8')
start=parent.index('configs=[');end=parent.index("source=read(",start)
parent=parent[:start]+"configs=[dict(name='normal-playback-reference',media='media-r21/colored-overlap.mkv',language='eng',audio=True,steps=[])]\n"+parent[end:]
parent=parent.replace("worker=work/'native-mkv-edges-r20.py'", "worker=work/'native-overlap-reference-r22.py'")
parent=parent.replace("root=work/'native-mkv-overlap-r21'", "root=work/'native-overlap-reference-r22'")
parent=parent.replace("for phase in ['before','after']:", "for phase in ['before']:")
ast.parse(parent)
target=work/'run-native-overlap-reference-r22.py';assert not target.exists();target.write_text(parent,encoding='utf-8',newline='\n')
