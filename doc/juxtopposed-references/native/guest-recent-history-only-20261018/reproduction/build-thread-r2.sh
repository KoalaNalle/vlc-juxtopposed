#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/e/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-recent-history-only-20261018"
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
export JOBS=1 MESON_BUILD='-j 1'
unset CC CXX
exec > "$work/build-thread-r2.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/build-thread-r2-exit.txt"; exit "$code"' EXIT
make -C "$base/win64/modules/gui/qt" -j1 tests/test_thread_runner.moc test_thread_runner.exe
cp "$base/win64/modules/gui/qt/.libs/test_thread_runner.exe" "$work/test_thread_runner.exe"
