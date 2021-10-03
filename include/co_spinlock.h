#pragma once

#include <atomic>
#include <list>

class co_spinlock
{
    std::atomic<bool> locked__ { false };

public:
    void lock();
    void unlock();
};