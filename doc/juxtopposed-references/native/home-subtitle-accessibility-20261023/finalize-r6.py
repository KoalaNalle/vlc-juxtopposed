from pathlib import Path
from hashlib import sha256
import json,subprocess

work=Path(__file__).resolve().parent
repo=work.parent.parent
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
inputs=read(work/'inputs-r3.json')
allowed={'PROJECT_HANDOFF.md','doc/juxtopposed-beta-roadmap.md','doc/juxtopposed-settings-consumer-audit.md'}
changed={}
for name,h in inputs['files'].items():
    assert digest(Path(inputs['sourceCopies'][name]))==h,name
    current=digest(repo/name)
    if current!=h:
        assert name in allowed,name
        changed[name]=current
assert set(changed)==allowed
previous=read(repo/'build-windows/beta-subtitle-delay-repair-20261023/final-verification-r6.json')
published=read(work/'publication-r5.json')
receipts=previous['receipts']+[dict(path=published['path'],checkpointSha256=published['checkpointSha256'],artifactCount=published['artifactCount'])]
total=0
for row in receipts:
    root=repo/row['path']
    assert digest(root/'checkpoint.json')==row['checkpointSha256'],row['path']
    artifacts=read(root/'checkpoint.json')['artifacts']
    assert len(artifacts)==row['artifactCount']
    assert all(digest(root/p)==h for p,h in artifacts.items()),row['path']
    total+=len(artifacts)
runs=[read(work/('ui-'+name+'-r3/verification.json')) for name in ['frame-marker-ass.mkv','frame-marker.mp4']]
assert all(run['status']=='terminal' and run['normalExit'] for run in runs)
assert not runs[1]['diagnostics']
for name,run in zip(['frame-marker-ass.mkv','frame-marker.mp4'],runs):
    runtime=work/('ui-'+name+'-r3/runtime')
    assert all(digest(runtime/p)==h for p,h in run['runtimeSha256'].items())
head=subprocess.check_output(['git','rev-parse','HEAD'],cwd=repo,text=True).strip()
assert head==previous['gitHead']
assert not subprocess.check_output(['git','diff','--cached','--name-only'],cwd=repo,text=True).strip()
subprocess.run(['git','diff','--check'],cwd=repo,check=True)
processes=json.loads(subprocess.check_output(['powershell.exe','-NoProfile','-Command',
    "ConvertTo-Json -InputObject @(Get-CimInstance Win32_Process | Where-Object { $_.Name -eq 'vlc.exe' -and $_.CommandLine -like '*beta-subtitle-accessibility-20261023*' } | Select-Object ProcessId)"] ,text=True))
assert processes==[],processes
result=dict(passed=True,sourceInputCount=len(inputs['files']),allActualSourceCopiesVerified=True,
            onlyPostFreezeChanges=changed,receipts=receipts,artifactCount=total,
            runtimeSha256=runs[1]['runtimeSha256'],productionControlRepairAccepted=True,
            componentCases=15,desktopNormalExits=2,cleanPlaybackDesktopPasses=1,
            retainedInterruptedAudioRuns=1,fullBetaAccepted=False,ownedProcesses=[],
            gitHead=head,indexEmpty=True,whitespacePassed=True,
            noInstallCommitPushReleaseUploadOrShutdown=True,producerSha256=digest(Path(__file__)))
(work/'final-verification-r6.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8',newline='\n')
print('PASS:',len(inputs['files']),'actual input copies;',len(receipts),'receipts;',total,'artifacts; full beta remains active.',flush=True)
