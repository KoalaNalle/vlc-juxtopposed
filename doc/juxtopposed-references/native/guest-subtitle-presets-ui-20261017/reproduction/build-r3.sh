#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/e/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-presets-20261017"
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
export JOBS=1 MESON_BUILD='-j 1'
unset CC CXX
exec > >(tee "$work/build-r3.log") 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/build-r3-exit.txt"; exit "$code"' EXIT
make -C "$base/win64/modules/gui/qt" -j1 libqt_plugin.la
mkdir "$work/ordinary-native-r2"
cp "$base/win64/modules/gui/qt/.libs/libqt_plugin.dll" "$work/ordinary-native-r2/"
cp "$base/win64/src/.libs/libvlccore.dll" "$work/ordinary-native-r2/"
make -C "$base/win64/modules/gui/qt" -f Makefile -f "$repo/modules/gui/qt/tests/native_ui/subtitle_presets_journey.mk" -j1 QA_OUT="$work/native-ui-r3" owned-native-ui
python3 - <<'CHECK'
from pathlib import Path
import hashlib,json
from mesonbuild.mparser import Parser
r=Path('/mnt/e/Modding/vlc-juxtopposed');w=r/'build-windows/beta-presets-20261017'
for file in [w/'ordinary-inputs-r2.json',w/'ui-inputs-r3.json']:
 for n,h in json.loads(file.read_text()).items():assert hashlib.sha256((r/n).read_bytes()).hexdigest()==h,n
Parser((r/'modules/gui/qt/meson.build').read_text(),'modules/gui/qt/meson.build').parse()
print('Ordinary GUI and private preset UI rebuilt sequentially; unchanged native targets retained; all inputs match. Meson syntax passes.')
CHECK
