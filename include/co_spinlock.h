#pragma once

#include <atomic>
#include <list>

#include "co_nocopy.h"

class co_spinlock : public co_nocopy
{
    std::atomic<bool> locked__ { false };

public:
    void lock();
    void unlock();
    bool try_lock();
};