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
exec > >(tee "$work/subtitles-build-r1.log") 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/subtitles-build-r1-exit.txt"; exit "$code"' EXIT
make -C "$base/win64/modules/gui/qt" -j1 test_qt_subtitles_preferences.exe
python3 - <<'CHECK'
from pathlib import Path
import hashlib,json,shutil
root=Path('/mnt/e/Modding/vlc-juxtopposed');work=root/'build-windows/beta-checks-20261017'
for name,digest in json.loads((work/'subtitles-inputs-r1.json').read_text()).items():assert hashlib.sha256((root/name).read_bytes()).hexdigest()==digest,name
out=work/'subtitles-native-r1';out.mkdir(exist_ok=False)
source=Path('/root/vlc-juxtopposed-build/win64/modules/gui/qt/.libs/test_qt_subtitles_preferences.exe')
shutil.copyfile(source,out/source.name)
print('Standard subtitle test target staged on E:; frozen source inputs match.')
CHECK
