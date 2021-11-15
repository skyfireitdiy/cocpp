#pragma once

#include <atomic>
#include <list>

#include "co_define.h"
#include "co_nocopy.h"

CO_NAMESPACE_BEGIN
class co_spinlock : private co_nocopy
{
    std::atomic<bool> locked__ { false };

public:
    void lock();
    void unlock();
    bool try_lock();
};

CO_NAMESPACE_END