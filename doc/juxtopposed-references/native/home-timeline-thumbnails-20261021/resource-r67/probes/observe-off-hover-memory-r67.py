"""Read only the owned process's memory after the fixed benchmark ends."""
from pathlib import Path
import ctypes as C
from ctypes import wintypes as W
import json,time
from resource_metrics_r45 import snapshot,k

work=Path(__file__).resolve().parent
out=work/'ordinary-resource-A1-r67'
def read():return json.loads((out/'verification.json').read_text(encoding='utf-8'))
while read()['status'] not in ['awaiting-paused-final-inspection','terminal']:
    time.sleep(1)
v=read();assert v['status']=='awaiting-paused-final-inspection'
k.OpenProcess.argtypes=[W.DWORD,W.BOOL,W.DWORD];k.OpenProcess.restype=W.HANDLE
k.CloseHandle.argtypes=[W.HANDLE];k.CloseHandle.restype=W.BOOL
k.QueryFullProcessImageNameW.argtypes=[W.HANDLE,W.DWORD,W.LPWSTR,C.POINTER(W.DWORD)]
k.QueryFullProcessImageNameW.restype=W.BOOL
handle=k.OpenProcess(0x0410,False,v['pid']);assert handle,C.get_last_error()
try:
    buffer=C.create_unicode_buffer(32768);length=W.DWORD(len(buffer))
    assert k.QueryFullProcessImageNameW(handle,0,buffer,C.byref(length))
    assert Path(buffer.value).resolve()==(out/'runtime/vlc.exe').resolve()
    samples=[];start=time.perf_counter()
    for i in range(37):
        delay=start+5*i-time.perf_counter()
        if delay>0:time.sleep(delay)
        assert read()['status']=='awaiting-paused-final-inspection'
        data=snapshot(handle)
        samples.append(dict(elapsedSeconds=time.perf_counter()-start,
                            privateBytes=data['privateBytes'],workingSetBytes=data['workingSetBytes']))
        status='terminal' if i==36 else 'running'
        temp=out/'off-hover-memory.pending.json'
        temp.write_text(json.dumps(dict(status=status,ownedPid=v['pid'],samples=samples,
            scope='Separate 180-second paused memory observation with pointer intentionally outside the timeline; no GPU counter query, mouse/key input or native app control from this observer. The existing board observer remains separate.'),indent=2)+'\n',encoding='utf-8',newline='\n')
        temp.replace(out/'off-hover-memory.json')
    print('Owned off-hover memory observer terminal:',len(samples),'samples',flush=True)
finally:k.CloseHandle(handle)
