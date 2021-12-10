_Pragma("once");

#include "co_define.h"
#include "co_noncopyable.h"

#include <atomic>

CO_NAMESPACE_BEGIN

class co_spinlock final : private co_noncopyable
{
public:
    enum class lock_type
    {
        in_coroutine,
        in_thread
    };

private:
    const lock_type   lock_type__;
    std::atomic<bool> locked__ { false };

public:
    co_spinlock(lock_type lt = lock_type::in_coroutine);
    void lock();
    bool try_lock();
    void unlock();
};

CO_NAMESPACE_END