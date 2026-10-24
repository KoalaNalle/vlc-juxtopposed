#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/e/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-network-artwork-20261018"
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
export JOBS=1 MESON_BUILD='-j 1'
unset CC CXX
exec > >(tee "$work/build-diagnostic-r2.log") 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/build-diagnostic-r2-exit.txt"; exit "$code"' EXIT
make -C "$base/win64/modules/gui/qt" -f Makefile -f "$work/diagnostic.mk" -j1 QA_OUT="$work/native-diagnostic-r1" DIAGNOSTIC_SOURCE="$work/color_svg_image_provider_diagnostic.cpp" owned-native-ui
