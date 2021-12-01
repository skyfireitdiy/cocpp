_Pragma("once");

#include <atomic>
#include <list>

#include "co_define.h"
#include "co_noncopyable.h"
#include "co_spinlock.h"

CO_NAMESPACE_BEGIN

class co_ctx;
class co_mutex : private co_noncopyable
{
    co_ctx*            owner__ { nullptr };
    co_spinlock        spinlock__;
    std::list<co_ctx*> wait_list__;

public:
    void lock();
    void unlock();
    bool try_lock();
};

CO_NAMESPACE_END