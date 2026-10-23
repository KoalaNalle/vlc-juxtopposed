#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/c/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-timeline-thumbnails-20261021"
qt="$base/contrib/contrib-win64/qtdeclarative"
backup="$work/qt-scrollbar-original-r93"
test ! -e "$backup"
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
exec > "$work/build-qt-scrollbar-r93.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/build-qt-scrollbar-r93-exit.txt"; exit "$code"' EXIT
mkdir -p "$backup"
cp "$qt/src/quicktemplates/qquickscrollbar.cpp" "$backup/qquickscrollbar.cpp"
cp "$qt/vlc_build/lib/libQt6QuickTemplates2.a" "$backup/build-libQt6QuickTemplates2.a"
cp "$base/contrib/x86_64-w64-mingw32/lib/libQt6QuickTemplates2.a" "$backup/installed-libQt6QuickTemplates2.a"
sha256sum "$backup/"* > "$backup/sha256.txt"
cd "$qt"
patch --dry-run -p1 < "$repo/contrib/src/qtdeclarative/0003-ScrollBar-keep-accessibility-action-connections-unique.patch"
patch -p1 < "$repo/contrib/src/qtdeclarative/0003-ScrollBar-keep-accessibility-action-connections-unique.patch"
ninja -C vlc_build -j2 lib/libQt6QuickTemplates2.a
cp vlc_build/lib/libQt6QuickTemplates2.a "$base/contrib/x86_64-w64-mingw32/lib/libQt6QuickTemplates2.a"
sha256sum src/quicktemplates/qquickscrollbar.cpp vlc_build/lib/libQt6QuickTemplates2.a "$base/contrib/x86_64-w64-mingw32/lib/libQt6QuickTemplates2.a" > "$work/qt-scrollbar-patched-r93-sha256.txt"
