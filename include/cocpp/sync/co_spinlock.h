_Pragma("once");

#include "cocpp/core/co_define.h"
#include "cocpp/utils/co_noncopyable.h"

#include <atomic>

CO_NAMESPACE_BEGIN

class co_spinlock final : private co_noncopyable
{
private:
    std::atomic_flag locked__ = ATOMIC_FLAG_INIT; // 是否锁定
public:
    void lock();     // 加锁
    bool try_lock(); // 尝试加锁
    void unlock();   // 解锁
};

CO_NAMESPACE_END