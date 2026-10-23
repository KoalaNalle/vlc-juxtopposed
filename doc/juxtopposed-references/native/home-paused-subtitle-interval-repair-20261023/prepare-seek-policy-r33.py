from pathlib import Path
import ast
work=Path(__file__).resolve().parent
text=(work/'native-mkv-interval-r25.py').read_text(encoding='utf-8')
point="options+=['--aout=mmdevice','--mmdevice-backend=wasapi'] if audio else ['--no-audio']"
assert text.count(point)==1
text=text.replace(point,point+"\nif cfg.get('subtitleDisabled'):options+=['--no-spu']")
text=text.replace("result['requestedSubtitleLanguageSelected']=any(t['language']==cfg['language'] and t['selected'] for t in tracks())",
    "result['requestedSubtitleLanguageSelected']=any(t['language']==cfg['language'] and t['selected'] for t in tracks())\n    result['subtitleSelectionMatchesScenario']=(not any(t['selected'] for t in tracks())) if cfg.get('subtitleDisabled') else result['requestedSubtitleLanguageSelected']")
text=text.replace("assert result['initialPauseReached'] and result['requestedSubtitleLanguageSelected']", "assert result['initialPauseReached'] and result['subtitleSelectionMatchesScenario']")
text=text.replace("int(step['target']*1_000_000),False", "int(step['target']*1_000_000),cfg.get('fast',False)")
ast.parse(text)
target=work/'native-seek-policy-r33.py';assert not target.exists()
target.write_text(text,encoding='utf-8',newline='\n')
parent=(work/'run-native-mkv-headers-r30.py').read_text(encoding='utf-8')
start=parent.index('configs=[');end=parent.index('source=read(',start)
configs="""configs=[
    dict(name='precise-subtitle-disabled',media='media-r26/cbr-overlap.mkv',language='eng',audio=True,subtitleDisabled=True,steps=[step('far',55,2,False),step('back',5,0,False),step('middle',31,0,False)]),
    dict(name='fast-subtitle-disabled',media='media-r26/cbr-overlap.mkv',language='eng',audio=True,subtitleDisabled=True,fast=True,steps=[step('far',55,2,False),step('back',5,0,False),step('middle',31,0,False)]),
    dict(name='fast-selected-subtitle-existing',media='media-r26/cbr-overlap.mkv',language='eng',audio=True,fast=True,steps=[step('far',55,2,False)],scope='Preserve existing fast-seek output and bounded reads. Missing old long cue is recorded, not accepted as a complete subtitle workflow.'),
]
"""
parent=parent[:start]+configs+parent[end:]
parent=parent.replace("worker=work/'native-mkv-interval-r25.py'", "worker=work/'native-seek-policy-r33.py'")
parent=parent.replace("root=work/'native-mkv-headers-r30'", "root=work/'native-seek-policy-r33'")
parent=parent.replace("control='Full-payload interval scan versus header-only interval scan; both are positive caption controls.'", "control='Precise seeking with subtitles disabled, fast seeking disabled/selected subtitle behavior. The selected fast-seek long cue remains absent in both phases; that preserves policy, not full caption acceptance.'")
ast.parse(parent)
target=work/'run-native-seek-policy-r33.py';assert not target.exists()
target.write_text(parent,encoding='utf-8',newline='\n')
