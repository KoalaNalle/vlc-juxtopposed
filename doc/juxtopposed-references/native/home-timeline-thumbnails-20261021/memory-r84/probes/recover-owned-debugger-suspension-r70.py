"""Release one leftover debugger suspension per thread in this owned test only."""
from pathlib import Path
import ctypes as C
from ctypes import wintypes as W
import json

work=Path(__file__).resolve().parent;out=work/'ordinary-memory-r70'
v=json.loads((out/'verification.json').read_text(encoding='utf-8'))
record=out/'debugger-suspension-recovery.json';assert not record.exists()
api=C.WinDLL('kernel32',use_last_error=True)
api.OpenProcess.argtypes=[W.DWORD,W.BOOL,W.DWORD];api.OpenProcess.restype=W.HANDLE
api.QueryFullProcessImageNameW.argtypes=[W.HANDLE,W.DWORD,W.LPWSTR,C.POINTER(W.DWORD)];api.QueryFullProcessImageNameW.restype=W.BOOL
api.CloseHandle.argtypes=[W.HANDLE]
handle=api.OpenProcess(0x1000,False,v['pid']);assert handle
try:
    name=C.create_unicode_buffer(32768);length=W.DWORD(len(name))
    assert api.QueryFullProcessImageNameW(handle,0,name,C.byref(length))
    assert Path(name.value).resolve()==(out/'runtime/vlc.exe').resolve()
finally:api.CloseHandle(handle)
class Entry(C.Structure):
    _fields_=[('dwSize',W.DWORD),('cntUsage',W.DWORD),('threadId',W.DWORD),('ownerPid',W.DWORD),('basePriority',W.LONG),('deltaPriority',W.LONG),('flags',W.DWORD)]
api.CreateToolhelp32Snapshot.argtypes=[W.DWORD,W.DWORD];api.CreateToolhelp32Snapshot.restype=W.HANDLE
api.Thread32First.argtypes=[W.HANDLE,C.POINTER(Entry)];api.Thread32First.restype=W.BOOL
api.Thread32Next.argtypes=[W.HANDLE,C.POINTER(Entry)];api.Thread32Next.restype=W.BOOL
api.OpenThread.argtypes=[W.DWORD,W.BOOL,W.DWORD];api.OpenThread.restype=W.HANDLE
api.GetProcessIdOfThread.argtypes=[W.HANDLE];api.GetProcessIdOfThread.restype=W.DWORD
api.ResumeThread.argtypes=[W.HANDLE];api.ResumeThread.restype=W.DWORD
snapshot=api.CreateToolhelp32Snapshot(4,0);assert snapshot not in [None,C.c_void_p(-1).value]
entry=Entry();entry.dwSize=C.sizeof(entry);ids=[]
try:
    exists=api.Thread32First(snapshot,C.byref(entry))
    while exists:
        if entry.ownerPid==v['pid']:ids.append(entry.threadId)
        exists=api.Thread32Next(snapshot,C.byref(entry))
finally:api.CloseHandle(snapshot)
assert ids
rows=[]
for tid in ids:
    thread=api.OpenThread(0x802,False,tid)
    if not thread:
        rows.append(dict(threadId=tid,opened=False,error=C.get_last_error()));continue
    try:
        assert api.GetProcessIdOfThread(thread)==v['pid']
        previous=api.ResumeThread(thread)
        rows.append(dict(threadId=tid,opened=True,previousSuspendCount=previous))
    finally:api.CloseHandle(thread)
result=dict(ownedPid=v['pid'],scope='One suspension level released only from exact owned test threads after outside-debugger process inspection reported all192 waiting Suspended. No loop to zero and no process termination.',rows=rows,allPreviousCountsExactlyOne=all(r.get('previousSuspendCount')==1 for r in rows))
record.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8',newline='\n')
print('Owned debugger suspension cleanup:',len(rows),'threads; prior counts',sorted({r.get('previousSuspendCount',-1) for r in rows}))
