#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Prepare private native Library test sources from this checkout.

The shipping Core and plugins remain untouched. Exact production decision
helpers are compiled into the test EXE, while a private receiver plugin adds
one test entry point. Use a fresh output directory.
"""
import argparse
from hashlib import sha256
import json
from pathlib import Path
import shutil

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output-dir', type=Path, required=True)
args = parser.parse_args()
root = Path(__file__).resolve().parents[5]
out = args.output_dir.resolve()
out.mkdir(parents=True, exist_ok=False)
inputs = out / 'input'

def between(text, first, last):
    start = text.index(first)
    return text[start:text.index(last, start + len(first))]

def save(path, text):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding='utf-8', newline='\n')

wrapper = r'''
/* Private test-only entry; no exported diagnostics in the shipping plugin. */
__declspec(dllexport) LRESULT vlc_test_library_copydata(vlc_object_t *root,
                                                     const void *bytes, size_t size);
LRESULT vlc_test_library_copydata(vlc_object_t *root, const void *bytes, size_t size)
{
    if (!bytes || size > UINT32_MAX) return -1;
    intf_thread_t *intf = vlc_object_create(root, sizeof(*intf));
    if (!intf) return -1;
    HWND window = CreateWindowW(L"STATIC", L"Owned Library policy fixture only",
        0, 0, 0, 0, 0, NULL, NULL, GetModuleHandle(NULL), NULL);
    if (!window) { vlc_object_delete(intf); return -1; }
    SetWindowLongPtr(window, GWLP_USERDATA, (LONG_PTR)intf);
    COPYDATASTRUCT data = { .dwData = 0, .cbData = (DWORD)size, .lpData = (void *)bytes };
    LRESULT result = WMCOPYWNDPROC(window, WM_COPYDATA, 0, (LPARAM)&data);
    SetWindowLongPtr(window, GWLP_USERDATA, 0);
    DestroyWindow(window);
    vlc_object_delete(intf);
    return result;
}

'''
policy_header = '''#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <vlc_common.h>
#include "libvlc.h"
'''
policy_exports = '''
bool vlc_test_library_single_instance(vlc_object_t *obj);
bool vlc_test_library_single_instance(vlc_object_t *obj)
{ return LibrarySingleInstance(vlc_object_instance(obj)); }
int vlc_test_library_open_mode(vlc_object_t *obj);
int vlc_test_library_open_mode(vlc_object_t *obj)
{ return LibraryOpenMode(vlc_object_instance(obj)); }
'''
manifest = {'scope': 'Current production policy bodies and private receiver entry; no shipping diagnostic exports', 'sha256': {}}
for relative in ('tests/test_library_preferences.cpp', 'tests/library_preferences_probe.c',
                 'maininterface/library_preferences_model.cpp', 'maininterface/library_preferences_model.hpp'):
    source = root / 'modules/gui/qt' / relative
    target = inputs / relative
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(source, target)
sender = (root / 'src/win32/specific.c').read_text(encoding='utf-8')
policies = between(sender, 'static bool LibrarySingleInstance', 'static void LibraryFreeArguments')
save(inputs / 'tests/library_policies.c', policy_header + policies + policy_exports)
receiver = (root / 'modules/control/win_msg.c').read_text(encoding='utf-8')
marker = 'static void *HelperThread'
assert receiver.count(marker) == 1
save(inputs / 'tests/win_msg.c', receiver.replace(marker, wrapper + marker, 1))
for path in inputs.rglob('*'):
    if path.is_file(): manifest['sha256'][str(path.relative_to(out))] = sha256(path.read_bytes()).hexdigest()
save(out / 'source-identities.json', json.dumps(manifest, indent=2) + '\n')
print('Private Library native inputs prepared: ' + str(inputs))
