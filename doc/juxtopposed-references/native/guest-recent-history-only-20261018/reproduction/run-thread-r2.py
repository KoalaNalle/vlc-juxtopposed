from pathlib import Path
import sys,json
r=Path('E:/Modding/vlc-juxtopposed');w=r/'build-windows/beta-recent-history-only-20261018';p=r/'build-windows/beta-sidebar-shell-20261018/ordinary-runtime-r3'
sys.path.insert(0,str(r/'test/libvlc'))
from native_http_runtime import environment,run_owned
out=w/'thread-r2';out.mkdir(exist_ok=True)
result=run_owned([w/'test_thread_runner.exe','-o',out/'test.xml,xml','-o',out/'test.txt,txt'],p,out/'process',timeout=90)
(out/'verification.json').write_text(json.dumps(result,indent=2)+'\n')
print(result)
print((out/'test.txt').read_text()[-1500:] if (out/'test.txt').exists() else 'No test report')
