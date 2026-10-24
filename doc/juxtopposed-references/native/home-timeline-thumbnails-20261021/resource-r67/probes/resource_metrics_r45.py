"""Read-only Windows process/system/engine counters; no extra packages."""
import ctypes as C
from ctypes import wintypes as W
import os,subprocess,time

k=C.WinDLL('kernel32',use_last_error=True)
p=C.WinDLL('psapi',use_last_error=True)
class Memory(C.Structure):
    _fields_=[('cb',W.DWORD),('pageFaults',W.DWORD)]+[(n,C.c_size_t) for n in ['peakWorkingSet','workingSet','pagedPeak','paged','nonpagedPeak','nonpaged','pagefile','peakPagefile','privateBytes']]
k.GetProcessTimes.argtypes=[W.HANDLE]+[C.POINTER(W.FILETIME)]*4
k.GetProcessTimes.restype=W.BOOL
k.GetSystemTimes.argtypes=[C.POINTER(W.FILETIME)]*3
k.GetSystemTimes.restype=W.BOOL
p.GetProcessMemoryInfo.argtypes=[W.HANDLE,C.POINTER(Memory),W.DWORD]
p.GetProcessMemoryInfo.restype=W.BOOL
def ticks(t):return (t.dwHighDateTime<<32)|t.dwLowDateTime
def snapshot(handle):
    a,b,c,d=[W.FILETIME() for _ in range(4)]
    assert k.GetProcessTimes(handle,C.byref(a),C.byref(b),C.byref(c),C.byref(d)),C.get_last_error()
    idle,kernel,user=[W.FILETIME() for _ in range(3)]
    assert k.GetSystemTimes(C.byref(idle),C.byref(kernel),C.byref(user)),C.get_last_error()
    m=Memory();m.cb=C.sizeof(m)
    assert p.GetProcessMemoryInfo(handle,C.byref(m),m.cb),C.get_last_error()
    return dict(wall=time.perf_counter(),process=ticks(c)+ticks(d),idle=ticks(idle),system=ticks(kernel)+ticks(user),workingSetBytes=m.workingSet,privateBytes=m.privateBytes)

class CounterValue(C.Structure):
    class Value(C.Union):_fields_=[('doubleValue',C.c_double),('longValue',W.LONG),('largeValue',C.c_longlong)]
    _fields_=[('status',W.DWORD),('value',Value)]
class CounterItem(C.Structure):_fields_=[('name',W.LPWSTR),('value',CounterValue)]
class Gpu:
    def __init__(self):
        self.api=C.WinDLL('pdh');self.query=W.HANDLE();self.counter=W.HANDLE();self.error=None
        self.api.PdhOpenQueryW.argtypes=[W.LPCWSTR,C.c_size_t,C.POINTER(W.HANDLE)]
        self.api.PdhAddEnglishCounterW.argtypes=[W.HANDLE,W.LPCWSTR,C.c_size_t,C.POINTER(W.HANDLE)]
        self.api.PdhCollectQueryData.argtypes=[W.HANDLE]
        self.api.PdhGetFormattedCounterArrayW.argtypes=[W.HANDLE,W.DWORD,C.POINTER(W.DWORD),C.POINTER(W.DWORD),C.c_void_p]
        self.api.PdhCloseQuery.argtypes=[W.HANDLE]
        for f in ['PdhOpenQueryW','PdhAddEnglishCounterW','PdhCollectQueryData','PdhGetFormattedCounterArrayW','PdhCloseQuery']:
            getattr(self.api,f).restype=W.LONG
        for code in [self.api.PdhOpenQueryW(None,0,C.byref(self.query)),self.api.PdhAddEnglishCounterW(self.query,r'\GPU Engine(*)\Utilization Percentage',0,C.byref(self.counter))]:
            if code:self.error=hex(code&0xffffffff)
        if not self.error:self.api.PdhCollectQueryData(self.query)
    def sample(self,pid):
        if self.error:return dict(available=False,error=self.error)
        code=self.api.PdhCollectQueryData(self.query)
        if code:return dict(available=False,error=hex(code&0xffffffff))
        size=W.DWORD();count=W.DWORD()
        code=self.api.PdhGetFormattedCounterArrayW(self.counter,0x200,C.byref(size),C.byref(count),None)
        if (code&0xffffffff)!=0x800007d2:return dict(available=False,error=hex(code&0xffffffff))
        data=C.create_string_buffer(size.value)
        code=self.api.PdhGetFormattedCounterArrayW(self.counter,0x200,C.byref(size),C.byref(count),data)
        if code:return dict(available=False,error=hex(code&0xffffffff))
        rows=C.cast(data,C.POINTER(CounterItem));owned={}
        for i in range(count.value):
            name=rows[i].name
            if name.startswith('pid_'+str(pid)+'_') and rows[i].value.status in [0,1]:
                owned[name]=rows[i].value.value.doubleValue
        return dict(available=bool(owned),engines=owned,busiestEnginePercent=max(owned.values(),default=None))
    def close(self):
        if self.query:self.api.PdhCloseQuery(self.query);self.query=W.HANDLE()

def nvidia_sample():
    try:
        r=subprocess.run(['C:/Windows/System32/nvidia-smi.exe','--query-gpu=index,name,power.draw,utilization.gpu,clocks.gr,temperature.gpu','--format=csv,noheader,nounits'],capture_output=True,text=True,timeout=3,creationflags=subprocess.CREATE_NO_WINDOW)
        if r.returncode:return dict(available=False,error=r.stderr.strip())
        rows=[]
        for line in r.stdout.splitlines():
            values=[v.strip() for v in line.split(',')]
            if len(values)!=6:continue
            row=dict(zip(['index','name','boardPowerW','deviceUtilizationPercent','graphicsClockMHz','temperatureC'],values))
            for name in ['index','boardPowerW','deviceUtilizationPercent','graphicsClockMHz','temperatureC']:
                try:row[name]=float(row[name])
                except ValueError:pass
            rows.append(row)
        return dict(available=bool(rows),scope='Whole GPU board including other applications; not process-attributable power',devices=rows)
    except (OSError,subprocess.TimeoutExpired) as e:return dict(available=False,error=str(e))

def sample_process(child,duration,destination):
    import json
    gpu=Gpu();previous=snapshot(child._handle);start=previous['wall'];samples=[]
    try:
        for i in range(duration):
            wait=start+i+1-time.perf_counter()
            if wait>0:time.sleep(wait)
            assert child.poll() is None,'Owned VLC exited during measurement'
            current=snapshot(child._handle);wall=current['wall']-previous['wall'];total=current['system']-previous['system']
            row=dict(elapsedSeconds=current['wall']-start,intervalSeconds=wall,cpuOneCorePercent=(current['process']-previous['process'])/1e7/wall*100,cpuMachinePercent=(current['process']-previous['process'])/1e7/wall*100/os.cpu_count(),systemCpuPercent=(1-(current['idle']-previous['idle'])/total)*100,workingSetBytes=current['workingSetBytes'],privateBytes=current['privateBytes'],gpu=gpu.sample(child.pid))
            if i%4==0:row['board']=nvidia_sample()
            samples.append(row);previous=current
            destination.write_text(json.dumps(dict(status='measuring',logicalCpuCount=os.cpu_count(),samples=samples),indent=2)+'\n',encoding='utf-8')
    finally:gpu.close()
    result=dict(status='terminal',logicalCpuCount=os.cpu_count(),samples=samples)
    destination.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    return result
