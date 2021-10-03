#include "co_spinlock.h"

#include "co.h"

void co_spinlock::lock()
{
    bool lock = false;
    while (!locked__.compare_exchange_strong(lock, true))
    {
        lock = false;
        co::schedule_switch();
    }
}

void co_spinlock::unlock()
{
    bool lock = true;
    while (!locked__.compare_exchange_strong(lock, false))
    {
        lock = true;
        co::schedule_switch();
    }
}

bool co_spinlock::try_lock()
{
    bool lock = false;
    return locked__.compare_exchange_strong(lock, true);
}