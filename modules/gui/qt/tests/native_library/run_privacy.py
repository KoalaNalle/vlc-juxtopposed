#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Run the real native Privacy controller and its controlled SQLite failure mode."""
import argparse,ctypes,hashlib,json,os,subprocess,sys
from pathlib import Path
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--package',type=Path,required=True)
parser.add_argument('--output-dir',type=Path,required=True)
a=parser.parse_args();package=a.package.resolve();out=a.output_dir.resolve()
assert (package/'portable').is_dir(),'Use an owned portable native test runtime'
out.mkdir(parents=True,exist_ok=False);normal=out/'normal';normal.mkdir();(normal/'temp').mkdir()
program=package/'test_qt_privacy_preferences.exe'
paths=['test_qt_privacy_preferences.exe','libvlc.dll','libvlccore.dll','plugins/misc/libmedialibrary_plugin.dll']
report=dict(passed=False,runtimeSha256={n:hashlib.sha256((package/n).read_bytes()).hexdigest()for n in paths})
env={k:v for k,v in os.environ.items() if not k.startswith('VLC_')}
env.update(VLC_PLUGIN_PATH=str(package/'plugins'),TEMP=str(normal/'temp'),TMP=str(normal/'temp'))
kernel=ctypes.WinDLL('kernel32',use_last_error=True);kernel.SetErrorMode.argtypes=[ctypes.c_uint];kernel.SetErrorMode.restype=ctypes.c_uint
previous=kernel.SetErrorMode(0x8003)
child=None
try:
    with (normal/'stdout.txt').open('wb')as stdout,(normal/'stderr.txt').open('wb')as stderr:
        try:child=subprocess.Popen([str(program),'-o',str(normal/'qtest.txt')+',txt'],cwd=normal,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
        finally:kernel.SetErrorMode(previous)
        try:code=child.wait(timeout=60);report['forcedCleanupUsed']=False
        except subprocess.TimeoutExpired:child.kill();code=child.wait(timeout=5);report['forcedCleanupUsed']=True
    text=(normal/'qtest.txt').read_text(encoding='utf-8')if(normal/'qtest.txt').exists()else''
    report.update(exitCode=code,normalExit=code==0 and not report['forcedCleanupUsed'],qtestCases=14)
    assert report['normalExit']and 'Totals: 14 passed, 0 failed, 0 skipped, 0 blacklisted' in text,text
    root=Path(__file__).resolve().parents[5]
    command=[sys.executable,str(root/'test/libvlc/native_history_clear.py'),'--qt-controller','--package',str(package),'--output-dir',str(out/'database-failure')]
    with (out/'pipe-stdout.txt').open('wb')as stdout,(out/'pipe-stderr.txt').open('wb')as stderr:
        pipe=subprocess.run(command,cwd=root,stdout=stdout,stderr=stderr,timeout=80,creationflags=subprocess.CREATE_NO_WINDOW)
    report['databaseFailure']=json.loads((out/'database-failure/verification.json').read_text(encoding='utf-8'))
    assert pipe.returncode==0 and report['databaseFailure']['passed']
    report['passed']=True
finally:
    if child is not None and child.poll()is None:child.kill();child.wait(timeout=5)
    (out/'verification.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps(dict(passed=report['passed'],nativeQtCases=14,realDatabaseControllerChecks=9),indent=2),flush=True)
raise SystemExit(0 if report['passed']else 1)
