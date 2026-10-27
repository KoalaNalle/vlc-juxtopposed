#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Internal bounded-runner worker: record with the real LibVLC C API."""
from pathlib import Path
import ctypes as C,json,os,sys,time
runtime,media,config,out=map(Path,sys.argv[1:])
dll_directory=os.add_dll_directory(str(runtime))
vlc=C.CDLL(str(runtime/'libvlc.dll'))
def bind(name,result,*arguments):
 fn=getattr(vlc,name);fn.restype=result;fn.argtypes=arguments;return fn
ptr=C.c_void_p
new=bind('libvlc_new',ptr,C.c_int,C.POINTER(C.c_char_p))
release=bind('libvlc_release',None,ptr)
media_new=bind('libvlc_media_new_path',ptr,C.c_char_p)
media_release=bind('libvlc_media_release',None,ptr)
player_new=bind('libvlc_media_player_new',ptr,ptr,ptr,ptr)
player_release=bind('libvlc_media_player_release',None,ptr)
set_media=bind('libvlc_media_player_set_media',None,ptr,ptr)
play=bind('libvlc_media_player_play',C.c_int,ptr)
state=bind('libvlc_media_player_get_state',C.c_int,ptr)
get_time=bind('libvlc_media_player_get_time',C.c_int64,ptr)
record=bind('libvlc_media_player_record',None,ptr,C.c_bool,C.c_char_p)
options=['--no-ignore-config','--config='+str(config),'--no-media-library','--no-metadata-network-access',
 '--extraintf=','--control=','--vout=dummy','--aout=dummy','--no-hw-dec','--avcodec-threads=1',
 '--file-logging','--verbose=2','--log-verbose=2','--logfile='+str(out/'vlc.log')]
args=[s.encode('utf-8') for s in options]
instance=new(len(args),(C.c_char_p*len(args))(*args));assert instance
player=None
report={}
try:
 player=player_new(instance,None,None);assert player
 item=media_new(str(media).encode('utf-8'));assert item
 set_media(player,item);media_release(item);assert play(player)==0
 deadline=time.monotonic()+12
 while state(player)!=2 or get_time(player)<100:
  assert state(player)!=6,'native playback failed'
  assert time.monotonic()<deadline,'playback start deadline'
  time.sleep(.01)
 report['timeWhenRecordingRequested']=get_time(player)
 record(player,True,None)
 time.sleep(.65)
 record(player,False,None)
 time.sleep(.15)
 report['timeWhenRecordingStopped']=get_time(player)
finally:
 if player:player_release(player)
 release(instance)
(out/'probe.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8',newline='\n')
