"""Bound an owned Windows child while deliberately closing only its stdout sink."""
import ctypes
from pathlib import Path
import subprocess
import threading

def run_closed_sink(command, runtime, output, env, timeout):
    result = dict(forcedCleanupUsed=False, sinkReaderErrors=[], sinkPrefixBytes=None)
    kernel = ctypes.WinDLL('kernel32', use_last_error=True)
    kernel.SetErrorMode.argtypes = [ctypes.c_uint]
    kernel.SetErrorMode.restype = ctypes.c_uint
    previous = kernel.SetErrorMode(0x8003)
    child = reader = None
    try:
        with (output / 'stderr.txt').open('wb') as stderr:
            child = subprocess.Popen(command, cwd=runtime, env=env, stdout=subprocess.PIPE,
                stderr=stderr, creationflags=subprocess.CREATE_NO_WINDOW)
            result['pid'] = child.pid
            def close_sink():
                try:
                    with child.stdout:
                        prefix = child.stdout.read(4096)
                        (output / 'stdout-prefix.bin').write_bytes(prefix)
                        result['sinkPrefixBytes'] = len(prefix)
                except BaseException as error:
                    result['sinkReaderErrors'].append(repr(error))
            reader = threading.Thread(target=close_sink, daemon=True)
            reader.start()
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
        if reader is not None:
            reader.join(timeout=5)
            result['sinkReaderJoined'] = not reader.is_alive()
    result['normalExit'] = (result.get('exitCode') == 0 and not result['forcedCleanupUsed']
        and result.get('sinkReaderJoined') and not result['sinkReaderErrors'] and result['sinkPrefixBytes'] == 4096)
    return result
