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
exec > >(tee "$work/events-build-r4.log") 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/events-build-r4-exit.txt"; exit "$code"' EXIT
make -C "$base/win64/src" -j1 libvlccore.la
make -C "$base/win64/modules" -j1 libaddonsfsstorage_plugin.la libaddonsvorepository_plugin.la
make -C "$base/win64/test" -f Makefile -f "$repo/test/addons-native.mk" -j1 ADDONS_OUT="$work/events-native-r4" addons-native
cp "$base/win64/src/.libs/libvlccore.dll" "$work/events-native-r4/"
cp "$base/win64/modules/.libs/libaddonsfsstorage_plugin.dll" "$work/events-native-r4/"
cp "$base/win64/modules/.libs/libaddonsvorepository_plugin.dll" "$work/events-native-r4/"
python3 - <<'CHECK'
from pathlib import Path
import hashlib,json
repo=Path('/mnt/e/Modding/vlc-juxtopposed');work=repo/'build-windows/beta-extensions-20261017'
snapshot=json.loads((work/'events-inputs-r4.json').read_text())
for name,digest in snapshot.items():assert hashlib.sha256((repo/name).read_bytes()).hexdigest()==digest,name
print('All',len(snapshot),'frozen inputs match.')
CHECK
