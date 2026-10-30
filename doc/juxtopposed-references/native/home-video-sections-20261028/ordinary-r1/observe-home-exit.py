from pathlib import Path
from hashlib import file_digest
import ctypes
from ctypes import wintypes
import json
import sys
import time

root = Path(__file__).resolve().parent
repo = root.parents[1]
run = root / sys.argv[1]
record = json.loads((run / 'verification.json').read_text())
def digest(path):
    with Path(path).open('rb') as stream:
        return file_digest(stream, 'sha256').hexdigest()
kernel = ctypes.WinDLL('kernel32', use_last_error=True)
kernel.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
kernel.OpenProcess.restype = wintypes.HANDLE
kernel.WaitForSingleObject.argtypes = [wintypes.HANDLE, wintypes.DWORD]
kernel.GetExitCodeProcess.argtypes = [wintypes.HANDLE, ctypes.POINTER(wintypes.DWORD)]
kernel.CloseHandle.argtypes = [wintypes.HANDLE]
handle = kernel.OpenProcess(0x100000 | 0x1000, False, record['pid'])
assert handle, ctypes.get_last_error()
print('Observing owned PID', record['pid'], flush=True)
try:
    assert kernel.WaitForSingleObject(handle, 600000) == 0, 'Owned process remains running; inspect it.'
    code = wintypes.DWORD()
    assert kernel.GetExitCodeProcess(handle, ctypes.byref(code))
finally:
    kernel.CloseHandle(handle)
result = dict(pid=record['pid'], exitCode=code.value, capturedAt=time.time(),
    sourceUnchanged=all(digest(repo / n)==h for n,h in record['sourceSha256'].items()),
    runtimeUnchanged=all(digest(run / 'runtime' / n)==h for n,h in record['runtimeSha256'].items()),
    fixturesUnchanged=all(digest(n)==h for n,h in record['fixtureSha256'].items()),
    producerUnchanged=digest(root/'run-home-ui-r1.py')==record['producerSha256'],
    humanConfigUnchanged=all((digest(n) if Path(n).is_file() else None)==h for n,h in record['humanBeforeSha256'].items()))
(run/'observed-exit.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
assert code.value==0 and all(v for k,v in result.items() if k.endswith('Unchanged'))
