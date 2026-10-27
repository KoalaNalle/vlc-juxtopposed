from pathlib import Path
import argparse,ctypes as C,json,re,subprocess,time
from ctypes import wintypes as W
from resource_metrics_r45 import Gpu,CounterItem
parser=argparse.ArgumentParser();parser.add_argument('--trial',choices=['B1','A1'],required=True);args=parser.parse_args()
work=Path(__file__).resolve().parent;out=work/('ordinary-resource-'+args.trial+'-r64')
owned=json.loads((out/'verification.json').read_text())['pid']
gpu=Gpu();samples=[];start=time.perf_counter()
def all_engines():
    size=W.DWORD();count=W.DWORD()
    code=gpu.api.PdhGetFormattedCounterArrayW(gpu.counter,0x200,C.byref(size),C.byref(count),None)
    if (code&0xffffffff)!=0x800007d2:return {}
    data=C.create_string_buffer(size.value)
    if gpu.api.PdhGetFormattedCounterArrayW(gpu.counter,0x200,C.byref(size),C.byref(count),data):return {}
    rows=C.cast(data,C.POINTER(CounterItem));result={}
    for i in range(count.value):
        m=re.match(r'pid_(\d+)_.*_engtype_(.*)',rows[i].name)
        if m and rows[i].value.status in [0,1] and rows[i].value.value.doubleValue>.005:
            pid,kind=m.groups();entry=result.setdefault(pid,{})
            entry[kind]=entry.get(kind,0)+rows[i].value.value.doubleValue
    return result
try:
    for i in range(300):
        wait=start+2*i-time.perf_counter()
        if wait>0:time.sleep(wait)
        ownedGpu=gpu.sample(owned)
        telemetry=subprocess.run(['C:/Windows/System32/nvidia-smi.exe','--query-gpu=pstate,clocks.gr,clocks.mem,power.draw,utilization.gpu,utilization.memory,temperature.gpu','--format=csv,noheader,nounits'],capture_output=True,text=True,timeout=3,creationflags=subprocess.CREATE_NO_WINDOW)
        status=json.loads((out/'verification.json').read_text())['status']
        samples.append(dict(elapsedSeconds=time.perf_counter()-start,phase=status,ownedGpu=ownedGpu,activeProcessEnginePercent=all_engines(),boardCsv=telemetry.stdout.strip(),boardQueryExit=telemetry.returncode))
        (out/'board-observer-r64.json').write_text(json.dumps(dict(status='running',ownedPid=owned,scope='Read-only whole-board power/P-state/core/memory clocks plus per-process engine classes; numeric PIDs only, no other app names. Same observer used in matched B1/A1 trials; starts at the paused preview gate, with a 600 second cap and 20 second post-exit cooldown.',samples=samples),indent=2)+'\n',encoding='utf-8')
        if status=='terminal' and sum(s['phase']=='terminal' for s in samples)>=10:break
finally:gpu.close()
(out/'board-observer-r64.json').write_text(json.dumps(dict(status='terminal',ownedPid=owned,samples=samples),indent=2)+'\n',encoding='utf-8')
print('Board observer terminal',args.trial,'samples',len(samples),flush=True)
