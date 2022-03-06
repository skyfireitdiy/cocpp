#include "cocpp/sync/co_spinlock.h"
#include "cocpp/core/co_define.h"
#include "cocpp/core/co_env.h"
#include "cocpp/core/co_manager.h"
#include <atomic>
#include <thread>

CO_NAMESPACE_BEGIN

void co_spinlock::lock()
{
    while (locked__.test_and_set(std::memory_order::acquire))
    {
        co_manager::instance()->current_env()->schedule_switch();
    }
}

bool co_spinlock::try_lock()
{
    return !locked__.test_and_set(std::memory_order::acquire);
}

void co_spinlock::unlock()
{
    locked__.clear(std::memory_order::release);
}

CO_NAMESPACE_END