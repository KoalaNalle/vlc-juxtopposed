from pathlib import Path
from hashlib import sha256
import ctypes, json, os, re, shutil, subprocess, sys, time

work = Path(__file__).resolve().parent
repo = work.parent.parent
out = work / 'ui-current-hover-r7'
assert not out.exists()
read = lambda p: json.loads(p.read_text(encoding='utf-8-sig'))
digest = lambda p: sha256(p.read_bytes()).hexdigest()
def write(p, value):
    p.write_text(json.dumps(value, indent=2)+'\n', encoding='utf-8', newline='\n')

sys.path.insert(0, str(repo / 'test/libvlc'))
from native_http_runtime import stage_runtime, environment, regenerate_cache
accepted = read(work / 'final-verification-r6.json')
assert accepted['passed'] and accepted['selectedRepairAccepted']
base = repo / accepted['runtime']
assert all(digest(base / n) == h for n, h in accepted['runtimeSha256'].items())
source_names = set(read(work / 'inputs-ordinary-r3.json')['files'])
source_names.add(Path(__file__).relative_to(repo).as_posix())
inputs = {n:digest(repo / n) for n in sorted(source_names)}
snapshot = work / 'source-current-hover-r7'
assert not snapshot.exists()
snapshot.mkdir()
for n, h in inputs.items():
    destination = snapshot / n
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(repo / n, destination)
    assert digest(destination) == h
manifest = work / 'inputs-current-hover-r7.json'
write(manifest, dict(files=inputs, sourceCount=len(inputs), sourceSnapshot=str(snapshot), fullBetaAccepted=False))
media = repo / 'build-windows/beta-timeline-thumbnails-20261021/resource-fixtures-r45/motion-1080p50-silent.mp4'
assert media.is_file()
media_hash = digest(media)
out.mkdir()
runtime = out / 'runtime'
stage_runtime(base, runtime)
identities = {n:digest(runtime / n) for n in accepted['runtimeSha256']}
assert identities == accepted['runtimeSha256']
gui = (runtime / 'plugins/gui/libqt_plugin.dll').read_bytes()
assert b'VLC_OWNED_UI_REPORT' not in gui and b'VLC_OWNED_HOVER_TRACE' not in gui
regenerate_cache(runtime, out / 'cache')
env = environment(runtime, out)
env.pop('VLC_PLUGIN_PATH', None)
env.pop('QT_D3D_MAX_FRAME_LATENCY', None)
env.update(QT_LOGGING_RULES='qt.rhi.general=true;qt.quick.handler.hover.debug=true;qt.quick.focus.debug=true',
           QT_FORCE_STDERR_LOGGING='1',
           QT_MESSAGE_PATTERN='OWNED_QT_DIAG|%{type}|%{category}|%{time process}|%{message}')
config = out / 'vlcrc'
config.write_text('metadata-network-access=0\nml-network-discovery=0\none-instance=0\nqt-updates-notif=0\nqt-auto-raise=0\nqt-remember-window-geometry=0\nqt-controls-auto-hide=0\nqt-timeline-thumbnails=1\n', encoding='utf-8', newline='\n')
ini = runtime / 'portable/vlc/vlc-qt-interface.ini'
ini.parent.mkdir(parents=True, exist_ok=True)
ini.write_text('[General]\ngraphics-api=4\n[MainWindow]\ncolor-scheme-index=2\ninterface-scale=1\n', encoding='utf-8', newline='\n')
command = [str(runtime / 'vlc.exe'), '--intf=qt', '--no-one-instance', '--no-media-library', '--no-crashdump',
           '--no-qt-privacy-ask', '--no-qt-updates-notif', '--no-metadata-network-access', '--no-ml-network-discovery',
           '--config='+str(config), '--file-logging', '--log-verbose=2', '--verbose=2', '--logfile='+str(out / 'vlc.log'),
           '--no-qt-video-autoresize', '--restore-playback-pos=0', '--start-paused', '--fullscreen',
           '--vout=direct3d11,none', str(media)]
result = dict(status='starting', passed=False, privateCore=False, privateGui=False, privateObserver=False,
              fullBetaAccepted=False, forcedCleanupUsed=False, sourceManifestSha256=digest(manifest),
              sourceInputCount=len(inputs), runtimeSha256=identities, fixture=str(media), fixtureSha256=media_hash,
              runnerSha256=digest(Path(__file__)), startedAtUtc=time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()),
              scope='Current ordinary tuple: visible paused preview hold with built-in named Qt hover/focus logging, deliberate exit and reentry, playback resume and normal quit. Original historical loss attribution and continuous pixel recording remain unqualified.')
k = ctypes.WinDLL('kernel32', use_last_error=True)
k.SetErrorMode.argtypes = [ctypes.c_uint]
k.SetErrorMode.restype = ctypes.c_uint
previous = k.SetErrorMode(0x8003)
try:
    with (out / 'stdout.txt').open('wb') as stdout, (out / 'stderr.txt').open('wb') as stderr:
        child = subprocess.Popen(command, cwd=runtime, env=env, stdout=stdout, stderr=stderr, creationflags=subprocess.CREATE_NO_WINDOW)
        result.update(status='running', pid=child.pid)
        write(out / 'verification.json', result)
        print('Owned ordinary current-hover PID', child.pid, flush=True)
        try:
            result['exitCode'] = child.wait(timeout=1200)
        except subprocess.TimeoutExpired:
            result.update(status='observation-timeout-process-left-running')
            write(out / 'verification.json', result)
            raise SystemExit('Inspect saved PID; no automatic cleanup/restart')
finally:
    k.SetErrorMode(previous)
stderr = (out / 'stderr.txt').read_text(encoding='utf-8', errors='replace')
pattern = r'OWNED_QT_DIAG\|debug\|qt.quick.handler.hover\|([^|]*)\|"timelineHoverHandler" hovered (true|false) -> (true|false)'
transitions = [dict(qtProcessTime=m.group(1).strip(), before=m.group(2)=='true', after=m.group(3)=='true') for m in re.finditer(pattern, stderr)]
logs = '\n'.join((out / n).read_text(encoding='utf-8', errors='replace') for n in ['vlc.log','stderr.txt'])
result.update(status='terminal', normalExit=result['exitCode']==0,
              sourceInputsUnchanged=all(digest(repo / n)==h for n,h in inputs.items()),
              runtimeUnchanged=all(digest(runtime / n)==h for n,h in identities.items()),
              fixtureUnchanged=digest(media)==media_hash, hoverTransitions=transitions,
              deactivations=[s for s in stderr.splitlines() if '|qt.quick.focus|' in s and 'deactivated' in s],
              realD3D11Opened='using vout display module "direct3d11"' in logs,
              diagnostics=[s for s in logs.splitlines() if re.search(r'\berror:|OWNED_QT_DIAG\|(?:warning|critical|fatal)\||Assertion failed|Binding loop|ReferenceError|TypeError',s)])
write(out / 'verification.json', result)
print(json.dumps({k:result[k] for k in ['status','pid','normalExit','sourceInputsUnchanged','runtimeUnchanged','fixtureUnchanged','diagnostics','hoverTransitions']}), flush=True)
