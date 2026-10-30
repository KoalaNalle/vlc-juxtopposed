// Exercises the actual medialibrary SWMRLock.cpp and SWMRLock.h.
// Compile with the deterministic include directory first for a fixed legal
// notify_one choice, or omit it to stress the real configured Windows CV.
// Test-only private visibility reads counters under the real mutex and permits
// an explicitly reported rescue wake after a failed deadline. No layout edit.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "compat/ConditionVariable.h"
#include "compat/Mutex.h"
#include "compat/Thread.h"
#include <vector>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <thread>
#define private public
#include "utils/SWMRLock.h"
#undef private
// One translation unit keeps the test-visible class definition consistent.
// Compile this harness alone: do not additionally compile or link SWMRLock.cpp.
#include "utils/SWMRLock.cpp"

using namespace std::chrono;
using medialibrary::utils::SWMRLock;

template<typename Predicate>
bool waitUntil(Predicate predicate, milliseconds limit = milliseconds(2000))
{
    const auto deadline = steady_clock::now() + limit;
    do {
        if (predicate()) return true;
        std::this_thread::sleep_for(milliseconds(1));
    } while (steady_clock::now() < deadline);
    return predicate();
}

[[noreturn]] void setupFailure(const char *stage)
{
    std::fprintf(stderr, "setup or rescue timeout: %s\n", stage);
    std::fflush(stderr);
    std::_Exit(2); // owned test process only, never a VLC/background job
}

bool runTrial(unsigned trial)
{
    SWMRLock lock;
    std::atomic_bool priorityHeld{false}, allowWrite{false};
    std::atomic_bool writerDone{false}, readerDone{false};
    lock.lock_read(); // Existing reader A is the last active lock owner.
    std::thread writer([&] {
        lock.acquire_priority_access();
        priorityHeld = true;
        if (!waitUntil([&] { return allowWrite.load(); })) setupFailure("allow writer");
        lock.lock_write();
        lock.unlock_write();
        lock.release_priority_access();
        writerDone = true;
    });
    if (!waitUntil([&] { return priorityHeld.load(); })) setupFailure("priority held");

    // B must enter the shared wait queue before W. B cannot acquire a read
    // lock because W already holds priority, although A is still reading.
    std::thread reader([&] {
        lock.lock_read();
        lock.unlock_read();
        readerDone = true;
    });
    if (!waitUntil([&] {
        std::lock_guard<medialibrary::compat::Mutex> guard(lock.m_lock);
        return lock.m_nbReader == 1 && lock.m_nbReaderWaiting == 1;
    })) setupFailure("reader B queued");

    allowWrite = true;
    if (!waitUntil([&] {
        std::lock_guard<medialibrary::compat::Mutex> guard(lock.m_lock);
        return lock.m_nbReader == 1 && lock.m_nbReaderWaiting == 1
            && lock.m_nbWriterWaiting == 1 && !lock.m_writing;
    })) setupFailure("priority writer W queued");

    lock.unlock_read(); // Only changed production line is exercised here.
    const bool completedWithoutRescue = waitUntil([&] {
        return writerDone.load() && readerDone.load();
    }, milliseconds(500));
    if (!completedWithoutRescue)
    {
        std::lock_guard<medialibrary::compat::Mutex> guard(lock.m_lock);
        std::fprintf(stderr,
            "trial %u: stranded waiters: readers=%u readers_waiting=%u writing=%d writers_waiting=%u priority_owners=%zu\n",
            trial, lock.m_nbReader, lock.m_nbReaderWaiting, int(lock.m_writing),
            lock.m_nbWriterWaiting, lock.m_priorityAccessOwners.size());
        lock.m_cond.notify_all(); // Diagnostic cleanup; cannot count as pass.
    }
    if (!waitUntil([&] { return writerDone.load() && readerDone.load(); }))
        setupFailure("post-failure rescue");
    writer.join();
    reader.join();
    if (!completedWithoutRescue)
        std::fprintf(stderr, "trial %u: FAIL (explicit rescue used)\n", trial);
    return completedWithoutRescue;
}

int main(int argc, char **argv)
{
    const unsigned trials = argc > 1 ? unsigned(std::strtoul(argv[1], nullptr, 10)) : 1;
    if (trials == 0 || trials > 10000) return 2;
    for (unsigned trial = 1; trial <= trials; ++trial)
        if (!runTrial(trial)) return 1;
    std::printf("PASS: %u trials, no rescue wake used\n", trials);
    return 0;
}
