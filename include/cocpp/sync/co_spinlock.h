_Pragma("once");

#include "cocpp/core/co_define.h"
#include "cocpp/utils/co_noncopyable.h"

#include <atomic>

CO_NAMESPACE_BEGIN

class co_spinlock final : private co_noncopyable
{
private:
    std::atomic_flag locked__ = ATOMIC_FLAG_INIT;

public:
    void lock();
    bool try_lock();
    void unlock();
};

CO_NAMESPACE_END