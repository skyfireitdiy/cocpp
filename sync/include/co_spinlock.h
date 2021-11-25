#pragma once

#include <atomic>
#include <list>

#include "co_define.h"
#include "co_noncopyable.h"

CO_NAMESPACE_BEGIN
class co_spinlock : private co_noncopyable
{
    std::atomic<bool> locked__ { false };

public:
    void lock();
    void unlock();
    bool try_lock();
};

CO_NAMESPACE_END