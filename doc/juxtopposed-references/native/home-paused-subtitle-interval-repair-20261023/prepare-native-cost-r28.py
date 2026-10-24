from pathlib import Path
import ast
work=Path(__file__).resolve().parent
parent=(work/'run-native-mkv-interval-r25.py').read_text(encoding='utf-8')
start=parent.index('configs=[');end=parent.index("source=read(",start)
configs='''for name in ['cbr-overlap','cbr-gaps']:
    probe=read(work/'media-r26'/(name+'-probe.json'));assert int(probe['format']['size'])>25_000_000 and 59<float(probe['format']['duration'])<61
probe=read(work/'media-r26/ass-overlap-probe.json');assert next(s for s in probe['streams'] if s['codec_type']=='subtitle')['codec_name']=='ass'
reference=read(work/'native-ass-overlap-reference-r27/before/normal-playback-reference/result.json');assert reference['passed'] and reference['teardownComplete']
long_ref='native-overlap-mask-reference-r24/before/normal-playback-reference/normal-long-alone-35s.png'
both_ref='native-overlap-mask-reference-r24/before/normal-playback-reference/normal-overlap-31s.png'
ass_long='native-ass-overlap-reference-r27/before/normal-playback-reference/normal-long-alone-35s.png'
ass_both='native-ass-overlap-reference-r27/before/normal-playback-reference/normal-overlap-31s.png'
configs=[
    dict(name='cbr-empty-gaps',media='media-r26/cbr-gaps.mkv',language='eng',audio=True,steps=[step('first-wide-gap',55,2,False),step('back-gap',5,0,False),step('middle-gap',25,2,False),step('last-gap',45,1,False),step('repeated-middle-gap',25,2,False),step('repeated-back-gap',5,0,False),step('active-middle',21,2,True)]),
    dict(name='cbr-overlapping',media='media-r26/cbr-overlap.mkv',language='eng',audio=True,steps=[dict(**step('first-wide-long',55,2,True),referencePicture=long_ref),dict(**step('back-long',5,0,True),referencePicture=long_ref),dict(**step('both',31,0,True),referencePicture=both_ref),dict(**step('long-alone',35,0,True),referencePicture=long_ref),dict(**step('repeated-back-long',5,0,True),referencePicture=long_ref)]),
    dict(name='ass-overlapping',media='media-r26/ass-overlap.mkv',language='eng',audio=True,steps=[dict(**step('cold-both',31,0,True),referencePicture=ass_both),dict(**step('long-alone',35,0,True),referencePicture=ass_long),dict(**step('first-long',5,0,True),referencePicture=ass_long),dict(**step('first-overlap',11,1,True)),dict(**step('long-after-first',15,1,True),referencePicture=ass_long),dict(**step('both-again',31,0,True),referencePicture=ass_both)]),
]
'''
parent=parent[:start]+configs+parent[end:]
parent=parent.replace("root=work/'native-mkv-interval-r25'", "root=work/'native-mkv-cost-r28'")
parent=parent.replace("fixtureProducerSha256=digest(work/'generate-subtitle-seek-edges-r20.sh')", "fixtureProducerSha256=digest(work/'generate-subtitle-cost-r26.sh')")
parent=parent.replace("for p in ['plugins/demux/libmkv_plugin.dll','plugins/codec/libsubsdec_plugin.dll','plugins/text_renderer/libfreetype_plugin.dll']", "for p in ['plugins/demux/libmkv_plugin.dll','plugins/codec/libsubsdec_plugin.dll','plugins/codec/liblibass_plugin.dll','plugins/text_renderer/libfreetype_plugin.dll']")
ast.parse(parent);target=work/'run-native-mkv-cost-r28.py';assert not target.exists();target.write_text(parent,encoding='utf-8',newline='\n')
