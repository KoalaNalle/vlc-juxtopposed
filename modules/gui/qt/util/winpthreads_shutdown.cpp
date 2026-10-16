// SPDX-License-Identifier: GPL-2.0-or-later
// MinGW Qt module compatibility for statically linked winpthreads.
#include <windows.h>
#include <pthread.h>

extern "C" __declspec(dllimport) BOOLEAN NTAPI RtlDllShutdownInProgress();
extern "C" int __real_pthread_key_delete(pthread_key_t key);

extern "C" int __wrap_pthread_key_delete(pthread_key_t key)
{
    // ExitProcess has killed the other threads before running DLL destructors.
    // One can have held winpthreads' TLS reader lock while executing a callback.
    // Taking its writer lock here would deadlock; the OS reclaims process TLS.
    // Ordinary key deletion and explicit FreeLibrary still unregister callbacks
    // through the real implementation before their code can be unloaded.
    if (RtlDllShutdownInProgress())
        return 0;
    return __real_pthread_key_delete(key);
}
