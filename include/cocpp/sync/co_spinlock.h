_Pragma("once");

#include "cocpp/core/co_define.h"
#include "cocpp/utils/co_noncopyable.h"

#include <atomic>

CO_NAMESPACE_BEGIN

class co_spinlock final : private co_noncopyable
{
public:
    enum class lock_type
    {
        in_coroutine, // 在协程中
        in_thread     // 在线程中
    };

private:
    static thread_local int lock_ctl_depth__;   // 当前线程的锁控制层次
    const lock_type         lock_type__;        // 锁类型
    std::atomic<bool>       locked__ { false }; // 是否锁定
public:
    co_spinlock(lock_type lt = lock_type::in_coroutine); // 构造
    void lock();                                         // 加锁
    bool try_lock();                                     // 尝试加锁
    void unlock();                                       // 解锁
    static bool can_interrupt();                                // 是否可中断
};

CO_NAMESPACE_END