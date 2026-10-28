// Test-only legal condition-variable scheduler. Never install with VLC.
// notify_one chooses the oldest registered waiter deterministically.
#pragma once
#include "compat/Mutex.h"
#include <condition_variable>
#include <list>

namespace medialibrary { namespace compat {
class ConditionVariable
{
    struct Waiter { std::condition_variable ready; bool notified = false; };
    std::list<Waiter *> m_waiters;
public:
    // SWMRLock calls wait/notify while holding the same external m_lock.
    // The list is protected by that lock, as are each waiter's predicates.
    template<typename Predicate>
    void wait(std::unique_lock<Mutex>& lock, Predicate predicate)
    {
        while (!predicate())
        {
            Waiter waiter;
            m_waiters.push_back(&waiter);
            waiter.ready.wait(lock, [&] { return waiter.notified; });
            m_waiters.remove(&waiter);
        }
    }
    void notify_one() noexcept
    {
        if (m_waiters.empty()) return;
        auto *waiter = m_waiters.front();
        waiter->notified = true;
        waiter->ready.notify_one();
    }
    void notify_all() noexcept
    {
        for (auto *waiter : m_waiters)
        {
            waiter->notified = true;
            waiter->ready.notify_one();
        }
    }
};
}}
