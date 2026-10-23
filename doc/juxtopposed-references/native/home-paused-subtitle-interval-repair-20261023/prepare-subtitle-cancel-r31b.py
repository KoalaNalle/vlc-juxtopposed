from pathlib import Path
import ast
work=Path(__file__).resolve().parent
text=(work/'native-subtitle-cancel-r31.py').read_text(encoding='utf-8')
text=text.replace('import argparse,ctypes as C,json,os,re,time','import argparse,ctypes as C,json,os,re,time,threading')
point='player=playerNew(instance,None,None);assert player'
callback='''# LibVLC's public log callback copies only format strings and signals events.
# It never calls playback/stop/log-registration APIs and never blocks the input.
# Windows file-stream buffering can hide the earlier preroll lines until later.
armed=False
request_event=threading.Event();anchor_event=threading.Event();finished_event=threading.Event()
timeline=[]
LogCallback=C.CFUNCTYPE(None,C.c_void_p,C.c_int,C.c_void_p,C.c_char_p,C.c_void_p)
@LogCallback
def on_log(opaque,level,context,fmt,varargs):
    if not armed or not fmt:return
    format_string=fmt.decode('utf-8',errors='replace')
    if format_string.startswith('seek request to i_pos = '):
        timeline.append(dict(kind='request',at=time.monotonic(),format=format_string));request_event.set()
    elif request_event.is_set() and format_string.startswith('seek: preroll{ track: '):
        timeline.append(dict(kind='anchor',at=time.monotonic(),format=format_string));anchor_event.set()
    elif request_event.is_set() and format_string.startswith('seek: preroll{ req: '):
        timeline.append(dict(kind='finished',at=time.monotonic(),format=format_string));finished_event.set()
set_log=bind('libvlc_log_set',None,C.c_void_p,LogCallback,C.c_void_p)
set_log(instance,on_log,None)
'''
assert text.count(point)==1
text=text.replace(point,callback+point)
text=text.replace("before_io=process_io();started=time.monotonic();result['seekReturn']=seek(player,55_000_000,False)",
    "before_io=process_io();started=time.monotonic();armed=True;result['seekReturn']=seek(player,55_000_000,False)")
text=text.replace("text=logtext();now_io=process_io()\n        anchor_seen=anchor in text\n        finished_seen=finished_marker in text",
    "now_io=process_io()\n        anchor_seen=anchor_event.is_set()\n        finished_seen=finished_event.is_set()")
text=text.replace("triggered=trigger,state=state(player))", "triggered=trigger)")
text=text.replace("result['finalSeekMarkerPresent']=finished_marker in text", "result['finalSeekMarkerPresent']=finished_event.is_set()\nresult['callbackTimeline']=timeline\nresult['fileSeekMarkerPresent']=finished_marker in text")
text=text.replace("Index-phase qualification additionally requires AV preroll anchors, increased own-process reads, and no final seek/preroll marker even after teardown.",
    "Index-phase qualification uses a non-blocking public LibVLC log callback: request and AV anchors, increased own-process reads, and no final seek/preroll callback even after teardown.")
ast.parse(text)
target=work/'native-subtitle-cancel-r31b.py';assert not target.exists()
target.write_text(text,encoding='utf-8',newline='\n')
parent=(work/'run-native-subtitle-cancel-r31.py').read_text(encoding='utf-8')
parent=parent.replace("worker=work/'native-subtitle-cancel-r31.py'", "worker=work/'native-subtitle-cancel-r31b.py'")
parent=parent.replace("root=work/'native-subtitle-cancel-r31'", "root=work/'native-subtitle-cancel-r31b'")
parent=parent.replace("control='Full-payload interval scan versus header-only interval scan; both are positive caption controls.'", "control='Ordinary Stop during private full-payload and header-only subtitle-index scans; original file-log observation attempts are retained separately.'")
ast.parse(parent)
target=work/'run-native-subtitle-cancel-r31b.py';assert not target.exists()
target.write_text(parent,encoding='utf-8',newline='\n')
