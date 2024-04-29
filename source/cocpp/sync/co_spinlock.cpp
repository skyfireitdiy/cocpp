#include "cocpp/sync/co_spinlock.h"
#include "cocpp/core/co_define.h"
#include "cocpp/core/co_env.h"
#include "cocpp/core/co_manager.h"
#include <atomic>
#include <thread>

using namespace std;

CO_NAMESPACE_BEGIN

void co_spinlock::lock()
{
    while (locked__.test_and_set(memory_order::acq_rel))
    {
        CoYield();
    }
}

bool co_spinlock::try_lock()
{
    return !locked__.test_and_set(memory_order::acq_rel);
}

void co_spinlock::unlock()
{
    locked__.clear(memory_order::release);
}

CO_NAMESPACE_END
