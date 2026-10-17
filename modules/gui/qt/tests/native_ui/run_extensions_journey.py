#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Owned native Extensions model/page, file chooser and restart journeys."""
from pathlib import Path
import argparse,ctypes,hashlib,json,os,subprocess,sys,time,zipfile
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--package',type=Path,required=True)
parser.add_argument('--qa-gui',type=Path,required=True)
parser.add_argument('--output-dir',type=Path,required=True)
parser.add_argument('--modes',nargs='+',choices=['empty','edit','reopen','startup-on','override-off','live'],default=['empty','edit','reopen','startup-on','override-off'])
args=parser.parse_args();ordinary=args.package.resolve();qa=args.qa_gui.resolve();output=args.output_dir.resolve()
assert ordinary.drive.upper()==qa.drive.upper()==output.drive.upper()
output.mkdir(parents=True,exist_ok=False)
root=Path(__file__).resolve().parents[5]
sys.path.insert(0,str(root/'test/libvlc'))
from native_http_runtime import stage_runtime,environment as owned_environment,run_owned
runtime=output/'runtime';stage_runtime(ordinary,runtime)
target=runtime/'plugins/gui/libqt_plugin.dll';target.unlink();os.link(qa,target)
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
ordinary_sha=digest(ordinary/'plugins/gui/libqt_plugin.dll')
assert digest(qa)!=ordinary_sha
assert b'VLC_OWNED_UI_REPORT' not in (ordinary/'plugins/gui/libqt_plugin.dll').read_bytes()
configuration=output/'vlcrc'
configuration.write_text('metadata-network-access=0\nml-network-discovery=0\none-instance=0\nqt-updates-notif=0\n',encoding='utf-8',newline='\n')
userdata=output/'userdata';(userdata/'lua/extensions').mkdir(parents=True)
script='''function descriptor() return {title="%s",version="1.0",author="Owned test",shortdesc="%s",description="Owned fixture",capabilities={}} end
function activate() end
function deactivate() end
'''
for name,title in [('Alpha.lua','Owned startup marker'),('Zulu.lua','Owned second marker')]:
    (userdata/'lua/extensions'/name).write_text(script%(title,title),encoding='utf-8',newline='\n')
payload=(script%('Owned installed package','Owned installed package')).encode('utf-8')
package=output/'Package Ää 日本語.VLP'
manifest='''<?xml version="1.0" encoding="UTF-8"?><videolan><addons><addon type="extension" id="8beaeec8-7b16-45f9-83c0-a0ecc57d0401" version="1.0"><name>Aardvark beta fixture</name><creator>Owned test</creator><summary>Owned local package</summary><description>Owned fixture</description><resource type="extension">owned-beta-fixture.lua</resource></addon></addons></videolan>'''
with zipfile.ZipFile(package,'w') as archive:
    archive.writestr('manifest.xml',manifest);archive.writestr('owned-beta-fixture.lua',payload)
env=owned_environment(runtime,output)
for key in ('QML_DISABLE_DISK_CACHE','QML_FORCE_DISK_CACHE','QT_QUICK_BACKEND'):env.pop(key,None)
env.update(QT_ENABLE_REGEXP_JIT='1',QML_DISK_CACHE='aot',QT_DISABLE_SHADER_DISK_CACHE='1',QT_QPA_PLATFORM='windows',
    HOME=str(userdata),USERPROFILE=str(userdata),VLC_USERDATA_PATH=str(userdata))
cache=run_owned([runtime/'vlc-cache-gen.exe',runtime/'plugins'],runtime,output/'cache')
assert cache['normalExit']
report=dict(passed=False,ordinaryGuiUnchanged=False,privateGuiOnly=True,cases={},fullBetaAccepted=False,
    runtimeSha256={name:digest(runtime/name) for name in ['libvlccore.dll','plugins/gui/libqt_plugin.dll','plugins/misc/libaddonsfsstorage_plugin.dll','plugins/misc/libaddonsvorepository_plugin.dll']},
    qaSourceSha256=digest(Path(__file__).with_name('extensions_journey.cpp')),ordinaryGuiSha256=ordinary_sha)
def save():(output/'verification.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8',newline='\n')
child=None
try:
    for mode in args.modes:
        case=output/mode;case.mkdir()
        data=output/'empty-userdata' if mode=='empty' else userdata;data.mkdir(exist_ok=True)
        child_env=env|dict(VLC_USERDATA_PATH=str(data),VLC_OWNED_UI_REPORT=str(case/'journey.json'),
            VLC_OWNED_UI_OUTPUT=str(case),VLC_OWNED_UI_BASE=str(output),VLC_OWNED_UI_CONFIG=str(configuration),
            VLC_OWNED_UI_MODE=mode,VLC_OWNED_ADDONS_PACKAGE=str(package))
        command=[str(runtime/'vlc.exe'),'--intf=qt','--no-one-instance','--no-media-library','--no-crashdump',
            '--no-qt-privacy-ask','--no-qt-updates-notif','--no-metadata-network-access','--http-proxy=direct://',
            '--no-ml-network-discovery','--config='+str(configuration),'--file-logging','--log-verbose=2','--verbose=2','--logfile='+str(case/'vlc.log')]
        if mode=='override-off':command.append('--no-qt-autoload-extensions')
        (case/'launch.json').write_text(json.dumps(command,indent=2)+'\n',encoding='utf-8')
        result=dict(normalExit=False,forcedCleanupUsed=False);report['cases'][mode]=result
        kernel=ctypes.WinDLL('kernel32');kernel.SetErrorMode.argtypes=[ctypes.c_uint];kernel.SetErrorMode.restype=ctypes.c_uint
        previous=kernel.SetErrorMode(0x8003)
        try:
            with (case/'stdout.txt').open('wb') as stdout,(case/'stderr.txt').open('wb') as stderr:
                started=time.monotonic();child=subprocess.Popen(command,cwd=runtime,env=child_env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
                result['pid']=child.pid
                try:code=child.wait(timeout=100)
                except subprocess.TimeoutExpired:
                    result['forcedCleanupUsed']=True;child.kill();code=child.wait(timeout=5)
                result.update(exitCode=code,normalExit=code==0 and not result['forcedCleanupUsed'],elapsedSeconds=time.monotonic()-started)
        finally:kernel.SetErrorMode(previous)
        path=case/'journey.json';journey=json.loads(path.read_text()) if path.is_file() else {}
        result['journey']=journey
        late=case/'late-window-event.json';result['lateWindowEvent']=json.loads(late.read_text()) if late.exists() else None
        logs='\n'.join(p.read_text(encoding='utf-8',errors='replace') for p in [case/'vlc.log',case/'stderr.txt'] if p.is_file())
        terms=('ReferenceError','TypeError','Binding loop','Cannot assign','Unable to assign','unregistered datatype','is not a type','failed to load component','Assertion failed')
        result['diagnostics']=[line for line in logs.splitlines() if any(term in line for term in terms)]
        save();print(mode,'normal exit',result['normalExit'],'passed',journey.get('passed'),'checks',len(journey.get('checks',[])),'diagnostics',len(result['diagnostics']),flush=True)
        assert result['normalExit'] and journey.get('passed') and not result['diagnostics'],mode
        assert result['lateWindowEvent'] is not None,'MainCtx destruction probe did not run'
        if mode=='edit':assert (userdata/'lua/extensions/owned-beta-fixture.lua').read_bytes()==payload
    report['ordinaryGuiUnchanged']=digest(ordinary/'plugins/gui/libqt_plugin.dll')==ordinary_sha
    assert report['ordinaryGuiUnchanged'];report['passed']=True;save()
finally:
    if child is not None and child.poll() is None:child.kill();child.wait(timeout=5)
    report['ordinaryGuiUnchanged']=digest(ordinary/'plugins/gui/libqt_plugin.dll')==ordinary_sha
    save()
