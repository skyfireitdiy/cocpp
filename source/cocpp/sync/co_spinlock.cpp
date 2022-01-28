#include "cocpp/sync/co_spinlock.h"
#include "cocpp/core/co_define.h"
#include "cocpp/interface/co_this_co.h"
#include <thread>

CO_NAMESPACE_BEGIN

void co_spinlock::lock()
{
    while (locked__.test_and_set(std::memory_order_acquire))
    {
        std::this_thread::yield();
    }
}

bool co_spinlock::try_lock()
{
    return !locked__.test_and_set(std::memory_order_acquire);
}

void co_spinlock::unlock()
{
    locked__.clear(std::memory_order_release);
}

CO_NAMESPACE_END