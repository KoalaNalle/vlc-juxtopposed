from pathlib import Path
from hashlib import sha256
import json
work=Path(__file__).resolve().parent
source=(work/'run-native-r1.py').read_text(encoding='utf-8')
first=source.index("if args.phase=='before':\n")
last=source.index("media=repo/",first)
source=source[:first]+'''provenance=read(work/'layout-provenance-r1.json')
assert provenance['headerSha256']==digest(repo/'include/vlc/libvlc_media_player.h') and provenance['sourceSha256']==digest(repo/'test/libvlc/native_player_callback_layout.c')
'''+source[last:]
source=source.replace("root=work/('native-'+args.phase+'-r1');assert not root.exists();root.mkdir();records=[]", "cases=cases[1:]\nroot=work/'native-controls-r1b';assert not root.exists();root.mkdir();records=[]")
source=source.replace("        if not all(v for n,v in result['checks'].items() if n!='noUnrequestedFrameFeedback'):raise SystemExit('Inspect other failed behavior before proceeding')", "        # Observational unchanged controls retain every failed contract.\n        # In particular, post-step/resume caption loss must not be hidden.")
source=source.replace("write(root/'processes.json',dict(records=records,allProcessesNormal=all(r['normalExit'] for r in records),expectedBehaviorObserved=True,fullBetaAccepted=False))", "write(root/'processes.json',dict(records=records,allProcessesNormal=all(r['normalExit'] for r in records),completed=True,passed=False,scope='Unchanged controls for unsolicited feedback and newly exposed caption loss after real steps/resume; no repair acceptance.',fullBetaAccepted=False))")
target=work/'run-controls-r1b.py';assert not target.exists();target.write_text(source,encoding='utf-8',newline='\n')
(work/'controls-plan-r1b.json').write_text(json.dumps(dict(unchangedCore=True,productionApplied=False,initialNormalChild='native-before-r1/frame-marker.mkv-wasapi-external.srt/process.json',initialParentExit=1,reason='The first unchanged child confirms unsolicited events and correct one-frame steps, but externalDistinct also fails after steps/resume. Preserve it; characterize the other six controls without weakening or removing any worker check.',workerChanged=False,parentSha256=sha256(target.read_bytes()).hexdigest(),remainingCases=6,fullBetaAccepted=False),indent=2)+'\n',encoding='utf-8',newline='\n')
print('Prepared six remaining unchanged controls; existing first result retained.')
