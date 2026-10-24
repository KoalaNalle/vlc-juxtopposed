#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Shared staging and bounded process execution for owned Windows HTTP tests."""
from pathlib import Path
import ctypes, hashlib, os, subprocess


def stage_runtime(source, destination):
    source = Path(source).resolve()
    destination = Path(destination).resolve()
    assert source.drive.upper() == destination.drive.upper(), 'Use one volume for immutable runtime hard links'
    assert (source / 'vlc.exe').is_file() and (source / 'libvlccore.dll').is_file()
    destination.mkdir()
    for path in source.rglob('*'):
        relative = path.relative_to(source)
        if relative.parts[0] == 'portable' or path.name == 'plugins.dat':
            continue
        target = destination / relative
        if path.is_dir():
            target.mkdir(exist_ok=True)
        elif path.is_file():
            os.link(path, target)
    (destination / 'portable').mkdir()


def environment(runtime, output):
    env = {k: v for k, v in os.environ.items() if not k.startswith('VLC_')}
    for name in ['temp', 'userdata', 'appdata', 'localappdata']:
        (output / name).mkdir(exist_ok=True)
    env.update(VLC_PLUGIN_PATH=str(runtime / 'plugins'),
               VLC_USERDATA_PATH=str(output / 'userdata'),
               TEMP=str(output / 'temp'), TMP=str(output / 'temp'),
               APPDATA=str(output / 'appdata'), LOCALAPPDATA=str(output / 'localappdata'))
    return env


def run_owned(command, runtime, output, timeout=30):
    output.mkdir(exist_ok=True)
    env = environment(runtime, output)
    kernel = ctypes.WinDLL('kernel32', use_last_error=True)
    kernel.SetErrorMode.argtypes = [ctypes.c_uint]
    kernel.SetErrorMode.restype = ctypes.c_uint
    previous = kernel.SetErrorMode(0x8003)
    child = None
    result = dict(forcedCleanupUsed=False)
    try:
        with (output / 'stdout.txt').open('wb') as stdout, (output / 'stderr.txt').open('wb') as stderr:
            child = subprocess.Popen([str(arg) for arg in command], cwd=runtime, env=env,
                                     stdout=stdout, stderr=stderr, creationflags=subprocess.CREATE_NO_WINDOW)
            result['pid'] = child.pid
            try:
                result['exitCode'] = child.wait(timeout=timeout)
            except subprocess.TimeoutExpired:
                result['forcedCleanupUsed'] = True
                child.kill()
                result['exitCode'] = child.wait(timeout=5)
    finally:
        kernel.SetErrorMode(previous)
        if child is not None and child.poll() is None:
            result['forcedCleanupUsed'] = True
            child.kill()
            child.wait(timeout=5)
    result['normalExit'] = result['exitCode'] == 0 and not result['forcedCleanupUsed']
    return result


def regenerate_cache(runtime, output):
    result = run_owned([runtime / 'vlc-cache-gen.exe', runtime / 'plugins'], runtime, output)
    assert result['normalExit'], result


def runtime_hashes(runtime):
    paths = [runtime / 'libvlccore.dll', *runtime.glob('plugins/**/libhttp*_plugin.dll')]
    return {p.relative_to(runtime).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}
