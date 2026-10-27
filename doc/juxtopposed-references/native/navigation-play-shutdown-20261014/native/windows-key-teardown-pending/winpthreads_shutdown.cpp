// SPDX-License-Identifier: GPL-2.0-or-later
// Link the MinGW Qt DLL with --wrap=pthread_key_delete.
#include <windows.h>
#include <pthread.h>

extern "C" __declspec(dllimport) BOOLEAN NTAPI RtlDllShutdownInProgress();
extern "C" int __real_pthread_key_delete(pthread_key_t key);

extern "C" int __wrap_pthread_key_delete(pthread_key_t key)
{
    // ExitProcess has already killed the other threads. A key destructor can
    // still hold winpthreads' reader lock, so deleting a key in DLL detach
    // would wait forever. The OS reclaims the process's TLS storage here.
    // Explicit FreeLibrary and all normal key operations still use the real
    // function; they must unregister callbacks before their code is unloaded.
    if (RtlDllShutdownInProgress())
        return 0;
    return __real_pthread_key_delete(key);
}
