#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Owned HTTP/HTTPS proxy transport, authentication, TLS and certificate checks."""
from pathlib import Path
import argparse,base64,hashlib,json,re,select,socket,ssl,threading,time
from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer
from urllib.parse import urlsplit
from native_http_runtime import stage_runtime, regenerate_cache, run_owned, runtime_hashes

parser=argparse.ArgumentParser(description=__doc__)
for name in ['package','certificates','output-dir']:parser.add_argument('--'+name,type=Path,required=True)
args=parser.parse_args();out=args.output_dir.resolve();out.mkdir(parents=True,exist_ok=False)
cert=args.certificates.resolve()
events=[];server_errors=[];case_start=0
secret='owned-pwd';encoded=base64.b64encode(b'owned-user:owned-pwd').decode()
expected_auth='Basic '+encoded
origin_bytes=b'owned-origin-payload\n';proxy_bytes=b'owned-proxy-payload\n'
class Handler(BaseHTTPRequestHandler):
    def event(self):
        event=dict(role=self.server.role,method=self.command,path=self.path,proxyAuthorizationPresent=bool(self.headers.get('Proxy-Authorization')),ownedProxyAuthenticated=self.headers.get('Proxy-Authorization')==expected_auth,tlsVersion=self.connection.version() if isinstance(self.connection,ssl.SSLSocket) else None)
        events.append(event);return event
    def reply(self,code,data=b''):
        self.protocol_version='HTTP/1.0'
        if self.server.compat_partial and code==200:code=206
        self.send_response(code);self.send_header('Content-Length',str(len(data)));self.send_header('Content-Type','application/octet-stream')
        if code==407:self.send_header('Proxy-Authenticate','Basic realm="owned-loopback"')
        self.end_headers()
        if data:self.wfile.write(data)
    def do_GET(self):
        event=self.event()
        if len(events)-case_start>8:self.reply(503);return
        if self.server.role=='origin':self.reply(200,origin_bytes);return
        target=urlsplit(self.path)
        if target.scheme!='http' or target.hostname!='127.0.0.1' or target.port!=origin.server_port:self.reply(403);return
        if not event['ownedProxyAuthenticated']:self.reply(407);return
        self.reply(200,proxy_bytes)
    def do_CONNECT(self):
        event=self.event()
        if self.server.role!='proxy' or self.path!=f'127.0.0.1:{secure_origin.server_port}':self.reply(403);return
        if not event['ownedProxyAuthenticated']:self.reply(407);return
        with socket.create_connection(('127.0.0.1',secure_origin.server_port),timeout=5) as upstream:
            self.send_response(200,'Connection Established');self.end_headers();self.wfile.flush()
            deadline=time.monotonic()+15
            while time.monotonic()<deadline:
                readable,_,_=select.select([self.connection,upstream],[],[],0.1)
                for source in readable:
                    data=source.recv(65536)
                    if not data:return
                    target=upstream if source is self.connection else self.connection
                    target.sendall(data)
    def log_message(self,*args):pass
class Server(ThreadingHTTPServer):
    daemon_threads=True
    def get_request(self):
        connection,address=super().get_request();connection.settimeout(5)
        if self.context:
            try:connection=self.context.wrap_socket(connection,server_side=True)
            except (OSError,ssl.SSLError):connection.close();raise
        return connection,address
    def handle_error(self,request,client_address):server_errors.append(dict(role=self.role,client=str(client_address)))
def context(wrong=False):
    ctx=ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER);ctx.load_cert_chain(cert/('wrong-server.pem' if wrong else 'server-chain.pem'),cert/'server.key')
    ctx.set_ecdh_curve('secp521r1');ctx.set_alpn_protocols(['http/1.1']);return ctx
servers=[];threads=[]
for role,tls,wrong in [('origin',False,False),('origin',True,False),('proxy',False,False),('proxy',True,False),('proxy',True,True)]:
    server=Server(('127.0.0.1',0),Handler);server.role=role;server.compat_partial=False;server.context=context(wrong) if tls else None;servers.append(server)
origin,secure_origin,proxy,secure_proxy,wrong_proxy=servers
for server in servers:
    thread=threading.Thread(target=lambda s=server:s.serve_forever(poll_interval=0.05),daemon=True);thread.start();threads.append(thread)
report=dict(legacyResponseAdaptation="206 without Content-Range, intentionally rejected by modern reader and tolerated by legacy reader; finite response",passed=False,cases={},events=events,serverErrors=server_errors,systemProxySettingsUsed=False,personalCredentialsUsed=False,osCertificateStoreChanged=False)
def save():(out/'verification.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8',newline='\n')
try:
    runtime=out/'runtime';stage_runtime(args.package,runtime)
    regenerate_cache(runtime,out/'cache')
    report['runtimeSha256']=runtime_hashes(runtime)
    gnutls,=runtime.glob('plugins/**/libgnutls_plugin.dll')
    report['runtimeSha256'][gnutls.relative_to(runtime).as_posix()]=hashlib.sha256(gnutls.read_bytes()).hexdigest()
    report['certificateSha256']={p.relative_to(cert).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in [cert/'trust/ca.pem',cert/'server.pem',cert/'wrong-server.pem']}
    cases=[]
    for backend in ['modern','legacy']:
        cases.append((backend+'-http-proxy',backend,'http',proxy,False,'trust','1.2'))
        for protocol in ['1.2','1.3']:
            cases.extend([(backend+'-tls-proxy-'+protocol,backend,'http',secure_proxy,False,'trust',protocol),(backend+'-connect-'+protocol,backend,'https',proxy,False,'trust',protocol)])
    for protocol in ['1.2','1.3']:cases.append(('modern-nested-tls-'+protocol,'modern','https',secure_proxy,False,'trust',protocol))
    cases.extend([('modern-untrusted-proxy','modern','http',secure_proxy,True,'empty','1.2'),('modern-proxy-name-mismatch','modern','http',wrong_proxy,True,'trust','1.2')])
    cases=[case for case in cases if case[1]!="legacy" or case[2]=="http"]
    for name,backend,scheme,selected_proxy,reject,trust,protocol in cases:
        for server in servers:server.compat_partial=backend=="legacy"
        for server in servers:
            if server.context:
                version=ssl.TLSVersion.TLSv1_2 if protocol=='1.2' else ssl.TLSVersion.TLSv1_3
                server.context.minimum_version=version;server.context.maximum_version=version
        case=out/name;case.mkdir();config=case/'vlcrc';dump=case/'received.bin';log=case/'vlc.log'
        proxy_scheme='https' if selected_proxy.context else 'http'
        proxy_url=f'{proxy_scheme}://owned-user@127.0.0.1:{selected_proxy.server_port}'
        config.write_text(f'http-proxy={proxy_url}\nhttp-proxy-pwd={secret}\nmetadata-network-access=0\none-instance=0\n',encoding='utf-8',newline='\n')
        target=secure_origin if scheme=='https' else origin
        command=[str(runtime/'vlc.exe'),'--intf=dummy','--extraintf=','--control=','--no-one-instance','--no-media-library','--no-crashdump','--no-metadata-network-access','--no-ml-network-discovery','--play-and-exit','--demux=dump','--demuxdump-file='+str(dump),'--file-logging','--log-verbose=2','--verbose=2','--logfile='+str(log),'--config='+str(config),'--no-gnutls-system-trust','--gnutls-dir-trust='+str(cert/trust),f'{scheme}://127.0.0.1:{target.server_port}/owned?tls={protocol}']
        start=len(events);case_start=start
        result=run_owned(command,runtime,case)
        report['cases'][name]=result
        data=dump.read_bytes() if dump.exists() else b'';text=log.read_text(errors='replace') if log.exists() else ''
        case_events=events[start:];tls_version='TLSv'+protocol
        result.update(events=case_events,accessModules=re.findall(r'using access module "([^"]+)"',text),logHasCredential=secret in text or encoded in text,originSawProxyCredential=any(e['role']=='origin' and e['proxyAuthorizationPresent'] for e in case_events),payloadMatches=data==(origin_bytes if scheme=='https' else proxy_bytes),bytesReceived=len(data),expectedTls=tls_version)
        if reject:
            reason='certificate issuer is unknown' if trust=='empty' else 'name in the certificate does not match'
            result['certificateRejected']=not data and not case_events and 'Certificate verification failure:' in text and reason in text
            correct=result['certificateRejected']
        else:
            expected_modules=['http'] if backend=='modern' else ['oldhttp']
            correct=result['payloadMatches'] and result['accessModules']==expected_modules and any(e['role']=='proxy' and e['ownedProxyAuthenticated'] for e in case_events)
            if scheme=='https':correct &= any(e['role']=='origin' and e['tlsVersion']==tls_version for e in case_events)
            if selected_proxy.context:correct &= any(e['role']=='proxy' and e['tlsVersion']==tls_version for e in case_events)
        result['passed']=bool(correct and result['exitCode']==0 and not result['forcedCleanupUsed'] and not result['logHasCredential'] and not result['originSawProxyCredential'])
        print(name,'passed='+str(result['passed']),'exit='+str(result['exitCode']),'requests='+str(len(case_events)),flush=True);save();assert result['passed'],name
    report['passed']=not server_errors
    assert report['passed'],'unexpected owned server errors'
finally:
    for server in servers:server.shutdown();server.server_close()
    for thread in threads:thread.join(timeout=2)
    save()
