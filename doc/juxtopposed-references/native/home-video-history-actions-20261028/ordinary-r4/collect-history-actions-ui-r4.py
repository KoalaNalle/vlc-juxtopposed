"""Archive terminal reverse-direction UI observations; never launch a process."""
from pathlib import Path
from hashlib import sha256
import json
import shutil

repo=Path(__file__).resolve().parents[2]
work=Path(__file__).resolve().parent
source=work/'history-actions-ui-r4'
target=repo/'doc/juxtopposed-references/native/home-video-history-actions-20261028/ordinary-r4'
def digest(path): return sha256(Path(path).read_bytes()).hexdigest()
record=json.loads((source/'verification.json').read_text())
guards={key:value for key,value in record.items() if key.endswith('Unchanged')}
assert record['status']=='terminal' and record['exitCode']==0
assert record['moduleCounts']==['603'] and not record['diagnostics']
assert len(guards)==7 and all(guards.values())
producer=work/'run-history-actions-ui-r4.py'
assert digest(producer)==record['producerSha256']
assert record['producerSha256']=='aea65ecfccf39eacf84e8fa4ed45868fd7636f3be02a84b9f0b6d0221837f272'
if target.exists():
    assert [path.name for path in target.iterdir()]==['NOTES.md'], 'Only the newly drafted notes may exist'
else:
    target.mkdir()
copies=[]
def copy(path,relative):
    output=target/relative
    output.parent.mkdir(parents=True,exist_ok=True)
    shutil.copyfile(path,output)
    assert Path(path).read_bytes()==output.read_bytes()
    copies.append(dict(source=Path(path).relative_to(repo).as_posix(),destination=relative,
        size=output.stat().st_size,sha256=digest(output),byteIdentical=True))
observations=[]; images=[]
for path in sorted((source/'observations').iterdir()):
    assert path.suffix in ('.json','.png')
    copy(path,'observations/'+path.name)
    if path.suffix=='.json':
        state=json.loads(path.read_text())
        observations.append(dict(file=path.name,observedAt=state['observedAt'],screenshotMetadata=state['screenshots']))
    else:
        data=path.read_bytes()
        actual='JPEG' if data.startswith(b'\xff\xd8\xff') else 'PNG' if data.startswith(b'\x89PNG\r\n\x1a\n') else None
        assert actual
        images.append(dict(file=path.name,actualFormat=actual,originalNameAndBytesPreserved=True))
assert len(observations)==8 and len(images)==11
copy(producer,producer.name)
copy(__file__,Path(__file__).name)
# This hardlink staging helper was already frozen and identity-checked in the
# separate recording probe preparation; no current production file is inspected.
frozen=repo/'doc/juxtopposed-references/native/recording-tail-fifo-20261028'
support=frozen/'inputs/probe_support.py'
expected=json.loads((frozen/'producer-identities.json').read_text())['probe_support.py']
assert digest(support)==expected
copy(support,'support/probe_support.py')
omitted=[key for key in ('humanBeforeSha256','priorProfileSha256') if key in record]
public={key:value for key,value in record.items() if key not in omitted}
public['sanitization']=dict(originalSha256=digest(source/'verification.json'),omittedFields=omitted)
(target/'verification.json').write_text(json.dumps(public,indent=2)+'\n',encoding='utf-8',newline='\n')
summary=dict(scope=record['scope'],status=record['status'],exitCode=record['exitCode'],
    moduleCounts=record['moduleCounts'],diagnostics=record['diagnostics'],guards=guards,
    ownerApprovedMutation=True,watchAgainIndividualRemovalPreservedContinue=True,
    continueHomeSectionClearAccepted=True,confirmationDefaultNo=True,
    bothHistorySectionsEmpty=True,allThreeGeneratedLibraryEntriesRetained=True,
    observations=observations,images=images,
    stage=record['stage'],hardlinkSupportSha256=expected,
    hardlinkSupportProvenance='Previously frozen recording-tail-fifo preparation input; not an extra original UI producer guard',
    rawLogsMediaBinariesSettingsCopied=False)
(target/'summary.json').write_text(json.dumps(summary,indent=2)+'\n',encoding='utf-8',newline='\n')
(target/'copy-verification.json').write_text(json.dumps(dict(byteIdenticalCopies=copies,
    sanitizedVerification=public['sanitization']),indent=2)+'\n',encoding='utf-8',newline='\n')
(target/'.gitattributes').write_text('* -text\n',encoding='utf-8',newline='\n')
print(json.dumps(dict(destination=str(target),observations=len(observations),images=len(images),
    originalCopiesVerified=len(copies),allSevenGuards=True,omittedFields=omitted)))
