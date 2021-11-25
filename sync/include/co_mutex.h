#pragma once

#include "co_ctx.h"
#include "co_define.h"
#include "co_noncopyable.h"
#include "co_spinlock.h"
#include <atomic>
#include <list>

CO_NAMESPACE_BEGIN

class co_ctx;

class co_mutex : private co_noncopyable
{
private:
    co_spinlock          spinlock__;
    std::atomic<co_ctx*> owner__ { nullptr }; // 当前mutex的所有者
    std::list<co_ctx*>   waited_ctx_list__;   // 当前mutex上的等待队列

public:
    void lock();
    void unlock();
    bool try_lock();
};

CO_NAMESPACE_END