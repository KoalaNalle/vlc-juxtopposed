from pathlib import Path
from hashlib import sha256
import ctypes,json,os,re,subprocess,sys,wave
repo=Path('C:/Modding/vlc-juxtopposed');work=Path('C:/Modding/vlc-juxtopposed/build-windows/beta-queue-workflow-20261022');current=Path(__file__).resolve().parent
phase=sys.argv[1];assert phase in ['before','after']
def digest(p):return sha256(p.read_bytes()).hexdigest()
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
source=work/('native-audio-'+phase+'-r23-source');allPriorInputs=read(source/'source-record.json')['files'];assert [p for p,h in allPriorInputs.items() if digest(repo/p)!=h]==['modules/gui/qt/Makefile.am'];inputs={p:h for p,h in allPriorInputs.items() if p!='modules/gui/qt/Makefile.am'}
assert all(digest(repo/p)==h and digest(source/p)==h for p,h in inputs.items())
out=current/'native-audio-r15';assert not out.exists();out.mkdir();userdata=out/'userdata';userdata.mkdir()
runtime=current/'ordinary-ui-r14/runtime';exe=work/('native-audio-'+phase+'-r23-bin/test_native_paused_audio_seek.exe');assert exe.is_file()
media={}
for seconds in [30,10]:
    path=out/('silence-'+str(seconds)+'.wav')
    with wave.open(str(path),'wb') as wav:
        wav.setnchannels(2);wav.setsampwidth(2);wav.setframerate(48000);wav.writeframes(b'\0'*4*48000*seconds)
    media[seconds]=path
env={k:v for k,v in os.environ.items() if not k.startswith('VLC_')}
env.update(PATH=str(runtime)+os.pathsep+env.get('PATH',''),VLC_LIB_PATH=str(runtime/'plugins'),VLC_LIBEXEC_PATH=str(runtime/'vlc-preparser.exe'),
    VLC_USERDATA_PATH=str(userdata),VLC_PAUSED_AUDIO_OUTPUT=str(out/'observations'),VLC_PAUSED_AUDIO_LONG_WAV=str(media[30]),VLC_PAUSED_AUDIO_SHORT_WAV=str(media[10]),QT_QPA_PLATFORM='offscreen',QTEST_FUNCTION_TIMEOUT='90000')
result=dict(phase=phase,status='starting',passed=False,expectedNegativeVerified=False,fullBetaAccepted=False,forcedCleanupUsed=False,
    sourceRecordSha256=digest(source/'source-record.json'),excludedPriorDistributionFile='modules/gui/qt/Makefile.am',currentSourceManifestSha256=digest(current/'inputs-r14.json'),producerSha256=digest(Path(__file__)),executableSha256=digest(exe),
    runtimeSha256={p:digest(runtime/p) for p in ['libvlc.dll','libvlccore.dll','plugins/audio_output/libwasapi_plugin.dll']},mediaSha256={str(n):digest(p) for n,p in media.items()})
def save():write(out/'verification.json',result)
kernel=ctypes.WinDLL('kernel32',use_last_error=True);kernel.SetErrorMode.argtypes=[ctypes.c_uint];kernel.SetErrorMode.restype=ctypes.c_uint;previous=kernel.SetErrorMode(0x8003)
try:
    with (out/'stdout.txt').open('wb') as stdout,(out/'stderr.txt').open('wb') as stderr:
        child=subprocess.Popen([str(exe),'-o','results.txt,txt','-o','results.xml,xunitxml'],cwd=out,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
        result.update(status='running',pid=child.pid);save();print('Owned paused-audio native PID',child.pid,flush=True)
        try:result['exitCode']=child.wait(timeout=120)
        except subprocess.TimeoutExpired:
            result['status']='observation-timeout-process-left-running';save();raise SystemExit('Inspect the saved PID; no automatic cleanup or restart')
finally:kernel.SetErrorMode(previous)
text=(out/'results.txt').read_text(encoding='utf-8-sig',errors='replace') if (out/'results.txt').exists() else ''
totals=re.search(r'Totals:\s*(\d+) passed, (\d+) failed, (\d+) skipped',text)
counts=dict(zip(['passed','failed','skipped'],map(int,totals.groups()))) if totals else None
failures=re.findall(r'^FAIL!\s*:\s*TestNativePausedAudioSeek::([^\n]+?\))',text,re.M)
logs='\n'.join(p.read_text(encoding='utf-8-sig',errors='replace') for p in [out/'stdout.txt',out/'stderr.txt',*(out/'observations').glob('*.log')])
diagnostics=[line for line in logs.splitlines() if re.search(r'\berror:|Assertion failed|ReferenceError|TypeError|Binding loop',line)]
diagnostics += [line for line in text.splitlines() if re.match(r'^(?:QWARN|QFATAL|QCRITICAL)',line)]
nativeLogs=list((out/'observations').glob('native-*.log'))
wasapi=sum('using aout stream module "wasapi"' in p.read_text(encoding='utf-8-sig',errors='replace') for p in nativeLogs)
positive=phase=='after' and result['exitCode']==0 and counts==dict(passed=7,failed=0,skipped=0)
negative=phase=='before' and result['exitCode']==3 and counts==dict(passed=4,failed=3,skipped=0) and set(failures)=={'actualInitialAndPlayedPausedSeeks(initial-position)','actualInitialAndPlayedPausedSeeks(initial-time)','actualPausedMediaSwitchUsesTheNewDuration()'}
result.update(status='terminal',normalTermination=result['exitCode'] in [0,3],counts=counts,failedCases=failures,diagnostics=diagnostics,
    actualWasapiLogs=wasapi,nativeLogCount=len(nativeLogs),sourceInputsUnchanged=all(digest(repo/p)==h for p,h in inputs.items()),
    runtimeUnchanged=all(digest(runtime/p)==h for p,h in result['runtimeSha256'].items()),passed=positive and not diagnostics and wasapi==5,expectedNegativeVerified=negative and not diagnostics and wasapi==5,
    scope='Real production controller and NORMAL-lock native player, initial/played paused WAV seeking, zero/repeated seeks, actual WASAPI resume and different-duration media replacement. No QML or physical audibility/DPI acceptance.')
save();print(json.dumps({k:result[k] for k in ['exitCode','passed','expectedNegativeVerified','counts','failedCases','diagnostics','actualWasapiLogs']},indent=2))
raise SystemExit(0 if result['passed'] or result['expectedNegativeVerified'] else 1)
