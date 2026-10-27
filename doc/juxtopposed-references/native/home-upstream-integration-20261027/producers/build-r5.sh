#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/c/Modding/vlc-juxtopposed
work="$repo/build-windows/upstream-integration-20261027"
out="$work/build-r5"
test ! -e "$out"
mkdir "$out"
exec > "$out/build.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$out/exit.txt"; exit "$code"' EXIT
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
unset CC CXX
date -u +%FT%TZ > "$out/start.txt"
printf '%s\n' c7cdbdb5d7ed8bd7023252e88fad8a5344ea14c9 > "$out/source-commit.txt"
# Source cleanliness is checked by the Windows parent before and after.
# Index cleanliness is checked by the Windows parent before and after.
sha256sum "$work/build-r5.sh" > "$out/producer.sha256"
"$base/win64/config.status" --config > "$out/config-before.txt"
make -C "$base/win64" -j2 V=0
make -C "$base/win64" -j2 V=0 DESTDIR="$out/install" install
make -C "$base/win64/test" -f Makefile -f "$repo/test/mmdevice-native.mk" -j2 MMDEVICE_OUT="$base/owned-upstream-mmdevice-r5" mmdevice-stop-native
make -C "$base/win64/test" -f Makefile -f "$repo/test/record-start-native.mk" -j2 RECORD_START_OUT="$base/owned-upstream-record-r5" owned-record-start
make -C "$base/win64/modules/gui/qt" -j2 test_qt_csd_button_model.exe
mkdir "$out/probes"
cp "$base/owned-upstream-mmdevice-r5/.libs/test_mmdevice_stop.exe" "$out/probes/"
cp "$base/owned-upstream-record-r5/.libs/test_record_start.exe" "$out/probes/"
cp "$base/win64/modules/gui/qt/.libs/test_qt_csd_button_model.exe" "$out/probes/"
sha256sum "$out/probes/"* > "$out/probes.sha256"
# Source cleanliness is checked by the Windows parent before and after.
# Index cleanliness is checked by the Windows parent before and after.
sha256sum -c "$out/producer.sha256"
date -u +%FT%TZ > "$out/end.txt"
