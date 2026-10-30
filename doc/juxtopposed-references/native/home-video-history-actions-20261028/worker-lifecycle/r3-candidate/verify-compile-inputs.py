"""Read-only verifier for the exact native build inputs, callable from Windows."""
from pathlib import Path
import json
from worker_support import digest,check_map
prep=Path(__file__).resolve().parent
repo=Path('/mnt/c/Modding/vlc-juxtopposed')
record=json.loads((prep/'build/verification.json').read_text())
assert record['status']=='terminal' and record['success'] and record['exitCode']==0
graph=prep/'build/compiler-inputs.json'
assert digest(graph)==record['compilerInputsSha256']
checks=dict(initial=check_map(repo,json.loads((prep/'initial-inputs.json').read_text())),
    toolchain=check_map('/',json.loads((prep/'linux-inputs.json').read_text())),
    compiled=check_map('/',json.loads(graph.read_text())),
    producers=check_map(prep,json.loads((prep/'producer-identities.json').read_text())))
print(json.dumps(dict(success=not any(checks.values()),mismatches=checks)))
assert not any(checks.values())
