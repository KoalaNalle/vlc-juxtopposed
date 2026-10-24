#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Exercise the real authenticated Lua HTTP remote on an owned loopback listener."""
import argparse
import base64
import ctypes
import hashlib
import json
import os
from pathlib import Path
import queue
import shutil
import socket
import subprocess
import threading
import time
import urllib.error
import urllib.parse
import urllib.request
import wave

parser = argparse.ArgumentParser(description=__doc__)
for name in ('package', 'core', 'hotkeys', 'lua', 'http-lua', 'httprequests-lua', 'http-ui', 'executable', 'output-dir'):
    parser.add_argument('--' + name, type=Path, required=True)
args = parser.parse_args()
output = args.output_dir.resolve()
output.mkdir(parents=True, exist_ok=False)
runtime = output / 'runtime'
shutil.copytree(args.package, runtime)
assert (runtime / 'portable').is_dir()
shutil.copy2(args.core, runtime / 'libvlccore.dll')
shutil.copy2(args.hotkeys, runtime / 'plugins/control/libhotkeys_plugin.dll')
lua_plugin, = runtime.glob('plugins/**/liblua_plugin.dll')
shutil.copy2(args.lua, lua_plugin)
shutil.copy2(args.http_lua, runtime / 'lua/intf/http.luac')
shutil.copy2(args.httprequests_lua, runtime / 'lua/intf/modules/httprequests.luac')
shutil.copy2(args.http_ui, runtime / 'lua/http/js/ui.js')
shutil.copy2(args.executable, runtime / 'test_controls_http.exe')
media = output / 'owned-silent.wav'
with wave.open(str(media), 'wb') as stream:
    stream.setnchannels(1); stream.setsampwidth(2); stream.setframerate(8000)
    stream.writeframes(b'\0\0' * (60 * 8000))
# Owned test data only; never receive or inspect a person's credentials.
password = 'owned-controls-loopback-fixture'
opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))
kernel = ctypes.WinDLL('kernel32', use_last_error=True)
kernel.SetErrorMode.argtypes = [ctypes.c_uint]; kernel.SetErrorMode.restype = ctypes.c_uint
report = dict(passed=False, expectedChecks=29, checks={}, processes={}, runtimeSha256={})
for path in (runtime / 'libvlccore.dll', runtime / 'plugins/control/libhotkeys_plugin.dll', runtime / 'test_controls_http.exe', lua_plugin,
             runtime / 'lua/intf/http.luac', runtime / 'lua/intf/modules/httprequests.luac', runtime / 'lua/http/js/ui.js'):
    report['runtimeSha256'][path.relative_to(runtime).as_posix()] = hashlib.sha256(path.read_bytes()).hexdigest()

def check(condition, name):
    report['checks'][name] = bool(condition)
    print(json.dumps(dict(check=name, passed=bool(condition))), flush=True)
    assert condition, name

def free_port():
    with socket.socket() as listener:
        listener.bind(('127.0.0.1', 0))
        return listener.getsockname()[1]

class Driver:
    def __init__(self, name, port, secret):
        self.name, self.port = name, port
        self.case = output / name; self.case.mkdir()
        for folder in ('temp', 'userdata', 'appdata', 'localappdata'):
            (self.case / folder).mkdir()
        env = {key: value for key, value in os.environ.items() if not key.startswith('VLC_')}
        env.update(VLC_PLUGIN_PATH=str(runtime / 'plugins'), VLC_USERDATA_PATH=str(self.case / 'userdata'),
                   TEMP=str(self.case / 'temp'), TMP=str(self.case / 'temp'), APPDATA=str(self.case / 'appdata'),
                   LOCALAPPDATA=str(self.case / 'localappdata'), CONTROLS_FIXTURE_HTTP_PASSWORD=secret)
        self.log = (self.case / 'stderr.txt').open('wb')
        self.stdout_log = (self.case / 'stdout.txt').open('wb')
        self.ready = self.case / 'ready.txt'
        self.result = dict(forcedCleanupUsed=False, normalExit=False)
        report['processes'][name] = self.result
        previous = kernel.SetErrorMode(0x8003)
        try:
            self.child = subprocess.Popen([str(runtime / 'test_controls_http.exe'), str(port), media.as_uri(), str(self.ready)],
                cwd=runtime, env=env, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=self.log,
                creationflags=subprocess.CREATE_NO_WINDOW)
        finally:
            kernel.SetErrorMode(previous)
        self.result['ownedPid'] = self.child.pid
        self.lines = queue.Queue()
        def read_output():
            for line in iter(self.child.stdout.readline, b''):
                self.stdout_log.write(line); self.stdout_log.flush()
                self.lines.put(line.decode('utf-8').strip())
        self.reader = threading.Thread(target=read_output, daemon=True)
        self.reader.start()
    def state(self):
        deadline = time.monotonic() + 35
        while not self.ready.exists():
            assert self.child.poll() is None, self.name + ' exited before readiness: ' + str(self.child.returncode)
            assert time.monotonic() < deadline, self.name + ' readiness timeout'
            time.sleep(.05)
        return self.ready.read_text(encoding='utf-8').strip()
    def command(self, command):
        self.child.stdin.write((command + '\n').encode('ascii')); self.child.stdin.flush()
        return self.lines.get(timeout=5)
    def request(self, path='/requests/status.json', auth=password):
        request = urllib.request.Request('http://127.0.0.1:' + str(self.port) + path)
        if auth is not None:
            request.add_header('Authorization', 'Basic ' + base64.b64encode((':' + auth).encode()).decode())
        try:
            with opener.open(request, timeout=5) as response:
                return response.status, response.read()
        except urllib.error.HTTPError as error:
            return error.code, error.read()
    def status(self, **command):
        code, body = self.request('/requests/status.json' + ('?' + urllib.parse.urlencode(command) if command else ''))
        assert code == 200, 'authenticated status failed: ' + str(code)
        return json.loads(body)
    def seek(self, seconds):
        assert self.command('SEEK ' + str(seconds)) == 'OK'
        deadline = time.monotonic() + 5
        while True:
            value = self.status()['time']
            if abs(value - seconds) < .5: return
            assert time.monotonic() < deadline, 'paused seek did not settle'
            time.sleep(.02)
    def wheel(self, axis, steps):
        return self.status(command='juxtopposed_wheel', axis=axis, steps=steps)
    def close(self):
        if self.child.poll() is None:
            self.child.stdin.write(b'QUIT\n'); self.child.stdin.flush()
            try: self.child.wait(timeout=10)
            except subprocess.TimeoutExpired:
                self.result['forcedCleanupUsed'] = True
                self.child.kill(); self.child.wait(timeout=5)
        self.reader.join(timeout=2)
        self.result.update(exitCode=self.child.returncode,
            normalExit=self.child.returncode == 0 and not self.result['forcedCleanupUsed'])
        self.stdout_log.close(); self.log.close()
        return self.result['normalExit']

driver = None
try:
    port = free_port(); driver = Driver('normal', port, password)
    check(driver.state() == 'active', 'authenticated-service-active')
    check(driver.request(auth=None)[0] == 401, 'unauthenticated-request-rejected')
    check(driver.request(auth='wrong-owned-password')[0] == 401, 'wrong-password-rejected')
    code, body = driver.request('/')
    check(code == 200 and b'id="controlContainer"' in body and b'src="js/ui.js"' in body,
          'authenticated-browser-page-mounts-wheel-target-and-script')
    code, body = driver.request('/js/ui.js')
    check(code == 200 and hashlib.sha256(body).hexdigest() == report['runtimeSha256']['lua/http/js/ui.js'],
          'real-service-serves-exact-tested-browser-wheel-script')
    data = driver.status(command='pl_forcepause')
    deadline = time.monotonic() + 5
    while data['state'] != 'paused' and time.monotonic() < deadline:
        time.sleep(.02); data = driver.status()
    report['initialAudioState'] = {key: data.get(key) for key in ('state', 'volume', 'time', 'length')}
    check(data['state'] == 'paused' and abs(data['volume'] - 128) <= 1, 'real-silent-audio-paused-at-known-volume')
    initial = data['volume']
    check(driver.command('LINK') == 'LINK 1', 'wheel-link-is-on-by-default')
    driver.seek(10)
    data = driver.wheel('y', 1)
    check(abs(data['time'] - 13) < .5 and data['volume'] == initial,
          'default-linked-remote-uses-active-physical-vertical-seek')
    check(abs(driver.wheel('x', 1)['time'] - 13) < .5,
          'default-linked-remote-uses-active-physical-horizontal-ignore')
    assert driver.command('SAVED y 0 1') == 'OK'; driver.seek(10)
    data = driver.wheel('y', 1)
    check(abs(data['time'] - 13) < .5 and data['volume'] == initial,
          'pending-saved-mouse-mapping-does-not-change-linked-remote-before-restart')
    assert driver.command('LINK 0') == 'OK'
    data = driver.wheel('y', 1)
    deadline = time.monotonic() + 3
    while data['volume'] <= initial and time.monotonic() < deadline:
        time.sleep(.02); data = driver.status()
    report['firstRemoteVolume'] = data['volume']
    check(data['volume'] > initial, 'remote-vertical-volume-with-physical-vertical-seek')
    assert driver.command('MAP y 0 1') == 'OK'
    data = driver.wheel('y', 1)
    deadline = time.monotonic() + 3
    while abs(data['volume'] - initial) > 1 and time.monotonic() < deadline:
        time.sleep(.02); data = driver.status()
    check(abs(data['volume'] - initial) <= 1, 'remote-reverse-volume-applies-live')
    assert driver.command('LINK 1') == 'OK'; driver.seek(10)
    data = driver.wheel('y', 1)
    check(abs(data['time'] - 13) < .5 and abs(data['volume'] - initial) <= 1,
          'relink-restores-active-mouse-policy-despite-pending-config')
    assert driver.command('LINK 0') == 'OK'
    data = driver.wheel('y', 1)
    deadline = time.monotonic() + 3
    while data['volume'] >= initial and time.monotonic() < deadline:
        time.sleep(.02); data = driver.status()
    check(data['volume'] < initial and abs(data['time'] - 13) < .5,
          'unlink-restores-independent-reversed-volume-without-overwriting-it')
    assert driver.command('MAP x 2 0') == 'OK'; driver.seek(10)
    data = driver.wheel('x', 1)
    # The paused UI reports the pending seek point. The resumed clock is
    # checked independently through the native driver below.
    deadline = time.monotonic() + 5
    while abs(data['time'] - 13) >= .5 and time.monotonic() < deadline:
        time.sleep(.02); data = driver.status()
    check(abs(data['time'] - 13) < .5, 'remote-horizontal-seek-with-physical-horizontal-ignore')
    assert driver.command('MAP x 3 0') == 'OK'; driver.seek(10); driver.wheel('x', 1)
    time.sleep(.08)
    check(abs(driver.status()['time'] - 7) < .5, 'legacy-remote-seek-direction')
    assert driver.command('MAP x 3 1') == 'OK'; driver.seek(10); driver.wheel('x', 1)
    time.sleep(.08)
    check(abs(driver.status()['time'] - 13) < .5, 'reverse-composes-with-legacy-seek-direction')
    assert driver.command('MAP x -1 0') == 'OK'; driver.seek(10)
    check(abs(driver.wheel('x', 1)['time'] - 10) < .5, 'ignored-remote-axis-has-no-effect')
    before = driver.status()
    for axis, steps in [('z', 1), ('y', 'nan'), ('y', 'inf'), ('y', 9), ('y', -9), ('y', 1.5), ('y', 0)]:
        driver.wheel(axis, steps)
    after = driver.status()
    check(abs(after['time'] - before['time']) < .5 and after['volume'] == before['volume'], 'malformed-and-unbounded-wheel-inputs-have-no-effect')
    # This browser-facing response must not include the configured password.
    check(password.encode() not in driver.request()[1], 'status-response-does-not-expose-password')
    driver.status(command='pl_forceresume')
    deadline = time.monotonic() + 5
    actual_time = int(driver.command('TIME').split()[1]) / 1000000
    while not 10 <= actual_time < 13 and time.monotonic() < deadline:
        time.sleep(.05); actual_time = int(driver.command('TIME').split()[1]) / 1000000
    report['resumedNativeClockSeconds'] = actual_time
    check(10 <= actual_time < 13, 'resume-renders-at-requested-paused-seek-position')
    check(driver.close(), 'normal-service-shutdown'); driver = None

    driver = Driver('password-empty', free_port(), '')
    check(driver.state() == 'password-required', 'empty-password-state-is-explicit')
    check(driver.request('/', auth=None)[0] == 403 and driver.request(auth=None)[0] != 200,
          'empty-password-does-not-enable-player-commands')
    check(driver.close(), 'password-required-service-shutdown'); driver = None

    with socket.socket() as blocker:
        blocker.setsockopt(socket.SOL_SOCKET, socket.SO_EXCLUSIVEADDRUSE, 1)
        blocker.bind(('127.0.0.1', 0)); blocker.listen(1)
        occupied = blocker.getsockname()[1]
        driver = Driver('occupied-port', occupied, password)
        check(driver.state() == 'failed', 'occupied-port-reports-real-service-failure')
        check(driver.close(), 'failed-service-shutdown'); driver = None
    driver = Driver('retry-after-port-released', occupied, password)
    check(driver.state() == 'active' and driver.request()[0] == 200, 'fresh-retry-after-port-released')
    check(driver.close(), 'retried-service-shutdown'); driver = None
    assert len(report['checks']) == report['expectedChecks'], 'Unexpected HTTP check count: ' + str(len(report['checks']))
    report['passed'] = True
except Exception as error:
    report['error'] = str(error)
finally:
    if driver is not None: driver.close()
    report['passed'] = report['passed'] and all(p['normalExit'] for p in report['processes'].values())
    (output / 'verification.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8', newline='\n')
print(json.dumps(dict(passed=report['passed'], checks=len(report['checks']))), flush=True)
raise SystemExit(0 if report['passed'] else 1)
