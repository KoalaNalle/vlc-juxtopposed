#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Owned Windows proxy-settings boundary and real loopback WinHTTP PAC tests."""
import argparse,ctypes,hashlib,json,os,re,shutil,subprocess,threading,time
from pathlib import Path
from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer

parser=argparse.ArgumentParser(description=__doc__)
for name in ['package','executable','output-dir']:parser.add_argument('--'+name,type=Path,required=True)
parser.add_argument('--core',type=Path)
parser.add_argument('--expected-failure',action='store_true')
parser.add_argument('--expected-local-bypass-failure',action='store_true')
parser.add_argument('--native-core',action='store_true')
args=parser.parse_args();out=args.output_dir.resolve();out.mkdir(parents=True,exist_ok=False)
runtime=out/'runtime';runtime.mkdir()
source=args.package.resolve()
assert source.drive.upper()==runtime.drive.upper(), 'Owned runtime hard links require the same volume'
mutable={'libvlccore.dll','test_network_proxy.exe'}
for path in source.rglob('*'):
    rel=path.relative_to(source)
    if rel.parts[0]=='portable' or path.name=='plugins.dat':continue
    dest=runtime/rel
    if path.is_dir():dest.mkdir(exist_ok=True)
    elif rel.as_posix() in mutable:shutil.copy2(path,dest)
    else:os.link(path,dest)
(runtime/'portable').mkdir()
shutil.copy2(args.executable,runtime/'test_network_proxy.exe')
if args.core:shutil.copy2(args.core,runtime/'libvlccore.dll')
for name in ['temp','userdata','appdata','localappdata']:(out/name).mkdir()
requests=[]
class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        requests.append(dict(path=self.path,authorizationPresent=bool(self.headers.get('Authorization')),
            proxyAuthorizationPresent=bool(self.headers.get('Proxy-Authorization'))))
        if self.path=='http://media.owned.invalid/owned-vlc-proxy-transport':
            data=b'owned-vlc-proxy-transport\r\n'
            self.send_response(200);self.send_header('Content-Type','application/octet-stream')
            self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data);return
        if self.path.endswith('/rejected'):
            self.send_response(401);self.send_header('WWW-Authenticate','Negotiate')
            self.send_header('Content-Length','0');self.send_header('Cache-Control','no-store');self.end_headers();return
        data=b'function FindProxyForURL(url,host) { return host === "direct.owned.invalid" || url.indexOf("/direct") >= 0 ? "DIRECT" : "PROXY pac.owned.invalid:18080"; }'
        self.send_response(200);self.send_header('Content-Type','application/x-ns-proxy-autoconfig')
        self.send_header('Cache-Control','no-store')
        self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data)
    def log_message(self,*args):pass
server=ThreadingHTTPServer(('127.0.0.1',0),Handler);server.daemon_threads=True
thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
env={k:v for k,v in os.environ.items() if not k.startswith('VLC_')}
env.update(VLC_PLUGIN_PATH=str(runtime/'plugins'),VLC_USERDATA_PATH=str(out/'userdata'),
    TEMP=str(out/'temp'),TMP=str(out/'temp'),APPDATA=str(out/'appdata'),LOCALAPPDATA=str(out/'localappdata'))
expected_failure=args.expected_failure or args.expected_local_bypass_failure
report=dict(passed=False,expectedFailure=expected_failure,expectedLocalBypassFailure=args.expected_local_bypass_failure,forcedCleanupUsed=False,windowsSettingsOwned=True,
    realWinHttpPac=True,nativeCoreEntry=args.native_core,runtimeSha256={n:hashlib.sha256((runtime/n).read_bytes()).hexdigest() for n in ['libvlccore.dll','test_network_proxy.exe']})
kernel=ctypes.WinDLL('kernel32',use_last_error=True);kernel.SetErrorMode.argtypes=[ctypes.c_uint];kernel.SetErrorMode.restype=ctypes.c_uint
child=None
try:
    with (out/'stdout.txt').open('wb') as stdout,(out/'stderr.txt').open('wb') as stderr:
        previous=kernel.SetErrorMode(0x8003)
        command=[str(runtime/'test_network_proxy.exe'),f'http://127.0.0.1:{server.server_port}/owned.pac']
        if args.native_core:command.append('--native-core')
        if args.native_core and args.expected_failure:
            assert report['runtimeSha256']['libvlccore.dll']=='218708c12b18fa9e69b9242fd0c0ea38c4ad4f61bf6c9b0798bd7cd8cf91d682'
            command.append('--allow-missing-core-hook')
        try:child=subprocess.Popen(command,
            cwd=runtime,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
        finally:kernel.SetErrorMode(previous)
        report['ownedPid']=child.pid;started=time.monotonic()
        try:report['exitCode']=child.wait(timeout=35)
        except subprocess.TimeoutExpired:
            report['forcedCleanupUsed']=True;child.kill();report['exitCode']=child.wait(timeout=5)
        report['elapsedSeconds']=time.monotonic()-started
    text=(out/'stdout.txt').read_text(encoding='utf-8',errors='replace')
    report['ownedCoreImportBoundaryConfirmed']='# native_core=1 owned_settings_boundary=1' in text
    report['checks']=re.findall(r'^(ok|not ok) (\d+) (.*)$',text,re.M)
    report['normalTermination']=not report['forcedCleanupUsed'] and report['exitCode']==(1 if expected_failure else 0)
    if args.expected_local_bypass_failure:
        report['passed']=report['normalTermination'] and len(report['checks'])==19 and all(c[0]=='ok' for c in report['checks'][:18]) and report['checks'][-1]==('not ok','19','local bypass does not bypass an IPv6 literal')
    elif args.expected_failure:
        report['passed']=report['normalTermination'] and len(report['checks'])==6 and all(c[0]=='ok' for c in report['checks'][:5]) and report['checks'][-1]==('not ok','6','empty explicit value uses owned Windows proxy')
    else:
        expected_checks=45 if args.native_core else 41
        transport=any(c['path']=='http://media.owned.invalid/owned-vlc-proxy-transport' for c in requests)
        report['actualVlcProxyTransportExercised']=transport
        report['passed']=report['normalTermination'] and len(report['checks'])==expected_checks and all(c[0]=='ok' for c in report['checks']) and bool(requests) and not any(c['authorizationPresent'] or c['proxyAuthorizationPresent'] for c in requests) and (not args.native_core or report['ownedCoreImportBoundaryConfirmed'] and transport)
finally:
    if child is not None and child.poll() is None:
        report['forcedCleanupUsed']=True;child.kill();child.wait(timeout=5)
    server.shutdown();server.server_close();thread.join(timeout=2)
    report['pacRequests']=requests
    (out/'verification.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps(report),flush=True)
raise SystemExit(0 if report['passed'] else 1)
