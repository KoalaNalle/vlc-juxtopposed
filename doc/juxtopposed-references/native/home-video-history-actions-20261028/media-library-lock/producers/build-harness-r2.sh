#!/usr/bin/env bash
set -euo pipefail
review=/mnt/c/Modding/vlc-juxtopposed/build-windows/beta-home-sections-20261028/ml-lock-review
dep=/root/vlc-juxtopposed-build/contrib/contrib-win64/medialibrary
out="$review/build-r2"
test ! -e "$out"
mkdir "$out"
exec > "$out/build.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$out/exit.txt"; exit "$code"' EXIT
sha256sum "$review/build-harness-r2.sh" "$review/run-harness-r2.py" "$review/swmr-priority-wake.patch" "$review/repository-integration.patch" "$review/swmr_priority_wake_test.cpp" "$review/deterministic/compat/ConditionVariable.h" "$review/baseline/utils/SWMRLock.cpp" "$review/baseline/utils/SWMRLock.h" "$review/candidate/utils/SWMRLock.cpp" "$review/candidate/utils/SWMRLock.h" "$dep/src/compat/Mutex.h" "$dep/src/compat/Thread.h" "$dep/src/compat/ConditionVariable.h" "$dep/vlc_build/config.h" "$dep/include/Fixup.h" > "$out/inputs.sha256"
x86_64-w64-mingw32-g++ --version > "$out/compiler.txt"
for variant in baseline candidate; do
    for scheduler in deterministic native; do
        includes=()
        if test "$scheduler" = deterministic; then
            includes+=("-I$review/deterministic")
        fi
        x86_64-w64-mingw32-g++ -std=c++14 -O2 -g -static -pthread -DHAVE_CONFIG_H=1 \
            "${includes[@]}" "-I$review/$variant" "-I$dep/src" "-I$dep/vlc_build" "-I$dep/include" \
            "$review/swmr_priority_wake_test.cpp" -o "$out/$variant-$scheduler.exe"
        sha256sum "$out/$variant-$scheduler.exe" >> "$out/binaries.sha256"
    done
done
sha256sum -c "$out/inputs.sha256"
