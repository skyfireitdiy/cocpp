_Pragma("once");

#include "cocpp/core/co_define.h"
#include "cocpp/sync/co_spinlock.h"
#include "cocpp/utils/co_noncopyable.h"

#include <deque>
#include <set>

CO_NAMESPACE_BEGIN

class co_ctx;

class co_shared_mutex : private co_noncopyable
{
private:
    enum class lock_type
    {
        unique,   // 唯一锁
        shared,   // 共享锁
        unlocked, // 未锁
    };

    struct shared_lock_context
    {
        lock_type type; // 锁类型
        co_ctx*   ctx;  // 当前锁的持有者
    };

    co_spinlock                     spinlock__;                          // 互斥锁
    lock_type                       lock_type__ { lock_type::unlocked }; // 锁类型
    std::deque<shared_lock_context> wait_deque__;                        // 等待队列
    std::set<co_ctx*>               owners__;                            // 持有者
    void                            wake_up_waiters__();                 // 唤醒等待队列
public:
    void lock();            // 加锁
    void unlock();          // 解锁
    bool try_lock();        // 尝试加锁
    void lock_shared();     // 加共享锁
    void unlock_shared();   // 解共享锁
    bool try_lock_shared(); // 尝试加共享锁
};

CO_NAMESPACE_END