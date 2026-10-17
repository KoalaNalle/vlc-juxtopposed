#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Real HTTP media routing through WinHTTP PAC with process-owned settings/DNS."""
import argparse, hashlib, json, re, shutil, threading
from pathlib import Path
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from native_http_runtime import stage_runtime, regenerate_cache, run_owned, runtime_hashes

parser = argparse.ArgumentParser(description=__doc__)
for name in ['package', 'executable', 'output-dir']:
    parser.add_argument('--' + name, type=Path, required=True)
parser.add_argument('--expected-failure', action='store_true')
args = parser.parse_args()
out = args.output_dir.resolve()
out.mkdir(parents=True, exist_ok=False)
runtime = out / 'runtime'
stage_runtime(args.package, runtime)
shutil.copy2(args.executable, runtime / 'test_network_pac_transport.exe')
events = []


class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        role = self.server.role
        events.append(dict(role=role, path=self.path,
                           authorizationPresent=bool(self.headers.get('Authorization')),
                           proxyAuthorizationPresent=bool(self.headers.get('Proxy-Authorization'))))
        if role == 'pac':
            data = ('function FindProxyForURL(url,host) { if (url.indexOf("?route=proxy") >= 0) '
                    'return "PROXY 127.0.0.1:' + str(proxy.server_port) + '"; return "DIRECT"; }').encode()
        else:
            data = b'origin-payload\n' if role == 'origin' else b'proxy-no-auth\n'
        self.send_response(200)
        self.send_header('Content-Type', 'application/x-ns-proxy-autoconfig' if role == 'pac' else 'application/octet-stream')
        self.send_header('Content-Length', str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def log_message(self, *args):
        pass


servers = []
threads = []
for role in ['origin', 'proxy', 'pac']:
    server = ThreadingHTTPServer(('127.0.0.1', 0), Handler)
    server.role = role
    server.daemon_threads = True
    servers.append(server)
origin, proxy, pac = servers
for server in servers:
    thread = threading.Thread(target=lambda s=server: s.serve_forever(poll_interval=0.05), daemon=True)
    thread.start()
    threads.append(thread)
urls = [f'http://media.owned.invalid:{origin.server_port}/owned?route={route}' for route in ['proxy', 'direct']]
report = dict(passed=False, expectedFailure=args.expected_failure, events=events,
              systemProxySettingsReadOrChanged=False, personalCredentialsUsed=False,
              dnsScope='media.owned.invalid mapped to 127.0.0.1 inside this owned process only')
try:
    regenerate_cache(runtime, out / 'cache')
    result = run_owned([runtime / 'test_network_pac_transport.exe',
                        f'http://127.0.0.1:{pac.server_port}/owned.pac', *urls], runtime, out / 'case')
    report.update(result)
    text = (out / 'case/stdout.txt').read_text(encoding='utf-8', errors='replace')
    checks = re.findall(r'^(ok|not ok) (\d+) (.*)$', text, re.M)
    report['checks'] = checks
    report['pacInputUrls'] = re.findall(r'^# PAC url=(\S+) result=', text, re.M)
    report['noCredentialsSent'] = not any(e['authorizationPresent'] or e['proxyAuthorizationPresent'] for e in events)
    report['runtimeSha256'] = runtime_hashes(runtime)
    report['executableSha256'] = hashlib.sha256((runtime / 'test_network_pac_transport.exe').read_bytes()).hexdigest()
    if args.expected_failure:
        correct = report['exitCode'] == 1 and len(checks) == 6 and all(c[0] == 'ok' for c in checks[:5]) and checks[5] == ('not ok', '6', 'actual HTTP routing preserves the complete path and query')
        correct &= report['pacInputUrls'] == [f'http://media.owned.invalid:{origin.server_port}']
    else:
        correct = report['normalExit'] and len(checks) == 9 and all(c[0] == 'ok' for c in checks)
        correct &= report['pacInputUrls'] == urls
        correct &= any(e['role'] == 'proxy' and e['path'] == urls[0] for e in events)
        correct &= any(e['role'] == 'origin' and e['path'] == '/owned?route=direct' for e in events)
    report['passed'] = bool(correct and report['noCredentialsSent'] and not report['forcedCleanupUsed'])
finally:
    for server in servers:
        server.shutdown()
        server.server_close()
    for thread in threads:
        thread.join(timeout=2)
    (out / 'verification.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8', newline='\n')
print('PAC transport:', 'PASS' if report['passed'] else 'FAIL', flush=True)
raise SystemExit(0 if report['passed'] else 1)
