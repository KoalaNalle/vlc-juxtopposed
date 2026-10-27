#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/c/Modding/vlc-juxtopposed
work="$repo/build-windows/upstream-integration-20261027"
out="$work/csd-r10"
test ! -e "$out"
mkdir "$out"
exec > "$out/build.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$out/exit.txt"; exit "$code"' EXIT
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
unset CC CXX
sha256sum "$work/csd-r10.sh" "$repo/modules/gui/qt/tests/test_qt_csd_button_model.cpp" "$repo/modules/gui/qt/util/csdbuttonmodel.cpp" "$repo/modules/gui/qt/util/csdbuttonmodel.hpp" "$repo/modules/gui/qt/util/winpthreads_shutdown.cpp" > "$out/inputs.sha256"
make -C "$base/win64/modules/gui/qt" -j2 V=0 test_qt_csd_button_model.exe
cp "$base/win64/modules/gui/qt/.libs/test_qt_csd_button_model.exe" "$out/"
sha256sum -c "$out/inputs.sha256"
sha256sum "$out/test_qt_csd_button_model.exe"
