#include "co_spinlock.h"
#include "co_define.h"
#include "co_this_co.h"

CO_NAMESPACE_BEGIN

void co_spinlock::lock()
{
    bool lk = false;
    while (!locked__.compare_exchange_strong(lk, true))
    {
        this_co::yield();
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
        this_co::yield();
        lk = true;
    }
}

CO_NAMESPACE_END