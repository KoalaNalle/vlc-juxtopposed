from pathlib import Path
from hashlib import sha256
import json

work = Path(__file__).resolve().parent
digest = lambda p: sha256(p.read_bytes()).hexdigest()
core = digest(work / 'startup-candidate-r10-bin/libvlccore.dll')
worker = (work / 'native-core-trace-r9c.py').read_text(encoding='utf-8')
worker = worker.replace('diagnosticOnly=True', 'candidateOnly=True')
worker = worker.replace('traceSourceRecordSha256=', 'candidateSourceRecordSha256=')
worker = worker.replace('core-trace-r9c-source/source-record.json', 'startup-candidate-r10-source/source-record.json')
parent = (work / 'run-native-core-trace-r9c.py').read_text(encoding='utf-8')
parent = parent.replace("traceRecord['diagnosticOnly']", "traceRecord['candidateOnly']")
parent = parent.replace("traceRecord['traceSha256']", "traceRecord['candidateSha256']")
parent = parent.replace('core-trace-r9c', 'startup-candidate-r10')
parent = parent.replace("assert b'OWNED_DECODER_TRACE WAIT_ENTER cat=' in (work/'startup-candidate-r10-bin/libvlccore.dll').read_bytes()", "assert digest(work/'startup-candidate-r10-bin/libvlccore.dll')=='" + core + "'")
parent = parent.replace('cbd4547dbe1b209871fb49a2905bb5563e15dd6ceced7832d1fcb66268fccff9', core)
parent = parent.replace('diagnosticOnly=True', 'candidateOnly=True')
parent = parent.replace('traceSourceRecordSha256=', 'candidateSourceRecordSha256=')
assert 'core-trace-r9c' not in parent and 'OWNED_DECODER_TRACE' not in parent
assert 'diagnosticOnly' not in worker and 'diagnosticOnly' not in parent
for name, content in [('native-startup-candidate-r10.py', worker), ('run-native-startup-candidate-r10.py', parent)]:
    target = work / name
    assert not target.exists()
    target.write_text(content, encoding='utf-8', newline='\n')
record = dict(coreSha256=core, generatorSha256=digest(Path(__file__)), inheritedWorkerSha256=digest(work / 'native-core-trace-r9c.py'), inheritedParentSha256=digest(work / 'run-native-core-trace-r9c.py'), workerSha256=digest(work / 'native-startup-candidate-r10.py'), parentSha256=digest(work / 'run-native-startup-candidate-r10.py'))
(work / 'native-startup-r10-producers.json').write_text(json.dumps(record, indent=2)+'\n', encoding='utf-8', newline='\n')
print(json.dumps(record, indent=2))
