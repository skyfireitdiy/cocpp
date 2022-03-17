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
    co_ctx*             owner__ { nullptr };
    co_spinlock         spinlock__;
    std::deque<co_ctx*> wait_deque__;

public:
    void lock();
    void unlock();
    bool try_lock();
};

CO_NAMESPACE_END