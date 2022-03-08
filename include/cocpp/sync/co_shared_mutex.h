_Pragma("once");

#include "cocpp/core/co_define.h"
#include "cocpp/sync/co_spinlock.h"
#include "cocpp/utils/co_noncopyable.h"

#include <deque>
#include <set>

CO_NAMESPACE_BEGIN

class co_ctx;

class co_shared_mutex : private co_noncopyable
{
private:
    enum class lock_type
    {
        unique,
        shared,
        unlocked,
    };

    struct shared_lock_context
    {
        lock_type type;
        co_ctx*   ctx;
    };

    co_spinlock                     spinlock__;
    lock_type                       lock_type__ { lock_type::unlocked };
    std::deque<shared_lock_context> wait_deque__;
    std::set<co_ctx*>               owners__;
    void                            wake_up_waiters__();

public:
    void lock();
    void unlock();
    bool try_lock();
    void lock_shared();
    void unlock_shared();
    bool try_lock_shared();
};

CO_NAMESPACE_END