from pathlib import Path
from hashlib import sha256
import json
work=Path(__file__).resolve().parent
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
raw=read(work/'ordinary-ui-r37/verification.json')
assert raw['normalExit'] and raw['exitCode']==0 and raw['sourceInputsUnchanged'] and raw['runtimeUnchanged'] and raw['mediaUnchanged']
assert raw['runtimeSha256']['plugins/demux/libmkv_plugin.dll']=='f46d8d158e61e99c4ee293337f7cef0b159c82586288e915a386a690d558280c'
for group in ['native-subtitle-ordinary-r36c','ordinary-seek-policy-r38','ordinary-subtitle-cancel-r38']:
    result=read(work/group/'comparison.json');assert result['passed'] and result['allProcessesNormal']
observations=[
    dict(step='startup-windowed',elapsed='00:00:00',progress=.0003553663940297513,paused=True,visual='Red video surface. No visible caption; no initial-subtitle acceptance.'),
    dict(step='accessibility-value-attempt',requestedProgress=.52,progressBefore=.0003553663940297513,progressAfter=.0003553663940297513,outcome='Setter returned without an error, but two observations showed no actual seek. Separate accessibility issue remains.'),
    dict(step='windowed-physical-seek',elapsed='00:00:31',progress=.5253623188405797,paused=True,visual='SECOND SHORT OVERLAY visible at lower edge. LONG line is covered by the playback bar in the 960x540 window.'),
    dict(step='fullscreen-overlap',elapsed='00:00:31',progress=.5254174176675385,paused=True,visual='Both SECOND SHORT OVERLAY and LONG OVERLAPPING CUE readable on red. Bottom gradient darkens the lower line.'),
    dict(step='fullscreen-after-short-ended',elapsed='00:00:36',progress=.6144834930777423,paused=True,visual='Only LONG OVERLAPPING CUE remains on red after the 30-32s cue ended.'),
    dict(step='fullscreen-backward-after-first-short-ended',elapsed='00:00:15',progress=.2502262664290117,paused=True,visual='LONG OVERLAPPING CUE remains on green after seeking backward past the 10-12s cue.'),
    dict(step='subtitle-menu-open',elapsed='00:00:15',progress=.2502262664290117,paused=True,subtitleRows=['Track 1 - [English]']),
    dict(step='subtitle-menu-escape',elapsed='00:00:15',progress=.2502262664290117,paused=True,menuDismissed=True),
    dict(step='resume',elapsed='00:00:20',progress=.34462670953340774,button='Pause'),
    dict(step='pause-again',elapsed='00:00:27',progress=.4646167480135263,button='Play'),
    dict(step='quit',exitCode=0,forcedCleanup=False),
]
result=dict(selectedPausedMatroskaSeekAccepted=True,nativeMatrixAccepted=True,ordinaryFullscreenSeekAccepted=True,
    completeDesktopSubtitleWorkflowAccepted=False,fullBetaAccepted=False,runtimeSha256=raw['runtimeSha256'],
    rawVerificationSha256=digest(work/'ordinary-ui-r37/verification.json'),pid=raw['pid'],windowId=7604168,
    method='Live Computer Use screenshots and accessibility observations from the owned ordinary GUI. Visual interpretations are recorded explicitly, not independent desktop pixel-mask tests. Immediate accessibility states sometimes lagged the picture; settled observations were refreshed before conclusions.',
    observations=observations,remaining=['Small-window playback controls cover the bottom caption line; fullscreen bottom gradient also darkens it.','Direct accessibility progress value-setting did not commit a seek.','Initial subtitles, paused track replacement, external subtitles, language/default policy and synchronization remain separate.'],
    questionsForMorning=[],scope='Selected source repair accepted for precise Matroska subtitle seeking. Full desktop workflow and 13-category/32-surface beta remain open. No installs, commit/push, release/upload or shutdown.')
(work/'desktop-subtitle-observations-r39.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps({k:result[k] for k in ['selectedPausedMatroskaSeekAccepted','completeDesktopSubtitleWorkflowAccepted','fullBetaAccepted']}))
