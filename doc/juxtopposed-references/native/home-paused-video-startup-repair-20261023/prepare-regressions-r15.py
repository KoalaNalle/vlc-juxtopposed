from pathlib import Path
from hashlib import sha256
import ast,json,shutil
repo=Path('C:/Modding/vlc-juxtopposed');work=Path(__file__).resolve().parent
prior=repo/'build-windows/beta-queue-workflow-20261022'
digest=lambda p:sha256(p.read_bytes()).hexdigest()
def save(name,text):
    ast.parse(text);target=work/name;assert not target.exists();target.write_text(text,encoding='utf-8',newline='\n')

worker=(work/'native-core-trace-r9c.py').read_text(encoding='utf-8')
worker=worker.replace('diagnosticOnly=True,privateCore=True','diagnosticOnly=False,privateCore=False')
worker=worker.replace("traceSourceRecordSha256=digest(work/'core-trace-r9c-source/source-record.json')", "coreBuildScopeSha256=digest(work/'scope-r14.json')")
worker=worker.replace('inputs-r4.json','inputs-r14.json')
save('native-tracks-r15.py',worker)
parent=(work/'run-native-core-trace-r9c.py').read_text(encoding='utf-8')
parent=parent.replace('inputs-r4.json','inputs-r14.json').replace('len(inputs)==2139','len(inputs)==2147')
start=parent.index('traceRecord=');end=parent.index('root=work/',start)
parent=parent[:start]+parent[end:]
parent=parent.replace("root=work/'native-core-trace-r9c'", "root=work/'native-tracks-r15'")
parent=parent.replace("base=work/'ordinary-ui-r4/runtime';protected=read(work/'ordinary-ui-r4/verification.json')", "base=work/'ordinary-ui-r14/runtime';protected=read(work/'ordinary-ui-r14/verification.json')")
parent=parent.replace("worker=work/'native-core-trace-r9c.py'", "worker=work/'native-tracks-r15.py'")
start=parent.index(";(runtime/'libvlccore.dll').unlink()");end=parent.index(';identities=',start)
parent=parent[:start]+parent[end:]
parent=parent.replace('cbd4547dbe1b209871fb49a2905bb5563e15dd6ceced7832d1fcb66268fccff9','000f1159b7d190d83edaba83739b97b869669ca2eec2ba97bc35d6c51f9bb9cb')
parent=parent.replace('diagnosticOnly=True,privateCore=True', 'diagnosticOnly=False,privateCore=False')
parent=parent.replace("traceSourceRecordSha256=digest(work/'core-trace-r9c-source/source-record.json')", "coreBuildScopeSha256=digest(work/'scope-r14.json')")
assert 'core-trace-r9c' not in parent
save('run-native-tracks-r15.py',parent)

audio=(prior/'run-native-audio-r23.py').read_text(encoding='utf-8')
audio=audio.replace("work=Path(__file__).resolve().parent", "work=Path('C:/Modding/vlc-juxtopposed/build-windows/beta-queue-workflow-20261022');current=Path(__file__).resolve().parent")
audio=audio.replace("inputs=read(source/'source-record.json')['files']", "allPriorInputs=read(source/'source-record.json')['files'];assert [p for p,h in allPriorInputs.items() if digest(repo/p)!=h]==['modules/gui/qt/Makefile.am'];inputs={p:h for p,h in allPriorInputs.items() if p!='modules/gui/qt/Makefile.am'}")
audio=audio.replace("out=work/('native-audio-'+phase+'-r23')", "out=current/'native-audio-r15'")
audio=audio.replace("runtime=work/'candidate-runtime-r23'", "runtime=current/'ordinary-ui-r14/runtime'")
audio=audio.replace("sourceRecordSha256=digest(source/'source-record.json')", "sourceRecordSha256=digest(source/'source-record.json'),excludedPriorDistributionFile='modules/gui/qt/Makefile.am',currentSourceManifestSha256=digest(current/'inputs-r14.json')")
save('run-native-audio-r15.py',audio)

bookmarks=(prior/'run-bookmarks-core-r23.py').read_text(encoding='utf-8')
bookmarks=bookmarks.replace("work=Path(__file__).resolve().parent", "work=Path('C:/Modding/vlc-juxtopposed/build-windows/beta-queue-workflow-20261022');current=Path(__file__).resolve().parent")
bookmarks=bookmarks.replace("out=work/'native-bookmarks-core-r23'", "out=current/'native-bookmarks-r15'")
bookmarks=bookmarks.replace("runtime=work/'candidate-runtime-r23'", "runtime=current/'ordinary-ui-r14/runtime'")
save('run-native-bookmarks-r15.py',bookmarks)

record=dict(producerSha256=digest(Path(__file__)),sourceManifestSha256=digest(work/'inputs-r14.json'),scripts={p:digest(work/p) for p in ['native-tracks-r15.py','run-native-tracks-r15.py','run-native-audio-r15.py','run-native-bookmarks-r15.py']},scope='Replays the unchanged actual native test executables and source-cue investigation on ordinary Core000f/GUI716. The earlier audio executable source record differs only in a distribution Makefile; actual compiled test/controller sources remain checked.')
(work/'regression-producers-r15.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps(record,indent=2))
