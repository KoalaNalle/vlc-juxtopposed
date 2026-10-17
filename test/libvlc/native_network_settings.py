#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Actual VLC source buffering and GNU TLS trust/cipher checks on owned servers."""
import argparse,ctypes,hashlib,json,os,re,shutil,socketserver,ssl,subprocess,threading,time
from pathlib import Path
from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer

parser=argparse.ArgumentParser(description=__doc__)
for name in ['package','core','executable','certificates','output-dir']:parser.add_argument('--'+name,type=Path,required=True)
parser.add_argument('--tls-version',choices=('1.2','1.3'),default='1.2')
args=parser.parse_args();out=args.output_dir.resolve();out.mkdir(parents=True,exist_ok=False)
runtime=out/'runtime';runtime.mkdir();source=args.package.resolve();certs=args.certificates.resolve()
assert source.drive.upper()==runtime.drive.upper(), 'Owned runtime hard links require the same volume'
for path in source.rglob('*'):
    rel=path.relative_to(source)
    if rel.parts[0]=='portable' or path.name=='plugins.dat':continue
    dest=runtime/rel
    if path.is_dir():dest.mkdir(exist_ok=True)
    elif rel.as_posix()=='libvlccore.dll':shutil.copy2(path,dest)
    else:os.link(path,dest)
(runtime/'portable').mkdir();shutil.copy2(args.core,runtime/'libvlccore.dll')
shutil.copy2(args.executable,runtime/'test_network_settings.exe')
for name in ['temp','userdata','appdata','localappdata']:(out/name).mkdir()
payload=b'owned-network-cache-fixture\n';media=out/'owned-cache.bin';media.write_bytes(payload)
http_requests=[];tls_connections=[];tls_errors=[];server_names=[]
class HTTP(BaseHTTPRequestHandler):
    def do_GET(self):
        http_requests.append(self.path);self.send_response(200)
        self.send_header('Content-Type','application/octet-stream');self.send_header('Content-Length',str(len(payload)))
        self.end_headers();self.wfile.write(payload)
    def log_message(self,*args):pass
context=ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
version=ssl.TLSVersion.TLSv1_2 if args.tls_version=='1.2' else ssl.TLSVersion.TLSv1_3
context.minimum_version=version;context.maximum_version=version
context.set_ciphers('ECDHE-ECDSA-AES256-GCM-SHA384');context.set_ecdh_curve('secp521r1')
context.load_cert_chain(str(certs/'server-chain.pem'),str(certs/'server.key'))
context.set_servername_callback(lambda socket,name,ctx:server_names.append(name))
class TLS(socketserver.BaseRequestHandler):
    def handle(self):
        self.request.settimeout(5)
        try:
            with context.wrap_socket(self.request,server_side=True) as secure:
                cipher=secure.cipher();negotiated_version=secure.version();data=b''
                while len(data)<5:
                    part=secure.recv(5-len(data))
                    if not part:break
                    data+=part
                tls_connections.append(dict(cipher=cipher,version=negotiated_version,ownedPing=data==b'PING\n'))
                if data==b'PING\n':secure.sendall(b'PONG\n')
        except (ssl.SSLError,ConnectionError,TimeoutError) as error:tls_errors.append(type(error).__name__)
class TLSServer(socketserver.ThreadingTCPServer):
    daemon_threads=True
    def handle_error(self,*args):tls_errors.append('unexpected-server-error')
http=ThreadingHTTPServer(('127.0.0.1',0),HTTP);http.daemon_threads=True
tls=TLSServer(('127.0.0.1',0),TLS)
threads=[threading.Thread(target=s.serve_forever,daemon=True) for s in [http,tls]]
for thread in threads:thread.start()
env={k:v for k,v in os.environ.items() if not k.startswith('VLC_')}
env.update(VLC_PLUGIN_PATH=str(runtime/'plugins'),VLC_USERDATA_PATH=str(out/'userdata'),
    TEMP=str(out/'temp'),TMP=str(out/'temp'),APPDATA=str(out/'appdata'),LOCALAPPDATA=str(out/'localappdata'),
    HOME=str(out/'userdata'),USERPROFILE=str(out/'userdata'))
report=dict(passed=False,expectedChecks=50,forcedCleanupUsed=False,systemTrustStoreModified=False,
    ownCertificateDirectory=True,realTlsServer=True,tlsVersion=args.tls_version,nativeConsumerSeams=False,runtimeSha256={})
gnutls,=runtime.glob('plugins/**/libgnutls_plugin.dll')
for path in [runtime/'libvlccore.dll',runtime/'test_network_settings.exe',gnutls]:
    report['runtimeSha256'][path.relative_to(runtime).as_posix()]=hashlib.sha256(path.read_bytes()).hexdigest()
report['ownedCertificateSha256']={str(p.relative_to(certs)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [certs/'trust/ca.pem',certs/'server.pem']}
kernel=ctypes.WinDLL('kernel32',use_last_error=True);kernel.SetErrorMode.argtypes=[ctypes.c_uint];kernel.SetErrorMode.restype=ctypes.c_uint
child=None
try:
    command=[str(runtime/'test_network_settings.exe'),str(tls.server_address[1]),str(certs/'trust'),str(certs/'empty'),media.as_uri(),f'http://127.0.0.1:{http.server_port}/owned-cache']
    with (out/'stdout.txt').open('wb') as stdout,(out/'stderr.txt').open('wb') as stderr:
        previous=kernel.SetErrorMode(0x8003)
        try:child=subprocess.Popen(command,cwd=runtime,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
        finally:kernel.SetErrorMode(previous)
        report['ownedPid']=child.pid;started=time.monotonic()
        try:report['exitCode']=child.wait(timeout=60)
        except subprocess.TimeoutExpired:
            report['forcedCleanupUsed']=True;child.kill();report['exitCode']=child.wait(timeout=5)
        report['elapsedSeconds']=time.monotonic()-started
    text=(out/'stdout.txt').read_text(encoding='utf-8',errors='replace')
    stderr=(out/'stderr.txt').read_text(encoding='utf-8',errors='replace')
    report['checks']=re.findall(r'^(ok|not ok) (\d+) (.*)$',text,re.M)
    report['gnutlsBannerObserved']='GnuTLS' in stderr
    report['normalExit']=report['exitCode']==0 and not report['forcedCleanupUsed']
    report['passed']=report['normalExit'] and len(report['checks'])==50 and all(c[0]=='ok' for c in report['checks']) and report['gnutlsBannerObserved'] and len(http_requests)==2 and sum(c['ownedPing'] for c in tls_connections)==7 and all(c['version']=='TLSv'+args.tls_version for c in tls_connections) and 'unexpected-server-error' not in tls_errors
finally:
    if child is not None and child.poll() is None:
        report['forcedCleanupUsed']=True;child.kill();child.wait(timeout=5)
    for server in [http,tls]:server.shutdown();server.server_close()
    for thread in threads:thread.join(timeout=2)
    report.update(httpRequests=http_requests,tlsConnections=tls_connections,tlsExpectedRejections=tls_errors,serverNames=server_names)
    (out/'verification.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps(report),flush=True)
raise SystemExit(0 if report['passed'] else 1)
