#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Decode a checked16-bit FLAC fixture and compare its complete PCM identity.

Uses the ordinary launcher, packetizer, decoder, transcode and raw/file modules.
Requires a known STREAMINFO PCM digest. No Qt, primary-player or all-format claim.
"""
import argparse
import ctypes
from hashlib import md5, sha256
import json
from pathlib import Path
import subprocess
from native_http_runtime import environment

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--package',type=Path,required=True)
parser.add_argument('--media',type=Path,required=True)
parser.add_argument('--output-dir',type=Path,required=True)
args=parser.parse_args()
runtime,source,out=(p.resolve() for p in (args.package,args.media,args.output_dir))
assert out!=runtime and runtime not in out.parents and out!=source.parent
assert (runtime/'portable').is_dir()
blob=source.read_bytes()
assert blob[:4]==b'fLaC' and blob[4]&127==0 and int.from_bytes(blob[5:8],'big')==34
info=blob[8:42];packed=int.from_bytes(info[10:18],'big')
samples=packed&((1<<36)-1);rate=packed>>44;channels=((packed>>41)&7)+1;bits=((packed>>36)&31)+1
assert bits==16 and samples>0 and 0<rate<=655350 and 1<=channels<=8
assert info[18:34]!=b'\0'*16 and samples*channels*2<=256*1024*1024
out.mkdir(parents=True,exist_ok=False)
destination=out/'decoded.pcm';config=out/'vlcrc'
config.write_text('metadata-network-access=0\nml-network-discovery=0\n',encoding='utf-8')
escaped=destination.as_posix().replace("'","\\'")
sout=f"--sout=#transcode{{acodec=s16l,channels={channels},samplerate={rate}}}:std{{access=file{{no-overwrite}},mux=raw,dst='{escaped}'}}"
command=[str(runtime/'vlc.exe'),'--intf=dummy','--no-one-instance','--no-media-library','--no-crashdump',
 '--no-metadata-network-access','--no-ml-network-discovery','--no-video','--no-spu','--no-sub-autodetect-file',
 '--no-sout-video','--no-sout-spu','--no-sout-keep','--play-and-exit','--file-logging','--verbose=2','--log-verbose=2',
 '--logfile='+str(out/'vlc.log'),'--config='+str(config),sout,str(source)]
env=environment(runtime,out);env.pop('VLC_PLUGIN_PATH',None)
paths=['libvlccore.dll','plugins/packetizer/libpacketizer_flac_plugin.dll','plugins/demux/libflacsys_plugin.dll']
def digest(p):return sha256(p.read_bytes()).hexdigest()
before={name:digest(runtime/name) for name in paths};media_before=sha256(blob).hexdigest()
kernel=ctypes.WinDLL('kernel32');previous=kernel.SetErrorMode(0x8003)
report={'passed':False,'forcedCleanupUsed':False,'sourceSha256':media_before,'expectedSamples':samples,
 'rate':rate,'channels':channels,'bits':bits,'streamInfoMd5':info[18:34].hex(),'scope':__doc__}
child=None
try:
 with (out/'stdout.txt').open('wb') as stdout,(out/'stderr.txt').open('wb') as stderr:
  child=subprocess.Popen(command,cwd=runtime,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
  try:report['exitCode']=child.wait(timeout=30)
  except subprocess.TimeoutExpired:
   report['forcedCleanupUsed']=True;child.kill();report['exitCode']=child.wait(timeout=5)
finally:
 kernel.SetErrorMode(previous)
 if child is not None and child.poll() is None:child.kill();child.wait(timeout=5);report['forcedCleanupUsed']=True
raw=destination.read_bytes() if destination.exists() else b''
log=(out/'vlc.log').read_text(encoding='utf-8',errors='replace') if (out/'vlc.log').exists() else ''
report.update(normalExit=report.get('exitCode')==0 and not report['forcedCleanupUsed'],decodedSamples=len(raw)//(channels*2),
 decodedMd5=md5(raw).hexdigest(),decodedSha256=sha256(raw).hexdigest(),sourceUnchanged=digest(source)==media_before,
 protectedRuntimeUnchanged=before=={p:digest(runtime/p) for p in paths},runtimeSha256=before,
 nativeWarnings=[line for line in log.splitlines() if 'warning:' in line],
 nativeErrors=[line for line in log.splitlines() if 'error:' in line],runnerSha256=digest(Path(__file__)))
report['passed']=(report['normalExit'] and report['sourceUnchanged'] and report['protectedRuntimeUnchanged']
 and not report['nativeErrors'] and len(raw)==samples*channels*2 and md5(raw).digest()==info[18:34])
(out/'launch.json').write_text(json.dumps(command,indent=2)+'\n',encoding='utf-8')
(out/'verification.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print(json.dumps(report,indent=2));raise SystemExit(0 if report['passed'] else 1)
