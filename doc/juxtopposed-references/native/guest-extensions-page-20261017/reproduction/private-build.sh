#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/e/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-extensions-20261017"
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
export JOBS=1 MESON_BUILD='-j 1'
unset CC CXX
exec > >(tee "$work/ui-build-r10.log") 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/ui-build-r10-exit.txt"; exit "$code"' EXIT
make -C "$base/win64/modules/gui/qt" -f Makefile -f "$repo/modules/gui/qt/tests/native_ui/extensions_journey.mk" -j1 QA_OUT="$work/page-native-ui-r10" owned-native-ui
python3 - <<'CHECK'
from pathlib import Path
import hashlib,json
root=Path('/mnt/e/Modding/vlc-juxtopposed');work=root/'build-windows/beta-extensions-20261017'
for n,v in json.loads((work/'page-inputs-r5.json').read_text()).items():assert hashlib.sha256((root/n).read_bytes()).hexdigest()==v,n
for n,v in json.loads((work/'ui-inputs-r10.json').read_text()).items():assert hashlib.sha256((root/n).read_bytes()).hexdigest()==v,n
print('Ordinary page and private fixture frozen inputs match.')
CHECK
