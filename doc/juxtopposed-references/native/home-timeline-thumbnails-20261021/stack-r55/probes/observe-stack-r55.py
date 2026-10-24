from pathlib import Path
import ctypes as C,json,subprocess,time
from ctypes import wintypes as W
repo=Path('C:/Modding/vlc-juxtopposed');work=repo/'build-windows/beta-timeline-thumbnails-20261021';out=work/'ui-extended-stack-r55'
def read(p):return json.loads(p.read_text(encoding='utf-8'))
r=read(out/'verification.json');pid=r['pid'];expected=str((out/'runtime/vlc.exe').resolve()).lower()
k=C.WinDLL('kernel32',use_last_error=True);k.OpenProcess.argtypes=[W.DWORD,W.BOOL,W.DWORD];k.OpenProcess.restype=W.HANDLE;k.CloseHandle.argtypes=[W.HANDLE];k.QueryFullProcessImageNameW.argtypes=[W.HANDLE,W.DWORD,W.LPWSTR,C.POINTER(W.DWORD)];k.GetExitCodeProcess.argtypes=[W.HANDLE,C.POINTER(W.DWORD)]
h=k.OpenProcess(0x1000,False,pid);assert h
name=C.create_unicode_buffer(32768);size=W.DWORD(len(name));assert k.QueryFullProcessImageNameW(h,0,name,C.byref(size));assert str(Path(name.value).resolve()).lower()==expected
receipt=dict(ownedPid=pid,expectedExecutable=expected,status='waiting-for-repeated-preview',purpose='Non-invasive native stack diagnosis. Timings in debugger-attached journey are not acceptance measurements.',snapshots=[])
def save():(out/'stack-observer.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8')
def alive():
 code=W.DWORD();return bool(k.GetExitCodeProcess(h,C.byref(code))) and code.value==259
save();print('Owned stack observer waiting on PID',pid,flush=True)
try:
 deadline=time.monotonic()+600
 while alive():
  try:j=read(out/'journey.json')
  except (FileNotFoundError,json.JSONDecodeError):j={}
  if len(j.get('previews',[]))>=3:break
  assert time.monotonic()<deadline,'Preview observation deadline expired'
  time.sleep(.02)
 for i in range(3):
  if not alive():break
  symbols=out/('local-symbols-'+str(i));symbols.mkdir()
  debugger=repo/'build-windows/homecoming-20261018/tools/WinDbg/amd64/cdb.exe';log=out/f'render-stack-{i}.txt'
  with (out/f'debugger-output-{i}.txt').open('wb') as transcript:
   debugger_result=subprocess.run([str(debugger),'-pv','-sins','-y',str(symbols),'-p',str(pid),'-logo',str(log),'-c','lm m libqt_plugin; ~* kb; .detach; q'],stdout=transcript,stderr=transcript,timeout=20,creationflags=subprocess.CREATE_NO_WINDOW)
  receipt['snapshots'].append(dict(index=i,exitCode=debugger_result.returncode,stackFile=log.name,stackBytes=log.stat().st_size,ownedStillLive=alive()))
  save();assert debugger_result.returncode==0
  time.sleep(.15)
 receipt['status']='terminal';save();print('Stack observer terminal:',len(receipt['snapshots']),'detached snapshots',flush=True)
finally:k.CloseHandle(h)
