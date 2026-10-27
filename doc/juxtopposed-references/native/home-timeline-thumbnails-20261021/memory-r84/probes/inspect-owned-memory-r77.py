from pathlib import Path
import ctypes as C
from ctypes import wintypes as W
import json,subprocess,time

repo=Path('C:/Modding/vlc-juxtopposed');work=Path(__file__).resolve().parent
trial=work/'ordinary-memory-r70'
read=lambda p:json.loads(p.read_text(encoding='utf-8'))
assert len(read(trial/'verification.json')['phases'])==4
v=read(trial/'verification.json');assert v['status']=='awaiting-paused-final-inspection'
api=C.WinDLL('kernel32',use_last_error=True)
api.OpenProcess.argtypes=[W.DWORD,W.BOOL,W.DWORD];api.OpenProcess.restype=W.HANDLE
api.QueryFullProcessImageNameW.argtypes=[W.HANDLE,W.DWORD,W.LPWSTR,C.POINTER(W.DWORD)];api.QueryFullProcessImageNameW.restype=W.BOOL
api.CloseHandle.argtypes=[W.HANDLE]
handle=api.OpenProcess(0x1000,False,v['pid']);assert handle
try:
    name=C.create_unicode_buffer(32768);length=W.DWORD(len(name))
    assert api.QueryFullProcessImageNameW(handle,0,name,C.byref(length))
    assert Path(name.value).resolve()==(trial/'runtime/vlc.exe').resolve()
finally:api.CloseHandle(handle)
out=work/'memory-native-r77';out.mkdir();symbols=work/'os-symbols-r71';assert (symbols/'ntdll.pdb').exists()
debugger=repo/'build-windows/homecoming-20261018/tools/WinDbg/amd64/cdb.exe'
result=dict(status='starting',ownedVlcPid=v['pid'],noninvasive=True,nonsuspending=True,afterAllResourceIntervals=True,
            forcedCleanupUsed=False,fullBetaAccepted=False)
def save():(out/'verification.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8',newline='\n')
with (out/'debugger-output.txt').open('wb') as stdout:
    child=subprocess.Popen([str(debugger),'-pvr','-sins','-y',str(symbols),'-p',str(v['pid']),
        '-logo',str(out/'memory-stack.txt'),'-c','~; ~* kb 12; .detach; q'],
        stdout=stdout,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
    result.update(status='running',debuggerPid=child.pid);save()
    print('Owned post-measurement noninvasive debugger',child.pid,'target',v['pid'],flush=True)
    try:result['debuggerExitCode']=child.wait(timeout=90)
    except subprocess.TimeoutExpired:
        result['status']='observer-timeout-process-left-running';save();raise
result['status']='terminal';save()
print('Owned noninvasive memory observer terminal',result['debuggerExitCode'],flush=True)
