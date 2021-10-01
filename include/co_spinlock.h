#pragma once

#include <atomic>
#include <list>

class co_spinlock
{
    std::atomic<bool>               token_locked__;
    std::list<unsigned long long*>  req_list__;
    std::atomic<unsigned long long> token__;

    void lock_index__();
    void unlock_index__();

public:
    void lock();
    void unlock();
};