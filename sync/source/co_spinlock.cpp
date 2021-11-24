#include "co_spinlock.h"
#include "co.h"
#include "co_define.h"
#include "co_this_co.h"

CO_NAMESPACE_BEGIN

void co_spinlock::lock()
{
    // CO_O_DEBUG("spinlock start lock: %d", (bool)locked__);
    bool lock = false;
    while (!locked__.compare_exchange_strong(lock, true))
    {
        this_co::yield();
        co::current_env()->reset_safepoint();
        lock = false;
    }
    // CO_O_DEBUG("spinlock locked: %d", (bool)locked__);
}

void co_spinlock::unlock()
{
    // CO_O_DEBUG("spinlock start unlock: %d", (bool)locked__);
    bool lock = true;
    while (!locked__.compare_exchange_strong(lock, false))
    {
        this_co::yield();
        co::current_env()->reset_safepoint();
        lock = true;
    }
    // CO_O_DEBUG("spinlock unlocked: %d", (bool)locked__);
}

bool co_spinlock::try_lock()
{
    // CO_O_DEBUG("spinlock try lock: %d", (bool)locked__);
    bool lock = false;
    if (locked__.compare_exchange_strong(lock, true))
    {
        // CO_O_DEBUG("spinlock try lock succeed: %d", (bool)locked__);
        return true;
    }
    else
    {
        // CO_O_DEBUG("spinlock try lock failed: %d", (bool)locked__);
        return false;
    }
}

CO_NAMESPACE_END