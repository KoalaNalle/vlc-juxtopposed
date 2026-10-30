"""Keep an owned output pipe open and readable while bounding real cancellation."""
import ctypes,subprocess,threading,time

def run_paced_sink(command,runtime,out,env,timeout):
    kernel=ctypes.WinDLL('kernel32',use_last_error=True)
    kernel.SetErrorMode.argtypes=[ctypes.c_uint];kernel.SetErrorMode.restype=ctypes.c_uint
    previous=kernel.SetErrorMode(0x8003)
    row=dict(forcedCleanupUsed=False,readerErrors=[],bytesRead=0)
    child=reader=None
    try:
        with (out/'stderr.txt').open('wb') as stderr:
            child=subprocess.Popen(command,cwd=runtime,env=env,stdout=subprocess.PIPE,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
            row['pid']=child.pid
            def pump():
                try:
                    with child.stdout,(out/'output.mkv').open('xb') as output:
                        while chunk:=child.stdout.read(4096):
                            output.write(chunk);row['bytesRead']+=len(chunk)
                            if row['bytesRead']>=8192 and not (out/'cancel.ready').exists():
                                (out/'cancel.ready').write_text('8192 actual output bytes observed; pipe remains open\n')
                                row['readyAtBytes']=row['bytesRead']
                            time.sleep(.01)
                except BaseException as error:row['readerErrors'].append(repr(error))
            reader=threading.Thread(target=pump,daemon=True);reader.start()
            try:row['exitCode']=child.wait(timeout=timeout)
            except subprocess.TimeoutExpired:
                row['forcedCleanupUsed']=True;child.kill();row['exitCode']=child.wait(timeout=5)
    finally:
        kernel.SetErrorMode(previous)
        if child is not None and child.poll() is None:
            row['forcedCleanupUsed']=True;child.kill();child.wait(timeout=5)
        if reader is not None:
            reader.join(timeout=5);row['readerJoined']=not reader.is_alive()
    row['normalExit']=row.get('exitCode')==0 and not row['forcedCleanupUsed'] and row.get('readerJoined') and not row['readerErrors'] and row.get('readyAtBytes')==8192
    return row
