#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Actual VLC command-line/saved proxy policy and credential authority checks."""
import argparse, base64, json, re, threading
from pathlib import Path
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from native_http_runtime import stage_runtime, regenerate_cache, run_owned, runtime_hashes

parser = argparse.ArgumentParser(description=__doc__)
for name in ['package', 'output-dir']:
    parser.add_argument('--' + name, type=Path, required=True)
args = parser.parse_args()
out = args.output_dir.resolve()
out.mkdir(parents=True, exist_ok=False)
runtime = out / 'runtime'
stage_runtime(args.package, runtime)
events = []
encoded = base64.b64encode(b'owned-user:owned-pwd').decode()


class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        authenticated = self.headers.get('Proxy-Authorization') == 'Basic ' + encoded
        events.append(dict(role=self.server.role, port=self.server.server_port, path=self.path,
                           proxyAuthorizationPresent=bool(self.headers.get('Proxy-Authorization')),
                           ownedProxyAuthenticated=authenticated))
        if self.server.require_auth and not authenticated:
            self.send_response(407)
            self.send_header('Proxy-Authenticate', 'Basic realm="owned-loopback"')
            self.send_header('Content-Length', '0')
            self.end_headers()
            return
        data = b'origin-payload\n' if self.server.role == 'origin' else b'proxy-payload\n' if authenticated else b'proxy-no-auth\n'
        self.send_response(200)
        self.send_header('Content-Type', 'application/octet-stream')
        self.send_header('Content-Length', str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def log_message(self, *args):
        pass


servers = []
threads = []
for role in ['origin', 'proxy', 'proxy']:
    server = ThreadingHTTPServer(('127.0.0.1', 0), Handler)
    server.role = role
    server.require_auth = False
    server.daemon_threads = True
    servers.append(server)
origin, proxy, replacement = servers
for server in servers:
    thread = threading.Thread(target=lambda s=server: s.serve_forever(poll_interval=0.05), daemon=True)
    thread.start()
    threads.append(thread)
proxy_url = f'http://127.0.0.1:{proxy.server_port}'
report = dict(passed=False, cases={}, events=events, systemProxySettingsUsed=False, personalCredentialsUsed=False)
try:
    regenerate_cache(runtime, out / 'cache')
    report['runtimeSha256'] = runtime_hashes(runtime)
    cases = [
        ('cli-proxy', 'direct://', ['--http-proxy=' + proxy_url], b'proxy-no-auth\n', ''),
        ('cli-direct', proxy_url, ['--http-proxy=direct://'], b'origin-payload\n', ''),
        ('saved-password', f'http://owned-user@127.0.0.1:{proxy.server_port}', [], b'proxy-payload\n', 'owned-pwd'),
        ('different-cli-authority', f'http://owned-user@127.0.0.1:{proxy.server_port}',
         [f'--http-proxy=http://owned-user@127.0.0.1:{replacement.server_port}'], b'proxy-no-auth\n', 'owned-pwd'),
        ('cli-embedded-secret', f'http://owned-user@127.0.0.1:{proxy.server_port}',
         [f'--http-proxy=http://owned-user:owned-pwd@127.0.0.1:{replacement.server_port}'], b'proxy-payload\n', 'stale-owned-pwd'),
    ]
    for name, config_proxy, cli, expected, password in cases:
        for server in [proxy, replacement]:
            server.require_auth = name in ['saved-password', 'cli-embedded-secret']
        case = out / name
        case.mkdir()
        config, dump, log = case / 'vlcrc', case / 'received.bin', case / 'vlc.log'
        config.write_text(f'http-proxy={config_proxy}\nhttp-proxy-pwd={password}\nmetadata-network-access=0\none-instance=0\n', encoding='utf-8', newline='\n')
        command = [runtime / 'vlc.exe', '--intf=dummy', '--extraintf=', '--control=', '--no-one-instance',
                   '--no-media-library', '--no-crashdump', '--no-metadata-network-access', '--no-ml-network-discovery',
                   '--play-and-exit', '--demux=dump', '--demuxdump-file=' + str(dump), '--file-logging',
                   '--log-verbose=2', '--verbose=2', '--logfile=' + str(log), '--config=' + str(config),
                   *cli, f'http://127.0.0.1:{origin.server_port}/owned-media.bin']
        start = len(events)
        result = run_owned(command, runtime, case)
        actual = dump.read_bytes() if dump.exists() else None
        text = log.read_text(encoding='utf-8', errors='replace') if log.exists() else ''
        result.update(payloadMatches=actual == expected, accessModules=re.findall(r'using access module "([^"]+)"', text),
                      logHasCredential=any(s in text for s in ['owned-pwd', encoded, base64.b64encode(b'owned-user:stale-owned-pwd').decode()]),
                      events=events[start:])
        result['originSawProxyCredential'] = any(e['role'] == 'origin' and e['proxyAuthorizationPresent'] for e in result['events'])
        result['passed'] = result['normalExit'] and result['payloadMatches'] and result['accessModules'] == ['http'] and not result['logHasCredential'] and not result['originSawProxyCredential']
        report['cases'][name] = result
        print(name, 'PASS' if result['passed'] else 'FAIL', flush=True)
        assert result['passed'], name
    report['passed'] = True
finally:
    for server in servers:
        server.shutdown()
        server.server_close()
    for thread in threads:
        thread.join(timeout=2)
    (out / 'verification.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8', newline='\n')
raise SystemExit(0 if report['passed'] else 1)
