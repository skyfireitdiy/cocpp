_Pragma("once");

#include "co_define.h"
#include "co_noncopyable.h"
#include "co_spinlock.h"

#include <list>
#include <mutex>
#include <unordered_set>

CO_NAMESPACE_BEGIN

class co_shared_mutex : private co_noncopyable
{
private:
    enum class lock_type
    {
        unique,
        shared
    };

    struct lock_context
    {
        lock_type type;
        co_ctx*   ctx;

        bool operator==(const lock_context& other) const;
    };

    struct lock_context_hasher
    {
        std::size_t operator()(const lock_context& other) const;
    };

    co_spinlock                                           spinlock__;
    std::list<lock_context>                               wait_list__;
    std::unordered_set<lock_context, lock_context_hasher> owners__;

public:
    void lock();
    void unlock();
    bool try_lock();
    void lock_shared();
    void unlock_shared();
    bool try_lock_shared();
};

CO_NAMESPACE_END