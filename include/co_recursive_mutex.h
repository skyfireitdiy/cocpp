#pragma once

#include "co_nocopy.h"
#include "co_spinlock.h"

class co_ctx;

class co_recursive_mutex final : public co_nocopy
{
private:
    co_spinlock          spinlock__;
    std::atomic<co_ctx*> owner__ { nullptr }; // 当前mutex的所有者
    std::list<co_ctx*>   waited_ctx_list__;   // 当前mutex上的等待队列
    unsigned long long   lock_count__ { 0 };  // 锁定次数
public:
    void lock();
    void unlock();
    bool try_lock();
};