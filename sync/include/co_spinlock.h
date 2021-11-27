_Pragma("once");

#include <atomic>
#include <list>

#include "co_define.h"
#include "co_noncopyable.h"

CO_NAMESPACE_BEGIN

class co_ctx;
class co_spinlock : private co_noncopyable
{
    std::atomic<co_ctx*> locked__ { nullptr };

public:
    void lock();
    void unlock();
    bool try_lock();
};

CO_NAMESPACE_END