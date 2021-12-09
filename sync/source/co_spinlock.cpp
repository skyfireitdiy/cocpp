#include "co_spinlock.h"
#include "co_define.h"
#include "co_this_co.h"
#include <thread>

CO_NAMESPACE_BEGIN

void co_spinlock::lock()
{
    bool lk = false;
    while (!locked__.compare_exchange_strong(lk, true))
    {
        if (need_schedule__)
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
    bool lk = false;
    return locked__.compare_exchange_strong(lk, true);
}

void co_spinlock::unlock()
{
    bool lk = true;
    while (!locked__.compare_exchange_strong(lk, false))
    {
        if (need_schedule__)
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

co_spinlock::co_spinlock(bool need_schedule)
    : need_schedule__(need_schedule)
{
}

CO_NAMESPACE_END