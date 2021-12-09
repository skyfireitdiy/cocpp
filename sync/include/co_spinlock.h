_Pragma("once");

#include "co_define.h"
#include "co_noncopyable.h"

#include <atomic>

CO_NAMESPACE_BEGIN

class co_spinlock final : private co_noncopyable
{
private:
    const bool        need_schedule__ { true };
    std::atomic<bool> locked__ { false };

public:
    co_spinlock(bool need_schedule = true);
    void lock();
    bool try_lock();
    void unlock();
};

CO_NAMESPACE_END