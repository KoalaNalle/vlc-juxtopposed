from pathlib import Path
import hashlib,json,os,subprocess,time
root=Path('E:/Modding/vlc-juxtopposed');work=Path(__file__).resolve().parent
out=work/'library-regressions-r1';out.mkdir(exist_ok=False);(out/'logs').mkdir()
source=root/'build-windows/beta-extensions-20261017/page-runtime-r4'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
selected=['libvlccore.dll','plugins/gui/libqt_plugin.dll','plugins/misc/libmedialibrary_plugin.dll','vlc.exe','vlc-preparser.exe','plugins/access/libfilesystem_plugin.dll','plugins/control/libwin_msg_plugin.dll']
before={name:sha(source/name) for name in selected}
report={'sourceHead':subprocess.check_output(['E:/Apps/Git/cmd/git.exe','rev-parse','HEAD'],cwd=root,text=True).strip(),'ordinaryRuntimeSha256':before,'cases':{},'passed':False,'activeRunnerPid':None}
def save():(out/'verification.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8',newline='\n')
for scope,expected in [('metadata',18),('opening',42),('process',9)]:
 build=work/'library-native-r1'
 if scope=='process':build=build/'process'
 case=out/scope
 command=['E:/Apps/Python312/python.exe',str(root/'modules/gui/qt/tests/native_library/run.py'),scope,'--package',str(source),'--native-build',str(build),'--output-dir',str(case)]
 if scope!='process':command+=['--module-build',str(work/'library-native-r1/modules')]
 result={'expectedCases':expected,'command':command,'accepted':False};report['cases'][scope]=result
 started=time.monotonic()
 with (out/'logs'/(scope+'.stdout.txt')).open('wb') as stdout,(out/'logs'/(scope+'.stderr.txt')).open('wb') as stderr:
  child=subprocess.Popen(command,cwd=root,env=os.environ.copy(),stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
  report['activeRunnerPid']=child.pid;save();code=child.wait()
 report['activeRunnerPid']=None;result.update(runnerExitCode=code,elapsedSeconds=time.monotonic()-started)
 verification=case/'results/verification.json'
 if verification.exists():
  record=json.loads(verification.read_text(encoding='utf-8-sig'));result['nativeResult']=record
  result['accepted']=code==0 and record['terminationConfirmed'] and not record['timedOut'] and not record['forcedCleanupUsed'] and record['counts']=={'tests':expected,'failures':0,'errors':0,'skipped':0}
 save();print(scope,'PASS' if result['accepted'] else 'FAIL','cases',result.get('nativeResult',{}).get('counts'),flush=True)
 if not result['accepted']:break
report['sourceRuntimePreserved']=before=={name:sha(source/name) for name in selected}
report['passed']=len(report['cases'])==3 and all(c['accepted'] for c in report['cases'].values()) and report['sourceRuntimePreserved'];save()
raise SystemExit(0 if report['passed'] else 1)
