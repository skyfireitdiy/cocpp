_Pragma("once");

#include "cocpp/core/co_define.h"
#include "cocpp/sync/co_spinlock.h"
#include "cocpp/utils/co_noncopyable.h"

#include <deque>

CO_NAMESPACE_BEGIN

class co_ctx;

class co_recursive_mutex final : private co_noncopyable_with_move
{
private:
    co_spinlock         spinlock__;
    co_ctx*             owner__ { nullptr };
    std::deque<co_ctx*> wait_deque__;
    unsigned long long  lock_count__ { 0 };

public:
    co_recursive_mutex() = default;
    co_recursive_mutex(co_recursive_mutex&& other) noexcept;
    co_recursive_mutex& operator=(co_recursive_mutex&& other) noexcept;
    void lock();
    void unlock();
    bool try_lock();
};

CO_NAMESPACE_END