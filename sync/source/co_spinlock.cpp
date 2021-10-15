#include "co_spinlock.h"
#include "co.h"
#include "co_this_co.h"

CO_NAMESPACE_BEGIN

void co_spinlock::lock()
{
    bool lock = false;
    while (!locked__.compare_exchange_strong(lock, true))
    {
        lock = false;
        this_co::yield();
    }
}

void co_spinlock::unlock()
{
    bool lock = true;
    while (!locked__.compare_exchange_strong(lock, false))
    {
        lock = true;
        this_co::yield();
    }
}

bool co_spinlock::try_lock()
{
    bool lock = false;
    return locked__.compare_exchange_strong(lock, true);
}

CO_NAMESPACE_END