_Pragma("once");

#include <atomic>
#include <deque>

#include "co_define.h"
#include "co_noncopyable.h"
#include "co_spinlock.h"

CO_NAMESPACE_BEGIN

class co_ctx;
class co_mutex : private co_noncopyable
{
    co_ctx*             owner__ { nullptr };
    co_spinlock         spinlock__;
    std::deque<co_ctx*> wait_deque__;

public:
    void lock();
    void unlock();
    bool try_lock();
};

CO_NAMESPACE_END