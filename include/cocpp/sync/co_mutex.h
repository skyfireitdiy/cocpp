_Pragma("once");

#include <atomic>
#include <deque>

#include "cocpp/core/co_define.h"
#include "cocpp/sync/co_spinlock.h"
#include "cocpp/utils/co_noncopyable.h"

CO_NAMESPACE_BEGIN

class co_ctx;
class co_mutex : private co_noncopyable
{
    co_ctx*             owner__ { nullptr }; // 持有者
    co_spinlock         spinlock__;          // 互斥锁
    std::deque<co_ctx*> wait_deque__;        // 等待队列
public:
    void lock();     // 加锁
    void unlock();   // 解锁
    bool try_lock(); // 尝试加锁
};

CO_NAMESPACE_END