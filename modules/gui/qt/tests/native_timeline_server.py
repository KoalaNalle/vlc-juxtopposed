# SPDX-License-Identifier: GPL-2.0-or-later
"""Owned loopback-only HLS fixture server; stdin EOF/stop gives a normal exit."""
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path
import sys
from threading import Thread

fixtures = Path(sys.argv[1]).resolve()
playlist = Path(sys.argv[2]).resolve()
segments = {"/" + file.name: file.read_bytes() for file in fixtures.glob("*.ts")}
requests = 0


class Handler(BaseHTTPRequestHandler):
    def log_message(self, *args):
        pass

    def do_GET(self):
        global requests
        if self.path == "/live.m3u8":
            requests += 1
            print(f"PLAYLIST {requests}", flush=True)
            body = playlist.read_bytes()
            content_type = "application/vnd.apple.mpegurl"
        elif self.path in segments:
            body = segments[self.path]
            content_type = "video/mp2t"
        else:
            self.send_error(404)
            return
        self.send_response(200)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-cache")
        self.end_headers()
        try:
            self.wfile.write(body)
        except (BrokenPipeError, ConnectionResetError, ConnectionAbortedError):
            pass  # A native seek/stop may cancel an in-flight fixture response.


with HTTPServer(("127.0.0.1", 0), Handler) as server:
    def stop_on_stdin():
        for line in sys.stdin:
            if line.strip() == "stop":
                break
        server.shutdown()

    Thread(target=stop_on_stdin, daemon=True).start()
    print(f"PORT {server.server_port}", flush=True)
    server.serve_forever(poll_interval=0.1)
