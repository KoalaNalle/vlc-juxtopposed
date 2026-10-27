// SPDX-License-Identifier: GPL-2.0-or-later
#include <windows.h>
#include <cstdlib>
int main(int argc,char** argv) {
    if (argc!=3) return 40;
    const auto module=LoadLibraryA(argv[1]);
    if (!module) return 41;
    const auto run=reinterpret_cast<int(*)(int)>(GetProcAddress(module,"run_native_key_probe"));
    if (!run) return 42;
    const auto mode=std::atoi(argv[2]);
    const auto result=run(mode);
    if (result!=0) return result;
    if (mode==1 && !FreeLibrary(module)) return 43;
    return 0;
}
