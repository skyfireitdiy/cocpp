_Pragma("once");

#include "co_define.h"
#include "co_noncopyable.h"
#include "co_spinlock.h"

#include <deque>
#include <mutex>
#include <unordered_set>

CO_NAMESPACE_BEGIN

class co_ctx;

class co_shared_mutex : private co_noncopyable
{
private:
    enum class lock_type
    {
        unique,
        shared
    };

    struct shared_lock_context
    {
        lock_type type;
        co_ctx*   ctx;

        bool operator==(const shared_lock_context& other) const;
    };

    struct lock_context_hasher
    {
        std::size_t operator()(const shared_lock_context& other) const;
    };

    co_spinlock                                                  spinlock__;
    std::deque<shared_lock_context>                              wait_deque__;
    std::unordered_set<shared_lock_context, lock_context_hasher> owners__;

    void wake_up_waiters__();

public:
    void lock();
    void unlock();
    bool try_lock();
    void lock_shared();
    void unlock_shared();
    bool try_lock_shared();
};

CO_NAMESPACE_END