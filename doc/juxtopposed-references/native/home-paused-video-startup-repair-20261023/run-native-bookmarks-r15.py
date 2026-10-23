from pathlib import Path
import ctypes,hashlib,json,os,re,subprocess,sys
repo=Path('C:/Modding/vlc-juxtopposed');work=Path('C:/Modding/vlc-juxtopposed/build-windows/beta-queue-workflow-20261022');current=Path(__file__).resolve().parent
phase=sys.argv[1];assert phase in ['before','after']
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
suffix=sys.argv[2] if len(sys.argv)>2 else ''
assert suffix in ['', 'b']
out=current/'native-bookmarks-r15';assert not out.exists();out.mkdir();userdata=out/'userdata';userdata.mkdir()
runtime=current/'ordinary-ui-r14/runtime';exe=work/('native-'+phase+'-r16-bin/test_qt_native_bookmarks_chapters.exe')
fixtures=work/'ordinary-ui-r15/media';assert exe.is_file()
prior_native=read(work/'native-after-r16/verification.json')
assert phase=='after' and digest(exe)==prior_native['exeSha256']
assert digest(repo/'modules/gui/qt/player/input_models.cpp')==prior_native['sourceSha256']['input_models.cpp']
assert digest(repo/'modules/gui/qt/tests/test_native_bookmarks_chapters.cpp')==prior_native['sourceSha256']['test_native_bookmarks_chapters.cpp']

env={k:v for k,v in os.environ.items() if not k.startswith('VLC_')}
env.update(PATH=str(runtime)+os.pathsep+env.get('PATH',''),VLC_LIB_PATH=str(runtime/'plugins'),VLC_LIBEXEC_PATH=str(runtime/'vlc-preparser.exe'),
    VLC_USERDATA_PATH=str(userdata),VLC_CHAPTER_FIXTURE_DIR=str(fixtures),QT_QPA_PLATFORM='offscreen',QTEST_FUNCTION_TIMEOUT='90000')
expected=['initTestCase','actualThreeChapterMetadataAndSelection','actualBookmarkAddRenameSeekAndRemove',
    'actualMediaSwitchRejectsOldBookmarksBeforeGuiDispatch','actualLateNativeMediaRegistrationRestoresAdd',
    'actualStopSameMediaRestartRetainsBookmark','actualNativeRestartPersistsBookmarkAndMetadata','actualModelDetachAndOwnerDestruction','cleanupTestCase']
result=dict(phase=phase,status='starting',passed=False,expectedNegativeVerified=False,forcedCleanupUsed=False,
    producerSha256=digest(Path(__file__)),exeSha256=digest(exe),
    runtimeSha256={p:digest(runtime/p) for p in ['libvlc.dll','libvlccore.dll','plugins/misc/libmedialibrary_plugin.dll']},
    mediaSha256={p:digest(fixtures/p) for p in ['chaptered-colors.mkv','paused-seek-colors.mp4']},
    sourceSha256={p:digest(work/('native-'+phase+'-r16-source')/p) for p in ['input_models.cpp','test_native_bookmarks_chapters.cpp']})
def save():write(out/'verification.json',result)
k=ctypes.WinDLL('kernel32',use_last_error=True);k.SetErrorMode.argtypes=[ctypes.c_uint];k.SetErrorMode.restype=ctypes.c_uint;previous=k.SetErrorMode(0x8003)
try:
    with (out/'stdout.txt').open('wb') as stdout,(out/'stderr.txt').open('wb') as stderr:
        process=subprocess.Popen([str(exe),'-o','results.txt,txt','-o','results.xml,xunitxml'],cwd=out,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
        result.update(status='running',pid=process.pid);save();print('Owned native model PID',process.pid,flush=True)
        try:result['exitCode']=process.wait(timeout=180)
        except subprocess.TimeoutExpired:
            result.update(status='timeout-process-left-running');save();raise SystemExit('Inspect exact saved PID; no automatic cleanup/restart')
finally:k.SetErrorMode(previous)
if not (out/'results.txt').is_file():
    result.update(status='terminal-no-test-results',normalExit=result['exitCode']==0,error='Process exited before producing QtTest results')
    save();raise SystemExit(json.dumps(result))
text=(out/'results.txt').read_text(encoding='utf-8-sig',errors='replace')
passes=re.findall(r'^PASS\s*:\s*TestNativeBookmarksChapters::([^\(]+)\(\)',text,re.M)
failures=re.findall(r'^FAIL!\s*:\s*TestNativeBookmarksChapters::([^\(]+)\(\)',text,re.M)
totals=re.search(r'Totals:\s*(\d+) passed, (\d+) failed, (\d+) skipped',text)
counts=dict(zip(['passed','failed','skipped'],map(int,totals.groups()))) if totals else None
logs='\n'.join(p.read_text(encoding='utf-8-sig',errors='replace') for p in [out/'stdout.txt',out/'stderr.txt',*out.glob('*.log')])
diagnostics=[l for l in logs.splitlines() if re.search(r'\berror:|Assertion failed|TypeError|ReferenceError|Binding loop',l)]
network=bool(re.search(r'mDNS:\s*listening|Initializing libupnp|datagram port 9875|224\.2\.127\.254',logs,re.I))
expected_shutdown=[l for l in diagnostics if l=='main error: window closed']
unexpected=[l for l in diagnostics if l!='main error: window closed']
# This exact close-notify message also occurs once per native instance in the
# retained October14 fixture. Keep it visible and require that it is at teardown.
shutdown_context=True
for path in out.glob('bookmarks-chapters-native-*.log'):
    lines=path.read_text(encoding='utf-8-sig',errors='replace').splitlines()
    closes=[i for i,l in enumerate(lines) if l=='main error: window closed']
    shutdown_context &= len(closes)==1 and any('main debug: exiting' in l for l in lines[closes[0]+1:closes[0]+12])
shutdown_ok=len(expected_shutdown)==2 and shutdown_context
positive=phase=='after' and result['exitCode']==0 and passes==expected and counts==dict(passed=9,failed=0,skipped=0)
negative=phase=='before' and result['exitCode']==1 and failures==['actualThreeChapterMetadataAndSelection'] and counts==dict(passed=8,failed=1,skipped=0) and 'chapterTime.valid()' in text
result.update(status='terminal',normalExit=result['exitCode'] in [0,1],passed=positive and not unexpected and shutdown_ok and not network,
    expectedNegativeVerified=negative and not unexpected and shutdown_ok and not network,counts=counts,passedCases=passes,failedCases=failures,diagnostics=diagnostics,
    expectedShutdownDiagnostics=expected_shutdown,shutdownContextVerified=shutdown_ok,unexpectedDiagnostics=unexpected,networkDiscoveryObserved=network,
    scope='Maintained native test on real local ML, production controller and NORMAL-lock playlist: real chapter metadata/selection, bookmark mutation/seek, switching, persistence and settled destruction. No Windows QML or physical device acceptance.')
save();print(json.dumps({p:result[p] for p in ['phase','exitCode','passed','expectedNegativeVerified','counts','diagnostics']},indent=2))
raise SystemExit(0 if result['passed'] or result['expectedNegativeVerified'] else 1)
