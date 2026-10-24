#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/e/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-checks-20261017"
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
export JOBS=1 MESON_BUILD='-j 1'
unset CC CXX
exec > >(tee "$work/settings-ui-build-r5.log") 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/settings-ui-build-r5-exit.txt"; exit "$code"' EXIT
make -C "$base/win64/modules/gui/qt" -f Makefile -f "$repo/modules/gui/qt/tests/native_ui/extensions_journey.mk" -j1 QA_OUT="$work/settings-native-ui-r5" owned-native-ui
python3 - <<'CHECK'
from pathlib import Path
import hashlib,json
root=Path('/mnt/e/Modding/vlc-juxtopposed');work=root/'build-windows/beta-checks-20261017';ext=root/'build-windows/beta-extensions-20261017'
for file in [work/'gui-fixes-inputs-r3.json',work/'settings-ui-inputs-r5.json']:
 for name,digest in json.loads(file.read_text()).items():assert hashlib.sha256((root/name).read_bytes()).hexdigest()==digest,name
print('Private native settings quality GUI built; ordinary and private frozen inputs match.')
CHECK
