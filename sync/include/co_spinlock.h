_Pragma("once");

#include "co_define.h"
#include "co_noncopyable.h"

#include <atomic>

CO_NAMESPACE_BEGIN

class co_spinlock final : private co_noncopyable
{
private:
    std::atomic<bool> locked__ { false };

public:
    void lock();
    bool try_lock();
    void unlock();
};

CO_NAMESPACE_END