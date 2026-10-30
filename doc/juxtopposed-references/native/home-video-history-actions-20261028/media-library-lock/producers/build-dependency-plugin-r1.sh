#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/c/Modding/vlc-juxtopposed
review="$repo/build-windows/beta-home-sections-20261028/ml-lock-review"
dep="$base/contrib/contrib-win64/medialibrary"
depbuild="$dep/vlc_build"
archive="$base/contrib/x86_64-w64-mingw32/lib/libmedialibrary.a"
modules="$base/win64/modules"
plugin="$modules/.libs/libmedialibrary_plugin.dll"
patchfile="$repo/contrib/src/medialibrary/swmr-priority-wake.patch"
out="$review/dependency-plugin-r1"
test ! -e "$out"
mkdir "$out"
exec > "$out/build.log" 2>&1
locked=0
phase=preflight
finish()
{
    code=$?
    trap - EXIT
    printf '%s\n' "$phase" > "$out/final-phase.txt"
    if test "$locked" = 1; then
        date -u +%Y-%m-%dT%H:%M:%SZ > "$out/lock-released.txt"
        flock -u 9
    fi
    printf '%s\n' "$code" > "$out/exit.txt"
    exit "$code"
}
trap finish EXIT
exec 9> "$base/.vlc-build.lock"
flock -n 9
locked=1
date -u +%Y-%m-%dT%H:%M:%SZ > "$out/lock-acquired.txt"
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
unset CC CXX
cd "$repo"
sha256sum "$review/build-dependency-plugin-r1.sh" > "$out/producer.sha256"
git rev-parse HEAD > "$out/commit.txt"
git diff -- contrib/src/medialibrary/rules.mak > "$out/tracked-recipe.patch"
cp "$patchfile" "$out/swmr-priority-wake.patch"
sha256sum "$repo/contrib/src/medialibrary/rules.mak" "$patchfile" > "$out/recipe-inputs.sha256"
test "$(sha256sum "$dep/src/utils/SWMRLock.cpp" | cut -d' ' -f1)" = f286e33baa44a70253d654f835b713adcccb0246fbee02b0dfc2895005d5090d
test "$(sha256sum "$dep/src/utils/SWMRLock.h" | cut -d' ' -f1)" = bbb38e67682e8754f4709a85411134a47f274c9bd1aad5d14a3686710b0b60c8
cmp "$depbuild/src/libmedialibrary.a" "$archive"
mkdir "$out/original" "$out/candidate"
cp "$dep/src/utils/SWMRLock.cpp" "$out/original/"
cp "$dep/src/utils/SWMRLock.h" "$out/original/"
cp "$archive" "$out/original/libmedialibrary.a"
cp "$plugin" "$out/original/libmedialibrary_plugin.dll"
cp "$modules/libmedialibrary_plugin.la" "$out/original/libmedialibrary_plugin.la"
sha256sum "$dep/src/utils/SWMRLock.cpp" "$dep/src/utils/SWMRLock.h" "$depbuild/src/libmedialibrary.a" "$archive" "$plugin" "$modules/libmedialibrary_plugin.la" > "$out/original-active.sha256"
sha256sum "$out/original/"* > "$out/original-backups.sha256"
stat -c '%h %s %y %n' "$archive" "$plugin" > "$out/original-stat.txt"
x86_64-w64-mingw32-g++ --version > "$out/compiler.txt"
meson --version > "$out/meson-version.txt"
x86_64-w64-mingw32-objdump -d --disassemble=_ZN12medialibrary5utils8SWMRLock11unlock_readEv "$plugin" > "$out/original-unlock-read.asm.txt"
grep -q '_ZNSt18condition_variable10notify_oneEv' "$out/original-unlock-read.asm.txt"

# These sources feed only the library plugin. The parent's active Qt/Core/test
# source edits are deliberately outside this inventory and outside our targets.
git ls-files -z -- modules/misc/medialibrary include | xargs -0 sha256sum > "$out/plugin-inputs.sha256"
sha256sum "$modules/Makefile" "$base/win64/config.h" "$base/win64/src/libvlccore.la" "$base/win64/src/.libs/libvlccore.dll" "$base/win64/modules/gui/qt/.libs/libqt_plugin.dll" "$base/win64/modules/gui/qt/.libs/test_qt_home_activity.exe" >> "$out/plugin-inputs.sha256"
find "$modules/misc/medialibrary" -type f \( -name '*.lo' -o -name '*.o' \) -print0 | sort -z | xargs -0 sha256sum > "$out/plugin-objects-before.sha256"

phase=patch-dependency
printf '%s\n' "$phase" > "$out/phase.txt"
patch --fuzz=0 --dry-run -p1 -d "$dep" < "$patchfile"
patch --fuzz=0 -p1 -d "$dep" < "$patchfile"
cmp "$dep/src/utils/SWMRLock.cpp" "$review/candidate/utils/SWMRLock.cpp"
test "$(sha256sum "$dep/src/utils/SWMRLock.cpp" | cut -d' ' -f1)" = 57a6cfd0d6fa5fab4d9c17e4301c8016cc7bc73b3880880ef930b501504c404e
find "$dep/src" "$dep/include" -type f -print0 | sort -z | xargs -0 sha256sum > "$out/dependency-inputs.sha256"
sha256sum "$depbuild/build.ninja" "$depbuild/config.h" >> "$out/dependency-inputs.sha256"

phase=build-dependency
printf '%s\n' "$phase" > "$out/phase.txt"
ninja -C "$depbuild" -n src/libmedialibrary.a > "$out/dependency-plan.txt"
timeout --foreground 900 ninja -C "$depbuild" -j2 -v src/libmedialibrary.a > "$out/dependency-build.log" 2>&1
phase=install-dependency
printf '%s\n' "$phase" > "$out/phase.txt"
meson install -C "$depbuild" --no-rebuild --only-changed --dry-run > "$out/dependency-install-plan.txt"
timeout --foreground 180 meson install -C "$depbuild" --no-rebuild --only-changed > "$out/dependency-install.log" 2>&1
cmp "$depbuild/src/libmedialibrary.a" "$archive"
test "$(sha256sum "$archive" | cut -d' ' -f1)" != "$(sha256sum "$out/original/libmedialibrary.a" | cut -d' ' -f1)"
sha256sum "$depbuild/src/libmedialibrary.a" "$archive" > "$out/candidate-archive.sha256"

phase=relink-plugin
printf '%s\n' "$phase" > "$out/phase.txt"
# This target has no concrete static-archive prerequisite. Its exact old marker
# is already preserved; removing that single marker forces a real plugin link.
test -f "$modules/libmedialibrary_plugin.la"
rm -- "$modules/libmedialibrary_plugin.la"
make -C "$modules" -n -j2 V=1 libmedialibrary_plugin.la > "$out/plugin-plan.txt"
timeout --foreground 900 make -C "$modules" -j2 V=1 libmedialibrary_plugin.la > "$out/plugin-build.log" 2>&1
test -s "$modules/libmedialibrary_plugin.la"
test -s "$plugin"
grep -q -- '-lmedialibrary' "$out/plugin-build.log"
x86_64-w64-mingw32-objdump -d --disassemble=_ZN12medialibrary5utils8SWMRLock11unlock_readEv "$plugin" > "$out/candidate-unlock-read.asm.txt"
grep -q '_ZNSt18condition_variable10notify_allEv' "$out/candidate-unlock-read.asm.txt"
if grep -q '_ZNSt18condition_variable10notify_oneEv' "$out/candidate-unlock-read.asm.txt"; then
    printf '%s\n' 'Unexpected old notify_one in candidate unlock_read' >&2
    exit 3
fi
test "$(sha256sum "$plugin" | cut -d' ' -f1)" != "$(sha256sum "$out/original/libmedialibrary_plugin.dll" | cut -d' ' -f1)"
cp "$plugin" "$out/candidate/libmedialibrary_plugin.dll"
cp "$modules/libmedialibrary_plugin.la" "$out/candidate/libmedialibrary_plugin.la"
sha256sum "$out/candidate/"* "$plugin" > "$out/candidate-plugin.sha256"

phase=verify
printf '%s\n' "$phase" > "$out/phase.txt"
sha256sum -c "$out/producer.sha256" > "$out/producer-verification.txt"
sha256sum -c "$out/recipe-inputs.sha256" > "$out/recipe-verification.txt"
sha256sum -c "$out/dependency-inputs.sha256" > "$out/dependency-verification.txt"
sha256sum -c "$out/plugin-inputs.sha256" > "$out/plugin-input-verification.txt"
sha256sum -c "$out/plugin-objects-before.sha256" > "$out/plugin-object-verification.txt"
sha256sum -c "$out/original-backups.sha256" > "$out/backup-verification.txt"
phase=complete
printf '%s\n' "$phase" > "$out/phase.txt"
date -u +%Y-%m-%dT%H:%M:%SZ > "$out/completed.txt"
