_Pragma("once");

#include "cocpp/core/co_define.h"
#include "cocpp/utils/co_noncopyable.h"

#include <atomic>

CO_NAMESPACE_BEGIN

class co_spinlock final : private co_noncopyable_with_move
{
private:
    std::atomic_flag locked__ = ATOMIC_FLAG_INIT;

public:
    co_spinlock() = default;
    co_spinlock(co_spinlock&& other) noexcept;
    co_spinlock& operator=(co_spinlock&& other) noexcept;
    void         lock();
    bool         try_lock();
    void         unlock();
};

CO_NAMESPACE_END