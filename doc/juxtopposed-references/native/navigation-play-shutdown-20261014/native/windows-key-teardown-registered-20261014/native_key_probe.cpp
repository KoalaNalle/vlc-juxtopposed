// SPDX-License-Identifier: GPL-2.0-or-later
#include <windows.h>
#include <pthread.h>
#include <cerrno>
#include <cstdio>
#include <limits>

namespace {
pthread_key_t key;
bool initialized=false;
HANDLE entered=nullptr;
HANDLE release=nullptr;
struct KeyCleanup {
    ~KeyCleanup() {
        if (initialized && pthread_key_delete(key)!=0)
            ExitProcess(31);
    }
} cleanup;

void destructor(void*) {
    SetEvent(entered);
    WaitForSingleObject(release,INFINITE);
}
void* worker(void*) {
    pthread_setspecific(key,reinterpret_cast<void*>(1));
    return nullptr;
}
}

extern "C" __declspec(dllexport) int run_native_key_probe(int mode)
{
    if (mode==0) {
        pthread_key_t normal;
        if (pthread_key_create(&normal,nullptr)!=0) return 10;
        if (pthread_setspecific(normal,reinterpret_cast<void*>(7))!=0) return 11;
        if (pthread_getspecific(normal)!=reinterpret_cast<void*>(7)) return 12;
        if (pthread_key_delete(normal)!=0) return 13;
        if (pthread_key_delete(std::numeric_limits<pthread_key_t>::max())!=EINVAL) return 14;
        return 0;
    }
    if (mode==1) {
        if (pthread_key_create(&key,nullptr)!=0) return 20;
        initialized=true;
        return 0;
    }
    entered=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    release=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    if (!entered || !release || pthread_key_create(&key,destructor)!=0) return 21;
    initialized=true;
    pthread_t thread;
    if (pthread_create(&thread,nullptr,worker,nullptr)!=0) return 22;
    if (WaitForSingleObject(entered,5000)!=WAIT_OBJECT_0) return 23;
    std::puts("Native thread is inside its TLS destructor; main is returning.");
    std::fflush(stdout);
    return 0;
}
