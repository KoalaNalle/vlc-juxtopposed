#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Execute the actual HTTP UI script in Qt's JS engine with an owned DOM seam."""
import argparse
import hashlib
import json
from pathlib import Path
from PySide6.QtCore import QCoreApplication
from PySide6.QtQml import QJSEngine

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--repo-root', type=Path, required=True)
parser.add_argument('--output-dir', type=Path, required=True)
args = parser.parse_args()
script = args.repo_root.resolve() / 'share/lua/http/js/ui.js'
output = args.output_dir.resolve(); output.mkdir(parents=True, exist_ok=False)
app = QCoreApplication([])
engine = QJSEngine()

def evaluate(source):
    value = engine.evaluate(source)
    assert not value.isError(), value.toString()
    return value

evaluate('''
var ready, handler, listenerOptions, timers = [], sent = [], current_que = 'main';
var area = { addEventListener: function(type, cb, options) {
    if (type !== 'wheel') throw Error('Unexpected listener');
    handler = cb; listenerOptions = options;
}};
var document = { getElementById: function(id) { return id === 'controlContainer' ? area : null; }};
var window = { setTimeout: function(cb, delay) { timers.push({cb:cb, delay:delay}); return timers.length; }};
function sendCommand(command) { sent.push(command); }
function $(arg) {
    if (typeof arg === 'function') { ready = arg; return; }
    // Stop before the independent legacy slider setup. The complete original
    // file is parsed and its original ready callback is executed unchanged.
    if (arg === '#seekSlider') throw Error('Owned slider boundary');
    return { closest: function(selector) {
        if (selector !== 'input, select, textarea, button, [contenteditable], .ui-slider')
            throw Error('Unexpected form exclusion selector');
        return {length:arg && arg.form ? 1 : 0};
    }};
}
function flush() { var pending = timers; timers = []; pending.forEach(function(t) { t.cb(); }); }
function reset() { flush(); sent = []; current_que = 'main'; }
function wheel(x, y, mode, flags) {
    var e = { deltaX:x, deltaY:y, deltaMode:mode || 0, target:{}, prevented:false,
              preventDefault:function() { this.prevented = true; }};
    Object.keys(flags || {}).forEach(function(key) { e[key] = flags[key]; });
    handler(e); return e;
}
function check(ok, message) { if (!ok) throw Error(message); }
''')
evaluate(script.read_text(encoding='utf-8'))
evaluate("try { ready(); } catch (error) { if (error.message !== 'Owned slider boundary') throw error; }")
checks = []
def case(name, source):
    evaluate(source); checks.append(name); print('PASS', name, flush=True)

case('Wheel listener is non-passive and high-resolution deltas are batched', '''
check(typeof handler === 'function' && listenerOptions.passive === false, 'listener');
wheel(0,-15); wheel(0,-15); check(sent.length === 0 && timers.length === 1 && timers[0].delay === 80, 'batch');
flush(); check(sent.length === 0, 'fractional delta');
wheel(0,-10); flush(); check(sent.length === 1 && sent[0].axis === 'y' && sent[0].steps === 1, 'accumulation');
''')
case('Both axes retain direction and line/page deltas are bounded to eight steps', '''
reset(); wheel(-80,120); flush();
check(sent.length === 2 && sent[0].axis === 'x' && sent[0].steps === -2 && sent[1].axis === 'y' && sent[1].steps === -3, 'direction');
reset(); wheel(0,-2,1); flush(); check(sent.length === 1 && sent[0].steps === 2, 'line');
reset(); wheel(0,-100,2); flush(); check(sent.length === 1 && sent[0].steps === 8, 'page limit');
reset(); wheel(1e9,-1e9); flush(); check(sent.length === 2 && sent.every(function(c) { return c.steps === 8; }), 'pixel limit');
''')
case('Forms, sliders, zoom modifiers and other queues retain their native gestures', '''
reset();
['ctrlKey','metaKey','altKey','shiftKey'].forEach(function(key) {
    var flags = {}; flags[key] = true; check(!wheel(0,-40,0,flags).prevented, key);
});
check(!wheel(0,-40,0,{target:{form:true}}).prevented, 'form');
current_que = 'stream'; check(!wheel(0,-40).prevented, 'stream'); current_que = 'main';
check(timers.length === 0 && sent.length === 0, 'excluded gesture scheduled');
''')
case('Non-finite input and a queue change cannot send a wheel command', '''
reset(); check(!wheel(NaN,0).prevented && !wheel(0,Infinity).prevented, 'nonfinite');
check(timers.length === 0, 'invalid scheduled');
wheel(0,-40); current_que = 'stream'; flush(); check(sent.length === 0, 'stale queue');
current_que = 'main'; wheel(0,-40); flush();
check(sent.length === 1 && sent[0].command === 'juxtopposed_wheel' && sent[0].steps === 1, 'fresh command');
''')
report = dict(passed=True, checks=checks, sourceSha256=hashlib.sha256(script.read_bytes()).hexdigest(),
              engine='Qt QJSEngine', actualDomBrowserAccepted=False, nativeServerAccepted=False)
(output / 'verification.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8', newline='\n')
