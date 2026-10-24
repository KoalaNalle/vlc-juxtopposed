from pathlib import Path
from hashlib import sha256
import ast,json
work=Path(__file__).resolve().parent;repo=work.parent.parent
prior=repo/'build-windows/beta-external-subtitle-selection-20261023'
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
last=read(prior/'final-verification-r9.json');assert last['passed']
old=read(prior/'inputs-r3.json')['files']
assert {p for p,h in old.items() if digest(repo/p)!=h}==set(last['onlyPostFreezeChanges'])|{'test/libvlc/generate_paused_subtitle_tracks.sh'}
review=read(repo/'doc/juxtopposed-references/native/home-external-subtitle-selection-investigation-20261023/review.json')
assert all(digest(repo/p)==h for p,h in review['productionSources'].items())
source=prior/'native-tracks-extra-r3.py';assert digest(source)==read(prior/'extra-probe-r3.json')['probeSha256']
s=source.read_text(encoding='utf-8-sig')
needle="parser.add_argument('--external-track-count',type=int,default=1)";assert s.count(needle)==1
s=s.replace(needle,needle+"\nparser.add_argument('--check-missing-external',action='store_true')")
needle="        invalidPath=fixtures/'intentionally-missing-subtitle.srt';assert not invalidPath.exists()\n        result['invalidAddReturn']=slave(player,0,invalidPath.as_uri().encode(),True)\n        time.sleep(.7);observe('invalid-external-added-paused')"
assert s.count(needle)==1;s=s.replace(needle,"        if args.check_missing_external:\n"+'\n'.join('    '+line for line in needle.splitlines()))
needle="invalidExternal=rows['invalid-external-added-paused']\nchecks['invalidAdditionPreservesTracks']=invalidExternal['subtitleTracks']==beforeExternal['subtitleTracks']\nchecks['invalidAdditionPreservesCaption']=invalidExternal.get('captionMaskSha256')==beforeExternal.get('captionMaskSha256')\nchecks['invalidAdditionPreservesPicture']=invalidExternal.get('frameMarker')==beforeExternal.get('frameMarker')\nif args.audio=='none':checks['invalidAdditionPreservesTime']=invalidExternal['timeUs']==beforeExternal['timeUs']"
assert s.count(needle)==1;s=s.replace(needle,"if args.check_missing_external:\n"+'\n'.join('    '+line for line in needle.splitlines()))
s=s.replace("externalSelection=args.external_selection,externalPolicy=args.external_policy,", "externalSelection=args.external_selection,externalPolicy=args.external_policy,externalFile=args.external_file,expectedExternalTrackCount=args.external_track_count,checkedMissingExternal=args.check_missing_external,")
s=s.replace('Native paused selected-subtitle replacement, caption masks, actual frame-marker conservation, sampled pause and resume. Cold captions, automatic external selection, physical audibility, desktop and full beta require separate checks.', 'Native paused explicit/automatic/load-only subtitle requests, optional missing-file requests, caption masks, actual frame-marker conservation, sampled pause and resume. Cold captions, physical audibility, desktop and full beta require separate checks.')
ast.parse(s)
target=repo/'test/libvlc/native_paused_subtitle_tracks.py';target.write_text(s,encoding='utf-8',newline='\n')
(work/'maintained-promotion-r1.json').write_text(json.dumps(dict(sourceProbeSha256=digest(source),maintainedProducerSha256=digest(target),productionApplied=False,ordinaryBuildAccepted=False,fullBetaAccepted=False),indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps(read(work/'maintained-promotion-r1.json')))
