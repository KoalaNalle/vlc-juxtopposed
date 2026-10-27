from pathlib import Path
from hashlib import sha256
import json
work=Path(__file__).resolve().parent
source=work/'native-tracks-extra-r2.py';s=source.read_text(encoding='utf-8-sig')
needle="        result['externalAddReturn']=slave(player,0,externalPath.as_uri().encode(),args.external_selection!='preserve')"
assert s.count(needle)==1
s=s.replace(needle,"        invalidPath=fixtures/'intentionally-missing-subtitle.srt';assert not invalidPath.exists()\n        result['invalidAddReturn']=slave(player,0,invalidPath.as_uri().encode(),True)\n        time.sleep(.7);observe('invalid-external-added-paused')\n"+needle)
needle="addedExternal=rows['external-added-selected-paused']";assert s.count(needle)==1
s=s.replace(needle,needle+"\ninvalidExternal=rows['invalid-external-added-paused']\nchecks['invalidAdditionPreservesTracks']=invalidExternal['subtitleTracks']==beforeExternal['subtitleTracks']\nchecks['invalidAdditionPreservesCaption']=invalidExternal.get('captionMaskSha256')==beforeExternal.get('captionMaskSha256')\nchecks['invalidAdditionPreservesPicture']=invalidExternal.get('frameMarker')==beforeExternal.get('frameMarker')\nif args.audio=='none':checks['invalidAdditionPreservesTime']=invalidExternal['timeUs']==beforeExternal['timeUs']")
out=work/'native-tracks-extra-r3.py';assert not out.exists();out.write_text(s,encoding='utf-8',newline='\n')
(work/'extra-probe-r3.json').write_text(json.dumps(dict(sourceSha256=sha256(source.read_bytes()).hexdigest(),probeSha256=sha256(out.read_bytes()).hexdigest(),scope='ASS/MKS source registration plus missing-file request preserving the current selection/caption/picture/time. Probe is a private extension of the frozen maintained worker; not an ordinary acceptance claim.'),indent=2)+'\n',encoding='utf-8',newline='\n')
