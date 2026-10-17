#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/e/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-checks-20261017"
recipes="$repo/modules/gui/qt/tests/native_library"
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
export JOBS=1 MESON_BUILD='-j 1'
unset CC CXX
exec > >(tee "$work/library-build-r1.log") 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/library-build-r1-exit.txt"; exit "$code"' EXIT
python3 "$recipes/prepare.py" --output-dir "$work/library-prepared-r1"
make -C "$base/win64/modules/gui/qt" -f Makefile -f "$recipes/preferences.mk" -j1 library-preferences-native PENDING="$work/library-prepared-r1/input" SHARED_STORE="$repo/modules/gui/qt" PROPOSAL_OUT="$work/library-native-r1"
make -C "$base/win64/modules" -f Makefile -f "$recipes/modules.mk" -j1 library-private-fixture-modules PENDING="$repo/modules/gui/qt" WINMSG_SOURCE="$work/library-prepared-r1/input/tests/win_msg.c" LIBRARY_MODULE_OUT="$work/library-native-r1/modules"
make -C "$base/win64/modules/gui/qt" -f Makefile -f "$recipes/process.mk" -j1 library-process-native PROCESS_OUT="$work/library-native-r1/process"
python3 - <<'CHECK'
from pathlib import Path
import hashlib,json
root=Path('/mnt/e/Modding/vlc-juxtopposed');work=root/'build-windows/beta-checks-20261017'
inputs=json.loads((work/'library-inputs-r1.json').read_text())
for name,digest in inputs.items():assert hashlib.sha256((root/name).read_bytes()).hexdigest()==digest,name
print('Fresh Library test fixtures built; all frozen source inputs match.')
CHECK
