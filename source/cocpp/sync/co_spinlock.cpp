#include "cocpp/sync/co_spinlock.h"
#include "cocpp/core/co_define.h"
#include "cocpp/interface/co_this_co.h"
#include <thread>

CO_NAMESPACE_BEGIN

thread_local int co_spinlock::lock_ctl_depth__ = 0;

void co_spinlock::lock()
{
    ++lock_ctl_depth__;
    CoDefer([] { --lock_ctl_depth__; });
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
}

bool co_spinlock::try_lock()
{
    ++lock_ctl_depth__;
    CoDefer([] { --lock_ctl_depth__; });
    bool lk = false;
    return locked__.compare_exchange_strong(lk, true);
}

void co_spinlock::unlock()
{
    ++lock_ctl_depth__;
    CoDefer([] { --lock_ctl_depth__; });
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
}

co_spinlock::co_spinlock(lock_type lt)
    : lock_type__(lt)
{
}

bool co_spinlock::can_interrupt()
{
    return lock_ctl_depth__ == 0;
}

CO_NAMESPACE_END