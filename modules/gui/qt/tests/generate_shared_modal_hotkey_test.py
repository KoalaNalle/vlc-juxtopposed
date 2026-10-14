# SPDX-License-Identifier: GPL-2.0-or-later
"""Extract exact production key-routing methods; no duplicated test policy."""
import argparse
import hashlib
import json
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--repo-root', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
root = args.repo_root.resolve()
sources = {name: (root / name).read_text(encoding='utf-8') for name in [
    'modules/gui/qt/maininterface/interface_window_handler.cpp',
    'modules/gui/qt/maininterface/mainctx.cpp']}

def method(text, signature):
    if text.count(signature) != 1:
        raise SystemExit('Expected one production definition: ' + signature)
    begin = text.index(signature)
    brace = text.index('{', begin)
    depth, end = 0, brace
    while end < len(text):
        if text.startswith('//', end):
            end = text.find('\n', end)
            if end < 0: break
        elif text.startswith('/*', end):
            close = text.find('*/', end + 2)
            if close < 0: break
            end = close + 2
            continue
        elif text[end] in ['"', "'"]:
            quote = text[end]
            end += 1
            while end < len(text) and text[end] != quote:
                end += 2 if text[end] == '\\' else 1
        elif text[end] == '{': depth += 1
        elif text[end] == '}':
            depth -= 1
            if not depth: return text[begin:end + 1] + '\n'
        end += 1
    raise SystemExit('Unterminated production method: ' + signature)

handler = sources['modules/gui/qt/maininterface/interface_window_handler.cpp']
context = sources['modules/gui/qt/maininterface/mainctx.cpp']
begin, end = '    case QEvent::KeyPress:', '    case QEvent::Wheel:'
if handler.count(begin) != 1 or handler.count(end) != 1:
    raise SystemExit('Expected the single production key-event branch')
branch = handler[handler.index(begin):handler.index(end)]
identity = {name: hashlib.sha256((root / name).read_bytes()).hexdigest() for name in sources}
output = '// Exact production method/branch extraction; dependency adapters are in the test.\n'
output += '// SOURCE_SHA256 ' + json.dumps(identity, sort_keys=True) + '\n'
output += 'static const char routingProductionIdentity[] = R"routing(' + json.dumps(identity, sort_keys=True) + ')routing";\n'
output += method(handler, 'bool InterfaceWindowHandler::applyKeyEvent(QKeyEvent * event) const')
output += method(context, 'void MainCtx::sendHotkey(Qt::Key key , Qt::KeyboardModifiers modifiers)')
output += method(context, 'void MainCtx::sendVLCHotkey(int vlcHotkey)')
output += 'bool InterfaceWindowHandler::eventFilter(QObject *, QEvent *event)\n{\n    switch (event->type())\n    {\n'
output += branch + '    default: return false;\n    }\n}\n'
data = output.encode()
args.output.parent.mkdir(parents=True, exist_ok=True)
if not args.output.exists() or args.output.read_bytes() != data:
    args.output.write_bytes(data)
manifest = {'sourceSha256': identity, 'includeSha256': hashlib.sha256(data).hexdigest(),
    'scope': 'Exact production key-only event branch, applyKeyEvent, sendHotkey and sendVLCHotkey. Other window/interface methods are not extracted.'}
args.output.with_suffix(args.output.suffix + '.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8', newline='\n')
print(json.dumps(manifest, indent=2))
