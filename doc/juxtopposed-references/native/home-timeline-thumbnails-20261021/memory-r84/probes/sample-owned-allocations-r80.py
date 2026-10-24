"""Bounded post-measurement allocator sample in the exact owned test process."""
from pathlib import Path
import ctypes as C
from ctypes import wintypes as W
import json,subprocess

repo=Path('C:/Modding/vlc-juxtopposed');work=Path(__file__).resolve().parent
trial=work/'ordinary-memory-disabled-r78'
v=json.loads((trial/'verification.json').read_text(encoding='utf-8'))
assert v['status']=='awaiting-paused-final-inspection' and len(v['phases'])==1
api=C.WinDLL('kernel32',use_last_error=True)
api.OpenProcess.argtypes=[W.DWORD,W.BOOL,W.DWORD];api.OpenProcess.restype=W.HANDLE
api.QueryFullProcessImageNameW.argtypes=[W.HANDLE,W.DWORD,W.LPWSTR,C.POINTER(W.DWORD)];api.QueryFullProcessImageNameW.restype=W.BOOL
api.CloseHandle.argtypes=[W.HANDLE]
h=api.OpenProcess(0x1000,False,v['pid']);assert h
try:
    name=C.create_unicode_buffer(32768);length=W.DWORD(len(name));assert api.QueryFullProcessImageNameW(h,0,name,C.byref(length))
    assert Path(name.value).resolve()==(trial/'runtime/vlc.exe').resolve()
finally:api.CloseHandle(h)
out=work/'allocation-native-r80';out.mkdir()
commands=out/'commands.txt'
body=['r @$t0=0']
action='r @$t0=@$t0+1; .echo OWNED_ALLOC_malloc; r rcx; kb 14; .if (@$t0 >= 32) { .echo OWNED_ALLOC_COMPLETE; r @$t0; bc *; .detach; q; } .else { gc; }'
body.append('bp msvcrt!malloc "'+action+'"')
body.append('g')
commands.write_text('\n'.join(body)+'\n',encoding='utf-8',newline='\n')
result=dict(status='starting',ownedVlcPid=v['pid'],invasiveDebugger=True,afterAllResourceIntervals=True,
            temporaryAllocatorBreakpoints=True,maximumObservedAllocations=32,maximumCapturedStacks=32,
            detachOnDebuggerExit=True,forcedCleanupUsed=False,fullBetaAccepted=False)
def save():(out/'verification.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8',newline='\n')
debugger=repo/'build-windows/homecoming-20261018/tools/WinDbg/amd64/cdb.exe'
with (out/'debugger-output.txt').open('wb') as stdout:
    child=subprocess.Popen([str(debugger),'-pd','-sins','-y',str(work/'os-symbols-r71'),'-p',str(v['pid']),
        '-logo',str(out/'allocation-stack.txt'),'-cf',str(commands)],stdout=stdout,stderr=subprocess.STDOUT,
        creationflags=subprocess.CREATE_NO_WINDOW)
    result.update(status='running',debuggerPid=child.pid);save()
    print('Owned bounded allocator observer',child.pid,'target',v['pid'],flush=True)
    try:result['debuggerExitCode']=child.wait(timeout=90)
    except subprocess.TimeoutExpired:
        result['status']='observer-timeout-process-left-running';save();raise
text=(out/'allocation-stack.txt').read_text(encoding='utf-8',errors='replace')
result.update(status='terminal',boundedCaptureCompleted=any(line.strip()=='OWNED_ALLOC_COMPLETE' for line in text.splitlines()),
              detached='Detached' in text,allocationRecords=sum(line.strip()=='OWNED_ALLOC_malloc' for line in text.splitlines()))
save();print('Bounded allocator observer terminal',result['debuggerExitCode'],'completed',result['boundedCaptureCompleted'],'detached',result['detached'],flush=True)
