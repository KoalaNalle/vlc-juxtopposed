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
exec > >(tee "$work/page-build-r5.log") 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/page-build-r5-exit.txt"; exit "$code"' EXIT
make -C "$base/win64/src" -j1 libvlccore.la
make -C "$base/win64/modules/gui/qt" -j1 libqt_plugin.la
mkdir "$work/page-native-r5"
cp "$base/win64/src/.libs/libvlccore.dll" "$work/page-native-r5/"
cp "$base/win64/modules/gui/qt/.libs/libqt_plugin.dll" "$work/page-native-r5/"
python3 - <<'CHECK'
from pathlib import Path
import hashlib,json
from mesonbuild.mparser import Parser
root=Path('/mnt/e/Modding/vlc-juxtopposed');work=root/'build-windows/beta-extensions-20261017'
snapshot=json.loads((work/'page-inputs-r5.json').read_text())
for name,digest in snapshot.items():assert hashlib.sha256((root/name).read_bytes()).hexdigest()==digest,name
Parser((root/'modules/gui/qt/meson.build').read_text(),'modules/gui/qt/meson.build').parse()
print('All',len(snapshot),'frozen page inputs match; Meson syntax passes.')
CHECK
