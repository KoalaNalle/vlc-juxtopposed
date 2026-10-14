#!/usr/bin/env python3
"""Run the actual Qt network-stream model with isolated loopback HTTP audio."""
import argparse
import hashlib
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
import os
from pathlib import Path
import re
import subprocess
import threading

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--package', type=Path, required=True)
parser.add_argument('--test-exe', type=Path, required=True)
parser.add_argument('--media', type=Path, required=True)
parser.add_argument('--output-dir', type=Path, required=True)
args = parser.parse_args()
package, test_exe, output = args.package.resolve(), args.test_exe.resolve(), args.output_dir.resolve()
body = args.media.resolve().read_bytes()
assert body[:4] == b'RIFF' and body[8:12] == b'WAVE'
output.mkdir(parents=True, exist_ok=False)
requests = []


class Handler(BaseHTTPRequestHandler):
    protocol_version = 'HTTP/1.1'

    def serve(self, send_body):
        requests.append({'method': self.command, 'path': self.path, 'range': self.headers.get('Range')})
        if self.path.split('?')[0] != '/audio.wav':
            self.send_error(404)
            return
        start, end = 0, len(body) - 1
        header = self.headers.get('Range')
        if header:
            match = re.fullmatch(r'bytes=(\d*)-(\d*)', header)
            if not match:
                self.send_error(416)
                return
            if match[1]:
                start = int(match[1])
                if match[2]: end = min(end, int(match[2]))
            elif match[2]:
                start = max(0, len(body) - int(match[2]))
            if start > end:
                self.send_error(416)
                return
        self.send_response(206 if header else 200)
        self.send_header('Content-Type', 'audio/wav')
        self.send_header('Content-Length', str(end - start + 1))
        self.send_header('Accept-Ranges', 'bytes')
        self.send_header('Connection', 'close')
        if header: self.send_header('Content-Range', f'bytes {start}-{end}/{len(body)}')
        self.end_headers()
        if send_body:
            try: self.wfile.write(body[start:end + 1])
            except (BrokenPipeError, ConnectionResetError): pass

    def do_GET(self): self.serve(True)
    def do_HEAD(self): self.serve(False)
    def log_message(self, *args): pass


server = ThreadingHTTPServer(('127.0.0.1', 0), Handler)
server.daemon_threads = True
thread = threading.Thread(target=server.serve_forever, daemon=True)
thread.start()
environment = os.environ.copy()
environment['VLC_PLUGIN_PATH'] = str(package / 'plugins')
environment['VLC_LIBEXEC_PATH'] = str(package / 'vlc-preparser.exe')
environment['VLC_USERDATA_PATH'] = str(output / 'userdata')
environment['VLC_STREAM_FIXTURE_URL'] = f'http://127.0.0.1:{server.server_port}/audio.wav'
try:
    with (output / 'stdout.txt').open('w', encoding='utf-8', newline='\n') as stdout, \
         (output / 'stderr.txt').open('w', encoding='utf-8', newline='\n') as stderr:
        run = subprocess.run([str(test_exe), '-o', 'results.txt,txt'], cwd=output, env=environment, stdout=stdout, stderr=stderr,
                             timeout=55, creationflags=subprocess.CREATE_NO_WINDOW)
finally:
    server.shutdown()
    server.server_close()
    thread.join(2)
result = {'exitCode': run.returncode, 'fixtureSha256': hashlib.sha256(body).hexdigest(),
          'requests': requests, 'httpFixtureLoopbackOnly': True, 'isolatedUserdata': True,
          'libraryDiscovery': 'Native media library retains its normal LAN discovery behavior.'}
(output / 'result.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8', newline='\n')
report = (output / 'results.txt').read_text(encoding='utf-8', errors='replace')
print(report.strip())
if run.returncode:
    print((output / 'stderr.txt').read_text(encoding='utf-8', errors='replace')[-1500:])
else:
    assert any(r['method'] == 'GET' for r in requests), 'Native playback did not fetch loopback audio'
    for case in ['initTestCase', 'validationAndUnavailableLibrary', 'unnamedQueueOrderingAndDuplicateActivation',
                 'namedPlaylistSaveAndActualHttpPlayback', 'restartPersistence', 'cleanupTestCase']:
        assert f'PASS   : TestNetworkStream::{case}()' in report, f'Missing native case: {case}'
    assert '6 passed, 0 failed, 0 skipped' in report, 'Native suite did not fully execute'
    print('Passed: actual HTTP audio, ordered queue/save, duplicate guards and restart persistence.')
raise SystemExit(run.returncode)
