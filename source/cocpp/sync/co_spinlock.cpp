#include "cocpp/sync/co_spinlock.h"
#include "cocpp/core/co_define.h"
#include "cocpp/interface/co_this_co.h"
#include <thread>

CO_NAMESPACE_BEGIN

void co_spinlock::lock()
{
    std::atomic_thread_fence(std::memory_order_acquire);
    bool lk = false;
    while (!locked__.compare_exchange_strong(lk, true))
    {
        if (lock_type__ == lock_type::in_coroutine)
        {
            this_co::yield();
        }
        else
        {
            std::this_thread::yield();
        }
        lk = false;
    }
    std::atomic_thread_fence(std::memory_order_release);
}

bool co_spinlock::try_lock()
{
    std::atomic_thread_fence(std::memory_order_acquire);
    bool lk  = false;
    auto ret = locked__.compare_exchange_strong(lk, true);
    std::atomic_thread_fence(std::memory_order_release);
    return ret;
}

void co_spinlock::unlock()
{
    std::atomic_thread_fence(std::memory_order_acquire);
    bool lk = true;
    while (!locked__.compare_exchange_strong(lk, false))
    {
        if (lock_type__ == lock_type::in_coroutine)
        {
            this_co::yield();
        }
        else
        {
            std::this_thread::yield();
        }
        lk = true;
    }
    std::atomic_thread_fence(std::memory_order_release);
}

co_spinlock::co_spinlock(lock_type lt)
    : lock_type__(lt)
{
}

CO_NAMESPACE_END