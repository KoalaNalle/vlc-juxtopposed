from pathlib import Path
from hashlib import sha256
import ast,json
work=Path(__file__).resolve().parent
source=work/'publish-investigation-r5.py'
failure=dict(stage='Publication preflight before receipt directory creation',scriptSha256=sha256(source.read_bytes()).hexdigest(),reason='The frozen-target assertion failed for original-noaudio: refresh targets were [1021334,1061334,5021334,5061334], versus initial paused 1021334 and explicit requested 5000000. This is a candidate behavior defect, not a check to waive.',productionApplied=False,candidateAccepted=False)
(work/'publication-r5-failure.json').write_text(json.dumps(failure,indent=2)+'\n',encoding='utf-8',newline='\n')
text=source.read_text(encoding='utf-8')
old="    assert len(refresh)==4 and refresh[:2]==[a['played-paused']['timeUs']]*2 and refresh[2:]==[5_000_000]*2,(case,refresh,a['played-paused']['timeUs'])"
new='''    assert len(refresh)==4,(case,refresh)
    def last_point(row):
        points=[v['point']['ts'] for v in row['timerEvents'] if v['event']=='update']
        return points[-1] if points else None
    outputPoints=[last_point(a[n]) for n in ['played-paused','normal-pause-finnish','normal-pause-english','seek-first-paused','finnish-selected-paused','external-explicit-selected-paused']]
    if case.endswith('noaudio'):
        assert outputPoints[1]-outputPoints[0]==40_000 and outputPoints[2]-outputPoints[1]==40_000
        assert outputPoints[4]-outputPoints[3]==40_000 and outputPoints[5]-outputPoints[4]==40_000
        assert last_point(b['seek-first-paused'])==last_point(b['finnish-selected-paused'])==last_point(b['external-explicit-selected-paused'])'''
assert text.count(old)==1;text=text.replace(old,new)
text=text.replace("refreshTargetsUs=refresh,coldInitialBrightPixels", "refreshTargetsUs=refresh,reportedOutputPointTimesUs=outputPoints,reportedPointDriftPerRefreshUs=40_000 if case.endswith('noaudio') else None,frameConservationNotAccepted=True,coldInitialBrightPixels")
text=text.replace("candidateReplacementObservations=20,fullBetaAccepted=False", "candidateReplacementObservations=20,candidateRejectedForApplication=True,frameTimeDriftObserved=True,fullBetaAccepted=False")
text=text.replace("candidateObservationsSupported=True,selectedRepairAccepted=False", "candidateCaptionObservationsSupported=True,candidateRejectedForApplication=True,frameTimeDriftObserved=True,selectedRepairAccepted=False")
text=text.replace("    write(target/'review.json',review)", "    write(target/'review.json',review)")
needle="write(target/'review.json',review)"
replacement='''review['candidateRejectedForApplication']=True
review['frameTimeDrift']='Both no-audio cases advance the paused video-source timer point by 40,000us per selection refresh; the matching unchanged controls hold the point. WASAPI best-source points remain stale, so they cannot qualify exact video conservation.'
review['next']='Trace the current displayed-video/SMPTE source and seek target normalization, or use subtitle-only refill, before another candidate. Require repeated selection to conserve the paused picture/time, then maintained native and ordinary/desktop validation.'
shutil.copyfile(work/'publication-r5-failure.json',target/'publication-r5-failure.json')
write(target/'review.json',review)'''
assert text.count(needle)==1;text=text.replace(needle,replacement)
old='''The targets are the ordinary pause time twice, then the explicit five-second
seek twice. Generated colored video retains the expected coarse target colors.
Reviewed native images show the Finnish and external first-cue text correctly.'''
new='''Generated colored video retains the expected coarse color regions, and native
images show the Finnish/external text correctly. However, both no-audio cases
advance the paused video-source timer point by40ms on each selection refresh.
At25fps this is one frame. The matching unchanged controls retain the point.
This violates frame conservation, so the candidate is rejected for application.
The publication preflight caught this defect before writing the receipt; its
failed assertion is preserved rather than waived as an acceptance check.'''
assert old in text;text=text.replace(old,new)
text=text.replace("The diagnostic candidate Core5837a5b7 remains separate.", "The diagnostic candidate Core5837a5b7 remains separate and must not be promoted.")
text=text.replace("'publication-investigation-r5.json'", "'publication-investigation-r5b.json'")
ast.parse(text)
target=work/'publish-investigation-r5b.py';assert not target.exists();target.write_text(text,encoding='utf-8',newline='\n')
print('Prepared failure-preserving investigation receipt; candidate remains rejected')
