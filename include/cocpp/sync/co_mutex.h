_Pragma("once");

#include <atomic>
#include <deque>

#include "cocpp/core/co_define.h"
#include "cocpp/sync/co_spinlock.h"
#include "cocpp/utils/co_noncopyable.h"

CO_NAMESPACE_BEGIN

class co_ctx;
class co_mutex : private co_noncopyable_with_move
{
private:
    co_ctx*             owner__ { nullptr };
    co_spinlock         spinlock__;
    std::deque<co_ctx*> wait_deque__;

public:
    co_mutex() = default;
    co_mutex(co_mutex&& other) noexcept;
    co_mutex& operator=(co_mutex&& other) noexcept;
    void      lock();
    void      unlock();
    bool      try_lock();
};

CO_NAMESPACE_END