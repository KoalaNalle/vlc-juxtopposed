from pathlib import Path
import ast,datetime,hashlib,json,os,subprocess,time
root=Path('E:/Modding/vlc-juxtopposed');work=Path(__file__).resolve().parent
out=work/'settings-components-r1';out.mkdir(exist_ok=False);(out/'logs').mkdir();(out/'results').mkdir()
tests=root/'modules/gui/qt/tests'
names=['global','interface','privacy','controls','playback','video','audio','subtitles','library','network','performance','recording','extensions']
files=[tests/('test_juxtopposed_'+name+'_settings.py') for name in names]
files += [tests/'test_juxtopposed_appearance.py',tests/'test_juxtopposed_dialogs_host.py']
inputs=set(files)|set(tests.glob('juxtopposed_*_fixture.py'))|set((root/'modules/gui/qt/dialogs/dialogs/qml').glob('*.qml'))|{root/'modules/gui/qt/style/JuxtopposedPalette.qml'}
frozen={p.relative_to(root).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(inputs)}
(out/'inputs.json').write_text(json.dumps(frozen,indent=2)+'\n',encoding='utf-8',newline='\n')
report={'scope':'Maintained supplied-state/offscreen Qt component checks; not native app, persistence, GPU, OS DPI or full beta acceptance.','cases':{},'passed':False,'activeChildPid':None,'sourceHead':subprocess.check_output(['E:/Apps/Git/cmd/git.exe','rev-parse','HEAD'],cwd=root,text=True).strip()}
def save():(out/'verification.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8',newline='\n')
child=None
env=os.environ.copy();env.pop('PYTHONOPTIMIZE',None)
try:
 for source in files:
  name=source.stem.removeprefix('test_juxtopposed_');case=out/'results'/name
  options={a.value for node in ast.walk(ast.parse(source.read_text(encoding='utf-8-sig'))) if isinstance(node,ast.Call) and isinstance(node.func,ast.Attribute) and node.func.attr=='add_argument' for a in node.args if isinstance(a,ast.Constant) and isinstance(a.value,str)}
  command=['E:/Apps/VlcUiTests/Scripts/python.exe',str(source),'--output-dir',str(case)]
  if '--repo-root' in options:command+=['--repo-root',str(root)]
  result={'command':command,'startedAtUtc':datetime.datetime.now(datetime.timezone.utc).isoformat(timespec='seconds'),'normalExit':False,'forcedCleanupUsed':False}
  report['cases'][name]=result;started=time.monotonic()
  with (out/'logs'/(name+'.stdout.txt')).open('wb') as stdout,(out/'logs'/(name+'.stderr.txt')).open('wb') as stderr:
   child=subprocess.Popen(command,cwd=root,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
   report['activeChildPid']=child.pid;save()
   try:code=child.wait(timeout=180)
   except subprocess.TimeoutExpired:result['forcedCleanupUsed']=True;child.kill();code=child.wait(timeout=5)
  report['activeChildPid']=None
  result.update(exitCode=code,normalExit=code==0 and not result['forcedCleanupUsed'],elapsedSeconds=time.monotonic()-started,verificationExists=(case/'verification.json').exists())
  result['accepted']=result['normalExit'] and result['verificationExists']
  if result['verificationExists']:
   record=json.loads((case/'verification.json').read_text(encoding='utf-8-sig'))
   result['reportedCaseCount']=record.get('caseCount',record.get('checkCount',len(record.get('cases',record.get('checks',[])))))
  save();print(name,'PASS' if result['accepted'] else 'FAIL','exit',code,'groups',result.get('reportedCaseCount'),flush=True)
 report['inputsUnchanged']=all(hashlib.sha256((root/name).read_bytes()).hexdigest()==digest for name,digest in frozen.items())
 report['passed']=all(c['accepted'] for c in report['cases'].values()) and report['inputsUnchanged'];save()
finally:
 if child is not None and child.poll() is None:child.kill();child.wait(timeout=5)
 report['activeChildPid']=None;save()
raise SystemExit(0 if report['passed'] else 1)
