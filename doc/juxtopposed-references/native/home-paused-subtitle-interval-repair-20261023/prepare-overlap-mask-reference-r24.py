from pathlib import Path
import ast
work=Path(__file__).resolve().parent
text=(work/'native-overlap-reference-r22.py').read_text(encoding='utf-8')
start=text.index('    pause(player,0)\n',text.index("observe('initial-paused')"));end=text.index('\nfinally:',start)
replacement='''    pause(player,0)
    result['overlap31Reached']=wait(lambda:state(player)==2 and getTime(player)>31_200_000,42)
    pause(player,1);result['repaused']=wait(lambda:state(player)==3,4);time.sleep(.3)
    observe('normal-overlap-31s',dict(name='both-cues',target=31,color=0,subtitle=True))
    pause(player,0)
    result['resumeAdvancedFromRequestedTarget']=wait(lambda:state(player)==2 and getTime(player)>35_200_000,10) and result['overlap31Reached']
    pause(player,1);result['repaused']=wait(lambda:state(player)==3,4);time.sleep(.3)
    observe('normal-long-alone-35s',dict(name='long-alone',target=35,color=0,subtitle=True))
'''
text=text[:start]+replacement+text[end:];ast.parse(text)
target=work/'native-overlap-mask-reference-r24.py';assert not target.exists();target.write_text(text,encoding='utf-8',newline='\n')
parent=(work/'run-native-overlap-reference-r22.py').read_text(encoding='utf-8')
parent=parent.replace("worker=work/'native-overlap-reference-r22.py'","worker=work/'native-overlap-mask-reference-r24.py'")
parent=parent.replace("root=work/'native-overlap-reference-r22'","root=work/'native-overlap-mask-reference-r24'")
ast.parse(parent);target=work/'run-native-overlap-mask-reference-r24.py';assert not target.exists();target.write_text(parent,encoding='utf-8',newline='\n')
