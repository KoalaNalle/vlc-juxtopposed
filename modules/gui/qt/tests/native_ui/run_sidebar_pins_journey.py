#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Bounded owned Windows sidebar-pin journeys; ordinary binary remains unchanged."""
import argparse
import ctypes
from hashlib import file_digest
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from threading import Thread


def digest(path):
    with path.open('rb') as stream:
        return file_digest(stream,'sha256').hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--package',type=Path,required=True)
    parser.add_argument('--qa-gui',type=Path,required=True)
    parser.add_argument('--fixtures',type=Path,required=True)
    parser.add_argument('--inputs',type=Path,required=True)
    parser.add_argument('--ordinary-inputs',type=Path,required=True)
    parser.add_argument('--output-dir',type=Path,required=True)
    parser.add_argument('--recent-stream',action='store_true',help='Exercise native Recent using an owned loopback HTTP media source')
    parser.add_argument('--modes',nargs='+',choices=['populate','reopen','disabled','replacement','history-off','history-off-optout','history-off-shutdown','titlebar-empty','titlebar-media','titlebar-video','titlebar-media-title','titlebar-close','titlebar-traditional'],default=['populate','reopen','disabled','replacement'])
    args=parser.parse_args()
    root=Path(__file__).resolve().parents[5]
    package,qa,fixtures,output=(path.resolve() for path in (args.package,args.qa_gui,args.fixtures,args.output_dir))
    assert os.name=='nt' and all(path.drive.upper()=='E:' for path in (root,package,qa,fixtures,output))
    assert output!=package and package not in output.parents
    manifests=[json.loads(path.read_text(encoding='utf-8-sig')) for path in (args.inputs,args.ordinary_inputs)]
    def inputs_match():return all(digest(root/name)==sha for manifest in manifests for name,sha in manifest.items())
    assert inputs_match(),'Changed compiled inputs'
    output.mkdir(parents=True,exist_ok=False)
    sys.path.insert(0,str(root/'test/libvlc'))
    from native_http_runtime import stage_runtime,environment,run_owned
    runtime=output/'runtime';stage_runtime(package,runtime)
    ordinary_sha=digest(package/'plugins/gui/libqt_plugin.dll')
    assert digest(qa)!=ordinary_sha
    with (package/'plugins/gui/libqt_plugin.dll').open('rb') as stream:
        tail=b''
        while chunk:=stream.read(1024*1024):
            content=tail+chunk;assert b'VLC_OWNED_UI_REPORT' not in content;tail=content[-128:]
    target=runtime/'plugins/gui/libqt_plugin.dll';target.unlink();os.link(qa,target)
    media=output/'Owned Pins Ää 日本語';media.mkdir()
    fixture_names=['first.flac','second.flac','owned-clip.mp4']
    fixture_hashes={name:digest(fixtures/name) for name in fixture_names}
    for name in fixture_names:shutil.copyfile(fixtures/name,media/name)
    configuration=output/'vlcrc'
    configuration.write_text('metadata-network-access=0\nml-network-discovery=0\none-instance=0\nqt-updates-notif=0\n',encoding='utf-8')
    userdata=output/'userdata';userdata.mkdir()
    env=environment(runtime,output)
    for name in ('QML_DISABLE_DISK_CACHE','QML_FORCE_DISK_CACHE','QT_QUICK_BACKEND'):env.pop(name,None)
    env.update(QML_DISK_CACHE='aot',QT_DISABLE_SHADER_DISK_CACHE='1',QT_QPA_PLATFORM='windows',
               HOME=str(userdata),USERPROFILE=str(userdata),VLC_USERDATA_PATH=str(userdata))
    cache=run_owned([runtime/'vlc-cache-gen.exe',runtime/'plugins'],runtime,output/'cache')
    assert cache['normalExit'],cache
    names=['libvlccore.dll','plugins/gui/libqt_plugin.dll','plugins/misc/libmedialibrary_plugin.dll']
    report=dict(passed=False,fullBetaAccepted=False,privateGuiOnly=True,ordinaryGuiSha256=ordinary_sha,
                runtimeSha256={name:digest(runtime/name) for name in names},cases={},runnerSha256=digest(Path(__file__)))
    def save():
        report['sourceInputsUnchanged']=inputs_match()
        report['ordinaryGuiUnchanged']=digest(package/'plugins/gui/libqt_plugin.dll')==ordinary_sha
        report['ownedMediaUnchanged']=all(digest(media/name)==sha and digest(fixtures/name)==sha for name,sha in fixture_hashes.items())
        (output/'verification.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    child=None
    server=None
    server_thread=None
    try:
        if args.recent_stream:
            class OwnedHandler(SimpleHTTPRequestHandler):
                def log_message(self,*args): pass
            server=ThreadingHTTPServer(('127.0.0.1',0),partial(OwnedHandler,directory=str(media)))
            server_thread=Thread(target=server.serve_forever,daemon=True)
            server_thread.start()
            env['VLC_OWNED_RECENT_URL']=f'http://127.0.0.1:{server.server_port}/first.flac'
        for mode in args.modes:
            case=output/mode;case.mkdir()
            data=userdata
            if mode in ('replacement','history-off-optout','history-off-shutdown') or mode.startswith('titlebar-'):data=output/(mode+'-userdata');data.mkdir()
            child_env=env|dict(VLC_USERDATA_PATH=str(data),VLC_OWNED_UI_REPORT=str(case/'journey.json'),
                VLC_OWNED_UI_OUTPUT=str(case),VLC_OWNED_UI_BASE=str(output),VLC_OWNED_UI_MODE=mode,
                VLC_OWNED_AUDIO_FOLDER=media.as_posix())
            command=[str(runtime/'vlc.exe'),'--intf=qt','--no-one-instance','--no-media-library' if mode=='disabled' or mode.startswith('history-off') else '--media-library',
                '--no-crashdump','--no-qt-privacy-ask','--no-qt-updates-notif','--no-metadata-network-access','--http-proxy=direct://',
                '--no-ml-network-discovery','--config='+str(configuration),'--file-logging','--log-verbose=2','--verbose=2',
                '--logfile='+str(case/'vlc.log'),'--no-qt-video-autoresize','--no-hw-dec','--no-sub-autodetect-file','--gain=0','--restore-playback-pos=0']
            if mode.startswith('titlebar-'):
                command+=['--qt-name-in-title' if mode=='titlebar-media-title' else '--no-qt-name-in-title']
                child_env['VLC_OWNED_TITLEBAR_ASSETS']=str(root/'build-windows/beta-shared-strip-20261018/assets-inventory.json')
            if mode=='titlebar-traditional':command+=['--no-qt-juxtopposed-controls']
            if mode=='history-off-optout':command+=['--no-save-recentplay','--no-remember-playback-pos']
            (case/'launch.json').write_text(json.dumps(command,indent=2)+'\n',encoding='utf-8')
            result=dict(normalExit=False,forcedCleanupUsed=False);report['cases'][mode]=result
            kernel=ctypes.WinDLL('kernel32');kernel.SetErrorMode.argtypes=[ctypes.c_uint];kernel.SetErrorMode.restype=ctypes.c_uint
            previous=kernel.SetErrorMode(0x8003)
            try:
                with (case/'stdout.txt').open('wb') as stdout,(case/'stderr.txt').open('wb') as stderr:
                    started=time.monotonic();child=subprocess.Popen(command,cwd=runtime,env=child_env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
                    result['pid']=child.pid
                    try:code=child.wait(timeout=180)
                    except subprocess.TimeoutExpired:
                        result['forcedCleanupUsed']=True;child.kill();code=child.wait(timeout=5)
                    result.update(exitCode=code,normalExit=code==0 and not result['forcedCleanupUsed'],elapsedSeconds=time.monotonic()-started)
            finally:kernel.SetErrorMode(previous)
            journey=case/'journey.json';result['journey']=json.loads(journey.read_text()) if journey.is_file() else {}
            progress=case/'progress.json'
            if not journey.is_file() and progress.is_file():result['partialJourney']=json.loads(progress.read_text())
            logs='\n'.join(path.read_text(encoding='utf-8',errors='replace') for path in (case/'vlc.log',case/'stderr.txt') if path.is_file())
            terms=('ReferenceError','TypeError','Binding loop','Cannot assign','Unable to assign','unregistered datatype','is not a type','failed to load component','Assertion failed','Signal uses unnamed parameter','SyntaxError','QSignalSpy:','SVGColorizer:','QFSFileEngine::open: No file name specified')
            result['diagnostics']=[line for line in logs.splitlines() if any(term in line for term in terms) or 'qt warning:' in line]
            if mode=='titlebar-close':assert result['journey'].get('nativeCloseTriggered'), 'Native Close affordance never reached the real CSD model'
            save()
            print(mode,'normalExit',result['normalExit'],'passed',result['journey'].get('passed'),'checks',len(result['journey'].get('checks',[])),'diagnostics',len(result['diagnostics']),flush=True)
            assert result['normalExit'] and result['journey'].get('passed') and not result['diagnostics'],mode
        save()
        assert report['sourceInputsUnchanged'] and report['ordinaryGuiUnchanged'] and report['ownedMediaUnchanged']
        report['passed']=True
    finally:
        if child is not None and child.poll() is None:child.kill();child.wait(timeout=5)
        if server is not None:
            server.shutdown();server.server_close()
            server_thread.join(timeout=3)
            assert not server_thread.is_alive(),'Owned loopback server did not terminate'
        save()
    return 0


if __name__=='__main__':raise SystemExit(main())
