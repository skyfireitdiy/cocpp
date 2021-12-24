_Pragma("once");

#include "co_define.h"
#include "co_noncopyable.h"
#include "co_spinlock.h"

#include <deque>

CO_NAMESPACE_BEGIN

class co_ctx;

class co_recursive_mutex final : private co_noncopyable
{
private:
    co_spinlock         spinlock__;          // 互斥锁
    co_ctx*             owner__ { nullptr }; // 当前mutex的所有者
    std::deque<co_ctx*> wait_deque__;        // 等待队列
    unsigned long long  lock_count__ { 0 };  // 锁定次数
public:                                      //
    void lock();                             // 加锁
    void unlock();                           // 解锁
    bool try_lock();                         // 尝试加锁
};

CO_NAMESPACE_END