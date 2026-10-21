#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Bounded owned Windows sidebar-pin journeys; ordinary binary remains unchanged."""
import argparse
import ctypes
from hashlib import file_digest
import json
import os
import re
from pathlib import Path
import shutil
import sqlite3
import subprocess
import sys
import time
import wave
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from threading import Thread, Event


def digest(path):
    with path.open('rb') as stream:
        return file_digest(stream,'sha256').hexdigest()


def favorite_failure_lock(case, userdata, stop, state):
    """Reserve only this fixture's SQLite writer; never execute a data mutation."""
    connection=None
    try:
        request=case/'favorite-failure-ready.json';deadline=time.monotonic()+45
        while not request.is_file():
            if stop.wait(.05) or time.monotonic()>deadline:
                raise RuntimeError('Owned app did not reach database-lock precondition')
        data=json.loads(request.read_text(encoding='utf-8'))
        database=Path(data['database']).resolve();media_id=int(data['mediaId'])
        assert database.is_relative_to(userdata.resolve()) and database.name=='ml.db' and media_id>0
        state.update(database=str(database),mediaId=media_id)
        connection=sqlite3.connect(database.as_uri()+'?mode=rw',uri=True,timeout=5)
        connection.execute('BEGIN IMMEDIATE')
        def favorite():
            row=connection.execute('SELECT is_favorite FROM Media WHERE id_media=?',(media_id,)).fetchone()
            assert row is not None and row[0] in (0,1),row
            return bool(row[0])
        state['before']=favorite();assert state['before'] is False
        (case/'favorite-failure-locked').write_text('ready\n',encoding='utf-8')
        observed=case/'favorite-failure-observed';deadline=time.monotonic()+35
        while not observed.is_file():
            if stop.wait(.05) or time.monotonic()>deadline:
                raise RuntimeError('Owned app did not report a failed native favorite write while lock was held')
        state['afterFailedWrite']=favorite();assert state['afterFailedWrite'] is False
        connection.rollback();connection.close();connection=None
        state['releasedNormally']=True
        (case/'favorite-failure-unlocked').write_text('ready\n',encoding='utf-8')
    except Exception as error:
        state['error']=str(error)
        (case/'favorite-failure-lock-error.json').write_text(json.dumps(state,indent=2)+'\n',encoding='utf-8')
    finally:
        if connection is not None:connection.rollback();connection.close()
        (case/'favorite-failure-lock.json').write_text(json.dumps(state,indent=2)+'\n',encoding='utf-8')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--package',type=Path,required=True)
    parser.add_argument('--qa-gui',type=Path,required=True)
    parser.add_argument('--fixtures',type=Path,required=True)
    parser.add_argument('--inputs',type=Path,required=True)
    parser.add_argument('--ordinary-inputs',type=Path,required=True)
    parser.add_argument('--output-dir',type=Path,required=True)
    parser.add_argument('--recent-stream',action='store_true',help='Exercise native Recent using an owned loopback HTTP media source')
    parser.add_argument('--screen-name',help='Keep native test windows on this exact Qt screen name')
    parser.add_argument('--startup-gate',action='store_true',help='Wait before each journey for its startup-approved marker after human/desktop inspection; no timeout while waiting')
    parser.add_argument('--locked-config-error',choices=['replacement','read-denial'],default='replacement',
        help='Exact intentional failure stage of the selected Core; later merging Core reads before replacement')
    parser.add_argument('--window-geometry-source',type=Path,help='Copy only saved window geometry from an owned INI fixture before startup')
    parser.add_argument('--controls-video-fixture',type=Path,help='Owned generated long video for auto-hide playback tests')
    parser.add_argument('--fullscreen-screen-index',type=int,help='Actual connected index of the approved monitor, checked again by the native driver')
    parser.add_argument('--modes',nargs='+',choices=['populate','reopen','disabled','replacement','history-off','history-off-optout','history-off-shutdown','titlebar-empty','titlebar-media','titlebar-video','titlebar-media-title','titlebar-close','titlebar-traditional','titlebar-window','titlebar-compact','titlebar-compact-audio','titlebar-dialogs','titlebar-recovery','titlebar-home-menu','titlebar-busy','titlebar-busy-playing','setup-cancel','setup-finish','setup-reopen','profiles-paused','profiles-playing','profiles-reopen'],default=['populate','reopen','disabled','replacement'])
    parser._option_string_actions['--modes'].choices += ['transport-initial','transport-all','transport-current','transport-none']
    parser._option_string_actions['--modes'].choices += ['customizer-paused','customizer-reopen','customizer-playing']
    parser._option_string_actions['--modes'].choices += ['favorite-initial','favorite-reopen','favorite-disabled','favorite-failure']
    parser._option_string_actions['--modes'].choices += ['playlist-origin','playlist-origin-disabled']
    parser._option_string_actions['--modes'].choices += ['fullscreen-controls-audio-on','fullscreen-controls-audio-off','fullscreen-controls-video-on','fullscreen-controls-video-off']
    parser._option_string_actions['--modes'].choices += ['settings-activation-paused','settings-activation-reopen','settings-activation-playing']
    parser._option_string_actions['--modes'].choices += ['tooltip-edge']
    parser._option_string_actions['--modes'].choices += ['audio-device-stale','audio-device-paused','audio-device-reopen','audio-device-playing','audio-device-unmanaged']
    parser._option_string_actions['--modes'].choices += ['interface-title-initial','interface-title-off','interface-title-restored']
    parser._option_string_actions['--modes'].choices += ['geometry-editor-initial','geometry-editor-off','geometry-editor-reopen','geometry-editor-playing']
    parser._option_string_actions['--modes'].choices += ['popup-editor-initial','popup-editor-reopen','popup-editor-playing',
        'popup-cold-max','popup-never','popup-minimized','popup-tray-off']
    parser._option_string_actions['--modes'].choices += ['geometry-snapshot-normal','geometry-snapshot-maximized',
        'geometry-snapshot-system-maximized','geometry-snapshot-minimized','geometry-snapshot-fullscreen','geometry-snapshot-cross-fullscreen']
    parser._option_string_actions['--modes'].choices += ['fullscreen-screen-initial','fullscreen-screen-explicit-playing',
        'fullscreen-screen-auto-video','fullscreen-screen-explicit-video','fullscreen-screen-stale']
    parser._option_string_actions['--modes'].choices += ['auto-hide-editor-initial','auto-hide-editor-reopen',
        'auto-hide-audio-never-pinned','auto-hide-audio-fullscreen-pinned','auto-hide-audio-always-pinned',
        'auto-hide-video-never-floating','auto-hide-video-fullscreen-floating','auto-hide-video-always-pinned',
        'auto-hide-audio-always-floating-playing','auto-hide-video-always-floating-playing',
        'auto-hide-audio-always-floating-zero','auto-hide-audio-always-floating-long']
    args=parser.parse_args()
    root=Path(__file__).resolve().parents[5]
    package,qa,fixtures,output=(path.resolve() for path in (args.package,args.qa_gui,args.fixtures,args.output_dir))
    assert os.name=='nt'
    build_root=root/'build-windows'
    assert all(path.is_relative_to(build_root) for path in (package,qa,fixtures,output)), 'Keep owned native inputs and outputs inside this checkout build directory'
    assert output!=package and package not in output.parents
    manifests=[json.loads(path.read_text(encoding='utf-8-sig')) for path in (args.inputs,args.ordinary_inputs)]
    def inputs_match():return all(digest(root/name)==sha for manifest in manifests for name,sha in manifest.items())
    assert inputs_match(),'Changed compiled inputs'
    output.mkdir(parents=True,exist_ok=False)
    sys.path.insert(0,str(root/'test/libvlc'))
    from native_http_runtime import stage_runtime,environment,run_owned
    runtime=output/'runtime';stage_runtime(package,runtime)
    if args.window_geometry_source:
        from PySide6.QtCore import QDataStream, QSettings
        source=args.window_geometry_source.resolve()
        assert source.is_relative_to(build_root) and source.is_file() and args.screen_name
        settings=QSettings(str(source),QSettings.IniFormat)
        geometry=settings.value('geometry')
        assert geometry is not None
        serialized=QDataStream(geometry);serialized.setVersion(QDataStream.Qt_4_0)
        assert serialized.readUInt32()==0x1D94200 and serialized.readUInt8()==1
        serialized.readUInt8()
        assert serialized.readQString()==args.screen_name,'Saved geometry belongs to another monitor'
        target_settings=QSettings(str(runtime/'portable/vlc/vlc-qt-interface.ini'),QSettings.IniFormat)
        target_settings.setValue('geometry',geometry);target_settings.sync()
        assert target_settings.status()==QSettings.NoError
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
    settings_modes=any(mode.startswith('settings-activation-') for mode in args.modes)
    device_modes=any(mode.startswith('audio-device-') for mode in args.modes)
    title_modes=any(mode.startswith('interface-title-') for mode in args.modes)
    auto_hide_modes=any(mode.startswith('auto-hide-') for mode in args.modes)
    screen_modes=any(mode.startswith('fullscreen-screen-') for mode in args.modes)
    geometry_modes=any(mode.startswith('geometry-editor-') for mode in args.modes)
    popup_modes=any(mode.startswith('popup-') for mode in args.modes)
    if settings_modes or device_modes or title_modes or auto_hide_modes or screen_modes or geometry_modes or popup_modes:
        prefix='popup-' if popup_modes else 'geometry-editor-' if geometry_modes else 'settings-activation-' if settings_modes else 'audio-device-' if device_modes else 'interface-title-' if title_modes else 'fullscreen-screen-' if screen_modes else 'auto-hide-'
        assert all(mode.startswith(prefix) for mode in args.modes), 'Keep settings persistence isolated'
        with wave.open(str(media/'owned-settings.wav'),'wb') as stream:
            stream.setnchannels(2);stream.setsampwidth(2);stream.setframerate(48000)
            stream.writeframes(b'\0'*4*48000*120)
    generated_hashes={'owned-settings.wav':digest(media/'owned-settings.wav')} if settings_modes or device_modes or title_modes or auto_hide_modes or screen_modes or geometry_modes or popup_modes else {}
    if popup_modes:
        for name in ('owned-popup-next.wav','owned-popup-restored.wav'):
            shutil.copyfile(media/'owned-settings.wav',media/name)
            generated_hashes[name]=digest(media/name)
    generated_video_source=None
    if auto_hide_modes or screen_modes:
        assert args.controls_video_fixture is not None,'Auto-hide journeys require their owned generated long video'
        generated_video_source=args.controls_video_fixture.resolve()
        assert generated_video_source.is_relative_to(build_root) and generated_video_source.is_file()
        shutil.copyfile(generated_video_source,media/'owned-controls.mp4')
        generated_hashes['owned-controls.mp4']=digest(generated_video_source)
    configuration=output/'vlcrc'
    configuration.write_text('metadata-network-access=0\nml-network-discovery=0\none-instance=0\nqt-updates-notif=0\n',encoding='utf-8')
    if settings_modes:
        with configuration.open('a',encoding='utf-8') as stream:stream.write('qt-fs-controller=1\nvolume-step=6.4\n')
    if device_modes:
        initial_device='owned-disconnected-endpoint' if 'audio-device-stale' in args.modes else ''
        with configuration.open('a',encoding='utf-8') as stream:
            stream.write('aout=mmdevice\nmmdevice-backend=wasapi,none\nvolume-save=1\nmmdevice-volume=1\nmmdevice-audio-device='+initial_device+'\n')
    if title_modes:
        with configuration.open('a',encoding='utf-8') as stream:stream.write('qt-name-in-title=1\n')
    if geometry_modes:
        with configuration.open('a',encoding='utf-8') as stream:stream.write('qt-remember-window-geometry=1\nqt-controls-auto-hide=0\n')
    if popup_modes:
        with configuration.open('a',encoding='utf-8') as stream:stream.write('qt-system-tray=1\nqt-start-minimized=0\nqt-notification=2\nqt-notification-duration=777\nqt-auto-raise=0\nqt-controls-auto-hide=0\n')
    if auto_hide_modes:
        with configuration.open('a',encoding='utf-8') as stream:stream.write('qt-controls-auto-hide=0\nqt-controls-hide-timeout=1000\nqt-fs-controller=1\n')
    if screen_modes:
        assert args.fullscreen_screen_index is not None and args.fullscreen_screen_index>=0
        with configuration.open('a',encoding='utf-8') as stream:stream.write('qt-fullscreen-screennumber=-1\nqt-controls-auto-hide=0\nqt-fs-controller=1\nembedded-video=1\n')
    if any(mode.startswith('setup-') for mode in args.modes):
        assert all(mode.startswith('setup-') for mode in args.modes), 'Keep setup persistence isolated from unrelated pin cases'
        # Owned, already-decided fixture policy, not a human consent UI action.
        with configuration.open('a',encoding='utf-8') as stream:stream.write('qt-privacy-ask=0\n')
    userdata=output/'userdata';userdata.mkdir()
    env=environment(runtime,output)
    # The executable/DLL already locates this staged plugins folder on Windows.
    # Adding the same folder through VLC_PLUGIN_PATH loads autorun modules twice.
    env.pop('VLC_PLUGIN_PATH',None)
    for name in ('QML_DISABLE_DISK_CACHE','QML_FORCE_DISK_CACHE','QT_QUICK_BACKEND'):env.pop(name,None)
    env.update(QML_DISK_CACHE='aot',QT_DISABLE_SHADER_DISK_CACHE='1',QT_QPA_PLATFORM='windows',QT_FORCE_STDERR_LOGGING='1',QT_MESSAGE_PATTERN='OWNED_QT_DIAG|%{type}|%{message}',
               HOME=str(userdata),USERPROFILE=str(userdata),VLC_USERDATA_PATH=str(userdata))
    if args.screen_name:env['VLC_OWNED_UI_SCREEN']=args.screen_name
    if popup_modes:env.update(QT_LOGGING_RULES='qt.qpa.trayicon.debug=true',QT_MESSAGE_PATTERN='OWNED_QT_DIAG|%{type}|%{category}|%{message}')
    cache=run_owned([runtime/'vlc-cache-gen.exe',runtime/'plugins'],runtime,output/'cache')
    assert cache['normalExit'],cache
    names=['libvlccore.dll','plugins/gui/libqt_plugin.dll','plugins/misc/libmedialibrary_plugin.dll']
    report=dict(passed=False,fullBetaAccepted=False,privateGuiOnly=True,ordinaryGuiSha256=ordinary_sha,
                runtimeSha256={name:digest(runtime/name) for name in names},cases={},runnerSha256=digest(Path(__file__)))
    def save():
        report['sourceInputsUnchanged']=inputs_match()
        report['ordinaryGuiUnchanged']=digest(package/'plugins/gui/libqt_plugin.dll')==ordinary_sha
        report['ownedMediaUnchanged']=all(digest(media/name)==sha and digest(fixtures/name)==sha for name,sha in fixture_hashes.items())
        report['ownedGeneratedMediaUnchanged']=all(digest(media/name)==sha for name,sha in generated_hashes.items())
        if generated_video_source:report['ownedGeneratedVideoSourceUnchanged']=digest(generated_video_source)==generated_hashes['owned-controls.mp4']
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
            case_configuration=configuration
            if mode in ('popup-editor-playing','popup-never','popup-minimized','popup-tray-off'):
                case_configuration=case/'vlcrc'
                policy=0 if mode=='popup-never' else 1 if mode=='popup-minimized' else 2
                tray=0 if mode=='popup-tray-off' else 1
                case_configuration.write_text('metadata-network-access=0\nml-network-discovery=0\none-instance=0\nqt-updates-notif=0\nqt-start-minimized=0\nqt-auto-raise=0\nqt-controls-auto-hide=0\n'
                    +f'qt-system-tray={tray}\nqt-notification={policy}\nqt-notification-duration=777\n',encoding='utf-8')
            if mode=='geometry-editor-playing':
                case_configuration=case/'vlcrc'
                case_configuration.write_text('metadata-network-access=0\nml-network-discovery=0\none-instance=0\nqt-updates-notif=0\nqt-remember-window-geometry=1\nqt-controls-auto-hide=0\n',encoding='utf-8')
            geometry_before=None
            if mode.startswith('geometry-editor-'):
                from PySide6.QtCore import QSettings
                geometry_settings=QSettings(str(runtime/'portable/vlc/vlc-qt-interface.ini'),QSettings.IniFormat)
                geometry_settings.sync();geometry_before=bytes(geometry_settings.value('geometry'))
            if mode=='audio-device-unmanaged':
                case_configuration=case/'vlcrc'
                case_configuration.write_text('metadata-network-access=0\nml-network-discovery=0\naout=adummy\nvolume-save=1\n',encoding='utf-8')
            if mode.startswith('auto-hide-') and not mode.startswith('auto-hide-editor-'):
                case_configuration=case/'vlcrc'
                policy=0 if '-never-' in mode else 1 if '-fullscreen-' in mode else 2
                delay=0 if mode.endswith('-zero') else 10000 if mode.endswith('-long') else 1000
                case_configuration.write_text('metadata-network-access=0\nml-network-discovery=0\none-instance=0\nqt-updates-notif=0\nqt-fs-controller=1\n'
                    +f'qt-controls-auto-hide={policy}\nqt-controls-hide-timeout={delay}\n',encoding='utf-8')
            if mode in ('fullscreen-screen-explicit-video','fullscreen-screen-stale'):
                case_configuration=case/'vlcrc'
                index=999 if mode.endswith('-stale') else args.fullscreen_screen_index
                case_configuration.write_text('metadata-network-access=0\nml-network-discovery=0\none-instance=0\nqt-updates-notif=0\nqt-controls-auto-hide=0\nqt-fs-controller=1\nembedded-video=1\n'
                    +f'qt-fullscreen-screennumber={index}\n',encoding='utf-8')
            data=userdata
            if mode in ('replacement','history-off-optout','history-off-shutdown','favorite-disabled','favorite-failure','playlist-origin-disabled') or mode.startswith('titlebar-'):data=output/(mode+'-userdata');data.mkdir()
            child_env=env|dict(VLC_USERDATA_PATH=str(data),VLC_OWNED_UI_REPORT=str(case/'journey.json'),
                VLC_OWNED_UI_OUTPUT=str(case),VLC_OWNED_UI_BASE=str(output),VLC_OWNED_UI_MODE=mode,
                VLC_OWNED_AUDIO_FOLDER=media.as_posix(),VLC_OWNED_SIDEBAR_ASSETS=str(root/'doc/juxtopposed-references/figma/sidebar-icons-20261018/assets-inventory.json'))
            startup_gate=case/'startup-approved'
            if args.startup_gate:child_env['VLC_OWNED_UI_START_GATE']=str(startup_gate)
            if mode.startswith('setup-'):child_env['VLC_OWNED_SETUP_CONFIG']=str(configuration)
            if mode.startswith(('settings-activation-','audio-device-','interface-title-','auto-hide-','fullscreen-screen-','geometry-editor-','popup-')):child_env['VLC_OWNED_SETTINGS_CONFIG']=str(case_configuration)
            command=[str(runtime/'vlc.exe'),'--intf=qt','--no-one-instance','--no-media-library' if mode in ('disabled','favorite-disabled','playlist-origin-disabled') or mode.startswith('history-off') else '--media-library',
                '--no-crashdump','--no-qt-privacy-ask','--no-qt-updates-notif','--no-metadata-network-access','--http-proxy=direct://',
                '--no-ml-network-discovery','--config='+str(case_configuration),'--file-logging','--log-verbose=2','--verbose=2',
                '--logfile='+str(case/'vlc.log'),'--no-qt-video-autoresize','--no-hw-dec','--no-sub-autodetect-file','--gain=0','--restore-playback-pos=0']
            if mode.startswith('titlebar-'):
                command+=['--qt-name-in-title' if mode=='titlebar-media-title' else '--no-qt-name-in-title']
                child_env['VLC_OWNED_TITLEBAR_ASSETS']=str(root/'doc/juxtopposed-references/native/guest-shared-strip-20261018/assets-inventory.json')
            if mode=='titlebar-traditional':command+=['--no-qt-juxtopposed-controls']
            if mode=='history-off-optout':command+=['--no-save-recentplay','--no-remember-playback-pos']
            if mode.startswith('fullscreen-controls-'):
                command+=['--qt-fs-controller' if mode.endswith('-on') else '--no-qt-fs-controller']
            (case/'launch.json').write_text(json.dumps(command,indent=2)+'\n',encoding='utf-8')
            result=dict(normalExit=False,forcedCleanupUsed=False);report['cases'][mode]=result
            kernel=ctypes.WinDLL('kernel32');kernel.SetErrorMode.argtypes=[ctypes.c_uint];kernel.SetErrorMode.restype=ctypes.c_uint
            previous=kernel.SetErrorMode(0x8003)
            lock_stop=Event();lock_state={};lock_thread=None
            try:
                with (case/'stdout.txt').open('wb') as stdout,(case/'stderr.txt').open('wb') as stderr:
                    started=time.monotonic();child=subprocess.Popen(command,cwd=runtime,env=child_env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
                    result['pid']=child.pid
                    if args.startup_gate:
                        result['startupApprovalPending']=True;save()
                        print(mode,'waiting for desktop inspection:',startup_gate,flush=True)
                        while child.poll() is None and not startup_gate.is_file():time.sleep(.25)
                        result.update(startupApprovalPending=False,startupApproved=startup_gate.is_file(),startupWaitSeconds=time.monotonic()-started)
                        save()
                    if mode=='favorite-failure' and child.poll() is None:
                        lock_thread=Thread(target=favorite_failure_lock,args=(case,data,lock_stop,lock_state),daemon=True)
                        lock_thread.start()
                    try:code=child.wait(timeout=180)
                    except subprocess.TimeoutExpired:
                        result['forcedCleanupUsed']=True;child.kill();code=child.wait(timeout=5)
                    result.update(exitCode=code,normalExit=code==0 and not result['forcedCleanupUsed'],elapsedSeconds=time.monotonic()-started)
            finally:
                kernel.SetErrorMode(previous)
                lock_stop.set()
                if lock_thread is not None:
                    lock_thread.join(timeout=10)
                    assert not lock_thread.is_alive(),'Owned SQLite lock helper did not terminate'
                    result['databaseLock']=lock_state
            if mode=='favorite-failure' and lock_state.get('releasedNormally'):
                with sqlite3.connect(Path(lock_state['database']).as_uri()+'?mode=ro',uri=True) as reader:
                    row=reader.execute('SELECT is_favorite FROM Media WHERE id_media=?',(lock_state['mediaId'],)).fetchone()
                result['databaseLock']['afterRetry']=bool(row[0]) if row else None
            journey=case/'journey.json';result['journey']=json.loads(journey.read_text()) if journey.is_file() else {}
            progress=case/'progress.json'
            if not journey.is_file() and progress.is_file():result['partialJourney']=json.loads(progress.read_text())
            logs='\n'.join(path.read_text(encoding='utf-8',errors='replace') for path in (case/'vlc.log',case/'stderr.txt') if path.is_file())
            terms=('ReferenceError','TypeError','Binding loop','Cannot assign','Unable to assign','unregistered datatype','is not a type','failed to load component','Assertion failed','Signal uses unnamed parameter','SyntaxError','QSignalSpy:','SVGColorizer:','QFSFileEngine::open: No file name specified')
            result['diagnostics']=[line for line in logs.splitlines() if any(term in line for term in terms) or 'qt warning:' in line or any('OWNED_QT_DIAG|'+level+'|' in line for level in ('warning','critical','fatal'))]
            result['nativeErrors']=[line for line in logs.splitlines() if re.search(r'\berror:',line)]
            result['expectedNativeErrors']=[]
            result['unexpectedNativeErrors']=result['nativeErrors']
            if mode in ('settings-activation-paused','settings-activation-playing','audio-device-paused','audio-device-playing','interface-title-initial','auto-hide-editor-initial','fullscreen-screen-initial','geometry-editor-initial','popup-editor-initial'):
                # Exactly one deliberate locked-config write is part of this
                # journey. Retain its log; never excuse an unrelated failure.
                pattern=(r'\bmain error: cannot read configuration file: Permission denied$'
                    if args.locked_config_error=='read-denial' else
                    r'\bmain error: cannot replace configuration file \(Windows error (?:5|32)\)$')
                result['lockedConfigFailureStage']=args.locked_config_error
                expected=[line for line in result['nativeErrors'] if re.search(pattern,line)]
                result['expectedNativeErrors']=expected
                result['unexpectedNativeErrors']=[line for line in result['nativeErrors'] if line not in expected]
                failure_check='real locked-save failure rolls back policy and exposes retry feedback'
                assert len(expected)==1 and any(row.get('name')==failure_check and row.get('passed')
                    for row in result['journey'].get('checks',[])), 'Missing bounded config-sharing negative control'
            elif mode=='audio-device-stale':
                # This owned synthetic ID is deliberately rejected during
                # MMDevice startup. Require its exact diagnostic and actual
                # unselected/explicit-repair UI proof; retain every other error.
                expected=[line for line in result['nativeErrors'] if line==
                    'mmdevice error: cannot get selected device owned-disconnected-endpoint (error 0x80070057)']
                required={'actual device combo leaves a missing saved endpoint unselected',
                          'actual System default selection repairs the saved endpoint with restart feedback'}
                actual={row.get('name') for row in result['journey'].get('checks',[]) if row.get('passed')}
                assert len(expected)==1 and required.issubset(actual),'Missing bounded stale-endpoint negative control'
                result['expectedNativeErrors']=expected
                result['unexpectedNativeErrors']=[line for line in result['nativeErrors'] if line not in expected]
            result['nativeWarnings']=[line for line in logs.splitlines() if re.search(r'\bwarning:',line)]
            result['pluginModuleCounts']=re.findall(r'plug-ins loaded: (\d+) modules',logs)
            if mode=='titlebar-close':assert result['journey'].get('nativeCloseTriggered'), 'Native Close affordance never reached the real CSD model'
            if mode in ('titlebar-window','titlebar-compact','titlebar-compact-audio'):
                user32=ctypes.WinDLL('user32')
                user32.GetAsyncKeyState.argtypes=[ctypes.c_int];user32.GetAsyncKeyState.restype=ctypes.c_short
                result['nativeLeftButtonReleased']=(user32.GetAsyncKeyState(1)&0x8000)==0
            save()
            print(mode,'normalExit',result['normalExit'],'passed',result['journey'].get('passed'),'checks',len(result['journey'].get('checks',[])),'diagnostics',len(result['diagnostics']),flush=True)
            if mode=='favorite-failure':
                assert result['journey'].get('nativeFavoriteFailureAccepted') and lock_state.get('releasedNormally') and 'error' not in lock_state
                assert lock_state['before'] is False and lock_state['afterFailedWrite'] is False and lock_state.get('afterRetry') is True
            assert result.get('nativeLeftButtonReleased',True), 'Owned native drag left a pressed OS mouse button'
            if mode.startswith(('fullscreen-controls-','playlist-origin','settings-activation-','tooltip-edge','audio-device-','interface-title-','auto-hide-','fullscreen-screen-','geometry-snapshot-','geometry-editor-','popup-')):
                assert not result['unexpectedNativeErrors'],(mode,result['unexpectedNativeErrors'])
            if mode.startswith('settings-activation-'):
                assert result['journey'].get('nativeSettingsActivationAccepted'),mode
            if mode=='tooltip-edge':
                assert result['journey'].get('nativeTooltipEdgeAccepted'),mode
            if mode.startswith('audio-device-'):
                assert result['journey'].get('nativeAudioDeviceSettingsAccepted'),mode
            if mode.startswith('interface-title-'):
                assert result['journey'].get('nativeInterfaceTitleAccepted'),mode
            if mode.startswith('auto-hide-'):
                assert result['journey'].get('nativeAutoHideAccepted'),mode
            if mode.startswith('fullscreen-screen-'):
                assert result['journey'].get('nativeFullscreenScreenAccepted'),mode
            if mode.startswith('geometry-snapshot-'):
                assert result['journey'].get('nativeGeometrySnapshotAccepted'),mode
            if mode.startswith('popup-'):
                assert result['journey'].get('nativePopupDurationAccepted'),mode
                deliveries=re.findall(r'showMessage \( "VLC media player" "([^"]+)" .*? QPlatformSystemTrayIcon::NoIcon (\d+) \)',logs)
                expected=result['journey']['expectedNotificationNames']
                duration=result['journey']['startupNotificationDuration']
                result['platformDeliveries']=[dict(name=name,durationHintMs=int(ms)) for name,ms in deliveries]
                result['platformNotificationNamesMatch']=[name for name,ms in deliveries]==expected
                result['platformDurationHintsMatch']=all(int(ms)==duration for name,ms in deliveries)
                values=re.findall(r'^#?qt-notification-duration=(.*)$',case_configuration.read_text(encoding='utf-8'),re.M)
                result['independentSavedDuration']=int(values[0]) if len(values)==1 else None
                assert result['platformNotificationNamesMatch'] and result['platformDurationHintsMatch'],(mode,deliveries,expected,duration)
                assert result['independentSavedDuration']==result['journey'].get('savedNextStartupDuration',duration),(mode,values)
            if mode.startswith('geometry-editor-'):
                assert result['journey'].get('nativeGeometryEditorAccepted'),mode
                from PySide6.QtCore import QSettings,QDataStream,QRect,QPoint
                geometry_settings.sync();after=bytes(geometry_settings.value('geometry'))
                result['savedGeometryBeforeSha256']=__import__('hashlib').sha256(geometry_before).hexdigest()
                result['savedGeometryAfterSha256']=__import__('hashlib').sha256(after).hexdigest()
                if mode=='geometry-editor-off':
                    result['offStartupPreservesSavedGeometry']=after==geometry_before
                    assert result['offStartupPreservesSavedGeometry'],'Off startup unexpectedly saved moved/resized geometry'
                else:
                    data=QDataStream(geometry_settings.value('geometry'));data.setVersion(QDataStream.Qt_4_0)
                    assert data.readUInt32()==0x1D94200 and data.readUInt8()==1
                    data.readUInt8();data.readQString();data.readDouble()
                    screen_rect=QRect();position=QPoint();client=QRect();data>>screen_rect>>position>>client
                    result['onStartupSavesExitGeometry']=[client.x(),client.y(),client.width(),client.height()]==result['journey']['exitGeometry']
                    assert result['onStartupSavesExitGeometry'],'On startup did not save actual exit geometry'
            assert result['normalExit'] and result['journey'].get('passed') and not result['diagnostics'],mode
        save()
        assert report['sourceInputsUnchanged'] and report['ordinaryGuiUnchanged'] and report['ownedMediaUnchanged'] and report['ownedGeneratedMediaUnchanged']
        assert report.get('ownedGeneratedVideoSourceUnchanged',True)
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
