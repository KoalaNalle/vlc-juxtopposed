from pathlib import Path
from hashlib import sha256
import ast,json
work=Path(__file__).resolve().parent
digest=lambda p:sha256(p.read_bytes()).hexdigest()
worker=work/'native-mkv-interval-r25.py';parent=work/'run-native-mkv-interval-r25.py'
ast.parse(worker.read_text(encoding='utf-8'));tree=ast.parse(parent.read_text(encoding='utf-8'))
assignments=[n for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='refs' for t in n.targets)]
assert len(assignments)==1;refs=ast.literal_eval(assignments[0].value);assert len(refs)==5
assert not (work/'native-mkv-interval-r25').exists()
record=dict(generatorSha256=digest(work/'prepare-native-interval-r25.py'),preflightProducerSha256=digest(work/'prepare-native-interval-r25-preflight.py'),metadataFinalizerSha256=digest(Path(__file__)),preflightCorrections=['The pass-assignment appears twice; only the first belongs to observe(). This assertion failed before creating a worker.','The generator then created both valid scripts but tried to read a reference variable that belongs to the generated parent. Metadata finalization failed before native execution. This helper reads that literal from the parsed parent without executing it.'],workerSha256=digest(worker),parentSha256=digest(parent),references={p:digest(work/p) for p in refs},nativeExecutionStartedAtFinalization=False,scope='Matched full-plugin controls; reference comparison qualifies complete overlapping captions.11s uses the earlier visually verified candidate picture;15s/31s/35s use separate normal-playback controls. Production source/runtime remain unchanged.')
target=work/'native-interval-producers-r25.json';assert not target.exists();target.write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps(record,indent=2))
