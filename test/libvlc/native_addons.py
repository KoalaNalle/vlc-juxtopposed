#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Actual Windows add-on install/remove, restart and locked-file acceptance."""
import argparse
import ctypes
import hashlib
import json
import os
from pathlib import Path
import subprocess
import zipfile
from native_http_runtime import stage_runtime, environment, regenerate_cache


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime',type=Path,required=True)
    parser.add_argument('--probe',type=Path,required=True)
    parser.add_argument('--core',type=Path)
    parser.add_argument('--storage',type=Path)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--shutdown-runs',type=int,default=20)
    args=parser.parse_args()
    assert os.name=='nt' and 1<=args.shutdown_runs<=100
    source=args.runtime.resolve();out=args.output.resolve()
    assert source.drive.upper()==out.drive.upper()
    out.mkdir(parents=True,exist_ok=False)
    selected=['libvlccore.dll','plugins/misc/libaddonsfsstorage_plugin.dll',
              'plugins/misc/libaddonsvorepository_plugin.dll']
    before={name:digest(source/name) for name in selected}
    runtime=out/'runtime';stage_runtime(source,runtime)
    for replacement,name in [(args.core,selected[0]),(args.storage,selected[1])]:
        if replacement:
            target=runtime/name;target.unlink();os.link(replacement.resolve(),target)
    os.link(args.probe.resolve(),runtime/'test_addons.exe')
    regenerate_cache(runtime,out/'cache')
    payload=b'-- Owned inert fixture\nfunction descriptor() return {title="Owned beta fixture",version="1.0"} end\nfunction activate() end\nfunction deactivate() end\n'
    manifest='''<?xml version="1.0" encoding="UTF-8"?>
<videolan><addons><addon type="extension" id="8beaeec8-7b16-45f9-83c0-a0ecc57d0401" version="1.0"><name>Owned beta fixture</name><summary>Local install/remove fixture</summary><description>Inert owned test</description><creator>Test</creator><resource type="extension">owned-beta-fixture.lua</resource></addon></addons></videolan>'''
    package=out/'owned.vlp'
    with zipfile.ZipFile(package,'w',compression=zipfile.ZIP_DEFLATED) as archive:
        archive.writestr('manifest.xml',manifest)
        archive.writestr('owned-beta-fixture.lua',payload)
    results=[]
    kernel=ctypes.WinDLL('kernel32',use_last_error=True)
    kernel.SetErrorMode.argtypes=[ctypes.c_uint];kernel.SetErrorMode.restype=ctypes.c_uint
    kernel.CreateFileW.argtypes=[ctypes.c_wchar_p,ctypes.c_uint,ctypes.c_uint,ctypes.c_void_p,ctypes.c_uint,ctypes.c_uint,ctypes.c_void_p]
    kernel.CreateFileW.restype=ctypes.c_void_p
    kernel.CloseHandle.argtypes=[ctypes.c_void_p];kernel.CloseHandle.restype=ctypes.c_int

    def step(name,mode,userdata=None):
        case=out/name;case.mkdir()
        env=environment(runtime,case)
        if userdata: env['VLC_USERDATA_PATH']=str(userdata)
        previous=kernel.SetErrorMode(0x8003)
        try:
            with (case/'stdout.txt').open('wb') as stdout,(case/'stderr.txt').open('wb') as stderr:
                proc=subprocess.Popen([str(runtime/'test_addons.exe'),mode,str(package)],cwd=runtime,
                    env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
                forced=False
                try: code=proc.wait(timeout=25)
                except subprocess.TimeoutExpired:
                    forced=True;proc.kill();code=proc.wait(timeout=5)
        finally: kernel.SetErrorMode(previous)
        text=(case/'stdout.txt').read_text(encoding='utf-8',errors='replace')
        errors=(case/'stderr.txt').read_text(encoding='utf-8',errors='replace')
        result=dict(case=name,mode=mode,exitCode=code,forcedCleanupUsed=forced,
            checks=sum(line.startswith(('ok ','not ok ')) for line in text.splitlines()),
            accepted=code==0 and not forced and 'not ok ' not in text and 'Assertion failed' not in errors)
        results.append(result)
        print(name,code,'pass' if result['accepted'] else 'FAIL',flush=True)
        return result

    for mode in ['normal','remove-first','install-twice','queued-install','queued-remove-catalog']:
        result=step(mode,mode)
        files=list((out/mode/'userdata').rglob('owned-beta-fixture.lua'))
        expected=1 if mode=='install-twice' else 0
        result['filesMatchExpected']=len(files)==expected and all(p.read_bytes()==payload for p in files)
        result['accepted'] &= result['filesMatchExpected']

    data=out/'käyttäjä 日本語';data.mkdir()
    installed=data/'lua/extensions/owned-beta-fixture.lua'
    catalog=data/'catalog.xml'
    install=step('durable-install','install-only',data)
    install['installedBytesMatch']=installed.is_file() and installed.read_bytes()==payload
    install['catalogTracksFile']=catalog.is_file() and 'owned-beta-fixture.lua' in catalog.read_text(encoding='utf-8')
    install['accepted'] &= install['installedBytesMatch'] and install['catalogTracksFile']
    step('restart-installed','catalog-check',data)
    if installed.is_file():
        handle=kernel.CreateFileW(str(installed),0x80000000,1,None,3,0,None)
        assert handle!=ctypes.c_void_p(-1).value,ctypes.get_last_error()
        try: locked=step('locked-remove','catalog-locked-remove',data)
        finally: assert kernel.CloseHandle(handle)
        locked['filePreserved']=installed.is_file() and installed.read_bytes()==payload
        locked['catalogPreserved']=catalog.is_file() and 'owned-beta-fixture.lua' in catalog.read_text(encoding='utf-8')
        locked['accepted'] &= locked['filePreserved'] and locked['catalogPreserved']
        step('restart-after-failure','catalog-check',data)
        removed=step('unlocked-retry','catalog-remove',data)
        removed['fileRemoved']=not installed.exists()
        removed['catalogUntracksFile']=catalog.is_file() and 'owned-beta-fixture.lua' not in catalog.read_text(encoding='utf-8')
        removed['accepted'] &= removed['fileRemoved'] and removed['catalogUntracksFile']
        step('restart-after-removal','catalog-empty',data)
    for index in range(args.shutdown_runs):
        step(f'pending-shutdown-{index:02}','delete-pending')
    assert before=={name:digest(source/name) for name in selected},'Source runtime changed'
    report=dict(runtime={name:digest(runtime/name) for name in selected},
        probe=digest(runtime/'test_addons.exe'),cases=results,
        accepted=all(row['accepted'] for row in results),sourceRuntimePreserved=True)
    (out/'results.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8',newline='\n')
    return 0 if report['accepted'] else 1


if __name__=='__main__':
    raise SystemExit(main())
